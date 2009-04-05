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
	L = NULL;
	m_loaded = false;
	m_startup = true;

	m_confString[CONFIG_FILE] = getFilePath(FILE_TYPE_CONFIG, "config.lua");
	m_confBool[LOGIN_ONLY_LOGINSERVER] = false;

	m_confNumber[LOGIN_PORT] = m_confNumber[GAME_PORT] = m_confNumber[ADMIN_PORT] = m_confNumber[STATUS_PORT] = 0;
	m_confString[IP] = m_confString[RUNFILE] = m_confString[ERROR_LOG] = m_confString[OUT_LOG] = "";
}

bool ConfigManager::load()
{
	if(L)
		lua_close(L);

	L = lua_open();
	if(!L)
		return false;

	if(luaL_dofile(L, m_confString[CONFIG_FILE].c_str()))
	{
		lua_close(L);
		L = NULL;
		return false;
	}

	//parse config
	if(!m_loaded) //info that must be loaded one time (unless we reset the modules involved)
	{
		if(m_confString[IP] == "")
			m_confString[IP] = getGlobalString(L, "ip", "127.0.0.1");

		if(m_confNumber[LOGIN_PORT] == 0)
			m_confNumber[LOGIN_PORT] = getGlobalNumber(L, "loginPort", 7171);

		if(m_confNumber[GAME_PORT] == 0)
			m_confNumber[GAME_PORT] = getGlobalNumber(L, "gamePort", 7172);

		if(m_confNumber[ADMIN_PORT] == 0)
			m_confNumber[ADMIN_PORT] = getGlobalNumber(L, "adminPort", 7171);

		if(m_confNumber[STATUS_PORT] == 0)
			m_confNumber[STATUS_PORT] = getGlobalNumber(L, "statusPort", 7171);

		if(m_confString[RUNFILE] == "")
			m_confString[RUNFILE] = getGlobalString(L, "runFile", "");

		if(m_confString[OUT_LOG] == "")
			m_confString[OUT_LOG] = getGlobalString(L, "outLogName", "");

		if(m_confString[ERROR_LOG] == "")
			m_confString[ERROR_LOG] = getGlobalString(L, "errorLogName", "");

		m_confBool[TRUNCATE_LOGS] = getGlobalBool(L, "truncateLogsOnStartup", "yes");
		#ifdef MULTI_SQL_DRIVERS
		m_confString[SQL_TYPE] = getGlobalString(L, "sqlType", "sqlite");
		#endif
		m_confString[SQL_HOST] = getGlobalString(L, "sqlHost", "localhost");
		m_confNumber[SQL_PORT] = getGlobalNumber(L, "sqlPort", 3306);
		m_confString[SQL_DB] = getGlobalString(L, "sqlDatabase", "theforgottenserver");
		m_confString[SQL_USER] = getGlobalString(L, "sqlUser", "root");
		m_confString[SQL_PASS] = getGlobalString(L, "sqlPass", "");
		m_confString[SQL_FILE] = getGlobalString(L, "sqlFile", "forgottenserver.s3db");
		m_confNumber[SQL_KEEPALIVE] = getGlobalNumber(L, "sqlKeepAlive", 0);
		m_confNumber[MYSQL_READ_TIMEOUT] = getGlobalNumber(L, "mysqlReadTimeout", 10);
		m_confBool[OPTIMIZE_DB_AT_STARTUP] = getGlobalBool(L, "optimizeDatabaseAtStartup", "yes");
		m_confString[MAP_NAME] = getGlobalString(L, "mapName", "forgotten");
		m_confString[MAP_AUTHOR] = getGlobalString(L, "mapAuthor", "Unknown");
		m_confBool[GLOBALSAVE_ENABLED] = getGlobalBool(L, "globalSaveEnabled", "yes");
		m_confNumber[GLOBALSAVE_H] = getGlobalNumber(L, "globalSaveHour", 8);
		m_confString[HOUSE_RENT_PERIOD] = getGlobalString(L, "houseRentPeriod", "monthly");
		m_confNumber[WORLD_ID] = getGlobalNumber(L, "worldId", 0);
		m_confBool[RANDOMIZE_TILES] = getGlobalBool(L, "randomizeTiles", "yes");
		m_confBool[STORE_TRASH] = getGlobalBool(L, "storeTrash", "yes");
		m_confNumber[LOGIN_TRIES] = getGlobalNumber(L, "loginTries", 3);
		m_confNumber[RETRY_TIMEOUT] = getGlobalNumber(L, "retryTimeout", 30 * 1000);
		m_confNumber[LOGIN_TIMEOUT] = getGlobalNumber(L, "loginTimeout", 5 * 1000);
		m_confNumber[MAX_MESSAGEBUFFER] = getGlobalNumber(L, "maxMessageBuffer", 4);
		m_confNumber[MAX_PLAYERS] = getGlobalNumber(L, "maxPlayers");
		m_confNumber[DEFAULT_DESPAWNRANGE] = getGlobalNumber(L, "deSpawnRange", 2);
		m_confNumber[DEFAULT_DESPAWNRADIUS] = getGlobalNumber(L, "deSpawnRadius", 50);
		m_confNumber[PZ_LOCKED] = getGlobalNumber(L, "pzLocked", 0);
		m_confBool[EXPERIENCE_STAGES] = getGlobalBool(L, "experienceStages", "no");
		m_confString[DEFAULT_PRIORITY] = getGlobalString(L, "defaultPriority", "high");
		m_confBool[ABORT_SOCKET_FAIL] = getGlobalBool(L, "abortOnSocketFailure", "yes");
		#ifndef __LOGIN_SERVER__
		m_confBool[LOGIN_ONLY_LOGINSERVER] = getGlobalBool(L, "loginOnlyWithLoginServer", "no");
		#endif
		m_confString[PASSWORD_TYPE] = getGlobalString(L, "passwordType", "plain");
		m_confNumber[PASSWORDTYPE] = PASSWORD_TYPE_PLAIN;
	}

	m_confString[LOGIN_MSG] = getGlobalString(L, "loginMessage", "Welcome to the Forgotten Server!");
	m_confString[SERVER_NAME] = getGlobalString(L, "serverName");
	m_confString[OWNER_NAME] = getGlobalString(L, "ownerName");
	m_confString[OWNER_EMAIL] = getGlobalString(L, "ownerEmail");
	m_confString[URL] = getGlobalString(L, "url");
	m_confString[LOCATION] = getGlobalString(L, "location");
	m_confString[MOTD] = getGlobalString(L, "motd");
	m_confNumber[ALLOW_CLONES] = getGlobalNumber(L, "allowClones", 0);
	m_confDouble[RATE_EXPERIENCE] = getGlobalDouble(L, "rateExperience", 1.0f);
	m_confDouble[RATE_SKILL] = getGlobalDouble(L, "rateSkill", 1.0f);
	m_confDouble[RATE_MAGIC] = getGlobalDouble(L, "rateMagic", 1.0f);
	m_confNumber[RATE_LOOT] = getGlobalNumber(L, "rateLoot", 1);
	m_confNumber[RATE_SPAWN] = getGlobalNumber(L, "rateSpawn", 1);
	m_confNumber[PARTY_RADIUS_X] = getGlobalNumber(L, "experienceShareRadiusX", 30);
	m_confNumber[PARTY_RADIUS_Y] = getGlobalNumber(L, "experienceShareRadiusY", 30);
	m_confNumber[PARTY_RADIUS_Z] = getGlobalNumber(L, "experienceShareRadiusZ", 1);
	m_confDouble[PARTY_DIFFERENCE] = getGlobalDouble(L, "experienceShareLevelDifference", double(2 / 3));
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
	m_confNumber[HOUSE_PRICE] = getGlobalNumber(L, "housePriceEachSquare", 1000);
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
	m_confNumber[CRITICAL_HIT_CHANCE] = getGlobalNumber(L, "criticalHitChance", 5);
	m_confNumber[KICK_AFTER_MINUTES] = getGlobalNumber(L, "kickIdlePlayerAfterMinutes", 15);
	m_confBool[REMOVE_WEAPON_AMMO] = getGlobalBool(L, "removeWeaponAmmunition", "yes");
	m_confBool[REMOVE_WEAPON_CHARGES] = getGlobalBool(L, "removeWeaponCharges", "yes");
	m_confBool[REMOVE_RUNE_CHARGES] = getGlobalBool(L, "removeRuneCharges", "yes");
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
	m_confBool[HOUSE_NEED_PREMIUM] = getGlobalBool(L, "houseNeedPremium", "yes");
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
	m_confBool[TELEPORT_SUMMONS] = getGlobalBool(L, "teleportAllSummons", "no");
	m_confBool[TELEPORT_PLAYER_SUMMONS] = getGlobalBool(L, "teleportPlayerSummons", "no");
	m_confBool[PVP_TILE_IGNORE_PROTECTION] = getGlobalBool(L, "pvpTileIgnoreLevelAndVocationProtection", "yes");
	m_confBool[DISPLAY_CRITICAL_HIT] = getGlobalBool(L, "displayCriticalHitNotify", "no");
	m_confBool[ADVANCING_SKILL_LEVEL] = getGlobalBool(L, "displaySkillLevelOnAdvance", "no");
	m_confBool[CLEAN_PROTECTED_ZONES] = getGlobalBool(L, "cleanProtectedZones", "yes");
	m_confBool[SPELL_NAME_INSTEAD_WORDS] = getGlobalBool(L, "spellNameInsteadOfWords", "no");
	m_confBool[EMOTE_SPELLS] = getGlobalBool(L, "emoteSpells", "no");
	m_confNumber[MAX_PLAYER_SUMMONS] = getGlobalNumber(L, "maxPlayerSummons", 2);
	m_confBool[SAVE_GLOBAL_STORAGE] = getGlobalBool(L, "saveGlobalStorage", "yes");
	m_confBool[FORCE_CLOSE_SLOW_CONNECTION] = getGlobalBool(L, "forceSlowConnectionsToDisconnect", "no");
	m_confBool[BLESSING_ONLY_PREMIUM] = getGlobalBool(L, "blessingsOnlyPremium", "yes");
	m_confBool[BED_REQUIRE_PREMIUM] = getGlobalBool(L, "bedsRequirePremium", "yes");
	m_confNumber[FIELD_OWNERSHIP] = getGlobalNumber(L, "fieldOwnershipDuration", 5 * 1000);
	m_confBool[ALLOW_CHANGECOLORS] = getGlobalBool(L, "allowChangeColors", "yes");
	m_confBool[STOP_ATTACK_AT_EXIT] = getGlobalBool(L, "stopAttackingAtExit", "no");
	m_confNumber[EXTRA_PARTY_PERCENT] = getGlobalNumber(L, "extraPartyExperiencePercent", 5);
	m_confNumber[EXTRA_PARTY_LIMIT] = getGlobalNumber(L, "extraPartyExperienceLimit", 20);
	m_confBool[DISABLE_OUTFITS_PRIVILEGED] = getGlobalBool(L, "disableOutfitsForPrivilegedPlayers", "no");
	m_confBool[OLD_CONDITION_ACCURACY] = getGlobalBool(L, "oldConditionAccuracy", "no");
	m_confBool[HOUSE_STORAGE] = getGlobalBool(L, "useHouseDataStorage", "no");
	m_confBool[TRACER_BOX] = getGlobalBool(L, "promptExceptionTracerErrorBox", "yes");
	m_confNumber[LOGIN_PROTECTION] = getGlobalNumber(L, "loginProtectionPeriod", 10 * 1000);
	m_confBool[STORE_DIRECTION] = getGlobalBool(L, "storePlayerDirection", "no");
	m_confNumber[PLAYER_DEEPNESS] = getGlobalNumber(L, "playerQueryDeepness", 1);
	m_confDouble[CRITICAL_HIT_MUL] = getGlobalDouble(L, "criticalHitMultiplier", 1);
	m_confNumber[STAIRHOP_DELAY] = getGlobalNumber(L, "stairhopDelay", 2 * 1000);
	m_confNumber[RATE_STAMINA_LOSS] = getGlobalNumber(L, "rateStaminaLoss", 1);
	m_confDouble[RATE_STAMINA_GAIN] = getGlobalDouble(L, "rateStaminaGain", 1000 / 3);
	m_confDouble[RATE_STAMINA_THRESHOLD] = getGlobalDouble(L, "rateStaminaThresholdGain", 4);
	m_confDouble[RATE_STAMINA_ABOVE] = getGlobalDouble(L, "rateStaminaAboveNormal", 1.5f);
	m_confDouble[RATE_STAMINA_UNDER] = getGlobalDouble(L, "rateStaminaUnderNormal", 0.5f);
	m_confNumber[STAMINA_LIMIT_TOP] = getGlobalNumber(L, "staminaRatingLimitTop", 41 * 60);
	m_confNumber[STAMINA_LIMIT_BOTTOM] = getGlobalNumber(L, "staminaRatingLimitLimit", 14 * 60);
	m_confBool[DISPLAY_LOGGING] = getGlobalBool(L, "displayPlayersLogging", "yes");
	m_confBool[STAMINA_BONUS_PREMIUM] = getGlobalBool(L, "staminaThresholdOnlyPremium", "yes");
	m_confBool[BAN_UNKNOWN_BYTES] = getGlobalBool(L, "autoBanishUnknownBytes", "no");
	m_confNumber[BLESS_REDUCTION_BASE] = getGlobalNumber(L, "blessingReductionBase", 30);
	m_confNumber[BLESS_REDUCTION_DECREAMENT] = getGlobalNumber(L, "blessingReductionDecreament", 5);
	m_confBool[ALLOW_CHANGEADDONS] = getGlobalBool(L, "allowChangeAddons", "yes");

	m_loaded = true;
	return true;
}

bool ConfigManager::reload()
{
	if(!m_loaded)
		return false;

	return load();
}

const std::string& ConfigManager::getString(uint32_t _what) const
{
	if((m_loaded && _what < LAST_STRING_CONFIG) || _what <= CONFIG_FILE)
		return m_confString[_what];

	if(!m_startup)
		std::cout << "[Warning - ConfigManager::getString] " << _what << std::endl;

	return m_confString[DUMMY_STR];
}

bool ConfigManager::getBool(uint32_t _what) const
{
	if(m_loaded && _what < LAST_BOOL_CONFIG)
		return m_confBool[_what];

	if(!m_startup)
		std::cout << "[Warning - ConfigManager::getBool] " << _what << std::endl;

	return false;
}

int32_t ConfigManager::getNumber(uint32_t _what) const
{
	if(m_loaded && _what < LAST_NUMBER_CONFIG)
		return m_confNumber[_what];

	if(!m_startup)
		std::cout << "[Warning - ConfigManager::getNumber] " << _what << std::endl;

	return 0;
}

double ConfigManager::getDouble(uint32_t _what) const
{
	if(m_loaded && _what < LAST_DOUBLE_CONFIG)
		return m_confDouble[_what];

	if(!m_startup)
		std::cout << "[Warning - ConfigManager::getDouble] " << _what << std::endl;

	return 0;
}

bool ConfigManager::setString(uint32_t _what, const std::string& _value)
{
	if(_what < LAST_STRING_CONFIG)
	{
		m_confString[_what] = _value;
		return true;
	}

	std::cout << "[Warning - ConfigManager::setString] " << _what << std::endl;
	return false;
}

bool ConfigManager::setNumber(uint32_t _what, int32_t _value)
{
	if(_what < LAST_NUMBER_CONFIG)
	{
		m_confNumber[_what] = _value;
		return true;
	}

	std::cout << "[Warning - ConfigManager::setNumber] " << _what << std::endl;
	return false;
}

std::string ConfigManager::getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default /*= ""*/)
{
	lua_getglobal(_L, _identifier.c_str());
	if(!lua_isstring(_L, -1))
	{
		lua_pop(_L, 1);
		return _default;
	}

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
	{
		lua_pop(_L, 1);
		return _default;
	}

	int32_t val = (int32_t)lua_tonumber(_L, -1);
	lua_pop(_L, 1);
	return val;
}

double ConfigManager::getGlobalDouble(lua_State* _L, const std::string& _identifier, const double _default /*= 0*/)
{
	lua_getglobal(_L, _identifier.c_str());
	if(!lua_isnumber(_L, -1))
	{
		lua_pop(_L, 1);
		return _default;
	}

	double val = (double)lua_tonumber(_L, -1);
	lua_pop(_L, 1);
	return val;
}

void ConfigManager::getValue(const std::string& key, lua_State* _L)
{
	lua_getglobal(L, key.c_str());
	moveValue(L, _L);
}

void ConfigManager::moveValue(lua_State* from, lua_State* to)
{
	switch(lua_type(from, -1))
	{
		case LUA_TNIL:
			lua_pushnil(to);
			break;
		case LUA_TBOOLEAN:
			lua_pushboolean(to, lua_toboolean(from, -1));
			break;
		case LUA_TNUMBER:
			lua_pushnumber(to, lua_tonumber(from, -1));
			break;
		case LUA_TSTRING:
		{
			size_t len;
			const char* str = lua_tolstring(from, -1, &len);

			lua_pushlstring(to, str, len);
			break;
		}
		case LUA_TTABLE:
		{
			lua_newtable(to);		
			lua_pushnil(from); // First key
			while(lua_next(from, -2))
			{
				// Move value to the other state
				moveValue(from, to); // Value is popped, key is left
				// Move key to the other state
				lua_pushvalue(from, -1); // Make a copy of the key to use for the next iteration
				moveValue(from, to); // Key is in other state.
				// We still have the key in the 'from' state ontop of the stack

				lua_insert(to, -2); // Move key above value
				lua_settable(to, -3); // Set the key
			}

			break;
		}
		default:
			break;
	}

	lua_pop(from, 1); // Pop the value we just read
}
