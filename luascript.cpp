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
#include "luascript.h"
#include "scriptmanager.h"

#include <boost/filesystem.hpp>
#include <boost/any.hpp>
#include <iostream>
#include <iomanip>

#include "player.h"
#include "item.h"
#include "teleport.h"

#include "town.h"
#include "house.h"
#include "housetile.h"

#include "database.h"
#include "iologindata.h"
#include "ioban.h"
#include "iomapserialize.h"

#include "talkaction.h"
#include "spells.h"
#include "combat.h"
#include "condition.h"

#include "baseevents.h"
#include "monsters.h"
#include "raids.h"

#include "configmanager.h"
#include "vocation.h"
#include "status.h"
#include "game.h"
#include "chat.h"

extern Game g_game;
extern Monsters g_monsters;
extern Chat g_chat;
extern ConfigManager g_config;
extern Spells* g_spells;
extern TalkActions* g_talkActions;

enum
{
	EVENT_ID_LOADING = 1,
	EVENT_ID_USER = 1000,
};

ScriptEnviroment::AreaMap ScriptEnviroment::m_areaMap;
uint32_t ScriptEnviroment::m_lastAreaId = 0;
ScriptEnviroment::CombatMap ScriptEnviroment::m_combatMap;
uint32_t ScriptEnviroment::m_lastCombatId = 0;
ScriptEnviroment::ConditionMap ScriptEnviroment::m_conditionMap;
uint32_t ScriptEnviroment::m_lastConditionId = 0;

ScriptEnviroment::ThingMap ScriptEnviroment::m_globalMap;
ScriptEnviroment::StorageMap ScriptEnviroment::m_storageMap;
ScriptEnviroment::TempItemListMap ScriptEnviroment::m_tempItems;

ScriptEnviroment::ScriptEnviroment()
{
	m_lastUID = 70000;
	m_loaded = true;
	reset();
}

ScriptEnviroment::~ScriptEnviroment()
{
	for(CombatMap::iterator it = m_combatMap.begin(); it != m_combatMap.end(); ++it)
		delete it->second;

	m_combatMap.clear();
	for(AreaMap::iterator it = m_areaMap.begin(); it != m_areaMap.end(); ++it)
		delete it->second;

	m_areaMap.clear();
	reset();
}

void ScriptEnviroment::reset()
{
	m_scriptId = m_callbackId = 0;
	m_timerEvent = false;
	m_realPos = Position();

	m_interface = NULL;
	for(TempItemListMap::iterator mit = m_tempItems.begin(); mit != m_tempItems.end(); ++mit)
	{
		ItemList itemList = mit->second;
		for(ItemList::iterator it = itemList.begin(); it != itemList.end(); ++it)
		{
			if((*it)->getParent() == VirtualCylinder::virtualCylinder)
				g_game.freeThing(*it);
		}
	}

	m_tempItems.clear();
	for(DBResultMap::iterator it = m_tempResults.begin(); it != m_tempResults.end(); ++it)
	{
		if(it->second)
			it->second->free();
	}

	m_tempResults.clear();
	m_localMap.clear();
}

bool ScriptEnviroment::saveGameState()
{
	if(!g_config.getBool(ConfigManager::SAVE_GLOBAL_STORAGE))
		return true;

	Database* db = Database::getInstance();
	DBQuery query;

	query << "DELETE FROM `global_storage` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << ";";
	if(!db->executeQuery(query.str()))
		return false;

	DBInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `global_storage` (`key`, `world_id`, `value`) VALUES ");
	for(StorageMap::const_iterator it = m_storageMap.begin(); it != m_storageMap.end(); ++it)
	{
		char buffer[25 + it->second.length()];
		sprintf(buffer, "%u, %u, %s", it->first, g_config.getNumber(ConfigManager::WORLD_ID), db->escapeString(it->second).c_str());
		if(!query_insert.addRow(buffer))
			return false;
	}

	return query_insert.execute();
}

bool ScriptEnviroment::loadGameState()
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `key`, `value` FROM `global_storage` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << ";";
	if((result = db->storeQuery(query.str())))
	{
		do
			m_storageMap[result->getDataInt("key")] = result->getDataString("value");
		while(result->next());
		result->free();
	}

	query.str("");
	return true;
}

bool ScriptEnviroment::setCallbackId(int32_t callbackId, LuaScriptInterface* interface)
{
	if(!m_callbackId)
	{
		m_callbackId = callbackId;
		m_interface = interface;
		return true;
	}

	//nested callbacks are not allowed
	if(m_interface)
		m_interface->errorEx("Nested callbacks!");

	return false;
}

void ScriptEnviroment::getInfo(int32_t& scriptId, std::string& desc, LuaScriptInterface*& interface, int32_t& callbackId, bool& timerEvent)
{
	scriptId = m_scriptId;
	desc = m_eventdesc;
	interface = m_interface;
	callbackId = m_callbackId;
	timerEvent = m_timerEvent;
}

void ScriptEnviroment::addUniqueThing(Thing* thing)
{
	Item* item = thing->getItem();
	if(!item || !item->getUniqueId())
		return;

	if(m_globalMap[item->getUniqueId()])
	{
		if(item->getActionId() != 2000) //scripted quest system
			std::cout << "Duplicate uniqueId " << item->getUniqueId() << std::endl;
	}
	else
		m_globalMap[item->getUniqueId()] = thing;
}

void ScriptEnviroment::removeUniqueThing(Thing* thing)
{
	Item* item = thing->getItem();
	if(!item || !item->getUniqueId())
		return;

	ThingMap::iterator it = m_globalMap.find(item->getUniqueId());
	if(it != m_globalMap.end())
		m_globalMap.erase(it);
}

uint32_t ScriptEnviroment::addThing(Thing* thing)
{
	if(!thing || thing->isRemoved())
		return 0;

	for(ThingMap::iterator it = m_localMap.begin(); it != m_localMap.end(); ++it)
	{
		if(it->second == thing)
			return it->first;
	}

	if(Creature* creature = thing->getCreature())
	{
		m_localMap[creature->getID()] = thing;
		return creature->getID();
	}

	if(Item* item = thing->getItem())
	{
		uint32_t tmp = item->getUniqueId();
		if(tmp)
		{
			m_localMap[tmp] = thing;
			return tmp;
		}
	}

	while(m_localMap.find(m_lastUID) != m_localMap.end())
		++m_lastUID;

	m_localMap[m_lastUID] = thing;
	return m_lastUID;
}

void ScriptEnviroment::insertThing(uint32_t uid, Thing* thing)
{
	if(!m_localMap[uid])
		m_localMap[uid] = thing;
	else
		std::cout << "[Error - ScriptEnviroment::insertThing] Thing uid already taken" << std::endl;
}

Thing* ScriptEnviroment::getThingByUID(uint32_t uid)
{
	Thing* tmp = m_localMap[uid];
	if(tmp && !tmp->isRemoved())
		return tmp;

	tmp = m_globalMap[uid];
	if(tmp && !tmp->isRemoved())
		return tmp;

	if(uid >= 0x10000000)
	{
		tmp = g_game.getCreatureByID(uid);
		if(tmp && !tmp->isRemoved())
		{
			m_localMap[uid] = tmp;
			return tmp;
		}
	}

	return NULL;
}

Item* ScriptEnviroment::getItemByUID(uint32_t uid)
{
	if(Thing* tmp = getThingByUID(uid))
	{
		if(Item* item = tmp->getItem())
			return item;
	}

	return NULL;
}

Container* ScriptEnviroment::getContainerByUID(uint32_t uid)
{
	if(Item* tmp = getItemByUID(uid))
	{
		if(Container* container = tmp->getContainer())
			return container;
	}

	return NULL;
}

Creature* ScriptEnviroment::getCreatureByUID(uint32_t uid)
{
	if(Thing* tmp = getThingByUID(uid))
	{
		if(Creature* creature = tmp->getCreature())
			return creature;
	}

	return NULL;
}

Player* ScriptEnviroment::getPlayerByUID(uint32_t uid)
{
	if(Thing* tmp = getThingByUID(uid))
	{
		if(Creature* creature = tmp->getCreature())
		{
			if(Player* player = creature->getPlayer())
				return player;
		}
	}

	return NULL;
}

void ScriptEnviroment::removeThing(uint32_t uid)
{
	ThingMap::iterator it;
	it = m_localMap.find(uid);
	if(it != m_localMap.end())
		m_localMap.erase(it);

	it = m_globalMap.find(uid);
	if(it != m_globalMap.end())
		m_globalMap.erase(it);
}

uint32_t ScriptEnviroment::addCombatArea(CombatArea* area)
{
	uint32_t newAreaId = m_lastAreaId + 1;
	m_areaMap[newAreaId] = area;

	m_lastAreaId++;
	return newAreaId;
}

CombatArea* ScriptEnviroment::getCombatArea(uint32_t areaId)
{
	AreaMap::const_iterator it = m_areaMap.find(areaId);
	if(it != m_areaMap.end())
		return it->second;

	return NULL;
}

uint32_t ScriptEnviroment::addCombatObject(Combat* combat)
{
	uint32_t newCombatId = m_lastCombatId + 1;
	m_combatMap[newCombatId] = combat;

	m_lastCombatId++;
	return newCombatId;
}

Combat* ScriptEnviroment::getCombatObject(uint32_t combatId)
{
	CombatMap::iterator it = m_combatMap.find(combatId);
	if(it != m_combatMap.end())
		return it->second;

	return NULL;
}

uint32_t ScriptEnviroment::addConditionObject(Condition* condition)
{
	uint32_t newConditionId = m_lastConditionId + 1;
	m_conditionMap[newConditionId] = condition;

	m_lastConditionId++;
	return m_lastConditionId;
}

Condition* ScriptEnviroment::getConditionObject(uint32_t conditionId)
{
	ConditionMap::iterator it = m_conditionMap.find(conditionId);
	if(it != m_conditionMap.end())
		return it->second;

	return NULL;
}

void ScriptEnviroment::addTempItem(ScriptEnviroment* env, Item* item)
{
	m_tempItems[env].push_back(item);
}

void ScriptEnviroment::removeTempItem(ScriptEnviroment* env, Item* item)
{
	ItemList itemList = m_tempItems[env];
	ItemList::iterator it = std::find(itemList.begin(), itemList.end(), item);
	if(it != itemList.end())
		itemList.erase(it);
}

void ScriptEnviroment::removeTempItem(Item* item)
{
	for(TempItemListMap::iterator mit = m_tempItems.begin(); mit != m_tempItems.end(); ++mit)
	{
		ItemList itemList = mit->second;
		ItemList::iterator it = std::find(itemList.begin(), itemList.end(), item);
		if(it != itemList.end())
			itemList.erase(it);
	}
}

uint32_t ScriptEnviroment::addResult(DBResult* res)
{
	uint32_t lastId = 0;
	while(m_tempResults.find(lastId) != m_tempResults.end())
		lastId++;

	m_tempResults[lastId] = res;
	return lastId;
}

bool ScriptEnviroment::removeResult(uint32_t id)
{
	DBResultMap::iterator it = m_tempResults.find(id);
	if(it == m_tempResults.end())
		return false;

	if(it->second)
		it->second->free();

	m_tempResults.erase(it);
	return true;
}

DBResult* ScriptEnviroment::getResultByID(uint32_t id)
{
	DBResultMap::iterator it = m_tempResults.find(id);
	if(it != m_tempResults.end())
		return it->second;

	return NULL;
}

bool ScriptEnviroment::getStorage(const uint32_t key, std::string& value) const
{
	StorageMap::const_iterator it = m_storageMap.find(key);
	if(it != m_storageMap.end())
	{
		value = it->second;
		return true;
	}

	value = "-1";
	return false;
}

void ScriptEnviroment::streamVariant(std::stringstream& stream, const std::string& local, const LuaVariant& var)
{
	if(!local.empty())
		stream << "local " << local << " = {" << std::endl;

	stream << "type = " << var.type;
	switch(var.type)
	{
		case VARIANT_NUMBER:
			stream << "," << std::endl << "number = " << var.number;
			break;
		case VARIANT_STRING:
			stream << "," << std::endl << "string = \"" << var.text << "\"";
			break;
		case VARIANT_TARGETPOSITION:
		case VARIANT_POSITION:
		{
			stream << "," << std::endl;
			streamPosition(stream, "pos", var.pos);
			break;
		}
		case VARIANT_NONE:
		default:
			break;
	}

	if(!local.empty())
		stream << std::endl << "}" << std::endl;
}

void ScriptEnviroment::streamThing(std::stringstream& stream, const std::string& local, Thing* thing, uint32_t id/* = 0*/)
{
	if(!local.empty())
		stream << "local " << local << " = {" << std::endl;

	if(thing && thing->getItem())
	{
		const Item* item = thing->getItem();
		if(!id)
			id = addThing(thing);

		stream << "uid = " << id << "," << std::endl;
		stream << "itemid = " << item->getID() << "," << std::endl;
		if(item->hasSubType())
			stream << "type = " << item->getSubType() << "," << std::endl;
		else
			stream << "type = 0," << std::endl;

		stream << "actionid = " << item->getActionId() << std::endl;
	}
	else if(thing && thing->getCreature())
	{
		const Creature* creature = thing->getCreature();
		if(!id)
			id = creature->getID();

		stream << "uid = " << id << "," << std::endl;
		stream << "itemid = 1," << std::endl;
		if(creature->getPlayer())
			stream << "type = 1," << std::endl;
		else if(creature->getMonster())
			stream << "type = 2," << std::endl;
		else
			stream << "type = 3," << std::endl;

		if(const Player* player = creature->getPlayer())
			stream << "actionid = " << player->getGUID() << "," << std::endl;
		else
			stream << "actionid = 0" << std::endl;
	}
	else
	{
		stream << "uid = 0," << std::endl;
		stream << "itemid = 0," << std::endl;
		stream << "type = 0," << std::endl;
		stream << "actionid = 0" << std::endl;
	}

	if(!local.empty())
		stream << "}" << std::endl;
}

void ScriptEnviroment::streamPosition(std::stringstream& stream, const std::string& local, const Position& position, uint32_t stackpos)
{
	if(!local.empty())
		stream << "local " << local << " = {" << std::endl;

	stream << "x = " << position.x << "," << std::endl;
	stream << "y = " << position.y << "," << std::endl;
	stream << "z = " << position.z << "," << std::endl;

	stream << "stackpos = " << stackpos << std::endl;
	if(!local.empty())
		stream << "}" << std::endl;
}

void ScriptEnviroment::streamOutfit(std::stringstream& stream, const std::string& local, const Outfit_t& outfit)
{
	if(!local.empty())
		stream << "local " << local << " = {" << std::endl;

	stream << "lookType = " << outfit.lookType << "," << std::endl;
	stream << "lookTypeEx = " << outfit.lookTypeEx << "," << std::endl;

	stream << "lookHead = " << outfit.lookHead << "," << std::endl;
	stream << "lookBody = " << outfit.lookBody << "," << std::endl;
	stream << "lookLegs = " << outfit.lookLegs << "," << std::endl;
	stream << "lookFeet = " << outfit.lookFeet << "," << std::endl;

	stream << "lookAddons = " << outfit.lookAddons << std::endl;
	if(!local.empty())
		stream << "}" << std::endl;
}

std::string LuaScriptInterface::getError(ErrorCode_t code)
{
	switch(code)
	{
		case LUA_ERROR_PLAYER_NOT_FOUND:
			return "Player not found";
		case LUA_ERROR_MONSTER_NOT_FOUND:
			return "Monster not found";
		case LUA_ERROR_NPC_NOT_FOUND:
			return "NPC not found";
		case LUA_ERROR_CREATURE_NOT_FOUND:
			return "Creature not found";
		case LUA_ERROR_ITEM_NOT_FOUND:
			return "Item not found";
		case LUA_ERROR_THING_NOT_FOUND:
			return "Thing not found";
		case LUA_ERROR_TILE_NOT_FOUND:
			return "Tile not found";
		case LUA_ERROR_HOUSE_NOT_FOUND:
			return "House not found";
		case LUA_ERROR_COMBAT_NOT_FOUND:
			return "Combat not found";
		case LUA_ERROR_CONDITION_NOT_FOUND:
			return "Condition not found";
		case LUA_ERROR_AREA_NOT_FOUND:
			return "Area not found";
		case LUA_ERROR_CONTAINER_NOT_FOUND:
			return "Container not found";
		case LUA_ERROR_VARIANT_NOT_FOUND:
			return "Variant not found";
		case LUA_ERROR_VARIANT_UNKNOWN:
			return "Unknown variant type";
		case LUA_ERROR_SPELL_NOT_FOUND:
			return "Spell not found";
		default:
			break;
	}

	return "Invalid error code!";
}

ScriptEnviroment LuaScriptInterface::m_scriptEnv[21];
int32_t LuaScriptInterface::m_scriptEnvIndex = -1;

LuaScriptInterface::LuaScriptInterface(std::string interfaceName)
{
	m_luaState = NULL;
	m_interfaceName = interfaceName;
	m_lastEventTimerId = 1000;
}

LuaScriptInterface::~LuaScriptInterface()
{
	closeState();
}

bool LuaScriptInterface::reInitState()
{
	closeState();
	return initState();
}

bool LuaScriptInterface::loadBuffer(const std::string& text, Npc* npc/* = NULL*/)
{
	//loads buffer as a chunk at stack top
	int32_t ret = luaL_loadbuffer(m_luaState, text.c_str(), text.length(), "loadBuffer");
	if(ret)
	{
		m_lastError = popString(m_luaState);
		error(NULL, m_lastError);
		return false;
	}

	//check that it is loaded as a function
	if(!lua_isfunction(m_luaState, -1))
		return false;

	m_loadingFile = "buffer";
	reserveEnv();

	ScriptEnviroment* env = getEnv();
	env->setScriptId(EVENT_ID_LOADING, this);
	env->setNpc(npc);

	//execute it
	ret = lua_pcall(m_luaState, 0, 0, 0);
	if(ret)
	{
		error(NULL, popString(m_luaState));
		releaseEnv();
		return false;
	}

	releaseEnv();
	return true;
}

bool LuaScriptInterface::loadFile(const std::string& file, Npc* npc/* = NULL*/)
{
	//loads file as a chunk at stack top
	int32_t ret = luaL_loadfile(m_luaState, file.c_str());
	if(ret)
	{
		m_lastError = popString(m_luaState);
		std::cout << "[Error - LuaScriptInterface::loadFile] " << m_lastError << std::endl;
		return false;
	}

	//check that it is loaded as a function
	if(!lua_isfunction(m_luaState, -1))
		return false;

	m_loadingFile = file;
	reserveEnv();

	ScriptEnviroment* env = getEnv();
	env->setScriptId(EVENT_ID_LOADING, this);
	env->setNpc(npc);

	//execute it
	ret = lua_pcall(m_luaState, 0, 0, 0);
	if(ret)
	{
		error(NULL, popString(m_luaState));
		releaseEnv();
		return false;
	}

	releaseEnv();
	return true;
}

bool LuaScriptInterface::loadDirectory(const std::string& dir, Npc* npc/* = NULL*/)
{
	StringVec files;
	for(boost::filesystem::directory_iterator it(dir), end; it != end; ++it)
	{
		std::string s = it->leaf();
		if(!boost::filesystem::is_directory(it->status()) && (s.size() > 4 ? s.substr(s.size() - 4) : "") == ".lua")
			files.push_back(s);
	}

	std::sort(files.begin(), files.end());
	for(StringVec::iterator it = files.begin(); it != files.end(); ++it)
	{
		if(!loadFile(dir + (*it), npc))
			return false;
	}

	return true;
}

int32_t LuaScriptInterface::getEvent(const std::string& eventName)
{
	//get our events table
	lua_getfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	if(!lua_istable(m_luaState, -1))
	{
		lua_pop(m_luaState, 1);
		return -1;
	}

	//get current event function pointer
	lua_getglobal(m_luaState, eventName.c_str());
	if(!lua_isfunction(m_luaState, -1))
	{
		lua_pop(m_luaState, 1);
		return -1;
	}

	//save in our events table
	lua_pushnumber(m_luaState, m_runningEventId);
	lua_pushvalue(m_luaState, -2);

	lua_rawset(m_luaState, -4);
	lua_pop(m_luaState, 2);

	//reset global value of this event
	lua_pushnil(m_luaState);
	lua_setglobal(m_luaState, eventName.c_str());

	m_cacheFiles[m_runningEventId] = m_loadingFile + ":" + eventName;
	++m_runningEventId;
	return m_runningEventId - 1;
}

std::string LuaScriptInterface::getScript(int32_t scriptId)
{
	const static std::string tmp = "(Unknown script file)";
	if(scriptId != EVENT_ID_LOADING)
	{
		ScriptsCache::iterator it = m_cacheFiles.find(scriptId);
		if(it != m_cacheFiles.end())
			return it->second;

		return tmp;
	}

	return m_loadingFile;
}

void LuaScriptInterface::error(const char* function, const std::string& desc)
{
	int32_t script, callback;
	bool timer;
	std::string event;

	LuaScriptInterface* interface;
	getEnv()->getInfo(script, event, interface, callback, timer);
	if(interface)
	{
		std::cout << std::endl << "[Error - " << interface->getName() << "] " << std::endl;
		if(callback)
			std::cout << "In a callback: " << interface->getScript(callback) << std::endl;

		if(timer)
			std::cout << (callback ? "from" : "In") << " a timer event called from: " << std::endl;

		std::cout << interface->getScript(script) << std::endl << "Description: ";
	}
	else
		std::cout << std::endl << "[Lua Error] ";

	std::cout << event << std::endl;
	if(function)
		std::cout << "(" << function << ") ";

	std::cout << desc << std::endl;
}

bool LuaScriptInterface::pushFunction(int32_t function)
{
	lua_getfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	if(lua_istable(m_luaState, -1))
	{
		lua_pushnumber(m_luaState, function);
		lua_rawget(m_luaState, -2);

		lua_remove(m_luaState, -2);
		if(lua_isfunction(m_luaState, -1))
			return true;
	}

	return false;
}

bool LuaScriptInterface::initState()
{
	m_luaState = luaL_newstate();
	if(!m_luaState)
		return false;

	luaL_openlibs(m_luaState);
	registerFunctions();
	if(!loadDirectory(getFilePath(FILE_TYPE_OTHER, "lib/"), NULL))
		std::cout << "[Warning - LuaScriptInterface::initState] Cannot load " << getFilePath(FILE_TYPE_OTHER, "lib/") << std::endl;

	lua_newtable(m_luaState);
	lua_setfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");

	m_runningEventId = EVENT_ID_USER;
	return true;
}

bool LuaScriptInterface::closeState()
{
	if(!m_luaState)
		return false;

	m_cacheFiles.clear();
	for(LuaTimerEvents::iterator it = m_timerEvents.begin(); it != m_timerEvents.end(); ++it)
	{
		for(std::list<int32_t>::iterator lt = it->second.parameters.begin(); lt != it->second.parameters.end(); ++lt)
			luaL_unref(m_luaState, LUA_REGISTRYINDEX, *lt);

		it->second.parameters.clear();
		luaL_unref(m_luaState, LUA_REGISTRYINDEX, it->second.function);
	}

	m_timerEvents.clear();
	lua_close(m_luaState);
	return true;
}

void LuaScriptInterface::executeTimer(uint32_t eventIndex)
{
	LuaTimerEvents::iterator it = m_timerEvents.find(eventIndex);
	if(it != m_timerEvents.end())
	{
		//push function
		lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, it->second.function);

		//push parameters
		for(std::list<int32_t>::reverse_iterator rt = it->second.parameters.rbegin(); rt != it->second.parameters.rend(); ++rt)
			lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, *rt);

		//call the function
		if(reserveEnv())
		{
			ScriptEnviroment* env = getEnv();
			env->setTimerEvent();
			env->setScriptId(it->second.scriptId, this);

			callFunction(it->second.parameters.size());
			releaseEnv();
		}
		else
			std::cout << "[Error] Call stack overflow. LuaScriptInterface::executeTimer" << std::endl;

		//free resources
		for(std::list<int32_t>::iterator lt = it->second.parameters.begin(); lt != it->second.parameters.end(); ++lt)
			luaL_unref(m_luaState, LUA_REGISTRYINDEX, *lt);

		it->second.parameters.clear();
		luaL_unref(m_luaState, LUA_REGISTRYINDEX, it->second.function);
		m_timerEvents.erase(it);
	}
}

int32_t LuaScriptInterface::handleFunction(lua_State* L)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if(!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return 1;
	}

	lua_getfield(L, -1, "traceback");
	if(!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return 1;
	}

	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);

	lua_call(L, 2, 1);
	return 1;
}

bool LuaScriptInterface::callFunction(uint32_t params)
{
	int32_t size = lua_gettop(m_luaState), handler = lua_gettop(m_luaState) - params;
	lua_pushcfunction(m_luaState, handleFunction);

	bool result = false;
	lua_insert(m_luaState, handler);
	if(lua_pcall(m_luaState, params, 1, handler))
		LuaScriptInterface::error(NULL, LuaScriptInterface::popString(m_luaState));
	else
		result = (int32_t)LuaScriptInterface::popBoolean(m_luaState);

	lua_remove(m_luaState, handler);
	if((lua_gettop(m_luaState) + (int32_t)params + 1) != size)
		LuaScriptInterface::error(NULL, "Stack size changed!");

	return result;
}

void LuaScriptInterface::dumpStack(lua_State* L/* = NULL*/)
{
	if(!L)
		L = m_luaState;

	int32_t stack = lua_gettop(L);
	if(!stack)
		return;

	std::cout << "Stack size: " << stack << std::endl;
	for(int32_t i = 1; i <= stack ; ++i)
		std::cout << lua_typename(m_luaState, lua_type(m_luaState, -i)) << " " << lua_topointer(m_luaState, -i) << std::endl;
}

void LuaScriptInterface::pushVariant(lua_State* L, const LuaVariant& var)
{
	lua_newtable(L);
	setField(L, "type", var.type);
	switch(var.type)
	{
		case VARIANT_NUMBER:
			setField(L, "number", var.number);
			break;
		case VARIANT_STRING:
			setField(L, "string", var.text);
			break;
		case VARIANT_TARGETPOSITION:
		case VARIANT_POSITION:
		{
			lua_pushstring(L, "pos");
			pushPosition(L, var.pos);
			pushTable(L);
			break;
		}
		case VARIANT_NONE:
			break;
	}
}

void LuaScriptInterface::pushThing(lua_State* L, Thing* thing, uint32_t id/* = 0*/)
{
	lua_newtable(L);
	if(thing && thing->getItem())
	{
		const Item* item = thing->getItem();
		if(!id)
			id = getEnv()->addThing(thing);

		setField(L, "uid", id);
		setField(L, "itemid", item->getID());
		if(item->hasSubType())
			setField(L, "type", item->getSubType());
		else
			setField(L, "type", 0);

		setField(L, "actionid", item->getActionId());
	}
	else if(thing && thing->getCreature())
	{
		const Creature* creature = thing->getCreature();
		if(!id)
			id = creature->getID();

		setField(L, "uid", id);
		setField(L, "itemid", 1);
		if(creature->getPlayer())
			setField(L, "type", 1);
		else if(creature->getMonster())
			setField(L, "type", 2);
		else
			setField(L, "type", 3);

		if(const Player* player = creature->getPlayer())
			setField(L, "actionid", player->getGUID());
		else
			setField(L, "actionid", 0);
	}
	else
	{
		setField(L, "uid", 0);
		setField(L, "itemid", 0);
		setField(L, "type", 0);
		setField(L, "actionid", 0);
	}
}

void LuaScriptInterface::pushPosition(lua_State* L, const Position& position, uint32_t stackpos)
{
	lua_newtable(L);
	setField(L, "x", position.x);
	setField(L, "y", position.y);
	setField(L, "z", position.z);
	setField(L, "stackpos", stackpos);
}

void LuaScriptInterface::pushOutfit(lua_State* L, const Outfit_t& outfit)
{
	lua_newtable(L);
	setField(L, "lookType", outfit.lookType);
	setField(L, "lookTypeEx", outfit.lookTypeEx);
	setField(L, "lookHead", outfit.lookHead);
	setField(L, "lookBody", outfit.lookBody);
	setField(L, "lookLegs", outfit.lookLegs);
	setField(L, "lookFeet", outfit.lookFeet);
	setField(L, "lookAddons", outfit.lookAddons);
}

void LuaScriptInterface::pushCallback(lua_State* L, int32_t callback)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
}

LuaVariant LuaScriptInterface::popVariant(lua_State* L)
{
	LuaVariant var;
	var.type = (LuaVariantType_t)getField(L, "type");
	switch(var.type)
	{
		case VARIANT_NUMBER:
			var.number = getFieldUnsigned(L, "number");
			break;
		case VARIANT_STRING:
			var.text = getField(L, "string");
			break;
		case VARIANT_POSITION:
		case VARIANT_TARGETPOSITION:
		{
			lua_pushstring(L, "pos");
			lua_gettable(L, -2);
			popPosition(L, var.pos);
			break;
		}
		default:
			var.type = VARIANT_NONE;
			break;
	}

	lua_pop(L, 1); //table
	return var;
}

void LuaScriptInterface::popPosition(lua_State* L, PositionEx& position)
{
	if(!lua_isboolean(L, -1))
	{
		position.x = getField(L, "x");
		position.y = getField(L, "y");
		position.z = getField(L, "z");
		position.stackpos = getField(L, "stackpos");
	}
	else
		position = PositionEx();

	lua_pop(L, 1); //table
}

void LuaScriptInterface::popPosition(lua_State* L, Position& position, uint32_t& stackpos)
{
	stackpos = 0;
	if(!lua_isboolean(L, -1))
	{
		position.x = getField(L, "x");
		position.y = getField(L, "y");
		position.z = getField(L, "z");
		stackpos = getField(L, "stackpos");
	}
	else
		position = Position();

	lua_pop(L, 1); //table
}

bool LuaScriptInterface::popBoolean(lua_State* L)
{
	lua_pop(L, 1);
	return lua_toboolean(L, 0);
}

int64_t LuaScriptInterface::popNumber(lua_State* L)
{
	lua_pop(L, 1);
	if(lua_isboolean(L, 0))
		return (int64_t)lua_toboolean(L, 0);

	return (int64_t)lua_tonumber(L, 0);
}

double LuaScriptInterface::popFloatNumber(lua_State* L)
{
	lua_pop(L, 1);
	return lua_tonumber(L, 0);
}

std::string LuaScriptInterface::popString(lua_State* L)
{
	lua_pop(L, 1);
	const char* str = lua_tostring(L, 0);
	if(!str || !strlen(str))
		return std::string();

	return str;
}

int32_t LuaScriptInterface::popCallback(lua_State* L)
{
	return luaL_ref(L, LUA_REGISTRYINDEX);
}

Outfit_t LuaScriptInterface::popOutfit(lua_State* L)
{
	Outfit_t outfit;
	outfit.lookAddons = getField(L, "lookAddons");

	outfit.lookFeet = getField(L, "lookFeet");
	outfit.lookLegs = getField(L, "lookLegs");
	outfit.lookBody = getField(L, "lookBody");
	outfit.lookHead = getField(L, "lookHead");

	outfit.lookTypeEx = getField(L, "lookTypeEx");
	outfit.lookType = getField(L, "lookType");

	lua_pop(L, 1); //table
	return outfit;
}

void LuaScriptInterface::setField(lua_State* L, const char* index, int32_t val)
{
	lua_pushstring(L, index);
	lua_pushnumber(L, val);
	pushTable(L);
}

void LuaScriptInterface::setField(lua_State* L, const char* index, const std::string& val)
{
	lua_pushstring(L, index);
	lua_pushstring(L, val.c_str());
	pushTable(L);
}

void LuaScriptInterface::setFieldBool(lua_State* L, const char* index, bool val)
{
	lua_pushstring(L, index);
	lua_pushboolean(L, val);
	pushTable(L);
}

void LuaScriptInterface::setFieldFloat(lua_State* L, const char* index, double val)
{
	lua_pushstring(L, index);
	lua_pushnumber(L, val);
	pushTable(L);
}

void LuaScriptInterface::createTable(lua_State* L, const char* index)
{
	lua_pushstring(L, index);
	lua_newtable(L);
}

void LuaScriptInterface::createTable(lua_State* L, const char* index, int32_t narr, int32_t nrec)
{
	lua_pushstring(L, index);
	lua_createtable(L, narr, nrec);
}

void LuaScriptInterface::createTable(lua_State* L, int32_t index)
{
	lua_pushnumber(L, index);
	lua_newtable(L);
}

void LuaScriptInterface::createTable(lua_State* L, int32_t index, int32_t narr, int32_t nrec)
{
	lua_pushnumber(L, index);
	lua_createtable(L, narr, nrec);
}

void LuaScriptInterface::pushTable(lua_State* L)
{
	lua_settable(L, -3);
}

int64_t LuaScriptInterface::getField(lua_State* L, const char* key)
{
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]

	int64_t result = (int64_t)lua_tonumber(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

uint64_t LuaScriptInterface::getFieldUnsigned(lua_State* L, const char* key)
{
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]

	uint64_t result = (uint64_t)lua_tonumber(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

bool LuaScriptInterface::getFieldBool(lua_State* L, const char* key)
{
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]

	bool result = lua_toboolean(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

std::string LuaScriptInterface::getFieldString(lua_State* L, const char* key)
{
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]

	std::string result = lua_tostring(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

std::string LuaScriptInterface::getGlobalString(lua_State* L, const std::string& _identifier, const std::string& _default/* = ""*/)
{
	lua_getglobal(L, _identifier.c_str());
	if(!lua_isstring(L, -1))
	{
		lua_pop(L, 1);
		return _default;
	}

	int32_t len = (int32_t)lua_strlen(L, -1);
	std::string ret(lua_tostring(L, -1), len);

	lua_pop(L, 1);
	return ret;
}

bool LuaScriptInterface::getGlobalBool(lua_State* L, const std::string& _identifier, bool _default/* = false*/)
{
	lua_getglobal(L, _identifier.c_str());
	if(!lua_isboolean(L, -1))
	{
		lua_pop(L, 1);
		return booleanString(LuaScriptInterface::getGlobalString(L, _identifier, _default ? "yes" : "no"));
	}

	bool val = lua_toboolean(L, -1);
	lua_pop(L, 1);
	return val;
}

int32_t LuaScriptInterface::getGlobalNumber(lua_State* L, const std::string& _identifier, const int32_t _default/* = 0*/)
{
	return (int32_t)LuaScriptInterface::getGlobalDouble(L, _identifier, _default);
}

double LuaScriptInterface::getGlobalDouble(lua_State* L, const std::string& _identifier, const double _default/* = 0*/)
{
	lua_getglobal(L, _identifier.c_str());
	if(!lua_isnumber(L, -1))
	{
		lua_pop(L, 1);
		return _default;
	}

	double val = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return val;
}

void LuaScriptInterface::getValue(const std::string& key, lua_State* L, lua_State* _L)
{
	lua_getglobal(L, key.c_str());
	moveValue(L, _L);
}

void LuaScriptInterface::moveValue(lua_State* from, lua_State* to)
{
	switch(lua_type(from, -1))
	{
		case LUA_TNIL:
			lua_pushnil(to);
			break;
		case LUA_TBOOLEAN:
			lua_pushboolean(to, lua_toboolean(from, -1));
			break;
		case LUA_TNUMBER:
			lua_pushnumber(to, lua_tonumber(from, -1));
			break;
		case LUA_TSTRING:
		{
			size_t len;
			const char* str = lua_tolstring(from, -1, &len);

			lua_pushlstring(to, str, len);
			break;
		}
		case LUA_TTABLE:
		{
			lua_newtable(to);
			lua_pushnil(from); // First key
			while(lua_next(from, -2))
			{
				// Move value to the other state
				moveValue(from, to); // Value is popped, key is left
				// Move key to the other state
				lua_pushvalue(from, -1); // Make a copy of the key to use for the next iteration
				moveValue(from, to); // Key is in other state.
				// We still have the key in the 'from' state ontop of the stack

				lua_insert(to, -2); // Move key above value
				pushTable(to); // Set the key
			}

			break;
		}
		default:
			break;
	}

	lua_pop(from, 1); // Pop the value we just read
}

void LuaScriptInterface::registerFunctions()
{
	//example(...)
	//lua_register(L, "name", C_function);

	//getCreatureHealth(cid)
	lua_register(m_luaState, "getCreatureHealth", LuaScriptInterface::luaGetCreatureHealth);

	//getCreatureMaxHealth(cid)
	lua_register(m_luaState, "getCreatureMaxHealth", LuaScriptInterface::luaGetCreatureMaxHealth);

	//getCreatureMana(cid)
	lua_register(m_luaState, "getCreatureMana", LuaScriptInterface::luaGetCreatureMana);

	//getCreatureMaxMana(cid)
	lua_register(m_luaState, "getCreatureMaxMana", LuaScriptInterface::luaGetCreatureMaxMana);

	//getCreatureHideHealth(cid)
	lua_register(m_luaState, "getCreatureHideHealth", LuaScriptInterface::luaGetCreatureHideHealth);

	//doCreatureSetHideHealth(cid, hide)
	lua_register(m_luaState, "doCreatureSetHideHealth", LuaScriptInterface::luaDoCreatureSetHideHealth);

	//getCreatureSpeakType(cid)
	lua_register(m_luaState, "getCreatureSpeakType", LuaScriptInterface::luaGetCreatureSpeakType);

	//doCreatureSetSpeakType(cid, type)
	lua_register(m_luaState, "doCreatureSetSpeakType", LuaScriptInterface::luaDoCreatureSetSpeakType);

	//getCreatureLookDirection(cid)
	lua_register(m_luaState, "getCreatureLookDirection", LuaScriptInterface::luaGetCreatureLookDirection);

	//getPlayerLevel(cid)
	lua_register(m_luaState, "getPlayerLevel", LuaScriptInterface::luaGetPlayerLevel);

	//getPlayerExperience(cid)
	lua_register(m_luaState, "getPlayerExperience", LuaScriptInterface::luaGetPlayerExperience);

	//getPlayerMagLevel(cid[, ignoreBuffs = false])
	lua_register(m_luaState, "getPlayerMagLevel", LuaScriptInterface::luaGetPlayerMagLevel);

	//getPlayerSpentMana(cid)
	lua_register(m_luaState, "getPlayerSpentMana", LuaScriptInterface::luaGetPlayerSpentMana);

	//getPlayerFood(cid)
	lua_register(m_luaState, "getPlayerFood", LuaScriptInterface::luaGetPlayerFood);

	//getPlayerAccess(cid)
	lua_register(m_luaState, "getPlayerAccess", LuaScriptInterface::luaGetPlayerAccess);

	//getPlayerGhostAccess(cid)
	lua_register(m_luaState, "getPlayerGhostAccess", LuaScriptInterface::luaGetPlayerGhostAccess);

	//getPlayerSkillLevel(cid, skillid)
	lua_register(m_luaState, "getPlayerSkillLevel", LuaScriptInterface::luaGetPlayerSkillLevel);

	//getPlayerSkillTries(cid, skillid)
	lua_register(m_luaState, "getPlayerSkillTries", LuaScriptInterface::luaGetPlayerSkillTries);

	//getPlayerTown(cid)
	lua_register(m_luaState, "getPlayerTown", LuaScriptInterface::luaGetPlayerTown);

	//getPlayerVocation(cid)
	lua_register(m_luaState, "getPlayerVocation", LuaScriptInterface::luaGetPlayerVocation);

	//getPlayerIp(cid)
	lua_register(m_luaState, "getPlayerIp", LuaScriptInterface::luaGetPlayerIp);

	//getPlayerRequiredMana(cid, magicLevel)
	lua_register(m_luaState, "getPlayerRequiredMana", LuaScriptInterface::luaGetPlayerRequiredMana);

	//getPlayerRequiredSkillTries(cid, skillId, skillLevel)
	lua_register(m_luaState, "getPlayerRequiredSkillTries", LuaScriptInterface::luaGetPlayerRequiredSkillTries);

	//getPlayerItemCount(cid, itemid[, subType = -1])
	lua_register(m_luaState, "getPlayerItemCount", LuaScriptInterface::luaGetPlayerItemCount);

	//getPlayerMoney(cid)
	lua_register(m_luaState, "getPlayerMoney", LuaScriptInterface::luaGetPlayerMoney);

	//getPlayerSoul(cid)
	lua_register(m_luaState, "getPlayerSoul", LuaScriptInterface::luaGetPlayerSoul);

	//getPlayerFreeCap(cid)
	lua_register(m_luaState, "getPlayerFreeCap", LuaScriptInterface::luaGetPlayerFreeCap);

	//getPlayerLight(cid)
	lua_register(m_luaState, "getPlayerLight", LuaScriptInterface::luaGetPlayerLight);

	//getPlayerSlotItem(cid, slot)
	lua_register(m_luaState, "getPlayerSlotItem", LuaScriptInterface::luaGetPlayerSlotItem);

	//getPlayerWeapon(cid[, ignoreAmmo = false])
	lua_register(m_luaState, "getPlayerWeapon", LuaScriptInterface::luaGetPlayerWeapon);

	//getPlayerItemById(cid, deepSearch, itemId[, subType = -1])
	lua_register(m_luaState, "getPlayerItemById", LuaScriptInterface::luaGetPlayerItemById);

	//getPlayerDepotItems(cid, depotid)
	lua_register(m_luaState, "getPlayerDepotItems", LuaScriptInterface::luaGetPlayerDepotItems);

	//getPlayerGuildId(cid)
	lua_register(m_luaState, "getPlayerGuildId", LuaScriptInterface::luaGetPlayerGuildId);

	//getPlayerGuildName(cid)
	lua_register(m_luaState, "getPlayerGuildName", LuaScriptInterface::luaGetPlayerGuildName);

	//getPlayerGuildRankId(cid)
	lua_register(m_luaState, "getPlayerGuildRankId", LuaScriptInterface::luaGetPlayerGuildRankId);

	//getPlayerGuildRank(cid)
	lua_register(m_luaState, "getPlayerGuildRank", LuaScriptInterface::luaGetPlayerGuildRank);

	//getPlayerGuildNick(cid)
	lua_register(m_luaState, "getPlayerGuildNick", LuaScriptInterface::luaGetPlayerGuildNick);

	//getPlayerGuildLevel(cid)
	lua_register(m_luaState, "getPlayerGuildLevel", LuaScriptInterface::luaGetPlayerGuildLevel);

	//getPlayerGUID(cid)
	lua_register(m_luaState, "getPlayerGUID", LuaScriptInterface::luaGetPlayerGUID);

	//getPlayerNameDescription(cid)
	lua_register(m_luaState, "getPlayerNameDescription", LuaScriptInterface::luaGetPlayerNameDescription);

	//doPlayerSetNameDescription(cid, desc)
	lua_register(m_luaState, "doPlayerSetNameDescription", LuaScriptInterface::luaDoPlayerSetNameDescription);

	//getPlayerSpecialDescription(cid)
	lua_register(m_luaState, "getPlayerSpecialDescription", LuaScriptInterface::luaGetPlayerSpecialDescription);

	//doPlayerSetSpecialDescription(cid, desc)
	lua_register(m_luaState, "doPlayerSetSpecialDescription", LuaScriptInterface::luaDoPlayerSetSpecialDescription);

	//getPlayerAccountId(cid)
	lua_register(m_luaState, "getPlayerAccountId", LuaScriptInterface::luaGetPlayerAccountId);

	//getPlayerAccount(cid)
	lua_register(m_luaState, "getPlayerAccount", LuaScriptInterface::luaGetPlayerAccount);

	//getPlayerFlagValue(cid, flag)
	lua_register(m_luaState, "getPlayerFlagValue", LuaScriptInterface::luaGetPlayerFlagValue);

	//getPlayerCustomFlagValue(cid, flag)
	lua_register(m_luaState, "getPlayerCustomFlagValue", LuaScriptInterface::luaGetPlayerCustomFlagValue);

	//getPlayerPromotionLevel(cid)
	lua_register(m_luaState, "getPlayerPromotionLevel", LuaScriptInterface::luaGetPlayerPromotionLevel);

	//doPlayerSetPromotionLevel(cid, level)
	lua_register(m_luaState, "doPlayerSetPromotionLevel", LuaScriptInterface::luaDoPlayerSetPromotionLevel);

	//getPlayerGroupId(cid)
	lua_register(m_luaState, "getPlayerGroupId", LuaScriptInterface::luaGetPlayerGroupId);

	//doPlayerSetGroupId(cid, newGroupId)
	lua_register(m_luaState, "doPlayerSetGroupId", LuaScriptInterface::luaDoPlayerSetGroupId);

	//doPlayerSendOutfitWindow(cid)
	lua_register(m_luaState, "doPlayerSendOutfitWindow", LuaScriptInterface::luaDoPlayerSendOutfitWindow);

	//doPlayerLearnInstantSpell(cid, name)
	lua_register(m_luaState, "doPlayerLearnInstantSpell", LuaScriptInterface::luaDoPlayerLearnInstantSpell);

	//doPlayerUnlearnInstantSpell(cid, name)
	lua_register(m_luaState, "doPlayerUnlearnInstantSpell", LuaScriptInterface::luaDoPlayerUnlearnInstantSpell);

	//getPlayerLearnedInstantSpell(cid, name)
	lua_register(m_luaState, "getPlayerLearnedInstantSpell", LuaScriptInterface::luaGetPlayerLearnedInstantSpell);

	//getPlayerInstantSpellCount(cid)
	lua_register(m_luaState, "getPlayerInstantSpellCount", LuaScriptInterface::luaGetPlayerInstantSpellCount);

	//getPlayerInstantSpellInfo(cid, index)
	lua_register(m_luaState, "getPlayerInstantSpellInfo", LuaScriptInterface::luaGetPlayerInstantSpellInfo);

	//getInstantSpellInfo(cid, name)
	lua_register(m_luaState, "getInstantSpellInfo", LuaScriptInterface::luaGetInstantSpellInfo);

	//getCreatureStorage(uid, key)
	lua_register(m_luaState, "getCreatureStorage", LuaScriptInterface::luaGetCreatureStorage);

	//doCreatureSetStorage(uid, key, value)
	lua_register(m_luaState, "doCreatureSetStorage", LuaScriptInterface::luaDoCreatureSetStorage);

	//getStorage(key)
	lua_register(m_luaState, "getStorage", LuaScriptInterface::luaGetStorage);

	//doSetStorage(key, value)
	lua_register(m_luaState, "doSetStorage", LuaScriptInterface::luaDoSetStorage);

	//getChannelUsers(channelId)
	lua_register(m_luaState, "getChannelUsers", LuaScriptInterface::luaGetChannelUsers);

	//getPlayersOnline()
	lua_register(m_luaState, "getPlayersOnline", LuaScriptInterface::luaGetPlayersOnline);

	//getTileInfo(pos)
	lua_register(m_luaState, "getTileInfo", LuaScriptInterface::luaGetTileInfo);

	//getThingFromPos(pos[, displayError = true])
	lua_register(m_luaState, "getThingFromPos", LuaScriptInterface::luaGetThingFromPos);

	//getThing(uid)
	lua_register(m_luaState, "getThing", LuaScriptInterface::luaGetThing);

	//doTileQueryAdd(uid, pos[, flags[, displayError = true]])
	lua_register(m_luaState, "doTileQueryAdd", LuaScriptInterface::luaDoTileQueryAdd);

	//doItemRaidUnref(uid)
	lua_register(m_luaState, "doItemRaidUnref", LuaScriptInterface::luaDoItemRaidUnref);

	//getThingPosition(uid)
	lua_register(m_luaState, "getThingPosition", LuaScriptInterface::luaGetThingPosition);

	//getTileItemById(pos, itemId[, subType = -1])
	lua_register(m_luaState, "getTileItemById", LuaScriptInterface::luaGetTileItemById);

	//getTileItemByType(pos, type)
	lua_register(m_luaState, "getTileItemByType", LuaScriptInterface::luaGetTileItemByType);

	//getTileThingByPos(pos)
	lua_register(m_luaState, "getTileThingByPos", LuaScriptInterface::luaGetTileThingByPos);

	//getTopCreature(pos)
	lua_register(m_luaState, "getTopCreature", LuaScriptInterface::luaGetTopCreature);

	//doRemoveItem(uid[, count])
	lua_register(m_luaState, "doRemoveItem", LuaScriptInterface::luaDoRemoveItem);

	//doPlayerFeed(cid, food)
	lua_register(m_luaState, "doPlayerFeed", LuaScriptInterface::luaDoFeedPlayer);

	//doPlayerSendCancel(cid, text)
	lua_register(m_luaState, "doPlayerSendCancel", LuaScriptInterface::luaDoPlayerSendCancel);

	//doPlayerSendDefaultCancel(cid, ReturnValue)
	lua_register(m_luaState, "doPlayerSendDefaultCancel", LuaScriptInterface::luaDoSendDefaultCancel);

	//getSearchString(fromPosition, toPosition[, fromIsCreature = false[, toIsCreature = false]])
	lua_register(m_luaState, "getSearchString", LuaScriptInterface::luaGetSearchString);

	//getClosestFreeTile(cid, targetpos[, extended = false[, ignoreHouse = true]])
	lua_register(m_luaState, "getClosestFreeTile", LuaScriptInterface::luaGetClosestFreeTile);

	//doTeleportThing(cid, newpos[, pushmove])
	lua_register(m_luaState, "doTeleportThing", LuaScriptInterface::luaDoTeleportThing);

	//doTransformItem(uid, newId[, count/subType])
	lua_register(m_luaState, "doTransformItem", LuaScriptInterface::luaDoTransformItem);

	//doCreatureSay(uid, text[, type = SPEAK_SAY[, ghost = false[, cid = 0[, pos]]]])
	lua_register(m_luaState, "doCreatureSay", LuaScriptInterface::luaDoCreatureSay);

	//doSendMagicEffect(pos, type[, player])
	lua_register(m_luaState, "doSendMagicEffect", LuaScriptInterface::luaDoSendMagicEffect);

	//doSendDistanceShoot(fromPos, toPos, type[, player])
	lua_register(m_luaState, "doSendDistanceShoot", LuaScriptInterface::luaDoSendDistanceShoot);

	//doSendAnimatedText(pos, text, color[, player])
	lua_register(m_luaState, "doSendAnimatedText", LuaScriptInterface::luaDoSendAnimatedText);

	//doPlayerAddSkillTry(cid, skillid, n[, useMultiplier])
	lua_register(m_luaState, "doPlayerAddSkillTry", LuaScriptInterface::luaDoPlayerAddSkillTry);

	//doCreatureAddHealth(cid, health[, hitEffect[, hitColor[, force]]])
	lua_register(m_luaState, "doCreatureAddHealth", LuaScriptInterface::luaDoCreatureAddHealth);

	//doCreatureAddMana(cid, mana)
	lua_register(m_luaState, "doCreatureAddMana", LuaScriptInterface::luaDoCreatureAddMana);

	//setCreatureMaxHealth(cid, health)
	lua_register(m_luaState, "setCreatureMaxHealth", LuaScriptInterface::luaSetCreatureMaxHealth);

	//setCreatureMaxMana(cid, mana)
	lua_register(m_luaState, "setCreatureMaxMana", LuaScriptInterface::luaSetCreatureMaxMana);

	//doPlayerSetMaxCapacity(cid, cap)
	lua_register(m_luaState, "doPlayerSetMaxCapacity", LuaScriptInterface::luaDoPlayerSetMaxCapacity);

	//doPlayerAddSpentMana(cid, amount[, useMultiplier])
	lua_register(m_luaState, "doPlayerAddSpentMana", LuaScriptInterface::luaDoPlayerAddSpentMana);

	//doPlayerAddSoul(cid, soul)
	lua_register(m_luaState, "doPlayerAddSoul", LuaScriptInterface::luaDoPlayerAddSoul);

	//doPlayerAddItem(cid, itemid[, count/subtype[, canDropOnMap]])
	//doPlayerAddItem(cid, itemid[, count[, canDropOnMap[, subtype]]])
	//Returns uid of the created item
	lua_register(m_luaState, "doPlayerAddItem", LuaScriptInterface::luaDoPlayerAddItem);

	//doPlayerAddItemEx(cid, uid[, canDropOnMap = FALSE])
	lua_register(m_luaState, "doPlayerAddItemEx", LuaScriptInterface::luaDoPlayerAddItemEx);

	//doPlayerSendTextMessage(cid, MessageClasses, message)
	lua_register(m_luaState, "doPlayerSendTextMessage", LuaScriptInterface::luaDoPlayerSendTextMessage);

	//doPlayerSendChannelMessage(cid, author, message, SpeakClasses, channel)
	lua_register(m_luaState, "doPlayerSendChannelMessage", LuaScriptInterface::luaDoPlayerSendChannelMessage);

	//doPlayerSendToChannel(cid, targetId, SpeakClasses, message, channel[, time])
	lua_register(m_luaState, "doPlayerSendToChannel", LuaScriptInterface::luaDoPlayerSendToChannel);

	//doPlayerAddMoney(cid, money)
	lua_register(m_luaState, "doPlayerAddMoney", LuaScriptInterface::luaDoPlayerAddMoney);

	//doPlayerRemoveMoney(cid, money)
	lua_register(m_luaState, "doPlayerRemoveMoney", LuaScriptInterface::luaDoPlayerRemoveMoney);

	//doPlayerTransferMoneyTo(cid, target, money)
	lua_register(m_luaState, "doPlayerTransferMoneyTo", LuaScriptInterface::luaDoPlayerTransferMoneyTo);

	//doShowTextDialog(cid, itemid, text)
	lua_register(m_luaState, "doShowTextDialog", LuaScriptInterface::luaDoShowTextDialog);

	//doDecayItem(uid)
	lua_register(m_luaState, "doDecayItem", LuaScriptInterface::luaDoDecayItem);

	//doCreateItem(itemid[, type/count], pos)
	//Returns uid of the created item, only works on tiles.
	lua_register(m_luaState, "doCreateItem", LuaScriptInterface::luaDoCreateItem);

	//doCreateItemEx(itemid[, count/subType = -1])
	lua_register(m_luaState, "doCreateItemEx", LuaScriptInterface::luaDoCreateItemEx);

	//doTileAddItemEx(pos, uid)
	lua_register(m_luaState, "doTileAddItemEx", LuaScriptInterface::luaDoTileAddItemEx);

	//doAddContainerItemEx(uid, virtuid)
	lua_register(m_luaState, "doAddContainerItemEx", LuaScriptInterface::luaDoAddContainerItemEx);

	//doRelocate(pos, posTo[, creatures = true])
	//Moves all moveable objects from pos to posTo
	lua_register(m_luaState, "doRelocate", LuaScriptInterface::luaDoRelocate);

	//doCleanTile(pos[, forceMapLoaded = false])
	lua_register(m_luaState, "doCleanTile", LuaScriptInterface::luaDoCleanTile);

	//doCreateTeleport(itemid, topos, createpos)
	lua_register(m_luaState, "doCreateTeleport", LuaScriptInterface::luaDoCreateTeleport);

	//doCreateMonster(name, pos[, displayError = true])
	lua_register(m_luaState, "doCreateMonster", LuaScriptInterface::luaDoCreateMonster);

	//doCreateNpc(name, pos[, displayError = true])
	lua_register(m_luaState, "doCreateNpc", LuaScriptInterface::luaDoCreateNpc);

	//doSummonMonster(cid, name)
	lua_register(m_luaState, "doSummonMonster", LuaScriptInterface::luaDoSummonMonster);

	//doConvinceCreature(cid, target)
	lua_register(m_luaState, "doConvinceCreature", LuaScriptInterface::luaDoConvinceCreature);

	//getMonsterTargetList(cid)
	lua_register(m_luaState, "getMonsterTargetList", LuaScriptInterface::luaGetMonsterTargetList);

	//getMonsterFriendList(cid)
	lua_register(m_luaState, "getMonsterFriendList", LuaScriptInterface::luaGetMonsterFriendList);

	//doMonsterSetTarget(cid, target)
	lua_register(m_luaState, "doMonsterSetTarget", LuaScriptInterface::luaDoMonsterSetTarget);

	//doMonsterChangeTarget(cid)
	lua_register(m_luaState, "doMonsterChangeTarget", LuaScriptInterface::luaDoMonsterChangeTarget);

	//getMonsterInfo(name)
	lua_register(m_luaState, "getMonsterInfo", LuaScriptInterface::luaGetMonsterInfo);

	//doAddCondition(cid, condition)
	lua_register(m_luaState, "doAddCondition", LuaScriptInterface::luaDoAddCondition);

	//doRemoveCondition(cid, type[, subId])
	lua_register(m_luaState, "doRemoveCondition", LuaScriptInterface::luaDoRemoveCondition);

	//doRemoveConditions(cid[, onlyPersistent])
	lua_register(m_luaState, "doRemoveConditions", LuaScriptInterface::luaDoRemoveConditions);

	//doRemoveCreature(cid[, forceLogout = true])
	lua_register(m_luaState, "doRemoveCreature", LuaScriptInterface::luaDoRemoveCreature);

	//doMoveCreature(cid, direction)
	lua_register(m_luaState, "doMoveCreature", LuaScriptInterface::luaDoMoveCreature);

	//doPlayerSetPzLocked(cid, locked)
	lua_register(m_luaState, "doPlayerSetPzLocked", LuaScriptInterface::luaDoPlayerSetPzLocked);

	//doPlayerSetTown(cid, townid)
	lua_register(m_luaState, "doPlayerSetTown", LuaScriptInterface::luaDoPlayerSetTown);

	//doPlayerSetVocation(cid,voc)
	lua_register(m_luaState, "doPlayerSetVocation", LuaScriptInterface::luaDoPlayerSetVocation);

	//doPlayerRemoveItem(cid, itemid[, count[, subType]])
	lua_register(m_luaState, "doPlayerRemoveItem", LuaScriptInterface::luaDoPlayerRemoveItem);

	//doPlayerAddExperience(cid, amount)
	lua_register(m_luaState, "doPlayerAddExperience", LuaScriptInterface::luaDoPlayerAddExperience);

	//doPlayerSetGuildId(cid, id)
	lua_register(m_luaState, "doPlayerSetGuildId", LuaScriptInterface::luaDoPlayerSetGuildId);

	//doPlayerSetGuildLevel(cid, level[, rank])
	lua_register(m_luaState, "doPlayerSetGuildLevel", LuaScriptInterface::luaDoPlayerSetGuildLevel);

	//doPlayerSetGuildNick(cid, nick)
	lua_register(m_luaState, "doPlayerSetGuildNick", LuaScriptInterface::luaDoPlayerSetGuildNick);

	//doPlayerAddOutfit(cid, looktype, addon)
	lua_register(m_luaState, "doPlayerAddOutfit", LuaScriptInterface::luaDoPlayerAddOutfit);

	//doPlayerRemoveOutfit(cid, looktype[, addon = 0])
	lua_register(m_luaState, "doPlayerRemoveOutfit", LuaScriptInterface::luaDoPlayerRemoveOutfit);

	//doPlayerAddOutfitId(cid, outfitId, addon)
	lua_register(m_luaState, "doPlayerAddOutfitId", LuaScriptInterface::luaDoPlayerAddOutfitId);

	//doPlayerRemoveOutfitId(cid, outfitId[, addon = 0])
	lua_register(m_luaState, "doPlayerRemoveOutfitId", LuaScriptInterface::luaDoPlayerRemoveOutfitId);

	//canPlayerWearOutfit(cid, looktype[, addon = 0])
	lua_register(m_luaState, "canPlayerWearOutfit", LuaScriptInterface::luaCanPlayerWearOutfit);

	//canPlayerWearOutfitId(cid, outfitId[, addon = 0])
	lua_register(m_luaState, "canPlayerWearOutfitId", LuaScriptInterface::luaCanPlayerWearOutfitId);

	//doSetCreatureLight(cid, lightLevel, lightColor, time)
	lua_register(m_luaState, "doSetCreatureLight", LuaScriptInterface::luaDoSetCreatureLight);

	//getCreatureCondition(cid, condition[, subId])
	lua_register(m_luaState, "getCreatureCondition", LuaScriptInterface::luaGetCreatureCondition);

	//doCreatureSetDropLoot(cid, doDrop)
	lua_register(m_luaState, "doCreatureSetDropLoot", LuaScriptInterface::luaDoCreatureSetDropLoot);

	//getPlayerLossPercent(cid, lossType)
	lua_register(m_luaState, "getPlayerLossPercent", LuaScriptInterface::luaGetPlayerLossPercent);

	//doPlayerSetLossPercent(cid, lossType, newPercent)
	lua_register(m_luaState, "doPlayerSetLossPercent", LuaScriptInterface::luaDoPlayerSetLossPercent);

	//doPlayerSetLossSkill(cid, doLose)
	lua_register(m_luaState, "doPlayerSetLossSkill", LuaScriptInterface::luaDoPlayerSetLossSkill);

	//getPlayerLossSkill(cid)
	lua_register(m_luaState, "getPlayerLossSkill", LuaScriptInterface::luaGetPlayerLossSkill);

	//doPlayerSwitchSaving(cid)
	lua_register(m_luaState, "doPlayerSwitchSaving", LuaScriptInterface::luaDoPlayerSwitchSaving);

	//doPlayerSave(cid[, shallow = false])
	lua_register(m_luaState, "doPlayerSave", LuaScriptInterface::luaDoPlayerSave);

	//isPlayerPzLocked(cid)
	lua_register(m_luaState, "isPlayerPzLocked", LuaScriptInterface::luaIsPlayerPzLocked);

	//isPlayerSaving(cid)
	lua_register(m_luaState, "isPlayerSaving", LuaScriptInterface::luaIsPlayerSaving);

	//isCreature(cid)
	lua_register(m_luaState, "isCreature", LuaScriptInterface::luaIsCreature);

	//isContainer(uid)
	lua_register(m_luaState, "isContainer", LuaScriptInterface::luaIsContainer);

	//isMovable(uid)
	lua_register(m_luaState, "isMovable", LuaScriptInterface::luaIsMovable);

	//getCreatureByName(name)
	lua_register(m_luaState, "getCreatureByName", LuaScriptInterface::luaGetCreatureByName);

	//getPlayerByGUID(guid)
	lua_register(m_luaState, "getPlayerByGUID", LuaScriptInterface::luaGetPlayerByGUID);

	//getPlayerByNameWildcard(name~[, ret = false])
	lua_register(m_luaState, "getPlayerByNameWildcard", LuaScriptInterface::luaGetPlayerByNameWildcard);

	//getPlayerGUIDByName(name[, multiworld = false])
	lua_register(m_luaState, "getPlayerGUIDByName", LuaScriptInterface::luaGetPlayerGUIDByName);

	//getPlayerNameByGUID(guid[, multiworld = false[, displayError = true]])
	lua_register(m_luaState, "getPlayerNameByGUID", LuaScriptInterface::luaGetPlayerNameByGUID);

	//registerCreatureEvent(uid, eventName)
	lua_register(m_luaState, "registerCreatureEvent", LuaScriptInterface::luaRegisterCreatureEvent);

	//getContainerSize(uid)
	lua_register(m_luaState, "getContainerSize", LuaScriptInterface::luaGetContainerSize);

	//getContainerCap(uid)
	lua_register(m_luaState, "getContainerCap", LuaScriptInterface::luaGetContainerCap);

	//getContainerItem(uid, slot)
	lua_register(m_luaState, "getContainerItem", LuaScriptInterface::luaGetContainerItem);

	//doAddContainerItem(uid, itemid[, count/subType])
	lua_register(m_luaState, "doAddContainerItem", LuaScriptInterface::luaDoAddContainerItem);

	//getHouseInfo(houseId)
	lua_register(m_luaState, "getHouseInfo", LuaScriptInterface::luaGetHouseInfo);

	//getHouseAccessList(houseid, listId)
	lua_register(m_luaState, "getHouseAccessList", LuaScriptInterface::luaGetHouseAccessList);

	//getHouseByPlayerGUID(playerGUID)
	lua_register(m_luaState, "getHouseByPlayerGUID", LuaScriptInterface::luaGetHouseByPlayerGUID);

	//getHouseFromPos(pos)
	lua_register(m_luaState, "getHouseFromPos", LuaScriptInterface::luaGetHouseFromPos);

	//setHouseAccessList(houseid, listid, listtext)
	lua_register(m_luaState, "setHouseAccessList", LuaScriptInterface::luaSetHouseAccessList);

	//setHouseOwner(houseId, owner[, clean])
	lua_register(m_luaState, "setHouseOwner", LuaScriptInterface::luaSetHouseOwner);

	//getWorldType()
	lua_register(m_luaState, "getWorldType", LuaScriptInterface::luaGetWorldType);

	//setWorldType(type)
	lua_register(m_luaState, "setWorldType", LuaScriptInterface::luaSetWorldType);

	//getWorldTime()
	lua_register(m_luaState, "getWorldTime", LuaScriptInterface::luaGetWorldTime);

	//getWorldLight()
	lua_register(m_luaState, "getWorldLight", LuaScriptInterface::luaGetWorldLight);

	//getWorldCreatures(type)
	//0 players, 1 monsters, 2 npcs, 3 all
	lua_register(m_luaState, "getWorldCreatures", LuaScriptInterface::luaGetWorldCreatures);

	//getWorldUpTime()
	lua_register(m_luaState, "getWorldUpTime", LuaScriptInterface::luaGetWorldUpTime);

	//getGuildId(guildName)
	lua_register(m_luaState, "getGuildId", LuaScriptInterface::luaGetGuildId);

	//getGuildMotd(guildId)
	lua_register(m_luaState, "getGuildMotd", LuaScriptInterface::luaGetGuildMotd);

	//getPlayerSex(cid[, full = false])
	lua_register(m_luaState, "getPlayerSex", LuaScriptInterface::luaGetPlayerSex);

	//doPlayerSetSex(cid, newSex)
	lua_register(m_luaState, "doPlayerSetSex", LuaScriptInterface::luaDoPlayerSetSex);

	//createCombatArea({area}[, {extArea}])
	lua_register(m_luaState, "createCombatArea", LuaScriptInterface::luaCreateCombatArea);

	//createConditionObject(type[, ticks[, buff[, subId]]])
	lua_register(m_luaState, "createConditionObject", LuaScriptInterface::luaCreateConditionObject);

	//setCombatArea(combat, area)
	lua_register(m_luaState, "setCombatArea", LuaScriptInterface::luaSetCombatArea);

	//setCombatCondition(combat, condition)
	lua_register(m_luaState, "setCombatCondition", LuaScriptInterface::luaSetCombatCondition);

	//setCombatParam(combat, key, value)
	lua_register(m_luaState, "setCombatParam", LuaScriptInterface::luaSetCombatParam);

	//setConditionParam(condition, key, value)
	lua_register(m_luaState, "setConditionParam", LuaScriptInterface::luaSetConditionParam);

	//addDamageCondition(condition, rounds, time, value)
	lua_register(m_luaState, "addDamageCondition", LuaScriptInterface::luaAddDamageCondition);

	//addOutfitCondition(condition, outfit)
	lua_register(m_luaState, "addOutfitCondition", LuaScriptInterface::luaAddOutfitCondition);

	//setCombatCallBack(combat, key, function_name)
	lua_register(m_luaState, "setCombatCallback", LuaScriptInterface::luaSetCombatCallBack);

	//setCombatFormula(combat, type, mina, minb, maxa, maxb[, minl, maxl[, minm, maxm[, minc[, maxc]]]])
	lua_register(m_luaState, "setCombatFormula", LuaScriptInterface::luaSetCombatFormula);

	//setConditionFormula(combat, mina, minb, maxa, maxb)
	lua_register(m_luaState, "setConditionFormula", LuaScriptInterface::luaSetConditionFormula);

	//doCombat(cid, combat, param)
	lua_register(m_luaState, "doCombat", LuaScriptInterface::luaDoCombat);

	//createCombatObject()
	lua_register(m_luaState, "createCombatObject", LuaScriptInterface::luaCreateCombatObject);

	//doCombatAreaHealth(cid, type, pos, area, min, max, effect)
	lua_register(m_luaState, "doCombatAreaHealth", LuaScriptInterface::luaDoCombatAreaHealth);

	//doTargetCombatHealth(cid, target, type, min, max, effect)
	lua_register(m_luaState, "doTargetCombatHealth", LuaScriptInterface::luaDoTargetCombatHealth);

	//doCombatAreaMana(cid, pos, area, min, max, effect)
	lua_register(m_luaState, "doCombatAreaMana", LuaScriptInterface::luaDoCombatAreaMana);

	//doTargetCombatMana(cid, target, min, max, effect)
	lua_register(m_luaState, "doTargetCombatMana", LuaScriptInterface::luaDoTargetCombatMana);

	//doCombatAreaCondition(cid, pos, area, condition, effect)
	lua_register(m_luaState, "doCombatAreaCondition", LuaScriptInterface::luaDoCombatAreaCondition);

	//doTargetCombatCondition(cid, target, condition, effect)
	lua_register(m_luaState, "doTargetCombatCondition", LuaScriptInterface::luaDoTargetCombatCondition);

	//doCombatAreaDispel(cid, pos, area, type, effect)
	lua_register(m_luaState, "doCombatAreaDispel", LuaScriptInterface::luaDoCombatAreaDispel);

	//doTargetCombatDispel(cid, target, type, effect)
	lua_register(m_luaState, "doTargetCombatDispel", LuaScriptInterface::luaDoTargetCombatDispel);

	//doChallengeCreature(cid, target)
	lua_register(m_luaState, "doChallengeCreature", LuaScriptInterface::luaDoChallengeCreature);

	//numberToVariant(number)
	lua_register(m_luaState, "numberToVariant", LuaScriptInterface::luaNumberToVariant);

	//stringToVariant(string)
	lua_register(m_luaState, "stringToVariant", LuaScriptInterface::luaStringToVariant);

	//positionToVariant(pos)
	lua_register(m_luaState, "positionToVariant", LuaScriptInterface::luaPositionToVariant);

	//targetPositionToVariant(pos)
	lua_register(m_luaState, "targetPositionToVariant", LuaScriptInterface::luaTargetPositionToVariant);

	//variantToNumber(var)
	lua_register(m_luaState, "variantToNumber", LuaScriptInterface::luaVariantToNumber);

	//variantToString(var)
	lua_register(m_luaState, "variantToString", LuaScriptInterface::luaVariantToString);

	//variantToPosition(var)
	lua_register(m_luaState, "variantToPosition", LuaScriptInterface::luaVariantToPosition);

	//doChangeSpeed(cid, delta)
	lua_register(m_luaState, "doChangeSpeed", LuaScriptInterface::luaDoChangeSpeed);

	//doCreatureChangeOutfit(cid, outfit)
	lua_register(m_luaState, "doCreatureChangeOutfit", LuaScriptInterface::luaDoCreatureChangeOutfit);

	//doSetMonsterOutfit(cid, name, time)
	lua_register(m_luaState, "doSetMonsterOutfit", LuaScriptInterface::luaSetMonsterOutfit);

	//doSetItemOutfit(cid, item, time)
	lua_register(m_luaState, "doSetItemOutfit", LuaScriptInterface::luaSetItemOutfit);

	//doSetCreatureOutfit(cid, outfit, time)
	lua_register(m_luaState, "doSetCreatureOutfit", LuaScriptInterface::luaSetCreatureOutfit);

	//getCreatureOutfit(cid)
	lua_register(m_luaState, "getCreatureOutfit", LuaScriptInterface::luaGetCreatureOutfit);

	//getCreatureLastPosition(cid)
	lua_register(m_luaState, "getCreatureLastPosition", LuaScriptInterface::luaGetCreatureLastPosition);

	//getCreatureName(cid)
	lua_register(m_luaState, "getCreatureName", LuaScriptInterface::luaGetCreatureName);

	//getCreatureSpeed(cid)
	lua_register(m_luaState, "getCreatureSpeed", LuaScriptInterface::luaGetCreatureSpeed);

	//getCreatureBaseSpeed(cid)
	lua_register(m_luaState, "getCreatureBaseSpeed", LuaScriptInterface::luaGetCreatureBaseSpeed);

	//getCreatureTarget(cid)
	lua_register(m_luaState, "getCreatureTarget", LuaScriptInterface::luaGetCreatureTarget);

	//isSightClear(fromPos, toPos, floorCheck)
	lua_register(m_luaState, "isSightClear", LuaScriptInterface::luaIsSightClear);

	//isInArray(array, value[, caseSensitive = false])
	lua_register(m_luaState, "isInArray", LuaScriptInterface::luaIsInArray);

	//addEvent(callback, delay, ...)
	lua_register(m_luaState, "addEvent", LuaScriptInterface::luaAddEvent);

	//stopEvent(eventid)
	lua_register(m_luaState, "stopEvent", LuaScriptInterface::luaStopEvent);

	//getPlayersByAccountId(accId)
	lua_register(m_luaState, "getPlayersByAccountId", LuaScriptInterface::luaGetPlayersByAccountId);

	//getAccountIdByName(name)
	lua_register(m_luaState, "getAccountIdByName", LuaScriptInterface::luaGetAccountIdByName);

	//getAccountByName(name)
	lua_register(m_luaState, "getAccountByName", LuaScriptInterface::luaGetAccountByName);

	//getAccountIdByAccount(accName)
	lua_register(m_luaState, "getAccountIdByAccount", LuaScriptInterface::luaGetAccountIdByAccount);

	//getAccountByAccountId(accId)
	lua_register(m_luaState, "getAccountByAccountId", LuaScriptInterface::luaGetAccountByAccountId);

	//getIpByName(name)
	lua_register(m_luaState, "getIpByName", LuaScriptInterface::luaGetIpByName);

	//getPlayersByIp(ip[, mask = 0xFFFFFFFF])
	lua_register(m_luaState, "getPlayersByIp", LuaScriptInterface::luaGetPlayersByIp);

	//doPlayerPopupFYI(cid, message)
	lua_register(m_luaState, "doPlayerPopupFYI", LuaScriptInterface::luaDoPlayerPopupFYI);

	//doPlayerSendTutorial(cid, id)
	lua_register(m_luaState, "doPlayerSendTutorial", LuaScriptInterface::luaDoPlayerSendTutorial);

	//doPlayerSendMailByName(name, item[, town[, actor]])
	lua_register(m_luaState, "doPlayerSendTutorial", LuaScriptInterface::luaDoPlayerSendTutorial);

	//doPlayerAddMapMark(cid, pos, type[, description])
	lua_register(m_luaState, "doPlayerAddMapMark", LuaScriptInterface::luaDoPlayerAddMapMark);

	//doPlayerAddPremiumDays(cid, days)
	lua_register(m_luaState, "doPlayerAddPremiumDays", LuaScriptInterface::luaDoPlayerAddPremiumDays);

	//getPlayerPremiumDays(cid)
	lua_register(m_luaState, "getPlayerPremiumDays", LuaScriptInterface::luaGetPlayerPremiumDays);

	//doCreatureSetLookDirection(cid, dir)
	lua_register(m_luaState, "doCreatureSetLookDirection", LuaScriptInterface::luaDoCreatureSetLookDir);

	//getCreatureSkullType(cid[, target])
	lua_register(m_luaState, "getCreatureSkullType", LuaScriptInterface::luaGetCreatureSkullType);

	//doCreatureSetSkullType(cid, skull)
	lua_register(m_luaState, "doCreatureSetSkullType", LuaScriptInterface::luaDoCreatureSetSkullType);

	//getPlayerSkullEnd(cid)
	lua_register(m_luaState, "getPlayerSkullEnd", LuaScriptInterface::luaGetPlayerSkullEnd);

	//doPlayerSetSkullEnd(cid, time, type)
	lua_register(m_luaState, "doPlayerSetSkullEnd", LuaScriptInterface::luaDoPlayerSetSkullEnd);

	//getPlayerBalance(cid)
	lua_register(m_luaState, "getPlayerBalance", LuaScriptInterface::luaGetPlayerBalance);

	//getPlayerBlessing(cid, blessing)
	lua_register(m_luaState, "getPlayerBlessing", LuaScriptInterface::luaGetPlayerBlessing);

	//doPlayerAddBlessing(cid, blessing)
	lua_register(m_luaState, "doPlayerAddBlessing", LuaScriptInterface::luaDoPlayerAddBlessing);

	//getPlayerStamina(cid)
	lua_register(m_luaState, "getPlayerStamina", LuaScriptInterface::luaGetPlayerStamina);

	//doPlayerSetStamina(cid, minutes)
	lua_register(m_luaState, "doPlayerSetStamina", LuaScriptInterface::luaDoPlayerSetStamina);

	//doPlayerAddStamina(cid, minutes)
	lua_register(m_luaState, "doPlayerAddStamina", LuaScriptInterface::luaDoPlayerAddStamina);

	//doPlayerSetBalance(cid, balance)
	lua_register(m_luaState, "doPlayerSetBalance", LuaScriptInterface::luaDoPlayerSetBalance);

	//getCreatureNoMove(cid)
	lua_register(m_luaState, "getCreatureNoMove", LuaScriptInterface::luaGetCreatureNoMove);

	//doCreatureSetNoMove(cid, block)
	lua_register(m_luaState, "doCreatureSetNoMove", LuaScriptInterface::luaDoCreatureSetNoMove);

	//getPlayerIdleTime(cid)
	lua_register(m_luaState, "getPlayerIdleTime", LuaScriptInterface::luaGetPlayerIdleTime);

	//doPlayerSetIdleTime(cid, amount)
	lua_register(m_luaState, "doPlayerSetIdleTime", LuaScriptInterface::luaDoPlayerSetIdleTime);

	//getPlayerLastLoad(cid)
	lua_register(m_luaState, "getPlayerLastLoad", LuaScriptInterface::luaGetPlayerLastLoad);

	//getPlayerLastLogin(cid)
	lua_register(m_luaState, "getPlayerLastLogin", LuaScriptInterface::luaGetPlayerLastLogin);

	//getPlayerAccountManager(cid)
	lua_register(m_luaState, "getPlayerAccountManager", LuaScriptInterface::luaGetPlayerAccountManager);

	//getPlayerRates(cid)
	lua_register(m_luaState, "getPlayerRates", LuaScriptInterface::luaGetPlayerRates);

	//doPlayerSetRate(cid, type, value)
	lua_register(m_luaState, "doPlayerSetRate", LuaScriptInterface::luaDoPlayerSetRate);

	//getPlayerPartner(cid)
	lua_register(m_luaState, "getPlayerPartner", LuaScriptInterface::luaGetPlayerPartner);

	//doPlayerSetPartner(cid, guid)
	lua_register(m_luaState, "doPlayerSetPartner", LuaScriptInterface::luaDoPlayerSetPartner);

	//getPlayerParty(cid)
	lua_register(m_luaState, "getPlayerParty", LuaScriptInterface::luaGetPlayerParty);

	//doPlayerJoinParty(cid, lid)
	lua_register(m_luaState, "doPlayerJoinParty", LuaScriptInterface::luaDoPlayerJoinParty);

	//getPartyMembers(lid)
	lua_register(m_luaState, "getPartyMembers", LuaScriptInterface::luaGetPartyMembers);

	//getCreatureMaster(cid)
	lua_register(m_luaState, "getCreatureMaster", LuaScriptInterface::luaGetCreatureMaster);

	//getCreatureSummons(cid)
	lua_register(m_luaState, "getCreatureSummons", LuaScriptInterface::luaGetCreatureSummons);

	//getTownId(townName)
	lua_register(m_luaState, "getTownId", LuaScriptInterface::luaGetTownId);

	//getTownName(townId)
	lua_register(m_luaState, "getTownName", LuaScriptInterface::luaGetTownName);

	//getTownTemplePosition(townId[, displayError])
	lua_register(m_luaState, "getTownTemplePosition", LuaScriptInterface::luaGetTownTemplePosition);

	//getTownHouses(townId)
	lua_register(m_luaState, "getTownHouses", LuaScriptInterface::luaGetTownHouses);

	//getSpectators(centerPos, rangex, rangey[, multifloor = false])
	lua_register(m_luaState, "getSpectators", LuaScriptInterface::luaGetSpectators);

	//getVocationInfo(id)
	lua_register(m_luaState, "getVocationInfo", LuaScriptInterface::luaGetVocationInfo);

	//getGroupInfo(id)
	lua_register(m_luaState, "getGroupInfo", LuaScriptInterface::luaGetGroupInfo);

	//getWaypointList()
	lua_register(m_luaState, "getWaypointList", LuaScriptInterface::luaGetWaypointList);

	//getTalkActionList()
	lua_register(m_luaState, "getTalkActionList", LuaScriptInterface::luaGetTalkActionList);

	//getExperienceStageList()
	lua_register(m_luaState, "getExperienceStageList", LuaScriptInterface::luaGetExperienceStageList);

	//getItemIdByName(name[, displayError = true])
	lua_register(m_luaState, "getItemIdByName", LuaScriptInterface::luaGetItemIdByName);

	//getItemInfo(itemid)
	lua_register(m_luaState, "getItemInfo", LuaScriptInterface::luaGetItemInfo);

	//getItemAttribute(uid, key)
	lua_register(m_luaState, "getItemAttribute", LuaScriptInterface::luaGetItemAttribute);

	//doItemSetAttribute(uid, key, value)
	lua_register(m_luaState, "doItemSetAttribute", LuaScriptInterface::luaDoItemSetAttribute);

	//doItemEraseAttribute(uid, key)
	lua_register(m_luaState, "doItemEraseAttribute", LuaScriptInterface::luaDoItemEraseAttribute);

	//getItemWeight(uid[, precise = true])
	lua_register(m_luaState, "getItemWeight", LuaScriptInterface::luaGetItemWeight);

	//hasItemProperty(uid)
	lua_register(m_luaState, "hasItemProperty", LuaScriptInterface::luaHasItemProperty);
	
	//hasPlayerClient(cid)
	lua_register(m_luaState, "hasPlayerClient", LuaScriptInterface::luaHasPlayerClient);

	//isIpBanished(ip[, mask])
	lua_register(m_luaState, "isIpBanished", LuaScriptInterface::luaIsIpBanished);

	//isPlayerBanished(name/guid, type)
	lua_register(m_luaState, "isPlayerBanished", LuaScriptInterface::luaIsPlayerBanished);

	//isAccountBanished(accountId[, playerId])
	lua_register(m_luaState, "isAccountBanished", LuaScriptInterface::luaIsAccountBanished);

	//doAddIpBanishment(...)
	lua_register(m_luaState, "doAddIpBanishment", LuaScriptInterface::luaDoAddIpBanishment);

	//doAddPlayerBanishment(...)
	lua_register(m_luaState, "doAddPlayerBanishment", LuaScriptInterface::luaDoAddPlayerBanishment);

	//doAddAccountBanishment(...)
	lua_register(m_luaState, "doAddAccountBanishment", LuaScriptInterface::luaDoAddAccountBanishment);

	//doAddNotation(...)
	lua_register(m_luaState, "doAddNotation", LuaScriptInterface::luaDoAddNotation);

	//doAddStatement(...)
	lua_register(m_luaState, "doAddStatement", LuaScriptInterface::luaDoAddStatement);

	//doRemoveIpBanishment(ip[, mask])
	lua_register(m_luaState, "doRemoveIpBanishment", LuaScriptInterface::luaDoRemoveIpBanishment);

	//doRemovePlayerBanishment(name/guid, type)
	lua_register(m_luaState, "doRemovePlayerBanishment", LuaScriptInterface::luaDoRemovePlayerBanishment);

	//doRemoveAccountBanishment(accountId[, playerId])
	lua_register(m_luaState, "doRemoveAccountBanishment", LuaScriptInterface::luaDoRemoveAccountBanishment);

	//doRemoveNotations(accountId[, playerId])
	lua_register(m_luaState, "doRemoveNotations", LuaScriptInterface::luaDoRemoveNotations);

	//doRemoveStatements(name/guid[, channelId])
	lua_register(m_luaState, "doRemoveStatements", LuaScriptInterface::luaDoRemoveStatements);

	//getNotationsCount(accountId[, playerId])
	lua_register(m_luaState, "getNotationsCount", LuaScriptInterface::luaGetNotationsCount);

	//getStatementsCount(name/guid[, channelId])
	lua_register(m_luaState, "getStatementsCount", LuaScriptInterface::luaGetStatementsCount);

	//getBanData(value[, type[, param]])
	lua_register(m_luaState, "getBanData", LuaScriptInterface::luaGetBanData);

	//getBanReason(id)
	lua_register(m_luaState, "getBanReason", LuaScriptInterface::luaGetBanReason);

	//getBanAction(id)
	lua_register(m_luaState, "getBanAction", LuaScriptInterface::luaGetBanAction);

	//getBanList(type[, value[, param]])
	lua_register(m_luaState, "getBanList", LuaScriptInterface::luaGetBanList);

	//getExperienceStage(level)
	lua_register(m_luaState, "getExperienceStage", LuaScriptInterface::luaGetExperienceStage);

	//getDataDir()
	lua_register(m_luaState, "getDataDir", LuaScriptInterface::luaGetDataDir);

	//getLogsDir()
	lua_register(m_luaState, "getLogsDir", LuaScriptInterface::luaGetLogsDir);

	//getConfigFile()
	lua_register(m_luaState, "getConfigFile", LuaScriptInterface::luaGetConfigFile);

	//getConfigValue(key)
	lua_register(m_luaState, "getConfigValue", LuaScriptInterface::luaGetConfigValue);

	//getModList()
	lua_register(m_luaState, "getModList", LuaScriptInterface::luaGetModList);

	//getHighscoreString(skillId)
	lua_register(m_luaState, "getHighscoreString", LuaScriptInterface::luaGetHighscoreString);

	//getWaypointPosition(name)
	lua_register(m_luaState, "getWaypointPosition", LuaScriptInterface::luaGetWaypointPosition);

	//doWaypointAddTemporial(name, pos)
	lua_register(m_luaState, "doWaypointAddTemporial", LuaScriptInterface::luaDoWaypointAddTemporial);

	//getGameState()
	lua_register(m_luaState, "getGameState", LuaScriptInterface::luaGetGameState);

	//doSetGameState(id)
	lua_register(m_luaState, "doSetGameState", LuaScriptInterface::luaDoSetGameState);

	//doExecuteRaid(name)
	lua_register(m_luaState, "doExecuteRaid", LuaScriptInterface::luaDoExecuteRaid);

	//doCreatureExecuteTalkAction(cid, text[, ignoreAccess[, channelId]])
	lua_register(m_luaState, "doCreatureExecuteTalkAction", LuaScriptInterface::luaDoCreatureExecuteTalkAction);

	//doReloadInfo(id[, cid])
	lua_register(m_luaState, "doReloadInfo", LuaScriptInterface::luaDoReloadInfo);

	//doSaveServer()
	lua_register(m_luaState, "doSaveServer", LuaScriptInterface::luaDoSaveServer);

	//doCleanHouse(houseId)
	lua_register(m_luaState, "doCleanHouse", LuaScriptInterface::luaDoCleanHouse);

	//doCleanMap()
	lua_register(m_luaState, "doCleanMap", LuaScriptInterface::luaDoCleanMap);

	//doRefreshMap()
	lua_register(m_luaState, "doRefreshMap", LuaScriptInterface::luaDoRefreshMap);

	//doUpdateHouseAuctions()
	lua_register(m_luaState, "doUpdateHouseAuctions", LuaScriptInterface::luaDoUpdateHouseAuctions);

	//loadmodlib(lib)
	lua_register(m_luaState, "loadmodlib", LuaScriptInterface::luaL_loadmodlib);

	//domodlib(lib)
	lua_register(m_luaState, "domodlib", LuaScriptInterface::luaL_domodlib);

	//dodirectory(dir)
	lua_register(m_luaState, "dodirectory", LuaScriptInterface::luaL_dodirectory);

	//db table
	luaL_register(m_luaState, "db", LuaScriptInterface::luaDatabaseTable);

	//result table
	luaL_register(m_luaState, "result", LuaScriptInterface::luaResultTable);

	//bit table
	luaL_register(m_luaState, "bit", LuaScriptInterface::luaBitTable);

	//std table
	luaL_register(m_luaState, "std", LuaScriptInterface::luaStdTable);
}

const luaL_Reg LuaScriptInterface::luaDatabaseTable[] =
{
	//db.executeQuery(query)
	{"executeQuery", LuaScriptInterface::luaDatabaseExecute},

	//db.storeQuery(query)
	{"storeQuery", LuaScriptInterface::luaDatabaseStoreQuery},

	//db.escapeString(str)
	{"escapeString", LuaScriptInterface::luaDatabaseEscapeString},

	//db.escapeBlob(s, length)
	{"escapeBlob", LuaScriptInterface::luaDatabaseEscapeBlob},

	//db.lastInsertId()
	{"lastInsertId", LuaScriptInterface::luaDatabaseLastInsertId},

	//db.stringComparison()
	{"stringComparison", LuaScriptInterface::luaDatabaseStringComparison},

	//db.updateLimiter()
	{"updateLimiter", LuaScriptInterface::luaDatabaseUpdateLimiter},

	{NULL,NULL}
};

const luaL_Reg LuaScriptInterface::luaResultTable[] =
{
	//result.getDataInt(resId, s)
	{"getDataInt", LuaScriptInterface::luaResultGetDataInt},

	//result.getDataLong(resId, s)
	{"getDataLong", LuaScriptInterface::luaResultGetDataLong},

	//result.getDataString(resId, s)
	{"getDataString", LuaScriptInterface::luaResultGetDataString},

	//result.getDataStream(resId, s, length)
	{"getDataStream", LuaScriptInterface::luaResultGetDataStream},

	//result.next(resId)
	{"next", LuaScriptInterface::luaResultNext},

	//result.free(resId)
	{"free", LuaScriptInterface::luaResultFree},

	{NULL,NULL}
};

const luaL_Reg LuaScriptInterface::luaBitTable[] =
{
	//{"cast", LuaScriptInterface::luaBitCast},
	{"bnot", LuaScriptInterface::luaBitNot},
	{"band", LuaScriptInterface::luaBitAnd},
	{"bor", LuaScriptInterface::luaBitOr},
	{"bxor", LuaScriptInterface::luaBitXor},
	{"lshift", LuaScriptInterface::luaBitLeftShift},
	{"rshift", LuaScriptInterface::luaBitRightShift},
	//{"arshift", LuaScriptInterface::luaBitArithmeticalRightShift},

	//{"ucast", LuaScriptInterface::luaBitUCast},
	{"ubnot", LuaScriptInterface::luaBitUNot},
	{"uband", LuaScriptInterface::luaBitUAnd},
	{"ubor", LuaScriptInterface::luaBitUOr},
	{"ubxor", LuaScriptInterface::luaBitUXor},
	{"ulshift", LuaScriptInterface::luaBitULeftShift},
	{"urshift", LuaScriptInterface::luaBitURightShift},
	//{"uarshift", LuaScriptInterface::luaBitUArithmeticalRightShift},

	{NULL,NULL}
};

const luaL_Reg LuaScriptInterface::luaStdTable[] =
{
	{"cout", LuaScriptInterface::luaStdCout},
	{"cerr", LuaScriptInterface::luaStdCerr},
	{"clog", LuaScriptInterface::luaStdClog},

	{"md5", LuaScriptInterface::luaStdMD5},
	{"sha1", LuaScriptInterface::luaStdSHA1},

	{NULL, NULL}
};

int32_t LuaScriptInterface::internalGetPlayerInfo(lua_State* L, PlayerInfo_t info)
{
	ScriptEnviroment* env = getEnv();
	const Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		std::stringstream s;
		s << getError(LUA_ERROR_PLAYER_NOT_FOUND) << " when requesting player info #" << info;
		errorEx(s.str());

		lua_pushboolean(L, false);
		return 1;
	}

	int64_t value = 0;
	Position pos;
	switch(info)
	{
		case PlayerInfoNameDescription:
			lua_pushstring(L, player->getNameDescription().c_str());
			return 1;
		case PlayerInfoSpecialDescription:
			lua_pushstring(L, player->getSpecialDescription().c_str());
			return 1;
		case PlayerInfoAccess:
			value = player->getAccess();
			break;
		case PlayerInfoGhostAccess:
			value = player->getGhostAccess();
			break;
		case PlayerInfoLevel:
			value = player->getLevel();
			break;
		case PlayerInfoExperience:
			value = player->getExperience();
			break;
		case PlayerInfoManaSpent:
			value = player->getSpentMana();
			break;
		case PlayerInfoTown:
			value = player->getTown();
			break;
		case PlayerInfoPromotionLevel:
			value = player->getPromotionLevel();
			break;
		case PlayerInfoGUID:
			value = player->getGUID();
			break;
		case PlayerInfoAccountId:
			value = player->getAccount();
			break;
		case PlayerInfoAccount:
			lua_pushstring(L, player->getAccountName().c_str());
			return 1;
		case PlayerInfoPremiumDays:
			value = player->getPremiumDays();
			break;
		case PlayerInfoFood:
		{
			if(Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT))
				value = condition->getTicks() / 1000;

			break;
		}
		case PlayerInfoVocation:
			value = player->getVocationId();
			break;
		case PlayerInfoSoul:
			value = player->getSoul();
			break;
		case PlayerInfoFreeCap:
			value = (int64_t)player->getFreeCapacity();
			break;
		case PlayerInfoGuildId:
			value = player->getGuildId();
			break;
		case PlayerInfoGuildName:
			lua_pushstring(L, player->getGuildName().c_str());
			return 1;
		case PlayerInfoGuildRankId:
			value = player->getRankId();
			break;
		case PlayerInfoGuildRank:
			lua_pushstring(L, player->getRankName().c_str());
			return 1;
		case PlayerInfoGuildLevel:
			value = player->getGuildLevel();
			break;
		case PlayerInfoGuildNick:
			lua_pushstring(L, player->getGuildNick().c_str());
			return 1;
		case PlayerInfoGroupId:
			value = player->getGroupId();
			break;
		case PlayerInfoBalance:
			value = (g_config.getBool(ConfigManager::BANK_SYSTEM) ? player->balance : 0);
			break;
		case PlayerInfoStamina:
			value = player->getStaminaMinutes();
			break;
		case PlayerInfoLossSkill:
			lua_pushboolean(L, player->getLossSkill());
			return 1;
		case PlayerInfoMarriage:
			value = player->marriage;
			break;
		case PlayerInfoPzLock:
			lua_pushboolean(L, player->isPzLocked());
			return 1;
		case PlayerInfoSaving:
			lua_pushboolean(L, player->isSaving());
			return 1;
		case PlayerInfoIp:
			value = player->getIP();
			break;
		case PlayerInfoSkullEnd:
			value = player->getSkullEnd();
			break;
		case PlayerInfoOutfitWindow:
			player->sendOutfitWindow();
			lua_pushboolean(L, true);
			return 1;
		case PlayerInfoIdleTime:
			value = player->getIdleTime();
			break;
		case PlayerInfoClient:
			lua_pushboolean(L, player->hasClient());
			return 1;
		case PlayerInfoLastLoad:
			value = player->getLastLoad();
			break;
		case PlayerInfoLastLogin:
			value = player->getLastLogin();
			break;
		case PlayerInfoAccountManager:
			value = player->accountManager;
			break;
		default:
			errorEx("Unknown player info #" + info);
			value = 0;
			break;
	}

	lua_pushnumber(L, value);
	return 1;
}

//getPlayer[Info](uid)
int32_t LuaScriptInterface::luaGetPlayerNameDescription(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoNameDescription);
}

int32_t LuaScriptInterface::luaGetPlayerSpecialDescription(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSpecialDescription);
}

int32_t LuaScriptInterface::luaGetPlayerFood(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoFood);
}

int32_t LuaScriptInterface::luaGetPlayerAccess(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoAccess);
}

int32_t LuaScriptInterface::luaGetPlayerGhostAccess(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGhostAccess);
}

int32_t LuaScriptInterface::luaGetPlayerLevel(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoLevel);
}

int32_t LuaScriptInterface::luaGetPlayerExperience(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoExperience);
}

int32_t LuaScriptInterface::luaGetPlayerSpentMana(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoManaSpent);
}

int32_t LuaScriptInterface::luaGetPlayerVocation(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoVocation);
}

int32_t LuaScriptInterface::luaGetPlayerSoul(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSoul);
}

int32_t LuaScriptInterface::luaGetPlayerFreeCap(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoFreeCap);
}

int32_t LuaScriptInterface::luaGetPlayerGuildId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildId);
}

int32_t LuaScriptInterface::luaGetPlayerGuildName(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildName);
}

int32_t LuaScriptInterface::luaGetPlayerGuildRankId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildRankId);
}

int32_t LuaScriptInterface::luaGetPlayerGuildRank(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildRank);
}

int32_t LuaScriptInterface::luaGetPlayerGuildLevel(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildLevel);
}

int32_t LuaScriptInterface::luaGetPlayerGuildNick(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildNick);
}

int32_t LuaScriptInterface::luaGetPlayerTown(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoTown);
}

int32_t LuaScriptInterface::luaGetPlayerPromotionLevel(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoPromotionLevel);
}

int32_t LuaScriptInterface::luaGetPlayerGroupId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGroupId);
}

int32_t LuaScriptInterface::luaGetPlayerGUID(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGUID);
}

int32_t LuaScriptInterface::luaGetPlayerAccountId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoAccountId);
}

int32_t LuaScriptInterface::luaGetPlayerAccount(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoAccount);
}

int32_t LuaScriptInterface::luaGetPlayerPremiumDays(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoPremiumDays);
}

int32_t LuaScriptInterface::luaGetPlayerBalance(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoBalance);
}

int32_t LuaScriptInterface::luaGetPlayerStamina(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoStamina);
}

int32_t LuaScriptInterface::luaGetPlayerLossSkill(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoLossSkill);
}

int32_t LuaScriptInterface::luaGetPlayerPartner(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoMarriage);
}

int32_t LuaScriptInterface::luaIsPlayerPzLocked(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoPzLock);
}

int32_t LuaScriptInterface::luaIsPlayerSaving(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSaving);
}

int32_t LuaScriptInterface::luaGetPlayerIp(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoIp);
}

int32_t LuaScriptInterface::luaGetPlayerSkullEnd(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSkullEnd);
}

int32_t LuaScriptInterface::luaDoPlayerSendOutfitWindow(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoOutfitWindow);
}

int32_t LuaScriptInterface::luaGetPlayerIdleTime(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoIdleTime);
}

int32_t LuaScriptInterface::luaHasPlayerClient(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoClient);
}

int32_t LuaScriptInterface::luaGetPlayerLastLoad(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoLastLoad);
}

int32_t LuaScriptInterface::luaGetPlayerLastLogin(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoLastLogin);
}

int32_t LuaScriptInterface::luaGetPlayerAccountManager(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoAccountManager);
}
//

int32_t LuaScriptInterface::luaGetPlayerSex(lua_State* L)
{
	//getPlayerSex(cid[, full = false])
	bool full = false;
	if(lua_gettop(L) > 1)
		full = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Player* player = env->getPlayerByUID((uint32_t)popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	else
		lua_pushnumber(L, player->getSex(full));

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetNameDescription(lua_State* L)
{
	//doPlayerSetNameDescription(cid, description)
	std::string description = popString(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->nameDescription += description;
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetSpecialDescription(lua_State* L)
{
	//doPlayerSetSpecialDescription(cid, description)
	std::string description = popString(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setSpecialDescription(description);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerMagLevel(lua_State* L)
{
	//getPlayerMagLevel(cid[, ignoreBuffs = false])
	bool ignoreBuffs = false;
	if(lua_gettop(L) > 1)
		ignoreBuffs = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, (ignoreBuffs ? player->magLevel : player->getMagicLevel()));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerRequiredMana(lua_State* L)
{
	//getPlayerRequiredMana(cid, magicLevel)
	uint32_t magLevel = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, player->vocation->getReqMana(magLevel));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerRequiredSkillTries(lua_State* L)
{
	//getPlayerRequiredSkillTries(cid, skillId, skillLevel)
	int32_t sLevel = popNumber(L), sId = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, player->vocation->getReqSkillTries(sId, sLevel));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerFlagValue(lua_State* L)
{
	//getPlayerFlagValue(cid, flag)
	uint32_t index = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(index < PlayerFlag_LastFlag)
			lua_pushboolean(L, player->hasFlag((PlayerFlags)index));
		else
		{
			std::stringstream ss;
			ss << index;
			errorEx("No valid flag index - " + ss.str());
			lua_pushboolean(L, false);
		}
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerCustomFlagValue(lua_State* L)
{
	//getPlayerCustomFlagValue(cid, flag)
	uint32_t index = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(index < PlayerCustomFlag_LastFlag)
			lua_pushboolean(L, player->hasCustomFlag((PlayerCustomFlags)index));
		else
		{
			std::stringstream ss;
			ss << index;
			errorEx("No valid flag index - " + ss.str());
			lua_pushboolean(L, false);
		}
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerLearnInstantSpell(lua_State* L)
{
	//doPlayerLearnInstantSpell(cid, name)
	std::string spellName = popString(L);

	ScriptEnviroment* env = getEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	player->learnInstantSpell(spell->getName());
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerUnlearnInstantSpell(lua_State* L)
{
	//doPlayerUnlearnInstantSpell(cid, name)
	std::string spellName = popString(L);

	ScriptEnviroment* env = getEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	player->unlearnInstantSpell(spell->getName());
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerLearnedInstantSpell(lua_State* L)
{
	//getPlayerLearnedInstantSpell(cid, name)
	std::string spellName = popString(L);

	ScriptEnviroment* env = getEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, player->hasLearnedInstantSpell(spellName));
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerInstantSpellCount(lua_State* L)
{
	//getPlayerInstantSpellCount(cid)
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, g_spells->getInstantSpellCount(player));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerInstantSpellInfo(lua_State* L)
{
	//getPlayerInstantSpellInfo(cid, index)
	uint32_t index = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByIndex(player, index);
	if(!spell)
	{
		errorEx(getError(LUA_ERROR_SPELL_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setField(L, "name", spell->getName());
	setField(L, "words", spell->getWords());
	setField(L, "level", spell->getLevel());
	setField(L, "mlevel", spell->getMagicLevel());
	setField(L, "mana", spell->getManaCost(player));
	setField(L, "manapercent", spell->getManaPercent());
	return 1;
}

int32_t LuaScriptInterface::luaGetInstantSpellInfo(lua_State* L)
{
	//getInstantSpellInfo(name)
	InstantSpell* spell = g_spells->getInstantSpellByName(popString(L));
	if(!spell)
	{
		errorEx(getError(LUA_ERROR_SPELL_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setField(L, "name", spell->getName());
	setField(L, "words", spell->getWords());
	setField(L, "level", spell->getLevel());
	setField(L, "mlevel", spell->getMagicLevel());
	setField(L, "mana", spell->getManaCost(NULL));
	setField(L, "manapercent", spell->getManaPercent());
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveItem(lua_State* L)
{
	//doRemoveItem(uid[, count])
	int32_t count = -1;
	if(lua_gettop(L) > 1)
		count = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(g_game.internalRemoveItem(NULL, item, count) != RET_NOERROR)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemoveItem(lua_State* L)
{
	//doPlayerRemoveItem(cid, itemid, count[, subType])
	int32_t subType = -1;
	if(lua_gettop(L) > 3)
		subType = popNumber(L);

	uint32_t count = popNumber(L);
	uint16_t itemId = (uint16_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushboolean(L, g_game.removeItemOfType(player, itemId, count, subType));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoFeedPlayer(lua_State* L)
{
	//doFeedPlayer(cid, food)
	int32_t food = (int32_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->addDefaultRegeneration((food * 1000) * 3);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendCancel(lua_State* L)
{
	//doPlayerSendCancel(cid, text)
	std::string text = popString(L);
	ScriptEnviroment* env = getEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->sendCancel(text);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSendDefaultCancel(lua_State* L)
{
	//doPlayerSendDefaultCancel(cid, ReturnValue)
	ReturnValue ret = (ReturnValue)popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->sendCancelMessage(ret);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetSearchString(lua_State* L)
{
	//getSearchString(fromPosition, toPosition[, fromIsCreature = false[, toIsCreature = false]])
	PositionEx toPos, fromPos;
	bool toIsCreature = false, fromIsCreature = false;

	int32_t params = lua_gettop(L);
	if(params > 3)
		toIsCreature = popNumber(L);

	if(params > 2)
		fromIsCreature = popNumber(L);

	popPosition(L, toPos);
	popPosition(L, fromPos);
	if(!toPos.x || !toPos.y || !fromPos.x || !fromPos.y)
	{
		errorEx("wrong position(s) specified.");
		lua_pushboolean(L, false);
	}
	else
		lua_pushstring(L, g_game.getSearchString(fromPos, toPos, fromIsCreature, toIsCreature).c_str());

	return 1;
}

int32_t LuaScriptInterface::luaGetClosestFreeTile(lua_State* L)
{
	//getClosestFreeTile(cid, targetPos[, extended = false[, ignoreHouse = true]])
	uint32_t params = lua_gettop(L);
	bool ignoreHouse = true, extended = false;
	if(params > 3)
		ignoreHouse = popNumber(L);

	if(params > 2)
		extended = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		Position newPos = g_game.getClosestFreeTile(creature, pos, extended, ignoreHouse);
		if(newPos.x != 0)
			pushPosition(L, newPos, 0);
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTeleportThing(lua_State* L)
{
	//doTeleportThing(cid, newpos[, pushmove = TRUE])
	bool pushMove = true;
	if(lua_gettop(L) > 2)
		pushMove = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getEnv();
	if(Thing* tmp = env->getThingByUID(popNumber(L)))
		lua_pushboolean(L, g_game.internalTeleport(tmp, pos, pushMove) == RET_NOERROR);
	else
	{
		errorEx(getError(LUA_ERROR_THING_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTransformItem(lua_State* L)
{
	//doTransformItem(uid, newId[, count/subType])
	int32_t count = -1;
	if(lua_gettop(L) > 2)
		count = popNumber(L);

	uint16_t newId = popNumber(L);
	uint32_t uid = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const ItemType& it = Item::items[newId];
	if(it.stackable && count > 100)
		count = 100;

	Item* newItem = g_game.transformItem(item, newId, count);
	if(item->isRemoved())
		env->removeThing(uid);

	if(newItem && newItem != item)
		env->insertThing(uid, newItem);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSay(lua_State* L)
{
	//doCreatureSay(uid, text[, type = SPEAK_SAY[, ghost = false[, cid = 0[, pos]]]])
	uint32_t params = lua_gettop(L), cid = 0, uid = 0;
	PositionEx pos;
	if(params > 5)
		popPosition(L, pos);

	if(params > 4)
		cid = popNumber(L);

	bool ghost = false;
	if(params > 3)
		ghost = popNumber(L);

	SpeakClasses type = SPEAK_SAY;
	if(params > 2)
		type = (SpeakClasses)popNumber(L);

	std::string text = popString(L);

	uid = popNumber(L);
	if(params > 5 && (!pos.x || !pos.y))
	{
		errorEx("Invalid position specified.");
		lua_pushboolean(L, false);
		return 1;
	}

	ScriptEnviroment* env = getEnv();
	Creature* creature = env->getCreatureByUID(uid);
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	SpectatorVec list;
	if(cid)
	{
		Creature* target = env->getCreatureByUID(cid);
		if(!target)
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}

		list.push_back(target);
	}

	if(params > 5)
		lua_pushboolean(L, g_game.internalCreatureSay(creature, type, text, ghost, &list, &pos));
	else
		lua_pushboolean(L, g_game.internalCreatureSay(creature, type, text, ghost, &list));

	return 1;
}

int32_t LuaScriptInterface::luaDoSendMagicEffect(lua_State* L)
{
	//doSendMagicEffect(pos, type[, player])
	ScriptEnviroment* env = getEnv();
	SpectatorVec list;
	if(lua_gettop(L) > 2)
	{
		if(Creature* creature = env->getCreatureByUID(popNumber(L)))
			list.push_back(creature);
	}

	uint32_t type = popNumber(L);
	PositionEx pos;

	popPosition(L, pos);
	if(pos.x == 0xFFFF)
		pos = env->getRealPos();

	if(!list.empty())
		g_game.addMagicEffect(list, pos, type);
	else
		g_game.addMagicEffect(pos, type);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoSendDistanceShoot(lua_State* L)
{
	//doSendDistanceShoot(fromPos, toPos, type[, player])
	ScriptEnviroment* env = getEnv();
	SpectatorVec list;
	if(lua_gettop(L) > 3)
	{
		if(Creature* creature = env->getCreatureByUID(popNumber(L)))
			list.push_back(creature);
	}

	uint32_t type = popNumber(L);
	PositionEx toPos, fromPos;

	popPosition(L, toPos);
	popPosition(L, fromPos);
	if(fromPos.x == 0xFFFF)
		fromPos = env->getRealPos();

	if(toPos.x == 0xFFFF)
		toPos = env->getRealPos();

	if(!list.empty())
		g_game.addDistanceEffect(list, fromPos, toPos, type);
	else
		g_game.addDistanceEffect(fromPos, toPos, type);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddSkillTry(lua_State* L)
{
	//doPlayerAddSkillTry(uid, skillid, n[, useMultiplier])
	bool multiplier = true;
	if(lua_gettop(L) > 3)
		multiplier = popNumber(L);

	uint32_t n = popNumber(L), skillid = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->addSkillAdvance((skills_t)skillid, n, multiplier);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureSpeakType(lua_State* L)
{
	//getCreatureSpeakType(uid)
	ScriptEnviroment* env = getEnv();
	if(const Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, (SpeakClasses)creature->getSpeakType());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetSpeakType(lua_State* L)
{
	//doCreatureSetSpeakType(uid, type)
	SpeakClasses type = (SpeakClasses)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(type < SPEAK_CLASS_FIRST || type > SPEAK_CLASS_LAST)
		{
			errorEx("Invalid speak type!");
			lua_pushboolean(L, false);
			return 1;
		}

		creature->setSpeakType(type);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureHideHealth(lua_State* L)
{
	//getCreatureHideHealth(cid)
	ScriptEnviroment* env = getEnv();

	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, creature->getHideHealth());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetHideHealth(lua_State* L)
{
	//doCreatureSetHideHealth(cid, hide)
	bool hide = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		creature->setHideHealth(hide);
		g_game.addCreatureHealth(creature);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureAddHealth(lua_State* L)
{
	//doCreatureAddHealth(uid, health[, hitEffect[, hitColor[, force]]])
	int32_t params = lua_gettop(L);
	bool force = false;
	if(params > 4)
		force = popNumber(L);

	TextColor_t hitColor = TEXTCOLOR_UNKNOWN;
	if(params > 3)
		hitColor = (TextColor_t)popNumber(L);

	MagicEffect_t hitEffect = MAGIC_EFFECT_UNKNOWN;
	if(params > 2)
		hitEffect = (MagicEffect_t)popNumber(L);

	int32_t healthChange = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(healthChange) //do not post with 0 value
			g_game.combatChangeHealth(healthChange < 1 ? COMBAT_UNDEFINEDDAMAGE : COMBAT_HEALING,
				NULL, creature, healthChange, hitEffect, hitColor, force);

		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureAddMana(lua_State* L)
{
	//doCreatureAddMana(uid, mana[, aggressive])
	bool aggressive = true;
	if(lua_gettop(L) > 2)
		aggressive = popNumber(L);

	int32_t manaChange = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(aggressive)
			g_game.combatChangeMana(NULL, creature, manaChange);
		else
			creature->changeMana(manaChange);

		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddSpentMana(lua_State* L)
{
	//doPlayerAddSpentMana(cid, amount[, useMultiplier])
	bool multiplier = true;
	if(lua_gettop(L) > 2)
		multiplier = popNumber(L);

	uint32_t amount = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->addManaSpent(amount, multiplier);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddItem(lua_State* L)
{
	//doPlayerAddItem(cid, itemid[, count/subtype[, canDropOnMap]])
	//doPlayerAddItem(cid, itemid[, count[, canDropOnMap[, subtype]]])
	int32_t params = lua_gettop(L), subType = 1;
	if(params > 4)
		subType = popNumber(L);

	bool canDropOnMap = true;
	if(params > 3)
		canDropOnMap = popNumber(L);

	uint32_t count = 1;
	if(params > 2)
		count = popNumber(L);

	uint32_t itemId = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID((uint32_t)popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const ItemType& it = Item::items[itemId];
	int32_t itemCount = 1;
	if(params > 4)
		itemCount = std::max((uint32_t)1, count);
	else if(it.hasSubType())
	{
		if(it.stackable)
			itemCount = (int32_t)std::ceil((float)count / 100);

		subType = count;
	}

	while(itemCount > 0)
	{
		int32_t stackCount = std::min(100, subType);
		Item* newItem = Item::CreateItem(itemId, stackCount);
		if(!newItem)
		{
			errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}

		if(it.stackable)
			subType -= stackCount;

		ReturnValue ret = g_game.internalPlayerAddItem(NULL, player, newItem, canDropOnMap);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			lua_pushboolean(L, false);
			return 1;
		}

		--itemCount;
		if(itemCount)
			continue;

		if(newItem->getParent())
			lua_pushnumber(L, env->addThing(newItem));
		else //stackable item stacked with existing object, newItem will be released
			lua_pushnil(L);

		return 1;
	}

	lua_pushnil(L);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddItemEx(lua_State* L)
{
	//doPlayerAddItemEx(cid, uid[, canDropOnMap = false])
	bool canDropOnMap = false;
	if(lua_gettop(L) > 2)
		canDropOnMap = popNumber(L);

	uint32_t uid = (uint32_t)popNumber(L);
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(item->getParent() == VirtualCylinder::virtualCylinder)
		lua_pushnumber(L, g_game.internalPlayerAddItem(NULL, player, item, canDropOnMap));
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDoTileAddItemEx(lua_State* L)
{
	//doTileAddItemEx(pos, uid)
	uint32_t uid = (uint32_t)popNumber(L);
	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getEnv();
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(item->getParent() == VirtualCylinder::virtualCylinder)
		lua_pushnumber(L, g_game.internalAddItem(NULL, tile, item));
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDoRelocate(lua_State* L)
{
	//doRelocate(pos, posTo[, creatures = true])
	//Moves all moveable objects from pos to posTo

	bool creatures = true;
	if(lua_gettop(L) > 2)
		creatures = popNumber(L);

	PositionEx toPos;
	popPosition(L, toPos);

	PositionEx fromPos;
	popPosition(L, fromPos);

	Tile* fromTile = g_game.getTile(fromPos.x, fromPos.y, fromPos.z);
	if(!fromTile)
	{
		errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Tile* toTile = g_game.getTile(toPos.x, toPos.y, toPos.z);
	if(!toTile)
	{
		errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(fromTile != toTile)
	{
		for(int32_t i = fromTile->getThingCount() - 1; i >= 0; --i)
		{
			Thing* thing = fromTile->__getThing(i);
			if(thing)
			{
				if(Item* item = thing->getItem())
				{
					const ItemType& it = Item::items[item->getID()];
					if(!it.isGroundTile() && !it.alwaysOnTop && !it.isMagicField())
						g_game.internalTeleport(item, toPos, false, FLAG_IGNORENOTMOVEABLE);
				}
				else if(creatures)
				{
					Creature* creature = thing->getCreature();
					if(creature)
						g_game.internalTeleport(creature, toPos, true);
				}
			}
		}
	}

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoCleanTile(lua_State* L)
{
	//doCleanTile(pos, forceMapLoaded = false)
	//Remove all items from tile, ignore creatures
	bool forceMapLoaded = false;
	if(lua_gettop(L) > 1)
		forceMapLoaded = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	Tile* tile = g_game.getTile(pos);
	if(!tile)
	{
		errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	for(int32_t i = tile->getThingCount() - 1; i >= 1; --i) //ignore ground
	{
		if(Thing* thing = tile->__getThing(i))
		{
			if(Item* item = thing->getItem())
			{
				if(!item->isLoadedFromMap() || forceMapLoaded)
					g_game.internalRemoveItem(NULL, item);
			}
		}
	}

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendTextMessage(lua_State* L)
{
	//doPlayerSendTextMessage(cid, MessageClasses, message)
	std::string text = popString(L);
	uint32_t messageClass = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	player->sendTextMessage((MessageClasses)messageClass, text);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendChannelMessage(lua_State* L)
{
	//doPlayerSendChannelMessage(cid, author, message, SpeakClasses, channel)
	uint16_t channelId = popNumber(L);
	uint32_t speakClass = popNumber(L);
	std::string text = popString(L), name = popString(L);

	ScriptEnviroment* env = getEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	player->sendChannelMessage(name, text, (SpeakClasses)speakClass, channelId);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendToChannel(lua_State* L)
{
	//doPlayerSendToChannel(cid, targetId, SpeakClasses, message, channel[, time])
	ScriptEnviroment* env = getEnv();
	uint32_t time = 0;
	if(lua_gettop(L) > 5)
		time = popNumber(L);

	uint16_t channelId = popNumber(L);
	std::string text = popString(L);
	uint32_t speakClass = popNumber(L), targetId = popNumber(L);

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* creature = env->getCreatureByUID(targetId);
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	player->sendToChannel(creature, (SpeakClasses)speakClass, text, channelId, time);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoSendAnimatedText(lua_State* L)
{
	//doSendAnimatedText(pos, text, color[, player])
	ScriptEnviroment* env = getEnv();
	SpectatorVec list;
	if(lua_gettop(L) > 3)
	{
		if(Creature* creature = env->getCreatureByUID(popNumber(L)))
			list.push_back(creature);
	}

	uint32_t color = popNumber(L);
	std::string text = popString(L);

	PositionEx pos;
	popPosition(L, pos);
	if(pos.x == 0xFFFF)
		pos = env->getRealPos();

	if(!list.empty())
		g_game.addAnimatedText(list, pos, color, text);
	else
		g_game.addAnimatedText(pos, color, text);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerSkillLevel(lua_State* L)
{
	//getPlayerSkillLevel(cid, skillid)
	uint32_t skillId = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(skillId <= SKILL_LAST)
			lua_pushnumber(L, player->skills[skillId][SKILL_LEVEL]);
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerSkillTries(lua_State* L)
{
	//getPlayerSkillTries(cid, skillid)
	uint32_t skillid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(skillid <= SKILL_LAST)
			lua_pushnumber(L, player->skills[skillid][SKILL_TRIES]);
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetDropLoot(lua_State* L)
{
	//doCreatureSetDropLoot(cid, doDrop)
	bool doDrop = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		creature->setDropLoot(doDrop ? LOOT_DROP_FULL : LOOT_DROP_NONE);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerLossPercent(lua_State* L)
{
	//getPlayerLossPercent(cid, lossType)
	uint8_t lossType = (uint8_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(lossType <= LOSS_LAST)
		{
			uint32_t value = player->getLossPercent((lossTypes_t)lossType);
			lua_pushnumber(L, value);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetLossPercent(lua_State* L)
{
	//doPlayerSetLossPercent(cid, lossType, newPercent)
	uint32_t newPercent = popNumber(L);
	uint8_t lossType = (uint8_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(lossType <= LOSS_LAST)
		{
			player->setLossPercent((lossTypes_t)lossType, newPercent);
			lua_pushboolean(L, true);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetLossSkill(lua_State* L)
{
	//doPlayerSetLossSkill(cid, doLose)
	bool doLose = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setLossSkill(doLose);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoShowTextDialog(lua_State* L)
{
	//doShowTextDialog(cid, itemid, text)
	std::string text = popString(L);
	uint32_t itemId = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setWriteItem(NULL, 0);
		player->sendTextWindow(itemId, text);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoDecayItem(lua_State* L)
{
	//doDecayItem(uid)
	//Note: to stop decay set decayTo = 0 in items.xml
	ScriptEnviroment* env = getEnv();
	if(Item* item = env->getItemByUID(popNumber(L)))
	{
		g_game.startDecay(item);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetThingFromPos(lua_State* L)
{
	//getThingFromPos(pos[, displayError = true])
	//Note:
	//	stackpos = 255- top thing (movable item or creature)
	//	stackpos = 254- magic field
	//	stackpos = 253- top creature

	bool displayError = true;
	if(lua_gettop(L) > 1)
		displayError = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getEnv();
	Thing* thing = NULL;
	if(Tile* tile = g_game.getMap()->getTile(pos))
	{
		if(pos.stackpos == 255)
		{
			if(!(thing = tile->getTopCreature()))
			{
				Item* item = tile->getTopDownItem();
				if(item && item->isMoveable())
					thing = item;
			}
		}
		else if(pos.stackpos == 254)
			thing = tile->getFieldItem();
		else if(pos.stackpos == 253)
			thing = tile->getTopCreature();
		else
			thing = tile->__getThing(pos.stackpos);

		if(thing)
			pushThing(L, thing, env->addThing(thing));
		else
			pushThing(L, NULL, 0);

		return 1;
	}

	if(displayError)
		errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));

	pushThing(L, NULL, 0);
	return 1;
}

int32_t LuaScriptInterface::luaGetTileItemById(lua_State* L)
{
	//getTileItemById(pos, itemId[, subType = -1])
	ScriptEnviroment* env = getEnv();

	int32_t subType = -1;
	if(lua_gettop(L) > 2)
		subType = (int32_t)popNumber(L);

	int32_t itemId = (int32_t)popNumber(L);
	PositionEx pos;
	popPosition(L, pos);

	Tile* tile = g_game.getTile(pos);
	if(!tile)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	Item* item = g_game.findItemOfType(tile, itemId, false, subType);
	if(!item)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	pushThing(L, item, env->addThing(item));
	return 1;
}

int32_t LuaScriptInterface::luaGetTileItemByType(lua_State* L)
{
	//getTileItemByType(pos, type)
	uint32_t rType = (uint32_t)popNumber(L);
	if(rType >= ITEM_TYPE_LAST)
	{
		errorEx("Not a valid item type");
		pushThing(L, NULL, 0);
		return 1;
	}

	PositionEx pos;
	popPosition(L, pos);

	Tile* tile = g_game.getTile(pos);
	if(!tile)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	bool found = true;
	switch((ItemTypes_t)rType)
	{
		case ITEM_TYPE_TELEPORT:
		{
			if(!tile->hasFlag(TILESTATE_TELEPORT))
				found = false;

			break;
		}
		case ITEM_TYPE_MAGICFIELD:
		{
			if(!tile->hasFlag(TILESTATE_MAGICFIELD))
				found = false;

			break;
		}
		case ITEM_TYPE_MAILBOX:
		{
			if(!tile->hasFlag(TILESTATE_MAILBOX))
				found = false;

			break;
		}
		case ITEM_TYPE_TRASHHOLDER:
		{
			if(!tile->hasFlag(TILESTATE_TRASHHOLDER))
				found = false;

			break;
		}
		case ITEM_TYPE_BED:
		{
			if(!tile->hasFlag(TILESTATE_BED))
				found = false;

			break;
		}
		default:
			break;
	}

	if(!found)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	ScriptEnviroment* env = getEnv();
	Item* item = NULL;
	for(uint32_t i = 0; i < tile->getThingCount(); ++i)
	{
		if(!(item = tile->__getThing(i)->getItem()))
			continue;

		if(Item::items[item->getID()].type != (ItemTypes_t)rType)
			continue;

		pushThing(L, item, env->addThing(item));
		return 1;
	}

	pushThing(L, NULL, 0);
	return 1;
}

int32_t LuaScriptInterface::luaGetTileThingByPos(lua_State* L)
{
	//getTileThingByPos(pos)
	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		if(pos.stackpos == -1)
		{
			lua_pushnumber(L, -1);
			return 1;
		}
		else
		{
			pushThing(L, NULL, 0);
			return 1;
		}
	}

	if(pos.stackpos == -1)
	{
		lua_pushnumber(L, tile->getThingCount());
		return 1;
	}

	Thing* thing = tile->__getThing(pos.stackpos);
	if(!thing)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	pushThing(L, thing, env->addThing(thing));
	return 1;
}

int32_t LuaScriptInterface::luaGetTopCreature(lua_State* L)
{
	//getTopCreature(pos)
	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getEnv();
	Tile* tile = g_game.getTile(pos);
	if(!tile)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	Thing* thing = tile->getTopCreature();
	if(!thing || !thing->getCreature())
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	pushThing(L, thing, env->addThing(thing));
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateItem(lua_State* L)
{
	//doCreateItem(itemid[, type/count], pos)
	//Returns uid of the created item, only works on tiles.
	PositionEx pos;
	popPosition(L, pos);

	uint32_t count = 1;
	if(lua_gettop(L) > 1)
		count = popNumber(L);

	uint32_t itemId = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Tile* tile = g_game.getTile(pos);
	if(!tile)
	{
		errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const ItemType& it = Item::items[itemId];
	int32_t itemCount = 1, subType = 1;
	if(it.hasSubType())
	{
		if(it.stackable)
			itemCount = (int32_t)std::ceil((float)count / 100);

		subType = count;
	}
	else
		itemCount = std::max((uint32_t)1, count);

	while(itemCount > 0)
	{
		int32_t stackCount = std::min(100, subType);
		Item* newItem = Item::CreateItem(itemId, stackCount);
		if(!newItem)
		{
			errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}

		if(it.stackable)
			subType -= stackCount;

		ReturnValue ret = g_game.internalAddItem(NULL, tile, newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			lua_pushboolean(L, false);
			return 1;
		}

		--itemCount;
		if(itemCount)
			continue;

		if(newItem->getParent())
			lua_pushnumber(L, env->addThing(newItem));
		else //stackable item stacked with existing object, newItem will be released
			lua_pushnil(L);

		return 1;
	}

	lua_pushnil(L);
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateItemEx(lua_State* L)
{
	//doCreateItemEx(itemid[, count/subType])
	uint32_t count = 0;
	if(lua_gettop(L) > 1)
		count = popNumber(L);

	ScriptEnviroment* env = getEnv();
	const ItemType& it = Item::items[(uint32_t)popNumber(L)];
	if(it.stackable && count > 100)
		count = 100;

	Item* newItem = Item::CreateItem(it.id, count);
	if(!newItem)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	newItem->setParent(VirtualCylinder::virtualCylinder);
	env->addTempItem(env, newItem);

	lua_pushnumber(L, env->addThing(newItem));
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateTeleport(lua_State* L)
{
	//doCreateTeleport(itemid, toPosition, fromPosition)
	PositionEx createPos;
	popPosition(L, createPos);
	PositionEx toPos;
	popPosition(L, toPos);

	uint32_t itemId = (uint32_t)popNumber(L);
	ScriptEnviroment* env = getEnv();

	Tile* tile = g_game.getMap()->getTile(createPos);
	if(!tile)
	{
		errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Item* newItem = Item::CreateItem(itemId);
	Teleport* newTeleport = newItem->getTeleport();
	if(!newTeleport)
	{
		delete newItem;
		lua_pushboolean(L, false);
		return 1;
	}

	newTeleport->setDestination(toPos);
	if(g_game.internalAddItem(NULL, tile, newTeleport, INDEX_WHEREEVER, FLAG_NOLIMIT) != RET_NOERROR)
	{
		delete newItem;
		lua_pushboolean(L, false);
		return 1;
	}

	if(newItem->getParent())
		lua_pushnumber(L, env->addThing(newItem));
	else //stackable item stacked with existing object, newItem will be released
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureStorage(lua_State* L)
{
	//getCreatureStorage(cid, key)
	uint32_t key = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		std::string strValue;
		if(creature->getStorage(key, strValue))
		{
			int32_t intValue = atoi(strValue.c_str());
			if(intValue || strValue == "0")
				lua_pushnumber(L, intValue);
			else
				lua_pushstring(L, strValue.c_str());
		}
		else
			lua_pushnumber(L, -1);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetStorage(lua_State* L)
{
	//doCreatureSetStorage(cid, key[, value])
	std::string value;
	bool nil = true;
	if(lua_gettop(L) > 2)
	{
		if(!lua_isnil(L, -1))
		{
			value = popString(L);
			nil = false;
		}
		else
			lua_pop(L, 1);
	}

	uint32_t key = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(!nil)
			nil = creature->setStorage(key, value);
		else
			creature->eraseStorage(key);

		lua_pushboolean(L, nil);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetTileInfo(lua_State* L)
{
	//getTileInfo(pos)
	PositionEx pos;
	popPosition(L, pos);
	if(Tile* tile = g_game.getMap()->getTile(pos))
	{
		ScriptEnviroment* env = getEnv();
		pushThing(L, tile->ground, env->addThing(tile->ground));

		setFieldBool(L, "protection", tile->hasFlag(TILESTATE_PROTECTIONZONE));
		setFieldBool(L, "nopvp", tile->hasFlag(TILESTATE_NOPVPZONE));
		setFieldBool(L, "nologout", tile->hasFlag(TILESTATE_NOLOGOUT));
		setFieldBool(L, "pvp", tile->hasFlag(TILESTATE_PVPZONE));
		setFieldBool(L, "refresh", tile->hasFlag(TILESTATE_REFRESH));
		setFieldBool(L, "trashed", tile->hasFlag(TILESTATE_TRASHED));
		setFieldBool(L, "house", tile->hasFlag(TILESTATE_HOUSE));
		setFieldBool(L, "bed", tile->hasFlag(TILESTATE_BED));
		setFieldBool(L, "depot", tile->hasFlag(TILESTATE_DEPOT));

		setField(L, "things", tile->getThingCount());
		setField(L, "creatures", tile->getCreatureCount());
		setField(L, "items", tile->getItemCount());
		setField(L, "topItems", tile->getTopItemCount());
		setField(L, "downItems", tile->getDownItemCount());
	}
	else
	{
		errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetHouseFromPos(lua_State* L)
{
	//getHouseFromPos(pos)
	PositionEx pos;
	popPosition(L, pos);

	Tile* tile = g_game.getMap()->getTile(pos);
	if(!tile)
	{
		errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	HouseTile* houseTile = tile->getHouseTile();
	if(!houseTile)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	House* house = houseTile->getHouse();
	if(!house)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushnumber(L, house->getId());
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateMonster(lua_State* L)
{
	//doCreateMonster(name, pos[, displayError = true])
	bool displayError = true;
	if(lua_gettop(L) > 2)
		displayError = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	std::string name = popString(L);
	Monster* monster = Monster::createMonster(name.c_str());
	if(!monster)
	{
		if(displayError)
			errorEx("Monster with name '" + name + "' not found");

		lua_pushboolean(L, false);
		return 1;
	}

	if(!g_game.placeCreature(monster, pos))
	{
		delete monster;
		if(displayError)
			errorEx("Cannot create monster: " + name);

		lua_pushboolean(L, true);
		return 1;
	}

	ScriptEnviroment* env = getEnv();
	lua_pushnumber(L, env->addThing((Thing*)monster));
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateNpc(lua_State* L)
{
	//doCreateNpc(name, pos[, displayError = true])
	bool displayError = true;
	if(lua_gettop(L) > 2)
		displayError = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	std::string name = popString(L);
	Npc* npc = Npc::createNpc(name.c_str());
	if(!npc)
	{
		if(displayError)
			errorEx("Npc with name '" + name + "' not found");

		lua_pushboolean(L, false);
		return 1;
	}

	if(!g_game.placeCreature(npc, pos))
	{
		delete npc;
		if(displayError)
			errorEx("Cannot create npc: " + name);

		lua_pushboolean(L, true); //for scripting compatibility
		return 1;
	}

	ScriptEnviroment* env = getEnv();
	lua_pushnumber(L, env->addThing((Thing*)npc));
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveCreature(lua_State* L)
{
	//doRemoveCreature(cid[, forceLogout = true])
	bool forceLogout = true;
	if(lua_gettop(L) > 1)
		forceLogout = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(Player* player = creature->getPlayer())
			player->kickPlayer(true, forceLogout); //Players will get kicked without restrictions
		else
			g_game.removeCreature(creature); //Monsters/NPCs will get removed

		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddMoney(lua_State* L)
{
	//doPlayerAddMoney(cid, money)
	uint64_t money = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		g_game.addMoney(player, money);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemoveMoney(lua_State* L)
{
	//doPlayerRemoveMoney(cid,money)
	uint64_t money = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushboolean(L, g_game.removeMoney(player, money));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerTransferMoneyTo(lua_State* L)
{
	//doPlayerTransferMoneyTo(cid, target, money)
	uint64_t money = popNumber(L);
	std::string target = popString(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushboolean(L, player->transferMoneyTo(target, money));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetPzLocked(lua_State* L)
{
	//doPlayerSetPzLocked(cid, locked)
	bool locked = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(player->isPzLocked() != locked)
		{
			player->setPzLocked(locked);
			player->sendIcons();
		}

		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetTown(lua_State* L)
{
	//doPlayerSetTown(cid, townid)
	uint32_t townid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(Town* town = Towns::getInstance()->getTown(townid))
		{
			player->setMasterPosition(town->getPosition());
			player->setTown(townid);
			lua_pushboolean(L, true);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetVocation(lua_State* L)
{
	//doPlayerSetVocation(cid, voc)
	uint32_t voc = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setVocation(voc);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetSex(lua_State* L)
{
	//doPlayerSetSex(cid, sex)
	uint32_t newSex = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setSex(newSex);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddSoul(lua_State* L)
{
	//doPlayerAddSoul(cid, soul)
	int32_t soul = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->changeSoul(soul);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerItemCount(lua_State* L)
{
	//getPlayerItemCount(cid, itemid[, subType = -1])
	int32_t subType = -1;
	if(lua_gettop(L) > 2)
		subType = popNumber(L);

	uint32_t itemId = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, player->__getItemTypeCount(itemId, subType));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerMoney(lua_State* L)
{
	//getPlayerMoney(cid)
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, g_game.getMoney(player));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseInfo(lua_State* L)
{
	//getHouseInfo(houseId)
	bool displayError = true;
	if(lua_gettop(L) > 1)
		displayError = popNumber(L);

	House* house = Houses::getInstance()->getHouse(popNumber(L));
	if(!house)
	{
		if(displayError)
			errorEx(getError(LUA_ERROR_HOUSE_NOT_FOUND));

		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setField(L, "id", house->getId());
	setField(L, "name", house->getName().c_str());
	setField(L, "owner", house->getOwner());

	lua_pushstring(L, "entry");
	pushPosition(L, house->getEntry(), 0);
	pushTable(L);

	setField(L, "rent", house->getRent());
	setField(L, "price", house->getPrice());
	setField(L, "town", house->getTownId());
	setField(L, "paidUntil", house->getPaidUntil());
	setField(L, "warnings", house->getRentWarnings());
	setField(L, "lastWarning", house->getLastWarning());

	setFieldBool(L, "guildHall", house->isGuild());
	setField(L, "size", house->getSize());
	setField(L, "doors", house->getDoorsCount());
	setField(L, "beds", house->getBedsCount());
	setField(L, "tiles", house->getTilesCount());

	return 1;
}

int32_t LuaScriptInterface::luaGetHouseAccessList(lua_State* L)
{
	//getHouseAccessList(houseid, listid)
	uint32_t listid = popNumber(L);
	if(House* house = Houses::getInstance()->getHouse(popNumber(L)))
	{
		std::string list;
		if(house->getAccessList(listid, list))
			lua_pushstring(L, list.c_str());
		else
			lua_pushnil(L);
	}
	else
	{
		errorEx(getError(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseByPlayerGUID(lua_State* L)
{
	//getHouseByPlayerGUID(guid)
	if(House* house = Houses::getInstance()->getHouseByPlayerId(popNumber(L)))
		lua_pushnumber(L, house->getId());
	else
		lua_pushnil(L);
	return 1;
}

int32_t LuaScriptInterface::luaSetHouseAccessList(lua_State* L)
{
	//setHouseAccessList(houseid, listid, listtext)
	std::string list = popString(L);
	uint32_t listid = popNumber(L);

	if(House* house = Houses::getInstance()->getHouse(popNumber(L)))
	{
		house->setAccessList(listid, list);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetHouseOwner(lua_State* L)
{
	//setHouseOwner(houseId, owner[, clean])
	bool clean = true;
	if(lua_gettop(L) > 2)
		clean = popNumber(L);

	uint32_t owner = popNumber(L);
	if(House* house = Houses::getInstance()->getHouse(popNumber(L)))
		lua_pushboolean(L, house->setOwnerEx(owner, clean));
	else
	{
		errorEx(getError(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldType(lua_State* L)
{
	lua_pushnumber(L, (uint32_t)g_game.getWorldType());
	return 1;
}

int32_t LuaScriptInterface::luaSetWorldType(lua_State* L)
{
	//setWorldType(type)
	WorldType_t type = (WorldType_t)popNumber(L);

	if(type >= WORLD_TYPE_FIRST && type <= WORLD_TYPE_LAST)
	{
		g_game.setWorldType(type);
		lua_pushboolean(L, true);
	}
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetWorldTime(lua_State* L)
{
	//getWorldTime()
	lua_pushnumber(L, g_game.getLightHour());
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldLight(lua_State* L)
{
	//getWorldLight()
	LightInfo lightInfo;
	g_game.getWorldLightInfo(lightInfo);
	lua_pushnumber(L, lightInfo.level);
	lua_pushnumber(L, lightInfo.color);
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldCreatures(lua_State* L)
{
	//getWorldCreatures(type)
	//0 players, 1 monsters, 2 npcs, 3 all
	uint32_t type = popNumber(L), value;
	switch(type)
	{
		case 0:
			value = g_game.getPlayersOnline();
			break;
		case 1:
			value = g_game.getMonstersOnline();
			break;
		case 2:
			value = g_game.getNpcsOnline();
			break;
		case 3:
			value = g_game.getCreaturesOnline();
			break;
		default:
			lua_pushboolean(L, false);
			return 1;
	}

	lua_pushnumber(L, value);
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldUpTime(lua_State* L)
{
	//getWorldUpTime()
	uint32_t uptime = 0;
	if(Status* status = Status::getInstance())
		uptime = status->getUptime();

	lua_pushnumber(L, uptime);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerLight(lua_State* L)
{
	//getPlayerLight(cid)
	ScriptEnviroment* env = getEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
	{
		LightInfo lightInfo;
		player->getCreatureLight(lightInfo);
		lua_pushnumber(L, lightInfo.level);
		lua_pushnumber(L, lightInfo.color);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddExperience(lua_State* L)
{
	//doPlayerAddExperience(cid, amount)
	int64_t amount = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(amount > 0)
			player->addExperience(amount);
		else if(amount < 0)
			player->removeExperience(std::abs(amount));
		else
		{
			lua_pushboolean(L, false);
			return 1;
		}

		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerSlotItem(lua_State* L)
{
	//getPlayerSlotItem(cid, slot)
	uint32_t slot = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(Thing* thing = player->__getThing(slot))
			pushThing(L, thing, env->addThing(thing));
		else
			pushThing(L, NULL, 0);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerWeapon(lua_State* L)
{
	//getPlayerWeapon(cid[, ignoreAmmo = false])
	bool ignoreAmmo = false;
	if(lua_gettop(L) > 1)
		ignoreAmmo = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(Item* weapon = player->getWeapon(ignoreAmmo))
			pushThing(L, weapon, env->addThing(weapon));
		else
			pushThing(L, NULL, 0);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerItemById(lua_State* L)
{
	//getPlayerItemById(cid, deepSearch, itemId[, subType = -1])
	ScriptEnviroment* env = getEnv();

	int32_t subType = -1;
	if(lua_gettop(L) > 3)
		subType = (int32_t)popNumber(L);

	int32_t itemId = (int32_t)popNumber(L);
	bool deepSearch = popNumber(L);

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		pushThing(L, NULL, 0);
		return 1;
	}

	Item* item = g_game.findItemOfType(player, itemId, deepSearch, subType);
	if(!item)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	pushThing(L, item, env->addThing(item));
	return 1;
}

int32_t LuaScriptInterface::luaGetThing(lua_State* L)
{
	//getThing(uid)
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Thing* thing = env->getThingByUID(uid))
		pushThing(L, thing, uid);
	else
	{
		errorEx(getError(LUA_ERROR_THING_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTileQueryAdd(lua_State* L)
{
	//doTileQueryAdd(uid, pos[, flags[, displayError = true]])
	uint32_t flags = 0, params = lua_gettop(L);
	bool displayError = true;
	if(params > 3)
		displayError = popNumber(L);

	if(params > 2)
		flags = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Tile* tile = g_game.getTile(pos);
	if(!tile)
	{
		if(displayError)
			errorEx(getError(LUA_ERROR_TILE_NOT_FOUND));

		lua_pushnumber(L, (uint32_t)RET_NOTPOSSIBLE);
		return 1;
	}

	Thing* thing = env->getThingByUID(uid);
	if(!thing)
	{
		if(displayError)
			errorEx(getError(LUA_ERROR_THING_NOT_FOUND));

		lua_pushnumber(L, (uint32_t)RET_NOTPOSSIBLE);
		return 1;
	}

	lua_pushnumber(L, (uint32_t)tile->__queryAdd(0, thing, 1, flags));
	return 1;
}

int32_t LuaScriptInterface::luaDoItemRaidUnref(lua_State* L)
{
	//doItemRaidUnref(uid)
	ScriptEnviroment* env = getEnv();
	if(Item* item = env->getItemByUID(popNumber(L)))
	{
		if(Raid* raid = item->getRaid())
		{
			raid->unRef();
			item->setRaid(NULL);
			lua_pushboolean(L, true);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetThingPosition(lua_State* L)
{
	//getThingPosition(uid)
	ScriptEnviroment* env = getEnv();
	if(Thing* thing = env->getThingByUID(popNumber(L)))
	{
		Position pos = thing->getPosition();
		uint32_t stackpos = 0;
		if(Tile* tile = thing->getTile())
			stackpos = tile->__getIndexOfThing(thing);

		pushPosition(L, pos, stackpos);
	}
	else
	{
		errorEx(getError(LUA_ERROR_THING_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaCreateCombatObject(lua_State* L)
{
	//createCombatObject()
	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Combat* combat = new Combat;
	if(!combat)
	{
		errorEx(getError(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushnumber(L, env->addCombatObject(combat));
	return 1;
}

bool LuaScriptInterface::getArea(lua_State* L, std::list<uint32_t>& list, uint32_t& rows)
{
	rows = 0;
	uint32_t i = 0;

	lua_pushnil(L);
	while(lua_next(L, -2))
	{
		lua_pushnil(L);
		while(lua_next(L, -2))
		{
			list.push_back((uint32_t)lua_tonumber(L, -1));
			lua_pop(L, 1); //removes value, keeps key for next iteration
			++i;
		}

		lua_pop(L, 1); //removes value, keeps key for next iteration
		++rows;
		i = 0;
	}

	lua_pop(L, 1);
	return rows;
}

int32_t LuaScriptInterface::luaCreateCombatArea(lua_State* L)
{
	//createCombatArea( {area}[, {extArea}])
	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	CombatArea* area = new CombatArea;
	if(lua_gettop(L) > 1)
	{
		//has extra parameter with diagonal area information
		uint32_t rowsExtArea;
		std::list<uint32_t> listExtArea;

		getArea(L, listExtArea, rowsExtArea);
		/*setup all possible rotations*/
		area->setupExtArea(listExtArea, rowsExtArea);
	}

	if(lua_isnoneornil(L, -1)) //prevent crash
	{
		lua_pop(L, 2);
		lua_pushboolean(L, false);
		return 1;
	}

	uint32_t rowsArea = 0;
	std::list<uint32_t> listArea;
	getArea(L, listArea, rowsArea);

	area->setupArea(listArea, rowsArea);
	lua_pushnumber(L, env->addCombatArea(area));
	return 1;
}

int32_t LuaScriptInterface::luaCreateConditionObject(lua_State* L)
{
	//createConditionObject(type[, ticks[, buff[, subId]]])
	uint32_t params = lua_gettop(L), subId = 0;
	if(params > 3)
		subId = popNumber(L);

	bool buff = false;
	if(params > 2)
		buff = popNumber(L);

	int32_t ticks = 0;
	if(params > 1)
		ticks = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	if(Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, (ConditionType_t)popNumber(L), ticks, 0, buff, subId))
		lua_pushnumber(L, env->addConditionObject(condition));
	else
	{
		errorEx(getError(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaSetCombatArea(lua_State* L)
{
	//setCombatArea(combat, area)
	uint32_t areaId = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Combat* combat = env->getCombatObject(popNumber(L));
	if(!combat)
	{
		errorEx(getError(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const CombatArea* area = env->getCombatArea(areaId);
	if(!area)
	{
		errorEx(getError(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	combat->setArea(new CombatArea(*area));
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatCondition(lua_State* L)
{
	//setCombatCondition(combat, condition)
	uint32_t conditionId = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Combat* combat = env->getCombatObject(popNumber(L));
	if(!combat)
	{
		errorEx(getError(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const Condition* condition = env->getConditionObject(conditionId);
	if(!condition)
	{
		errorEx(getError(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	combat->setCondition(condition->clone());
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatParam(lua_State* L)
{
	//setCombatParam(combat, key, value)
	uint32_t value = popNumber(L);
	CombatParam_t key = (CombatParam_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Combat* combat = env->getCombatObject(popNumber(L));
	if(!combat)
	{
		errorEx(getError(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	else
	{
		combat->setParam(key, value);
		lua_pushboolean(L, true);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetConditionParam(lua_State* L)
{
	//setConditionParam(condition, key, value)
	int32_t value = (int32_t)popNumber(L);
	ConditionParam_t key = (ConditionParam_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	if(Condition* condition = env->getConditionObject(popNumber(L)))
	{
		condition->setParam(key, value);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaAddDamageCondition(lua_State* L)
{
	//addDamageCondition(condition, rounds, time, value)
	int32_t value = popNumber(L), time = popNumber(L), rounds = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	if(ConditionDamage* condition = dynamic_cast<ConditionDamage*>(env->getConditionObject(popNumber(L))))
	{
		condition->addDamage(rounds, time, value);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaAddOutfitCondition(lua_State* L)
{
	//addOutfitCondition(condition, outfit)
	Outfit_t outfit = popOutfit(L);
	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	if(ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(env->getConditionObject(popNumber(L))))
	{
		condition->addOutfit(outfit);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaSetCombatCallBack(lua_State* L)
{
	//setCombatCallBack(combat, key, functionName)
	std::string function = popString(L);
	CallBackParam_t key = (CallBackParam_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	Combat* combat = env->getCombatObject(popNumber(L));
	if(!combat)
	{
		errorEx(getError(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	LuaScriptInterface* interface = env->getInterface();
	combat->setCallback(key);

	CallBack* callback = combat->getCallback(key);
	if(!callback)
	{
		std::stringstream ss;
		ss << key;

		errorEx(ss.str() + " is not a valid callback key.");
		lua_pushboolean(L, false);
		return 1;
	}

	if(!callback->loadCallBack(interface, function))
	{
		errorEx("Cannot load callback");
		lua_pushboolean(L, false);
	}
	else
		lua_pushboolean(L, true);

	return 1;
}

int32_t LuaScriptInterface::luaSetCombatFormula(lua_State* L)
{
	//setCombatFormula(combat, type, mina, minb, maxa, maxb[, minl, maxl[, minm, maxm[, minc[, maxc]]]])
	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	int32_t params = lua_gettop(L), minc = 0, maxc = 0;
	if(params > 11)
		maxc = popNumber(L);

	if(params > 10)
		minc = popNumber(L);

	double minm = 1.0, maxm = 1.0, minl = 5.0, maxl = 5.0;
	if(params > 8)
	{
		maxm = popFloatNumber(L);
		minm = popFloatNumber(L);
	}

	if(params > 6)
	{
		maxl = popFloatNumber(L);
		minl = popFloatNumber(L);
	}

	double maxb = popFloatNumber(L), maxa = popFloatNumber(L),
		minb = popFloatNumber(L), mina = popFloatNumber(L);
	formulaType_t type = (formulaType_t)popNumber(L);
	if(Combat* combat = env->getCombatObject(popNumber(L)))
	{
		combat->setPlayerCombatValues(type, mina, minb, maxa, maxb, minl, maxl, minm, maxm, minc, maxc);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaSetConditionFormula(lua_State* L)
{
	//setConditionFormula(condition, mina, minb, maxa, maxb)
	ScriptEnviroment* env = getEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		errorEx("This function can only be used while loading the script.");
		lua_pushboolean(L, false);
		return 1;
	}

	double maxb = popFloatNumber(L), maxa = popFloatNumber(L),
		minb = popFloatNumber(L), mina = popFloatNumber(L);

	if(ConditionSpeed* condition = dynamic_cast<ConditionSpeed*>(env->getConditionObject(popNumber(L))))
	{
		condition->setFormulaVars(mina, minb, maxa, maxb);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoCombat(lua_State* L)
{
	//doCombat(cid, combat, param)
	ScriptEnviroment* env = getEnv();

	LuaVariant var = popVariant(L);
	uint32_t combatId = popNumber(L), cid = popNumber(L);

	Creature* creature = NULL;
	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);
		if(!creature)
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	const Combat* combat = env->getCombatObject(combatId);
	if(!combat)
	{
		errorEx(getError(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(var.type == VARIANT_NONE)
	{
		errorEx(getError(LUA_ERROR_VARIANT_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	switch(var.type)
	{
		case VARIANT_NUMBER:
		{
			Creature* target = g_game.getCreatureByID(var.number);
			if(!target || !creature || !creature->canSeeCreature(target))
			{
				lua_pushboolean(L, false);
				return 1;
			}

			if(combat->hasArea())
				combat->doCombat(creature, target->getPosition());
			else
				combat->doCombat(creature, target);

			break;
		}

		case VARIANT_POSITION:
		{
			combat->doCombat(creature, var.pos);
			break;
		}

		case VARIANT_TARGETPOSITION:
		{
			if(!combat->hasArea())
			{
				combat->postCombatEffects(creature, var.pos);
				g_game.addMagicEffect(var.pos, MAGIC_EFFECT_POFF);
			}
			else
				combat->doCombat(creature, var.pos);

			break;
		}

		case VARIANT_STRING:
		{
			Player* target = g_game.getPlayerByName(var.text);
			if(!target || !creature || !creature->canSeeCreature(target))
			{
				lua_pushboolean(L, false);
				return 1;
			}

			combat->doCombat(creature, target);
			break;
		}

		default:
		{
			errorEx(getError(LUA_ERROR_VARIANT_UNKNOWN));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoCombatAreaHealth(lua_State* L)
{
	//doCombatAreaHealth(cid, type, pos, area, min, max, effect)
	MagicEffect_t effect = (MagicEffect_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L), minChange = (int32_t)popNumber(L);
	uint32_t areaId = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	CombatType_t combatType = (CombatType_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = NULL;
	if(cid)
	{
		if(!(creature = env->getCreatureByUID(cid)))
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	const CombatArea* area = env->getCombatArea(areaId);
	if(area || !areaId)
	{
		CombatParams params;
		params.combatType = combatType;
		params.effects.impact = effect;

		Combat::doCombatHealth(creature, pos, area, minChange, maxChange, params);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatHealth(lua_State* L)
{
	//doTargetCombatHealth(cid, target, type, min, max, effect)
	MagicEffect_t effect = (MagicEffect_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L), minChange = (int32_t)popNumber(L);

	CombatType_t combatType = (CombatType_t)popNumber(L);
	uint32_t targetCid = popNumber(L), cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = NULL;
	if(cid)
	{
		if(!(creature = env->getCreatureByUID(cid)))
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target)
	{
		CombatParams params;
		params.combatType = combatType;
		params.effects.impact = effect;

		Combat::doCombatHealth(creature, target, minChange, maxChange, params);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoCombatAreaMana(lua_State* L)
{
	//doCombatAreaMana(cid, pos, area, min, max, effect)
	MagicEffect_t effect = (MagicEffect_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L), minChange = (int32_t)popNumber(L);
	uint32_t areaId = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = NULL;
	if(cid)
	{
		if(!(creature = env->getCreatureByUID(cid)))
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	const CombatArea* area = env->getCombatArea(areaId);
	if(area || !areaId)
	{
		CombatParams params;
		params.effects.impact = effect;

		Combat::doCombatMana(creature, pos, area, minChange, maxChange, params);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatMana(lua_State* L)
{
	//doTargetCombatMana(cid, target, min, max, effect)
	MagicEffect_t effect = (MagicEffect_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L), minChange = (int32_t)popNumber(L);
	uint32_t targetCid = popNumber(L), cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = NULL;
	if(cid)
	{
		if(!(creature = env->getCreatureByUID(cid)))
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	if(Creature* target = env->getCreatureByUID(targetCid))
	{
		CombatParams params;
		params.effects.impact = effect;

		Combat::doCombatMana(creature, target, minChange, maxChange, params);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoCombatAreaCondition(lua_State* L)
{
	//doCombatAreaCondition(cid, pos, area, condition, effect)
	MagicEffect_t effect = (MagicEffect_t)popNumber(L);
	uint32_t conditionId = popNumber(L), areaId = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = NULL;
	if(cid)
	{
		if(!(creature = env->getCreatureByUID(cid)))
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	if(const Condition* condition = env->getConditionObject(conditionId))
	{
		const CombatArea* area = env->getCombatArea(areaId);
		if(area || !areaId)
		{
			CombatParams params;
			params.effects.impact = effect;
			params.conditionList.push_back(condition);
			Combat::doCombatCondition(creature, pos, area, params);

			lua_pushboolean(L, true);
		}
		else
		{
			errorEx(getError(LUA_ERROR_AREA_NOT_FOUND));
			lua_pushboolean(L, false);
		}
	}
	else
	{
		errorEx(getError(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatCondition(lua_State* L)
{
	//doTargetCombatCondition(cid, target, condition, effect)
	MagicEffect_t effect = (MagicEffect_t)popNumber(L);
	uint32_t conditionId = popNumber(L), targetCid = popNumber(L), cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = NULL;
	if(cid)
	{
		if(!(creature = env->getCreatureByUID(cid)))
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	if(Creature* target = env->getCreatureByUID(targetCid))
	{
		if(const Condition* condition = env->getConditionObject(conditionId))
		{
			CombatParams params;
			params.effects.impact = effect;
			params.conditionList.push_back(condition);

			Combat::doCombatCondition(creature, target, params);
			lua_pushboolean(L, true);
		}
		else
		{
			errorEx(getError(LUA_ERROR_CONDITION_NOT_FOUND));
			lua_pushboolean(L, false);
		}
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoCombatAreaDispel(lua_State* L)
{
	//doCombatAreaDispel(cid, pos, area, type, effect)
	MagicEffect_t effect = (MagicEffect_t)popNumber(L);
	ConditionType_t dispelType = (ConditionType_t)popNumber(L);
	uint32_t areaId = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = NULL;
	if(cid)
	{
		if(!(creature = env->getCreatureByUID(cid)))
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	const CombatArea* area = env->getCombatArea(areaId);
	if(area || !areaId)
	{
		CombatParams params;
		params.effects.impact = effect;
		params.dispelType = dispelType;

		Combat::doCombatDispel(creature, pos, area, params);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatDispel(lua_State* L)
{
	//doTargetCombatDispel(cid, target, type, effect)
	MagicEffect_t effect = (MagicEffect_t)popNumber(L);
	ConditionType_t dispelType = (ConditionType_t)popNumber(L);
	uint32_t targetCid = popNumber(L), cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = NULL;
	if(cid)
	{
		if(!(creature = env->getCreatureByUID(cid)))
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}
	}

	if(Creature* target = env->getCreatureByUID(targetCid))
	{
		CombatParams params;
		params.effects.impact = effect;
		params.dispelType = dispelType;

		Combat::doCombatDispel(creature, target, params);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoChallengeCreature(lua_State* L)
{
	//doChallengeCreature(cid, target)
	ScriptEnviroment* env = getEnv();
	uint32_t targetCid = popNumber(L);

	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	target->challengeCreature(creature);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoSummonMonster(lua_State* L)
{
	//doSummonMonster(cid, name)
	std::string name = popString(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushnumber(L, g_game.placeSummon(creature, name));
	return 1;
}

int32_t LuaScriptInterface::luaDoConvinceCreature(lua_State* L)
{
	//doConvinceCreature(cid, target)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* target = env->getCreatureByUID(cid);
	if(!target)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	target->convinceCreature(creature);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterTargetList(lua_State* L)
{
	//getMonsterTargetList(cid)
	ScriptEnviroment* env = getEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const CreatureList& targetList = monster->getTargetList();
	CreatureList::const_iterator it = targetList.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != targetList.end(); ++it, ++i)
	{
		if(monster->isTarget(*it))
		{
			lua_pushnumber(L, i);
			lua_pushnumber(L, env->addThing(*it));
			pushTable(L);
		}
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterFriendList(lua_State* L)
{
	//getMonsterFriendList(cid)
	ScriptEnviroment* env = getEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* friendCreature;
	const CreatureList& friendList = monster->getFriendList();
	CreatureList::const_iterator it = friendList.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != friendList.end(); ++it, ++i)
	{
		friendCreature = (*it);
		if(!friendCreature->isRemoved() && friendCreature->getPosition().z == monster->getPosition().z)
		{
			lua_pushnumber(L, i);
			lua_pushnumber(L, env->addThing(*it));
			pushTable(L);
		}
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoMonsterSetTarget(lua_State* L)
{
	//doMonsterSetTarget(cid, target)
	uint32_t targetId = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		errorEx(getError(LUA_ERROR_MONSTER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* target = env->getCreatureByUID(targetId);
	if(!target)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(!monster->isSummon())
		lua_pushboolean(L, monster->selectTarget(target));
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDoMonsterChangeTarget(lua_State* L)
{
	//doMonsterChangeTarget(cid)
	ScriptEnviroment* env = getEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(!monster->isSummon())
		monster->searchTarget(TARGETSEARCH_RANDOM);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterInfo(lua_State* L)
{
	//getMonsterInfo(name)
	const MonsterType* mType = g_monsters.getMonsterType(popString(L));
	if(!mType)
	{
		errorEx(getError(LUA_ERROR_MONSTER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setField(L, "name", mType->name.c_str());
	setField(L, "description", mType->nameDescription.c_str());
	setField(L, "experience", mType->experience);
	setField(L, "health", mType->health);
	setField(L, "healthMax", mType->healthMax);
	setField(L, "manaCost", mType->manaCost);
	setField(L, "defense", mType->defense);
	setField(L, "armor", mType->armor);
	setField(L, "baseSpeed", mType->baseSpeed);
	setField(L, "lookCorpse", mType->lookCorpse);
	setField(L, "race", mType->race);
	setField(L, "skull", mType->skull);
	setField(L, "partyShield", mType->partyShield);
	setFieldBool(L, "summonable", mType->isSummonable);
	setFieldBool(L, "illusionable", mType->isIllusionable);
	setFieldBool(L, "convinceable", mType->isConvinceable);
	setFieldBool(L, "attackable", mType->isAttackable);
	setFieldBool(L, "hostile", mType->isHostile);
	createTable(L, "defenses");

	SpellList::const_iterator it = mType->spellDefenseList.begin();
	for(uint32_t i = 1; it != mType->spellDefenseList.end(); ++it, ++i)
	{
		createTable(L, i);
		setField(L, "speed", it->speed);
		setField(L, "chance", it->chance);
		setField(L, "range", it->range);

		setField(L, "minCombatValue", it->minCombatValue);
		setField(L, "maxCombatValue", it->maxCombatValue);
		setFieldBool(L, "isMelee", it->isMelee);
		pushTable(L);
	}

	pushTable(L);
	createTable(L, "attacks");

	it = mType->spellAttackList.begin();
	for(uint32_t i = 1; it != mType->spellAttackList.end(); ++it, ++i)
	{
		createTable(L, i);
		setField(L, "speed", it->speed);
		setField(L, "chance", it->chance);
		setField(L, "range", it->range);

		setField(L, "minCombatValue", it->minCombatValue);
		setField(L, "maxCombatValue", it->maxCombatValue);
		setFieldBool(L, "isMelee", it->isMelee);
		pushTable(L);
	}

	pushTable(L);
	createTable(L, "loot");

	LootItems::const_iterator lit = mType->lootItems.begin();
	for(uint32_t i = 1; lit != mType->lootItems.end(); ++lit, ++i)
	{
		createTable(L, i);
		if(lit->ids.size() > 1)
		{
			createTable(L, "ids");
			std::vector<uint16_t>::const_iterator iit = lit->ids.begin();
			for(uint32_t j = 1; iit != lit->ids.end(); ++iit, ++j)
			{
				lua_pushnumber(L, j);
				lua_pushnumber(L, (*iit));
				pushTable(L);
			}

			pushTable(L);
		}
		else
			setField(L, "id", lit->ids[0]);

		setField(L, "count", lit->count);
		setField(L, "chance", lit->chance);
		setField(L, "subType", lit->subType);
		setField(L, "actionId", lit->actionId);
		setField(L, "uniqueId", lit->uniqueId);
		setField(L, "text", lit->text);

		if(lit->childLoot.size() > 0)
		{
			createTable(L, "child");
			LootItems::const_iterator cit = lit->childLoot.begin();
			for(uint32_t j = 1; cit != lit->childLoot.end(); ++cit, ++j)
			{
				createTable(L, j);
				if(cit->ids.size() > 1)
				{
					createTable(L, "ids");
					std::vector<uint16_t>::const_iterator iit = cit->ids.begin();
					for(uint32_t k = 1; iit != cit->ids.end(); ++iit, ++k)
					{
						lua_pushnumber(L, k);
						lua_pushnumber(L, (*iit));
						pushTable(L);
					}

					pushTable(L);
				}
				else
					setField(L, "id", cit->ids[0]);

				setField(L, "count", cit->count);
				setField(L, "chance", cit->chance);
				setField(L, "subType", cit->subType);
				setField(L, "actionId", cit->actionId);
				setField(L, "uniqueId", cit->uniqueId);
				setField(L, "text", cit->text);

				pushTable(L);
			}

			pushTable(L);
		}

		pushTable(L);
	}

	pushTable(L);
	createTable(L, "summons");

	SummonList::const_iterator sit = mType->summonList.begin();
	for(uint32_t i = 1; sit != mType->summonList.end(); ++sit, ++i)
	{
		createTable(L, i);
		setField(L, "name", sit->name);
		setField(L, "chance", sit->chance);

		setField(L, "interval", sit->interval);
		setField(L, "amount", sit->amount);
		pushTable(L);
	}

	pushTable(L);
	return 1;
}

int32_t LuaScriptInterface::luaGetTalkActionList(lua_State* L)
{
	//getTalkactionList()
	TalkActionsMap::const_iterator it = g_talkActions->getFirstTalk();
	lua_newtable(L);
	for(uint32_t i = 1; it != g_talkActions->getLastTalk(); ++it, ++i)
	{
		createTable(L, i);
		setField(L, "words", it->first);
		setField(L, "access", it->second->getAccess());

		setFieldBool(L, "log", it->second->isLogged());
		setFieldBool(L, "hide", it->second->isHidden());

		setField(L, "channel", it->second->getChannel());
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetExperienceStageList(lua_State* L)
{
	//getExperienceStageList()
	if(!g_config.getBool(ConfigManager::EXPERIENCE_STAGES))
	{
		lua_pushboolean(L, false);
		return true;
	}

	StageList::const_iterator it = g_game.getFirstStage();
	lua_newtable(L);
	for(uint32_t i = 1; it != g_game.getLastStage(); ++it, ++i)
	{
		createTable(L, i);
		setField(L, "level", it->first);
		setFieldFloat(L, "multiplier", it->second);
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoAddCondition(lua_State* L)
{
	//doAddCondition(cid, condition)
	uint32_t conditionId = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Condition* condition = env->getConditionObject(conditionId);
	if(!condition)
	{
		errorEx(getError(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	creature->addCondition(condition->clone());
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveCondition(lua_State* L)
{
	//doRemoveCondition(cid, type[, subId])
	uint32_t subId = 0;
	if(lua_gettop(L) > 2)
		subId = popNumber(L);

	ConditionType_t conditionType = (ConditionType_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Condition* condition = creature->getCondition(conditionType, CONDITIONID_COMBAT, subId);
	if(!condition)
		condition = creature->getCondition(conditionType, CONDITIONID_DEFAULT, subId);

	if(condition)
		creature->removeCondition(condition);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveConditions(lua_State* L)
{
	//doRemoveConditions(cid[, onlyPersistent])
	bool onlyPersistent = true;
	if(lua_gettop(L) > 1)
		onlyPersistent = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	creature->removeConditions(CONDITIONEND_ABORT, onlyPersistent);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaNumberToVariant(lua_State* L)
{
	//numberToVariant(number)
	LuaVariant var;
	var.type = VARIANT_NUMBER;
	var.number = popNumber(L);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaStringToVariant(lua_State* L)
{
	//stringToVariant(string)
	LuaVariant var;
	var.type = VARIANT_STRING;
	var.text = popString(L);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaPositionToVariant(lua_State* L)
{
	//positionToVariant(pos)
	LuaVariant var;
	var.type = VARIANT_POSITION;
	popPosition(L, var.pos);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaTargetPositionToVariant(lua_State* L)
{
	//targetPositionToVariant(pos)
	LuaVariant var;
	var.type = VARIANT_TARGETPOSITION;
	popPosition(L, var.pos);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaVariantToNumber(lua_State* L)
{
	//variantToNumber(var)
	LuaVariant var = popVariant(L);

	uint32_t number = 0;
	if(var.type == VARIANT_NUMBER)
		number = var.number;

	lua_pushnumber(L, number);
	return 1;
}

int32_t LuaScriptInterface::luaVariantToString(lua_State* L)
{
	//variantToString(var)
	LuaVariant var = popVariant(L);

	std::string text = "";
	if(var.type == VARIANT_STRING)
		text = var.text;

	lua_pushstring(L, text.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaVariantToPosition(lua_State* L)
{
	//luaVariantToPosition(var)
	LuaVariant var = popVariant(L);

	PositionEx pos(0, 0, 0, 0);
	if(var.type == VARIANT_POSITION || var.type == VARIANT_TARGETPOSITION)
		pos = var.pos;

	pushPosition(L, pos, pos.stackpos);
	return 1;
}

int32_t LuaScriptInterface::luaDoChangeSpeed(lua_State* L)
{
	//doChangeSpeed(cid, delta)
	int32_t delta = (int32_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		g_game.changeSpeed(creature, delta);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetCreatureOutfit(lua_State* L)
{
	//doSetCreatureOutfit(cid, outfit, time)
	int32_t time = (int32_t)popNumber(L);
	Outfit_t outfit = popOutfit(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, Spell::CreateIllusion(creature, outfit, time) == RET_NOERROR);
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureOutfit(lua_State* L)
{
	//getCreatureOutfit(cid)
	ScriptEnviroment* env = getEnv();
	if(const Creature* creature = env->getCreatureByUID(popNumber(L)))
		pushOutfit(L, creature->getCurrentOutfit());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetMonsterOutfit(lua_State* L)
{
	//doSetMonsterOutfit(cid, name, time)
	int32_t time = (int32_t)popNumber(L);
	std::string name = popString(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, Spell::CreateIllusion(creature, name, time) == RET_NOERROR);
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetItemOutfit(lua_State* L)
{
	//doSetItemOutfit(cid, item, time)
	int32_t time = (int32_t)popNumber(L);
	uint32_t item = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, Spell::CreateIllusion(creature, item, time) == RET_NOERROR);
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetStorage(lua_State* L)
{
	//getStorage(key)
	ScriptEnviroment* env = getEnv();
	std::string strValue;
	if(env->getStorage(popNumber(L), strValue))
	{
		int32_t intValue = atoi(strValue.c_str());
		if(intValue || strValue == "0")
			lua_pushnumber(L, intValue);
		else
			lua_pushstring(L, strValue.c_str());
	}
	else
		lua_pushnumber(L, -1);

	return 1;
}

int32_t LuaScriptInterface::luaDoSetStorage(lua_State* L)
{
	//doSetStorage(value, key)
	std::string value;
	bool nil = false;
	if(lua_isnil(L, -1))
	{
		nil = true;
		lua_pop(L, 1);
	}
	else
		value = popString(L);

	ScriptEnviroment* env = getEnv();
	if(!nil)
		env->setStorage(popNumber(L), value);
	else
		env->eraseStorage(popNumber(L));

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerDepotItems(lua_State* L)
{
	//getPlayerDepotItems(cid, depotid)
	uint32_t depotid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(const Depot* depot = player->getDepot(depotid, true))
			lua_pushnumber(L, depot->getItemHoldingCount());
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGuildId(lua_State* L)
{
	//doPlayerSetGuildId(cid, id)
	uint32_t id = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(player->guildId)
		{
			player->leaveGuild();
			if(!id)
				lua_pushboolean(L, true);
			else if(IOGuild::getInstance()->guildExists(id))
				lua_pushboolean(L, IOGuild::getInstance()->joinGuild(player, id));
			else
				lua_pushboolean(L, false);
		}
		else if(id && IOGuild::getInstance()->guildExists(id))
			lua_pushboolean(L, IOGuild::getInstance()->joinGuild(player, id));
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGuildLevel(lua_State* L)
{
	//doPlayerSetGuildLevel(cid, level[, rank])
	uint32_t rank = 0;
	if(lua_gettop(L) > 2)
		rank = popNumber(L);

	GuildLevel_t level = (GuildLevel_t)popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushboolean(L, player->setGuildLevel(level, rank));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGuildNick(lua_State* L)
{
	//doPlayerSetGuildNick(cid, nick)
	std::string nick = popString(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setGuildNick(nick);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetGuildId(lua_State* L)
{
	//getGuildId(guildName)
	uint32_t guildId;
	if(IOGuild::getInstance()->getGuildId(guildId, popString(L)))
		lua_pushnumber(L, guildId);
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetGuildMotd(lua_State* L)
{
	//getGuildMotd(guildId)
	uint32_t guildId = popNumber(L);
	if(IOGuild::getInstance()->guildExists(guildId))
		lua_pushstring(L, IOGuild::getInstance()->getMotd(guildId).c_str());
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDoMoveCreature(lua_State* L)
{
	//doMoveCreature(cid, direction)
	uint32_t direction = popNumber(L);
	if(direction < NORTH || direction > NORTHEAST)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, g_game.internalMoveCreature(creature, (Direction)direction, FLAG_NOLIMIT));
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaIsCreature(lua_State* L)
{
	//isCreature(cid)
	ScriptEnviroment* env = getEnv();
	lua_pushboolean(L, env->getCreatureByUID(popNumber(L)) ? true : false);
	return 1;
}

int32_t LuaScriptInterface::luaIsContainer(lua_State* L)
{
	//isContainer(uid)
	ScriptEnviroment* env = getEnv();
	lua_pushboolean(L, env->getContainerByUID(popNumber(L)) ? true : false);
	return 1;
}

int32_t LuaScriptInterface::luaIsMovable(lua_State* L)
{
	//isMovable(uid)
	ScriptEnviroment* env = getEnv();
	Thing* thing = env->getThingByUID(popNumber(L));
	if(thing && thing->isPushable())
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureByName(lua_State* L)
{
	//getCreatureByName(name)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = g_game.getCreatureByName(popString(L)))
		lua_pushnumber(L, env->addThing(creature));
	else
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerByGUID(lua_State* L)
{
	//getPlayerByGUID(guid)
	ScriptEnviroment* env = getEnv();
	if(Player* player = g_game.getPlayerByGuid(popNumber(L)))
		lua_pushnumber(L, env->addThing(player));
	else
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerByNameWildcard(lua_State* L)
{
	//getPlayerByNameWildcard(name~[, ret = false])
	Player* player = NULL;
	bool pushRet = false;
	if(lua_gettop(L) > 1)
		pushRet = popNumber(L);

	ScriptEnviroment* env = getEnv();
	ReturnValue ret = g_game.getPlayerByNameWildcard(popString(L), player);
	if(ret == RET_NOERROR)
		lua_pushnumber(L, env->addThing(player));
	else if(pushRet)
		lua_pushnumber(L, ret);
	else
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerGUIDByName(lua_State* L)
{
	//getPlayerGUIDByName(name[, multiworld = false])
	bool multiworld = false;
	if(lua_gettop(L) > 1)
		multiworld = popNumber(L);

	std::string name = popString(L);
	uint32_t guid;
	if(Player* player = g_game.getPlayerByName(name.c_str()))
		lua_pushnumber(L, player->getGUID());
	else if(IOLoginData::getInstance()->getGuidByName(guid, name, multiworld))
		lua_pushnumber(L, guid);
	else
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerNameByGUID(lua_State* L)
{
	//getPlayerNameByGUID(guid[, multiworld = false[, displayError = true]])
	int32_t parameters = lua_gettop(L);
	bool multiworld = false, displayError = true;

	if(parameters > 2)
		displayError = popNumber(L);

	if(parameters > 1)
		multiworld = popNumber(L);

	uint32_t guid = popNumber(L);
	std::string name;
	if(!IOLoginData::getInstance()->getNameByGuid(guid, name, multiworld))
	{
		if(displayError)
			errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));

		lua_pushnil(L);
		return 1;
	}

	lua_pushstring(L, name.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayersByAccountId(lua_State* L)
{
	//getPlayersByAccountId(accId)
	PlayerVector players = g_game.getPlayersByAccount(popNumber(L));

	ScriptEnviroment* env = getEnv();
	PlayerVector::iterator it = players.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != players.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(*it));
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetIpByName(lua_State* L)
{
	//getIpByName(name)
	std::string name = popString(L);

	if(Player* player = g_game.getPlayerByName(name))
		lua_pushnumber(L, player->getIP());
	else
		lua_pushnumber(L, IOLoginData::getInstance()->getLastIPByName(name));

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayersByIp(lua_State* L)
{
	//getPlayersByIp(ip[, mask])
	uint32_t mask = 0xFFFFFFFF;
	if(lua_gettop(L) > 1)
		mask = (uint32_t)popNumber(L);

	PlayerVector players = g_game.getPlayersByIP(popNumber(L), mask);

	ScriptEnviroment* env = getEnv();
	PlayerVector::iterator it = players.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != players.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(*it));
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetAccountIdByName(lua_State* L)
{
	//getAccountIdByName(name)
	std::string name = popString(L);

	if(Player* player = g_game.getPlayerByName(name))
		lua_pushnumber(L, player->getAccount());
	else
		lua_pushnumber(L, IOLoginData::getInstance()->getAccountIdByName(name));

	return 1;
}

int32_t LuaScriptInterface::luaGetAccountByName(lua_State* L)
{
	//getAccountByName(name)
	std::string name = popString(L);

	if(Player* player = g_game.getPlayerByName(name))
		lua_pushstring(L, player->getAccountName().c_str());
	else
	{
		std::string tmp;
		IOLoginData::getInstance()->getAccountName(IOLoginData::getInstance()->getAccountIdByName(name), tmp);
		lua_pushstring(L, tmp.c_str());
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetAccountIdByAccount(lua_State* L)
{
	//getAccountIdByAccount(accName)
	uint32_t value = 0;
	IOLoginData::getInstance()->getAccountId(popString(L), value);
	lua_pushnumber(L, value);
	return 1;
}

int32_t LuaScriptInterface::luaGetAccountByAccountId(lua_State* L)
{
	//getAccountByAccountId(accId)
	std::string value = 0;
	IOLoginData::getInstance()->getAccountName(popNumber(L), value);
	lua_pushstring(L, value.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaRegisterCreatureEvent(lua_State* L)
{
	//registerCreatureEvent(cid, name)
	std::string name = popString(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, creature->registerCreatureEvent(name));
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetContainerSize(lua_State* L)
{
	//getContainerSize(uid)
	ScriptEnviroment* env = getEnv();
	if(Container* container = env->getContainerByUID(popNumber(L)))
		lua_pushnumber(L, container->size());
	else
	{
		errorEx(getError(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetContainerCap(lua_State* L)
{
	//getContainerCap(uid)
	ScriptEnviroment* env = getEnv();
	if(Container* container = env->getContainerByUID(popNumber(L)))
		lua_pushnumber(L, container->capacity());
	else
	{
		errorEx(getError(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetContainerItem(lua_State* L)
{
	//getContainerItem(uid, slot)
	uint32_t slot = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Container* container = env->getContainerByUID(popNumber(L)))
	{
		if(Item* item = container->getItem(slot))
			pushThing(L, item, env->addThing(item));
		else
			pushThing(L, NULL, 0);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CONTAINER_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;

}

int32_t LuaScriptInterface::luaDoAddContainerItemEx(lua_State* L)
{
	//doAddContainerItemEx(uid, virtuid)
	uint32_t virtuid = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Container* container = env->getContainerByUID(popNumber(L)))
	{
		Item* item = env->getItemByUID(virtuid);
		if(!item)
		{
			errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}

		if(item->getParent() != VirtualCylinder::virtualCylinder)
		{
			lua_pushboolean(L, false);
			return 1;
		}

		ReturnValue ret = g_game.internalAddItem(NULL, container, item);
		if(ret == RET_NOERROR)
			env->removeTempItem(item);

		lua_pushnumber(L, ret);
		return 1;
	}
	else
	{
		errorEx(getError(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}
}

int32_t LuaScriptInterface::luaDoAddContainerItem(lua_State* L)
{
	//doAddContainerItem(uid, itemid[, count/subType])
	uint32_t count = 1;
	if(lua_gettop(L) > 2)
		count = popNumber(L);

	uint16_t itemId = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Container* container = env->getContainerByUID((uint32_t)popNumber(L));
	if(!container)
	{
		errorEx(getError(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const ItemType& it = Item::items[itemId];
	int32_t itemCount = 1, subType = 1;
	if(it.hasSubType())
	{
		if(it.stackable)
			itemCount = (int32_t)std::ceil((float)count / 100);

		subType = count;
	}
	else
		itemCount = std::max((uint32_t)1, count);

	while(itemCount > 0)
	{
		int32_t stackCount = std::min(100, subType);
		Item* newItem = Item::CreateItem(itemId, stackCount);
		if(!newItem)
		{
			errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
			lua_pushboolean(L, false);
			return 1;
		}

		if(it.stackable)
			subType -= stackCount;

		ReturnValue ret = g_game.internalAddItem(NULL, container, newItem);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			lua_pushboolean(L, false);
			return 1;
		}

		--itemCount;
		if(itemCount)
			continue;

		if(newItem->getParent())
			lua_pushnumber(L, env->addThing(newItem));
		else //stackable item stacked with existing object, newItem will be released
			lua_pushnil(L);

		return 1;
	}

	lua_pushnil(L);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddOutfit(lua_State *L)
{
	//Consider using doPlayerAddOutfitId instead
	//doPlayerAddOutfit(cid, looktype, addon)
	uint32_t addon = popNumber(L), lookType = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID((uint32_t)popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Outfit outfit;
	if(Outfits::getInstance()->getOutfit(lookType, outfit))
	{
		lua_pushboolean(L, player->addOutfit(outfit.outfitId, addon));
		return 1;
	}

	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemoveOutfit(lua_State *L)
{
	//Consider using doPlayerRemoveOutfitId instead
	//doPlayerRemoveOutfit(cid, looktype[, addon = 0])
	uint32_t addon = 0xFF;
	if(lua_gettop(L) > 2)
		addon = popNumber(L);

	uint32_t lookType = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID((uint32_t)popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Outfit outfit;
	if(Outfits::getInstance()->getOutfit(lookType, outfit))
	{
		lua_pushboolean(L, player->removeOutfit(outfit.outfitId, addon));
		return 1;
	}

	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddOutfitId(lua_State *L)
{
	//doPlayerAddOutfitId(cid, outfitId, addon)
	uint32_t addon = popNumber(L), outfitId = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID((uint32_t)popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, player->addOutfit(outfitId, addon));
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemoveOutfitId(lua_State *L)
{
	//doPlayerRemoveOutfitId(cid, outfitId[, addon = 0])
	uint32_t addon = 0xFF;
	if(lua_gettop(L) > 2)
		addon = popNumber(L);

	uint32_t outfitId = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID((uint32_t)popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, player->removeOutfit(outfitId, addon));
	return 1;
}

int32_t LuaScriptInterface::luaCanPlayerWearOutfit(lua_State* L)
{
	//canPlayerWearOutfit(cid, looktype[, addon = 0])
	uint32_t addon = 0;
	if(lua_gettop(L) > 2)
		addon = popNumber(L);

	uint32_t lookType = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Outfit outfit;
	if(Outfits::getInstance()->getOutfit(lookType, outfit))
	{
		lua_pushboolean(L, player->canWearOutfit(outfit.outfitId, addon));
		return 1;
	}

	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaCanPlayerWearOutfitId(lua_State* L)
{
	//canPlayerWearOutfitId(cid, outfitId[, addon = 0])
	uint32_t addon = 0;
	if(lua_gettop(L) > 2)
		addon = popNumber(L);

	uint32_t outfitId = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, player->canWearOutfit(outfitId, addon));
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureChangeOutfit(lua_State* L)
{
	//doCreatureChangeOutfit(cid, outfit)
	Outfit_t outfit = popOutfit(L);
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(Player* player = creature->getPlayer())
			player->changeOutfit(outfit, false);
		else
			creature->defaultOutfit = outfit;

		if(!creature->hasCondition(CONDITION_OUTFIT, 1))
			g_game.internalCreatureChangeOutfit(creature, outfit);

		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSetCreatureLight(lua_State* L)
{
	//doSetCreatureLight(cid, lightLevel, lightColor, time)
	uint32_t time = popNumber(L);
	uint8_t color = (uint8_t)popNumber(L), level = (uint8_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_LIGHT, time, level | (color << 8));
		creature->addCondition(condition);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerPopupFYI(lua_State* L)
{
	//doPlayerPopupFYI(cid, message)
	std::string message = popString(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->sendFYIBox(message);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendTutorial(lua_State* L)
{
	//doPlayerSendTutorial(cid, id)
	uint8_t id = (uint8_t)popNumber(L);

	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	player->sendTutorial(id);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendMailByName(lua_State* L)
{
	//doPlayerSendMailByName(name, item[, town[, actor]])
	ScriptEnviroment* env = getEnv();
	int32_t params = lua_gettop(L);

	Creature* actor = NULL;
	if(params > 3)
		actor = env->getCreatureByUID(popNumber(L));

	uint32_t town = 0;
	if(params > 2)
		town = popNumber(L);

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(item->getParent() != VirtualCylinder::virtualCylinder)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, IOLoginData::getInstance()->playerMail(actor, popString(L), town, item));
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddMapMark(lua_State* L)
{
	//doPlayerAddMapMark(cid, pos, type[, description])
	std::string description;
	if(lua_gettop(L) > 3)
		description = popString(L);

	MapMarks_t type = (MapMarks_t)popNumber(L);
	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	player->sendAddMarker(pos, type, description);
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddPremiumDays(lua_State* L)
{
	//doPlayerAddPremiumDays(cid, days)
	int32_t days = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(player->premiumDays < 65535)
		{
			Account account = IOLoginData::getInstance()->loadAccount(player->getAccount());
			if(days < 0)
			{
				account.premiumDays = std::max((uint32_t)0, uint32_t(account.premiumDays + (int32_t)days));
				player->premiumDays = std::max((uint32_t)0, uint32_t(player->premiumDays + (int32_t)days));
			}
			else
			{
				account.premiumDays = std::min((uint32_t)65534, uint32_t(account.premiumDays + (uint32_t)days));
				player->premiumDays = std::min((uint32_t)65534, uint32_t(player->premiumDays + (uint32_t)days));
			}
			IOLoginData::getInstance()->saveAccount(account);
		}
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureLastPosition(lua_State* L)
{
	//getCreatureLastPosition(cid)
	ScriptEnviroment* env = getEnv();

	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		pushPosition(L, creature->getLastPosition(), 0);
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureName(lua_State* L)
{
	//getCreatureName(cid)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushstring(L, creature->getName().c_str());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureNoMove(lua_State* L)
{
	//getCreatureNoMove(cid)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, creature->getNoMove());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureSkullType(lua_State* L)
{
	//getCreatureSkullType(cid[, target])
	uint32_t tid = 0;
	if(lua_gettop(L) > 1)
		tid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(!tid)
			lua_pushnumber(L, creature->getSkull());
		else if(Creature* target = env->getCreatureByUID(tid))
			lua_pushnumber(L, creature->getSkullClient(target));
		else
		{
			errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, false);
		}
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetLookDir(lua_State* L)
{
	//doCreatureSetLookDirection(cid, dir)
	Direction dir = (Direction)popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(dir < NORTH || dir > WEST)
		{
			lua_pushboolean(L, false);
			return 1;
		}

		g_game.internalCreatureTurn(creature, dir);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetSkullType(lua_State* L)
{
	//doCreatureSetSkullType(cid, skull)
	Skulls_t skull = (Skulls_t)popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		creature->setSkull(skull);
		g_game.updateCreatureSkull(creature);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetSkullEnd(lua_State* L)
{
	//doPlayerSetSkullEnd(cid, time, type)
	Skulls_t _skull = (Skulls_t)popNumber(L);
	time_t _time = (time_t)std::max((int64_t)0, popNumber(L));

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setSkullEnd(_time, false, _skull);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureSpeed(lua_State* L)
{
	//getCreatureSpeed(cid)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getSpeed());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureBaseSpeed(lua_State* L)
{
	//getCreatureBaseSpeed(cid)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getBaseSpeed());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureTarget(lua_State* L)
{
	//getCreatureTarget(cid)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		Creature* target = creature->getAttackedCreature();
		lua_pushnumber(L, target ? env->addThing(target) : 0);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaIsSightClear(lua_State* L)
{
	//isSightClear(fromPos, toPos, floorCheck)
	PositionEx fromPos, toPos;
	bool floorCheck = popNumber(L);

	popPosition(L, toPos);
	popPosition(L, fromPos);

	lua_pushboolean(L, g_game.isSightClear(fromPos, toPos, floorCheck));
	return 1;
}

int32_t LuaScriptInterface::luaIsInArray(lua_State* L)
{
	//isInArray(array, value[, caseSensitive = false])
	bool caseSensitive = false;
	if(lua_gettop(L) > 2)
		caseSensitive = popNumber(L);

	boost::any value;
	if(lua_isnumber(L, -1))
		value = popFloatNumber(L);
	else if(lua_isboolean(L, -1))
		value = popBoolean(L);
	else if(lua_isstring(L, -1))
		value = popString(L);
	else
	{
		lua_pop(L, 1);
		lua_pushboolean(L, false);
		return 1;
	}

	const std::type_info& type = value.type();
	if(!caseSensitive && type == typeid(std::string))
		value = asLowerCaseString(boost::any_cast<std::string>(value));

	if(!lua_istable(L, -1))
	{
		boost::any data;
		if(lua_isnumber(L, -1))
			data = popFloatNumber(L);
		else if(lua_isboolean(L, -1))
			data = popBoolean(L);
		else if(lua_isstring(L, -1))
			data = popString(L);
		else
		{
			lua_pop(L, 1);
			lua_pushboolean(L, false);
			return 1;
		}

		if(type != data.type()) // check is it even same data type before searching deeper
			lua_pushboolean(L, false);
		else if(type == typeid(bool))
			lua_pushboolean(L, boost::any_cast<bool>(value) == boost::any_cast<bool>(data));
		else if(type == typeid(double))
			lua_pushboolean(L, boost::any_cast<double>(value) == boost::any_cast<double>(data));
		else if(caseSensitive)
			lua_pushboolean(L, boost::any_cast<std::string>(value) == boost::any_cast<std::string>(data));
		else
			lua_pushboolean(L, boost::any_cast<std::string>(value) == asLowerCaseString(boost::any_cast<std::string>(data)));

		return 1;
	}

	lua_pushnil(L);
	while(lua_next(L, -2))
	{
		boost::any data;
		if(lua_isnumber(L, -1))
			data = popFloatNumber(L);
		else if(lua_isboolean(L, -1))
			data = popBoolean(L);
		else if(lua_isstring(L, -1))
			data = popString(L);
		else
		{
			lua_pop(L, 1);
			break;
		}

		if(type != data.type()) // check is it same data type before searching deeper
			continue;

		if(type == typeid(bool))
		{
			if(boost::any_cast<bool>(value) != boost::any_cast<bool>(data))
				continue;

			lua_pushboolean(L, true);
			return 1;
		}
		else if(type == typeid(double))
		{
			if(boost::any_cast<double>(value) != boost::any_cast<double>(data))
				continue;

			lua_pushboolean(L, true);
			return 1;
		}
		else if(caseSensitive)
		{
			if(boost::any_cast<std::string>(value) != boost::any_cast<std::string>(data))
				continue;

			lua_pushboolean(L, true);
			return 1;
		}
		else if(boost::any_cast<std::string>(value) == asLowerCaseString(boost::any_cast<std::string>(data)))
		{
			lua_pushboolean(L, true);
			return 1;
		}
	}

	lua_pop(L, 2);
	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaAddEvent(lua_State* L)
{
	//addEvent(callback, delay, ...)
	ScriptEnviroment* env = getEnv();
	LuaScriptInterface* interface = env->getInterface();
	if(!interface)
	{
		errorEx("No valid script interface!");
		lua_pushboolean(L, false);
		return 1;
	}

	int32_t parameters = lua_gettop(L);
	if(!lua_isfunction(L, -parameters)) //-parameters means the first parameter from left to right
	{
		errorEx("Callback parameter should be a function.");
		lua_pushboolean(L, false);
		return 1;
	}

	std::list<int32_t> params;
	for(int32_t i = 0; i < parameters - 2; ++i) //-2 because addEvent needs at least two parameters
		params.push_back(luaL_ref(L, LUA_REGISTRYINDEX));

	uint32_t delay = std::max((int64_t)SCHEDULER_MINTICKS, popNumber(L));
	LuaTimerEvent eventDesc;

	eventDesc.parameters = params;
	eventDesc.function = luaL_ref(L, LUA_REGISTRYINDEX);
	eventDesc.scriptId = env->getScriptId();

	interface->m_timerEvents[++interface->m_lastEventTimerId] = eventDesc;
	Scheduler::getInstance().addEvent(createSchedulerTask(delay, boost::bind(
		&LuaScriptInterface::executeTimer, interface, interface->m_lastEventTimerId)));

	lua_pushnumber(L, interface->m_lastEventTimerId);
	return 1;
}

int32_t LuaScriptInterface::luaStopEvent(lua_State* L)
{
	//stopEvent(eventid)
	uint32_t eventId = popNumber(L);
	ScriptEnviroment* env = getEnv();

	LuaScriptInterface* interface = env->getInterface();
	if(!interface)
	{
		errorEx("No valid script interface!");
		lua_pushboolean(L, false);
		return 1;
	}

	LuaTimerEvents::iterator it = interface->m_timerEvents.find(eventId);
	if(it != interface->m_timerEvents.end())
	{
		for(std::list<int32_t>::iterator lt = it->second.parameters.begin(); lt != it->second.parameters.end(); ++lt)
			luaL_unref(interface->m_luaState, LUA_REGISTRYINDEX, *lt);
		it->second.parameters.clear();

		luaL_unref(interface->m_luaState, LUA_REGISTRYINDEX, it->second.function);
		interface->m_timerEvents.erase(it);
		lua_pushboolean(L, true);
	}
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureCondition(lua_State* L)
{
	//getCreatureCondition(cid, condition[, subId])
	uint32_t subId = 0, condition = 0;
	if(lua_gettop(L) > 2)
		subId = popNumber(L);

	condition = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, creature->hasCondition((ConditionType_t)condition, subId));
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerBlessing(lua_State* L)
{
	//getPlayerBlessings(cid, blessing)
	int16_t blessing = popNumber(L) - 1;

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushboolean(L, player->hasBlessing(blessing));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddBlessing(lua_State* L)
{
	//doPlayerAddBlessing(cid, blessing)
	int16_t blessing = popNumber(L) - 1;
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(!player->hasBlessing(blessing))
		{
			player->addBlessing(1 << blessing);
			lua_pushboolean(L, true);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetPromotionLevel(lua_State* L)
{
	//doPlayerSetPromotionLevel(cid, level)
	uint32_t level = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setPromotionLevel(level);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGroupId(lua_State* L)
{
	//doPlayerSetGroupId(cid, groupId)
	uint32_t groupId = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(Group* group = Groups::getInstance()->getGroup(groupId))
		{
			player->setGroup(group);
			lua_pushboolean(L, true);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMana(lua_State* L)
{
	//getCreatureMana(cid)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getMana());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMaxMana(lua_State* L)
{
	//getCreatureMaxMana(cid)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getMaxMana());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureHealth(lua_State* L)
{
	//getCreatureHealth(cid)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getHealth());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureLookDirection(lua_State* L)
{
	//getCreatureLookDirection(cid)
	ScriptEnviroment* env = getEnv();

	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getDirection());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMaxHealth(lua_State* L)
{
	//getCreatureMaxHealth(cid)
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getMaxHealth());
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetStamina(lua_State* L)
{
	//doPlayerSetStamina(cid, minutes)
	uint32_t minutes = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setStaminaMinutes(minutes);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddStamina(lua_State* L)
{
	//doPlayerAddStamina(cid, minutes)
	int32_t minutes = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setStaminaMinutes(player->getStaminaMinutes() + minutes);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetBalance(lua_State* L)
{
	//doPlayerSetBalance(cid, balance)
	uint32_t balance = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->balance = balance;
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetPartner(lua_State* L)
{
	//doPlayerSetPartner(cid, guid)
	uint32_t guid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->marriage = guid;
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerParty(lua_State* L)
{
	//getPlayerParty(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(cid))
	{
		if(Party* party = player->getParty())
			lua_pushnumber(L, env->addThing(party->getLeader()));
		else
			lua_pushnil(L);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerJoinParty(lua_State* L)
{
	//doPlayerJoinParty(cid, lid)
	ScriptEnviroment* env = getEnv();

	Player* leader = env->getPlayerByUID(popNumber(L));
	if(!leader)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	g_game.playerJoinParty(player->getID(), leader->getID());
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetPartyMembers(lua_State* L)
{
	//getPartyMembers(cid)
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(Party* party = player->getParty())
		{
			PlayerVector list = party->getMembers();
			list.push_back(party->getLeader());

			PlayerVector::const_iterator it = list.begin();
			lua_newtable(L);
			for(uint32_t i = 1; it != list.end(); ++it, ++i)
			{
				lua_pushnumber(L, i);
				lua_pushnumber(L, (*it)->getID());
				pushTable(L);
			}

			return 1;
		}
	}

	lua_pushboolean(L, false);
	return 1;
}

int32_t LuaScriptInterface::luaGetVocationInfo(lua_State* L)
{
	//getVocationInfo(id)
	uint32_t id = popNumber(L);
	Vocation* voc = Vocations::getInstance()->getVocation(id);
	if(!voc)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setField(L, "id", voc->getId());
	setField(L, "name", voc->getName().c_str());
	setField(L, "description", voc->getDescription().c_str());
	setField(L, "healthGain", voc->getGain(GAIN_HEALTH));
	setField(L, "healthGainTicks", voc->getGainTicks(GAIN_HEALTH));
	setField(L, "healthGainAmount", voc->getGainAmount(GAIN_HEALTH));
	setField(L, "manaGain", voc->getGain(GAIN_MANA));
	setField(L, "manaGainTicks", voc->getGainTicks(GAIN_MANA));
	setField(L, "manaGainAmount", voc->getGainAmount(GAIN_MANA));
	setField(L, "attackSpeed", voc->getAttackSpeed());
	setField(L, "baseSpeed", voc->getBaseSpeed());
	setField(L, "fromVocation", voc->getFromVocation());
	setField(L, "promotedVocation", Vocations::getInstance()->getPromotedVocation(id));
	setField(L, "soul", voc->getGain(GAIN_SOUL));
	setField(L, "soulAmount", voc->getGainAmount(GAIN_SOUL));
	setField(L, "soulTicks", voc->getGainTicks(GAIN_SOUL));
	setField(L, "capacity", voc->getGainCap());
	setFieldBool(L, "attackable", voc->isAttackable());
	setFieldBool(L, "needPremium", voc->isPremiumNeeded());
	setFieldFloat(L, "experienceMultiplier", voc->getExperienceMultiplier());
	return 1;
}

int32_t LuaScriptInterface::luaGetGroupInfo(lua_State* L)
{
	//getGroupInfo(id[, premium])
	bool premium = false;
	if(lua_gettop(L) > 1)
		premium = popNumber(L);

	Group* group = Groups::getInstance()->getGroup(popNumber(L));
	if(!group)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setField(L, "id", group->getId());
	setField(L, "name", group->getName().c_str());
	setField(L, "access", group->getAccess());
	setField(L, "ghostAccess", group->getGhostAccess());
	setField(L, "violationReasons", group->getViolationReasons());
	setField(L, "statementViolationFlags", group->getStatementViolationFlags());
	setField(L, "nameViolationFlags", group->getNameViolationFlags());
	setField(L, "flags", group->getFlags());
	setField(L, "customFlags", group->getCustomFlags());
	setField(L, "depotLimit", group->getDepotLimit(premium));
	setField(L, "maxVips", group->getMaxVips(premium));
	setField(L, "outfit", group->getOutfit());
	return 1;
}

int32_t LuaScriptInterface::luaGetChannelUsers(lua_State* L)
{
	//getChannelUsers(channelId)
	ScriptEnviroment* env = getEnv();
	uint16_t channelId = popNumber(L);

	if(ChatChannel* channel = g_chat.getChannelById(channelId))
	{
		UsersMap usersMap = channel->getUsers();
		UsersMap::iterator it = usersMap.begin();

		lua_newtable(L);
		for(int32_t i = 1; it != usersMap.end(); ++it, ++i)
		{
			lua_pushnumber(L, i);
			lua_pushnumber(L, env->addThing(it->second));
			pushTable(L);
		}
	}
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayersOnline(lua_State* L)
{
	//getPlayersOnline()
	ScriptEnviroment* env = getEnv();
	AutoList<Player>::iterator it = Player::autoList.begin();

	lua_newtable(L);
	for(int32_t i = 1; it != Player::autoList.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(it->second));
		pushTable(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetCreatureMaxHealth(lua_State* L)
{
	//setCreatureMaxHealth(uid, health)
	uint32_t maxHealth = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		creature->changeMaxHealth(maxHealth);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetCreatureMaxMana(lua_State* L)
{
	//setCreatureMaxMana(uid, mana)
	uint32_t maxMana = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		creature->changeMaxMana(maxMana);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetMaxCapacity(lua_State* L)
{
	//doPlayerSetMaxCapacity(uid, cap)
	double cap = popFloatNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setCapacity(cap);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMaster(lua_State* L)
{
	//getCreatureMaster(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Creature* master = creature->getMaster();
	lua_pushnumber(L, master ? env->addThing(master) : cid);
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureSummons(lua_State* L)
{
	//getCreatureSummons(cid)
	ScriptEnviroment* env = getEnv();

	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	const std::list<Creature*>& summons = creature->getSummons();
	CreatureList::const_iterator it = summons.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != summons.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(*it));
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetIdleTime(lua_State* L)
{
	//doPlayerSetIdleTime(cid, amount)
	int64_t amount = popNumber(L);
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setIdleTime(amount);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetNoMove(lua_State* L)
{
	//doCreatureSetNoMove(cid, block)
	bool block = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		creature->setNoMove(block);
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerRates(lua_State* L)
{
	//getPlayerRates(cid)
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	for(uint32_t i = SKILL_FIRST; i <= SKILL__LAST; ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, player->rates[(skills_t)i]);
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetRate(lua_State* L)
{
	//doPlayerSetRate(cid, type, value)
	float value = popFloatNumber(L);
	uint32_t type = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(type <= SKILL__LAST)
		{
			player->rates[(skills_t)type] = value;
			lua_pushboolean(L, true);
		}
		else
			lua_pushboolean(L, false);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSwitchSaving(lua_State* L)
{
	//doPlayerSwitchSaving(cid)
	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->switchSaving();
		lua_pushboolean(L, true);
	}
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSave(lua_State* L)
{
	//doPlayerSave(cid[, shallow = false])
	bool shallow = false;
	if(lua_gettop(L) > 1)
		shallow = popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushboolean(L, IOLoginData::getInstance()->savePlayer(player, false, shallow));
	else
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetTownId(lua_State* L)
{
	//getTownId(townName)
	std::string townName = popString(L);
	if(Town* town = Towns::getInstance()->getTown(townName))
		lua_pushnumber(L, town->getID());
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetTownName(lua_State* L)
{
	//getTownName(townId)
	uint32_t townId = popNumber(L);
	if(Town* town = Towns::getInstance()->getTown(townId))
		lua_pushstring(L, town->getName().c_str());
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetTownTemplePosition(lua_State* L)
{
	//getTownTemplePosition(townId)
	bool displayError = true;
	if(lua_gettop(L) >= 2)
		displayError = popNumber(L);

	uint32_t townId = popNumber(L);
	if(Town* town = Towns::getInstance()->getTown(townId))
		pushPosition(L, town->getPosition(), 255);
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetTownHouses(lua_State* L)
{
	//getTownHouses(townId)
	uint32_t townId = 0;
	if(lua_gettop(L) > 0)
		townId = popNumber(L);

	HouseMap::iterator it = Houses::getInstance()->getHouseBegin();
	lua_newtable(L);
	for(uint32_t i = 1; it != Houses::getInstance()->getHouseEnd(); ++i, ++it)
	{
		if(townId != 0 && it->second->getTownId() != townId)
			continue;

		lua_pushnumber(L, i);
		lua_pushnumber(L, it->second->getId());
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetSpectators(lua_State* L)
{
	//getSpectators(centerPos, rangex, rangey[, multifloor = false])
	bool multifloor = false;
	if(lua_gettop(L) > 3)
		multifloor = popNumber(L);

	uint32_t rangey = popNumber(L), rangex = popNumber(L);
	PositionEx centerPos;
	popPosition(L, centerPos);

	SpectatorVec list;
	g_game.getSpectators(list, centerPos, false, multifloor, rangex, rangex, rangey, rangey);
	if(list.empty())
	{
		lua_pushnil(L);
		return 1;
	}

	ScriptEnviroment* env = getEnv();
	SpectatorVec::const_iterator it = list.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != list.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(*it));
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetHighscoreString(lua_State* L)
{
	//getHighscoreString(skillId)
	uint16_t skillId = popNumber(L);
	if(skillId <= SKILL__LAST)
		lua_pushstring(L, g_game.getHighscoreString(skillId).c_str());
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaGetWaypointList(lua_State* L)
{
	//getWaypointList()
	WaypointMap waypointsMap = g_game.getMap()->waypoints.getWaypointsMap();
	WaypointMap::iterator it = waypointsMap.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != waypointsMap.end(); ++it, ++i)
	{
		createTable(L, i);
		setField(L, "name", it->first);
		setField(L, "pos", it->second->pos.x);
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetWaypointPosition(lua_State* L)
{
	//getWaypointPosition(name)
	if(WaypointPtr waypoint = g_game.getMap()->waypoints.getWaypointByName(popString(L)))
		pushPosition(L, waypoint->pos, 0);
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDoWaypointAddTemporial(lua_State* L)
{
	//doWaypointAddTemporial(name, pos)
	PositionEx pos;
	popPosition(L, pos);

	g_game.getMap()->waypoints.addWaypoint(WaypointPtr(new Waypoint(popString(L), pos)));
	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaGetGameState(lua_State* L)
{
	//getGameState()
	lua_pushnumber(L, g_game.getGameState());
	return 1;
}

int32_t LuaScriptInterface::luaDoSetGameState(lua_State* L)
{
	//doSetGameState(id)
	uint32_t id = popNumber(L);
	if(id >= GAME_STATE_FIRST && id <= GAME_STATE_LAST)
	{
		Dispatcher::getInstance().addTask(createTask(boost::bind(&Game::setGameState, &g_game, (GameState_t)id)));
		lua_pushboolean(L, true);
	}
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureExecuteTalkAction(lua_State* L)
{
	//doCreatureExecuteTalkAction(cid, text[, ignoreAccess[, channelId]])
	uint32_t params = lua_gettop(L), channelId = CHANNEL_DEFAULT;
	if(params > 3)
		channelId = popNumber(L);

	bool ignoreAccess = false;
	if(params > 2)
		ignoreAccess = popNumber(L);

	std::string text = popString(L);
	ScriptEnviroment* env = getEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, g_talkActions->onPlayerSay(creature, channelId, text, ignoreAccess));
	else
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoExecuteRaid(lua_State* L)
{
	//doExecuteRaid(name)
	std::string raidName = popString(L);
	if(Raids::getInstance()->getRunning())
	{
		lua_pushboolean(L, false);
		return 1;
	}

	Raid* raid = Raids::getInstance()->getRaidByName(raidName);
	if(!raid || !raid->isLoaded())
	{
		errorEx("Raid with name " + raidName + " does not exists.");
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, raid->startRaid());
	return 1;
}

int32_t LuaScriptInterface::luaDoReloadInfo(lua_State* L)
{
	//doReloadInfo(id[, cid])
	uint32_t cid = 0;
	if(lua_gettop(L) > 1)
		cid = popNumber(L);

	uint32_t id = popNumber(L);
	if(id >= RELOAD_FIRST && id <= RELOAD_LAST)
	{
		Scheduler::getInstance().addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
			boost::bind(&Game::reloadInfo, &g_game, (ReloadInfo_t)id, cid)));
		lua_pushboolean(L, true);
	}
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDoSaveServer(lua_State* L)
{
	//doSaveServer([shallow])
	bool shallow = false;
	if(lua_gettop(L) > 0)
		shallow = popNumber(L);

	Dispatcher::getInstance().addTask(createTask(boost::bind(&Game::saveGameState, &g_game, shallow)));
	return 1;
}

int32_t LuaScriptInterface::luaDoCleanHouse(lua_State* L)
{
	//doCleanHouse(houseId)
	uint32_t houseId = popNumber(L);
	if(House* house = Houses::getInstance()->getHouse(houseId))
	{
		house->clean();
		lua_pushboolean(L, true);
	}
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDoCleanMap(lua_State* L)
{
	//doCleanMap()
	uint32_t count = 0;
	g_game.cleanMap(count);
	lua_pushnumber(L, count);
	return 1;
}

int32_t LuaScriptInterface::luaDoRefreshMap(lua_State* L)
{
	//doRefreshMap()
	g_game.proceduralRefresh();
	return 1;
}

int32_t LuaScriptInterface::luaDoUpdateHouseAuctions(lua_State* L)
{
	//doUpdateHouseAuctions()
	lua_pushboolean(L, IOMapSerialize::getInstance()->updateAuctions());
	return 1;
}

int32_t LuaScriptInterface::luaGetItemIdByName(lua_State* L)
{
	//getItemIdByName(name[, displayError = true])
	bool displayError = true;
	if(lua_gettop(L) >= 2)
		displayError = popNumber(L);

	int32_t itemId = Item::items.getItemIdByName(popString(L));
	if(itemId == -1)
	{
		if(displayError)
			errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));

		lua_pushboolean(L, false);
	}
	else
		lua_pushnumber(L, itemId);

	return 1;
}

int32_t LuaScriptInterface::luaGetItemInfo(lua_State* L)
{
	//getItemInfo(itemid)
	const ItemType* item;
	if(!(item = Item::items.getElement(popNumber(L))))
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setFieldBool(L, "stopTime", item->stopTime);
	setFieldBool(L, "showCount", item->showCount);
	setFieldBool(L, "clientCharges", item->clientCharges);
	setFieldBool(L, "stackable", item->stackable);
	setFieldBool(L, "showDuration", item->showDuration);
	setFieldBool(L, "showCharges", item->showCharges);
	setFieldBool(L, "showAttributes", item->showCharges);
	setFieldBool(L, "distRead", item->allowDistRead);
	setFieldBool(L, "readable", item->canReadText);
	setFieldBool(L, "writable", item->canWriteText);
	setFieldBool(L, "forceSerialize", item->forceSerialize);
	setFieldBool(L, "vertical", item->isVertical);
	setFieldBool(L, "horizontal", item->isHorizontal);
	setFieldBool(L, "hangable", item->isHangable);
	setFieldBool(L, "usable", item->useable);
	setFieldBool(L, "movable", item->moveable);
	setFieldBool(L, "pickupable", item->pickupable);
	setFieldBool(L, "rotable", item->rotable);
	setFieldBool(L, "replacable", item->replaceable);
	setFieldBool(L, "hasHeight", item->hasHeight);
	setFieldBool(L, "blockSolid", item->blockSolid);
	setFieldBool(L, "blockPickupable", item->blockPickupable);
	setFieldBool(L, "blockProjectile", item->blockProjectile);
	setFieldBool(L, "blockPathing", item->blockPathFind);
	setFieldBool(L, "allowPickupable", item->allowPickupable);
	setFieldBool(L, "alwaysOnTop", item->alwaysOnTop);

	createTable(L, "floorChange");
	for(int32_t i = CHANGE_FIRST; i <= CHANGE_LAST; ++i)
	{
		lua_pushnumber(L, i);
		lua_pushboolean(L, item->floorChange[i - 1]);
		pushTable(L);
	}

	pushTable(L);
	setField(L, "magicEffect", (int32_t)item->magicEffect);
	setField(L, "fluidSource", (int32_t)item->fluidSource);
	setField(L, "weaponType", (int32_t)item->weaponType);
	setField(L, "bedPartnerDirection", (int32_t)item->bedPartnerDir);
	setField(L, "ammoAction", (int32_t)item->ammoAction);
	setField(L, "combatType", (int32_t)item->combatType);
	setField(L, "corpseType", (int32_t)item->corpseType);
	setField(L, "shootType", (int32_t)item->shootType);
	setField(L, "ammoType", (int32_t)item->ammoType);

	createTable(L, "transformUseTo");
	setField(L, "female", item->transformUseTo[PLAYERSEX_FEMALE]);
	setField(L, "male", item->transformUseTo[PLAYERSEX_MALE]);

	pushTable(L);
	setField(L, "transformToFree", item->transformToFree);
	setField(L, "transformEquipTo", item->transformEquipTo);
	setField(L, "transformDeEquipTo", item->transformDeEquipTo);
	setField(L, "clientId", item->clientId);
	setField(L, "maxItems", item->maxItems);
	setField(L, "slotPosition", item->slotPosition);
	setField(L, "wieldPosition", item->wieldPosition);
	setField(L, "speed", item->speed);
	setField(L, "maxTextLength", item->maxTextLen);
	setField(L, "writeOnceItemId", item->writeOnceItemId);
	setField(L, "attack", item->attack);
	setField(L, "extraAttack", item->extraAttack);
	setField(L, "defense", item->defense);
	setField(L, "extraDefense", item->extraDefense);
	setField(L, "armor", item->armor);
	setField(L, "breakChance", item->breakChance);
	setField(L, "hitChance", item->hitChance);
	setField(L, "maxHitChance", item->maxHitChance);
	setField(L, "runeLevel", item->runeLevel);
	setField(L, "runeMagicLevel", item->runeMagLevel);
	setField(L, "lightLevel", item->lightLevel);
	setField(L, "lightColor", item->lightColor);
	setField(L, "decayTo", item->decayTo);
	setField(L, "rotateTo", item->rotateTo);
	setField(L, "alwaysOnTopOrder", item->alwaysOnTopOrder);
	setField(L, "shootRange", item->shootRange);
	setField(L, "charges", item->charges);
	setField(L, "decayTime", item->decayTime);
	setField(L, "attackSpeed", item->attackSpeed);
	setField(L, "wieldInfo", item->wieldInfo);
	setField(L, "minRequiredLevel", item->minReqLevel);
	setField(L, "minRequiredMagicLevel", item->minReqMagicLevel);
	setField(L, "worth", item->worth);
	setField(L, "levelDoor", item->levelDoor);
	setField(L, "name", item->name.c_str());
	setField(L, "plural", item->pluralName.c_str());
	setField(L, "article", item->article.c_str());
	setField(L, "description", item->description.c_str());
	setField(L, "runeSpellName", item->runeSpellName.c_str());
	setField(L, "vocationString", item->vocationString.c_str());

	createTable(L, "abilities");
	setFieldBool(L, "manaShield", item->abilities.manaShield);
	setFieldBool(L, "invisible", item->abilities.invisible);
	setFieldBool(L, "regeneration", item->abilities.regeneration);
	setFieldBool(L, "preventLoss", item->abilities.preventLoss);
	setFieldBool(L, "preventDrop", item->abilities.preventDrop);
	setField(L, "elementType", (int32_t)item->abilities.elementType);
	setField(L, "elementDamage", item->abilities.elementDamage);
	setField(L, "speed", item->abilities.speed);
	setField(L, "healthGain", item->abilities.healthGain);
	setField(L, "healthTicks", item->abilities.healthTicks);
	setField(L, "manaGain", item->abilities.manaGain);
	setField(L, "manaTicks", item->abilities.manaTicks);
	setField(L, "conditionSuppressions", item->abilities.conditionSuppressions);

	//TODO: absorb, increment, reflect, skills, skillsPercent, stats, statsPercent

	pushTable(L);
	setField(L, "group", (int32_t)item->group);
	setField(L, "type", (int32_t)item->type);
	setFieldFloat(L, "weight", item->weight);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemAttribute(lua_State* L)
{
	//getItemAttribute(uid, key)
	std::string key = popString(L);
	ScriptEnviroment* env = getEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushnil(L);
		return 1;
	}

	boost::any value = item->getAttribute(key);
	if(value.empty())
		lua_pushnil(L);
	else if(value.type() == typeid(std::string))
		lua_pushstring(L, boost::any_cast<std::string>(value).c_str());
	else if(value.type() == typeid(int32_t))
		lua_pushnumber(L, boost::any_cast<int32_t>(value));
	else if(value.type() == typeid(float))
		lua_pushnumber(L, boost::any_cast<float>(value));
	else if(value.type() == typeid(bool))
		lua_pushboolean(L, boost::any_cast<bool>(value));
	else
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaDoItemSetAttribute(lua_State* L)
{
	//doItemSetAttribute(uid, key, value)
	boost::any value;
	if(lua_isnumber(L, -1))
	{
		float tmp = popFloatNumber(L);
		if(std::floor(tmp) < tmp)
			value = tmp;
		else
			value = (int32_t)tmp;
	}
	else if(lua_isboolean(L, -1))
		value = popBoolean(L);
	else if(lua_isstring(L, -1))
		value = popString(L);
	else
	{
		lua_pop(L, 1);
		errorEx("Invalid data type");

		lua_pushboolean(L, false);
		return 1;
	}

	std::string key = popString(L);
	ScriptEnviroment* env = getEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	if(value.type() == typeid(int32_t))
	{
		if(key == "uid")
		{
			int32_t tmp = boost::any_cast<int32_t>(value);
			if(tmp < 1000 || tmp > 0xFFFF)
			{
				errorEx("Value for protected key \"uid\" must be in range of 1000 to 65535");
				lua_pushboolean(L, false);
				return 1;
			}

			item->setUniqueId(tmp);
		}		
		else if(key == "aid")
			item->setActionId(boost::any_cast<int32_t>(value));
		else
			item->setAttribute(key, boost::any_cast<int32_t>(value));
	}
	else
		item->setAttribute(key, value);

	lua_pushboolean(L, true);
	return 1;
}

int32_t LuaScriptInterface::luaDoItemEraseAttribute(lua_State* L)
{
	//doItemEraseAttribute(uid, key)
	std::string key = popString(L);
	ScriptEnviroment* env = getEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	bool ret = true;
	if(key == "uid")
	{
		errorEx("Attempt to erase protected key \"uid\".");
		ret = false;
	}
	else if(key != "aid")
		item->eraseAttribute(key);
	else
		item->resetActionId();

	lua_pushboolean(L, ret);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemWeight(lua_State* L)
{
	//getItemWeight(itemid[, precise = true])
	bool precise = true;
	if(lua_gettop(L) > 2)
		precise = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	double weight = item->getWeight();
	if(precise)
	{
		std::stringstream ws;
		ws << std::fixed << std::setprecision(2) << weight;
		weight = atof(ws.str().c_str());
	}

	lua_pushnumber(L, weight);
	return 1;
}

int32_t LuaScriptInterface::luaHasItemProperty(lua_State* L)
{
	//hasItemProperty(uid, prop)
	uint32_t prop = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		errorEx(getError(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	//Check if the item is a tile, so we can get more accurate properties
	bool tmp = item->hasProperty((ITEMPROPERTY)prop);
	if(item->getTile() && item->getTile()->ground == item)
		tmp = item->getTile()->hasProperty((ITEMPROPERTY)prop);

	lua_pushboolean(L, tmp);
	return 1;
}

int32_t LuaScriptInterface::luaIsIpBanished(lua_State* L)
{
	//isIpBanished(ip[, mask])
	uint32_t mask = 0xFFFFFFFF;
	if(lua_gettop(L) > 1)
		mask = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->isIpBanished((uint32_t)popNumber(L), mask));
	return 1;
}

int32_t LuaScriptInterface::luaIsPlayerBanished(lua_State* L)
{
	//isPlayerBanished(name/guid, type)
	PlayerBan_t type = (PlayerBan_t)popNumber(L);
	if(lua_isnumber(L, -1))
		lua_pushboolean(L, IOBan::getInstance()->isPlayerBanished((uint32_t)popNumber(L), type));
	else
		lua_pushboolean(L, IOBan::getInstance()->isPlayerBanished(popString(L), type));

	return 1;
}

int32_t LuaScriptInterface::luaIsAccountBanished(lua_State* L)
{
	//isAccountBanished(accountId[, playerId])
	uint32_t playerId = 0;
	if(lua_gettop(L) > 1)
		playerId = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->isAccountBanished((uint32_t)popNumber(L), playerId));
	return 1;
}

int32_t LuaScriptInterface::luaDoAddIpBanishment(lua_State* L)
{
	//doAddIpBanishment(ip[, mask[, length[, reason[, comment[, admin[, statement]]]]]])
	uint32_t admin = 0, reason = 21, mask = 0xFFFFFFFF, params = lua_gettop(L);
	int64_t length = time(NULL) + g_config.getNumber(ConfigManager::IPBANISHMENT_LENGTH);
	std::string statement, comment;

	if(params > 6)
		statement = popString(L);

	if(params > 5)
		admin = popNumber(L);

	if(params > 4)
		comment = popString(L);

	if(params > 3)
		reason = popNumber(L);

	if(params > 2)
		length = popNumber(L);

	if(params > 1)
		mask = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->addIpBanishment((uint32_t)popNumber(L),
		length, reason, comment, admin, mask, statement));
	return 1;
}

int32_t LuaScriptInterface::luaDoAddPlayerBanishment(lua_State* L)
{
	//doAddPlayerBanishment(name/guid[, type[, length[, reason[, action[, comment[, admin[, statement]]]]]]])
	uint32_t admin = 0, reason = 21, params = lua_gettop(L);
	int64_t length = -1;
	std::string statement, comment;

	ViolationAction_t action = ACTION_NAMELOCK;
	PlayerBan_t type = PLAYERBAN_LOCK;
	if(params > 7)
		statement = popString(L);

	if(params > 6)
		admin = popNumber(L);

	if(params > 5)
		comment = popString(L);

	if(params > 4)
		action = (ViolationAction_t)popNumber(L);

	if(params > 3)
		reason = popNumber(L);

	if(params > 2)
		length = popNumber(L);

	if(params > 1)
		type = (PlayerBan_t)popNumber(L);

	if(lua_isnumber(L, -1))
		lua_pushboolean(L, IOBan::getInstance()->addPlayerBanishment((uint32_t)popNumber(L),
			length, reason, action, comment, admin, type, statement));
	else
		lua_pushboolean(L, IOBan::getInstance()->addPlayerBanishment(popString(L),
			length, reason, action, comment, admin, type, statement));

	return 1;
}

int32_t LuaScriptInterface::luaDoAddAccountBanishment(lua_State* L)
{
	//doAddAccountBanishment(accountId[, playerId[, length[, reason[, action[, comment[, admin[, statement]]]]]]])
	uint32_t admin = 0, reason = 21, playerId = 0, params = lua_gettop(L);
	int64_t length = time(NULL) + g_config.getNumber(ConfigManager::BAN_LENGTH);
	std::string statement, comment;

	ViolationAction_t action = ACTION_BANISHMENT;
	if(params > 7)
		statement = popString(L);

	if(params > 6)
		admin = popNumber(L);

	if(params > 5)
		comment = popString(L);

	if(params > 4)
		action = (ViolationAction_t)popNumber(L);

	if(params > 3)

		reason = popNumber(L);

	if(params > 2)
		length = popNumber(L);

	if(params > 1)
		playerId = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->addAccountBanishment((uint32_t)popNumber(L),
		length, reason, action, comment, admin, playerId, statement));
	return 1;
}

int32_t LuaScriptInterface::luaDoAddNotation(lua_State* L)
{
	//doAddNotation(accountId[, playerId[, reason[, comment[, admin[, statement]]]]]])
	uint32_t admin = 0, reason = 21, playerId = 0, params = lua_gettop(L);
	std::string statement, comment;

	if(params > 5)
		statement = popString(L);

	if(params > 4)
		admin = popNumber(L);

	if(params > 3)
		comment = popString(L);

	if(params > 2)
		reason = popNumber(L);

	if(params > 1)
		playerId = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->addNotation((uint32_t)popNumber(L),
		reason, comment, admin, playerId, statement));
	return 1;
}

int32_t LuaScriptInterface::luaDoAddStatement(lua_State* L)
{
	//doAddStatement(name/guid[, channelId[, reason[, comment[, admin[, statement]]]]]])
	uint32_t admin = 0, reason = 21, params = lua_gettop(L);
	int16_t channelId = -1;
	std::string statement, comment;

	if(params > 5)
		statement = popString(L);

	if(params > 4)
		admin = popNumber(L);

	if(params > 3)
		comment = popString(L);

	if(params > 2)
		reason = popNumber(L);

	if(params > 1)
		channelId = popNumber(L);

	if(lua_isnumber(L, -1))
		lua_pushboolean(L, IOBan::getInstance()->addStatement((uint32_t)popNumber(L),
			reason, comment, admin, channelId, statement));
	else
		lua_pushboolean(L, IOBan::getInstance()->addStatement(popString(L),
			reason, comment, admin, channelId, statement));

	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveIpBanishment(lua_State* L)
{
	//doRemoveIpBanishment(ip[, mask])
	uint32_t mask = 0xFFFFFFFF;
	if(lua_gettop(L) > 1)
		mask = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->removeIpBanishment(
		(uint32_t)popNumber(L), mask));
	return 1;
}

int32_t LuaScriptInterface::luaDoRemovePlayerBanishment(lua_State* L)
{
	//doRemovePlayerBanishment(name/guid, type)
	PlayerBan_t type = (PlayerBan_t)popNumber(L);
	if(lua_isnumber(L, -1))
		lua_pushboolean(L, IOBan::getInstance()->removePlayerBanishment((uint32_t)popNumber(L), type));
	else
		lua_pushboolean(L, IOBan::getInstance()->removePlayerBanishment(popString(L), type));

	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveAccountBanishment(lua_State* L)
{
	//doRemoveAccountBanishment(accountId[, playerId])
	uint32_t playerId = 0;
	if(lua_gettop(L) > 1)
		playerId = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->removeAccountBanishment((uint32_t)popNumber(L), playerId));
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveNotations(lua_State* L)
{
	//doRemoveNotations(accountId[, playerId])
	uint32_t playerId = 0;
	if(lua_gettop(L) > 1)
		playerId = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->removeNotations((uint32_t)popNumber(L), playerId));
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveStatements(lua_State* L)
{
	//doRemoveStatements(name/guid[, channelId])
	int16_t channelId = -1;
	if(lua_gettop(L) > 1)
		channelId = popNumber(L);

	if(lua_isnumber(L, -1))
		lua_pushboolean(L, IOBan::getInstance()->removeStatements((uint32_t)popNumber(L), channelId));
	else
		lua_pushboolean(L, IOBan::getInstance()->removeStatements(popString(L), channelId));

	return 1;
}

int32_t LuaScriptInterface::luaGetNotationsCount(lua_State* L)
{
	//getNotationsCount(accountId[, playerId])
	uint32_t playerId = 0;
	if(lua_gettop(L) > 1)
		playerId = popNumber(L);

	lua_pushnumber(L, IOBan::getInstance()->getNotationsCount((uint32_t)popNumber(L), playerId));
	return 1;
}

int32_t LuaScriptInterface::luaGetStatementsCount(lua_State* L)
{
	//getStatementsCount(name/guid[, channelId])
	int16_t channelId = -1;
	if(lua_gettop(L) > 1)
		channelId = popNumber(L);

	if(lua_isnumber(L, -1))
		lua_pushnumber(L, IOBan::getInstance()->getStatementsCount((uint32_t)popNumber(L), channelId));
	else
		lua_pushnumber(L, IOBan::getInstance()->getStatementsCount(popString(L), channelId));

	return 1;
}

int32_t LuaScriptInterface::luaGetBanData(lua_State* L)
{
	//getBanData(value[, type[, param]])
	Ban tmp;
	uint32_t params = lua_gettop(L);
	if(params > 2)
		tmp.param = popNumber(L);

	if(params > 1)
		tmp.type = (Ban_t)popNumber(L);

	tmp.value = popNumber(L);
	if(!IOBan::getInstance()->getData(tmp))
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_newtable(L);
	setField(L, "id", tmp.id);
	setField(L, "type", tmp.type);
	setField(L, "value", tmp.value);
	setField(L, "param", tmp.param);
	setField(L, "added", tmp.added);
	setField(L, "expires", tmp.expires);
	setField(L, "adminId", tmp.adminId);
	setField(L, "reason", tmp.reason);
	setField(L, "action", tmp.action);
	setField(L, "comment", tmp.comment);
	setField(L, "statement", tmp.statement);
	return 1;
}

int32_t LuaScriptInterface::luaGetBanReason(lua_State* L)
{
	//getBanReason(id)
	lua_pushstring(L, getReason((ViolationAction_t)popNumber(L)).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetBanAction(lua_State* L)
{
	//getBanAction(id[, ipBanishment])
	bool ipBanishment = false;
	if(lua_gettop(L) > 1)
		ipBanishment = popNumber(L);

	lua_pushstring(L, getAction((ViolationAction_t)popNumber(L), ipBanishment).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetBanList(lua_State* L)
{
	//getBanList(type[, value[, param]])
	int32_t param = 0, params = lua_gettop(L);
	if(params > 2)
		param = popNumber(L);

	uint32_t value = 0;
	if(params > 1)
		value = popNumber(L);

	BansVec bans = IOBan::getInstance()->getList((Ban_t)popNumber(L), value, param);
	BansVec::const_iterator it = bans.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != bans.end(); ++it, ++i)
	{
		createTable(L, i);
		setField(L, "id", it->id);
		setField(L, "type", it->type);
		setField(L, "value", it->value);
		setField(L, "param", it->param);
		setField(L, "added", it->added);
		setField(L, "expires", it->expires);
		setField(L, "adminId", it->adminId);
		setField(L, "reason", it->reason);
		setField(L, "action", it->action);
		setField(L, "comment", it->comment);
		setField(L, "statement", it->statement);
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetExperienceStage(lua_State* L)
{
	//getExperienceStage(level[, divider])
	double divider = 1.0f;
	if(lua_gettop(L) > 1)
		divider = popFloatNumber(L);

	lua_pushnumber(L, g_game.getExperienceStage(popNumber(L), divider));
	return 1;
}

int32_t LuaScriptInterface::luaGetDataDir(lua_State* L)
{
	//getDataDir()
	lua_pushstring(L, getFilePath(FILE_TYPE_OTHER, "").c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetLogsDir(lua_State* L)
{
	//getLogsDir()
	lua_pushstring(L, getFilePath(FILE_TYPE_LOG, "").c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetConfigFile(lua_State* L)
{
	//getConfigFile()
	lua_pushstring(L, g_config.getString(ConfigManager::CONFIG_FILE).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetConfigValue(lua_State* L)
{
	//getConfigValue(key)
	g_config.getValue(popString(L), L);
	return 1;
}

int32_t LuaScriptInterface::luaGetModList(lua_State* L)
{
	//getModList()
	ModMap::iterator it = ScriptingManager::getInstance()->getFirstMod();
	lua_newtable(L);
	for(uint32_t i = 1; it != ScriptingManager::getInstance()->getLastMod(); ++it, ++i)
	{
		createTable(L, i);
		setField(L, "name", it->first);
		setField(L, "description", it->second.description);
		setField(L, "file", it->second.file);

		setField(L, "version", it->second.version);
		setField(L, "author", it->second.author);
		setField(L, "contact", it->second.contact);

		setFieldBool(L, "enabled", it->second.enabled);
		pushTable(L);
	}

	return 1;
}

int32_t LuaScriptInterface::luaL_loadmodlib(lua_State* L)
{
	//loadmodlib(lib)
	std::string name = asLowerCaseString(popString(L));
	for(LibMap::iterator it = ScriptingManager::getInstance()->getFirstLib();
		it != ScriptingManager::getInstance()->getLastLib(); ++it)
	{
		if(asLowerCaseString(it->first) != name)
			continue;

		luaL_loadstring(L, it->second.second.c_str());
		lua_pushvalue(L, -1);
		break;
	}

	return 1;
}

int32_t LuaScriptInterface::luaL_domodlib(lua_State* L)
{
	//domodlib(lib)
	std::string name = asLowerCaseString(popString(L));
	for(LibMap::iterator it = ScriptingManager::getInstance()->getFirstLib();
		it != ScriptingManager::getInstance()->getLastLib(); ++it)
	{
		if(asLowerCaseString(it->first) != name)
			continue;

		bool ret = luaL_dostring(L, it->second.second.c_str());
		if(ret)
			error(NULL, popString(L));

		lua_pushboolean(L, !ret);
		break;
	}

	return 1;
}

int32_t LuaScriptInterface::luaL_dodirectory(lua_State* L)
{
	std::string dir = popString(L);
	if(!getEnv()->getInterface()->loadDirectory(dir, NULL))
	{
		errorEx("Failed to load directory " + dir + ".");
		lua_pushboolean(L, false);
	}
	else
		lua_pushboolean(L, true);

	return 1;
}

#define EXPOSE_LOG(Name, Stream)\
	int32_t LuaScriptInterface::luaStd##Name(lua_State* L)\
	{\
		StringVec data;\
		for(int32_t i = 0, params = lua_gettop(L); i < params; ++i)\
			data.push_back(popString(L));\
\
		for(StringVec::reverse_iterator it = data.rbegin(); it != data.rend(); ++it)\
			Stream << (*it) << std::endl;\
\
		lua_pushnumber(L, data.size());\
		return 1;\
	}

EXPOSE_LOG(Cout, std::cout)
EXPOSE_LOG(Cerr, std::cerr)
EXPOSE_LOG(Clog, std::clog)

#undef EXPOSE_LOG

int32_t LuaScriptInterface::luaStdMD5(lua_State* L)
{
	//std.md5(string[, upperCase])
	bool upperCase = false;
	if(lua_gettop(L) > 1)
		upperCase = popNumber(L);

	lua_pushstring(L, transformToMD5(popString(L), upperCase).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaStdSHA1(lua_State* L)
{
	//std.sha1(string[, upperCase])
	bool upperCase = false;
	if(lua_gettop(L) > 1)
		upperCase = popNumber(L);

	lua_pushstring(L, transformToSHA1(popString(L), upperCase).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseExecute(lua_State* L)
{
	//db.executeQuery(query)
	DBQuery query; //lock mutex
	lua_pushboolean(L, Database::getInstance()->executeQuery(popString(L)));
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseStoreQuery(lua_State* L)
{
	//db.storeQuery(query)
	ScriptEnviroment* env = getEnv();

	DBQuery query; //lock mutex
	if(DBResult* res = Database::getInstance()->storeQuery(popString(L)))
		lua_pushnumber(L, env->addResult(res));
	else
		lua_pushboolean(L, false);

	return 1;
}

int32_t LuaScriptInterface::luaDatabaseEscapeString(lua_State* L)
{
	//db.escapeString(str)
	DBQuery query; //lock mutex
	lua_pushstring(L, Database::getInstance()->escapeString(popString(L)).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseEscapeBlob(lua_State* L)
{
	//db.escapeBlob(s, length)
	uint32_t length = popNumber(L);
	DBQuery query; //lock mutex

	lua_pushstring(L, Database::getInstance()->escapeBlob(popString(L).c_str(), length).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseLastInsertId(lua_State* L)
{
	//db.lastInsertId()
	DBQuery query; //lock mutex
	lua_pushnumber(L, Database::getInstance()->getLastInsertId());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseStringComparison(lua_State* L)
{
	//db.stringComparison()
	lua_pushstring(L, Database::getInstance()->getStringComparison().c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseUpdateLimiter(lua_State* L)
{
	//db.updateLimiter()
	lua_pushstring(L, Database::getInstance()->getUpdateLimiter().c_str());
	return 1;
}

#define CHECK_RESULT()\
	if(!res)\
	{\
		lua_pushboolean(L, false);\
		return 1;\
	}

int32_t LuaScriptInterface::luaResultGetDataInt(lua_State* L)
{
	//result.getDataInt(res, s)
	const std::string& s = popString(L);
	ScriptEnviroment* env = getEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	CHECK_RESULT()

	lua_pushnumber(L, res->getDataInt(s));
	return 1;
}

int32_t LuaScriptInterface::luaResultGetDataLong(lua_State* L)
{
	//result.getDataLong(res, s)
	const std::string& s = popString(L);
	ScriptEnviroment* env = getEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	CHECK_RESULT()

	lua_pushnumber(L, res->getDataLong(s));
	return 1;
}

int32_t LuaScriptInterface::luaResultGetDataString(lua_State* L)
{
	//result.getDataString(res, s)
	const std::string& s = popString(L);
	ScriptEnviroment* env = getEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	CHECK_RESULT()

	lua_pushstring(L, res->getDataString(s).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaResultGetDataStream(lua_State* L)
{
	//result.getDataStream(res, s)
	const std::string s = popString(L);
	ScriptEnviroment* env = getEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	CHECK_RESULT()

	uint64_t length = 0;
	lua_pushstring(L, res->getDataStream(s, length));

	lua_pushnumber(L, length);
	return 1;
}

int32_t LuaScriptInterface::luaResultNext(lua_State* L)
{
	//result.next(res)
	ScriptEnviroment* env = getEnv();

	DBResult* res = env->getResultByID(popNumber(L));
	CHECK_RESULT()

	lua_pushboolean(L, res->next());
	return 1;
}

int32_t LuaScriptInterface::luaResultFree(lua_State* L)
{
	//result.free(res)
	uint32_t rid = popNumber(L);
	ScriptEnviroment* env = getEnv();

	DBResult* res = env->getResultByID(rid);
	CHECK_RESULT()

	lua_pushboolean(L, env->removeResult(rid));
	return 1;
}

#undef CHECK_RESULT

int32_t LuaScriptInterface::luaBitNot(lua_State* L)
{
	int32_t number = (int32_t)popNumber(L);
	lua_pushnumber(L, ~number);
	return 1;
}

int32_t LuaScriptInterface::luaBitUNot(lua_State* L)
{
	uint32_t number = (uint32_t)popNumber(L);
	lua_pushnumber(L, ~number);
	return 1;
}

#define MULTI_OPERATOR(type, name, op)\
	int32_t LuaScriptInterface::luaBit##name(lua_State* L)\
	{\
		int32_t params = lua_gettop(L);\
		type value = (type)popNumber(L);\
		for(int32_t i = 2; i <= params; ++i)\
			value op popNumber(L);\
\
		lua_pushnumber(L, value);\
		return 1;\
	}

MULTI_OPERATOR(int32_t, And, &=)
MULTI_OPERATOR(int32_t, Or, |=)
MULTI_OPERATOR(int32_t, Xor, ^=)
MULTI_OPERATOR(uint32_t, UAnd, &=)
MULTI_OPERATOR(uint32_t, UOr, |=)
MULTI_OPERATOR(uint32_t, UXor, ^=)

#undef MULTI_OPERATOR

#define SHIFT_OPERATOR(type, name, op)\
	int32_t LuaScriptInterface::luaBit##name(lua_State* L)\
	{\
		type v2 = (type)popNumber(L), v1 = (type)popNumber(L);\
		lua_pushnumber(L, (v1 op v2));\
		return 1;\
	}

SHIFT_OPERATOR(int32_t, LeftShift, <<)
SHIFT_OPERATOR(int32_t, RightShift, >>)
SHIFT_OPERATOR(uint32_t, ULeftShift, <<)
SHIFT_OPERATOR(uint32_t, URightShift, >>)

#undef SHIFT_OPERATOR
