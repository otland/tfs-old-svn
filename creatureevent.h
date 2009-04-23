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

#ifndef __OTSERV_CREATUREEVENT_H__
#define __OTSERV_CREATUREEVENT_H__
#include "baseevents.h"
#include "enums.h"

#include "tile.h"

enum CreatureEventType_t
{
	CREATURE_EVENT_NONE,
	CREATURE_EVENT_LOGIN,
	CREATURE_EVENT_LOGOUT,
	CREATURE_EVENT_CHANNEL_JOIN,
	CREATURE_EVENT_CHANNEL_LEAVE,
	CREATURE_EVENT_ADVANCE,
	CREATURE_EVENT_LOOK,
	CREATURE_EVENT_MAIL_SEND,
	CREATURE_EVENT_MAIL_RECEIVE,
	CREATURE_EVENT_TEXTEDIT,
	CREATURE_EVENT_THINK,
	CREATURE_EVENT_STATSCHANGE,
	CREATURE_EVENT_COMBAT_AREA,
	CREATURE_EVENT_COMBAT,
	CREATURE_EVENT_ATTACK,
	CREATURE_EVENT_CAST,
	CREATURE_EVENT_KILL,
	CREATURE_EVENT_DEATH,
	CREATURE_EVENT_PREPAREDEATH
};

enum StatsChange_t
{
	STATSCHANGE_HEALTHGAIN,
	STATSCHANGE_HEALTHLOSS,
	STATSCHANGE_MANAGAIN,
	STATSCHANGE_MANALOSS
};

class CreatureEvent;

class CreatureEvents : public BaseEvents
{
	public:
		CreatureEvents();
		virtual ~CreatureEvents();

		// global events
		uint32_t playerLogin(Player* player);
		uint32_t playerLogout(Player* player);

		CreatureEvent* getEventByName(const std::string& name, bool forceLoaded = true);

	protected:
		virtual std::string getScriptBaseName() const {return "creaturescripts";}
		virtual void clear();

		virtual Event* getEvent(const std::string& nodeName);
		virtual bool registerEvent(Event* event, xmlNodePtr p);

		virtual LuaScriptInterface& getScriptInterface() {return m_scriptInterface;}
		LuaScriptInterface m_scriptInterface;

		//creature events
		typedef std::map<std::string, CreatureEvent*> CreatureEventList;
		CreatureEventList m_creatureEvents;
};

typedef std::map<uint32_t, Player*> UsersMap;

class CreatureEvent : public Event
{
	public:
		CreatureEvent(LuaScriptInterface* _interface);
		virtual ~CreatureEvent() {}

		virtual bool configureEvent(xmlNodePtr p);

		bool isLoaded() const {return m_isLoaded;}
		const std::string& getName() const {return m_eventName;}
		CreatureEventType_t getEventType() const {return m_type;}

		void copyEvent(CreatureEvent* creatureEvent);
		void clearEvent();

		//scripting
		uint32_t executeLogin(Player* player);
		uint32_t executeLogout(Player* player);
		uint32_t executeChannelJoin(Player* player, uint16_t channelId, UsersMap usersMap);
		uint32_t executeChannelLeave(Player* player, uint16_t channelId, UsersMap usersMap);
		uint32_t executeAdvance(Player* player, skills_t skill, uint32_t oldLevel, uint32_t newLevel);
		uint32_t executeLook(Player* player, Thing* thing, const Position& position, int16_t stackpos, int32_t lookDistance);
		uint32_t executeMailSend(Player* player, Player* receiver, Item* item, bool openBox);
		uint32_t executeMailReceive(Player* player, Player* sender, Item* item, bool openBox);
		uint32_t executeTextEdit(Player* player, Item *item, std::string newText);
		uint32_t executeThink(Creature* creature, uint32_t interval);
		uint32_t executeStatsChange(Creature* creature, Creature* attacker, StatsChange_t type, CombatType_t combat, int32_t value);
		uint32_t executeCombatArea(Creature* creature, Tile* tile, bool isAggressive);
		uint32_t executeCombat(Creature* creature, Creature* target);
		uint32_t executeAttack(Creature* creature, Creature* target);
		uint32_t executeCast(Creature* creature, Creature* target = NULL);
		uint32_t executeKill(Creature* creature, Creature* target);
		uint32_t executeDeath(Creature* creature, Item* corpse, Creature* lastHitKiller, Creature* mostDamageKiller);
		uint32_t executePrepareDeath(Creature* creature, Creature* lastHitKiller, Creature* mostDamageKiller);
		//

	protected:
		virtual std::string getScriptEventName() const;
		virtual std::string getScriptEventParams() const;

		bool m_isLoaded;
		std::string m_eventName;
		CreatureEventType_t m_type;
};

#endif
