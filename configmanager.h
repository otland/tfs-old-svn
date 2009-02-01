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

#ifndef _CONFIG_MANAGER_H
#define _CONFIG_MANAGER_H

#include <string>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

class ConfigManager
{
	public:
		ConfigManager();
		~ConfigManager();

		enum string_config_t
		{
			DUMMY_STR = 0,
			CONFIG_FILE,
			MAP_NAME,
			HOUSE_RENT_PERIOD,
			LOGIN_MSG,
			FIRST_MSG,
			SERVER_NAME,
			OWNER_NAME,
			OWNER_EMAIL,
			URL,
			LOCATION,
			IP,
			MOTD,
			WORLD_TYPE,
			MYSQL_HOST,
			MYSQL_USER,
			MYSQL_PASS,
			MYSQL_DB,
			ACCOUNT_MANAGER,
			START_CHOOSEVOC,
			ON_OR_OFF_CHARLIST,
			ALLOW_CHANGEOUTFIT,
			CANNOT_ATTACK_SAME_LOOKFEET,
			ONE_PLAYER_ON_ACCOUNT,
			AIMBOT_HOTKEY_ENABLED,
			SHOW_GAMEMASTERS_ONLINE,
			REMOVE_AMMO,
			REMOVE_RUNE_CHARGES,
			RANDOMIZE_TILES,
			DEFAULT_PRIORITY,
			EXPERIENCE_FROM_PLAYERS,
			SHUTDOWN_AT_SERVERSAVE,
			CLEAN_MAP_AT_SERVERSAVE,
			SERVERSAVE_ENABLED,
			FREE_PREMIUM,
			SQLITE_DB,
			#if defined __USE_MYSQL__ && defined __USE_SQLITE__
			SQL_TYPE,
			#endif
			PASSWORDTYPE,
			ADMIN_LOGS_ENABLED,
			MAP_AUTHOR,
			BROADCAST_BANISHMENTS,
			GENERATE_ACCOUNT_NUMBER,
			SAVE_GLOBAL_STORAGE,
			INGAME_GUILD_SYSTEM,
			REPLACE_KICK_ON_LOGIN,
			ENABLE_RULE_VIOLATION_REPORTS,
			OLD_CONDITION_ACCURACY,
			LAST_STRING_CONFIG /* this must be the last one */
		};

		enum integer_config_t
		{
			LOGIN_TRIES = 0,
			RETRY_TIMEOUT,
			LOGIN_TIMEOUT,
			PORT,
			SQL_PORT,
			MAX_PLAYERS,
			PZ_LOCKED,
			DEFAULT_DESPAWNRANGE,
			DEFAULT_DESPAWNRADIUS,
			ALLOW_CLONES,
			RATE_EXPERIENCE,
			RATE_SKILL,
			RATE_LOOT,
			RATE_MAGIC,
			RATE_SPAWN,
			SPAWNPOS_X,
			SPAWNPOS_Y,
			SPAWNPOS_Z,
			SPAWNTOWN_ID,
			SERVERSAVE_H,
			START_LEVEL,
			START_MAGICLEVEL,
			HOUSE_PRICE,
			KILLS_TO_RED,
			KILLS_TO_BAN,
			BAN_DAYS,
			FINAL_BAN_DAYS,
			HIGHSCORES_TOP,
			MAX_MESSAGEBUFFER,
			HIGHSCORES_UPDATETIME,
			ACTIONS_DELAY_INTERVAL,
			EX_ACTIONS_DELAY_INTERVAL,
			CRITICAL_HIT_CHANCE,
			KICK_AFTER_MINUTES,
			PROTECTION_LEVEL,
			DEATH_LOSE_PERCENT,
			PASSWORD_TYPE,
			#if defined __USE_MYSQL__ && defined __USE_SQLITE__
			SQLTYPE,
			#endif
			STATUSQUERY_TIMEOUT,
			FRAG_TIME,
			WHITE_SKULL_TIME,
			AUTO_SAVE_EACH_MINUTES,
			ALTERNATIVE_EXHAUST,
			LAST_INTEGER_CONFIG /* this must be the last one */
		};

		bool loadFile(const std::string& _filename);
		bool reload();

		const std::string& getString(uint32_t _what) const;
		int32_t getNumber(uint32_t _what) const;
		bool setNumber(uint32_t _what, int32_t _value);

	private:
		std::string getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default="");
		int32_t getGlobalNumber(lua_State* _L, const std::string& _identifier, const int32_t _default=0);
		std::string getGlobalStringField(lua_State* _L, const std::string& _identifier, const int32_t _key, const std::string& _default="");

		bool m_isLoaded;
		std::string m_confString[LAST_STRING_CONFIG];
		int32_t m_confInteger[LAST_INTEGER_CONFIG];
};

#endif /* _CONFIG_MANAGER_H */
