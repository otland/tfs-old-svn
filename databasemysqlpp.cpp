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

#ifdef __USE_MYSQLPP__

#include <iostream>

#include "database.h"
#include "databasemysqlpp.h"

#include "scheduler.h"
#include "configmanager.h"
#include "tools.h"

extern ConfigManager g_config;

DatabaseMySQLpp::DatabaseMySQLpp() :
	m_connection(NULL)
{
	m_driver = sql::mysql::get_mysql_driver_instance();
	assert(m_driver);
	m_driver->threadInit();

	assert(connect(false));
	m_connection->setSchema(g_config.getString(ConfigManager::SQL_DB));
	if(asLowerCaseString(g_config.getString(ConfigManager::HOUSE_STORAGE)) == "relational")
		return;

	//we cannot lock mutex here :)
	DBResult* result = storeQuery("SHOW variables LIKE 'max_allowed_packet';");
	assert(result);

	if(result->getDataLong("Value") < 16776192)
		std::clog << std::endl << "> WARNING: max_allowed_packet might be set too low for binary map storage." << std::endl
			<< "Use the following query to raise max_allow_packet: SET GLOBAL max_allowed_packet = 16776192;" << std::endl;

	result->free();
}

DatabaseMySQLpp::~DatabaseMySQLpp()
{
	if(m_connection)
		delete m_connection;

	if(m_driver)
	{
		m_driver->threadEnd();
		delete m_driver;
	}
}

bool DatabaseMySQLpp::connect(bool reconnect)
{
	if(reconnect && m_connection)
	{
		m_connection->close();
		delete m_connection;
	}

	m_connection = dynamic_cast<sql::mysql::MySQL_Connection*>(m_driver->connect(
		"tcp://"
			+
		g_config.getString(ConfigManager::SQL_HOST)
			+
		":"
			+
		asString(
			g_config.getNumber(ConfigManager::SQL_PORT)
		),
		g_config.getString(ConfigManager::SQL_USER),
		g_config.getString(ConfigManager::SQL_PASS)));
	return m_connection != NULL;
}

bool DatabaseMySQLpp::getParam(DBParam_t param)
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

bool DatabaseMySQLpp::beginTransaction()
{
	if(!m_connection)
		return false;

	return query("BEGIN");
}

bool DatabaseMySQLpp::rollback()
{
	if(!m_connection)
		return false;

	m_connection->rollback();
	return true;
}

bool DatabaseMySQLpp::commit()
{
	if(!m_connection)
		return false;

	m_connection->commit();
	return true;
}

bool DatabaseMySQLpp::query(const std::string &query)
{
	if(!m_connection && !connect(true))
		return false;

	sql::Statement* statement = m_connection->createStatement();
	if(sql::ResultSet* result = statement->executeQuery(query))
		delete result;

	delete statement;
	return true;
}

DBResult* DatabaseMySQLpp::storeQuery(const std::string &query)
{
	if(!m_connection && !connect(true))
		return false;

	sql::Statement* statement = m_connection->createStatement();
	if(sql::ResultSet* result = statement->executeQuery(query))
	{
		delete statement;
		DBResult* _result = (DBResult*)new MySQLppResult(result);
		return verifyResult(_result);
	}

	delete statement;
	return NULL;
}

std::string DatabaseMySQLpp::escapeBlob(const char* s, uint32_t)
{
	if(!m_connection || *s == '\0')
		return "''";

	return "'" + m_connection->escapeString(s) + "'";
}

uint64_t DatabaseMySQLpp::getLastInsertId()
{
	// TODO!
	return 0;
}

int32_t MySQLppResult::getDataInt(const std::string& s)
{
	if(!m_result)
		return 0;

	return m_result->getInt(s);
}

int64_t MySQLppResult::getDataLong(const std::string& s)
{
	if(!m_result)
		return 0;

	return m_result->getInt64(s);
}

std::string MySQLppResult::getDataString(const std::string& s)
{
	if(!m_result)
		return std::string();

	return m_result->getString(s);
}

const char* MySQLppResult::getDataStream(const std::string& s, uint64_t& size)
{
	if(!m_result)
	{
		size = 0;
		return "";
	}

	std::istream* result = m_result->getBlob(s);
	result->seekg(0, ios::end);
	size = result->tellg();

	char* tmp;
	result->get(tmp, size);
	return (const char*)tmp;
}

void MySQLppResult::free()
{
	if(!m_result)
	{
		std::clog << "[Critical - MySQLppResult::free] Trying to free already freed result!!!" << std::endl;
		return;
	}

	delete this;
}

bool MySQLppResult::next()
{
	return m_result->next();
}

MySQLppResult::~MySQLppResult()
{
	if(!m_result)
		return;

	delete m_result;
}

MySQLppResult::MySQLppResult(sql::ResultSet* result) :
	m_result(result)
{ }
#endif
