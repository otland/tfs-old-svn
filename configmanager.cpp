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

#include "definitions.h"
#include "configmanager.h"
#include <iostream>

ConfigManager::ConfigManager()
{
	m_isLoaded = false;
}

ConfigManager::~ConfigManager()
{
	//
}

bool ConfigManager::loadFile(const std::string& _filename)
{
	lua_State* L = lua_open();
	if(!L)
		return false;

	if(luaL_dofile(L, _filename.c_str()))
	{
		lua_close(L);
		return false;
	}

	//parse config
	if(!m_isLoaded) //info that must be loaded one time (unless we reset the modules involved)
	{
		m_confBoolean[SERVERSAVE_ENABLED] = (getGlobalString(L, "serverSaveEnabled", "yes") == "yes");
		m_confBoolean[SAVE_GLOBAL_STORAGE] = (getGlobalString(L, "saveGlobalStorage", "no") == "yes");
		m_confBoolean[INGAME_GUILD_SYSTEM] = (getGlobalString(L, "ingameGuildSystem", "yes") == "yes");

		m_confString[CONFIG_FILE] = _filename;
		m_confString[IP] = getGlobalString(L, "ip", "127.0.0.1");
		m_confString[MAP_NAME] = getGlobalString(L, "mapName", "forgotten");
		m_confString[MAP_AUTHOR] = getGlobalString(L, "mapAuthor", "Unknown");
		m_confString[HOUSE_RENT_PERIOD] = getGlobalString(L, "houseRentPeriod", "monthly");
		m_confString[MYSQL_HOST] = getGlobalString(L, "mysqlHost", "localhost");
		m_confString[MYSQL_USER] = getGlobalString(L, "mysqlUser", "root");
		m_confString[MYSQL_PASS] = getGlobalString(L, "mysqlPass", "");
		m_confString[MYSQL_DB] = getGlobalString(L, "mysqlDatabase", "theforgottenserver");
		m_confString[SQLITE_DB] = getGlobalString(L, "sqliteDatabase");
		m_confString[PASSWORDTYPE] = getGlobalString(L, "passwordType", "plain");

		m_confInteger[SQL_PORT] = getGlobalNumber(L, "mysqlPort", 3306);
		m_confInteger[PASSWORD_TYPE] = PASSWORD_TYPE_PLAIN;
		m_confInteger[SERVERSAVE_H] = getGlobalNumber(L, "serverSaveHour", 3);
		m_confInteger[ADMIN_PORT] = getGlobalNumber(L, "adminProtocolPort", 7171);
		m_confInteger[GAME_PORT] = getGlobalNumber(L, "gameProtocolPort", 7172);
		m_confInteger[LOGIN_PORT] = getGlobalNumber(L, "loginProtocolPort", 7171);
		m_confInteger[STATUS_PORT] = getGlobalNumber(L, "statusProtocolPort", 7171);

		#if defined __USE_MYSQL__ && defined __USE_SQLITE__
		m_confString[SQL_TYPE] = getGlobalString(L, "sqlType", "sqlite");
		m_confInteger[SQLTYPE] = SQL_TYPE_NONE;
		#endif
	}

	m_confBoolean[FREE_MEMORY_AT_SHUTDOWN] = (getGlobalString(L, "freeMemoryAtShutdown", "yes") == "yes");
	m_confBoolean[ACCOUNT_MANAGER] = (getGlobalString(L, "accountManager", "yes") == "yes");
	m_confBoolean[ON_OR_OFF_CHARLIST] = (getGlobalString(L, "displayOnOrOffAtCharlist", "no") == "yes");
	m_confBoolean[ALLOW_CHANGEOUTFIT] = (getGlobalString(L, "allowChangeOutfit", "yes") == "yes");
	m_confBoolean[ONE_PLAYER_ON_ACCOUNT] = (getGlobalString(L, "onePlayerOnlinePerAccount", "yes") == "yes");
	m_confBoolean[CANNOT_ATTACK_SAME_LOOKFEET] = (getGlobalString(L, "noDamageToSameLookfeet", "no") == "yes");
	m_confBoolean[AIMBOT_HOTKEY_ENABLED] = (getGlobalString(L, "hotkeyAimbotEnabled", "yes") == "yes");
	m_confBoolean[START_CHOOSEVOC] = (getGlobalString(L, "newPlayerChooseVoc", "no") == "yes");
	m_confBoolean[SHOW_GAMEMASTERS_ONLINE] = (getGlobalString(L, "displayGamemastersWithOnlineCommand", "no") == "yes");
	m_confBoolean[REMOVE_AMMO] = (getGlobalString(L, "removeAmmoWhenUsingDistanceWeapon", "yes") == "yes");
	m_confBoolean[REMOVE_RUNE_CHARGES] = (getGlobalString(L, "removeChargesFromRunes", "yes") == "yes");
	m_confBoolean[RANDOMIZE_TILES] = (getGlobalString(L, "randomizeTiles", "yes") == "yes");
	m_confBoolean[EXPERIENCE_FROM_PLAYERS] = (getGlobalString(L, "experienceByKillingPlayers", "no") == "yes");
	m_confBoolean[SHUTDOWN_AT_SERVERSAVE] = (getGlobalString(L, "shutdownAtServerSave", "no") == "yes");
	m_confBoolean[CLEAN_MAP_AT_SERVERSAVE] = (getGlobalString(L, "cleanMapAtServerSave", "yes") == "yes");
	m_confBoolean[FREE_PREMIUM] = (getGlobalString(L, "freePremium", "no") == "yes");
	m_confBoolean[ADMIN_LOGS_ENABLED] = (getGlobalString(L, "adminLogsEnabled", "no") == "yes");
	m_confBoolean[BROADCAST_BANISHMENTS] = (getGlobalString(L, "broadcastBanishments", "yes") == "yes");
	m_confBoolean[GENERATE_ACCOUNT_NUMBER] = (getGlobalString(L, "generateAccountNumber", "yes") == "yes");
	m_confBoolean[REPLACE_KICK_ON_LOGIN] = (getGlobalString(L, "replaceKickOnLogin", "yes") == "yes");
	m_confBoolean[OLD_CONDITION_ACCURACY] = (getGlobalString(L, "oldConditionAccuracy", "no") == "yes");
	m_confBoolean[ANIMATION_TEXT_ON_HEAL] = (getGlobalString(L, "animationTextOnHeal", "yes") == "yes");

	m_confString[DEFAULT_PRIORITY] = getGlobalString(L, "defaultPriority", "high");
	m_confString[MAP_STORAGE_TYPE] = getGlobalString(L, "mapStorageType", "relational");
	m_confString[LOGIN_MSG] = getGlobalString(L, "loginMessage", "Welcome to the Forgotten Server!");
	m_confString[SERVER_NAME] = getGlobalString(L, "serverName");
	m_confString[OWNER_NAME] = getGlobalString(L, "ownerName");
	m_confString[OWNER_EMAIL] = getGlobalString(L, "ownerEmail");
	m_confString[URL] = getGlobalString(L, "url");
	m_confString[LOCATION] = getGlobalString(L, "location");
	m_confString[MOTD] = getGlobalString(L, "motd");
	m_confString[WORLD_TYPE] = getGlobalString(L, "worldType", "pvp");

	m_confInteger[LOGIN_TRIES] = getGlobalNumber(L, "loginTries", 3);
	m_confInteger[RETRY_TIMEOUT] = getGlobalNumber(L, "retryTimeout", 30 * 1000);
	m_confInteger[LOGIN_TIMEOUT] = getGlobalNumber(L, "loginTimeout", 5 * 1000);
	m_confInteger[MAX_PLAYERS] = getGlobalNumber(L, "maxPlayers");
	m_confInteger[PZ_LOCKED] = getGlobalNumber(L, "pzLocked", 0);
	m_confInteger[DEFAULT_DESPAWNRANGE] = getGlobalNumber(L, "deSpawnRange", 2);
	m_confInteger[DEFAULT_DESPAWNRADIUS] = getGlobalNumber(L, "deSpawnRadius", 50);
	m_confInteger[ALLOW_CLONES] = getGlobalNumber(L, "allowClones", 0);
	m_confInteger[RATE_EXPERIENCE] = getGlobalNumber(L, "rateExp", 1);
	m_confInteger[RATE_SKILL] = getGlobalNumber(L, "rateSkill", 1);
	m_confInteger[RATE_LOOT] = getGlobalNumber(L, "rateLoot", 1);
	m_confInteger[RATE_MAGIC] = getGlobalNumber(L, "rateMagic", 1);
	m_confInteger[RATE_SPAWN] = getGlobalNumber(L, "rateSpawn", 1);
	m_confInteger[SPAWNPOS_X] = getGlobalNumber(L, "newPlayerSpawnPosX", 100);
	m_confInteger[SPAWNPOS_Y] = getGlobalNumber(L, "newPlayerSpawnPosY", 100);
	m_confInteger[SPAWNPOS_Z] = getGlobalNumber(L, "newPlayerSpawnPosZ", 7);
	m_confInteger[SPAWNTOWN_ID] = getGlobalNumber(L, "newPlayerTownId", 1);
	m_confInteger[START_LEVEL] = getGlobalNumber(L, "newPlayerLevel", 1);
	m_confInteger[START_MAGICLEVEL] = getGlobalNumber(L, "newPlayerMagicLevel", 0);
	m_confInteger[HOUSE_PRICE] = getGlobalNumber(L, "housePriceEachSQM", 1000);
	m_confInteger[KILLS_TO_RED] = getGlobalNumber(L, "killsToRedSkull", 3);
	m_confInteger[KILLS_TO_BAN] = getGlobalNumber(L, "killsToBan", 5);
	m_confInteger[BAN_DAYS] = getGlobalNumber(L, "banDays", 7);
	m_confInteger[FINAL_BAN_DAYS] = getGlobalNumber(L, "finalBanDays", 30);
	m_confInteger[HIGHSCORES_TOP] = getGlobalNumber(L, "highscoreDisplayPlayers", 10);
	m_confInteger[HIGHSCORES_UPDATETIME] = getGlobalNumber(L, "updateHighscoresAfterMinutes", 60);
	m_confInteger[ACTIONS_DELAY_INTERVAL] = getGlobalNumber(L, "timeBetweenActions", 200);
	m_confInteger[EX_ACTIONS_DELAY_INTERVAL] = getGlobalNumber(L, "timeBetweenExActions", 1000);
	m_confInteger[MAX_MESSAGEBUFFER] = getGlobalNumber(L, "maxMessageBuffer", 4);
	m_confInteger[CRITICAL_HIT_CHANCE] = getGlobalNumber(L, "criticalHitChance", 5);
	m_confInteger[KICK_AFTER_MINUTES] = getGlobalNumber(L, "kickIdlePlayerAfterMinutes", 15);
	m_confInteger[PROTECTION_LEVEL] = getGlobalNumber(L, "protectionLevel", 1);
	m_confInteger[DEATH_LOSE_PERCENT] = getGlobalNumber(L, "deathLosePercent", 10);
	m_confInteger[STATUSQUERY_TIMEOUT] = getGlobalNumber(L, "statusTimeout", 5 * 60 * 1000);
	m_confInteger[FRAG_TIME] = getGlobalNumber(L, "timeToDecreaseFrags", 24 * 60 * 60 * 1000);
	m_confInteger[WHITE_SKULL_TIME] = getGlobalNumber(L, "whiteSkullTime", 15 * 60 * 1000);
	m_confInteger[AUTO_SAVE_EACH_MINUTES] = getGlobalNumber(L, "autoSaveEachMinutes", 0);
	m_confInteger[STAIRHOP_DELAY] = getGlobalNumber(L, "stairJumpExhaustion", 2000);
	m_confInteger[ALTERNATIVE_EXHAUST] = getGlobalNumber(L, "alternativeExhaust", 1000);
	m_isLoaded = true;

	lua_close(L);
	return true;
}

bool ConfigManager::reload()
{
	if(!m_isLoaded)
		return false;

	return loadFile(m_confString[CONFIG_FILE]);
}

const std::string& ConfigManager::getString(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_STRING_CONFIG)
		return m_confString[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getString] " << _what << std::endl;
		return m_confString[DUMMY_STR];
	}
}

int32_t ConfigManager::getNumber(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_INTEGER_CONFIG)
		return m_confInteger[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getNumber] " << _what << std::endl;
		return 0;
	}
}

bool ConfigManager::getBoolean(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_BOOLEAN_CONFIG)
		return m_confBoolean[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getBoolean] " << _what << std::endl;
		return false;
	}
}

bool ConfigManager::setNumber(uint32_t _what, int32_t _value)
{
	if(m_isLoaded && _what < LAST_INTEGER_CONFIG)
	{
		m_confInteger[_what] = _value;
		return true;
	}
	else
	{
		std::cout << "Warning: [ConfigManager::setNumber] " << _what << std::endl;
		return false;
	}
}

std::string ConfigManager::getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isstring(_L, -1))
		return _default;

	int32_t len = (int32_t)lua_strlen(_L, -1);
	std::string ret(lua_tostring(_L, -1), len);
	lua_pop(_L,1);

	return ret;
}

int32_t ConfigManager::getGlobalNumber(lua_State* _L, const std::string& _identifier, const int32_t _default)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isnumber(_L, -1))
		return _default;

	int32_t val = (int32_t)lua_tonumber(_L, -1);
	lua_pop(_L,1);

	return val;
}

std::string ConfigManager::getGlobalStringField (lua_State* _L, const std::string& _identifier, const int32_t _key, const std::string& _default) {
	lua_getglobal(_L, _identifier.c_str());

	lua_pushnumber(_L, _key);
	lua_gettable(_L, -2);  /* get table[key] */
	if(!lua_isstring(_L, -1))
		return _default;
	std::string result = lua_tostring(_L, -1);
	lua_pop(_L, 2);  /* remove number and key*/
	return result;
}
