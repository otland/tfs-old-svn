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
#include "otpch.h"
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "globalevent.h"
#include "tools.h"
#include "player.h"

GlobalEvents::GlobalEvents():
	m_scriptInterface("GlobalEvent Interface")
{
	m_scriptInterface.initState();
}

GlobalEvents::~GlobalEvents()
{
	clear();
}

void GlobalEvents::clear()
{
	GlobalEventMap::iterator it;
	for(it = eventsMap.begin(); it != eventsMap.end(); ++it)
		delete it->second;

	eventsMap.clear();
	for(it = serverEventsMap.begin(); it != serverEventsMap.end(); ++it)
		delete it->second;

	serverEventsMap.clear();
	m_scriptInterface.reInitState();
}

Event* GlobalEvents::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "globalevent")
		return new GlobalEvent(&m_scriptInterface);

	return NULL;
}

bool GlobalEvents::registerEvent(Event* event, xmlNodePtr p, bool override)
{
	GlobalEvent* globalEvent = dynamic_cast<GlobalEvent*>(event);
	if(!globalEvent)
		return false;

	GlobalEventMap* map = &eventsMap;
	if(globalEvent->getEventType() != SERVER_EVENT_NONE)
		map = &serverEventsMap;

	GlobalEventMap::iterator it = map->find(globalEvent->getName());
	if(it == map->end())
	{
		map->insert(std::make_pair(globalEvent->getName(), globalEvent));
		return true;
	}

	if(override)
	{
		delete it->second;
		it->second = globalEvent;
		return true;
	}

	std::cout << "[Warning - GlobalEvents::configureEvent] Duplicate registered globalevent with name: " << globalEvent->getName() << std::endl;
	return false;
}

void GlobalEvents::startup()
{
	execute(SERVER_EVENT_STARTUP);

	Scheduler::getScheduler().addEvent(createSchedulerTask(GLOBAL_THINK_INTERVAL,
		boost::bind(&GlobalEvents::think, this, GLOBAL_THINK_INTERVAL)));
}

void GlobalEvents::think(uint32_t interval)
{
	uint32_t timeNow = time(NULL);
	for(GlobalEventMap::iterator it = eventsMap.begin(); it != eventsMap.end(); ++it)
	{
		if(timeNow > (it->second->getLastExecution() + it->second->getInterval()))
		{
			it->second->setLastExecution(timeNow);
			if(!it->second->executeThink(it->second->getInterval(), timeNow, interval))
				std::cout << "[Error - GlobalEvents::think] Couldn't execute event: "
					<< it->second->getName() << std::endl;
		}
	}

	Scheduler::getScheduler().addEvent(createSchedulerTask(interval,
		boost::bind(&GlobalEvents::think, this, interval)));
}

void GlobalEvents::execute(ServerEvent_t type)
{
	for(GlobalEventMap::iterator it = serverEventsMap.begin(); it != serverEventsMap.end(); ++it)
	{
		if(it->second->getEventType() == type)
			it->second->executeServerEvent();
	}
}

GlobalEventMap GlobalEvents::getServerEvents(ServerEvent_t type)
{
	GlobalEventMap ret;
	for(GlobalEventMap::iterator it = serverEventsMap.begin(); it != serverEventsMap.end(); ++it)
	{
		if(it->second->getEventType() == type)
			ret[it->first] = it->second;
	}

	return ret;
}

GlobalEvent::GlobalEvent(LuaScriptInterface* _interface):
	Event(_interface)
{
	m_lastExecution = time(NULL);
}

bool GlobalEvent::configureEvent(xmlNodePtr p)
{
	std::string strValue;
	if(!readXMLString(p, "name", strValue))
	{
		std::cout << "[Error - GlobalEvent::configureEvent] No name for a globalevent." << std::endl;
		return false;
	}

	m_name = strValue;
	m_eventType = SERVER_EVENT_NONE;
	if(readXMLString(p, "type", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "startup" || tmpStrValue == "start" || tmpStrValue == "load")
			m_eventType = SERVER_EVENT_STARTUP;
		else if(tmpStrValue == "shutdown" || tmpStrValue == "quit" || tmpStrValue == "exit")
			m_eventType = SERVER_EVENT_SHUTDOWN;
		else if(tmpStrValue == "record" || tmpStrValue="playersrecord")
			m_eventType = SERVER_EVENT_RECORD;
		else
		{
			std::cout << "[Error - GlobalEvent::configureEvent] No valid type \"" << strValue << "\" for globalevent with name " << m_name << std::endl;
			return false;
		}

		return true;
	}

	int32_t intValue;
	if(!readXMLInteger(p, "interval", intValue))
	{
		std::cout << "[Error - GlobalEvent::configureEvent] No interval for globalevent with name " << m_name << std::endl;
		return false;
	}

	m_interval = intValue;
	return true;
}

std::string GlobalEvent::getScriptEventName() const
{
	switch(m_eventType)
	{
		case SERVER_EVENT_STARTUP:
			return "onStartup";
		case SERVER_EVENT_SHUTDOWN:
			return "onShutdown";
		case SERVER_EVENT_RECORD:
			return "onPlayersRecord";

		case SERVER_EVENT_NONE:
		default:
			return "onThink";
	}
}

std::string GlobalEvent::getScriptEventParams() const
{
	switch(m_eventType)
	{
		case SERVER_EVENT_RECORD:
			return "current, old, cid";

		case SERVER_EVENT_NONE:
			return "interval, lastExecution, thinkInterval"";

		case SERVER_EVENT_STARTUP:
		case SERVER_EVENT_SHUTDOWN:
		default:
			return "";
	}
}

int32_t GlobalEvent::executeThink(uint32_t interval, uint32_t lastExecution, uint32_t thinkInterval)
{
	//onThink(interval, lastExecution, thinkInterval)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			std::stringstream scriptstream;
			scriptstream << "local interval = " << interval << std::endl;

			scriptstream << "local lastExecution = " << lastExecution << std::endl;
			scriptstream << "local thinkInterval = " << thinkInterval << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getFieldBool(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[125];
			sprintf(desc, "%s - %i (%i)", getName().c_str(), interval, lastExecution);
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			lua_State* L = m_scriptInterface->getLuaState();

			m_scriptInterface->pushFunction(m_scriptId);
			lua_pushnumber(L, interval);

			lua_pushnumber(L, lastExecution);
			lua_pushnumber(L, thinkInterval);

			bool result = m_scriptInterface->callFunction(3);
			m_scriptInterface->releaseScriptEnv();
			return result;
		}
	}
	else
	{
		std::cout << "[Error - GlobalEvent::executeThink] Call stack overflow." << std::endl;
		return 0;
	}
}

int32_t GlobalEvent::executeRecord(uint32_t current, uint32_t old, Player* player)
{
	//onRecord(current, old, cid)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			std::stringstream scriptstream;
			scriptstream << "local current = " << current << std::endl;
			scriptstream << "local old = " << old << std::endl;
			scriptstream << "local cid = " << env->addThing(player) << std::endl;

			scriptstream << m_scriptData;
			bool result = true;
			if(m_scriptInterface->loadBuffer(scriptstream.str()) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getFieldBool(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return result;
		}
		else
		{
			#ifdef __DEBUG_LUASCRIPTS__
			char desc[125];
			sprintf(desc, "%s - %i (%i)", getName().c_str(), interval, lastExecution);
			env->setEventDesc(desc);
			#endif

			env->setScriptId(m_scriptId, m_scriptInterface);
			lua_State* L = m_scriptInterface->getLuaState();

			m_scriptInterface->pushFunction(m_scriptId);
			lua_pushnumber(L, current);
			lua_pushnumber(L, old);
			lua_pushnumber(L, env->addThing(player));

			bool result = m_scriptInterface->callFunction(3);
			m_scriptInterface->releaseScriptEnv();
			return result;
		}
	}
	else
	{
		std::cout << "[Error - GlobalEvent::executeRecord] Call stack overflow." << std::endl;
		return 0;
	}
}

int32_t GlobalEvent::executeServerEvent()
{
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();
		if(m_scripted == EVENT_SCRIPT_BUFFER)
		{
			bool result = true;
			if(m_scriptInterface->loadBuffer(m_scriptData) != -1)
			{
				lua_State* L = m_scriptInterface->getLuaState();
				result = m_scriptInterface->getFieldBool(L, "_result");
			}

			m_scriptInterface->releaseScriptEnv();
			return result;
		}
		else
		{
			env->setScriptId(m_scriptId, m_scriptInterface);
			m_scriptInterface->pushFunction(m_scriptId);

			bool result = m_scriptInterface->callFunction(0);
			m_scriptInterface->releaseScriptEnv();
			return result;
		}
	}
	else
	{
		std::cout << "[Error - GlobalEvent::executeServerEvent] Call stack overflow." << std::endl;
		return 0;
	}
}
