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

#include "const.h"
#include "player.h"
#include "monster.h"
#include "npc.h"
#include "game.h"
#include "item.h"
#include "container.h"
#include "combat.h"
#include "house.h"
#include "tasks.h"
#include "tools.h"
#include "spells.h"
#include "configmanager.h"
#include "beds.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "actions.h"

extern Game g_game;
extern Spells* g_spells;
extern Actions* g_actions;
extern ConfigManager g_config;

Actions::Actions():
m_scriptInterface("Action Interface")
{
	m_scriptInterface.initState();
}

Actions::~Actions()
{
	clear();
}

inline void Actions::clearMap(ActionUseMap& map)
{
	ActionUseMap::iterator it = map.begin();
	while(it != map.end())
	{
		delete it->second;
		map.erase(it);
		it = map.begin();
	}
}

void Actions::clear()
{
	clearMap(useItemMap);
	clearMap(uniqueItemMap);
	clearMap(actionItemMap);

	m_scriptInterface.reInitState();
}

LuaScriptInterface& Actions::getScriptInterface()
{
	return m_scriptInterface;
}

std::string Actions::getScriptBaseName()
{
	return "actions";
}

Event* Actions::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "action")
		return new Action(&m_scriptInterface);
	
	return NULL;
}

bool Actions::registerEvent(Event* event, xmlNodePtr p)
{
	Action* action = dynamic_cast<Action*>(event);
	if(!action)
		return false;

	int32_t id, endId, tmp;
	bool success = true;
	if(readXMLInteger(p, "itemid", id))
	{
		if(useItemMap.find(id) != useItemMap.end())
		{
			std::cout << "[Warning - Actions::registerEvent] Duplicate registered item with id: " << id << std::endl;
			return false;
		}

		useItemMap[id] = action;
	}
	else if(readXMLInteger(p, "fromid", id) && readXMLInteger(p, "toid", endId))
	{
		tmp = id;
		if(useItemMap.find(id) != useItemMap.end())
		{
			std::cout << "[Warning - Actions::registerEvent] Duplicate registered item with id: " << id << ", in fromid: " << tmp << " and toid: " << endId << std::endl;
			success = false;
		}
		else
			useItemMap[id] = action;

		while(id < endId)
		{
			id++;
			if(useItemMap.find(id) != useItemMap.end())
			{
				std::cout << "[Warning - Actions::registerEvent] Duplicate registered item with id: " << id << ", in fromid: " << tmp << " and toid: " << endId << std::endl;
				continue;
			}

			useItemMap[id] = new Action(action);
		}
	}
	else if(readXMLInteger(p, "uniqueid", id))
	{
		if(uniqueItemMap.find(id) != uniqueItemMap.end())
		{
			std::cout << "[Warning - Actions::registerEvent] Duplicate registered item with uniqueid: " << id << std::endl;
			return false;
		}

		uniqueItemMap[id] = action;
	}
	else if(readXMLInteger(p, "fromuid", id) && readXMLInteger(p, "touid", endId))
	{
		tmp = id;
		if(uniqueItemMap.find(id) != uniqueItemMap.end())
		{
			std::cout << "[Warning - Actions::registerEvent] Duplicate registered item with uniqueid: " << id << ", in fromuid: " << tmp << " and touid: " << endId << std::endl;
			success = false;
		}
		else
			uniqueItemMap[id] = action;

		while(id < endId)
		{
			id++;
			if(uniqueItemMap.find(id) != uniqueItemMap.end())
			{
				std::cout << "[Warning - Actions::registerEvent] Duplicate registered item with uniqueid: " << id << ", in fromuid: " << tmp << " and touid: " << endId << std::endl;
				continue;
			}

			uniqueItemMap[id] = new Action(action);
		}
	}
	else if(readXMLInteger(p, "actionid", id))
	{
		if(actionItemMap.find(id) != actionItemMap.end())
		{
			std::cout << "[Warning - Actions::registerEvent] Duplicate registered item with actionid: " << id << std::endl;
			return false;
		}

		actionItemMap[id] = action;
	}
	else if(readXMLInteger(p, "fromaid", id) && readXMLInteger(p, "toaid", endId))
	{
		tmp = id;
		if(actionItemMap.find(id) != actionItemMap.end())
		{
			std::cout << "[Warning - Actions::registerEvent] Duplicate registered item with actionid: " << id << ", in fromaid: " << tmp << " and toaid: " << endId << std::endl;
			success = false;
		}
		else
			actionItemMap[id] = action;

		while(id < endId)
		{
			id++;
			if(actionItemMap.find(id) != actionItemMap.end())
			{
				std::cout << "[Warning - Actions::registerEvent] Duplicate registered item with actionid: " << id << ", in fromaid: " << tmp << " and toaid: " << endId << std::endl;
				continue;
			}

			actionItemMap[id] = new Action(action);
		}
	}
	else
		success = false;

	return success;
}

ReturnValue Actions::canUse(const Player* player, const Position& pos)
{
	const Position& playerPos = player->getPosition();
	if(pos.x != 0xFFFF)
	{
		if(playerPos.z > pos.z)
			return RET_FIRSTGOUPSTAIRS;
		else if(playerPos.z < pos.z)
			return RET_FIRSTGODOWNSTAIRS;
		else if(!Position::areInRange<1,1,0>(playerPos, pos))
			return RET_TOOFARAWAY;
	}

	return RET_NOERROR;
}

ReturnValue Actions::canUse(const Player* player, const Position& pos, const Item* item)
{
	Action* action = NULL;
	if((action = getAction(item, ACTION_UNIQUEID)))
	{
		ReturnValue ret = action->canExecuteAction(player, pos);
		if(ret != RET_NOERROR)
			return ret;

		return RET_NOERROR;
	}

	if((action = getAction(item, ACTION_ACTIONID)))
	{
		ReturnValue ret = action->canExecuteAction(player, pos);
		if(ret != RET_NOERROR)
			return ret;

		return RET_NOERROR;
	}

	if((action = getAction(item, ACTION_ITEMID)))
	{
		ReturnValue ret = action->canExecuteAction(player, pos);
		if(ret != RET_NOERROR)
			return ret;

		return RET_NOERROR;
	}

	if((action = getAction(item, ACTION_RUNEID)))
	{
		ReturnValue ret = action->canExecuteAction(player, pos);
		if(ret != RET_NOERROR)
			return ret;

		return RET_NOERROR;
	}

	return RET_NOERROR;
}

ReturnValue Actions::canUseFar(const Creature* creature, const Position& toPos, bool checkLineOfSight)
{
	if(toPos.x == 0xFFFF)
		return RET_NOERROR;

	const Position& creaturePos = creature->getPosition();
	if(creaturePos.z > toPos.z)
		return RET_FIRSTGOUPSTAIRS;
	else if(creaturePos.z < toPos.z)
		return RET_FIRSTGODOWNSTAIRS;
	else if(!Position::areInRange<7,5,0>(toPos, creaturePos))
		return RET_TOOFARAWAY;

	if(checkLineOfSight && !g_game.canThrowObjectTo(creaturePos, toPos))
		return RET_CANNOTTHROW;

	return RET_NOERROR;
}

Action* Actions::getAction(const Item* item, ActionType_t type /* = ACTION_ANY */) const
{
	if(item->getUniqueId() != 0 && (type == ACTION_ANY || type == ACTION_UNIQUEID))
	{
		ActionUseMap::const_iterator it = uniqueItemMap.find(item->getUniqueId());
		if(it != uniqueItemMap.end())
			return it->second;
	}

	if(item->getActionId() != 0 && (type == ACTION_ANY || type == ACTION_ACTIONID))
	{
		ActionUseMap::const_iterator it = actionItemMap.find(item->getActionId());
		if(it != actionItemMap.end())
			return it->second;
	}

	if(type == ACTION_ANY || type == ACTION_ITEMID)
	{
		ActionUseMap::const_iterator it = useItemMap.find(item->getID());
		if(it != useItemMap.end())
	   		return it->second;
	}

	if(type == ACTION_ANY || type == ACTION_RUNEID)
	{
		if(Action* runeSpell = g_spells->getRuneSpell(item->getID()))
			return runeSpell;
	}

	return NULL;
}

bool Actions::executeUse(Action* action, Player* player, Item* item,
	const PositionEx& posEx, uint32_t creatureId)
{
	return action->executeUse(player, item, posEx, posEx, false, creatureId);
}

ReturnValue Actions::internalUseItem(Player* player, const Position& pos, uint8_t index, Item* item, uint32_t creatureId)
{
	if(Door* door = item->getDoor())
	{
		if(!door->canUse(player))
			return RET_CANNOTUSETHISOBJECT;
	}

	int32_t tmp = item->getParent()->__getIndexOfThing(item);
	PositionEx posEx(pos, tmp);

	Action* action = NULL;
	if((action = getAction(item, ACTION_UNIQUEID)))
	{
		if(action->isScripted())
		{
			if(executeUse(action, player, item, posEx, creatureId))
				return RET_NOERROR;
		}
		else if(action->function)
		{
			if(action->function(player, item, posEx, posEx, false, creatureId))
				return RET_NOERROR;
		}
	}

	if((action = getAction(item, ACTION_ACTIONID)))
	{
		if(action->isScripted())
		{
			if(executeUse(action, player, item, posEx, creatureId))
				return RET_NOERROR;
		}
		else if(action->function)
		{
			if(action->function(player, item, posEx, posEx, false, creatureId))
				return RET_NOERROR;
		}
	}

	if((action = getAction(item, ACTION_ITEMID)))
	{
		if(action->isScripted())
		{
			if(executeUse(action, player, item, posEx, creatureId))
				return RET_NOERROR;
		}
		else if(action->function)
		{
			if(action->function(player, item, posEx, posEx, false, creatureId))
				return RET_NOERROR;
		}
	}

	if((action = getAction(item, ACTION_RUNEID)))
	{
		if(action->isScripted())
		{
			if(executeUse(action, player, item, posEx, creatureId))
				return RET_NOERROR;
		}
		else if(action->function)
		{
			if(action->function(player, item, posEx, posEx, false, creatureId))
				return RET_NOERROR;
		}
	}

	if(BedItem* bed = item->getBed())
	{
		if(!bed->canUse(player))
			return RET_CANNOTUSETHISOBJECT;

		bed->sleep(player);
		return RET_NOERROR;
	}

	bool executed = getAction(item) != NULL;
	if(Container* container = item->getContainer())
	{
		if(container->getCorpseOwner() != 0 && !player->canOpenCorpse(container->getCorpseOwner()))
			return RET_YOUARENOTTHEOWNER;

		Container* tmpContainer = NULL;
		if(Depot* depot = container->getDepot())
		{
			Depot* tmpDepot = player->getDepot(depot->getDepotId(), true);
			tmpDepot->setParent(depot->getParent());
			tmpContainer = tmpDepot;
		}
		else
			tmpContainer = container;

		int32_t oldId = player->getContainerID(tmpContainer);
		if(oldId != -1)
		{
			player->onCloseContainer(tmpContainer);
			player->closeContainer(oldId);
		}
		else
		{
			player->addContainer(index, tmpContainer);
			player->onSendContainer(tmpContainer);
		}

		executed = true;
	}

	if(item->isReadable())
	{
		if(item->canWriteText())
		{
			player->setWriteItem(item, item->getMaxWriteLength());
			player->sendTextWindow(item, item->getMaxWriteLength(), true);
		}
		else
		{
			player->setWriteItem(NULL);
			player->sendTextWindow(item, 0, false);
		}

		executed = true;
	}

	if(executed)
		return RET_CANNOTUSETHISOBJECT;

	return RET_NOERROR;
}

bool Actions::useItem(Player* player, const Position& pos, uint8_t index, Item* item, bool isHotkey)
{
	if(!player->canDoAction())
		return false;

	player->setNextActionTask(NULL);
	player->stopWalk();

	int32_t itemId = 0;
	uint32_t itemCount = 0;

	if(isHotkey)
	{
		int32_t subType = -1;
		if(item->hasSubType() && !item->hasCharges())
			subType = item->getSubType();

		itemCount = player->__getItemTypeCount(item->getID(), subType, false);
		itemId = item->getID();
	}

	ReturnValue ret = internalUseItem(player, pos, index, item, 0);
	if(ret != RET_NOERROR)
	{
		player->sendCancelMessage(ret);
		return false;
	}

	if(isHotkey)
		showUseHotkeyMessage(player, itemId, itemCount);

	player->setNextAction(OTSYS_TIME() + g_config.getNumber(ConfigManager::ACTIONS_DELAY_INTERVAL));
	return true;
}

bool Actions::executeUseEx(Action* action, Player* player, Item* item, const PositionEx& fromPosEx,
	const PositionEx& toPosEx, bool isHotkey, uint32_t creatureId)
{
	return (action->executeUse(player, item, fromPosEx, toPosEx, isHotkey, creatureId) || action->hasOwnErrorHandler());
}

ReturnValue Actions::internalUseItemEx(Player* player, const PositionEx& fromPosEx, const PositionEx& toPosEx,
	Item* item, bool isHotkey, uint32_t creatureId)
{
	Action* action = NULL;
	if((action = getAction(item, ACTION_UNIQUEID)))
	{
		ReturnValue ret = action->canExecuteAction(player, toPosEx);
		if(ret != RET_NOERROR)
			return ret;

		//only continue with next action in the list if the previous returns false
		if(executeUseEx(action, player, item, fromPosEx, toPosEx, isHotkey, creatureId))
			return RET_NOERROR;
	}

	if((action = getAction(item, ACTION_ACTIONID)))
	{
		ReturnValue ret = action->canExecuteAction(player, toPosEx);
		if(ret != RET_NOERROR)
			return ret;

		//only continue with next action in the list if the previous returns false
		if(executeUseEx(action, player, item, fromPosEx, toPosEx, isHotkey, creatureId))
			return RET_NOERROR;
	}

	if((action = getAction(item, ACTION_ITEMID)))
	{
		ReturnValue ret = action->canExecuteAction(player, toPosEx);
		if(ret != RET_NOERROR)
			return ret;

		//only continue with next action in the list if the previous returns false
		if(executeUseEx(action, player, item, fromPosEx, toPosEx, isHotkey, creatureId))
			return RET_NOERROR;
	}

	if((action = getAction(item, ACTION_RUNEID)))
	{
		ReturnValue ret = action->canExecuteAction(player, toPosEx);
		if(ret != RET_NOERROR)
			return ret;

		//only continue with next action in the list if the previous returns false
		if(executeUseEx(action, player, item, fromPosEx, toPosEx, isHotkey, creatureId))
			return RET_NOERROR;
	}

	return RET_CANNOTUSETHISOBJECT;
}

bool Actions::useItemEx(Player* player, const Position& fromPos, const Position& toPos,
	uint8_t toStackPos, Item* item, bool isHotkey, uint32_t creatureId /* = 0*/)
{
	if(!player->canDoAction())
		return false;

	player->setNextActionTask(NULL);
	player->stopWalk();

	Action* action = getAction(item);
	if(!action)
	{
		player->sendCancelMessage(RET_CANNOTUSETHISOBJECT);
		return false;
	}

	int32_t itemId = 0;
	uint32_t itemCount = 0;

	if(isHotkey)
	{
		int32_t subType = -1;
		if(item->hasSubType() && !item->hasCharges())
			subType = item->getSubType();

		itemCount = player->__getItemTypeCount(item->getID(), subType, false);
		itemId = item->getID();
	}

	int32_t fromStackPos = item->getParent()->__getIndexOfThing(item);
	PositionEx fromPosEx(fromPos, fromStackPos);
	PositionEx toPosEx(toPos, toStackPos);

	ReturnValue ret = internalUseItemEx(player, fromPosEx, toPosEx, item, isHotkey, creatureId);
	if(ret != RET_NOERROR)
	{
		player->sendCancelMessage(ret);
		return false;
	}

	if(isHotkey)
		showUseHotkeyMessage(player, itemId, itemCount);

	player->setNextAction(OTSYS_TIME() + g_config.getNumber(ConfigManager::EX_ACTIONS_DELAY_INTERVAL));
	return true;
}

void Actions::showUseHotkeyMessage(Player* player, int32_t id, uint32_t count)
{
	const ItemType& it = Item::items[id];
	char buffer[40 + it.name.size()];
	if(count == 1)
		sprintf(buffer, "Using the last %s...", it.name.c_str());
	else
		sprintf(buffer, "Using one of %d %s...", count, it.pluralName.c_str());

	player->sendTextMessage(MSG_INFO_DESCR, buffer);
}

Action::Action(LuaScriptInterface* _interface):
Event(_interface)
{
	allowFarUse = false;
	checkLineOfSight = true;
}

Action::Action(const Action* copy) : Event(copy)
{
	allowFarUse = copy->allowFarUse;
	checkLineOfSight = copy->checkLineOfSight;
}

Action::~Action()
{
	//
}

bool Action::configureEvent(xmlNodePtr p)
{
	std::string strValue;
	if(readXMLString(p, "allowfaruse", strValue))
	{
		if(booleanString(strValue))
			setAllowFarUse(true);
	}

	if(readXMLString(p, "blockwalls", strValue))
	{
		if(!booleanString(strValue))
			setCheckLineOfSight(false);
	}

	return true;
}

bool Action::loadFunction(const std::string& functionName)
{
	std::string tmpFunctionName = asLowerCaseString(functionName);
	if(tmpFunctionName == "increaseitemid")
		function = increaseItemId;
	else if(tmpFunctionName == "decreaseitemid")
		function = decreaseItemId;
	else
	{
		std::cout << "[Warning - Action::loadFunction] Function \"" << functionName << "\" does not exist." << std::endl;
		return false;
	}

	m_scripted = false;
	return true;
}

bool Action::increaseItemId(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo, bool extendedUse, uint32_t creatureId)
{
	if(player && item)
	{
		g_game.transformItem(item, item->getID() + 1);
		return true;
	}

	return false;
}

bool Action::decreaseItemId(Player* player, Item* item, const PositionEx& posFrom, const PositionEx& posTo, bool extendedUse, uint32_t creatureId)
{
	if(player && item)
	{
		g_game.transformItem(item, item->getID() - 1);
		return true;
	}

	return false;
}

std::string Action::getScriptEventName()
{
	return "onUse";
}

ReturnValue Action::canExecuteAction(const Player* player, const Position& toPos)
{
	ReturnValue ret = RET_NOERROR;
	if(!getAllowFarUse())
		ret = g_actions->canUse(player, toPos);
	else
		ret = g_actions->canUseFar(player, toPos, getCheckLineOfSight());

	return ret;
}

bool Action::executeUse(Player* player, Item* item, const PositionEx& fromPos, const PositionEx& toPos, bool extendedUse, uint32_t creatureId)
{
	//onUse(cid, item, fromPosition, itemEx, toPosition)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << player->getName() << " - " << item->getID() << " " << fromPos << "|" << toPos;
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);
		uint32_t itemId = env->addThing(item);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		LuaScriptInterface::pushThing(L, item, itemId);
		LuaScriptInterface::pushPosition(L, fromPos, fromPos.stackpos);

		Thing* thing = g_game.internalGetThing(player, toPos, toPos.stackpos);
		if(thing && (thing != item || !extendedUse))
		{
			uint32_t thingId = env->addThing(thing);
			LuaScriptInterface::pushThing(L, thing, thingId);
			LuaScriptInterface::pushPosition(L, toPos, toPos.stackpos);
		}
		else
		{
			LuaScriptInterface::pushThing(L, NULL, 0);
			LuaScriptInterface::pushPosition(L, fromPos, fromPos.stackpos);
		}

		int32_t result = m_scriptInterface->callFunction(5);
		m_scriptInterface->releaseScriptEnv();
		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error - Action::executeUse]: Call stack overflow." << std::endl;
		return false;
	}
}
