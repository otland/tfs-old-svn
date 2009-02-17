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

#include "enums.h"
#include "luascript.h"
#include "baseevents.h"

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
	CREATURE_EVENT_THINK,
	CREATURE_EVENT_STATSCHANGE,
	CREATURE_EVENT_ATTACK,
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

typedef std::list<uint32_t> UsersList;

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
		uint32_t executeOnLogin(Player* player);
		uint32_t executeOnLogout(Player* player);
		uint32_t executeOnChannelJoin(Player* player, uint16_t channelId, UsersList usersList);
		uint32_t executeOnChannelLeave(Player* player, uint16_t channelId, UsersList usersList);
		uint32_t executeOnAdvance(Player* player, skills_t skill, uint32_t oldLevel, uint32_t newLevel);
		uint32_t executeOnLook(Player* player, const Position& position, uint8_t stackpos);
		uint32_t executeOnMailSend(Player* player, Player* receiver, Item* item, bool openBox);
		uint32_t executeOnMailReceive(Player* player, Player* sender, Item* item, bool openBox);
		uint32_t executeOnThink(Creature* creature, uint32_t interval);
		uint32_t executeOnStatsChange(Creature* creature, Creature* attacker, StatsChange_t type, CombatType_t combat, int32_t value);
		uint32_t executeOnAttack(Creature* creature, Creature* target);
		uint32_t executeOnKill(Creature* creature, Creature* target);
		uint32_t executeOnDeath(Creature* creature, Item* corpse, Creature* lastHitKiller, Creature* mostDamageKiller);
		uint32_t executeOnPrepareDeath(Creature* creature, Creature* lastHitKiller, Creature* mostDamageKiller);
		//

	protected:
		virtual std::string getScriptEventName() const;
		virtual std::string getScriptEventParams() const;

		bool m_isLoaded;
		std::string m_eventName;
		CreatureEventType_t m_type;
};

#endif
