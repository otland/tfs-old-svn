//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#include "otpch.h"

#include <iostream>

#if defined __WINDOWS__ || defined WIN32
#include <winsock2.h>
#endif

#include "database.h"
#include "databasemysql.h"
#ifdef __MYSQL_ALT_INCLUDE__
#include "errmsg.h"
#else
#include <mysql/errmsg.h>
#endif

#include "configmanager.h"
#include "scheduler.h"
extern ConfigManager g_config;

DatabaseMySQL::DatabaseMySQL()
{
	m_connected = false;
	if(!mysql_init(&m_handle))
	{
		std::cout << "Failed to initialize MySQL connection handler." << std::endl;
		return;
	}

	my_bool reconnect = true;
	mysql_options(&m_handle, MYSQL_OPT_RECONNECT, &reconnect);
	if(!mysql_real_connect(&m_handle, g_config.getString(ConfigManager::SQL_HOST).c_str(), g_config.getString(ConfigManager::SQL_USER).c_str(), g_config.getString(ConfigManager::SQL_PASS).c_str(), g_config.getString(ConfigManager::SQL_DB).c_str(), g_config.getNumber(ConfigManager::SQL_PORT), NULL, 0))
	{
		std::cout << "Failed connecting to database. MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		return;
	}

	if(MYSQL_VERSION_ID < 50019)
	{
		//MySQL servers < 5.0.19 has a bug where MYSQL_OPT_RECONNECT is (incorrectly) reset by mysql_real_connect calls
		//See http://dev.mysql.com/doc/refman/5.0/en/mysql-options.html for more information.
		std::cout << "[Warning] Outdated MySQL server detected. Consider upgrading to a newer version." << std::endl;
	}

#ifndef __DISABLE_DIRTY_RECONNECT__
	m_attempts = 0;
#endif
	m_connected = true;
	uint32_t keepAlive = g_config.getNumber(ConfigManager::SQL_KEEPALIVE);
	if(keepAlive)
		Scheduler::getScheduler().addEvent(createSchedulerTask((keepAlive * 1000), boost::bind(&DatabaseMySQL::keepAlive, this)));
}

DatabaseMySQL::~DatabaseMySQL()
{
	mysql_close(&m_handle);
}

bool DatabaseMySQL::getParam(DBParam_t param)
{
	switch(param)
	{
		case DBPARAM_MULTIINSERT:
			return true;
			break;
		default:
			return false;
			break;
	}
}

bool DatabaseMySQL::beginTransaction()
{
	return executeQuery("BEGIN");
}

bool DatabaseMySQL::rollback()
{
	if(!m_connected)
		return false;

	if(mysql_rollback(&m_handle) != 0)
	{
		std::cout << "mysql_rollback(): MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		return false;
	}

	return true;
}

bool DatabaseMySQL::commit()
{
	if(!m_connected)
		return false;

	if(mysql_commit(&m_handle) != 0)
	{
		std::cout << "mysql_commit(): MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		return false;
	}

	return true;
}

bool DatabaseMySQL::executeQuery(const std::string &query)
{
	if(!m_connected)
		return false;

	if(mysql_real_query(&m_handle, query.c_str(), query.length()) != 0)
	{
		int32_t error = mysql_errno(&m_handle);
		if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR)
		{
#ifndef __DISABLE_DIRTY_RECONNECT__
			if(reconnect())
				return executeQuery(query);
#else
			m_connected = false;
#endif
		}

		std::cout << "mysql_real_query(): " << query << ": MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		return false;

	}

	if(MYSQL_RES* m_res = mysql_store_result(&m_handle))
		mysql_free_result(m_res);

	return true;
}

DBResult* DatabaseMySQL::storeQuery(const std::string &query)
{
	if(!m_connected)
		return NULL;

	if(mysql_real_query(&m_handle, query.c_str(), query.length()) != 0)
	{
		int32_t error = mysql_errno(&m_handle);
		if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR)
		{
#ifndef __DISABLE_DIRTY_RECONNECT__
			if(reconnect())
				return storeQuery(query);
#else
			m_connected = false;
#endif
		}

		std::cout << "mysql_real_query(): " << query << ": MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		return NULL;

	}

	if(MYSQL_RES* m_res = mysql_store_result(&m_handle))
	{
		DBResult* res = (DBResult*)new MySQLResult(m_res);
		return verifyResult(res);
	}

	int32_t error = mysql_errno(&m_handle);
	if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR)
	{
#ifndef __DISABLE_DIRTY_RECONNECT__
		if(reconnect())
			return storeQuery(query);
#else
		m_connected = false;
#endif
	}

	std::cout << "mysql_store_result(): " << query << ": MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
	return NULL;
}

std::string DatabaseMySQL::escapeString(const std::string &s)
{
	return escapeBlob(s.c_str(), s.length());
}

std::string DatabaseMySQL::escapeBlob(const char* s, uint32_t length)
{
	if(!s)
		return std::string("''");

	char* output = new char[length * 2 + 1];
	mysql_real_escape_string(&m_handle, output, s, length);

	std::string res = "'";
	res += output;
	res += "'";

	delete[] output;
	return res;
}

void DatabaseMySQL::freeResult(DBResult* res)
{
	delete (MySQLResult*)res;
}

void DatabaseMySQL::keepAlive()
{
	int32_t delay = g_config.getNumber(ConfigManager::SQL_KEEPALIVE);
	if(time(NULL) > (m_lastUse + delay))
		executeQuery("SHOW TABLES;");

	Scheduler::getScheduler().addEvent(createSchedulerTask((delay * 1000), boost::bind(&DatabaseMySQL::keepAlive, this)));
}

#ifndef __DISABLE_DIRTY_RECONNECT__
bool DatabaseMySQL::reconnect()
{
	if(m_attempts > MAX_RECONNECT_ATTEMPTS)
	{
		m_connected = false;
		std::cout << "Failed reconnecting to database. MYSQL ERROR: " << mysql_error(&m_handle) << std::endl;
		return false;
	}
	else if(mysql_real_connect(&m_handle, g_config.getString(ConfigManager::SQL_HOST).c_str(), g_config.getString(ConfigManager::SQL_USER).c_str(), g_config.getString(ConfigManager::SQL_PASS).c_str(), g_config.getString(ConfigManager::SQL_DB).c_str(), g_config.getNumber(ConfigManager::SQL_PORT), NULL, 0))
		m_attempts = 0;
	else
		m_attempts++;

	return true;
}
#endif

int32_t MySQLResult::getDataInt(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(m_row[it->second] == NULL)
			return 0;
		else
			return atoi(m_row[it->second]);
	}

	std::cout << "Error during getDataInt(" << s << ")." << std::endl;
	return 0; // Failed
}

int64_t MySQLResult::getDataLong(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(m_row[it->second] == NULL)
			return 0;
		else
			return ATOI64(m_row[it->second]);
	}

	std::cout << "Error during getDataLong(" << s << ")." << std::endl;
	return 0; // Failed
}

std::string MySQLResult::getDataString(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(m_row[it->second] == NULL)
			return std::string("");
		else
			return std::string(m_row[it->second]);
	}

	std::cout << "Error during getDataString(" << s << ")." << std::endl;
	return std::string(""); // Failed
}

const char* MySQLResult::getDataStream(const std::string &s, uint64_t &size)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(m_row[it->second] == NULL)
		{
			size = 0;
			return NULL;
		}
		else
		{
			size = mysql_fetch_lengths(m_handle)[it->second];
			return m_row[it->second];
		}
	}

	std::cout << "Error during getDataStream(" << s << ")." << std::endl;
	size = 0;
	return NULL;
}

bool MySQLResult::next()
{
	m_row = mysql_fetch_row(m_handle);
	return m_row != NULL;
}

MySQLResult::MySQLResult(MYSQL_RES* res)
{
	m_handle = res;
	m_listNames.clear();

	MYSQL_FIELD* field;
	int32_t i = 0;
	while((field = mysql_fetch_field(m_handle)))
	{
		m_listNames[field->name] = i;
		i++;
	}
}

MySQLResult::~MySQLResult()
{
	mysql_free_result(m_handle);
}
