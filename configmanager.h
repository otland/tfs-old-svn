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
		virtual ~ConfigManager();

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
			SQL_HOST,
			SQL_USER,
			SQL_PASS,
			SQL_DB,
			DEFAULT_PRIORITY,
			#ifdef MULTI_SQL_DRIVERS
			SQL_TYPE,
			#endif
			SQL_FILE,
			PASSWORD_TYPE,
			MAP_AUTHOR,
			LAST_STRING_CONFIG /* this must be the last one */
		};

		enum number_config_t
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
			GLOBALSAVE_H,
			START_LEVEL,
			START_MAGICLEVEL,
			HOUSE_PRICE,
			KILLS_TO_RED,
			KILLS_TO_BAN,
			HIGHSCORES_TOP,
			MAX_MESSAGEBUFFER,
			HIGHSCORES_UPDATETIME,
			ACTIONS_DELAY_INTERVAL,
			EX_ACTIONS_DELAY_INTERVAL,
			CRITICAL_HIT_CHANCE,
			KICK_AFTER_MINUTES,
			PROTECTION_LEVEL,
			PASSWORDTYPE,
			STATUSQUERY_TIMEOUT,
			LEVEL_TO_FORM_GUILD,
			MIN_GUILDNAME,
			MAX_GUILDNAME,
			LEVEL_TO_BUY_HOUSE,
			HOUSES_PER_ACCOUNT,
			WHITE_SKULL_TIME,
			FRAG_TIME,
			MAX_VIOLATIONCOMMENT_SIZE,
			NOTATIONS_TO_BAN,
			WARNINGS_TO_FINALBAN,
			WARNINGS_TO_DELETION,
			BAN_LENGTH,
			FINALBAN_LENGTH,
			IPBANISHMENT_LENGTH,
			MAX_PLAYER_SUMMONS,
			LAST_NUMBER_CONFIG /* this must be the last one */
		};

		enum double_config_t
		{
			LAST_DOUBLE_CONFIG /* this must be the last one */
		};

		enum bool_config_t
		{
			GLOBALSAVE_ENABLED = 0,
			START_CHOOSEVOC,
			ON_OR_OFF_CHARLIST,
			ONE_PLAYER_ON_ACCOUNT,
			REMOVE_AMMO,
			REMOVE_RUNE_CHARGES,
			EXPERIENCE_FROM_PLAYERS,
			RANDOMIZE_TILES,
			SHUTDOWN_AT_GLOBALSAVE,
			CLEAN_MAP_AT_GLOBALSAVE,
			FREE_PREMIUM,
			ADMIN_LOGS_ENABLED,
			GENERATE_ACCOUNT_NUMBER,
			BANK_SYSTEM,
			REMOVE_PREMIUM_ON_INIT,
			//TELEPORT_SUMMONS,
			//TELEPORT_PLAYER_SUMMONS,
			PVP_TILE_IGNORE_PROTECTION,
			DISPLAY_CRITICAL_HIT,
			ADVANCING_SKILL_LEVEL,
			CLEAN_PROTECTED_ZONES,
			//SPELL_NAME_INSTEAD_WORDS,
			REPLACE_KICK_ON_LOGIN,
			PREMIUM_FOR_PROMOTION,
			SHOW_HEALING_DAMAGE,
			BROADCAST_BANISHMENTS,
			SAVE_GLOBAL_STORAGE,
			INGAME_GUILD_MANAGEMENT,
			HOUSE_BUY_AND_SELL,
			HOUSE_NEED_PREMIUM,
			HOUSE_RENTASPRICE,
			HOUSE_PRICEASRENT,
			ACCOUNT_MANAGER,
			NAMELOCK_MANAGER,
			ALLOW_CHANGEOUTFIT,
			CANNOT_ATTACK_SAME_LOOKFEET,
			AIMBOT_HOTKEY_ENABLED,
			FORCE_CLOSE_SLOW_CONNECTION,
			LAST_BOOL_CONFIG /* this must be the last one */
		};

		bool loadFile(const std::string& _filename);
		bool reload();

		const std::string& getString(uint32_t _what) const;
		bool getBool(uint32_t _what) const;
		int32_t getNumber(uint32_t _what) const;
		double getDouble(uint32_t _what) const;

		bool setNumber(uint32_t _what, int32_t _value);

	private:
		std::string getGlobalString(lua_State* _L, const std::string& _identifier, const std::string& _default = "");
		bool getGlobalBool(lua_State* _L, const std::string& _identifier, const std::string& _default = "no");
		int32_t getGlobalNumber(lua_State* _L, const std::string& _identifier, const int32_t _default = 0);
		double getGlobalDouble(lua_State* _L, const std::string& _identifier, const int32_t _default = 0);

		std::string getGlobalStringField(lua_State* _L, const std::string& _identifier, const int32_t _key, const std::string& _default = "");

		bool m_isLoaded;
		std::string m_confString[LAST_STRING_CONFIG];
		bool m_confBool[LAST_BOOL_CONFIG];
		int32_t m_confNumber[LAST_NUMBER_CONFIG];
		double m_confDouble[LAST_DOUBLE_CONFIG];
};

#endif /* _CONFIG_MANAGER_H */
