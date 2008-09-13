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

#ifdef __USE_SQLITE__
#include <iostream>
#include "databasesqlite.h"
#include "configmanager.h"
#include "tools.h"

extern ConfigManager g_config;

DatabaseSqLite::DatabaseSqLite()
{
	init();
}

DatabaseSqLite::~DatabaseSqLite()
{
	disconnect();
}

bool DatabaseSqLite::m_fieldnames = false;

bool DatabaseSqLite::init()
{
	m_initialized = false;
	m_connected = false;

	// test for existence of database file;
	// sqlite3_open will create a new one if it isn't there (what we don't want)
	if(!fileExists(g_config.getString(ConfigManager::SQLITE_DB).c_str()))
		return m_initialized;

	// Initialize sqlite
	if(sqlite3_open(g_config.getString(ConfigManager::SQLITE_DB).c_str(), &m_handle) != SQLITE_OK)
	{
		std::cout << "SQLITE ERROR sqlite_init" << std::endl;
		sqlite3_close(m_handle);
	}
	else
		m_initialized = true;

	return m_initialized;
}

bool DatabaseSqLite::connect()
{
	//don't need to connect
	m_connected = true;
	return true;
}

bool DatabaseSqLite::disconnect()
{
	if(m_initialized)
	{
		sqlite3_close(m_handle);
		m_initialized = false;
		return true;
	}
	return false;
}

bool DatabaseSqLite::executeQuery(DBQuery &q)
{
	if(!m_initialized || !m_connected)
		return false;

	std::string s = q.str();
	const char* querytext = s.c_str();
	// Execute the query
	if(sqlite3_exec(m_handle, querytext, 0, 0, &zErrMsg) != SQLITE_OK)
	{
		std::cout << "SQLITE ERROR sqlite_exec: " << q.str() << " " << zErrMsg << std::endl;
		sqlite3_free(zErrMsg);
		return false;
	}

	// All is ok
	q.reset();
	return true;
}

std::string DatabaseSqLite::escapeBlob(const char* s, uint32_t length)
{
	std::string buf = "x'";

	char* hex = new char[2 + 1]; //need one extra byte for null-character

	for(uint32_t i = 0; i < length; i++)
	{
		sprintf(hex, "%02x", ((unsigned char)s[i]));
		buf += hex;
	}

	delete[] hex;

	buf += "'";
	return buf;
}

bool DatabaseSqLite::storeQuery(DBQuery &q, DBResult &dbres)
{
	if(!m_initialized || !m_connected)
		return false;

	std::string s = q.str();
	const char* querytext = s.c_str();

	q.reset();
	dbres.clear();
	// Execute the query
	if(sqlite3_exec(m_handle, querytext, DatabaseSqLite::callback, &dbres, &zErrMsg) != SQLITE_OK)
	{
		std::cout << "SQLITE ERROR sqlite_exec: " << q.str() << " " << zErrMsg << std::endl;
		sqlite3_free(zErrMsg);
		return false;
	}
	DatabaseSqLite::m_fieldnames = false;

	// Check if there are rows in the query
	return dbres.getNumRows() > 0;
}

bool DatabaseSqLite::rollback()
{
	DBQuery query;
	query << "ROLLBACK;";
	return executeQuery(query);
}

bool DatabaseSqLite::commit()
{
	DBQuery query;
	query << "COMMIT;";
	return executeQuery(query);
}

int DatabaseSqLite::callback(void *db, int num_fields, char **results, char **columnNames)
{
	DBResult* dbres = (DBResult*)db;
	if(!DatabaseSqLite::m_fieldnames)
	{
		for(int i = 0; i < num_fields; i++)
			dbres->setFieldName(std::string(columnNames[i]), i);
		DatabaseSqLite::m_fieldnames = true;
	}
	dbres->addRow(results, num_fields);
	return 0;
}
#endif
