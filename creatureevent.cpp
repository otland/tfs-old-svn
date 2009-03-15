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
		std::cout << "[Error - CreatureEvents::registerEvent] Trying to register event without type!" << std::endl;
		return false;
	}

	if(CreatureEvent* oldEvent = getEventByName(creatureEvent->getName(), false))
	{
		//if there was an event with the same that is not loaded (happens when realoading), it is reused
		if(!oldEvent->isLoaded() && oldEvent->getEventType() == creatureEvent->getEventType())
			oldEvent->copyEvent(creatureEvent);

		return false;
	}

	//if not, register it normally
	m_creatureEvents[creatureEvent->getName()] = creatureEvent;
	return true;
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
		if(it->second->getEventType() == CREATURE_EVENT_LOGIN && !it->second->executeLogin(player))
			return 0;
	}

	return 1;
}

uint32_t CreatureEvents::playerLogout(Player* player)
{
	//fire global event if is registered
	for(CreatureEventList::iterator it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it)
	{
		if(it->second->getEventType() == CREATURE_EVENT_LOGOUT && !it->second->executeLogout(player))
			return 0;
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
	if(!readXMLString(p, "name", str))
	{
		std::cout << "[Error - CreatureEvent::configureEvent] No name for creature event." << std::endl;
		return false;
	}

	m_eventName = str;
	if(!readXMLString(p, "type", str))
	{
		std::cout << "[Error - CreatureEvent::configureEvent] No type for creature event."  << std::endl;
		return false;
	}

	std::string tmpStr = asLowerCaseString(str);
	if(tmpStr == "login")
		m_type = CREATURE_EVENT_LOGIN;
	else if(tmpStr == "logout")
		m_type = CREATURE_EVENT_LOGOUT;
	else if(tmpStr == "joinchannel")
		m_type = CREATURE_EVENT_CHANNEL_JOIN;
	else if(tmpStr == "leavechannel")
		m_type = CREATURE_EVENT_CHANNEL_LEAVE;
	else if(tmpStr == "advance")
		m_type = CREATURE_EVENT_ADVANCE;
	else if(tmpStr == "sendmail")
		m_type = CREATURE_EVENT_MAIL_SEND;
	else if(tmpStr == "receivemail")
		m_type = CREATURE_EVENT_MAIL_RECEIVE;
	else if(tmpStr == "look")
		m_type = CREATURE_EVENT_LOOK;
	else if(tmpStr == "think")
		m_type = CREATURE_EVENT_THINK;
	else if(tmpStr == "statschange")
		m_type = CREATURE_EVENT_STATSCHANGE;
	else if(tmpStr == "areacombat")
		m_type = CREATURE_EVENT_COMBAT_AREA;
	else if(tmpStr == "combat")
		m_type = CREATURE_EVENT_COMBAT;
	else if(tmpStr == "attack")
		m_type = CREATURE_EVENT_ATTACK;
	else if(tmpStr == "cast")
		m_type = CREATURE_EVENT_CAST;
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

	m_isLoaded = true;
	return true;
}

std::string CreatureEvent::getScriptEventName() const
{
	switch(m_type)
	{
		case CREATURE_EVENT_LOGIN:
			return "onLogin";
		case CREATURE_EVENT_LOGOUT:
			return "onLogout";
		case CREATURE_EVENT_CHANNEL_JOIN:
			return "onJoinChannel";
		case CREATURE_EVENT_CHANNEL_LEAVE:
			return "onLeaveChannel";
		case CREATURE_EVENT_THINK:
			return "onThink";
		case CREATURE_EVENT_ADVANCE:
			return "onAdvance";
		case CREATURE_EVENT_LOOK:
			return "onLook";
		case CREATURE_EVENT_MAIL_SEND:
			return "onSendMail";
		case CREATURE_EVENT_MAIL_RECEIVE:
			return "onReceiveMail";
		case CREATURE_EVENT_STATSCHANGE:
			return "onStatsChange";
		case CREATURE_EVENT_COMBAT_AREA:
			return "onAreaCombat";
		case CREATURE_EVENT_COMBAT:
			return "onCombat";
		case CREATURE_EVENT_ATTACK:
			return "onAttack";
		case CREATURE_EVENT_CAST:
			return "onCast";
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

std::string CreatureEvent::getScriptEventParams() const
{
	switch(m_type)
	{
		case CREATURE_EVENT_LOGIN:
		case CREATURE_EVENT_LOGOUT:
			return "cid";
		case CREATURE_EVENT_CHANNEL_JOIN:
		case CREATURE_EVENT_CHANNEL_LEAVE:
			return "cid, channel, users";
		case CREATURE_EVENT_ADVANCE:
			return "cid, skill, oldLevel, newLevel";
		case CREATURE_EVENT_LOOK:
			return "cid, position";
		case CREATURE_EVENT_MAIL_SEND:
			return "cid, receiver, item, openBox";
		case CREATURE_EVENT_MAIL_RECEIVE:
			return "cid, sender, item, openBox";
		case CREATURE_EVENT_THINK:
			return "cid, interval";
		case CREATURE_EVENT_STATSCHANGE:
			return "cid, attacker, type, combat, value";
		case CREATURE_EVENT_COMBAT_AREA:
			return "cid, tileItem, tilePosition, isAggressive";
		case CREATURE_EVENT_COMBAT:
		case CREATURE_EVENT_ATTACK:
		case CREATURE_EVENT_CAST:
		case CREATURE_EVENT_KILL:
			return "cid, target";
		case CREATURE_EVENT_DEATH:
			return "cid, corpse, lastHitKiller, mostDamageKiller";
		case CREATURE_EVENT_PREPAREDEATH:
			return "cid, lastHitKiller, mostDamageKiller";
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
	m_scripted = EVENT_SCRIPT_FALSE;
	m_isLoaded = false;
}

uint32_t CreatureEvent::executeLogin(Player* player)
{
	//onLogin(cid)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(player) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));

			int32_t result = m_scriptInterface->callFunction(1);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeLogin] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeLogout(Player* player)
{
	//onLogout(cid)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(player) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));

			int32_t result = m_scriptInterface->callFunction(1);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeLogout] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeChannelJoin(Player* player, uint16_t channelId, UsersList usersList)
{
	//onJoinChannel(cid, channel, users)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(player) << std::endl;
			scriptstream << "channel = " << channelId << std::endl;
			scriptstream << "users = {}" << std::endl;
			for(UsersList::iterator it = usersList.begin(); it != usersList.end(); ++it)
				scriptstream << "table.insert(users, " << (*it) << ")" << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, channelId);
			UsersList::iterator it = usersList.begin();

			lua_newtable(L);
			for(int32_t i = 1; it != usersList.end(); ++it, ++i)
			{
				lua_pushnumber(L, i);
				lua_pushnumber(L, (*it));
				lua_settable(L, -3);
			}

			int32_t result = m_scriptInterface->callFunction(3);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeChannelJoin] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeChannelLeave(Player* player, uint16_t channelId, UsersList usersList)
{
	//onLeaveChannel(cid, channel, users)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(player) << std::endl;
			scriptstream << "channel = " << channelId << std::endl;
			scriptstream << "users = {}" << std::endl;
			for(UsersList::iterator it = usersList.begin(); it != usersList.end(); ++it)
				scriptstream << "table.insert(users, " << (*it) << ")" << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, channelId);
			UsersList::iterator it = usersList.begin();

			lua_newtable(L);
			for(int32_t i = 1; it != usersList.end(); ++it, ++i)
			{
				lua_pushnumber(L, i);
				lua_pushnumber(L, (*it));
				lua_settable(L, -3);
			}

			int32_t result = m_scriptInterface->callFunction(3);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeChannelLeave] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeAdvance(Player* player, skills_t skill, uint32_t oldLevel, uint32_t newLevel)
{
	//onAdvance(cid, skill, oldLevel, newLevel)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(player) << std::endl;
			scriptstream << "skill = " << skill << std::endl;
			scriptstream << "oldLevel = " << oldLevel << std::endl;
			scriptstream << "newLevel = " << newLevel << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, (uint32_t)skill);
			lua_pushnumber(L, oldLevel);
			lua_pushnumber(L, newLevel);

			int32_t result = m_scriptInterface->callFunction(4);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeAdvance] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeMailSend(Player* player, Player* receiver, Item* item, bool openBox)
{
	//onSendMail(cid, receiver, item, openBox)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(player) << std::endl;
			scriptstream << "receiver = " << env->addThing(receiver) << std::endl;
			env->streamThing(scriptstream, "item", item, env->addThing(item));
			scriptstream << "openBox = " << (openBox ? LUA_TRUE : LUA_FALSE) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[30];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, env->addThing(receiver));
			LuaScriptInterface::pushThing(L, item, env->addThing(item));
			lua_pushnumber(L, openBox ? LUA_TRUE : LUA_FALSE);

			int32_t result = m_scriptInterface->callFunction(4);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeMailSend] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeMailReceive(Player* player, Player* sender, Item* item, bool openBox)
{
	//onReceiveMail(cid, sender, item, openBox)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(player) << std::endl;
			scriptstream << "sender = " << env->addThing(sender) << std::endl;
			env->streamThing(scriptstream, "item", item, env->addThing(item));
			scriptstream << "openBox = " << (openBox ? LUA_TRUE : LUA_FALSE) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[30];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			lua_pushnumber(L, env->addThing(sender));
			LuaScriptInterface::pushThing(L, item, env->addThing(item));
			lua_pushnumber(L, openBox ? LUA_TRUE : LUA_FALSE);

			int32_t result = m_scriptInterface->callFunction(4);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeMailReceive] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeLook(Player* player, const Position& position, uint8_t stackpos)
{
	//onLook(cid, position)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(player->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(player) << std::endl;
			env->streamPosition(scriptstream, "position", position, stackpos);

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[30];
			sprintf(desc, "%s", player->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(player->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(player));
			LuaScriptInterface::pushPosition(L, position, stackpos);

			int32_t result = m_scriptInterface->callFunction(2);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeLook] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeThink(Creature* creature, uint32_t interval)
{
	//onThink(cid, interval)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(creature) << std::endl;
			scriptstream << "interval = " << interval << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, interval);

			int32_t result = m_scriptInterface->callFunction(2);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeThink] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeStatsChange(Creature* creature, Creature* attacker, StatsChange_t type, CombatType_t combat, int32_t value)
{
	//onStatsChange(cid, attacker, type, combat, value)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(creature) << std::endl;
			scriptstream << "attacker = " << env->addThing(attacker) << std::endl;
			scriptstream << "type = " << (uint32_t)type << std::endl;
			scriptstream << "combat = " << (uint32_t)combat << std::endl;
			scriptstream << "value = " << value << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(attacker));
			lua_pushnumber(L, (uint32_t)type);
			lua_pushnumber(L, (uint32_t)combat);
			lua_pushnumber(L, value);

			int32_t result = m_scriptInterface->callFunction(5);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeStatsChange] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeCombatArea(Creature* creature, Tile* tile, bool isAggressive)
{
	//onAreaCombat(cid, tileItem, tilePosition, isAggressive)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(creature) << std::endl;
			env->streamThing(scriptstream, "tileItem", tile, env->addThing(tile));
			env->streamPosition(scriptstream, "tilePosition", tile->getPosition(), 0);
			scriptstream << "isAggressive = " << (isAggressive ? LUA_TRUE : LUA_FALSE) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEventDesc(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			LuaScriptInterface::pushThing(L, tile, env->addThing(tile));
			LuaScriptInterface::pushPosition(L, tile->getPosition(), 0);
			lua_pushnumber(L, isAggressive ? LUA_TRUE : LUA_FALSE);

			int32_t result = m_scriptInterface->callFunction(4);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeAreaCombat] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeCombat(Creature* creature, Creature* target)
{
	//onCombat(cid, target)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(creature) << std::endl;
			scriptstream << "target = " << env->addThing(target) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEventDesc(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			int32_t result = m_scriptInterface->callFunction(2);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeCombat] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeAttack(Creature* creature, Creature* target)
{
	//onAttack(cid, target)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(creature) << std::endl;
			scriptstream << "target = " << env->addThing(target) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEventDesc(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			int32_t result = m_scriptInterface->callFunction(2);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeAttack] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeCast(Creature* creature, Creature* target/* = NULL*/)
{
	//onCast(cid[, target])
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(creature) << std::endl;
			scriptstream << "target = ";
			if(target)
				scriptstream << env->addThing(target);
			else
				scriptstream << "nil";

			scriptstream << std::endl << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEventDesc(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			int32_t result = m_scriptInterface->callFunction(2);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeCast] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeKill(Creature* creature, Creature* target)
{
	//onKill(cid, target)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(creature) << std::endl;
			scriptstream << "target = " << env->addThing(target) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			std::stringstream desc;
			desc << creature->getName();
			env->setEventDesc(desc.str());
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(target));

			int32_t result = m_scriptInterface->callFunction(2);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeKill] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeDeath(Creature* creature, Item* corpse, Creature* lastHitKiller, Creature* mostDamageKiller)
{
	//onDeath(cid, corpse, lastHitKiller, mostDamageKiller)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(creature) << std::endl;
			env->streamThing(scriptstream, "corpse", corpse, env->addThing(corpse));
			scriptstream << "lastHitKiller = " << env->addThing(lastHitKiller) << std::endl;
			scriptstream << "mostDamageKiller = " << env->addThing(mostDamageKiller) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			LuaScriptInterface::pushThing(L, corpse, env->addThing(corpse));
			lua_pushnumber(L, env->addThing(lastHitKiller));
			lua_pushnumber(L, env->addThing(mostDamageKiller));

			int32_t result = m_scriptInterface->callFunction(4);
			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}

	}
	else
	{
		std::cout << "[Error - CreatureEvent::executeDeath] Call stack overflow." << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executePrepareDeath(Creature* creature, Creature* lastHitKiller, Creature* mostDamageKiller)
{
	//onPrepareDeath(cid, lastHitKiller, mostDamageKiller)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			env->setRealPos(creature->getPosition());

			std::stringstream scriptstream;
			scriptstream << "cid = " << env->addThing(creature) << std::endl;
			scriptstream << "lastHitKiller = " << env->addThing(lastHitKiller) << std::endl;
			scriptstream << "mostDamageKiller = " << env->addThing(mostDamageKiller) << std::endl;

			scriptstream << m_scriptData;
			int32_t result = LUA_NO_ERROR;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getField(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return (result == LUA_TRUE);
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[35];
			sprintf(desc, "%s", creature->getName().c_str());
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			env->setRealPos(creature->getPosition());

			lua_State* L = m_scriptInterface->getLuaState();
			m_scriptInterface->pushFunction(m_scriptId);

			lua_pushnumber(L, env->addThing(creature));
			lua_pushnumber(L, env->addThing(lastHitKiller));
			lua_pushnumber(L, env->addThing(mostDamageKiller));

			int32_t result = m_scriptInterface->callFunction(3);
			m_scriptInterface->releaseScriptEnv();

			return (result == LUA_TRUE);
		}
	}
	else
	{
		std::cout << "[Error - CreatureEvent::executePrepareDeath] Call stack overflow." << std::endl;
		return 0;
	}
}
