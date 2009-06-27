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

#ifndef __GLOBALEVENT__
#define __GLOBALEVENT__
#include "baseevents.h"

#include "const.h"
#include "scheduler.h"

#define GLOBAL_THINK_INTERVAL 1000

enum ServerEvent_t
{
	SERVER_EVENT_NONE,
	SERVER_EVENT_STARTUP,
	SERVER_EVENT_SHUTDOWN
};

class GlobalEvent;
class GlobalEvents : public BaseEvents
{
	public:
		GlobalEvents();
		virtual ~GlobalEvents();

		void startup();
		void shutdown();
		void think(uint32_t interval);

	protected:
		virtual std::string getScriptBaseName() const {return "globalevents";}
		virtual void clear();

		virtual Event* getEvent(const std::string& nodeName);
		virtual bool registerEvent(Event* event, xmlNodePtr p, bool override);

		virtual LuaScriptInterface& getScriptInterface() {return m_scriptInterface;}
		LuaScriptInterface m_scriptInterface;

		typedef std::map<std::string, GlobalEvent*> GlobalEventMap;
		GlobalEventMap eventsMap, serverEventsMap;
};

class GlobalEvent : public Event
{
	public:
		GlobalEvent(LuaScriptInterface* _interface);
		virtual ~GlobalEvent() {}

		virtual bool configureEvent(xmlNodePtr p);
		int32_t executeThink(uint32_t interval, uint32_t lastExecution, uint32_t thinkInterval);
		int32_t executeServerEvent();

		ServerEvent_t getEventType() const {return m_eventType;}
		std::string getName() const {return m_name;}
		uint32_t getInterval() const {return m_interval;}

		uint32_t getLastExecution() const {return m_lastExecution;}
		void setLastExecution(uint32_t time) {m_lastExecution = time;}

	protected:
		virtual std::string getScriptEventName() const;
		virtual std::string getScriptEventParams() const;

		std::string m_name;
		uint32_t m_interval, m_lastExecution;
		ServerEvent_t m_eventType;
};
#endif
