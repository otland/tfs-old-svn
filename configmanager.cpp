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
#include "tools.h"
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
		m_confString[CONFIG_FILE] = _filename;
		m_confString[IP] = getGlobalString(L, "ip", "127.0.0.1");
		m_confNumber[PORT] = getGlobalNumber(L, "port", 7171);
		#ifdef MULTI_SQL_DRIVERS
		m_confString[SQL_TYPE] = getGlobalString(L, "sqlType", "sqlite");
		#endif
		m_confString[SQL_HOST] = getGlobalString(L, "sqlHost", "localhost");
		m_confNumber[SQL_PORT] = getGlobalNumber(L, "sqlPort", 3306);
		m_confString[SQL_DB] = getGlobalString(L, "sqlDatabase", "theforgottenserver");
		m_confString[SQL_USER] = getGlobalString(L, "sqlUser", "root");
		m_confString[SQL_PASS] = getGlobalString(L, "sqlPass", "");
		m_confString[SQL_FILE] = getGlobalString(L, "sqlFile", "forgottenserver.s3db");
		m_confString[PASSWORD_TYPE] = getGlobalString(L, "passwordType", "plain");
		m_confNumber[PASSWORDTYPE] = PASSWORD_TYPE_PLAIN;
		m_confString[MAP_NAME] = getGlobalString(L, "mapName", "forgotten");
		m_confString[MAP_AUTHOR] = getGlobalString(L, "mapAuthor", "Unknown");
		m_confBool[GLOBALSAVE_ENABLED] = getGlobalBool(L, "globalSaveEnabled", "yes");
		m_confNumber[GLOBALSAVE_H] = getGlobalNumber(L, "globalSaveHour", 8);
		m_confString[HOUSE_RENT_PERIOD] = getGlobalString(L, "houseRentPeriod", "monthly");
	}

	m_confString[LOGIN_MSG] = getGlobalString(L, "loginMessage", "Welcome to the Forgotten Server!");
	m_confString[SERVER_NAME] = getGlobalString(L, "serverName");
	m_confString[OWNER_NAME] = getGlobalString(L, "ownerName");
	m_confString[OWNER_EMAIL] = getGlobalString(L, "ownerEmail");
	m_confString[URL] = getGlobalString(L, "url");
	m_confString[LOCATION] = getGlobalString(L, "location");
	m_confNumber[LOGIN_TRIES] = getGlobalNumber(L, "loginTries", 3);
	m_confNumber[RETRY_TIMEOUT] = getGlobalNumber(L, "retryTimeout", 30 * 1000);
	m_confNumber[LOGIN_TIMEOUT] = getGlobalNumber(L, "loginTimeout", 5 * 1000);
	m_confString[MOTD] = getGlobalString(L, "motd");
	m_confNumber[MAX_PLAYERS] = getGlobalNumber(L, "maxPlayers");
	m_confNumber[PZ_LOCKED] = getGlobalNumber(L, "pzLocked", 0);
	m_confNumber[DEFAULT_DESPAWNRANGE] = getGlobalNumber(L, "deSpawnRange", 2);
	m_confNumber[DEFAULT_DESPAWNRADIUS] = getGlobalNumber(L, "deSpawnRadius", 50);
	m_confNumber[ALLOW_CLONES] = getGlobalNumber(L, "allowClones", 0);
	m_confNumber[RATE_EXPERIENCE] = getGlobalNumber(L, "rateExp", 1);
	m_confNumber[RATE_SKILL] = getGlobalNumber(L, "rateSkill", 1);
	m_confNumber[RATE_LOOT] = getGlobalNumber(L, "rateLoot", 1);
	m_confNumber[RATE_MAGIC] = getGlobalNumber(L, "rateMagic", 1);
	m_confNumber[RATE_SPAWN] = getGlobalNumber(L, "rateSpawn", 1);
	m_confNumber[SPAWNPOS_X] = getGlobalNumber(L, "newPlayerSpawnPosX", 100);
	m_confNumber[SPAWNPOS_Y] = getGlobalNumber(L, "newPlayerSpawnPosY", 100);
	m_confNumber[SPAWNPOS_Z] = getGlobalNumber(L, "newPlayerSpawnPosZ", 7);
	m_confNumber[SPAWNTOWN_ID] = getGlobalNumber(L, "newPlayerTownId", 1);
	m_confString[WORLD_TYPE] = getGlobalString(L, "worldType", "pvp");
	m_confBool[ACCOUNT_MANAGER] = getGlobalBool(L, "accountManager", "yes");
	m_confBool[NAMELOCK_MANAGER] = getGlobalBool(L, "namelockManager", "no");
	m_confNumber[START_LEVEL] = getGlobalNumber(L, "newPlayerLevel", 1);
	m_confNumber[START_MAGICLEVEL] = getGlobalNumber(L, "newPlayerMagicLevel", 0);
	m_confBool[START_CHOOSEVOC] = getGlobalBool(L, "newPlayerChooseVoc", "no");
	m_confNumber[HOUSE_PRICE] = getGlobalNumber(L, "housePriceEachSQM", 1000);
	m_confNumber[WHITE_SKULL_TIME] = getGlobalNumber(L, "whiteSkullTime", 15 * 60 * 1000);
	m_confNumber[KILLS_TO_RED] = getGlobalNumber(L, "killsToRedSkull", 3);
	m_confNumber[KILLS_TO_BAN] = getGlobalNumber(L, "killsToBan", 5);
	m_confNumber[HIGHSCORES_TOP] = getGlobalNumber(L, "highscoreDisplayPlayers", 10);
	m_confNumber[HIGHSCORES_UPDATETIME] = getGlobalNumber(L, "updateHighscoresAfterMinutes", 60);
	m_confBool[ON_OR_OFF_CHARLIST] = getGlobalBool(L, "displayOnOrOffAtCharlist", "no");
	m_confBool[ALLOW_CHANGEOUTFIT] = getGlobalBool(L, "allowChangeOutfit", "yes");
	m_confBool[ONE_PLAYER_ON_ACCOUNT] = getGlobalBool(L, "onePlayerOnlinePerAccount", "yes");
	m_confBool[CANNOT_ATTACK_SAME_LOOKFEET] = getGlobalBool(L, "noDamageToSameLookfeet", "no");
	m_confBool[AIMBOT_HOTKEY_ENABLED] = getGlobalBool(L, "hotkeyAimbotEnabled", "yes");
	m_confNumber[ACTIONS_DELAY_INTERVAL] = getGlobalNumber(L, "timeBetweenActions", 200);
	m_confNumber[EX_ACTIONS_DELAY_INTERVAL] = getGlobalNumber(L, "timeBetweenExActions", 1000);
	m_confNumber[MAX_MESSAGEBUFFER] = getGlobalNumber(L, "maxMessageBuffer", 4);
	m_confNumber[CRITICAL_HIT_CHANCE] = getGlobalNumber(L, "criticalHitChance", 5);
	m_confNumber[KICK_AFTER_MINUTES] = getGlobalNumber(L, "kickIdlePlayerAfterMinutes", 15);
	m_confBool[REMOVE_AMMO] = getGlobalBool(L, "removeAmmoWhenUsingDistanceWeapon", "yes");
	m_confBool[REMOVE_RUNE_CHARGES] = getGlobalBool(L, "removeChargesFromRunes", "yes");
	m_confBool[RANDOMIZE_TILES] = getGlobalBool(L, "randomizeTiles", "yes");
	m_confString[DEFAULT_PRIORITY] = getGlobalString(L, "defaultPriority", "high");
	m_confBool[EXPERIENCE_FROM_PLAYERS] = getGlobalBool(L, "experienceByKillingPlayers", "no");
	m_confBool[SHUTDOWN_AT_GLOBALSAVE] = getGlobalBool(L, "shutdownAtGlobalSave", "no");
	m_confBool[CLEAN_MAP_AT_GLOBALSAVE] = getGlobalBool(L, "cleanMapAtGlobalSave", "yes");
	m_confBool[FREE_PREMIUM] = getGlobalBool(L, "freePremium", "no");
	m_confNumber[PROTECTION_LEVEL] = getGlobalNumber(L, "protectionLevel", 1);
	m_confBool[ADMIN_LOGS_ENABLED] = getGlobalBool(L, "adminLogsEnabled", "no");
	m_confNumber[STATUSQUERY_TIMEOUT] = getGlobalNumber(L, "statusTimeout", 5 * 60 * 1000);
	m_confBool[BROADCAST_BANISHMENTS] = getGlobalBool(L, "broadcastBanishments", "yes");
	m_confBool[GENERATE_ACCOUNT_NUMBER] = getGlobalBool(L, "generateAccountNumber", "yes");
	m_confBool[INGAME_GUILD_MANAGEMENT] = getGlobalBool(L, "ingameGuildManagement", "yes");
	m_confNumber[LEVEL_TO_FORM_GUILD] = getGlobalNumber(L, "levelToFormGuild", 8);
	m_confNumber[MIN_GUILDNAME] = getGlobalNumber(L, "guildNameMinLength", 4);
	m_confNumber[MAX_GUILDNAME] = getGlobalNumber(L, "guildNameMaxLength", 20);
	m_confNumber[LEVEL_TO_BUY_HOUSE] = getGlobalNumber(L, "levelToBuyHouse", 1);
	m_confNumber[HOUSES_PER_ACCOUNT] = getGlobalNumber(L, "housesPerAccount", 0);
	m_confBool[HOUSE_BUY_AND_SELL] = getGlobalBool(L, "buyableAndSellableHouses", "yes");
	m_confBool[REPLACE_KICK_ON_LOGIN] = getGlobalBool(L, "replaceKickOnLogin", "yes");
	m_confBool[HOUSE_NEED_PREMIUM] = getGlobalBool(L, "houseNeedPremiumAccount", "yes");
	m_confBool[HOUSE_RENTASPRICE] = getGlobalBool(L, "houseRentAsPrice", "no");
	m_confBool[HOUSE_PRICEASRENT] = getGlobalBool(L, "housePriceAsRent", "no");
	m_confNumber[FRAG_TIME] = getGlobalNumber(L, "timeToDecreaseFrags", 24 * 60 * 60 * 1000);
	m_confNumber[MAX_VIOLATIONCOMMENT_SIZE] = getGlobalNumber(L, "maxViolationCommentSize", 60);
	m_confNumber[NOTATIONS_TO_BAN] = getGlobalNumber(L, "notationsToBan", 3);
	m_confNumber[WARNINGS_TO_FINALBAN] = getGlobalNumber(L, "warningsToFinalBan", 4);
	m_confNumber[WARNINGS_TO_DELETION] = getGlobalNumber(L, "warningsToDeletion", 5);
	m_confNumber[BAN_LENGTH] = getGlobalNumber(L, "banLength", 7 * 24 * 60 * 60);
	m_confNumber[FINALBAN_LENGTH] = getGlobalNumber(L, "finalBanLength", 30 * 24 * 60 * 60);
	m_confNumber[IPBANISHMENT_LENGTH] = getGlobalNumber(L, "ipBanishmentLength", 1 * 24 * 60 * 60);
	m_confBool[BANK_SYSTEM] = getGlobalBool(L, "bankSystem", "yes");
	m_confBool[PREMIUM_FOR_PROMOTION] = getGlobalBool(L, "premiumForPromotion", "yes");
	m_confBool[REMOVE_PREMIUM_ON_INIT] = getGlobalBool(L, "removePremiumOnInit", "yes");
	m_confBool[SHOW_HEALING_DAMAGE] = getGlobalBool(L, "showHealingDamage", "no");
	//m_confBool[TELEPORT_SUMMONS] = getGlobalString(L, "teleportAllSummons", "no");
	//m_confBool[TELEPORT_PLAYER_SUMMONS] = getGlobalBool(L, "teleportPlayerSummons", "no");
	m_confBool[PVP_TILE_IGNORE_PROTECTION] = getGlobalBool(L, "pvpTileIgnoreLevelAndVocationProtection", "yes");
	m_confBool[DISPLAY_CRITICAL_HIT] = getGlobalBool(L, "displayCriticalHitNotify", "no");
	m_confBool[ADVANCING_SKILL_LEVEL] = getGlobalBool(L, "displaySkillLevelOnAdvance", "no");
	m_confBool[CLEAN_PROTECTED_ZONES] = getGlobalBool(L, "cleanProtectedZones", "yes");
	//m_confBool[SPELL_NAME_INSTEAD_WORDS] = getGlobalBool(L, "spellNameInsteadOfWordsOnCast", "no");
	m_confNumber[MAX_PLAYER_SUMMONS] = getGlobalNumber(L, "maxPlayerSummons", 2);
	m_confBool[SAVE_GLOBAL_STORAGE] = getGlobalBool(L, "saveGlobalStorage", "yes");
	m_confBool[FORCE_CLOSE_SLOW_CONNECTION] = getGlobalBool(L, "forceSlowConnectionsToDisconnect", "no");
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

bool ConfigManager::getBool(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_BOOL_CONFIG)
		return m_confBool[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getBool] " << _what << std::endl;
		return false;
	}
}

int32_t ConfigManager::getNumber(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_NUMBER_CONFIG)
		return m_confNumber[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getNumber] " << _what << std::endl;
		return 0;
	}
}

double ConfigManager::getDouble(uint32_t _what) const
{
	if(m_isLoaded && _what < LAST_DOUBLE_CONFIG)
		return m_confDouble[_what];
	else
	{
		std::cout << "Warning: [ConfigManager::getDouble] " << _what << std::endl;
		return 0;
	}
}

bool ConfigManager::setNumber(uint32_t _what, int32_t _value)
{
	if(m_isLoaded && _what < LAST_NUMBER_CONFIG)
	{
		m_confNumber[_what] = _value;
		return true;
	}
	else
	{
		std::cout << "Warning: [ConfigManager::setNumber] " << _what << std::endl;
		return false;
	}
}

std::string ConfigManager::getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default /*= ""*/)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isstring(_L, -1))
		return _default;

	int32_t len = (int32_t)lua_strlen(_L, -1);
	std::string ret(lua_tostring(_L, -1), len);
	lua_pop(_L, 1);

	return ret;
}

bool ConfigManager::getGlobalBool(lua_State* _L, const std::string& _identifier, const std::string& _default /*= "no"*/)
{
	return booleanString(getGlobalString(_L, _identifier, _default));
}

int32_t ConfigManager::getGlobalNumber(lua_State* _L, const std::string& _identifier, const int32_t _default /*= 0*/)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isnumber(_L, -1))
		return _default;

	int32_t val = (int32_t)lua_tonumber(_L, -1);
	lua_pop(_L, 1);

	return val;
}

double ConfigManager::getGlobalDouble(lua_State* _L, const std::string& _identifier, const int32_t _default /*= 0*/)
{
	lua_getglobal(_L, _identifier.c_str());

	if(!lua_isnumber(_L, -1))
		return _default;

	double val = (double)lua_tonumber(_L, -1);
	lua_pop(_L, 1);

	return val;
}

std::string ConfigManager::getGlobalStringField(lua_State* _L, const std::string& _identifier, const int32_t _key, const std::string& _default /*= ""*/)
{
	lua_getglobal(_L, _identifier.c_str());

	lua_pushnumber(_L, _key);
	lua_gettable(_L, -2);  /* get table[key] */
	if(!lua_isstring(_L, -1))
		return _default;

	std::string result = lua_tostring(_L, -1);
	lua_pop(_L, 2);  /* remove number and key*/
	return result;
}
