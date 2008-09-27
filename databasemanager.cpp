//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Datbase Manager
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

#include "databasemanager.h"
#include "configmanager.h"
#include <iostream>

extern ConfigManager g_config;

bool DatabaseManager::optimizeTables()
{
	Database* db = Database::getInstance();
	DBQuery query;
	DBResult* result;

	if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
	{
		query << "VACUUM;";
		if(db->executeQuery(query.str()))
		{
			std::cout << "> Optimized database." << std::endl;
			return true;
		}
	}
	else
	{
		query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQL_DB)) << " AND `DATA_FREE` > 0;";
		if((result = db->storeQuery(query.str())))
		{
			do
			{
				if(result->getDataInt("DATA_FREE") > 0)
				{
					query.str("");
					std::cout << "> Optimizing table: " << result->getDataString("TABLE_NAME") << std::endl;
					query << "OPTIMIZE TABLE `" << result->getDataString("TABLE_NAME") << "`;";
				}
			}
			while(result->next());
			db->freeResult(result);
			return true;
		}
	}
	return false;
}

bool DatabaseManager::tableExists(std::string table)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;
	if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
		query << "SELECT `name` FROM `sqlite_master` WHERE `name` = " << db->escapeString(table) << ";";
	else
		query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQL_DB)) << " AND `TABLE_NAME` = " << db->escapeString(table) << ";";

	if((result = db->storeQuery(query.str())))
	{
		db->freeResult(result);
		return true;
	}
	return false;
}

bool DatabaseManager::isDatabaseSetup()
{
	Database* db = Database::getInstance();
	if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
	{
		//a pre-setup sqlite database is already included
		return true;
	}

	DBQuery query;
	DBResult* result;

	query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQL_DB)) << ";";
	if((result = db->storeQuery(query.str())))
	{
		db->freeResult(result);
		return true;
	}

	//no tables in database
	return false;
}

int DatabaseManager::getDatabaseVersion()
{
	if(tableExists("server_config"))
	{
		int value = 0;
		if(getDatabaseConfig("db_version", value))
			return value;

		return 1;
	}
	return 0;
}

int DatabaseManager::updateDatabase()
{
	if(!isDatabaseSetup())
	{
		/**
		 * TODO: Setup database tables.
		 * Using SOURCE schema.mysql; won't work (only a CLI command), and obviously
		 * parsing the file and executing the lines one by one won't work (possibly if
		 * we use a multi_query function), and executing the whole whole file content
		 * as a query doesn't work either.
		 *
		 * std::cout << "> Database is empty, creating default tables...";
		 * Database* db = Database::getInstance();
		 * DBQuery query;
		 * query << "SOURCE schema.mysql;";
		 * if(db->executeQuery(query.str()))
		 * 	std::cout << " success." << std::endl;
		 * else
		 * 	std::cout << " failed, make sure schema.mysql is in the same directory as the server." << std::endl;
		*/
		return -2;
	}

	Database* db = Database::getInstance();

	//TODO: Support for SQLite (was working on it until I saw the process to alter/drop field from a table...).
	if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
		return -1;

	switch(getDatabaseVersion())
	{
		case 0:
		{
			DBQuery query;
			DBResult* result;

			std::cout << "> Updating database to version: 1..." << std::endl;
			query.str("");
			query << "CREATE TABLE IF NOT EXISTS `server_config` ( `config` VARCHAR(35) NOT NULL DEFAULT '', `value` INT NOT NULL ) ENGINE = InnoDB;";
			db->executeQuery(query.str());

			query.str("");
			query << "INSERT INTO `server_config` VALUES ('db_version', 1);";
			db->executeQuery(query.str());

			query.str("");
			query << "SELECT `TABLE_SCHEMA` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQL_DB)) << " AND `TABLE_NAME` = 'server_motd';";

			if(!(result = db->storeQuery(query.str())))
			{
				//Create server_record table
				query.str("");
				query << "CREATE TABLE IF NOT EXISTS `server_record` ( `record` INT NOT NULL, `timestamp` BIGINT NOT NULL, PRIMARY KEY (`timestamp`) ) ENGINE = InnoDB;";
				db->executeQuery(query.str());

				//Create server_motd table
				query.str("");
				query << "CREATE TABLE `server_motd` ( `id` INT NOT NULL AUTO_INCREMENT, `text` TEXT NOT NULL, PRIMARY KEY (`id`) ) ENGINE = InnoDB;";
				db->executeQuery(query.str());

				//Create global_storage table
				query.str("");
				query << "CREATE TABLE IF NOT EXISTS `global_storage` ( `key` INT UNSIGNED NOT NULL, `value` INT NOT NULL, PRIMARY KEY  (`key`) ) ENGINE = InnoDB;";
				db->executeQuery(query.str());

				//Insert data to server_record table
				query.str("");
				query << "INSERT INTO `server_record` VALUES (0, 0);";
				db->executeQuery(query.str());

				//Insert data to server_motd table
				query.str("");
				query << "INSERT INTO `server_motd` VALUES (1, 'Welcome to The Forgotten Server!');";
				db->executeQuery(query.str());

				//Update groups
				query.str("");
				query << "ALTER TABLE `groups` ADD `customflags` BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER `flags`;";
				db->executeQuery(query.str());
				query.str("");
				query << "ALTER TABLE `groups` ADD `violationaccess` INT NOT NULL DEFAULT 0 AFTER `access`;";
				db->executeQuery(query.str());

				/**
				 * We cannot use TRUNCATE because of the foreign keys,
				 * so we'll be using UPDATE method instead.
				 * //query.str("");
				 * //query << "TRUNCATE `groups`;";
				 * //db->executeQuery(query.str());
				 */

				bool imported[6] = {false, false, false, false, false, false};
				bool groupWarn = false;
				query.str("");
				query << "SELECT `id` FROM `groups`;";
				if((result = db->storeQuery(query.str())))
				{
					do
					{
						int type = result->getDataInt("id");
						if(type >= 1 && type <= 6)
							imported[type - 1] = true;
						else if(!groupWarn)
							groupWarn = true;

						query.str("");
						bool execQuery = true;
						switch(type)
						{
							case 1:
								query << "UPDATE `groups` SET `name` = 'Player', `flags` = 0, `access` = 0, `maxdepotitems` = 0, `maxviplist` = 0 WHERE `id` = " << type << ";";
								break;

							case 2:
								query << "UPDATE `groups` SET `name` = 'Tutor', `flags` = 16809984, `customflags` = 1, `access` = 1, `maxdepotitems` = 0, `maxviplist` = 0 WHERE `id` = " << type << ";";
								break;

							case 3:
								query << "UPDATE `groups` SET `name` = 'Senior Tutor', `flags` = 68736352256, `customflags` = 3, `access` = 2, `violationaccess` = 1, `maxdepotitems` = 0, `maxviplist` = 0 WHERE `id` = " << type << ";";
								break;

							case 4:
								query << "UPDATE `groups` SET `name` = 'Game Master', `flags` = 492842123151, `customflags` = 63, `access` = 3, `violationaccess` = 2, `maxdepotitems` = 4000, `maxviplist` = 200 WHERE `id` = " << type << ";";
								break;

							case 5:
								query << "UPDATE `groups` SET `name` = 'Community Manager', `flags` = 542239465466, `customflags` = 1279, `access` = 4, `violationaccess` = 3, `maxdepotitems` = 6000, `maxviplist` = 300 WHERE `id` = " << type << ";";
								break;

							case 6:
								query << "UPDATE `groups` SET `name` = 'God', `flags` = 546534563834, `customflags` = 2047, `access` = 5, `violationaccess` = 3, `maxdepotitems` = 8000, `maxviplist` = 400 WHERE `id` = " << type << ";";
								break;

							default:
								execQuery = false;
								break;
						}

						if(execQuery)
							db->executeQuery(query.str());
					}
					while(result->next());
				}

				if(!imported[0])
				{
					query.str("");
					query << "INSERT INTO `groups` VALUES (1, 'Player', 0, 0, 0, 0, 0, 0);";
					db->executeQuery(query.str());
				}

				if(!imported[1])
				{
					query.str("");
					query << "INSERT INTO `groups` VALUES (2, 'Tutor', 16809984, 1, 1, 0, 0, 0);";
					db->executeQuery(query.str());
				}

				if(!imported[2])
				{
					query.str("");
					query << "INSERT INTO `groups` VALUES (3, 'Senior Tutor', 68736352256, 3, 2, 1, 0, 0);";
					db->executeQuery(query.str());
				}

				if(!imported[3])
				{
					query.str("");
					query << "INSERT INTO `groups` VALUES (4, 'Game Master', 492842123151, 63, 3, 2, 4000, 200);";
					db->executeQuery(query.str());
				}

				if(!imported[4])
				{
					query.str("");
					query << "INSERT INTO `groups` VALUES (5, 'Community Manager', 542239465466, 1279, 4, 3, 6000, 300);";
					db->executeQuery(query.str());
				}

				if(!imported[5])
				{
					query.str("");
					query << "INSERT INTO `groups` VALUES (6, 'God', 546534563834, 2047, 5, 3, 8000, 400);";
					db->executeQuery(query.str());
				}

				if(groupWarn)
					std::cout << "WARNING: It appears that you have more than 6 groups, there is nothing wrong with that you have more than 6 groups, but we didn't update them with the new settings from 0.3 which you might want to do!" << std::endl;

				//Update players table
				query.str("");
				query << "ALTER TABLE `players` ADD `balance` BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER `blessings`;";
				db->executeQuery(query.str());
				query.str("");
				query << "ALTER TABLE `players` ADD `stamina` BIGINT UNSIGNED NOT NULL DEFAULT 201660000 AFTER `balance`;";
				db->executeQuery(query.str());
				query.str("");
				query << "ALTER TABLE `players` CHANGE `loss_experience` `loss_experience` INT NOT NULL DEFAULT 10;";
				db->executeQuery(query.str());
				query.str("");
				query << "ALTER TABLE `players` CHANGE `loss_mana` `loss_mana` INT NOT NULL DEFAULT 10;";
				db->executeQuery(query.str());
				query.str("");
				query << "ALTER TABLE `players` CHANGE `loss_skills` `loss_skills` INT NOT NULL DEFAULT 10;";
				db->executeQuery(query.str());
				query.str("");
				query << "ALTER TABLE `players` ADD `loss_items` INT NOT NULL DEFAULT 10 AFTER `loss_skills`;";
				db->executeQuery(query.str());
				query.str("");
				query << "ALTER TABLE `players` ADD `marriage` INT UNSIGNED NOT NULL DEFAULT 0;";
				db->executeQuery(query.str());
				query.str("");
				query << "UPDATE `players` SET `loss_experience` = 10, `loss_mana` = 10, `loss_skills` = 10, `loss_items` = 10;";
				db->executeQuery(query.str());

				//Update accounts table
				query.str("");
				query << "ALTER TABLE `accounts` DROP `type`;";
				db->executeQuery(query.str());

				//Update player deaths table
				query.str("");
				query << "ALTER TABLE `player_deaths` DROP `is_player`;";
				db->executeQuery(query.str());

				//Update house table
				query.str("");
				query << "ALTER TABLE `houses` CHANGE `warnings` `warnings` INT NOT NULL DEFAULT 0;";
				db->executeQuery(query.str());
				query.str("");
				query << "ALTER TABLE `houses` ADD `lastwarning` INT UNSIGNED NOT NULL DEFAULT 0;";
				db->executeQuery(query.str());

				//Update bans table
				query.str("");

				query << "CREATE TABLE IF NOT EXISTS `bans2` ( `id` INT UNSIGNED NOT NULL auto_increment, `type` TINYINT(1) NOT NULL COMMENT 'this field defines if its ip, account, player, or any else ban', `value` INT UNSIGNED NOT NULL COMMENT 'ip, player guid, account number', `param` INT UNSIGNED NOT NULL DEFAULT 4294967295 COMMENT 'mask', `active` TINYINT(1) NOT NULL DEFAULT TRUE, `expires` INT UNSIGNED NOT NULL, `added` INT UNSIGNED NOT NULL, `admin_id` INT UNSIGNED NOT NULL DEFAULT 0, `comment` TEXT NOT NULL DEFAULT '', `reason` INT UNSIGNED NOT NULL DEFAULT 0, `action` INT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY  (`id`), KEY `type` (`type`, `value`) ) ENGINE = InnoDB;";
				if(db->executeQuery(query.str()))
				{
					query.str("");
					query << "SELECT * FROM `bans`;";
					if((result = db->storeQuery(query.str())))
					{
						do
						{
							query.str("");
							bool execQuery = false;
							switch(result->getDataInt("type"))
							{
								case 1:
									query << "INSERT INTO `bans2` (`type`, `value`, `param`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (1, " <<  result->getDataInt("ip") << ", " << result->getDataInt("mask") << ", " << (result->getDataInt("expired") ? 0 : 1) << ", " << result->getDataInt("time") << ", 0, " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
									break;

								case 2:
									query << "INSERT INTO `bans2` (`type`, `value`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (2, " << result->getDataInt("player") << ", " << (result->getDataInt("expired") ? 0 : 1) << ", 0, " << result->getDataInt("time") << ", " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
									break;

								case 3:
									query << "INSERT INTO `bans2` (`type`, `value`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (3, " << result->getDataInt("player") << ", " << (result->getDataInt("expired") ? 0 : 1) << ", " << result->getDataInt("time") << ", 0, " << result->getDataInt("banned_by") << ", '" << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
									break;

								case 4:
								case 5:
									query << "INSERT INTO `bans2` (`type`, `value`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << result->getDataInt("type") << ", " << result->getDataInt("player") << ", " << (result->getDataInt("expired") ? 0 : 1) << ", 0, " << result->getDataInt("time") << ", " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
									break;

								default:
									execQuery = false;
									break;
							}

							if(execQuery)
								db->executeQuery(query.str());
						}
						while(result->next());
						db->freeResult(result);
					}
					query.str("");
					query << "DROP TABLE `bans`;";
					db->executeQuery(query.str());

					query.str("");
					query << "RENAME TABLE `bans2` TO `bans`;";
					db->executeQuery(query.str());
				}

				//Update triggers
				query.str("");
				query << "DROP TRIGGER IF EXISTS `ondelete_accounts`;";
				db->executeQuery(query.str());
				query.str("");
				query << "DROP TRIGGER IF EXISTS `ondelete_players`;";
				db->executeQuery(query.str());

				query.str("");
				query << "CREATE TRIGGER `ondelete_accounts` BEFORE DELETE ON `accounts` FOR EACH ROW BEGIN DELETE FROM `bans` WHERE `type` != 1 AND `type` != 2 AND `value` = OLD.`id`; END;";
				db->executeQuery(query.str());

				query.str("");
				query << "CREATE TRIGGER `ondelete_players` BEFORE DELETE ON `players` FOR EACH ROW BEGIN DELETE FROM `bans` WHERE `type` = 2 AND `value` = OLD.`id`; UPDATE `houses` SET `owner` = 0 WHERE `owner` = OLD.`id`; END;";
				db->executeQuery(query.str());
			}
			else
				db->freeResult(result);

			return 1;
			break;
		}

		default:
		{
			break;
		}
	}
	return -1;
}

bool DatabaseManager::getDatabaseConfig(std::string config, int &value)
{
	value = 0;

	Database* db = Database::getInstance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `value` FROM `server_config` WHERE `config` = " << db->escapeString(config) << ";";
	if((result = db->storeQuery(query.str())))
	{
		value = result->getDataInt("value");
		db->freeResult(result);
		return true;
	}
	return false;
}
