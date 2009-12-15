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

#ifndef __MOVEMENT__
#define __MOVEMENT__

#include "baseevents.h"
#include "creature.h"

enum MoveEvent_t
{
	MOVE_EVENT_FIRST = 0,
	MOVE_EVENT_STEP_IN = MOVE_EVENT_FIRST,
	MOVE_EVENT_STEP_OUT = 1,
	MOVE_EVENT_EQUIP = 2,
	MOVE_EVENT_DEEQUIP = 3,
	MOVE_EVENT_ADD_ITEM = 4,
	MOVE_EVENT_REMOVE_ITEM = 5,
	MOVE_EVENT_ADD_ITEM_ITEMTILE = 6,
	MOVE_EVENT_REMOVE_ITEM_ITEMTILE = 7,
	MOVE_EVENT_NONE = 8,
	MOVE_EVENT_LAST = MOVE_EVENT_REMOVE_ITEM_ITEMTILE
};

class MoveEvent;
typedef std::list<MoveEvent*> EventList;

struct MoveEventList
{
	EventList moveEvent[MOVE_EVENT_NONE];
};

class MoveEvents : public BaseEvents
{
	public:
		MoveEvents();
		virtual ~MoveEvents();

		uint32_t onCreatureMove(Creature* actor, Creature* creature, const Tile* fromTile, const Tile* toTile, bool isStepping);
		uint32_t onPlayerEquip(Player* player, Item* item, slots_t slot, bool isCheck);
		uint32_t onPlayerDeEquip(Player* player, Item* item, slots_t slot, bool isRemoval);
		uint32_t onItemMove(Creature* actor, Item* item, Tile* tile, bool isAdd);

		MoveEvent* getEvent(Item* item, MoveEvent_t eventType);
		bool hasEquipEvent(Item* item);
		bool hasTileEvent(Item* item);

		void onRemoveTileItem(const Tile* tile, Item* item);
		void onAddTileItem(const Tile* tile, Item* item);

	protected:
		const Tile* m_lastCacheTile;
		std::vector<Item*> m_lastCacheItemVector;

		virtual std::string getScriptBaseName() const {return "movements";}
		virtual void clear();

		virtual Event* getEvent(const std::string& nodeName);
		virtual bool registerEvent(Event* event, xmlNodePtr p, bool override);

		virtual LuaScriptInterface& getInterface() {return m_interface;}
		LuaScriptInterface m_interface;

		void registerItemID(int32_t itemId, MoveEvent_t eventType);
		void registerActionID(int32_t actionId, MoveEvent_t eventType);
		void registerUniqueID(int32_t uniqueId, MoveEvent_t eventType);

		typedef std::map<int32_t, MoveEventList> MoveListMap;
		MoveListMap m_itemIdMap;
		MoveListMap m_uniqueIdMap;
		MoveListMap m_actionIdMap;

		typedef std::map<Position, MoveEventList> MovePosListMap;
		MovePosListMap m_positionMap;
		void clearMap(MoveListMap& map);

		void addEvent(MoveEvent* moveEvent, int32_t id, MoveListMap& map, bool override);
		MoveEvent* getEvent(Item* item, MoveEvent_t eventType, slots_t slot);

		void addEvent(MoveEvent* moveEvent, Position pos, MovePosListMap& map, bool override);
		MoveEvent* getEvent(const Tile* tile, MoveEvent_t eventType);
};

typedef uint32_t (MoveFunction)(Item* item);
typedef uint32_t (StepFunction)(Creature* creature, Item* item);
typedef uint32_t (EquipFunction)(MoveEvent* moveEvent, Player* player, Item* item, slots_t slot, bool boolean);

class MoveEvent : public Event
{
	public:
		MoveEvent(LuaScriptInterface* _interface);
		MoveEvent(const MoveEvent* copy);
		virtual ~MoveEvent();

		MoveEvent_t getEventType() const;
		void setEventType(MoveEvent_t type);

		virtual bool configureEvent(xmlNodePtr p);
		virtual bool loadFunction(const std::string& functionName);

		uint32_t fireStepEvent(Creature* actor, Creature* creature, Item* item, const Position& pos, const Position& fromPos, const Position& toPos);
		uint32_t fireAddRemItem(Creature* actor, Item* item, Item* tileItem, const Position& pos);
		uint32_t fireEquip(Player* player, Item* item, slots_t slot, bool boolean);

		//scripting
		uint32_t executeStep(Creature* actor, Creature* creature, Item* item, const Position& pos, const Position& fromPos, const Position& toPos);
		uint32_t executeEquip(Player* player, Item* item, slots_t slot);
		uint32_t executeAddRemItem(Creature* actor, Item* item, Item* tileItem, const Position& pos);

		uint32_t getWieldInfo() const {return wieldInfo;}
		uint32_t getSlot() const {return slot;}

		int32_t getReqLevel() const {return reqLevel;}
		int32_t getReqMagLv() const {return reqMagLevel;}
		bool isPremium() const {return premium;}

		const VocationMap& getVocEquipMap() const {return vocEquipMap;}
		const std::string& getVocationString() const {return vocationString;}

	protected:
		virtual std::string getScriptEventName() const;
		virtual std::string getScriptEventParams() const;

		static StepFunction StepInField;
		static MoveFunction AddItemField;
		static EquipFunction EquipItem;
		static EquipFunction DeEquipItem;

		MoveEvent_t m_eventType;
		StepFunction* stepFunction;
		MoveFunction* moveFunction;
		EquipFunction* equipFunction;

		uint32_t wieldInfo;
		uint32_t slot;

		int32_t reqLevel;
		int32_t reqMagLevel;
		bool premium;

		VocationMap vocEquipMap;
		std::string vocationString;
};
#endif
