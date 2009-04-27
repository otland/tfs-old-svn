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
#include "enums.h"
#include <iostream>

#include "databasemanager.h"
#include "tools.h"

#include "configmanager.h"
extern ConfigManager g_config;

bool DatabaseManager::optimizeTables()
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_MYSQL:
		{
			query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQL_DB)) << " AND `DATA_FREE` > 0;";
			if(!(result = db->storeQuery(query.str())))
				break;

			query.str("");
			do
			{
				std::cout << "> Optimizing table: " << result->getDataString("TABLE_NAME") << std::endl;
				query << "OPTIMIZE TABLE `" << result->getDataString("TABLE_NAME") << "`;";
				if(!db->executeQuery(query.str()))
					std::cout << "WARNING: Optimization failed." << std::endl;

				query.str("");
			}
			while(result->next());
			result->free();
			return true;
		}

		case DATABASE_ENGINE_SQLITE:
		{
			query << "VACUUM;";
			if(!db->executeQuery(query.str()))
				break;

			std::cout << "> Optimized database." << std::endl;
			return true;
		}

		case DATABASE_ENGINE_POSTGRESQL:
		{
			query << "VACUUM FULL;";
			if(!db->executeQuery(query.str()))
				break;

			std::cout << "> Optimized database." << std::endl;
			return true;
		}

		default:
			break;
	}

	return false;
}

bool DatabaseManager::triggerExists(std::string trigger)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_SQLITE:
			query << "SELECT `name` FROM `sqlite_master` WHERE `type` = 'trigger' AND `name` = " << db->escapeString(trigger) << ";";
			break;

		case DATABASE_ENGINE_MYSQL:
			query << "SELECT `TRIGGER_NAME` FROM `information_schema`.`triggers` WHERE `TRIGGER_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQL_DB)) << " AND `TRIGGER_NAME` = " << db->escapeString(trigger) << ";";
			break;

		case DATABASE_ENGINE_POSTGRESQL:
			//TODO: PostgreSQL support
			return true;

		default:
			return false;
	}

	if(!(result = db->storeQuery(query.str())))
		return false;

	result->free();
	return true;
}

bool DatabaseManager::tableExists(std::string table)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_SQLITE:
			query << "SELECT `name` FROM `sqlite_master` WHERE `type` = 'table' AND `name` = " << db->escapeString(table) << ";";
			break;

		case DATABASE_ENGINE_MYSQL:
			query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQL_DB)) << " AND `TABLE_NAME` = " << db->escapeString(table) << ";";
			break;

		case DATABASE_ENGINE_POSTGRESQL:
			//TODO: PostgreSQL support
			return true;

		default:
			return false;
	}

	if(!(result = db->storeQuery(query.str())))
		return false;

	result->free();
	return true;
}

bool DatabaseManager::isDatabaseSetup()
{
	Database* db = Database::getInstance();
	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_MYSQL:
		{
			DBQuery query;
			query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQL_DB)) << ";";

			DBResult* result;
			if(!(result = db->storeQuery(query.str())))
				return false;

			result->free();
			return true;
		}

		case DATABASE_ENGINE_SQLITE:
			//a pre-setup sqlite database is already included
			return true;

		case DATABASE_ENGINE_POSTGRESQL:
			//TODO: PostgreSQL support
			return true;

		default:
			break;
	}

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

uint32_t DatabaseManager::updateDatabase()
{
	Database* db = Database::getInstance();
	uint32_t version = getDatabaseVersion();
	if(db->getDatabaseEngine() == DATABASE_ENGINE_ODBC)
		return version;

	if(version < 6 && db->getDatabaseEngine() == DATABASE_ENGINE_POSTGRESQL)
	{
		std::cout << "> WARNING: Couldn't update database - PostgreSQL support available since version 6, please use latest schema.pgsql." << std::endl;
		registerDatabaseConfig("db_version", 6);
		return 6;
	}

	switch(version)
	{
		case 0:
		{
			DBResult* result;
			std::cout << "> Updating database to version: 1..." << std::endl;

			DBQuery query;
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
				//TODO: 0.2 migration SQLite support
				std::cerr << "> SQLite migration from 0.2 support not available, please use latest database!" << std::endl;
				return 1;
			}

			if(!tableExists("server_motd"))
			{
				//Create server_record table
				query.str("");
				query << "CREATE TABLE IF NOT EXISTS `server_record` ( `record` INT NOT NULL, `timestamp` BIGINT NOT NULL, PRIMARY KEY (`timestamp`) ) ENGINE = InnoDB;";
				db->executeQuery(query.str());

				//Create server_reports table
				query.str("");
				query << "CREATE TABLE IF NOT EXISTS `server_reports` ( `id` INT NOT NULL AUTO_INCREMENT, `player_id` INT UNSIGNED NOT NULL DEFAULT 0, `posx` INT NOT NULL DEFAULT 0, `posy` INT NOT NULL DEFAULT 0, `posz` INT NOT NULL DEFAULT 0, `timestamp` BIGINT NOT NULL DEFAULT 0, `report` TEXT NOT NULL, `reads` INT NOT NULL DEFAULT 0, PRIMARY KEY (`id`), KEY (`player_id`), KEY (`reads`) ) ENGINE = InnoDB;";
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

				//Update players table
				query.str("");
				query << "ALTER TABLE `players` ADD `balance` BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER `blessings`;";
				db->executeQuery(query.str());
				query.str("");
				query << "ALTER TABLE `players` ADD `stamina` BIGINT UNSIGNED NOT NULL DEFAULT 201660000 AFTER `balance`;";
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
				query << "CREATE TABLE IF NOT EXISTS `bans2` ( `id` INT UNSIGNED NOT NULL auto_increment, `type` TINYINT(1) NOT NULL COMMENT 'this field defines if its ip, account, player, or any else ban', `value` INT UNSIGNED NOT NULL COMMENT 'ip, player guid, account number', `param` INT UNSIGNED NOT NULL DEFAULT 4294967295 COMMENT 'mask', `active` TINYINT(1) NOT NULL DEFAULT TRUE, `expires` INT UNSIGNED NOT NULL, `added` INT UNSIGNED NOT NULL, `admin_id` INT UNSIGNED NOT NULL DEFAULT 0, `comment` TEXT NOT NULL, `reason` INT UNSIGNED NOT NULL DEFAULT 0, `action` INT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY  (`id`), KEY `type` (`type`, `value`) ) ENGINE = InnoDB;";
				if(db->executeQuery(query.str()))
				{
					query.str("");
					query << "SELECT * FROM `bans`;";
					if((result = db->storeQuery(query.str())))
					{
						do
						{
							query.str("");
							switch(result->getDataInt("type"))
							{
								case 1:
									query << "INSERT INTO `bans2` (`type`, `value`, `param`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (1, " <<  result->getDataInt("ip") << ", " << result->getDataInt("mask") << ", " << (result->getDataInt("time") <= time(NULL) ? 0 : 1) << ", " << result->getDataInt("time") << ", 0, " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
									break;

								case 2:
									query << "INSERT INTO `bans2` (`type`, `value`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (2, " << result->getDataInt("player") << ", " << (result->getDataInt("time") <= time(NULL) ? 0 : 1) << ", 0, " << result->getDataInt("time") << ", " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
									break;

								case 3:
									query << "INSERT INTO `bans2` (`type`, `value`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (3, " << result->getDataInt("player") << ", " << (result->getDataInt("time") <= time(NULL) ? 0 : 1) << ", " << result->getDataInt("time") << ", 0, " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
									break;

								case 4:
								case 5:
									query << "INSERT INTO `bans2` (`type`, `value`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << result->getDataInt("type") << ", " << result->getDataInt("player") << ", " << (result->getDataInt("time") <= time(NULL) ? 0 : 1) << ", 0, " << result->getDataInt("time") << ", " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
									break;

								default:
									break;
							}

							if(query.str() != "")
								db->executeQuery(query.str());
						}
						while(result->next());
						result->free();
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

			query.str("");
			return 1;
		}

		case 1:
		{
			DBResult* result;
			std::cout << "> Updating database to version: 2..." << std::endl;

			DBQuery query;
			if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
				query << "ALTER TABLE `players` ADD `promotion` INTEGER NOT NULL DEFAULT 0;";
			else
				query << "ALTER TABLE `players` ADD `promotion` INT NOT NULL DEFAULT 0;";
			db->executeQuery(query.str());

			query.str("");
			query << "SELECT `player_id`, `value` FROM `player_storage` WHERE `key` = 30018 AND `value` > 0";
			if((result = db->storeQuery(query.str())))
			{
				do
				{
					query.str("");
					query << "UPDATE `players` SET `promotion` = " << result->getDataLong("value") << " WHERE `id` = " << result->getDataInt("player_id") << ";";
					db->executeQuery(query.str());
				}
				while(result->next());
				result->free();
			}

			query.str("");
			query << "DELETE FROM `player_storage` WHERE `key` = 30018;";
			db->executeQuery(query.str());

			query.str("");
			query << "ALTER TABLE `accounts` ADD `name` VARCHAR(32) NOT NULL DEFAULT '';";
			db->executeQuery(query.str());

			query.str("");
			query << "SELECT `id` FROM `accounts`;";
			if((result = db->storeQuery(query.str())))
			{
				do
				{
					query.str("");
					query << "UPDATE `accounts` SET `name` = '" << result->getDataInt("id") << "' WHERE `id` = " << result->getDataInt("id") << ";";
					db->executeQuery(query.str());
				}
				while(result->next());
				result->free();
			}

			query.str("");
			if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
				query << "ALTER TABLE `players` ADD `deleted` BOOLEAN NOT NULL DEFAULT 0;";
			else
				query << "ALTER TABLE `players` ADD `deleted` TINYINT(1) NOT NULL DEFAULT 0;";

			db->executeQuery(query.str());
			query.str("");
			if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
			{
				query << "ALTER TABLE `player_items` CHANGE `attributes` `attributes` BLOB NOT NULL;";
				db->executeQuery(query.str());
				query.str("");
			}

			registerDatabaseConfig("db_version", 2);
			return 2;
		}

		case 2:
		{
			DBResult* result;
			std::cout << "> Updating database to version: 3..." << std::endl;

			DBQuery query;
			query << "UPDATE `players` SET `vocation` = `vocation` - 4 WHERE `vocation` >= 5 AND `vocation` <= 8;";
			db->executeQuery(query.str());

			query.str("");
			query << "SELECT COUNT(`id`) AS `count` FROM `players` WHERE `vocation` > 4;";
			if((result = db->storeQuery(query.str())) && result->getDataInt("count"))
			{
				std::cout << "[Warning] There are still " << result->getDataInt("count") << " players with vocation above 4, please mind to update them manually." << std::endl;
				result->free();
			}

			query.str("");
			registerDatabaseConfig("db_version", 3);
			return 3;
		}

		case 3:
		{
			std::cout << "> Updating database to version: 4..." << std::endl;

			DBQuery query;
			query << "ALTER TABLE `houses` ADD `name` VARCHAR(255) NOT NULL;";
			db->executeQuery(query.str());

			query.str("");
			if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
			{
				query << "ALTER TABLE `houses` ADD `size` INTEGER NOT NULL DEFAULT 0;";
				db->executeQuery(query.str());

				query.str("");
				query << "ALTER TABLE `houses` ADD `town` INTEGER NOT NULL DEFAULT 0;";
				db->executeQuery(query.str());

				query.str("");
				query << "ALTER TABLE `houses` ADD `price` INTEGER NOT NULL DEFAULT 0;";
				db->executeQuery(query.str());

				query.str("");
				query << "ALTER TABLE `houses` ADD `rent` INTEGER NOT NULL DEFAULT 0;";
			}
			else
			{
				query << "ALTER TABLE `houses` ADD `size` INT UNSIGNED NOT NULL DEFAULT 0;";
				db->executeQuery(query.str());

				query.str("");
				query << "ALTER TABLE `houses` ADD `town` INT UNSIGNED NOT NULL DEFAULT 0;";
				db->executeQuery(query.str());

				query.str("");
				query << "ALTER TABLE `houses` ADD `price` INT UNSIGNED NOT NULL DEFAULT 0;";
				db->executeQuery(query.str());

				query.str("");
				query << "ALTER TABLE `houses` ADD `rent` INT UNSIGNED NOT NULL DEFAULT 0;";
			}

			db->executeQuery(query.str());
			query.str("");

			registerDatabaseConfig("db_version", 4);
			return 4;
		}

		case 4:
		{
			std::cout << "> Updating database to version: 5..." << std::endl;

			DBQuery query;
			query << "ALTER TABLE `player_deaths` ADD `altkilled_by` VARCHAR(255) NOT NULL;";
			db->executeQuery(query.str());

			query.str("");
			registerDatabaseConfig("db_version", 5);
			return 5;
		}

		case 5:
		{
			std::cout << "> Updating database to version: 6..." << std::endl;

			DBQuery query;
			switch(db->getDatabaseEngine())
			{
				case DATABASE_ENGINE_MYSQL:
				{
					query << "ALTER TABLE `global_storage` CHANGE `value` `value` VARCHAR(255) NOT NULL DEFAULT '0'";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `player_storage` CHANGE `value` `value` VARCHAR(255) NOT NULL DEFAULT '0'";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `tiles` CHANGE `x` `x` INT(5) UNSIGNED NOT NULL, CHANGE `y` `y` INT(5) UNSIGNED NOT NULL, CHANGE `z` `z` TINYINT(2) UNSIGNED NOT NULL;";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `tiles` ADD INDEX (`x`, `y`, `z`);";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `tile_items` ADD INDEX (`sid`);";
					break;
				}

				case DATABASE_ENGINE_SQLITE:
				{
					query << "ALTER TABLE `global_storage` RENAME TO `global_storage2`;";
					db->executeQuery(query.str());

					query.str("");
					query << "CREATE TABLE `global_storage` (`key` INTEGER NOT NULL, `value` VARCHAR(255) NOT NULL DEFAULT '0', UNIQUE (`key`));";
					db->executeQuery(query.str());

					query.str("");
					query << "INSERT INTO `global_storage` SELECT * FROM `global_storage2`;";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `player_storage` RENAME TO `player_storage2`;";
					db->executeQuery(query.str());

					query.str("");
					query << "CREATE TABLE `player_storage` (`player_id` INTEGER NOT NULL, `key` INTEGER NOT NULL, `value` VARCHAR(255) NOT NULL DEFAULT '0', UNIQUE (`player_id`, `key`), FOREIGN KEY (`player_id`) REFERENCES `players` (`id`));";
					db->executeQuery(query.str());

					query.str("");
					query << "INSERT INTO `player_storage` SELECT * FROM `player_storage2`;";
					break;
				}

				default:
					break;
			}

			std::string tmp = query.str();
			if(tmp.length() > 0)
			{
				db->executeQuery(tmp);
				query.str("");
			}

			registerDatabaseConfig("db_version", 6);
			return 6;
		}

		case 6:
		{
			std::cout << "> Updating database to version: 7..." << std::endl;
			if(g_config.getBool(ConfigManager::INGAME_GUILD_MANAGEMENT))
			{
				DBResult* result;

				DBQuery query;
				query << "SELECT `r`.`id`, `r`.`guild_id` FROM `guild_ranks` r LEFT JOIN `guilds` g ON `r`.`guild_id` = `g`.`id` WHERE `g`.`ownerid` = `g`.`id` AND `r`.`level` = 3;";
				if((result = db->storeQuery(query.str())))
				{
					do
					{
						query.str("");
						query << "UPDATE `guilds`, `players` SET `guilds`.`ownerid` = `players`.`id` WHERE `guilds`.`id` = " << result->getDataInt("guild_id") << " AND `players`.`rank_id` = " << result->getDataInt("id") << ";";
						db->executeQuery(query.str());
					}
					while(result->next());
					result->free();
				}

				query.str("");
			}

			registerDatabaseConfig("db_version", 7);
			return 7;
		}

		case 7:
		{
			//DBResult* result;
			std::cout << "> Updating database version to: 8..." << std::endl;

			DBQuery query;
			switch(db->getDatabaseEngine())
			{
				case DATABASE_ENGINE_MYSQL:
				{
					std::string queryList[] = {
						"ALTER TABLE `server_motd` CHANGE `id` `id` INT UNSIGNED NOT NULL;",
						"ALTER TABLE `server_motd` DROP PRIMARY KEY;",
						"ALTER TABLE `server_motd` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `server_motd` ADD UNIQUE (`id`, `world_id`);",
						"ALTER TABLE `server_record` DROP PRIMARY KEY;",
						"ALTER TABLE `server_record` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `server_record` ADD UNIQUE (`timestamp`, `record`, `world_id`);",
						"ALTER TABLE `server_reports` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `server_reports` ADD INDEX (`world_id`);",
						"ALTER TABLE `players` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `global_storage` DROP PRIMARY KEY;",
						"ALTER TABLE `global_storage` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `global_storage` ADD UNIQUE (`key`, `world_id`);",
						"ALTER TABLE `guilds` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `guilds` ADD UNIQUE (`name`, `world_id`);",
						"ALTER TABLE `house_lists` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `house_lists` ADD UNIQUE (`house_id`, `world_id`, `listid`);",
						"ALTER TABLE `houses` CHANGE `id` `id` INT NOT NULL;",
						"ALTER TABLE `houses` DROP PRIMARY KEY;",
						"ALTER TABLE `houses` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `houses` ADD UNIQUE (`id`, `world_id`);",
						"ALTER TABLE `tiles` CHANGE `id` `id` INT NOT NULL;",
						"ALTER TABLE `tiles` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `tiles` ADD UNIQUE (`id`, `world_id`);",
						"ALTER TABLE `tile_items` ADD `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0;",
						"ALTER TABLE `tile_items` ADD UNIQUE (`tile_id`, `world_id`, `sid`);"
					};

					for(uint32_t i = 0; i < sizeof(queryList) / sizeof(std::string); i++)
					{
						query << queryList[i];
						db->executeQuery(query.str());
						query.str("");
					}
					break;
				}

				case DATABASE_ENGINE_SQLITE:
				case DATABASE_ENGINE_POSTGRESQL:
				default:
				{
					//TODO
					break;
				}
			}

			registerDatabaseConfig("db_version", 8);
			return 8;
		}

		case 8:
		{
			std::cout << "> Updating database to version: 9..." << std::endl;

			DBQuery query;
			query << "ALTER TABLE `bans` ADD `statement` VARCHAR(255) NOT NULL;";
			db->executeQuery(query.str());

			query.str("");
			registerDatabaseConfig("db_version", 9);
			return 9;
		}

		case 9:
		{
			std::cout << "> Updating database to version: 10..." << std::endl;
			registerDatabaseConfig("db_version", 10);
			return 10;
		}

		case 10:
		{
			std::cout << "> Updating database to version: 11..." << std::endl;

			DBQuery query;
			query << "ALTER TABLE `players` ADD `description` VARCHAR(255) NOT NULL DEFAULT '';";
			db->executeQuery(query.str());

			query.str("");
			if(!tableExists("house_storage"))
			{
				query << "CREATE TABLE `house_data` (`house_id` INT UNSIGNED NOT NULL, `world_id` TINYINT(2) UNSIGNED NOT NULL DEFAULT 0, ";
				query << "`data` LONGBLOB NOT NULL, UNIQUE (`house_id`, `world_id`)";
				if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
					query << ", FOREIGN KEY (`house_id`) REFERENCES `houses` (`id`) ON DELETE CASCADE";

				query << ")";
				if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
					query << " ENGINE = InnoDB";

				query << ";";
			}
			else
				query << "ALTER TABLE `house_storage` RENAME TO `house_data`;";

			db->executeQuery(query.str());
			query.str("");

			registerDatabaseConfig("db_version", 11);
			return 11;
		}

		case 11:
		{
			//DBResult* result;
			std::cout << "> Updating database to version: 12..." << std::endl;

			DBQuery query;
			query << "UPDATE `players` SET `stamina` = 151200000 WHERE `stamina` > 151200000;";
			db->executeQuery(query.str());

			query.str("");
			query << "UPDATE `players` SET `loss_experience` = `loss_experience` * 10, `loss_mana` = `loss_mana` * 10,";
			query << "`loss_skills` = `loss_skills` * 10, `loss_items` = `loss_items` * 10;";
			db->executeQuery(query.str());

			query.str("");
			switch(db->getDatabaseEngine())
			{
				case DATABASE_ENGINE_MYSQL:
				{
					query << "ALTER TABLE `players` CHANGE `stamina` `stamina` INT NOT NULL DEFAULT 151200000;";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `players` CHANGE `loss_experience` `loss_experience` INT NOT NULL DEFAULT 100;";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `players` CHANGE `loss_mana` `loss_mana` INT NOT NULL DEFAULT 100;";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `players` CHANGE `loss_skills` `loss_skills` INT NOT NULL DEFAULT 100;";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `players` CHANGE `loss_items` `loss_items` INT NOT NULL DEFAULT 100;";
					db->executeQuery(query.str());

					query.str("");
					query << "ALTER TABLE `players` ADD `loss_containers` INT NOT NULL DEFAULT 100 AFTER `loss_skills`;";
					break;
				}

				case DATABASE_ENGINE_SQLITE:
				case DATABASE_ENGINE_POSTGRESQL:
				default:
				{
					//TODO
					break;
				}
			}

			std::string tmp = query.str();
			if(tmp.length() > 0)
			{
				db->executeQuery(tmp);
				query.str("");
			}

			registerDatabaseConfig("db_version", 12);
			return 12;
		}

		case 12:
		{
			std::cout << "> Updating database to version: 13..." << std::endl;

			DBQuery query;
			query << "ALTER TABLE `accounts` DROP KEY `group_id`;";
			db->executeQuery(query.str());

			query.str("");
			query << "ALTER TABLE `players` DROP KEY `group_id`;";
			db->executeQuery(query.str());

			query.str("");
			query << "DROP TABLE `groups`;";
			db->executeQuery(query.str());

			registerDatabaseConfig("db_version", 13);
			return 13;
		}

		default:
			break;
	}

	return 0;
}

bool DatabaseManager::getDatabaseConfig(std::string config, int32_t &value)
{
	value = 0;
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `value` FROM `server_config` WHERE `config` = " << db->escapeString(config) << ";";
	if(!(result = db->storeQuery(query.str())))
		return false;

	value = result->getDataInt("value");
	result->free();
	return true;
}

void DatabaseManager::registerDatabaseConfig(std::string config, int32_t value)
{
	Database* db = Database::getInstance();
	DBQuery query;

	int32_t tmp = 0;
	if(!getDatabaseConfig(config, tmp))
		query << "INSERT INTO `server_config` VALUES (" << db->escapeString(config) << ", " << value << ");";
	else
		query << "UPDATE `server_config` SET `value` = " << value << " WHERE `config` = " << db->escapeString(config) << ";";

	db->executeQuery(query.str());
}

void DatabaseManager::checkPasswordType()
{
	PasswordType_t newValue = (PasswordType_t)g_config.getNumber(ConfigManager::PASSWORDTYPE);
	int32_t value = (int32_t)PASSWORD_TYPE_PLAIN;
	if(getDatabaseConfig("password_type", value))
	{
		if(newValue != (PasswordType_t)value)
		{
			switch(newValue)
			{
				case PASSWORD_TYPE_MD5:
				{
					if((PasswordType_t)value != PASSWORD_TYPE_PLAIN)
					{
						std::cout << "> WARNING: You cannot change the passwordType to MD5, change it back in config.lua to \"sha1\"." << std::endl;
						return;
					}

					registerDatabaseConfig("password_type", (int32_t)newValue);
					Database* db = Database::getInstance();

					DBQuery query;
					if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL || db->getDatabaseEngine() == DATABASE_ENGINE_POSTGRESQL)
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
							result->free();
						}
					}

					std::cout << "> All passwords are now MD5 hashed." << std::endl;
					break;
				}

				case PASSWORD_TYPE_SHA1:
				{
					if((PasswordType_t)value != PASSWORD_TYPE_PLAIN)
					{
						std::cout << "> WARNING: You cannot change the passwordType to SHA1, change it back in config.lua to \"md5\"." << std::endl;
						return;
					}

					registerDatabaseConfig("password_type", (int32_t)newValue);
					Database* db = Database::getInstance();

					DBQuery query;
					if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL || db->getDatabaseEngine() == DATABASE_ENGINE_POSTGRESQL)
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
							result->free();
						}
					}

					std::cout << "> All passwords are now SHA1 hashed." << std::endl;
					break;
				}

				default:
				{
					std::cout << "> WARNING: You cannot switch from hashed passwords to plain text, change back the passwordType in config.lua to the passwordType you were previously using." << std::endl;
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
	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_MYSQL:
		{
			std::string triggerName[5] =
			{
				"ondelete_accounts",
				"oncreate_guilds",
				"ondelete_guilds",
				"oncreate_players",
				"ondelete_players",
			};

			std::string triggerStatement[5] =
			{
				"CREATE TRIGGER `ondelete_accounts` BEFORE DELETE ON `accounts` FOR EACH ROW BEGIN DELETE FROM `bans` WHERE `type` != 1 AND `type` != 2 AND `value` = OLD.`id`; END;",
				"CREATE TRIGGER `oncreate_guilds` AFTER INSERT ON `guilds` FOR EACH ROW BEGIN INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('the Leader', 3, NEW.`id`); INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('a Vice-Leader', 2, NEW.`id`); INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('a Member', 1, NEW.`id`); END;",
				"CREATE TRIGGER `ondelete_guilds` BEFORE DELETE ON `guilds` FOR EACH ROW BEGIN UPDATE `players` SET `guildnick` = '', `rank_id` = 0 WHERE `rank_id` IN (SELECT `id` FROM `guild_ranks` WHERE `guild_id` = OLD.`id`); END;",
				"CREATE TRIGGER `oncreate_players` AFTER INSERT ON `players` FOR EACH ROW BEGIN INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 0, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 1, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 2, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 3, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 4, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 5, 10); INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 6, 10); END;",
				"CREATE TRIGGER `ondelete_players` BEFORE DELETE ON `players` FOR EACH ROW BEGIN DELETE FROM `bans` WHERE `type` = 2 AND `value` = OLD.`id`; UPDATE `houses` SET `owner` = 0 WHERE `owner` = OLD.`id`; END;"
			};

			for(int32_t i = 0; i < 5; i++)
			{
				if(!triggerExists(triggerName[i]))
				{
					std::cout << "> Trigger: " << triggerName[i] << " does not exist, creating it..." << std::endl;
					db->executeQuery(triggerStatement[i]);
				}
			}

			break;
		}

		case DATABASE_ENGINE_SQLITE:
		{
			std::string triggerName[27] =
			{
				"oncreate_guilds",
				"oncreate_players",
				"ondelete_accounts",
				"ondelete_players",
				"ondelete_guilds",
				"oninsert_players",
				"onupdate_players",
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

			std::string triggerStatement[27] =
			{
				"CREATE TRIGGER \"oncreate_guilds\" AFTER INSERT ON \"guilds\" BEGIN INSERT INTO \"guild_ranks\" (\"name\", \"level\", \"guild_id\") VALUES (\"the Leader\", 3, NEW.\"id\"); INSERT INTO \"guild_ranks\" (\"name\", \"level\", \"guild_id\") VALUES (\"a Vice-Leader\", 2, NEW.\"id\"); INSERT INTO \"guild_ranks\" (\"name\", \"level\", \"guild_id\") VALUES (\"a Member\", 1, NEW.\"id\"); END;",

				"CREATE TRIGGER \"oncreate_players\" AFTER INSERT ON \"players\" BEGIN INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 0, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 1, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 2, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 3, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 4, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 5, 10); INSERT INTO \"player_skills\" (\"player_id\", \"skillid\", \"value\") VALUES (NEW.\"id\", 6, 10); END;",
				"CREATE TRIGGER \"ondelete_accounts\" BEFORE DELETE ON \"accounts\" FOR EACH ROW BEGIN DELETE FROM \"players\" WHERE \"account_id\" = OLD.\"id\"; DELETE FROM \"bans\" WHERE \"type\" != 1 AND \"type\" != 2 AND \"value\" = OLD.\"id\"; END;",

				"CREATE TRIGGER \"ondelete_players\" BEFORE DELETE ON \"players\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'DELETE on table \"players\" violates foreign: \"ownerid\" from table \"guilds\"') WHERE (SELECT \"id\" FROM \"guilds\" WHERE \"ownerid\" = OLD.\"id\") IS NOT NULL; DELETE FROM \"player_viplist\" WHERE \"player_id\" = OLD.\"id\" OR \"vip_id\" = OLD.\"id\"; DELETE FROM \"player_storage\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"player_skills\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"player_items\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"player_depotitems\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"player_spells\" WHERE \"player_id\" = OLD.\"id\"; DELETE FROM \"bans\" WHERE \"type\" = 2 AND \"value\" = OLD.\"id\"; UPDATE \"houses\" SET \"owner\" = 0 WHERE \"owner\" = OLD.\"id\"; END;",
				"CREATE TRIGGER \"ondelete_guilds\" BEFORE DELETE ON \"guilds\" FOR EACH ROW BEGIN UPDATE \"players\" SET \"guildnick\" = '', \"rank_id\" = 0 WHERE \"rank_id\" IN (SELECT \"id\" FROM \"guild_ranks\" WHERE \"guild_id\" = OLD.\"id\"); DELETE FROM \"guild_ranks\" WHERE \"guild_id\" = OLD.\"id\"; END;",

				"CREATE TRIGGER \"oninsert_players\" BEFORE INSERT ON \"players\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'INSERT on table \"players\" violates foreign: \"account_id\"') WHERE NEW.\"account_id\" IS NULL OR (SELECT \"id\" FROM \"accounts\" WHERE \"id\" = NEW.\"account_id\") IS NULL; END;",
				"CREATE TRIGGER \"onupdate_players\" BEFORE UPDATE ON \"players\" FOR EACH ROW BEGIN SELECT RAISE(ROLLBACK, 'UPDATE on table \"players\" violates foreign: \"account_id\"') WHERE NEW.\"account_id\" IS NULL OR (SELECT \"id\" FROM \"accounts\" WHERE \"id\" = NEW.\"account_id\") IS NULL; END;",

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

			for(int32_t i = 0; i < 27; i++)
			{
				if(!triggerExists(triggerName[i]))
				{
					std::cout << "> Trigger: " << triggerName[i] << " does not exist, creating it..." << std::endl;
					db->executeQuery(triggerStatement[i]);
				}
			}

			break;
		}

		case DATABASE_ENGINE_POSTGRESQL:
			//TODO: PostgreSQL support
			break;

		default:
			break;
	}
}
