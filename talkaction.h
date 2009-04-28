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

#ifndef __TALKACTION_H__
#define __TALKACTION_H__
#include "otsystem.h"
#include "enums.h"

#include "baseevents.h"
#include "luascript.h"
#include "player.h"
#include "tools.h"

enum TalkActionFilter
{
	TALKFILTER_QUOTATION,
	TALKFILTER_WORD,
	TALKFILTER_LAST
};
class TalkAction;

class TalkActions : public BaseEvents
{
	public:
		TalkActions();
		virtual ~TalkActions();

		bool onPlayerSay(Creature* creature, uint16_t channelId, const std::string& words, bool ignoreAccess);

	protected:
		virtual std::string getScriptBaseName() const {return "talkactions";}
		virtual void clear();

		virtual Event* getEvent(const std::string& nodeName);
		virtual bool registerEvent(Event* event, xmlNodePtr p);

		virtual LuaScriptInterface& getScriptInterface() {return m_scriptInterface;}
		LuaScriptInterface m_scriptInterface;

		typedef std::map<std::string, TalkAction*> TalkActionsMap;
		TalkActionsMap talksMap;
};

typedef bool (TalkFunction)(Creature* creature, const std::string& words, const std::string& param);
struct TalkFunction_t;

class TalkAction : public Event
{
	public:
		TalkAction(LuaScriptInterface* _interface);
		virtual ~TalkAction() {}

		virtual bool configureEvent(xmlNodePtr p);
		virtual bool loadFunction(const std::string& functionName);

		int32_t executeSay(Creature* creature, const std::string& words, const std::string& param, uint16_t channel);
		TalkFunction* function;

		std::string getWords() const {return m_words;}
		TalkActionFilter getFilter() const {return m_filter;}

		uint32_t getAccess() const {return m_access;}
		StringVec getExceptions() {return m_exceptions;}
		int32_t getChannel() const {return m_channel;}

		bool isLogged() const {return m_logged;}
		bool isSensitive() const {return m_sensitive;}

	protected:
		static TalkFunction_t definedFunctions[];

		virtual std::string getScriptEventName() const {return "onSay";}
		virtual std::string getScriptEventParams() const {return "cid, words, param, channel";}

		static TalkFunction serverDiag;
		static TalkFunction sellHouse;
		static TalkFunction buyHouse;
		static TalkFunction joinGuild;
		static TalkFunction createGuild;
		static TalkFunction ghost;
		static TalkFunction addSkill;
		static TalkFunction changeThingProporties;
		static TalkFunction showBanishmentInfo;

		std::string m_words;
		TalkActionFilter m_filter;
		uint32_t m_access;
		int32_t m_channel;
		bool m_logged, m_sensitive;
		StringVec m_exceptions;
};

struct TalkFunction_t
{
	const char* name;
	TalkFunction* callback;
};

#endif
