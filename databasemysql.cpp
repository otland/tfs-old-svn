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

#ifdef __USE_MYSQL__
#include "otpch.h"

#include <iostream>
#ifdef WIN32
#include <boost/asio.hpp>
#endif
#include "databasemysql.h"

#ifdef __MYSQL_ALT_INCLUDE__
#include "errmsg.h"
#else
#include <mysql/errmsg.h>
#endif

#include "configmanager.h"
extern ConfigManager g_config;

DatabaseMySQL::DatabaseMySQL()
{
	init();
}

DatabaseMySQL::~DatabaseMySQL()
{
	disconnect();
}

bool DatabaseMySQL::init()
{
	m_initialized = false;
	m_connected = false;

	// Initialize mysql
	if(mysql_init(&m_handle) == NULL)
		std::cout << "> MySQL ERROR: mysql_init" << std::endl;
	else
		m_initialized = true;

	return m_initialized;
}

bool DatabaseMySQL::connect()
{
	if(m_connected)
		return true;

	if(!m_initialized && !init())
		return false;

	// Connect to the MySQL host
	if(!mysql_real_connect(&m_handle, g_config.getString(ConfigManager::MYSQL_HOST).c_str(), g_config.getString(ConfigManager::MYSQL_USER).c_str(), g_config.getString(ConfigManager::MYSQL_PASS).c_str(), g_config.getString(ConfigManager::MYSQL_DB).c_str(), g_config.getNumber(ConfigManager::SQL_PORT), NULL, 0))
	{
		std::cout << "> MySQL ERROR mysql_real_connect: " << mysql_error(&m_handle)  << std::endl;
		return false;
	}

	if(MYSQL_VERSION_ID < 50019)
	{
		//MySQL servers < 5.0.19 has a bug where MYSQL_OPT_RECONNECT is (incorrectly) reset by mysql_real_connect calls
		//See http://dev.mysql.com/doc/refman/5.0/en/mysql-options.html for more information.
		std::cout << "[Warning] Outdated MySQL server detected. Consider upgrading to a newer version." << std::endl;
	}

	m_connected = true;
	return true;
}

bool DatabaseMySQL::disconnect()
{
	if(m_initialized)
	{
		mysql_close(&m_handle);
		m_initialized = false;
		return true;
	}
	return false;
}

bool DatabaseMySQL::executeQuery(DBQuery &q)
{
	if(!m_initialized || !m_connected)
		return false;

	#ifdef __MYSQL_QUERY_DEBUG__
	std::cout << q.str() << std::endl;
	#endif
	std::string s = q.str();
	const char* querytext = s.c_str();
	int32_t querylength = s.length(); //strlen(querytext);
	// Execute the query
	if(mysql_real_query(&m_handle, querytext, querylength) != 0)
	{
		std::cout << "> MySQL ERROR mysql_real_query: " << q.str() << " " << mysql_error(&m_handle)  << std::endl;
		int32_t error = mysql_errno(&m_handle);
		if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR)
			m_connected = false;
		return false;
	}

	// All is ok
	q.reset();
	return true;
}

std::string DatabaseMySQL::escapeBlob(const char* s, uint32_t length)
{
	// remember about quoiting even an empty string!
	if(!s)
		return std::string("''");

	// the worst case is 2n + 1
	char* output = new char[length * 2 + 1];

	// quotes escaped string and frees temporary buffer
	mysql_real_escape_string(&m_handle, output, s, length);
	std::string r = "'";
	r += output;
	r += "'";
	delete[] output;
	return r;
}

bool DatabaseMySQL::storeQuery(DBQuery &q, DBResult &dbres)
{
	MYSQL_ROW row;
	MYSQL_FIELD *fields;
	MYSQL_RES *r;
	uint32_t num_fields;

	// Execute the query
	if(!this->executeQuery(q))
		return false;

	// Getting results from the query
	r = mysql_store_result(&m_handle);
	if(!r)
	{
		std::cout << "> MySQL ERROR mysql_store_result: " << q.getText() << " " << mysql_error(&m_handle)  << std::endl;
		int32_t error = mysql_errno(&m_handle);
		if(error == CR_SERVER_LOST || error == CR_SERVER_GONE_ERROR)
			m_connected = false;
		return false;
	}

	// Getting the rows of the result
	num_fields = mysql_num_fields(r);

	dbres.clear();
	// Getting the field names
	//dbres.clearFieldNames();
	fields = mysql_fetch_fields(r);
	for(uint32_t i=0; i < num_fields; ++i)
		dbres.setFieldName(std::string(fields[i].name), i);

	// Adding the rows to a list
	//dbres.clearRows();
	while((row = mysql_fetch_row(r)))
	{
		//get column sizes
		unsigned long* lengths = mysql_fetch_lengths(r);
		dbres.addRow(row, lengths, num_fields);
	}

	// Free query result
	mysql_free_result(r);
	r = NULL;

	// Check if there are rows in the query
	return dbres.getNumRows() > 0;
}

bool DatabaseMySQL::rollback()
{
	if(!m_initialized || !m_connected)
		return false;

	#ifdef __MYSQL_QUERY_DEBUG__
	std::cout << "ROLLBACK;" << std::endl;
	#endif
	if(mysql_rollback(&m_handle) != 0)
	{
		std::cout << "> MySQL ERROR mysql_rollback: " << mysql_error(&m_handle)  << std::endl;
		return false;
	}
	return true;
}

bool DatabaseMySQL::commit()
{
	if(!m_initialized || !m_connected)
		return false;

	#ifdef __MYSQL_QUERY_DEBUG__
	std::cout << "COMMIT;" << std::endl;
	#endif
	if(mysql_commit(&m_handle) != 0)
	{
		std::cout << "> MySQL ERROR mysql_commit: " << mysql_error(&m_handle)  << std::endl;
		return false;
	}
	return true;
}
#endif
