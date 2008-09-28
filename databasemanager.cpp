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
#include "enums.h"
#include "tools.h"

#include <iostream>

extern ConfigManager g_config;

bool DatabaseManager::optimizeTables()
{
	Database* db = Database::getInstance();
	DBQuery query;
	DBResult* result;

	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_SQLITE:
		{
			query << "VACUUM;";
			if(db->executeQuery(query.str()))
			{
				std::cout << "> Optimized database." << std::endl;
				return true;
			}
			break;
		}

		case DATABASE_ENGINE_POSTGRESQL:
		{
			query << "VACUUM FULL;";
			if(db->executeQuery(query.str()))
			{
				std::cout << "> Optimized database." << std::endl;
				return true;
			}
			break;
		}

		case DATABASE_ENGINE_MYSQL:
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
			break;
		}

		default:
			break;
	}
	return false;
}

bool DatabaseManager::triggerExists(std::string trigger)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;
	if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
		query << "SELECT `name` FROM `sqlite_master` WHERE `type` = 'trigger' AND `name` = " << db->escapeString(trigger) << ";";
	else
		query << "SELECT `TRIGGER_NAME` FROM `information_schema`.`triggers` WHERE `TRIGGER_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQL_DB)) << " AND `TRIGGER_NAME` = " << db->escapeString(trigger) << ";";

	if((result = db->storeQuery(query.str())))
	{
		db->freeResult(result);
		return true;
	}
	return false;
}

bool DatabaseManager::tableExists(std::string table)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;
	if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
		query << "SELECT `name` FROM `sqlite_master` WHERE `type` = 'table' AND `name` = " << db->escapeString(table) << ";";
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

int32_t DatabaseManager::getDatabaseVersion()
{
	if(tableExists("server_config"))
	{
		int32_t value = 0;
		if(getDatabaseConfig("db_version", value))
			return value;

		return 1;
	}
	return 0;
}

int32_t DatabaseManager::updateDatabase()
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
	/**
	 * Until we are sure that PostgreSQL and the ODBC layer
	 * works fine in this update process they will remain
	 * disabled.
	 */
	if(db->getDatabaseEngine() == DATABASE_ENGINE_POSTGRESQL
		|| db->getDatabaseEngine() == DATABASE_ENGINE_ODBC)
	{
		return -1;
	}

	switch(getDatabaseVersion())
	{
		case 0:
		{
			DBQuery query;
			DBResult* result;

			std::cout << "> Updating database to version: 1..." << std::endl;

			query.str("");
			if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
				query << "CREATE TABLE IF NOT EXISTS `server_config` ( `config` VARCHAR(35) NOT NULL DEFAULT '', `value` INTEGER NOT NULL );";
			else
				query << "CREATE TABLE IF NOT EXISTS `server_config` ( `config` VARCHAR(35) NOT NULL DEFAULT '', `value` INT NOT NULL ) ENGINE = InnoDB;";
			db->executeQuery(query.str());

			query.str("");
			query << "INSERT INTO `server_config` VALUES ('db_version', 1);";
			db->executeQuery(query.str());

			if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
			{
				//Updating from 0.2 with SQLite is not supported yet, so we'll stop here.
				return -1;
			}

			if(!tableExists("server_motd"))
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
						int32_t type = result->getDataInt("id");
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
			return 1;
			break;
		}

		default:
			break;
	}
	return -1;
}

bool DatabaseManager::getDatabaseConfig(std::string config, int32_t &value)
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

void DatabaseManager::registerDatabaseConfig(std::string config, int32_t value)
{
	Database* db = Database::getInstance();
	DBQuery query;

	int32_t v = 0;
	if(!getDatabaseConfig(config, v))
		query << "INSERT INTO `server_config` VALUES (" << db->escapeString(config) << ", " << value << ");";
	else
		query << "UPDATE `server_config` SET `value` = " << value << " WHERE `config` = " << db->escapeString(config) << ";";

	db->executeQuery(query.str());
}

void DatabaseManager::checkPasswordType()
{
	PasswordType_t value = PASSWORD_TYPE_PLAIN;
	PasswordType_t newValue = (PasswordType_t)g_config.getNumber(ConfigManager::PASSWORDTYPE);
	if(getDatabaseConfig("password_type", (int32_t&)value))
	{
		if(value != newValue)
		{
			switch(newValue)
			{
				case PASSWORD_TYPE_MD5:
				{
					if(value != PASSWORD_TYPE_PLAIN)
					{
						std::cout << "> WARNING: You can not change the passwordType to MD5, change it back in config.lua to \"sha1\"." << std::endl;
						return;
					}

					registerDatabaseConfig("password_type", (int32_t)newValue);

					Database* db = Database::getInstance();
					DBQuery query;
					if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL
						|| db->getDatabaseEngine() == DATABASE_ENGINE_POSTGRESQL)
					{
						query << "UPDATE `accounts` SET `password` = md5(`password`);";
						db->executeQuery(query.str());
					}
					else
					{
						DBResult* result;
						query << "SELECT `id`, `password` FROM `accounts`;";
						if((result = db->storeQuery(query.str())))
						{
							do
							{
								query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToMD5(result->getDataString("password"))) << " WHERE `id` = " << result->getDataInt("id") << ";";
								db->executeQuery(query.str());
							}
							while(result->next());
							db->freeResult(result);
						}
					}
					std::cout << "> All passwords are now MD5 hashed." << std::endl;
					break;
				}

				case PASSWORD_TYPE_SHA1:
				{
					if(value != PASSWORD_TYPE_PLAIN)
					{
						std::cout << "> WARNING: You can not change the passwordType to SHA1, change it back in config.lua to \"md5\"." << std::endl;
						return;
					}

					registerDatabaseConfig("password_type", (int32_t)newValue);

					Database* db = Database::getInstance();
					DBQuery query;
					if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL
						|| db->getDatabaseEngine() == DATABASE_ENGINE_POSTGRESQL)
					{
						query << "UPDATE `accounts` SET `password` = sha1(`password`);";
						db->executeQuery(query.str());
					}
					else
					{
						DBResult* result;
						query << "SELECT `id`, `password` FROM `accounts`;";
						if((result = db->storeQuery(query.str())))
						{
							do
							{
								query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToSHA1(result->getDataString("password"))) << " WHERE `id` = " << result->getDataInt("id") << ";";
								db->executeQuery(query.str());
							}
							while(result->next());
							db->freeResult(result);
						}
					}
					std::cout << "> All passwords are now SHA1 hashed." << std::endl;
					break;
				}

				default:
				{
					std::cout << "> WARNING: You can not switch from hashed passwords to plain text, change back the passwordType in config.lua to the passwordType you were previously using." << std::endl;
					break;
				}
			}
		}
	}
	else
	{
		registerDatabaseConfig("password_type", (int32_t)newValue);
		if(g_config.getBool(ConfigManager::ACCOUNT_MANAGER))
		{
			switch(newValue)
			{
				case PASSWORD_TYPE_MD5:
				{
					Database* db = Database::getInstance();
					DBQuery query;
					query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToMD5("1")) << " WHERE `id` = 1 AND `password` = '1';";
					db->executeQuery(query.str());
					break;
				}

				case PASSWORD_TYPE_SHA1:
				{
					Database* db = Database::getInstance();
					DBQuery query;
					query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToSHA1("1")) << " WHERE `id` = 1 AND `password` = '1';";
					db->executeQuery(query.str());
					break;
				}

				default:
					break;
			}
		}
	}
}

void DatabaseManager::checkTriggers()
{
	Database* db = Database::getInstance();

	if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
	{
		std::string triggerName[5] =
		{
			"ondelete_accounts",
			"oncreate_guilds",
			"ondelete_guilds",
			"oncreate_players",
			"ondelete_players"
		};

		std::string triggerStatement[5] =
		{
			"CREATE TRIGGER `ondelete_accounts` BEFORE DELETE ON `accounts` FOR EACH ROW BEGIN DELETE FROM `bans` WHERE `type` != 1 AND `type` != 2 AND `value` = OLD.`id`; END;",
			"CREATE TRIGGER `oncreate_guilds` AFTER INSERT ON `guilds` FOR EACH ROW BEGIN INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('the Leader', 3, NEW.`id`); INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('a Vice-Leader', 2, NEW.`id`); INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('a Member', 1, NEW.`id`); END;",
			"CREATE TRIGGER `ondelete_guilds` BEFORE DELETE ON `guilds` FOR EACH ROW BEGIN UPDATE `players` SET `guildnick` = '', `rank_id` = 0 WHERE `rank_id` IN (SELECT `id` FROM `guild_ranks` WHERE `guild_id` = OLD.`id`); END;",
			"CREATE TRIGGER `oncreate_players` AFTER INSERT ON `players` FOR EACH ROW BEGIN INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 0, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 1, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 2, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 3, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 4, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 5, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 6, 10); END;",
			"CREATE TRIGGER `ondelete_players` BEFORE DELETE ON `players` FOR EACH ROW BEGIN DELETE FROM `bans` WHERE `type` = 2 AND `value` = OLD.`id`; UPDATE `houses` SET `owner` = 0 WHERE `owner` = OLD.`id`; END;"
		};

		for(int i = 0; i < 5; i++)
		{
			if(!triggerExists(triggerName[i]))
			{
				std::cout << "> Trigger: " << triggerName[i] << " does not exist, creating it..." << std::endl;
				db->executeQuery(triggerStatement[i]);
			}
		}
	}
	else if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
	{
		std::string triggerName[28] =
		{
			"oncreate_guilds",
			"oncreate_players",
			"ondelete_accounts",
			"ondelete_players",
			"ondelete_guilds",
			"oninsert_players",
			"onupdate_players",
			"ondelete_groups",
			"oninsert_guilds",
			"onupdate_guilds",
			"ondelete_houses",
			"ondelete_tiles",
			"oninsert_guild_ranks",
			"onupdate_guild_ranks",
			"oninsert_house_lists",
			"onupdate_house_lists",
			"oninsert_player_depotitems",
			"onupdate_player_depotitems",
			"oninsert_player_skills",
			"onupdate_player_skills",
			"oninsert_player_storage",
			"onupdate_player_storage",
			"oninsert_player_viplist",
			"onupdate_player_viplist",
			"oninsert_tile_items",
			"onupdate_tile_items",
			"oninsert_player_spells",
			"onupdate_player_spells"
		};

		std::string triggerStatement[28] =
		{
			"CREATE TRIGGER \"oncreate_guilds\" AFTER INSERT ON \"guilds\" BEGIN INSERT INTO \"guild_ranks\" (\"name\", \"level\", \"guild_id\") VALUES (\"the Leader\", 3, NEW.\"id\"); INSERT INTO \"guild_ranks\" (\"name\", \"level\", \"guild_id\") VALUES (\"a Vice-Leader\", 2, NEW.\"id\"); INSERT INTO \"guild_ranks\" (\"name\", \"level\", \"guild_id\") VALUES (\"a Member\", 1, NEW.\"id\"); END;",
			"CREATE TRIGGER \"oncreate_players\" AFTER INSERT ON \"players\" BEGIN INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 0, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 1, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 2, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 3, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 4, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 5, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 6, 10); END;",
			"CREATE TRIGGER \"ondelete_accounts\" BEFORE DELETE ON \"accounts\" FOR EACH ROW BEGIN DELETE FROM \"players\" WHERE \"account_id\" = OLD.\"id\"; DELETE FROM \"bans\" WHERE \"type\" != 1 AND \"type\" != 2 AND \"value\" = OLD.\"id\"; END;",
			"CREATE TRIGGER \"ondelete_players\" BEFORE DELETE ON \"players\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'DELETE on table \"players\" violates foreign: \"ownerid\" from table \"guilds\"') WHERE (SELECT \"id\" FROM \"guilds\" WHERE \"ownerid\" = OLD.\"id\") IS NOT NULL; DELETE FROM \"player_viplist\" WHERE \"player_id\" = OLD.\"id\" OR \"vip_id\" = OLD.\"id\"; DELETE FROM \"player_storage\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"player_skills\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"player_items\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"player_depotitems\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"player_spells\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"bans\" WHERE \"type\" = 2 AND \"value\" = OLD.\"id\"; UPDATE \"houses\" SET \"owner\" = 0 WHERE \"owner\" = OLD.\"id\"; END;",
			"CREATE TRIGGER \"ondelete_guilds\" BEFORE DELETE ON \"guilds\" FOR EACH ROW BEGIN UPDATE \"players\" SET \"guildnick\" = '', \"rank_id\" = 0 WHERE \"rank_id\" IN (SELECT \"id\" FROM \"guild_ranks\" WHERE \"guild_id\" = OLD.\"id\"); DELETE FROM \"guild_ranks\" WHERE \"guild_id\" = OLD.\"id\"; END;",
			"CREATE TRIGGER \"oninsert_players\" BEFORE INSERT ON \"players\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"players\" violates foreign: \"account_id\"') WHERE NEW.\"account_id\" IS NULL OR (SELECT \"id\" FROM \"accounts\" WHERE \"id\" = NEW.\"account_id\") IS NULL; SELECT RAISE(ROLLBACK, 'INSERT on table \"players\" violates foreign: \"group_id\"') WHERE NEW.\"group_id\" IS NULL OR (SELECT \"id\" FROM \"groups\" WHERE \"id\" = NEW.\"group_id\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_players\" BEFORE UPDATE ON \"players\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"players\" violates foreign: \"account_id\"') WHERE NEW.\"account_id\" IS NULL OR (SELECT \"id\" FROM \"accounts\" WHERE \"id\" = NEW.\"account_id\") IS NULL; SELECT RAISE(ROLLBACK, 'UPDATE on table \"players\" violates foreign: \"group_id\"') WHERE NEW.\"group_id\" IS NULL OR (SELECT \"id\" FROM \"groups\" WHERE \"id\" = NEW.\"group_id\") IS NULL; END;",
			"CREATE TRIGGER \"ondelete_groups\" BEFORE DELETE ON \"groups\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'DELETE on table \"groups\" violates foreign: \"group_id\" from table \"players\"') WHERE (SELECT \"id\" FROM \"players\" WHERE \"group_id\" = OLD.\"id\") IS NOT NULL; END;",
			"CREATE TRIGGER \"oninsert_guilds\" BEFORE INSERT ON \"guilds\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"guilds\" violates foreign: \"ownerid\"') WHERE NEW.\"ownerid\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"ownerid\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_guilds\" BEFORE UPDATE ON \"guilds\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"guilds\" violates foreign: \"ownerid\"') WHERE NEW.\"ownerid\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"ownerid\") IS NULL; END;",
			"CREATE TRIGGER \"ondelete_houses\" BEFORE DELETE ON \"houses\" FOR EACH ROW BEGIN DELETE FROM \"house_lists\" WHERE \"house_id\" = OLD.\"id\"; END;",
			"CREATE TRIGGER \"ondelete_tiles\" BEFORE DELETE ON \"tiles\" FOR EACH ROW BEGIN DELETE FROM \"tile_items\" WHERE \"tile_id\" = OLD.\"id\"; END;",
			"CREATE TRIGGER \"oninsert_guild_ranks\" BEFORE INSERT ON \"guild_ranks\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"guild_ranks\" violates foreign: \"guild_id\"') WHERE NEW.\"guild_id\" IS NULL OR (SELECT \"id\" FROM \"guilds\" WHERE \"id\" = NEW.\"guild_id\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_guild_ranks\" BEFORE UPDATE ON \"guild_ranks\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"guild_ranks\" violates foreign: \"guild_id\"') WHERE NEW.\"guild_id\" IS NULL OR (SELECT \"id\" FROM \"guilds\" WHERE \"id\" = NEW.\"guild_id\") IS NULL; END;",
			"CREATE TRIGGER \"oninsert_house_lists\" BEFORE INSERT ON \"house_lists\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"house_lists\" violates foreign: \"house_id\"') WHERE NEW.\"house_id\" IS NULL OR (SELECT \"id\" FROM \"houses\" WHERE \"id\" = NEW.\"house_id\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_house_lists\" BEFORE UPDATE ON \"house_lists\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"house_lists\" violates foreign: \"house_id\"') WHERE NEW.\"house_id\" IS NULL OR (SELECT \"id\" FROM \"houses\" WHERE \"id\" = NEW.\"house_id\") IS NULL; END;",
			"CREATE TRIGGER \"oninsert_player_depotitems\" BEFORE INSERT ON \"player_depotitems\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"player_depotitems\" violates foreign: \"player_id\"') WHERE NEW.\"player_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"player_id\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_player_depotitems\" BEFORE UPDATE ON \"player_depotitems\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"player_depotitems\" violates foreign: \"player_id\"') WHERE NEW.\"player_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"player_id\") IS NULL; END;",
			"CREATE TRIGGER \"oninsert_player_skills\" BEFORE INSERT ON \"player_skills\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"player_skills\" violates foreign: \"player_id\"') WHERE NEW.\"player_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"player_id\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_player_skills\" BEFORE UPDATE ON \"player_skills\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"player_skills\" violates foreign: \"player_id\"') WHERE NEW.\"player_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"player_id\") IS NULL; END;",
			"CREATE TRIGGER \"oninsert_player_storage\" BEFORE INSERT ON \"player_storage\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"player_storage\" violates foreign: \"player_id\"') WHERE NEW.\"player_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"player_id\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_player_storage\" BEFORE UPDATE ON \"player_storage\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"player_storage\" violates foreign: \"player_id\"') WHERE NEW.\"player_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"player_id\") IS NULL; END;",
			"CREATE TRIGGER \"oninsert_player_viplist\" BEFORE INSERT ON \"player_viplist\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"player_viplist\" violates foreign: \"player_id\"') WHERE NEW.\"player_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"player_id\") IS NULL; SELECT RAISE(ROLLBACK, 'INSERT on table \"player_viplist\" violates foreign: \"vip_id\"') WHERE NEW.\"vip_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"vip_id\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_player_viplist\" BEFORE UPDATE ON \"player_viplist\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"player_viplist\" violates foreign: \"vip_id\"') WHERE NEW.\"vip_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"vip_id\") IS NULL; END;",
			"CREATE TRIGGER \"oninsert_tile_items\" BEFORE INSERT ON \"tile_items\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"tile_items\" violates foreign: \"tile_id\"') WHERE NEW.\"tile_id\" IS NULL OR (SELECT \"id\" FROM \"tiles\" WHERE \"id\" = NEW.\"tile_id\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_tile_items\" BEFORE UPDATE ON \"tile_items\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"tile_items\" violates foreign: \"tile_id\"') WHERE NEW.\"tile_id\" IS NULL OR (SELECT \"id\" FROM \"tiles\" WHERE \"id\" = NEW.\"tile_id\") IS NULL; END;",
			"CREATE TRIGGER \"oninsert_player_spells\" BEFORE INSERT ON \"player_spells\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"player_spells\" violates foreign: \"player_id\"') WHERE NEW.\"player_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"player_id\") IS NULL; END;",
			"CREATE TRIGGER \"onupdate_player_spells\" BEFORE UPDATE ON \"player_spells\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"player_spells\" violates foreign: \"player_id\"') WHERE NEW.\"player_id\" IS NULL OR (SELECT \"id\" FROM \"players\" WHERE \"id\" = NEW.\"player_id\") IS NULL; END;"
		};

		for(int i = 0; i < 28; i++)
		{
			if(!triggerExists(triggerName[i]))
			{
				std::cout << "> Trigger: " << triggerName[i] << " does not exist, creating it..." << std::endl;
				db->executeQuery(triggerStatement[i]);
			}
		}
	}
}
