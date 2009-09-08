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
#include <iostream>
#include <boost/regex.hpp>

#include "database.h"
#include "databasesqlite.h"

#include "tools.h"
#include "configmanager.h"
extern ConfigManager g_config;

#if SQLITE_VERSION_NUMBER < 3003009
#define OTSYS_SQLITE3_PREPARE sqlite3_prepare
#else
#define OTSYS_SQLITE3_PREPARE sqlite3_prepare_v2
#endif

DatabaseSQLite::DatabaseSQLite()
{
	m_connected = false;
	// test for existence of database file;
	// sqlite3_open will create a new one if it isn't there (what we don't want)
	if(!fileExists(g_config.getString(ConfigManager::SQL_FILE).c_str()))
		return;

	// Initialize sqlite
	if(sqlite3_open(g_config.getString(ConfigManager::SQL_FILE).c_str(), &m_handle) != SQLITE_OK)
	{
		std::cout << "Failed to initialize SQLite connection." << std::endl;
		sqlite3_close(m_handle);
	}
	else
		m_connected = true;
}

bool DatabaseSQLite::getParam(DBParam_t param)
{
	switch(param)
	{
		case DBPARAM_MULTIINSERT:
		default:
			break;
	}

	return false;
}

std::string DatabaseSQLite::_parse(const std::string &s)
{
	std::string query = "";
	query.reserve(s.size());

	bool inString = false;
	for(uint32_t i = 0; i < s.length(); i++)
	{
		uint8_t ch = s[i];
		if(ch == '\'')
		{
			if(inString && s[i + 1] != '\'')
				inString = false;
			else
				inString = true;
		}

		if(ch == '`' && !inString)
			ch = '"';

		query += ch;
	}

	return query;
}

bool DatabaseSQLite::executeQuery(const std::string &query)
{
	boost::recursive_mutex::scoped_lock lockClass(sqliteLock);
	if(!m_connected)
		return false;

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << "SQLITE QUERY: " << query << std::endl;
	#endif

	std::string buf = _parse(query);
	sqlite3_stmt* stmt;
	// prepares statement
	if(OTSYS_SQLITE3_PREPARE(m_handle, buf.c_str(), buf.length(), &stmt, NULL) != SQLITE_OK)
	{
		sqlite3_finalize(stmt);
		std::cout << "OTSYS_SQLITE3_PREPARE(): SQLITE ERROR: " << sqlite3_errmsg(m_handle)  << " (" << buf << ")" << std::endl;
		return false;
	}

	// executes it once
	int32_t ret = sqlite3_step(stmt);
	if(ret != SQLITE_OK && ret != SQLITE_DONE && ret != SQLITE_ROW)
	{
		sqlite3_finalize(stmt);
		std::cout << "sqlite3_step(): SQLITE ERROR: " << sqlite3_errmsg(m_handle) << std::endl;
		return false;
	}

	// closes statement
	// at all not sure if it should be debugged - query was executed correctly...
	sqlite3_finalize(stmt);
	return true;
}

DBResult* DatabaseSQLite::storeQuery(const std::string &query)
{
	boost::recursive_mutex::scoped_lock lockClass(sqliteLock);
	if(!m_connected)
		return NULL;

	#ifdef __SQL_QUERY_DEBUG__
	std::cout << "SQLITE QUERY: " << query << std::endl;
	#endif

	std::string buf = _parse(query);
	sqlite3_stmt* stmt;
	// prepares statement
	if(OTSYS_SQLITE3_PREPARE(m_handle, buf.c_str(), buf.length(), &stmt, NULL) != SQLITE_OK)
	{
		sqlite3_finalize(stmt);
		std::cout << "OTSYS_SQLITE3_PREPARE(): SQLITE ERROR: " << sqlite3_errmsg(m_handle)  << " (" << buf << ")" << std::endl;
		return NULL;
	}

	DBResult* result = new SQLiteResult(stmt);
	return verifyResult(result);
}

std::string DatabaseSQLite::escapeString(const std::string &s)
{
	// remember about quoiting even an empty string!
	if(!s.size())
		return std::string("''");

	// the worst case is 2n + 3
	char* output = new char[s.length() * 2 + 3];
	// quotes escaped string and frees temporary buffer
	sqlite3_snprintf(s.length() * 2 + 1, output, "%Q", s.c_str());

	std::string r(output);
	delete[] output;

	//escape % and _ because we are using LIKE operator.
	r = boost::regex_replace(r, boost::regex("%"), "\\%");
	r = boost::regex_replace(r, boost::regex("_"), "\\_");
	if(r[r.length() - 1] != '\'')
		r += "'";

	return r;
}

std::string DatabaseSQLite::escapeBlob(const char* s, uint32_t length)
{
	std::string buf = "x'";
	char* hex = new char[2 + 1]; //need one extra byte for null-character
	for(uint32_t i = 0; i < length; i++)
	{
		sprintf(hex, "%02x", ((uint8_t)s[i]));
		buf += hex;
	}

	delete[] hex;
	buf += "'";
	return buf;
}

int32_t SQLiteResult::getDataInt(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
		return sqlite3_column_int(m_handle, it->second);

	std::cout << "Error during getDataInt(" << s << ")." << std::endl;
	return 0; // Failed
}

int64_t SQLiteResult::getDataLong(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
		return sqlite3_column_int64(m_handle, it->second);

	std::cout << "Error during getDataLong(" << s << ")." << std::endl;
	return 0; // Failed
}

std::string SQLiteResult::getDataString(const std::string &s)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end() )
	{
		std::string value = (const char*)sqlite3_column_text(m_handle, it->second);
		return value;
	}

	std::cout << "Error during getDataString(" << s << ")." << std::endl;
	return std::string(""); // Failed
}

const char* SQLiteResult::getDataStream(const std::string &s, uint64_t &size)
{
	listNames_t::iterator it = m_listNames.find(s);
	if(it != m_listNames.end())
	{
		const char* value = (const char*)sqlite3_column_blob(m_handle, it->second);
		size = sqlite3_column_bytes(m_handle, it->second);
		return value;
	}

	std::cout << "Error during getDataStream(" << s << ")." << std::endl;
	return NULL; // Failed
}

void SQLiteResult::free()
{
	if(m_handle)
	{
		sqlite3_finalize(m_handle);
		delete this;
	}
	else
		std::cout << "[Warning - SQLiteResult::free] Trying to free already freed result." << std::endl;
}

SQLiteResult::SQLiteResult(sqlite3_stmt* stmt)
{
	if(!stmt)
	{
		delete this;
		return;
	}

	m_handle = stmt;
	m_listNames.clear();

	int32_t fields = sqlite3_column_count(m_handle);
	for(int32_t i = 0; i < fields; i++)
		m_listNames[sqlite3_column_name(m_handle, i)] = i;
}
