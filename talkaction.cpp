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

#include <string>
#include <fstream>
#include <utility>
#include <cstring>
#include <cerrno>

#include "creature.h"
#include "definitions.h"
#include "player.h"
#include "npc.h"
#include "monsters.h"
#include "game.h"
#include "actions.h"
#include "house.h"
#include "iologindata.h"
#include "tools.h"
#include "ban.h"
#include "configmanager.h"
#include "town.h"
#include "spells.h"
#include "talkaction.h"
#include "movement.h"
#include "spells.h"
#include "weapons.h"

#include "raids.h"
#include "chat.h"
#include "quests.h"
#include "mounts.h"
#include "globalevent.h"

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
#include "outputmessage.h"
#include "connection.h"
#include "admin.h"
#include "status.h"
#include "protocollogin.h"
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <boost/version.hpp>

#include "talkaction.h"

extern ConfigManager g_config;
extern Actions* g_actions;
extern Monsters g_monsters;
extern Npcs g_npcs;
extern TalkActions* g_talkActions;
extern MoveEvents* g_moveEvents;
extern Spells* g_spells;
extern Weapons* g_weapons;
extern Game g_game;
extern Chat g_chat;
extern CreatureEvents* g_creatureEvents;
extern GlobalEvents* g_globalEvents;

TalkActions::TalkActions() :
m_scriptInterface("TalkAction Interface")
{
	m_scriptInterface.initState();
}

TalkActions::~TalkActions()
{
	clear();
}

void TalkActions::clear()
{
	for(TalkActionsMap::iterator it = talksMap.begin(); it != talksMap.end(); ++it)
		delete it->second;

	talksMap.clear();
	m_scriptInterface.reInitState();
}

LuaScriptInterface& TalkActions::getScriptInterface()
{
	return m_scriptInterface;
}

std::string TalkActions::getScriptBaseName()
{
	return "talkactions";
}

Event* TalkActions::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "talkaction")
		return new TalkAction(&m_scriptInterface);

	return NULL;
}

bool TalkActions::registerEvent(Event* event, xmlNodePtr p)
{
	TalkAction* talkAction = dynamic_cast<TalkAction*>(event);
	if(!talkAction)
		return false;

	talksMap[talkAction->getWords()] = talkAction;
	return true;
}

bool TalkActions::onPlayerSay(Player* player, uint16_t channelId, const std::string& words)
{
	std::string cmd[TALKFILTER_LAST], param[TALKFILTER_LAST];
	for(int32_t i = 0; i < TALKFILTER_LAST; ++i)
		cmd[i] = words;

	std::string::size_type loc = words.find('"', 0);
	if(loc != std::string::npos)
	{
		cmd[TALKFILTER_QUOTATION] = std::string(words, 0, loc);
		param[TALKFILTER_QUOTATION] = std::string(words, (loc + 1), (words.size() - (loc - 1)));
		trimString(cmd[TALKFILTER_QUOTATION]);
	}

	loc = words.find(" ", 0);
	if(loc != std::string::npos)
	{
		cmd[TALKFILTER_WORD] = std::string(words, 0, loc);
		param[TALKFILTER_WORD] = std::string(words, (loc + 1), (words.size() - (loc - 1)));

		std::string::size_type spaceLoc = words.find(" ", ++loc);
		if(spaceLoc != std::string::npos)
		{
			cmd[TALKFILTER_WORD_SPACED] = std::string(words, 0, spaceLoc);
			param[TALKFILTER_WORD_SPACED] = std::string(words, (spaceLoc + 1), (words.size() - (spaceLoc - 1)));
		}
	}

	TalkAction* talkAction = NULL;
	for(TalkActionsMap::iterator it = talksMap.begin(); it != talksMap.end(); ++it)
	{
		if(it->first == cmd[it->second->getFilter()] || (!it->second->isSensitive()
			&& boost::algorithm::iequals(it->first, cmd[it->second->getFilter()])))
		{
			talkAction = it->second;
			break;
		}
	}

	if(!talkAction || (talkAction->getChannel() != -1 && talkAction->getChannel() != channelId))
		return false;

	if(talkAction->getGroup() > player->getGroupId() || talkAction->getAccountType() > player->getAccountType() || player->isAccountManager())
	{
		if(player->isAccessPlayer())
		{
			player->sendTextMessage(MSG_STATUS_SMALL, "You cannot execute this talkaction.");
			return true;
		}

		return false;
	}

	if(talkAction->isLogged())
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, words.c_str());
		if(dirExists("data/logs/talkactions") || createDir("data/logs/talkactions"))
		{
			std::ostringstream ss;
			ss << "data/logs/talkactions/" << player->getName() << ".log";

			std::ofstream out(ss.str().c_str(), std::ios::app);

			time_t ticks = time(NULL);
			const tm* now = localtime(&ticks);
			char buf[32];
			strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", now);

			out << "[" << buf << "] " << words << std::endl;

			out.close();
		}
		else
			std::cout << "[Warning - TalkActions::onPlayerSay] Cannot access \"data/logs\" for writing: " << strerror(errno) << "." << std::endl;
	}

	if(talkAction->isScripted())
		return (talkAction->executeSay(player, cmd[talkAction->getFilter()], param[talkAction->getFilter()], channelId) != 0);

	if(TalkFunction* function = talkAction->getFunction())
		return function(player, cmd[talkAction->getFilter()], param[talkAction->getFilter()]);

	return false;
}

TalkAction::TalkAction(LuaScriptInterface* _interface) :
Event(_interface)
{
	m_function = NULL;
	m_filter = TALKFILTER_WORD;
	m_groupId = 1;
	m_channel = -1;
	m_logged = false;
	m_sensitive = true;
	m_accountType = ACCOUNT_TYPE_GOD;
}

bool TalkAction::configureEvent(xmlNodePtr p)
{
	std::string strValue;
	if(readXMLString(p, "words", strValue))
		m_words = strValue;
	else
	{
		std::cout << "[Error - TalkAction::configureEvent] No words for TalkAction." << std::endl;
		return false;
	}

	if(readXMLString(p, "filter", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "quotation")
			m_filter = TALKFILTER_QUOTATION;
		else if(tmpStrValue == "word")
			m_filter = TALKFILTER_WORD;
		else if(tmpStrValue == "word-spaced")
			m_filter = TALKFILTER_WORD_SPACED;
		else
			std::cout << "[Warning - TalkAction::configureEvent] Unknown filter for TalkAction: " << strValue << ", using default." << std::endl;
	}

	int32_t intValue;
	if(readXMLInteger(p, "group", intValue) || readXMLInteger(p, "groupid", intValue))
		m_groupId = intValue;

	if(readXMLInteger(p, "acctype", intValue) || readXMLInteger(p, "accounttype", intValue))
		m_accountType = (AccountType_t)intValue;

	if(readXMLInteger(p, "channel", intValue))
		m_channel = intValue;

	if(readXMLString(p, "log", strValue))
		m_logged = booleanString(asLowerCaseString(strValue));

	if(readXMLString(p, "case-sensitive", strValue) || readXMLString(p, "casesensitive", strValue) || readXMLString(p, "sensitive", strValue))
		m_sensitive = booleanString(asLowerCaseString(strValue));

	return true;
}

bool TalkAction::loadFunction(const std::string& functionName)
{
	m_functionName = asLowerCaseString(functionName);
	/*if(m_functionName == "newtype")
		m_function = newType;
	else
	{
		std::cout << "[Warning - TalkAction::loadFunction] Function \"" << m_functionName << "\" does not exist." << std::endl;
		return false;
	}*/

	m_scripted = false;
	return true;
};

std::string TalkAction::getScriptEventName()
{
	return "onSay";
}

int32_t TalkAction::executeSay(Creature* creature, const std::string& words, const std::string& param, uint16_t channel)
{
	//onSay(cid, words, param, channel)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnvironment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::ostringstream;
		ss << creature->getName() << " - " << words << "- " << param;
		env->setEventDesc(ss.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushstring(L, words.c_str());
		lua_pushstring(L, param.c_str());
		lua_pushnumber(L, channel);

		bool result = m_scriptInterface->callFunction(4);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::cout << "[Error - Talkaction::executeSay] Call stack overflow." << std::endl;
		return 0;
	}
}
