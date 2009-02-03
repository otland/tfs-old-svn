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

#ifndef __OTSERV_NPC_H__
#define __OTSERV_NPC_H__

#include "creature.h"
#include "luascript.h"
#include "templates.h"

class Npc;
class Player;
class NpcResponse;
struct NpcState;

typedef std::list<Npc*> NpcList;

class Npcs
{
	public:
		Npcs() {}
		virtual ~Npcs() {}

		void reload();
};

class NpcScriptInterface : public LuaScriptInterface
{
	public:
		NpcScriptInterface();
		virtual ~NpcScriptInterface();

		bool loadNpcLib(std::string file);

		static void pushState(lua_State *L, NpcState* state);
		static void popState(lua_State *L, NpcState* &state);

	protected:
		virtual void registerFunctions();

		static int32_t luaActionSay(lua_State* L);
		static int32_t luaActionMove(lua_State* L);
		static int32_t luaActionMoveTo(lua_State* L);
		static int32_t luaActionTurn(lua_State* L);
		static int32_t luaActionFollow(lua_State* L);
		static int32_t luaCreatureGetName(lua_State* L);
		static int32_t luaCreatureGetName2(lua_State* L);
		static int32_t luaCreatureGetPos(lua_State* L);
		static int32_t luaSelfGetPos(lua_State* L);
		static int32_t luagetDistanceTo(lua_State* L);
		static int32_t luaSetNpcFocus(lua_State* L);
		static int32_t luaGetNpcCid(lua_State* L);
		static int32_t luaGetNpcPos(lua_State* L);
		static int32_t luaGetNpcState(lua_State* L);
		static int32_t luaSetNpcState(lua_State* L);
		static int32_t luaGetNpcName(lua_State* L);
		static int32_t luaGetNpcParameter(lua_State* L);
		static int32_t luaOpenShopWindow(lua_State* L);
		static int32_t luaCloseShopWindow(lua_State* L);

	private:
		virtual bool initState();
		virtual bool closeState();

		bool m_libLoaded;
};

class NpcEventsHandler
{
	public:
		NpcEventsHandler(Npc* npc);
		virtual ~NpcEventsHandler();

		virtual void onCreatureAppear(const Creature* creature) {}
		virtual void onCreatureDisappear(const Creature* creature) {}
		virtual void onCreatureMove(const Creature* creature, const Position& oldPos, const Position& newPos) {}
		virtual void onCreatureSay(const Creature* creature, SpeakClasses, const std::string& text, Position* pos = NULL) {}
		virtual void onPlayerTrade(const Player* player, int32_t callback, uint16_t itemid,
			uint8_t count, uint8_t amount, bool ignoreCap, bool inBackpacks) {}
		virtual void onPlayerCloseChannel(const Player* player) {}
		virtual void onPlayerEndTrade(const Player* player) {}
		virtual void onThink() {}

		bool isLoaded();

	protected:
		Npc* m_npc;
		bool m_loaded;
};

class NpcScript : public NpcEventsHandler
{
	public:
		NpcScript(std::string file, Npc* npc);
		virtual ~NpcScript();

		virtual void onCreatureAppear(const Creature* creature);
		virtual void onCreatureDisappear(const Creature* creature);
		virtual void onCreatureMove(const Creature* creature, const Position& oldPos, const Position& newPos);
		virtual void onCreatureSay(const Creature* creature, SpeakClasses, const std::string& text, Position* pos = NULL);
		virtual void onPlayerTrade(const Player* player, int32_t callback, uint16_t itemid,
			uint8_t count, uint8_t amount, bool ignoreCap, bool inBackpacks);
		virtual void onPlayerCloseChannel(const Player* player);
		virtual void onPlayerEndTrade(const Player* player);
		virtual void onThink();

	private:
		NpcScriptInterface* m_scriptInterface;

		int32_t m_onCreatureAppear, m_onCreatureDisappear, m_onCreatureMove, m_onCreatureSay;
		int32_t m_onPlayerCloseChannel, m_onPlayerEndTrade, m_onThink;
};

enum InteractType_t
{
	INTERACT_TEXT,
	INTERACT_EVENT
};

enum NpcEvent_t
{
	EVENT_NONE,
	EVENT_BUSY,
	EVENT_THINK,
	EVENT_IDLE,
	EVENT_PLAYER_ENTER,
	EVENT_PLAYER_MOVE,
	EVENT_PLAYER_LEAVE,
	EVENT_PLAYER_SHOPSELL,
	EVENT_PLAYER_SHOPBUY,
	EVENT_PLAYER_SHOPCLOSE

	/*
	EVENT_CREATURE_ENTER,
	EVENT_CREATURE_MOVE,
	EVENT_CREATURE_LEAVE,
	*/
};

enum ResponseType_t
{
	RESPONSE_DEFAULT,
	RESPONSE_SCRIPT,
};

enum RespondParam_t
{
	RESPOND_DEFAULT = 0,
	RESPOND_MALE = 1 << 0,
	RESPOND_FEMALE = 1 << 1,
	RESPOND_PZBLOCK = 1 << 2,
	RESPOND_LOWMONEY = 1 << 3,
	RESPOND_NOAMOUNT = 1 << 4,
	RESPOND_LOWAMOUNT = 1 << 5,
	RESPOND_PREMIUM = 1 << 6,
	RESPOND_DRUID = 1 << 7,
	RESPOND_KNIGHT = 1 << 8,
	RESPOND_PALADIN = 1 << 9,
	RESPOND_SORCERER = 1 << 10,
	RESPOND_LOWLEVEL = 1 << 11
};

enum ReponseActionParam_t
{
	ACTION_NONE,
	ACTION_SETTOPIC,
	ACTION_SETLEVEL,
	ACTION_SETPRICE,
	ACTION_SETBUYPRICE,
	ACTION_SETSELLPRICE,
	ACTION_TAKEMONEY,
	ACTION_GIVEMONEY,
	ACTION_SELLITEM,
	ACTION_BUYITEM,
	ACTION_GIVEITEM,
	ACTION_TAKEITEM,
	ACTION_SETAMOUNT,
	ACTION_SETITEM,
	ACTION_SETSUBTYPE,
	ACTION_SETEFFECT,
	ACTION_SETSPELL,
	ACTION_SETLISTNAME,
	ACTION_SETLISTPNAME,
	ACTION_TEACHSPELL,
	ACTION_UNTEACHSPELL,
	ACTION_SETSTORAGE,
	ACTION_SETTELEPORT,
	ACTION_SCRIPT,
	ACTION_SCRIPTPARAM,
	ACTION_ADDQUEUE,
	ACTION_SETIDLE
};

enum ShopEvent_t
{
	SHOPEVENT_SELL,
	SHOPEVENT_BUY,
	SHOPEVENT_CLOSE
};

enum StorageComparision_t
{
	STORAGE_LESS,
	STORAGE_LESSOREQUAL,
	STORAGE_EQUAL,
	STORAGE_GREATEROREQUAL,
	STORAGE_GREATER
};

struct ResponseAction
{
	public:
		ResponseAction()
		{
			actionType = ACTION_NONE;
			key = intValue = 0;
			strValue = "";
			pos = Position();
		}

		ReponseActionParam_t actionType;
		int32_t key, intValue;
		std::string strValue;
		Position pos;
};

struct ListItem
{
	ListItem()
	{
		itemId = 0;
		subType = 1;
		sellPrice = buyPrice = -1;
		keywords = name = pluralName = "";
	}

	int32_t sellPrice, buyPrice, itemId, subType;
	std::string keywords, name, pluralName;
};

struct ScriptVars
{
	ScriptVars()
	{
		n1 = n2 = n3 = -1;
		b1 = b2 = b3 = false;
		s1 = s2 = s3 = "";
	}

	int32_t n1, n2, n3;
	bool b1, b2, b3;
	std::string s1, s2, s3;
};

typedef std::list<ResponseAction> ActionList;
typedef std::map<std::string, int32_t> ResponseScriptMap;
typedef std::list<NpcResponse*> ResponseList;

class NpcResponse
{
	public:
		struct ResponseProperties
		{
			ResponseProperties()
			{
				topic = amount = focusStatus = -1;
				output = "";
				interactType = INTERACT_TEXT;
				responseType = RESPONSE_DEFAULT;
				params = 0;
				storageId = storageValue = -1;
				storageComp = STORAGE_EQUAL;
				knowSpell = "";
				publicize = true;
			}

			int32_t topic, amount, focusStatus;
			std::list<std::string> inputList;
			std::string output;
			InteractType_t interactType;
			ResponseType_t responseType;
			uint32_t params;
			int32_t storageId, storageValue;
			StorageComparision_t storageComp;
			std::string knowSpell;
			ActionList actionList;
			std::list<ListItem> itemList;
			bool publicize;
		};

		NpcResponse(const ResponseProperties& _prop,
			ResponseList _subResponseList,
			ScriptVars _scriptVars)
		{
			prop = _prop;
			subResponseList = _subResponseList;
			scriptVars = _scriptVars;
		}

		NpcResponse(NpcResponse& rhs)
		{
			prop = rhs.prop;
			scriptVars = rhs.scriptVars;
			for(ResponseList::iterator it = rhs.subResponseList.begin(); it != rhs.subResponseList.end(); ++it)
			{
				NpcResponse* response = new NpcResponse(*(*it));
				subResponseList.push_back(response);
			}
		}

		virtual ~NpcResponse()
		{
			for(ResponseList::iterator it = subResponseList.begin(); it != subResponseList.end(); ++it)
				delete *it;

			subResponseList.clear();
		}

		uint32_t getParams() const {return prop.params;}
		std::string getInputText() const {return (prop.inputList.empty() ? "" : *prop.inputList.begin());}
		int32_t getTopic() const {return prop.topic;}
		int32_t getFocusState() const {return prop.focusStatus;}
		int32_t getStorageId() const {return prop.storageId;}
		int32_t getStorageValue() const {return prop.storageValue;}
		ResponseType_t getResponseType() const {return prop.responseType;}
		InteractType_t getInteractType() const {return prop.interactType;}
		StorageComparision_t getStorageComp() const {return prop.storageComp;}
		const std::string& getKnowSpell() const {return prop.knowSpell;}
		const std::string& getText() const {return prop.output;}
		int32_t getAmount() const {return prop.amount;}
		void setAmount(int32_t _amount) {prop.amount = _amount;}
		bool publicize() const {return prop.publicize;}

		std::string formatResponseString(Creature* creature) const;
		void addAction(ResponseAction action) {prop.actionList.push_back(action);}
		const std::list<std::string>& getInputList() const {return prop.inputList;}

		void setResponseList(ResponseList _list) {subResponseList.insert(subResponseList.end(),_list.begin(),_list.end());}
		const ResponseList& getResponseList() const {return subResponseList;}

		ActionList::const_iterator getFirstAction() const {return prop.actionList.begin();}
		ActionList::const_iterator getEndAction() const {return prop.actionList.end();}

		ResponseProperties prop;
		ResponseList subResponseList;
		ScriptVars scriptVars;
};

struct NpcState
{
	int32_t topic;
	bool isIdle, isQueued;
	int32_t price, sellPrice, buyPrice, amount, itemId, subType;
	bool ignoreCap, inBackpacks;
	std::string spellName, listName, listPluralName;
	int32_t level;
	uint64_t prevInteraction;
	std::string respondToText;
	uint32_t respondToCreature;
	std::string prevRespondToText;
	const NpcResponse* lastResponse;

	//script variables
	ScriptVars scriptVars;

	//Do not forget to update pushState/popState if you add more variables
};

struct Voice
{
	std::string text;
	uint32_t interval, margin;
	bool yell;
};

#define MAX_RAND_RANGE 10000000

class Npc : public Creature
{
	public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
		static uint32_t npcCount;
#endif
		virtual ~Npc();

		virtual Npc* getNpc() {return this;}
		virtual const Npc* getNpc() const {return this;}

		virtual bool isPushable() const {return false;}

		virtual uint32_t idRange() {return 0x80000000;}
		static AutoList<Npc> listNpc;
		void removeList() {listNpc.removeList(getID());}
		void addList() {listNpc.addList(this);}

		static Npc* createNpc(const std::string& name);

		virtual bool canSee(const Position& pos) const;

		bool load();
		void reload();

		virtual const std::string& getName() const {return name;}
		virtual const std::string& getNameDescription() const {return nameDescription;}

		void doSay(std::string msg, Player* focus = NULL, bool publicize = false, bool yell = false);
		void doMove(Direction dir);
		void doTurn(Direction dir);
		void doMoveTo(Position pos);
		bool isLoaded() {return loaded;}

		void onPlayerTrade(Player* player, ShopEvent_t type, int32_t callback, uint16_t itemId, uint8_t count,
			uint8_t amount, bool ignoreCap = false, bool inBackpacks = false);
		void onPlayerEndTrade(Player* player, int32_t buyCallback,
			int32_t sellCallback);

		void onPlayerCloseChannel(const Player* player);
		void setCreatureFocus(Creature* creature);

		NpcScriptInterface* getScriptInterface();

	protected:
		Npc(const std::string& _name);
		bool loaded;

		virtual void onAddTileItem(const Tile* tile, const Position& pos, const Item* item);
		virtual void onUpdateTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
			const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType);
		virtual void onRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
			const ItemType& iType, const Item* item);
		virtual void onUpdateTile(const Tile* tile, const Position& pos);

		virtual void onCreatureAppear(const Creature* creature, bool isLogin);
		virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
		virtual void onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
			const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport);

		virtual void onCreatureTurn(const Creature* creature, uint32_t stackpos);
		virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text, Position* pos = NULL);
		virtual void onCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit);
		virtual void onThink(uint32_t interval);
		virtual std::string getDescription(int32_t lookDistance) const {return nameDescription + ".";}

		bool isImmune(CombatType_t type) const {return true;}
		bool isImmune(ConditionType_t type) const {return true;}
		virtual bool isAttackable() const {return attackable;}
		virtual bool getNextStep(Direction& dir);

		bool canWalkTo(const Position& fromPos, Direction dir);
		bool getRandomStep(Direction& dir);

		void reset();
		bool loadFromXml(const std::string& name);

		const NpcResponse* getResponse(const ResponseList& list, const Player* player,
			NpcState* npcState, const std::string& text, bool exactMatch = false);
		const NpcResponse* getResponse(const Player* player, NpcState* npcState, const std::string& text);
		const NpcResponse* getResponse(const Player* player, NpcEvent_t eventType);
		const NpcResponse* getResponse(const Player* player, NpcState* npcState, NpcEvent_t eventType);
		std::string getEventResponseName(NpcEvent_t eventType);

		uint32_t getMatchCount(NpcResponse* response, StringVec wordList,
			bool exactMatch, int32_t& matchAllCount, int32_t& totalKeywordCount);

		void executeResponse(Player* player, NpcState* npcState, const NpcResponse* response);

		std::string formatResponse(Creature* creature, const NpcState* npcState, const NpcResponse* response) const;

		void onPlayerEnter(Player* player, NpcState* state);
		void onPlayerLeave(Player* player, NpcState* state);

		typedef std::map<std::string, std::string> ParametersMap;
		ParametersMap m_parameters;

		uint32_t loadParams(xmlNodePtr node);
		ResponseList loadInteraction(xmlNodePtr node);

		NpcState* getState(const Player* player, bool makeNew = true);

		void addShopPlayer(Player* player);
		void removeShopPlayer(const Player* player);
		void closeAllShopWindows();
		uint32_t getListItemPrice(uint16_t itemId, ShopEvent_t type);

		uint32_t walkTicks;
		std::string name, nameDescription, m_filename;
		int32_t talkRadius, idleTime, idleInterval, focusCreature;
		bool floorChange, attackable, isIdle, hasBusyReply, hasScriptedFocus, defaultPublic;
		int64_t lastVoice;

		typedef std::list<Player*> ShopPlayerList;
		ShopPlayerList shopPlayerList;

		typedef std::map<std::string, std::list<ListItem> > ItemListMap;
		ItemListMap itemListMap;

		ResponseScriptMap responseScriptMap;
		ResponseList responseList;

		typedef std::list<NpcState*> StateList;
		StateList stateList;

		typedef std::list<uint32_t> QueueList;
		QueueList queueList;

		typedef std::list<Voice> VoiceList;
		VoiceList voiceList;

		NpcEventsHandler* m_npcEventHandler;
		static NpcScriptInterface* m_scriptInterface;

		friend class Npcs;
		friend class NpcScriptInterface;
};

#endif
