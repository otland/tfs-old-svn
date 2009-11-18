////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////
#include "otpch.h"
#include "scheduler.h"

#include "database.h"
#include "databasemysql.h"

#ifdef __MYSQL_ALT_INCLUDE__
#include "errmsg.h"
#else
#include <mysql/errmsg.h>
#endif
#include <iostream>

#include "configmanager.h"
extern ConfigManager g_config;

DatabaseMySQL::DatabaseMySQL()
{
	m_connected = false;
	if(!mysql_init(&m_handle))
	{
		std::cout << std::endl << "Failed to initialize MySQL connection handler." << std::endl;
		return;
	}

	uint32_t readTimeout = g_config.getNumber(ConfigManager::MYSQL_READ_TIMEOUT);
	if(readTimeout)
		mysql_options(&m_handle, MYSQL_OPT_READ_TIMEOUT, (const char*)&readTimeout);

	uint32_t writeTimeout = g_config.getNumber(ConfigManager::MYSQL_WRITE_TIMEOUT);
	if(writeTimeout)
		mysql_options(&m_handle, MYSQL_OPT_WRITE_TIMEOUT, (const char*)&writeTimeout);

	connect();
	if(mysql_get_client_version() <= 50019)
	{
		//MySQL servers <= 5.0.19 has a bug where MYSQL_OPT_RECONNECT is (incorrectly) reset by mysql_real_connect calls
		//See http://dev.mysql.com/doc/refman/5.0/en/mysql-options.html for more information.
		std::cout << std::endl << "> WARNING: Outdated MySQL server detected, consider upgrading to a newer version." << std::endl;
	}

	if(g_config.getBool(ConfigManager::HOUSE_STORAGE))
	{
		//we cannot lock mutex here :)
		if(DBResult* result = storeQuery("SHOW variables LIKE 'max_allowed_packet';"))
		{
			if(result->getDataLong("Value") < 16776192)
			{
				std::cout << std::endl << "> WARNING: max_allowed_packet might be set too low for binary map storage." << std::endl;
				std::cout << "Use the following query to raise max_allow_packet: SET GLOBAL max_allowed_packet = 16776192;" << std::endl;
			}

			result->free();
		}
	}

	int32_t keepAlive = g_config.getNumber(ConfigManager::SQL_KEEPALIVE);
	if(keepAlive)
		Scheduler::getInstance().addEvent(createSchedulerTask((keepAlive * 1000), boost::bind(&DatabaseMySQL::keepAlive, this)));
}

bool DatabaseMySQL::getParam(DBParam_t param)
{
	switch(param)
	{
		case DBPARAM_MULTIINSERT:
			return true;
		default:
			break;
	}

	return false;
}

bool DatabaseMySQL::rollback()
{
	if(!m_connected)
		return false;

	if(mysql_rollback(&m_handle))
	{
		std::cout << "mysql_rollback() - MYSQL ERROR: " << mysql_error(&m_handle) << " (" << mysql_errno(&m_handle) << ")" << std::endl;
		return false;
	}

	return true;
}

bool DatabaseMySQL::commit()
{
	if(!m_connected)
		return false;

	if(mysql_commit(&m_handle))
	{
		std::cout << "mysql_commit() - MYSQL ERROR: " << mysql_error(&m_handle) << " (" << mysql_errno(&m_handle) << ")" << std::endl;
		return false;
	}

	return true;
}

bool DatabaseMySQL::executeQuery(const std::string &query)
{
	if(!m_connected)
		return false;

	bool state = true;
	if(mysql_real_query(&m_handle, query.c_str(), query.length()))
	{
		int32_t error = mysql_errno(&m_handle);
		if((error == CR_UNKNOWN_ERROR || error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR) && reconnect())
			return executeQuery(query);

		state = false;
		std::cout << "mysql_real_query(): " << query << " - MYSQL ERROR: " << mysql_error(&m_handle) << " (" << error << ")" << std::endl;
	}

	if(MYSQL_RES* tmp = mysql_store_result(&m_handle))
	{
		mysql_free_result(tmp);
		tmp = NULL;
	}

	return state;
}

DBResult* DatabaseMySQL::storeQuery(const std::string &query)
{
	if(!m_connected)
		return NULL;

	int32_t error = 0;
	if(mysql_real_query(&m_handle, query.c_str(), query.length()))
	{
		error = mysql_errno(&m_handle);
		if((error == CR_UNKNOWN_ERROR || error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR) && reconnect())
			return storeQuery(query);

		std::cout << "mysql_real_query(): " << query << " - MYSQL ERROR: " << mysql_error(&m_handle) << " (" << error << ")" << std::endl;
		return NULL;

	}

	if(MYSQL_RES* tmp = mysql_store_result(&m_handle))
	{
		DBResult* res = (DBResult*)new MySQLResult(tmp);
		return verifyResult(res);
	}

	error = mysql_errno(&m_handle);
	if((error == CR_UNKNOWN_ERROR || error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR) && reconnect())
		return storeQuery(query);

	std::cout << "mysql_store_result(): " << query << " - MYSQL ERROR: " << mysql_error(&m_handle) << " (" << error << ")" << std::endl;
	return NULL;
}

std::string DatabaseMySQL::escapeBlob(const char* s, uint32_t length)
{
	if(!s)
		return "''";

	char* output = new char[length * 2 + 1];
	mysql_real_escape_string(&m_handle, output, s, length);

	std::string res = "'";
	res += output;
	res += "'";

	delete[] output;
	return res;
}

void DatabaseMySQL::keepAlive()
{
	int32_t delay = g_config.getNumber(ConfigManager::SQL_KEEPALIVE);
	if(delay)
	{
		if(time(NULL) > (m_use + delay) && mysql_ping(&m_handle))
			reconnect();

		Scheduler::getInstance().addEvent(createSchedulerTask((delay * 1000), boost::bind(&DatabaseMySQL::keepAlive, this)));
	}
}

bool DatabaseMySQL::connect()
{
	if(m_connected)
	{
		m_connected = false;
		mysql_close(&m_handle);
	}

	if(!mysql_real_connect(&m_handle, g_config.getString(ConfigManager::SQL_HOST).c_str(), g_config.getString(ConfigManager::SQL_USER).c_str(),
		g_config.getString(ConfigManager::SQL_PASS).c_str(), g_config.getString(ConfigManager::SQL_DB).c_str(), g_config.getNumber(
		ConfigManager::SQL_PORT), NULL, 0))
	{
		std::cout << "Failed connecting to database - MYSQL ERROR: " << mysql_error(&m_handle) << " (" << mysql_errno(&m_handle) << ")" << std::endl;
		return false;
	}

	m_connected = true;
	m_attempts = 0;
	return true;
}

bool DatabaseMySQL::reconnect()
{
	while(m_attempts <= MAX_RECONNECT_ATTEMPTS)
	{
		m_attempts++;
		if(connect())
			return true;
	}

	std::cout << "Unable to reconnect - too many attempts, limit exceeded!" << std::endl;
	return false;
}

int32_t MySQLResult::getDataInt(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(!m_row[it->second])
			return 0;

		return atoi(m_row[it->second]);
	}

	if(refetch())
		return getDataInt(s);

	std::cout << "Error during getDataInt(" << s << ")." << std::endl;
	return 0; // Failed
}

int64_t MySQLResult::getDataLong(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(!m_row[it->second])
			return 0;

		return atoll(m_row[it->second]);
	}

	if(refetch())
		return getDataLong(s);

	std::cout << "Error during getDataLong(" << s << ")." << std::endl;
	return 0; // Failed
}

std::string MySQLResult::getDataString(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(!m_row[it->second])
			return "";

		return std::string(m_row[it->second]);
	}

	if(refetch())
		return getDataString(s);

	std::cout << "Error during getDataString(" << s << ")." << std::endl;
	return ""; // Failed
}

const char* MySQLResult::getDataStream(const std::string &s, uint64_t &size)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		if(!m_row[it->second])
		{
			size = 0;
			return NULL;
		}

		size = mysql_fetch_lengths(m_handle)[it->second];
		return m_row[it->second];
	}

	if(refetch())
		return getDataStream(s, size);

	std::cout << "Error during getDataStream(" << s << ")." << std::endl;
	size = 0;
	return NULL; // Failed
}

void MySQLResult::free()
{
	if(m_handle)
	{
		mysql_free_result(m_handle);
		m_handle = NULL;

		m_listNames.clear();
		delete this;
	}
	else
		std::cout << "[Warning - MySQLResult::free] Trying to free already freed result." << std::endl;
}

bool MySQLResult::next()
{
	m_row = mysql_fetch_row(m_handle);
	return m_row;
}

void MySQLResult::fetch()
{
	m_listNames.clear();
	int32_t i = 0;

	MYSQL_FIELD* field;
	while((field = mysql_fetch_field(m_handle)))
		m_listNames[field->name] = i++;
}

bool MySQLResult::refetch()
{
	if(m_attempts >= MAX_REFETCH_ATTEMPTS)
		return false;

	fetch();
	++m_attempts;
	return true;
}

MySQLResult::MySQLResult(MYSQL_RES* result)
{
	m_attempts = 0;
	if(result)
	{
		m_handle = result;
		fetch();
	}
	else
		delete this;
}
