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

#include <map>
#include <string>
#include "luascript.h"
#include "baseevents.h"
#include "creature.h"
#include "enums.h"

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

		bool onPlayerSay(Player* player, uint16_t channelId, const std::string& words);

	protected:
		virtual LuaScriptInterface& getScriptInterface();
		virtual std::string getScriptBaseName();
		virtual Event* getEvent(const std::string& nodeName);
		virtual bool registerEvent(Event* event, xmlNodePtr p);
		virtual void clear();

		typedef std::map<std::string, TalkAction*> TalkActionsMap;
		TalkActionsMap talksMap;

		LuaScriptInterface m_scriptInterface;
};

typedef bool (TalkFunction)(Player* player, const std::string& words, const std::string& param);
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
		int32_t getChannel() const {return m_channel;}

		bool isLogged() const {return m_logged;}
		bool isSensitive() const {return m_sensitive;}

	protected:
		static TalkFunction placeSummon;
		static TalkFunction serverDiag;
		static TalkFunction sellHouse;
		static TalkFunction buyHouse;
		static TalkFunction joinGuild;
		static TalkFunction createGuild;
		static TalkFunction ghost;
		static TalkFunction squelch;
		static TalkFunction clickTeleport;
		static TalkFunction addSkill;
		static TalkFunction changeThingProporties;
		static TalkFunction showBanishmentInfo;

		std::string m_words;
		TalkActionFilter m_filter;
		uint32_t m_access;
		int32_t m_channel;
		bool m_logged, m_sensitive;

		virtual std::string getScriptEventName();
		static TalkFunction_t definedFunctions[];
};

struct TalkFunction_t
{
	const char* name;
	TalkFunction* callback;
};

#endif
