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

#ifndef __BASEEVENTS_H__
#define __BASEEVENTS_H__
#include "otsystem.h"
#include "luascript.h"
#include <libxml/parser.h>

enum EventScript_t
{
	EVENT_SCRIPT_FALSE,
	EVENT_SCRIPT_BUFFER,
	EVENT_SCRIPT_TRUE
};

class Event;

class BaseEvents
{
	public:
		BaseEvents(): m_loaded(false) {}
		virtual ~BaseEvents() {}

		bool loadFromXml();
		bool reload();

		bool parseEventNode(xmlNodePtr p, std::string scriptsPath);
		bool isLoaded() const {return m_loaded;}

	protected:
		virtual std::string getScriptBaseName() const = 0;
		virtual void clear() = 0;

		virtual bool registerEvent(Event* event, xmlNodePtr p) = 0;
		virtual Event* getEvent(const std::string& nodeName) = 0;

		virtual LuaScriptInterface& getScriptInterface() = 0;

		bool m_loaded;
};

class Event
{
	public:
		Event(LuaScriptInterface* _interface): m_scriptInterface(_interface),
			m_scripted(EVENT_SCRIPT_FALSE), m_scriptId(0) {}
		Event(const Event* copy);
		virtual ~Event() {}

		virtual bool configureEvent(xmlNodePtr p) = 0;

		bool loadBuffer(const std::string& scriptFile);
		bool loadScript(const std::string& scriptBuffer, bool file);
		virtual bool loadFunction(const std::string& functionName);

		virtual bool isScripted() const {return m_scripted != EVENT_SCRIPT_FALSE;}

	protected:
		virtual std::string getScriptEventName() const = 0;
		virtual std::string getScriptEventParams() const = 0;

		LuaScriptInterface* m_scriptInterface;
		EventScript_t m_scripted;

		int32_t m_scriptId;
		std::string m_scriptData;
};


class CallBack
{
	public:
		CallBack();
		virtual ~CallBack() {};

		bool loadCallBack(LuaScriptInterface* _interface, std::string name);

	protected:
		int32_t m_scriptId;
		LuaScriptInterface* m_scriptInterface;

		bool m_loaded;

		std::string m_callbackName;
};

#endif
