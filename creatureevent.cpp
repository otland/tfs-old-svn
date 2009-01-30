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

#include "creatureevent.h"
#include "tools.h"
#include "player.h"
#ifdef __DEBUG_LUASCRIPTS__
#include <sstream>
#endif

CreatureEvents::CreatureEvents() :
m_scriptInterface("CreatureScript Interface")
{
	m_scriptInterface.initState();
}

CreatureEvents::~CreatureEvents()
{
	CreatureEventList::iterator it;
	for(it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
		delete it->second;
}

void CreatureEvents::clear()
{
	//clear creature events
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
		it->second->clearEvent();

	//clear lua state
	m_scriptInterface.reInitState();
}

LuaScriptInterface& CreatureEvents::getScriptInterface()
{
	return m_scriptInterface;
}

std::string CreatureEvents::getScriptBaseName()
{
	return "creaturescripts";
}

Event* CreatureEvents::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "event")
		return new CreatureEvent(&m_scriptInterface);

	return NULL;
}

bool CreatureEvents::registerEvent(Event* event, xmlNodePtr p)
{
	CreatureEvent* creatureEvent = dynamic_cast<CreatureEvent*>(event);
	if(!creatureEvent)
		return false;

	if(creatureEvent->getEventType() == CREATURE_EVENT_NONE)
	{
		std::cout << "[Error - CreatureEvents::registerEvent] Trying to register event without type!." << std::endl;
		return false;
	}

	CreatureEvent* oldEvent = getEventByName(creatureEvent->getName(), false);
	if(oldEvent)
	{
		//if there was an event with the same that is not loaded
		//(happens when realoading), it is reused
		if(oldEvent->isLoaded() == false && oldEvent->getEventType() == creatureEvent->getEventType())
			oldEvent->copyEvent(creatureEvent);

		return false;
	}
	else
	{
		//if not, register it normally
		m_creatureEvents[creatureEvent->getName()] = creatureEvent;
		return true;
	}
}

CreatureEvent* CreatureEvents::getEventByName(const std::string& name, bool forceLoaded /*= true*/)
{
	CreatureEventList::iterator it = m_creatureEvents.find(name);
	if(it != m_creatureEvents.end())
	{
		if(!forceLoaded || it->second->isLoaded())
			return it->second;
	}

	return NULL;
}

uint32_t CreatureEvents::playerLogin(Player* player)
{
	//fire global event if is registered
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
	{
		if(it->second->getEventType() == CREATURE_EVENT_LOGIN)
		{
			if(!it->second->executeOnLogin(player))
				return 0;
		}
	}

	return 1;
}

uint32_t CreatureEvents::playerLogout(Player* player)
{
	//fire global event if is registered
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
	{
		if(it->second->getEventType() == CREATURE_EVENT_LOGOUT)
		{
			if(!it->second->executeOnLogout(player))
				return 0;
		}
	}

	return 1;
}

/////////////////////////////////////

CreatureEvent::CreatureEvent(LuaScriptInterface* _interface) :
Event(_interface)
{
	m_type = CREATURE_EVENT_NONE;
	m_isLoaded = false;
}

bool CreatureEvent::configureEvent(xmlNodePtr p)
{
	std::string str;
	//Name that will be used in monster xml files and lua function to register events to reference this event
	if(readXMLString(p, "name", str))
		m_eventName = str;
	else
	{
		std::cout << "[Error - CreatureEvent::configureEvent] No name for creature event." << std::endl;
		return false;
	}

	if(readXMLString(p, "type", str))
	{
		std::string tmpStr = asLowerCaseString(str);
		if(tmpStr == "login")
			m_type = CREATURE_EVENT_LOGIN;
		else if(tmpStr == "logout")
			m_type = CREATURE_EVENT_LOGOUT;
		else if(tmpStr == "think")
			m_type = CREATURE_EVENT_THINK;
		else if(tmpStr == "advance")
			m_type = CREATURE_EVENT_ADVANCE;
		else if(tmpStr == "look")
			m_type = CREATURE_EVENT_LOOK;
		else if(tmpStr == "statschange")
			m_type = CREATURE_EVENT_STATSCHANGE;
		else if(tmpStr == "attack")
			m_type = CREATURE_EVENT_ATTACK;
		else if(tmpStr == "kill")
			m_type = CREATURE_EVENT_KILL;
		else if(tmpStr == "death")
			m_type = CREATURE_EVENT_DEATH;
		else if(tmpStr == "preparedeath")
			m_type = CREATURE_EVENT_PREPAREDEATH;
		else
		{
			std::cout << "[Error - CreatureEvent::configureEvent] No valid type for creature event." << str << std::endl;
			return false;
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::configureEvent] No type for creature event."  << std::endl;
		return false;
	}

	m_isLoaded = true;
	return true;
}

std::string CreatureEvent::getScriptEventName()
{
	//Depending on the type script event name is different
	switch(m_type)
	{
		case CREATURE_EVENT_LOGIN:
			return "onLogin";
		case CREATURE_EVENT_LOGOUT:
			return "onLogout";
		case CREATURE_EVENT_THINK:
			return "onThink";
		case CREATURE_EVENT_ADVANCE:
			return "onAdvance";
		case CREATURE_EVENT_LOOK:
			return "onLook";
		case CREATURE_EVENT_STATSCHANGE:
			return "onStatsChange";
		case CREATURE_EVENT_ATTACK:
			return "onAttack";
		case CREATURE_EVENT_KILL:
			return "onKill";
		case CREATURE_EVENT_DEATH:
			return "onDeath";
		case CREATURE_EVENT_PREPAREDEATH:
			return "onPrepareDeath";
		case CREATURE_EVENT_NONE:
		default:
			break;
	}

	return "";
}

void CreatureEvent::copyEvent(CreatureEvent* creatureEvent)
{
	m_scriptId = creatureEvent->m_scriptId;
	m_scriptInterface = creatureEvent->m_scriptInterface;
	m_scripted = creatureEvent->m_scripted;
	m_isLoaded = creatureEvent->m_isLoaded;
}

void CreatureEvent::clearEvent()
{
	m_scriptId = 0;
	m_scriptInterface = NULL;
	m_scripted = false;
	m_isLoaded = false;
}

uint32_t CreatureEvent::executeOnLogin(Player* player)
{
	//onLogin(cid)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		char desc[35];
		sprintf(desc, "%s", player->getName().c_str());
		env->setEventDesc(desc);
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);

		int32_t result = m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnLogin] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnLogout(Player* player)
{
	//onLogout(cid)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		char desc[35];
		sprintf(desc, "%s", player->getName().c_str());
		env->setEventDesc(desc);
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);

		int32_t result = m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnLogout] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnThink(Creature* creature, uint32_t interval)
{
	//onThink(cid, interval)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		char desc[35];
		sprintf(desc, "%s", creature->getName().c_str());
		env->setEventDesc(desc);
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, interval);

		int32_t result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnThink] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnAdvance(Player* player, skills_t skill, uint32_t oldLevel, uint32_t newLevel)
{
	//onAdvance(cid, skill, oldlevel, newlevel)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		char desc[35];
		sprintf(desc, "%s", player->getName().c_str());
		env->setEventDesc(desc);
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, (uint32_t)skill);
		lua_pushnumber(L, oldLevel);
		lua_pushnumber(L, newLevel);

		int32_t result = m_scriptInterface->callFunction(4);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnAdvance] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnLook(Player* player, const Position& position, uint8_t stackpos)
{
	//onLook(cid, position)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		char desc[30];
		sprintf(desc, "%s", player->getName().c_str());
		env->setEventDesc(desc);
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		LuaScriptInterface::pushPosition(L, position, stackpos);

		int32_t result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnLook] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnStatsChange(Creature* creature, Creature* attacker, StatsChange_t type, CombatType_t combat, int32_t value)
{
	//onStatsChange(cid, attacker, type, combat, value)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		char desc[35];
		sprintf(desc, "%s", creature->getName().c_str());
		env->setEventDesc(desc);
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t aid = env->addThing(attacker);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, aid);
		lua_pushnumber(L, (uint32_t)type);
		lua_pushnumber(L, (uint32_t)combat);
		lua_pushnumber(L, value);

		int32_t result = m_scriptInterface->callFunction(5);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnStatsChange] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnAttack(Creature* creature, Creature* target)
{
	//onAttack(cid, target)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << creature->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t targetId = env->addThing(target);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, targetId);

		int32_t result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnAttack] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnKill(Creature* creature, Creature* target)
{
	//onKill(cid, target)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << creature->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t targetId = env->addThing(target);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, targetId);

		int32_t result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnKill] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnDeath(Creature* creature, Item* corpse, Creature* lastHitKiller, Creature* mostDamageKiller)
{
	//onDeath(cid, corpse, lastHitKiller, mostDamageKiller)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		char desc[35];
		sprintf(desc, "%s", creature->getName().c_str());
		env->setEventDesc(desc);
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t corpseId = env->addThing(corpse);
		uint32_t lastKid = env->addThing(lastHitKiller);
		uint32_t mostKid = env->addThing(mostDamageKiller);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, corpseId);
		lua_pushnumber(L, lastKid);
		lua_pushnumber(L, mostKid);

		int32_t result = m_scriptInterface->callFunction(4);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnDeath] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnPrepareDeath(Creature* creature, Creature* lastHitKiller, Creature* mostDamageKiller)
{
	//onPrepareDeath(cid, lastHitKiller, mostDamageKiller)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		char desc[35];
		sprintf(desc, "%s", creature->getName().c_str());
		env->setEventDesc(desc);
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t lastKid = env->addThing(lastHitKiller);
		uint32_t mostKid = env->addThing(mostDamageKiller);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, lastKid);
		lua_pushnumber(L, mostKid);

		int32_t result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeOnPrepareDeath] Call stack overflow." << std::endl;
		return 0;
	}
}
