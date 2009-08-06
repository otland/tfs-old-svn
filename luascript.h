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

#ifndef __LUASCRIPT__
#define __LUASCRIPT__
#include "otsystem.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include "position.h"
#include "database.h"

class Thing;
class Creature;
class Player;
class Item;
class Container;
class AreaCombat;
class Combat;
class Condition;
class Npc;

enum LuaVariantType_t
{
	VARIANT_NONE = 0,
	VARIANT_NUMBER,
	VARIANT_POSITION,
	VARIANT_TARGETPOSITION,
	VARIANT_STRING
};

struct LuaVariant
{
	LuaVariant()
	{
		type = VARIANT_NONE;
		text = "";
		pos.x = pos.y = pos.z = pos.stackpos = number = 0;
	}

	LuaVariantType_t type;
	std::string text;
	PositionEx pos;
	uint32_t number;
};

class LuaScriptInterface;
class Game;
class Npc;

class ScriptEnviroment
{
	public:
		ScriptEnviroment();
		virtual ~ScriptEnviroment();

		void resetEnv();
		void resetCallback() {m_callbackId = 0;}

		static bool saveGameState();
		static bool loadGameState();

		LuaScriptInterface* getScriptInterface() {return m_interface;}
		int32_t getScriptId() {return m_scriptId;}
		int32_t getCallbackId() {return m_callbackId;}
		std::string getEventDesc() {return m_eventdesc;}
		void getEventInfo(int32_t& scriptId, std::string& desc, LuaScriptInterface*& scriptInterface, int32_t& callbackId, bool& timerEvent);

		void setScriptId(int32_t scriptId, LuaScriptInterface* scriptInterface)
			{m_scriptId = scriptId; m_interface = scriptInterface;}
		bool setCallbackId(int32_t callbackId, LuaScriptInterface* scriptInterface);
		void setEventDesc(const std::string& desc) {m_eventdesc = desc;}

		void setTimerEvent() {m_timerEvent = true;}
		void resetTimerEvent() {m_timerEvent = false;}

		static void addUniqueThing(Thing* thing);
		static void removeUniqueThing(Thing* thing);

		uint32_t addThing(Thing* thing);
		void insertThing(uint32_t uid, Thing* thing);

		void addTempItem(Item* item);
		void removeTempItem(Item* item);

		uint32_t addResult(DBResult* res);
		bool removeResult(uint32_t rid);
		DBResult* getResult(uint32_t rid);

		bool getGlobalStorageValue(const uint32_t key, std::string& value) const;
		bool addGlobalStorageValue(const uint32_t key, const std::string& value);
		bool eraseGlobalStorageValue(const uint32_t key);

		void streamVariant(std::stringstream& stream, const std::string& local, const LuaVariant& var);
		void streamThing(std::stringstream& stream, const std::string& local, Thing* thing, uint32_t thingId);
		void streamPosition(std::stringstream& stream, const std::string& local, const PositionEx& position);
		void streamPosition(std::stringstream& stream, const std::string& local, const Position& position, uint32_t stackpos);

		void setRealPos(const Position& realPos) {m_realPos = realPos;}
		Position getRealPos() {return m_realPos;}

		void setNpc(Npc* npc) {m_curNpc = npc;}
		Npc* getNpc() const {return m_curNpc;}

		Thing* getThingByUID(uint32_t uid);
		Item* getItemByUID(uint32_t uid);
		Container* getContainerByUID(uint32_t uid);
		Creature* getCreatureByUID(uint32_t uid);
		Player* getPlayerByUID(uint32_t uid);

		void removeItemByUID(uint32_t uid);

		static uint32_t addCombatArea(AreaCombat* area);
		static AreaCombat* getCombatArea(uint32_t areaId);

		static uint32_t addCombatObject(Combat* combat);
		static Combat* getCombatObject(uint32_t combatId);

		static uint32_t addConditionObject(Condition* condition);
		static Condition* getConditionObject(uint32_t conditionId);

		static uint32_t getLastCombatId() {return m_lastCombatId;}

	private:
		typedef std::map<uint64_t, Thing*> ThingMap;
		typedef std::vector<const LuaVariant*> VariantVector;
		typedef std::map<uint32_t, std::string> StorageMap;
		typedef std::map<uint32_t, AreaCombat*> AreaMap;
		typedef std::map<uint32_t, Combat*> CombatMap;
		typedef std::map<uint32_t, Condition*> ConditionMap;
		typedef std::list<Item*> ItemList;
		typedef std::map<uint32_t, DBResult*> DBResMap;

		//script file id
		LuaScriptInterface* m_interface;
		int32_t m_scriptId, m_callbackId;
		std::string m_eventdesc;
		bool m_timerEvent;
		//script event desc

		static StorageMap m_globalStorageMap;
		static ThingMap m_globalMap;

		ThingMap m_localMap;
		ItemList m_tempItems;
		DBResMap m_tempResults;

		//area map
		static uint32_t m_lastAreaId;
		static AreaMap m_areaMap;

		//combat map
		static uint32_t m_lastCombatId;
		static CombatMap m_combatMap;

		//condition map
		static uint32_t m_lastConditionId;
		static ConditionMap m_conditionMap;

		//for npc scripts
		Npc* m_curNpc;
		int32_t m_lastUID;
		Position m_realPos;
		bool m_loaded;
};

enum PlayerInfo_t
{
	PlayerInfoFood,
	PlayerInfoAccess,
	PlayerInfoGhostAccess,
	PlayerInfoLevel,
	PlayerInfoExperience,
	PlayerInfoManaSpent,
	PlayerInfoVocation,
	PlayerInfoTown,
	PlayerInfoPromotionLevel,
	PlayerInfoSoul,
	PlayerInfoFreeCap,
	PlayerInfoGuildId,
	PlayerInfoGuildName,
	PlayerInfoGuildRankId,
	PlayerInfoGuildRank,
	PlayerInfoGuildLevel,
	PlayerInfoGuildNick,
	PlayerInfoGroupId,
	PlayerInfoGUID,
	PlayerInfoAccountId,
	PlayerInfoAccount,
	PlayerInfoPremiumDays,
	PlayerInfoBalance,
	PlayerInfoStamina,
	PlayerInfoLossSkill,
	PlayerInfoMarriage,
	PlayerInfoPzLock,
	PlayerInfoSaving,
	PlayerInfoIp,
	PlayerInfoSkullEnd,
	PlayerInfoOutfitWindow,
	PlayerInfoNameDescription,
	PlayerInfoSpecialDescription,
	PlayerInfoIdleTime,
	PlayerInfoClient,
	PlayerInfoLastLogin,
	PlayerInfoLastLoginSaved,
	PlayerInfoAccountManager
};

enum ErrorCode_t
{
	LUA_ERROR_PLAYER_NOT_FOUND,
	LUA_ERROR_MONSTER_NOT_FOUND,
	LUA_ERROR_NPC_NOT_FOUND,
	LUA_ERROR_CREATURE_NOT_FOUND,
	LUA_ERROR_ITEM_NOT_FOUND,
	LUA_ERROR_THING_NOT_FOUND,
	LUA_ERROR_TILE_NOT_FOUND,
	LUA_ERROR_HOUSE_NOT_FOUND,
	LUA_ERROR_COMBAT_NOT_FOUND,
	LUA_ERROR_CONDITION_NOT_FOUND,
	LUA_ERROR_AREA_NOT_FOUND,
	LUA_ERROR_CONTAINER_NOT_FOUND,
	LUA_ERROR_VARIANT_NOT_FOUND,
	LUA_ERROR_VARIANT_UNKNOWN,
	LUA_ERROR_SPELL_NOT_FOUND
};

#define reportErrorFunc(a) reportError(__FUNCTION__, a)

class Position;

class LuaScriptInterface
{
	public:
		LuaScriptInterface(std::string interfaceName);
		virtual ~LuaScriptInterface();

		virtual bool initState();
		bool reInitState();

		int32_t loadBuffer(const std::string& text, Npc* npc = NULL);
		int32_t loadFile(const std::string& file, Npc* npc = NULL);

		static ScriptEnviroment* getScriptEnv()
		{
			assert(m_scriptEnvIndex >= 0 && m_scriptEnvIndex < 21);
			return &m_scriptEnv[m_scriptEnvIndex];
		}
		static bool reserveScriptEnv()
		{
			++m_scriptEnvIndex;
			if(m_scriptEnvIndex > 20)
			{
				--m_scriptEnvIndex;
				return false;
			}

			return true;
		}
		static void releaseScriptEnv()
		{
			if(m_scriptEnvIndex >= 0)
			{
				m_scriptEnv[m_scriptEnvIndex].resetEnv();
				--m_scriptEnvIndex;
			}
		}

		std::string getInterfaceName() {return m_interfaceName;}
		lua_State* getLuaState() {return m_luaState;}
		const std::string& getLastLuaError() const {return m_lastLuaError;}

		bool pushFunction(int32_t functionId);
		int32_t callFunction(uint32_t nParams);

		static int32_t luaErrorHandler(lua_State* L);
		static void reportError(const char* function, const std::string& error_desc);

		int32_t getEvent(const std::string& eventName);
		const std::string& getFileById(int32_t scriptId);

		void dumpLuaStack();

		//push/pop common structures
		static void pushThing(lua_State* L, Thing* thing, uint32_t thingid);
		static void pushVariant(lua_State* L, const LuaVariant& var);
		static void pushPosition(lua_State* L, const PositionEx& position);
		static void pushPosition(lua_State* L, const Position& position, uint32_t stackpos);
		static void pushCallback(lua_State* L, int32_t callback);

		static LuaVariant popVariant(lua_State* L);
		static void popPosition(lua_State* L, PositionEx& position);
		static void popPosition(lua_State* L, Position& position, uint32_t& stackpos);
		static bool popBoolean(lua_State* L);
		static int64_t popNumber(lua_State* L);
		static double popFloatNumber(lua_State* L);
		static std::string popString(lua_State* L);
		static int32_t popCallback(lua_State* L);

		static int32_t getField(lua_State* L, const char* key);
		static uint32_t getFieldU32(lua_State* L, const char* key);
		static void setField(lua_State* L, const char* index, int32_t val);
		static void setField(lua_State* L, const char* index, const std::string& val);
		static std::string getFieldString(lua_State* L, const char* key);
		static void setFieldBool(lua_State* L, const char* index, bool val);
		static bool getFieldBool(lua_State* L, const char* key);
		static void setFieldDouble(lua_State* L, const char* index, double val);

		static std::string getGlobalString(lua_State* L, const std::string& _identifier, const std::string& _default = "");
		static bool getGlobalBool(lua_State* L, const std::string& _identifier, bool _default = false);
		static int32_t getGlobalNumber(lua_State* L, const std::string& _identifier, const int32_t _default = 0);
		static double getGlobalDouble(lua_State* L, const std::string& _identifier, const double _default = 0);
		static void getValue(const std::string& key, lua_State* L, lua_State* _L);
		static void moveValue(lua_State* from, lua_State* to);

	protected:
		virtual bool closeState();

		virtual void registerFunctions();

		static std::string getErrorDesc(ErrorCode_t code);
		static bool getArea(lua_State* L, std::list<uint32_t>& list, uint32_t& rows);

		//lua functions
		static int32_t luaDoRemoveItem(lua_State* L);
		static int32_t luaDoFeedPlayer(lua_State* L);
		static int32_t luaDoPlayerSendCancel(lua_State* L);
		static int32_t luaDoSendDefaultCancel(lua_State* L);
		static int32_t luaGetSearchString(lua_State* L);
		static int32_t luaGetClosestFreeTile(lua_State* L);
		static int32_t luaDoTeleportThing(lua_State* L);
		static int32_t luaDoTransformItem(lua_State* L);
		static int32_t luaDoSendMagicEffect(lua_State* L);
		static int32_t luaDoChangeTypeItem(lua_State* L);
		static int32_t luaDoSendAnimatedText(lua_State* L);
		static int32_t luaDoSendDistanceShoot(lua_State* L);
		static int32_t luaDoShowTextWindow(lua_State* L);
		static int32_t luaDoShowTextDialog(lua_State* L);
		static int32_t luaDoDecayItem(lua_State* L);
		static int32_t luaDoCreateItem(lua_State* L);
		static int32_t luaDoCreateItemEx(lua_State* L);
		static int32_t luaDoCreateTeleport(lua_State* L);
		static int32_t luaDoCreateMonster(lua_State* L);
		static int32_t luaDoCreateNpc(lua_State* L);
		static int32_t luaDoSummonMonster(lua_State* L);
		static int32_t luaDoConvinceCreature(lua_State* L);
		static int32_t luaGetMonsterTargetList(lua_State* L);
		static int32_t luaGetMonsterFriendList(lua_State* L);
		static int32_t luaDoMonsterSetTarget(lua_State* L);
		static int32_t luaDoMonsterChangeTarget(lua_State* L);
		static int32_t luaDoAddCondition(lua_State* L);
		static int32_t luaDoRemoveCondition(lua_State* L);
		static int32_t luaDoRemoveConditions(lua_State* L);
		static int32_t luaDoRemoveCreature(lua_State* L);
		static int32_t luaDoMoveCreature(lua_State* L);

		static int32_t luaDoCreatureSay(lua_State* L);

		static int32_t luaDoPlayerAddSkillTry(lua_State* L);
		static int32_t luaDoCreatureAddHealth(lua_State* L);
		static int32_t luaDoCreatureAddMana(lua_State* L);
		static int32_t luaSetCreatureMaxHealth(lua_State* L);
		static int32_t luaSetCreatureMaxMana(lua_State* L);
		static int32_t luaDoPlayerSetMaxCapacity(lua_State* L);
		static int32_t luaDoPlayerAddSpentMana(lua_State* L);
		static int32_t luaDoPlayerAddItem(lua_State* L);
		static int32_t luaDoPlayerAddItemEx(lua_State* L);
		static int32_t luaDoTileAddItemEx(lua_State* L);
		static int32_t luaDoAddContainerItemEx(lua_State* L);
		static int32_t luaDoRelocate(lua_State* L);
		static int32_t luaDoCleanTile(lua_State* L);
		static int32_t luaDoPlayerSendTextMessage(lua_State* L);
		static int32_t luaDoPlayerSendChannelMessage(lua_State* L);
		static int32_t luaDoPlayerSendToChannel(lua_State* L);
		static int32_t luaDoPlayerAddMoney(lua_State* L);
		static int32_t luaDoPlayerRemoveMoney(lua_State* L);
		static int32_t luaDoPlayerTransferMoneyTo(lua_State* L);
		static int32_t luaDoPlayerSetPzLocked(lua_State* L);
		static int32_t luaDoPlayerSetTown(lua_State* L);
		static int32_t luaDoPlayerSetVocation(lua_State* L);
		static int32_t luaDoPlayerRemoveItem(lua_State* L);
		static int32_t luaDoPlayerAddSoul(lua_State* L);
		static int32_t luaDoPlayerAddStamina(lua_State* L);
		static int32_t luaDoPlayerSetStamina(lua_State* L);
		static int32_t luaDoPlayerAddExperience(lua_State* L);
		static int32_t luaDoPlayerSetGuildId(lua_State* L);
		static int32_t luaDoPlayerSetGuildRank(lua_State* L);
		static int32_t luaDoPlayerSetGuildNick(lua_State* L);
		static int32_t luaDoPlayerSetSex(lua_State* L);
		static int32_t luaDoPlayerSetIdleTime(lua_State* L);
		static int32_t luaGetPlayerIdleTime(lua_State* L);
		static int32_t luaDoSetCreatureLight(lua_State* L);
		static int32_t luaDoCreatureSetLookDir(lua_State* L);
		static int32_t luaGetCreatureHideHealth(lua_State* L);
		static int32_t luaDoCreatureSetHideHealth(lua_State* L);
		static int32_t luaGetCreatureSpeakType(lua_State* L);
		static int32_t luaDoCreatureSetSpeakType(lua_State* L);
		static int32_t luaGetCreatureSkullType(lua_State* L);
		static int32_t luaDoCreatureSetSkullType(lua_State* L);
		static int32_t luaGetPlayerSkullEnd(lua_State* L);
		static int32_t luaDoPlayerSetSkullEnd(lua_State* L);
		static int32_t luaDoPlayerSwitchSaving(lua_State* L);
		static int32_t luaDoPlayerSave(lua_State* L);
		static int32_t luaDoPlayerSendOutfitWindow(lua_State* L);
		static int32_t luaDoCreatureExecuteTalkAction(lua_State* L);

		//queries
		static int32_t luaGetCreatureByName(lua_State* L);
		static int32_t luaGetPlayerByNameWildcard(lua_State* L);
		static int32_t luaGetPlayerGUIDByName(lua_State* L);
		static int32_t luaGetPlayerNameByGUID(lua_State* L);
		static int32_t luaGetPlayersByAccountId(lua_State* L);
		static int32_t luaGetAccountIdByName(lua_State* L);
		static int32_t luaGetAccountByName(lua_State* L);
		static int32_t luaGetAccountIdByAccount(lua_State* L);
		static int32_t luaGetAccountByAccountId(lua_State* L);
		static int32_t luaGetIpByName(lua_State* L);
		static int32_t luaGetPlayersByIp(lua_State* L);

		//bans
		static int32_t luaIsIpBanished(lua_State* L);
		static int32_t luaIsPlayerBanished(lua_State* L);
		static int32_t luaIsAccountBanished(lua_State* L);
		static int32_t luaDoAddIpBanishment(lua_State* L);
		static int32_t luaDoAddPlayerBanishment(lua_State* L);
		static int32_t luaDoAddAccountBanishment(lua_State* L);
		static int32_t luaDoAddNotation(lua_State* L);
		static int32_t luaDoAddStatement(lua_State* L);
		static int32_t luaDoRemoveIpBanishment(lua_State* L);
		static int32_t luaDoRemovePlayerBanishment(lua_State* L);
		static int32_t luaDoRemoveAccountBanishment(lua_State* L);
		static int32_t luaDoRemoveNotations(lua_State* L);
		static int32_t luaDoRemoveStatements(lua_State* L);
		static int32_t luaGetNotationsCount(lua_State* L);
		static int32_t luaGetStatementsCount(lua_State* L);
		static int32_t luaGetBanData(lua_State* L);
		static int32_t luaGetBanReason(lua_State* L);
		static int32_t luaGetBanAction(lua_State* L);
		static int32_t luaGetBanList(lua_State* L);

		//custom modifiers
		static int32_t luaGetPlayerRates(lua_State* L);
		static int32_t luaDoPlayerSetRate(lua_State* L);
		static int32_t luaDoCreatureSetDropLoot(lua_State* L);
		static int32_t luaGetPlayerLossPercent(lua_State* L);
		static int32_t luaDoPlayerSetLossPercent(lua_State* L);
		static int32_t luaDoPlayerSetLossSkill(lua_State* L);
		static int32_t luaGetPlayerLossSkill(lua_State* L);

		//get item info
		static int32_t luaGetItemWeaponType(lua_State* L);
		static int32_t luaGetItemRWInfo(lua_State* L);
		static int32_t luaGetItemProtection(lua_State* L);
		static int32_t luaGetThingFromPos(lua_State* L);
		static int32_t luaGetThing(lua_State* L);
		static int32_t luaGetThingPos(lua_State* L);
		static int32_t luaGetTileItemById(lua_State* L);
		static int32_t luaGetTileItemByType(lua_State* L);
		static int32_t luaGetTileThingByPos(lua_State* L);
		static int32_t luaGetTopCreature(lua_State* L);
		static int32_t luaHasProperty(lua_State* L);
		static int32_t luaHasClient(lua_State* L);

		//set item
		static int32_t luaDoSetItemActionId(lua_State* L);
		static int32_t luaDoSetItemText(lua_State* L);
		static int32_t luaDoSetItemSpecialDescription(lua_State* L);
		static int32_t luaDoSetItemProtection(lua_State* L);
		static int32_t luaDoItemRaidUnref(lua_State* L);

		//get tile info
		static int32_t luaGetTileInfo(lua_State* L);
		static int32_t luaDoTileQueryAdd(lua_State* L);

		//houses
		static int32_t luaGetHouseInfo(lua_State* L);
		static int32_t luaGetHouseAccessList(lua_State* L);
		static int32_t luaGetHouseByPlayerGUID(lua_State* L);
		static int32_t luaGetHouseFromPos(lua_State* L);
		static int32_t luaSetHouseOwner(lua_State* L);
		static int32_t luaSetHouseAccessList(lua_State* L);

		//get creature info functions
		static int32_t luaDoPlayerSetNameDescription(lua_State* L);
		static int32_t luaGetPlayerNameDescription(lua_State* L);
		static int32_t luaDoPlayerSetSpecialDescription(lua_State* L);
		static int32_t luaGetPlayerSpecialDescription(lua_State* L);
		static int32_t luaGetPlayerFood(lua_State* L);
		static int32_t luaGetPlayerAccess(lua_State* L);
		static int32_t luaGetPlayerGhostAccess(lua_State* L);
		static int32_t luaGetPlayerLevel(lua_State* L);
		static int32_t luaGetPlayerExperience(lua_State* L);
		static int32_t luaGetPlayerMagLevel(lua_State* L);
		static int32_t luaGetPlayerSpentMana(lua_State* L);
		static int32_t luaGetCreatureMana(lua_State* L);
		static int32_t luaGetCreatureMaxMana(lua_State* L);
		static int32_t luaGetCreatureHealth(lua_State* L);
		static int32_t luaGetCreatureMaxHealth(lua_State* L);
		static int32_t luaGetCreatureSpeed(lua_State* L);
		static int32_t luaGetCreatureBaseSpeed(lua_State* L);
		static int32_t luaGetCreatureTarget(lua_State* L);
		static int32_t luaGetCreatureLookDirection(lua_State* L);
		static int32_t luaGetPlayerSkillLevel(lua_State* L);
		static int32_t luaGetPlayerSkillTries(lua_State* L);
		static int32_t luaGetPlayerVocation(lua_State* L);
		static int32_t luaGetPlayerTown(lua_State* L);
		static int32_t luaGetPlayerItemCount(lua_State* L);
		static int32_t luaGetPlayerMoney(lua_State* L);
		static int32_t luaGetPlayerSoul(lua_State* L);
		static int32_t luaGetPlayerStamina(lua_State* L);
		static int32_t luaGetPlayerFreeCap(lua_State* L);
		static int32_t luaGetPlayerLight(lua_State* L);
		static int32_t luaGetPlayerSlotItem(lua_State* L);
		static int32_t luaGetPlayerWeapon(lua_State* L);
		static int32_t luaGetPlayerItemById(lua_State* L);
		static int32_t luaGetPlayerRequiredMana(lua_State* L);
		static int32_t luaGetPlayerRequiredSkillTries(lua_State* L);
		static int32_t luaGetPlayerIp(lua_State* L);
		static int32_t luaGetPlayerLastLogin(lua_State* L);
		static int32_t luaGetPlayerLastLoginSaved(lua_State* L);
		static int32_t luaGetPlayerAccountManager(lua_State* L);
		static int32_t luaGetPlayerAccountId(lua_State* L);
		static int32_t luaGetPlayerAccount(lua_State* L);

		static int32_t luaGetPlayerDepotItems(lua_State* L);
		static int32_t luaGetPlayerGuildId(lua_State* L);
		static int32_t luaGetPlayerGuildName(lua_State* L);
		static int32_t luaGetPlayerGuildRank(lua_State* L);
		static int32_t luaGetPlayerGuildRankId(lua_State* L);
		static int32_t luaGetPlayerGuildLevel(lua_State* L);
		static int32_t luaGetPlayerGuildNick(lua_State* L);
		static int32_t luaGetPlayerSex(lua_State* L);
		static int32_t luaGetPlayerGUID(lua_State* L);
		static int32_t luaGetPlayerFlagValue(lua_State* L);
		static int32_t luaGetPlayerCustomFlagValue(lua_State* L);
		static int32_t luaGetCreatureCondition(lua_State* L);

		static int32_t luaGetDepotId(lua_State* L);
		static int32_t luaGetVocationInfo(lua_State* L);
		static int32_t luaGetGroupInfo(lua_State* L);
		static int32_t luaGetMonsterInfo(lua_State* L);
		static int32_t luaGetMonsterHealingSpells(lua_State* L);
		static int32_t luaGetMonsterAttackSpells(lua_State* L);
		static int32_t luaGetMonsterLootList(lua_State* L);
		static int32_t luaGetMonsterSummonList(lua_State* L);

		static int32_t luaGetPlayerPromotionLevel(lua_State* L);
		static int32_t luaDoPlayerSetPromotionLevel(lua_State* L);
		static int32_t luaGetPlayerGroupId(lua_State* L);
		static int32_t luaDoPlayerSetGroupId(lua_State* L);

		static int32_t luaDoPlayerLearnInstantSpell(lua_State* L);
		static int32_t luaDoPlayerUnlearnInstantSpell(lua_State* L);
		static int32_t luaGetPlayerLearnedInstantSpell(lua_State* L);
		static int32_t luaGetPlayerInstantSpellCount(lua_State* L);
		static int32_t luaGetPlayerInstantSpellInfo(lua_State* L);
		static int32_t luaGetInstantSpellInfo(lua_State* L);

		static int32_t luaGetPlayerPartner(lua_State* L);
		static int32_t luaDoPlayerSetPartner(lua_State* L);
		static int32_t luaGetPlayerParty(lua_State* L);
		static int32_t luaDoPlayerJoinParty(lua_State* L);
		static int32_t luaGetPartyMembers(lua_State* L);

		static int32_t luaGetPlayerStorageValue(lua_State* L);
		static int32_t luaDoPlayerSetStorageValue(lua_State* L);
		static int32_t luaDoPlayerAddBlessing(lua_State* L);
		static int32_t luaGetPlayerBlessing(lua_State* L);

		static int32_t luaGetGlobalStorageValue(lua_State* L);
		static int32_t luaSetGlobalStorageValue(lua_State* L);

		static int32_t luaDoPlayerAddOutfit(lua_State* L);
		static int32_t luaDoPlayerRemoveOutfit(lua_State* L);
		static int32_t luaDoPlayerAddOutfitId(lua_State* L);
		static int32_t luaDoPlayerRemoveOutfitId(lua_State* L);
		static int32_t luaCanPlayerWearOutfit(lua_State* L);

		static int32_t luaGetWorldType(lua_State* L);
		static int32_t luaSetWorldType(lua_State* L);
		static int32_t luaGetWorldTime(lua_State* L);
		static int32_t luaGetWorldLight(lua_State* L);
		static int32_t luaGetWorldCreatures(lua_State* L);
		static int32_t luaGetWorldUpTime(lua_State* L);

		static int32_t luaGetGuildId(lua_State* L);
		static int32_t luaGetGuildMotd(lua_State* L);

		//type validation
		static int32_t luaIsPlayerPzLocked(lua_State* L);
		static int32_t luaIsPlayerSaving(lua_State* L);
		static int32_t luaIsCreature(lua_State* L);
		static int32_t luaIsContainer(lua_State* L);
		static int32_t luaIsCorpse(lua_State* L);
		static int32_t luaIsMovable(lua_State* L);

		//container
		static int32_t luaGetContainerSize(lua_State* L);
		static int32_t luaGetContainerCap(lua_State* L);
		static int32_t luaGetContainerCapById(lua_State* L);
		static int32_t luaGetContainerItem(lua_State* L);
		static int32_t luaDoAddContainerItem(lua_State* L);

		//
		static int32_t luaCreateCombatObject(lua_State* L);
		static int32_t luaCreateCombatArea(lua_State* L);
		static int32_t luaSetCombatArea(lua_State* L);
		static int32_t luaSetCombatCondition(lua_State* L);
		static int32_t luaSetCombatParam(lua_State* L);
		static int32_t luaCreateConditionObject(lua_State* L);
		static int32_t luaSetConditionParam(lua_State* L);
		static int32_t luaAddDamageCondition(lua_State* L);
		static int32_t luaAddOutfitCondition(lua_State* L);

		static int32_t luaSetCombatCallBack(lua_State* L);
		static int32_t luaSetCombatFormula(lua_State* L);
		static int32_t luaSetConditionFormula(lua_State* L);
		static int32_t luaDoCombat(lua_State* L);

		static int32_t luaDoAreaCombatHealth(lua_State* L);
		static int32_t luaDoTargetCombatHealth(lua_State* L);

		//
		static int32_t luaDoAreaCombatMana(lua_State* L);
		static int32_t luaDoTargetCombatMana(lua_State* L);

		static int32_t luaDoAreaCombatCondition(lua_State* L);
		static int32_t luaDoTargetCombatCondition(lua_State* L);

		static int32_t luaDoAreaCombatDispel(lua_State* L);
		static int32_t luaDoTargetCombatDispel(lua_State* L);

		static int32_t luaDoChallengeCreature(lua_State* L);

		static int32_t luaNumberToVariant(lua_State* L);
		static int32_t luaStringToVariant(lua_State* L);
		static int32_t luaPositionToVariant(lua_State* L);
		static int32_t luaTargetPositionToVariant(lua_State* L);

		static int32_t luaVariantToNumber(lua_State* L);
		static int32_t luaVariantToString(lua_State* L);
		static int32_t luaVariantToPosition(lua_State* L);

		static int32_t luaDoChangeSpeed(lua_State* L);
		static int32_t luaGetExperienceStage(lua_State* L);

		static int32_t luaDoCreatureChangeOutfit(lua_State* L);
		static int32_t luaSetCreatureOutfit(lua_State* L);
		static int32_t luaGetCreatureOutfit(lua_State* L);
		static int32_t luaSetMonsterOutfit(lua_State* L);
		static int32_t luaSetItemOutfit(lua_State* L);
		static int32_t luaGetCreaturePosition(lua_State* L);
		static int32_t luaGetCreatureLastPosition(lua_State* L);
		static int32_t luaGetCreatureName(lua_State* L);
		static int32_t luaGetCreatureMaster(lua_State* L);
		static int32_t luaGetCreatureSummons(lua_State* L);

		static int32_t luaIsItemStackable(lua_State* L);
		static int32_t luaIsItemRune(lua_State* L);
		static int32_t luaIsItemFluidContainer(lua_State* L);
		static int32_t luaIsItemContainer(lua_State* L);
		static int32_t luaIsItemMovable(lua_State* L);
		static int32_t luaIsItemDoor(lua_State* L);
		static int32_t luaGetItemLevelDoor(lua_State* L);
		static int32_t luaGetItemIdByName(lua_State* L);

		static int32_t luaIsSightClear(lua_State* L);
		static int32_t luaDebugPrint(lua_State* L);
		static int32_t luaGetFluidSourceType(lua_State* L);
		static int32_t luaGetHighscoreString(lua_State* L);
		static int32_t luaIsInArray(lua_State* L);
		static int32_t luaWait(lua_State* L);
		static int32_t luaAddEvent(lua_State* L);
		static int32_t luaStopEvent(lua_State* L);
		static int32_t luaRegisterCreatureEvent(lua_State* L);

		static int32_t luaGetPlayerBalance(lua_State* L);
		static int32_t luaDoPlayerSetBalance(lua_State* L);
		static int32_t luaDoPlayerPopupFYI(lua_State* L);
		static int32_t luaDoPlayerSendTutorial(lua_State* L);
		static int32_t luaDoPlayerAddMapMark(lua_State* L);
		static int32_t luaGetPlayerPremiumDays(lua_State* L);
		static int32_t luaDoPlayerAddPremiumDays(lua_State* L);
		static int32_t luaGetCreatureNoMove(lua_State* L);
		static int32_t luaDoCreatureSetNoMove(lua_State* L);

		static int32_t luaGetTownId(lua_State* L);
		static int32_t luaGetTownName(lua_State* L);
		static int32_t luaGetTownTemplePosition(lua_State* L);
		static int32_t luaGetTownHouses(lua_State* L);

		static int32_t luaGetSpectators(lua_State* L);
		static int32_t luaGetGameState(lua_State* L);
		static int32_t luaDoSetGameState(lua_State* L);
		static int32_t luaGetChannelUsers(lua_State* L);
		static int32_t luaGetPlayersOnline(lua_State* L);
		static int32_t luaDoExecuteRaid(lua_State* L);
		static int32_t luaDoReloadInfo(lua_State* L);
		static int32_t luaDoSaveServer(lua_State* L);
		static int32_t luaDoCleanHouse(lua_State* L);
		static int32_t luaDoCleanMap(lua_State* L);
		static int32_t luaDoRefreshMap(lua_State* L);
		static int32_t luaDoUpdateHouseAuctions(lua_State* L);

		static int32_t luaGetItemDescriptionsById(lua_State* L);
		static int32_t luaGetItemWeightById(lua_State* L);
		static int32_t luaGetItemDescriptions(lua_State* L);
		static int32_t luaGetItemWeight(lua_State* L);
		static int32_t luaSetItemName(lua_State* L);
		static int32_t luaSetItemPluralName(lua_State* L);
		static int32_t luaSetItemArticle(lua_State* L);
		static int32_t luaGetItemAttack(lua_State* L);
		static int32_t luaSetItemAttack(lua_State* L);
		static int32_t luaGetItemExtraAttack(lua_State* L);
		static int32_t luaSetItemExtraAttack(lua_State* L);
		static int32_t luaGetItemDefense(lua_State* L);
		static int32_t luaSetItemDefense(lua_State* L);
		static int32_t luaGetItemExtraDefense(lua_State* L);
		static int32_t luaSetItemExtraDefense(lua_State* L);
		static int32_t luaGetItemArmor(lua_State* L);
		static int32_t luaSetItemArmor(lua_State* L);
		static int32_t luaGetItemAttackSpeed(lua_State* L);
		static int32_t luaSetItemAttackSpeed(lua_State* L);
		static int32_t luaGetItemHitChance(lua_State* L);
		static int32_t luaSetItemHitChance(lua_State* L);
		static int32_t luaGetItemShootRange(lua_State* L);
		static int32_t luaSetItemShootRange(lua_State* L);

		static int32_t luaGetTalkActionList(lua_State* L);
		static int32_t luaGetExperienceStageList(lua_State* L);
		static int32_t luaGetWaypointList(lua_State* L);
		static int32_t luaGetWaypointPosition(lua_State* L);
		static int32_t luaDoWaypointAddTemporial(lua_State* L);
		static int32_t luaGetDataDir(lua_State* L);
		static int32_t luaGetLogsDir(lua_State* L);
		static int32_t luaGetConfigFile(lua_State* L);
		static int32_t luaGetConfigValue(lua_State* L);
		static int32_t luaGetModList(lua_State* L);

		static int32_t luaL_loadmodlib(lua_State* L);
		static int32_t luaL_domodlib(lua_State* L);

		static int32_t luaHashMD5(lua_State* L);
		static int32_t luaHashSHA1(lua_State* L);
		static int32_t luaPrint(lua_State* L);
		static int32_t internalGetPlayerInfo(lua_State* L, PlayerInfo_t info);

		static const luaL_Reg luaDatabaseReg[8];
		static int32_t luaDatabaseExecute(lua_State* L);
		static int32_t luaDatabaseStoreQuery(lua_State* L);
		static int32_t luaDatabaseEscapeString(lua_State* L);
		static int32_t luaDatabaseEscapeBlob(lua_State* L);
		static int32_t luaDatabaseLastInsertId(lua_State* L);
		static int32_t luaDatabaseStringComparison(lua_State* L);
		static int32_t luaDatabaseUpdateLimiter(lua_State* L);

		static const luaL_Reg luaResultReg[7];
		static int32_t luaResultGetDataInt(lua_State* L);
		static int32_t luaResultGetDataLong(lua_State* L);
		static int32_t luaResultGetDataString(lua_State* L);
		static int32_t luaResultGetDataStream(lua_State* L);
		static int32_t luaResultNext(lua_State* L);
		static int32_t luaResultFree(lua_State* L);

		static const luaL_Reg luaBitReg[13];
		static int32_t luaBitNot(lua_State* L);
		static int32_t luaBitAnd(lua_State* L);
		static int32_t luaBitOr(lua_State* L);
		static int32_t luaBitXor(lua_State* L);
		static int32_t luaBitLeftShift(lua_State* L);
		static int32_t luaBitRightShift(lua_State* L);
		static int32_t luaBitUNot(lua_State* L);
		static int32_t luaBitUAnd(lua_State* L);
		static int32_t luaBitUOr(lua_State* L);
		static int32_t luaBitUXor(lua_State* L);
		static int32_t luaBitULeftShift(lua_State* L);
		static int32_t luaBitURightShift(lua_State* L);

		lua_State* m_luaState;
		std::string m_lastLuaError;

	private:
		void executeTimerEvent(uint32_t eventIndex);

		static ScriptEnviroment m_scriptEnv[21];
		static int32_t m_scriptEnvIndex;

		int32_t m_runningEventId;
		uint32_t m_lastEventTimerId;
		std::string m_loadingFile, m_interfaceName;

		//script file cache
		typedef std::map<int32_t , std::string> ScriptsCache;
		ScriptsCache m_cacheFiles;

		//events information
		struct LuaTimerEventDesc
		{
			int32_t scriptId, function;
			std::list<int32_t> parameters;
		};

		typedef std::map<uint32_t , LuaTimerEventDesc > LuaTimerEvents;
		LuaTimerEvents m_timerEvents;
};
#endif
