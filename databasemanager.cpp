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
#include <stack>

#include "databasemanager.h"
#include "tools.h"

#include "ban.h"

#include "configmanager.h"
extern ConfigManager g_config;

bool DatabaseManager::optimizeTables()
{
	Database* db = Database::getInstance();
	DBQuery query;

	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_MYSQL:
		{
			query << "SELECT `TABLE_NAME` FROM `information_schema`.`TABLES` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::MYSQL_DB)) << " AND `DATA_FREE` > 0;";
			DBResult* result = db->storeQuery(query.str());
			if(!result)
				return false;

			do
			{
				std::string tableName = result->getDataString("TABLE_NAME");
				std::cout << "> Optimizing table " << tableName << "..." << std::flush;

				query.str("");
				query << "OPTIMIZE TABLE `" << tableName << "`;";
				if(db->executeQuery(query.str()))
					std::cout << " [success]" << std::endl;
				else
					std::cout << " [failed]" << std::endl;
			}
			while(result->next());
			db->freeResult(result);
			break;
		}

		case DATABASE_ENGINE_SQLITE:
		{
			if(!db->executeQuery("VACUUM;"))
				return false;

			std::cout << "> Optimized database." << std::endl;
			break;
		}

		default:
		{
			std::cout << "> Optimization is not supported for this database engine." << std::endl;
			break;
		}
	}
	return true;
}

bool DatabaseManager::triggerExists(const std::string& triggerName)
{
	Database* db = Database::getInstance();
	DBQuery query;

	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_MYSQL:
			query << "SELECT `name` FROM `sqlite_master` WHERE `type` = 'trigger' AND `name` = " << db->escapeString(triggerName) << ";";
			break;

		case DATABASE_ENGINE_SQLITE:
			query << "SELECT `TRIGGER_NAME` FROM `information_schema`.`TRIGGERS` WHERE `TRIGGER_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::SQLITE_DB)) << " AND `TRIGGER_NAME` = " << db->escapeString(triggerName) << ";";
			break;

		default:
			return false;
	}

	DBResult* result = db->storeQuery(query.str());
	if(!result)
		return false;

	db->freeResult(result);
	return true;
}

bool DatabaseManager::tableExists(const std::string& tableName)
{
	Database* db = Database::getInstance();
	DBQuery query;
	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_MYSQL:
			query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::MYSQL_DB)) << " AND `TABLE_NAME` = " << db->escapeString(tableName) << ";";
			break;

		case DATABASE_ENGINE_SQLITE:
			query << "SELECT `name` FROM `sqlite_master` WHERE `type` = 'table' AND `name` = " << db->escapeString(tableName) << ";";
			break;

		default:
			return false;
	}

	DBResult* result = db->storeQuery(query.str());
	if(!result)
		return false;

	db->freeResult(result);
	return true;
}

bool DatabaseManager::isDatabaseSetup()
{
	Database* db = Database::getInstance();
	DBQuery query;
	switch(db->getDatabaseEngine())
	{
		case DATABASE_ENGINE_MYSQL:
		{
			query << "SELECT `TABLE_NAME` FROM `information_schema`.`tables` WHERE `TABLE_SCHEMA` = " << db->escapeString(g_config.getString(ConfigManager::MYSQL_DB)) << ";";
			break;
		}

		case DATABASE_ENGINE_SQLITE:
		{
			query.str("SELECT `name` FROM `sqlite_master` WHERE `type` = 'table';");
			break;
		}

		default:
			return false;
	}

	DBResult* result = db->storeQuery(query.str());
	if(!result)
		return false;

	db->freeResult(result);
	return true;
}

int32_t DatabaseManager::getDatabaseVersion()
{
	if(!tableExists("server_config"))
	{
		Database* db = Database::getInstance();
		if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
			db->executeQuery("CREATE TABLE `server_config` (`config` VARCHAR(50) NOT NULL, `value` VARCHAR(256) NOT NULL DEFAULT '', UNIQUE(`config`)) ENGINE = InnoDB;");
		else
			db->executeQuery("CREATE TABLE `server_config` (`config` VARCHAR(50) NOT NULL, `value` VARCHAR(256) NOT NULL DEFAULT '', UNIQUE(`config`));");

		db->executeQuery("INSERT INTO `server_config` VALUES ('db_version', 0);");
		return 0;
	}

	int32_t version = 0;
	if(getDatabaseConfig("db_version", version))
		return version;

	return -1;
}

uint32_t DatabaseManager::updateDatabase()
{
	Database* db = Database::getInstance();
	DBQuery query;

	int32_t databaseVersion = getDatabaseVersion();
	if(databaseVersion < 0)
		return 0;

	switch(databaseVersion)
	{
		case 0:
		{
			std::cout << "> Updating database to version 1 (account names)" << std::endl;
			if (db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
				db->executeQuery("ALTER TABLE `accounts` ADD `name` VARCHAR(32) NOT NULL AFTER `id`;");
			else
				db->executeQuery("ALTER TABLE `accounts` ADD `name` VARCHAR(32) NOT NULL DEFAULT '';");

			db->executeQuery("UPDATE `accounts` SET `name` = `id`;");

			if (db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
				db->executeQuery("ALTER TABLE `accounts` ADD UNIQUE (`name`);");
			else
				db->executeQuery("CREATE UNIQUE INDEX IF NOT EXISTS account_name ON accounts(name);");

			registerDatabaseConfig("db_version", 1);
			return 1;
		}

		case 1:
		{
			std::cout << "> Updating database to version 2 (market offers)" << std::endl;
			if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
			{
				db->executeQuery("CREATE TABLE `market_offers` (`id` INT UNSIGNED NOT NULL AUTO_INCREMENT, `player_id` INT NOT NULL, `sale` TINYINT(1) NOT NULL DEFAULT 0, `itemtype` INT UNSIGNED NOT NULL, `amount` SMALLINT UNSIGNED NOT NULL, `created` BIGINT UNSIGNED NOT NULL, `anonymous` TINYINT(1) NOT NULL DEFAULT 0, `price` INT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY (`id`), KEY(`sale`, `itemtype`), KEY(`created`), FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE) ENGINE = InnoDB;");
			}
			else
			{
				db->executeQuery("CREATE TABLE `market_offers` (`id` INTEGER PRIMARY KEY NOT NULL, `player_id` INTEGER NOT NULL, `sale` BOOLEAN NOT NULL DEFAULT 0, `itemtype` UNSIGNED INTEGER NOT NULL, `amount` UNSIGNED INTEGER NOT NULL, `created` UNSIGNED INTEGER NOT NULL, `anonymous` BOOLEAN NOT NULL DEFAULT 0, `price` UNSIGNED INTEGER NOT NULL DEFAULT 0, FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE);");
				db->executeQuery("CREATE INDEX market_offers_idx ON market_offers(created);");
				db->executeQuery("CREATE INDEX market_offers_idx2 ON market_offers(sale, itemtype);");
			}

			registerDatabaseConfig("db_version", 2);
			return 2;
		}

		case 2:
		{
			std::cout << "> Updating database to version 3 (bank balance)" << std::endl;
			if(db->getDatabaseEngine() == DATABASE_ENGINE_SQLITE)
				db->executeQuery("DROP TRIGGER IF EXISTS `onupdate_players_after`;");

			db->executeQuery("ALTER TABLE `players` ADD `balance` BIGINT UNSIGNED NOT NULL DEFAULT 0;");
			registerDatabaseConfig("db_version", 3);
			return 3;
		}

		case 3:
		{
			std::cout << "> Updating database to version 4 (market history)" << std::endl;
			if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
				db->executeQuery("CREATE TABLE `market_history` (`id` INT UNSIGNED NOT NULL AUTO_INCREMENT, `player_id` INT NOT NULL, `sale` TINYINT(1) NOT NULL DEFAULT 0, `itemtype` INT UNSIGNED NOT NULL, `amount` SMALLINT UNSIGNED NOT NULL, `price` INT UNSIGNED NOT NULL DEFAULT 0, `expires_at` BIGINT UNSIGNED NOT NULL, `inserted` BIGINT UNSIGNED NOT NULL, `state` TINYINT(1) UNSIGNED NOT NULL, PRIMARY KEY(`id`), KEY(`player_id`, `sale`), FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE) ENGINE = InnoDB;");
			else
			{
				db->executeQuery("CREATE TABLE `market_history` (`id` INTEGER PRIMARY KEY NOT NULL, `player_id` INTEGER NOT NULL, `sale` BOOLEAN NOT NULL DEFAULT 0, `itemtype` UNSIGNED INTEGER NOT NULL, `amount` UNSIGNED INTEGER NOT NULL, `price` UNSIGNED INTEGER NOT NULL DEFAULT 0, `expires_at` UNSIGNED INTEGER NOT NULL, `inserted` UNSIGNED INTEGER NOT NULL, `state` UNSIGNED INTEGER NOT NULL, FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE);");
				db->executeQuery("CREATE INDEX market_history_idx ON market_history(player_id, sale);");
			}

			registerDatabaseConfig("db_version", 4);
			return 4;
		}

		case 4:
		{
			std::cout << "> Updating database to version 5 (black skull & guild wars)" << std::endl;
			if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
			{
				db->executeQuery("ALTER TABLE `players` CHANGE `redskull` `skull` TINYINT(1) NOT NULL DEFAULT '0', CHANGE `redskulltime` `skulltime` INT(11) NOT NULL DEFAULT '0';");
				db->executeQuery("CREATE TABLE IF NOT EXISTS `guild_wars` ( `id` int(11) NOT NULL AUTO_INCREMENT, `guild1` int(11) NOT NULL DEFAULT '0', `guild2` int(11) NOT NULL DEFAULT '0', `name1` varchar(255) NOT NULL, `name2` varchar(255) NOT NULL, `status` tinyint(2) NOT NULL DEFAULT '0', `started` bigint(15) NOT NULL DEFAULT '0', `ended` bigint(15) NOT NULL DEFAULT '0', PRIMARY KEY (`id`), KEY `guild1` (`guild1`), KEY `guild2` (`guild2`)) ENGINE=InnoDB;");
				db->executeQuery("CREATE TABLE IF NOT EXISTS `guildwar_kills` (`id` int(11) NOT NULL AUTO_INCREMENT, `killer` varchar(50) NOT NULL, `target` varchar(50) NOT NULL, `killerguild` int(11) NOT NULL DEFAULT '0', `targetguild` int(11) NOT NULL DEFAULT '0', `warid` int(11) NOT NULL DEFAULT '0', `time` bigint(15) NOT NULL, PRIMARY KEY (`id`), KEY `warid` (`warid`), FOREIGN KEY (`warid`) REFERENCES `guild_wars`(`id`) ON DELETE CASCADE) ENGINE=InnoDB;");
			}
			else
			{
				db->executeQuery("ALTER TABLE `players` ADD `skull` INTEGER NOT NULL DEFAULT 0;");
				db->executeQuery("ALTER TABLE `players` ADD `skulltime` INTEGER NOT NULL DEFAULT 0;");
				db->executeQuery("CREATE TABLE IF NOT EXISTS `guild_wars` ( `id` INTEGER PRIMARY KEY NOT NULL, `guild1` INTEGER NOT NULL DEFAULT '0', `guild2` INTEGER NOT NULL DEFAULT '0', `name1` VARCHAR(255) NOT NULL, `name2` VARCHAR(255) NOT NULL, `status` INTEGER NOT NULL DEFAULT '0', `started` INTEGER NOT NULL DEFAULT '0', `ended` INTEGER NOT NULL DEFAULT '0');");
				db->executeQuery("CREATE TABLE IF NOT EXISTS `guildwar_kills` (`id` INTEGER PRIMARY KEY NOT NULL, `killer` varchar(50) NOT NULL, `target` varchar(50) NOT NULL, `killerguild` INTEGER NOT NULL DEFAULT '0', `targetguild` INTEGER NOT NULL DEFAULT '0', `warid` INTEGER NOT NULL DEFAULT '0', `time` INTEGER NOT NULL, FOREIGN KEY (`warid`) REFERENCES `guild_wars` (`id`));");
				db->executeQuery("CREATE INDEX guild_wars_idx ON guild_wars(guild1);");
				db->executeQuery("CREATE INDEX guild_wars_idx2 ON guild_wars(guild2);");
			}

			registerDatabaseConfig("db_version", 5);
			return 5;
		}

		case 5:
		{
			std::cout << "> Updating database to version 6 (market bug fix)" << std::endl;
			db->executeQuery("DELETE FROM `market_offers` WHERE `amount` = 0;");
			registerDatabaseConfig("db_version", 6);
			return 6;
		}

		case 6:
		{
			std::cout << "> Updating database to version 7 (offline training)" << std::endl;
			db->executeQuery("ALTER TABLE `players` ADD `offlinetraining_time` SMALLINT UNSIGNED NOT NULL DEFAULT 43200;");
			db->executeQuery("ALTER TABLE `players` ADD `offlinetraining_skill` INT NOT NULL DEFAULT -1;");
			registerDatabaseConfig("db_version", 7);
			return 7;
		}

		case 7:
		{
			if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
			{
				std::cout << "> Updating database to version 8 (account viplist with description, icon and notify server side)" << std::endl;
				db->executeQuery("RENAME TABLE `player_viplist` TO `account_viplist`;");
				db->executeQuery("ALTER TABLE `account_viplist` DROP FOREIGN KEY `account_viplist_ibfk_1`;");
				db->executeQuery("UPDATE `account_viplist` SET `player_id` = (SELECT `account_id` FROM `players` WHERE `id` = `player_id`);");
				db->executeQuery("ALTER TABLE `account_viplist` CHANGE `player_id` `account_id` INT( 11 ) NOT NULL COMMENT 'id of account whose viplist entry it is';");
				db->executeQuery("ALTER TABLE `account_viplist` DROP FOREIGN KEY `account_viplist_ibfk_2`;");
				db->executeQuery("ALTER TABLE `account_viplist` CHANGE `vip_id` `player_id` INT( 11 ) NOT NULL COMMENT 'id of target player of viplist entry';");
				db->executeQuery("ALTER TABLE `account_viplist` DROP INDEX `player_id`, ADD INDEX `account_id` (`account_id`);");
				db->executeQuery("ALTER TABLE `account_viplist` DROP INDEX `vip_id`, ADD INDEX `player_id` (`player_id`);");
				db->executeQuery("ALTER TABLE `account_viplist` ADD FOREIGN KEY (`account_id`) REFERENCES `accounts` (`id`) ON DELETE CASCADE;");
				db->executeQuery("ALTER TABLE `account_viplist` ADD FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE;");
				db->executeQuery("ALTER TABLE `account_viplist` ADD `description` VARCHAR(128) NOT NULL DEFAULT '', ADD `icon` TINYINT( 2 ) UNSIGNED NOT NULL DEFAULT '0', ADD `notify` TINYINT( 1 ) NOT NULL DEFAULT '0';");

				// Remove duplicates
				DBResult* result = db->storeQuery("SELECT `account_id`, `player_id`, COUNT(*) AS `count` FROM `account_viplist` GROUP BY `account_id`, `player_id` HAVING COUNT(*) > 1;");
				if(result)
				{
					do
					{
						query.str("");
						query << "DELETE FROM `account_viplist` WHERE `account_id` = " << result->getDataInt("account_id") << " AND `player_id` = " << result->getDataInt("player_id") << " LIMIT " << (result->getDataInt("count") - 1) << ";";
						db->executeQuery(query.str());
					}
					while(result->next());
					db->freeResult(result);
				}

				// Remove if an account has over 200 entries
				result = db->storeQuery("SELECT `account_id`, COUNT(*) AS `count` FROM `account_viplist` GROUP BY `account_id` HAVING COUNT(*) > 200;");
				if(result)
				{
					do
					{
						query.str("");
						query << "DELETE FROM `account_viplist` WHERE `account_id` = " << result->getDataInt("account_id") << " LIMIT " << (result->getDataInt("count") - 200) << ";";
						db->executeQuery(query.str());
					}
					while(result->next());
					db->freeResult(result);
				}

				db->executeQuery("ALTER TABLE `account_viplist` ADD UNIQUE `account_player_index` (`account_id`, `player_id`);");

				registerDatabaseConfig("db_version", 8);
				return 8;
			}
			break;
		}

		case 8:
		{
			if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
			{
				std::cout << "> Updating database to version 9 (global inbox)" << std::endl;
				db->executeQuery("CREATE TABLE IF NOT EXISTS `player_inboxitems` (`player_id` int(11) NOT NULL, `sid` int(11) NOT NULL, `pid` int(11) NOT NULL DEFAULT '0', `itemtype` smallint(6) NOT NULL, `count` smallint(5) NOT NULL DEFAULT '0', `attributes` blob NOT NULL, UNIQUE KEY `player_id_2` (`player_id`,`sid`), KEY `player_id` (`player_id`), FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=latin1;");

				// Delete "market" item
				db->executeQuery("DELETE FROM `player_depotitems` WHERE `itemtype` = 14405;");

				// Move up items in depot chests
				DBResult* result = db->storeQuery("SELECT `player_id`, `pid`, (SELECT `dp2`.`sid` FROM `player_depotitems` AS `dp2` WHERE `dp2`.`player_id` = `dp1`.`player_id` AND `dp2`.`pid` = `dp1`.`sid` AND `itemtype` = 2594) AS `sid` FROM `player_depotitems` AS `dp1` WHERE `itemtype` = 2589;");
				if(result)
				{
					do
					{
						query.str("");
						query << "UPDATE `player_depotitems` SET `pid` = " << result->getDataInt("pid") << " WHERE `player_id` = " << result->getDataInt("player_id") << " AND `pid` = " << result->getDataInt("sid") << ";";
						db->executeQuery(query.str());
					}
					while(result->next());
					db->freeResult(result);
				}

				// Delete the depot lockers
				db->executeQuery("DELETE FROM `player_depotitems` WHERE `itemtype` = 2589;");

				// Delete the depot chests
				db->executeQuery("DELETE FROM `player_depotitems` WHERE `itemtype` = 2594;");

				std::ostringstream ss2;

				result = db->storeQuery("SELECT DISTINCT `player_id` FROM `player_depotitems` WHERE `itemtype` = 14404;");
				if(result)
				{
					do
					{
						int32_t runningId = 100;

						DBInsert stmt(db);
						stmt.setQuery("INSERT INTO `player_inboxitems` (`player_id`, `sid`, `pid`, `itemtype`, `count`, `attributes`) VALUES ");

						std::ostringstream sss;
						sss << "SELECT `sid` FROM `player_depotitems` WHERE `player_id` = " << result->getDataInt("player_id") << " AND `itemtype` = 14404;";
						DBResult* result2 = db->storeQuery(sss.str());
						if(result2)
						{
							do
							{
								std::stack<int32_t> sids;
								sids.push(result2->getDataInt("sid"));
								while(!sids.empty())
								{
									int32_t sid = sids.top();
									sids.pop();

									std::ostringstream ss;
									ss << "SELECT * FROM `player_depotitems` WHERE `player_id` = " << result->getDataInt("player_id") << " AND `pid` = " << sid << ";";
									DBResult* result3 = db->storeQuery(ss.str());
									if(result3)
									{
										do
										{
											unsigned long attrSize = 0;
											const char* attr = result3->getDataStream("attributes", attrSize);
											ss2 << result->getDataInt("player_id") << "," << ++runningId << ",0," << result3->getDataInt("itemtype") << "," << result3->getDataInt("count") << "," << db->escapeBlob(attr, attrSize);
											if(!stmt.addRow(ss2))
												std::cout << "Failed to add row!" << std::endl;

											sids.push(result3->getDataInt("sid"));

											std::ostringstream tmpss;
											tmpss << "DELETE FROM `player_depotitems` WHERE `player_id` = " << result->getDataInt("player_id") << " AND `sid` = " << result3->getDataInt("sid") << ";";
											db->executeQuery(tmpss.str());
										}
										while(result3->next());
										db->freeResult(result3);
									}
								}
							}
							while(result2->next());
							db->freeResult(result2);
						}

						if (!stmt.execute())
							std::cout << "Failed to execute statement!" << std::endl;
					}
					while(result->next());
					db->freeResult(result);
				}

				// Delete the inboxes
				db->executeQuery("DELETE FROM `player_depotitems` WHERE `itemtype` = 14404;");

				registerDatabaseConfig("db_version", 9);
				return 9;
			}
			break;
		}

		case 9:
		{
			std::cout << "> Updating database to version 10 (stamina)" << std::endl;
			db->executeQuery("ALTER TABLE `players` ADD `stamina` SMALLINT UNSIGNED NOT NULL DEFAULT 2520;");
			registerDatabaseConfig("db_version", 10);
			return 10;
		}

		/*
		case ?-1:
		{
			std::cout << "> Updating database to version ?" << std::endl;
			if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
			{
				db->executeQuery("CREATE TABLE IF NOT EXISTS `bans2` (`id` INT UNSIGNED NOT NULL AUTO_INCREMENT, `type` TINYINT(1) NOT NULL COMMENT 'this field defines if its ip, account, player, or any else ban', `value` INT UNSIGNED NOT NULL COMMENT 'ip, player guid, account number', `param` INT UNSIGNED NOT NULL DEFAULT 4294967295 COMMENT 'mask', `active` TINYINT(1) NOT NULL DEFAULT 1, `expires` INT UNSIGNED NOT NULL DEFAULT 0, `added` INT UNSIGNED NOT NULL, `admin_id` INT UNSIGNED NOT NULL DEFAULT 0, `comment` VARCHAR(1024) NOT NULL DEFAULT '', `reason` INT UNSIGNED NOT NULL DEFAULT 0, `action` INT UNSIGNED NOT NULL DEFAULT 0, `statement` VARCHAR(256) NOT NULL DEFAULT '', PRIMARY KEY (`id`), INDEX `type` (`type`, `value`), INDEX `active` (`active`)) ENGINE = InnoDB;");
			}
			else
			{
				db->executeQuery("CREATE TABLE IF NOT EXISTS `bans2` (`id` INTEGER NOT NULL, `type` UNSIGNED INTEGER NOT NULL, `value` UNSIGNED INTEGER NOT NULL, `param` UNSIGNED INTEGER NOT NULL DEFAULT 4294967295, `active` UNSIGNED INTEGER NOT NULL DEFAULT 1, `expires` UNSIGNED INTEGER NOT NULL DEFAULT 0, `added` UNSIGNED INTEGER NOT NULL, `admin_id` UNSIGNED INTEGER NOT NULL DEFAULT 0, `comment` VARCHAR(1024) NOT NULL DEFAULT '', `reason` UNSIGNED INTEGER NOT NULL DEFAULT 0, `action` UNSIGNED INTEGER NOT NULL DEFAULT 0, `statement` VARCHAR(256) NOT NULL DEFAULT '', PRIMARY KEY (`id`));");
				db->executeQuery("CREATE INDEX bans_index_type ON bans2(type, value);");
				db->executeQuery("CREATE INDEX bans_index_active ON bans2(active);");
			}

			DBResult* result = db->storeQuery("SELECT * FROM `bans`;");
			if(result)
			{
				do
				{
					switch(result->getDataInt("type"))
					{
						case BAN_IPADDRESS:
							query << "INSERT INTO `bans2` (`type`, `value`, `param`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (1, " << result->getDataInt("ip") << ", " << result->getDataInt("mask") << ", " << (result->getDataInt("time") <= time(NULL) ? 0 : 1) << ", " << result->getDataInt("time") << ", 0, " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
							break;

						case NAMELOCK_PLAYER:
							query << "INSERT INTO `bans2` (`type`, `value`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (2, " << result->getDataInt("player") << ", " << (result->getDataInt("time") <= time(NULL) ? 0 : 1) << ", 0, " << result->getDataInt("time") << ", " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
							break;

						case BAN_ACCOUNT:
							query << "INSERT INTO `bans2` (`type`, `value`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (3, " << result->getDataInt("player") << ", " << (result->getDataInt("time") <= time(NULL) ? 0 : 1) << ", " << result->getDataInt("time") << ", 0, " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
							break;

						case NOTATION_ACCOUNT:
						case DELETE_ACCOUNT:
							query << "INSERT INTO `bans2` (`type`, `value`, `active`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << result->getDataInt("type") << ", " << result->getDataInt("player") << ", " << (result->getDataInt("time") <= time(NULL) ? 0 : 1) << ", 0, " << result->getDataInt("time") << ", " << result->getDataInt("banned_by") << ", " << db->escapeString(result->getDataString("comment")) << ", " << result->getDataInt("reason_id") << ", " << result->getDataInt("action_id") << ");";
							break;
					}
				}
				while(result->next());
				db->freeResult(result);
			}

			db->executeQuery("DROP TABLE `bans`;");
			if(db->getDatabaseEngine() == DATABASE_ENGINE_MYSQL)
				db->executeQuery("RENAME TABLE `bans2` TO `bans`;");
			else
				db->executeQuery("ALTER TABLE `bans2` RENAME TO `bans`;");

			registerDatabaseConfig("db_version", ?);
			return ?;
		}
		*/

		default: break;
	}
	return 0;
}

bool DatabaseManager::getDatabaseConfig(const std::string& config, int32_t &value)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "SELECT `value` FROM `server_config` WHERE `config` = " << db->escapeString(config) << ";";
	DBResult* result = db->storeQuery(query.str());
	if(!result)
		return false;

	value = atoi(result->getDataString("value").c_str());
	db->freeResult(result);
	return true;
}

bool DatabaseManager::getDatabaseConfig(const std::string& config, std::string& value)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "SELECT `value` FROM `server_config` WHERE `config` = " << db->escapeString(config) << ";";
	DBResult* result = db->storeQuery(query.str());
	if(!result)
		return false;

	value = result->getDataString("value");
	db->freeResult(result);
	return true;
}

void DatabaseManager::registerDatabaseConfig(const std::string& config, int32_t value)
{
	Database* db = Database::getInstance();
	DBQuery query;

	int32_t tmp;
	if(!getDatabaseConfig(config, tmp))
		query << "INSERT INTO `server_config` VALUES (" << db->escapeString(config) << ", '" << value << "');";
	else
		query << "UPDATE `server_config` SET `value` = '" << value << "' WHERE `config` = " << db->escapeString(config) << ";";

	db->executeQuery(query.str());
}

void DatabaseManager::registerDatabaseConfig(const std::string& config, const std::string& value)
{
	Database* db = Database::getInstance();
	DBQuery query;

	std::string tmp;
	if(!getDatabaseConfig(config, tmp))
		query << "INSERT INTO `server_config` VALUES (" << db->escapeString(config) << ", " << db->escapeString(value) << ");";
	else
		query << "UPDATE `server_config` SET `value` = " << db->escapeString(value) << " WHERE `config` = " << db->escapeString(config) << ";";

	db->executeQuery(query.str());
}

void DatabaseManager::checkEncryption()
{
	int32_t currentValue = g_config.getNumber(ConfigManager::PASSWORD_TYPE);
	int32_t oldValue = 0;
	if(getDatabaseConfig("encryption", oldValue))
	{
		if(currentValue == oldValue)
			return;

		if(oldValue != PASSWORD_TYPE_PLAIN)
		{
			std::string oldName;
			if(oldValue == PASSWORD_TYPE_MD5)
				oldName = "md5";
			else if(oldValue == PASSWORD_TYPE_SHA1)
				oldName = "sha1";
			else
				oldName = "plain";

			g_config.setNumber(ConfigManager::PASSWORD_TYPE, oldValue);
			std::cout << "> WARNING: Unsupported password hashing switch! Change back passwordType in config.lua to \"" << oldName << "\"!" << std::endl;
			return;
		}

		switch(currentValue)
		{
			case PASSWORD_TYPE_MD5:
			{
				Database* db = Database::getInstance();
				DBQuery query;
				if(db->getDatabaseEngine() != DATABASE_ENGINE_MYSQL)
				{
					DBResult* result = db->storeQuery("SELECT `id`, `password`, `key` FROM `accounts`;");
					if(result)
					{
						do
						{
							query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToMD5(result->getDataString("password"))) << ", `key` = " << db->escapeString(transformToMD5(result->getDataString("key"))) << " WHERE `id` = " << result->getDataInt("id") << ";";
							db->executeQuery(query.str());
						}
						while(result->next());
						db->freeResult(result);
					}
				}
				else
					db->executeQuery("UPDATE `accounts` SET `password` = MD5(`password`), `key` = MD5(`key`);");

				std::cout << "> Password type has been updated to MD5." << std::endl;
				break;
			}

			case PASSWORD_TYPE_SHA1:
			{
				Database* db = Database::getInstance();
				DBQuery query;
				if(db->getDatabaseEngine() != DATABASE_ENGINE_MYSQL)
				{
					DBResult* result = db->storeQuery("SELECT `id`, `password`, `key` FROM `accounts`;");
					if(result)
					{
						do
						{
							query << "UPDATE `accounts` SET `password` = " << db->escapeString(transformToSHA1(result->getDataString("password"))) << ", `key` = " << db->escapeString(transformToSHA1(result->getDataString("key"))) << " WHERE `id` = " << result->getDataInt("id") << ";";
							db->executeQuery(query.str());
						}
						while(result->next());
						db->freeResult(result);
					}
				}
				else
					db->executeQuery("UPDATE `accounts` SET `password` = SHA1(`password`), `key` = SHA1(`key`);");

				std::cout << "> Password type has been updated to SHA1." << std::endl;
				break;
			}

			default: break;
		}
	}
	else if(g_config.getBoolean(ConfigManager::ACCOUNT_MANAGER))
	{
		switch(currentValue)
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

			default: break;
		}
	}
	registerDatabaseConfig("encryption", currentValue);
}

void DatabaseManager::checkTriggers()
{
	/*
	Database* db = Database::getInstance();
	switch(db->getDatabaseEngine())
	{
	}
	*/
}
