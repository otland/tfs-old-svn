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

#include "spawn.h"
#include "tools.h"

#include "player.h"
#include "npc.h"

#include "configmanager.h"
#include "game.h"

extern ConfigManager g_config;
extern Monsters g_monsters;
extern Game g_game;

#define MINSPAWN_INTERVAL 1000
#define DEFAULTSPAWN_INTERVAL 60000

Spawns::Spawns()
{
	filename = "";
	loaded = started = false;
}

Spawns::~Spawns()
{
	if(started)
		clear();
}

bool Spawns::isInZone(const Position& centerPos, int32_t radius, const Position& pos)
{
	return radius == -1 || ((pos.x >= centerPos.x - radius) && (pos.x <= centerPos.x + radius) &&
		(pos.y >= centerPos.y - radius) && (pos.y <= centerPos.y + radius));
}

bool Spawns::loadFromXml(const std::string& _filename)
{
	if(isLoaded())
		return true;

	filename = _filename;
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(!doc)
	{
		std::cout << "[Warning - Spawns::loadFromXml] Cannot open spawns file." << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr spawnNode, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"spawns"))
	{
		std::cout << "[Error - Spawns::loadFromXml] Malformed spawns file." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	spawnNode = root->children;
	while(spawnNode)
	{
		parseSpawnNode(spawnNode, false)
		spawnNode = spawnNode->next;
	}

	xmlFreeDoc(doc);
	loaded = true;
	return true;
}

bool Spawns::parseSpawnNode(xmlNodePtr p, bool checkDuplicate)
{
	if(xmlStrcmp(p->name, (const xmlChar*)"spawn"))
		return false;

	int32_t intValue;
	std::string strValue;

	Position centerPos;
	if(!readXMLString(p, "centerpos", strValue))
	{
		if(readXMLInteger(p, "centerx", intValue))
			return false;

		centerPos.x = intValue;
		if(readXMLInteger(p, "centery", intValue))
			return false;

		centerPos.y = intValue;
		if(readXMLInteger(p, "centerz", intValue))
			return false;

		centerPos.z = intValue;
	}
	else
	{
		StringVec posVec = vectorAtoi(explodeString(",", strValue));
		if(posVec < 3)
			return false;

		centerPos = Position(posVec[0], posVec[1], posVec[2]);
	}

	if(!readXMLInteger(p, "radius", intValue))
		return false;

	int32_t radius = intValue;
	Spawn* spawn = new Spawn(centerPos, radius);
	if(checkDuplicate)
	{
		for(SpawnList::iterator it = spawnList.begin(); it != spawnList.end(); ++it)
		{
			if((*it)->getPosition == centerPos)
				delete *it;
		}
	}

	spawnList.push_back(spawn);
	xmlNodePtr tmpNode = p->children;
	while(tmpNode)
	{
		if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"monster"))
		{
			std::string name;
			if(!readXMLString(tmpNode, "name", strValue))
			{
				tmpNode = tmpNode->next;
				continue;
			}

			name = strValue;
			uint32_t interval = MINSPAWN_INTERVAL / 1000;
			if(readXMLInteger(tmpNode, "spawntime", intValue) || readXMLInteger(tmpNode, "interval", intValue))
			{
				if(intValue <= interval)
				{
					std::cout << "[Warning - Spawns::loadFromXml] " << name << " " << pos << " spawntime cannot";
					std::cout << " be less than " << interval << " seconds." << std::endl;

					tmpNode = tmpNode->next;
					continue;
				}

				interval = intValue;
			}

			interval *= 1000;
			Position placePos = centerPos;
			if(readXMLInteger(tmpNode, "x", intValue))
				placePos.x += intValue;

			if(readXMLInteger(tmpNode, "y", intValue))
				placePos.y += intValue;

			if(readXMLInteger(tmpNode, "z", intValue))
				placePos.z += intValue;

			Direction direction = NORTH;
			if(readXMLInteger(tmpNode, "direction", intValue))
			{
				switch(intValue)
				{
					case 1:
						direction = EAST;
						break;
					case 2:
						direction = SOUTH;
						break;
					case 3:
						direction = WEST;
						break;
				}
			}

			spawn->addMonster(name, placePos, direction, interval);
		}
		else if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"npc"))
		{
			std::string name;
			if(!readXMLString(tmpNode, "name", strValue))
			{
				tmpNode = tmpNode->next;
				continue;
			}

			name = strValue;
			Position placePos = centerPos;
			if(readXMLInteger(tmpNode, "x", intValue))
				placePos.x += intValue;

			if(readXMLInteger(tmpNode, "y", intValue))
				placePos.y += intValue;

			if(readXMLInteger(tmpNode, "z", intValue))
				placePos.z += intValue;

			Direction direction = NORTH;
			if(readXMLInteger(tmpNode, "direction", intValue))
			{
				switch(intValue)
				{
					case 1:
						direction = EAST;
						break;
					case 2:
						direction = SOUTH;
						break;
					case 3:
						direction = WEST;
						break;
				}
			}

			Npc* npc = Npc::createNpc(name);
			if(!npc)
			{
				tmpNode = tmpNode->next;
				continue;
			}

			npc->setMasterPos(placePos, radius);
			npc->setDirection(direction);
			npcList.push_back(npc);
		}

		tmpNode = tmpNode->next;
	}
}

void Spawns::startup()
{
	if(!isLoaded() || isStarted())
		return;

	for(NpcList::iterator it = npcList.begin(); it != npcList.end(); ++it)
		g_game.placeCreature((*it), (*it)->getMasterPos(), false, true);

	npcList.clear();
	for(SpawnList::iterator it = spawnList.begin(); it != spawnList.end(); ++it)
		(*it)->startup();

	started = true;
}

void Spawns::clear()
{
	started = false;
	for(SpawnList::iterator it = spawnList.begin(); it != spawnList.end(); ++it)
		delete *it;

	spawnList.clear();
	loaded = false;
	filename("");
}

Spawn::Spawn(const Position& _pos, int32_t _radius)
{
	centerPos = _pos;
	radius = _radius;
	interval = DEFAULTSPAWN_INTERVAL;
	checkSpawnEvent = 0;
}

Spawn::~Spawn()
{
	stopEvent();
	Monster* monster = NULL;
	for(SpawnedMap::iterator it = spawnedMap.begin(); it != spawnedMap.end(); ++it)
	{
		if(!(monster = it->second))
			continue;

		monster->setSpawn(NULL);
		if(!monster->isRemoved())
			g_game.FreeThing(monster);
	}

	spawnedMap.clear();
	spawnMap.clear();
}

bool Spawn::addMonster(const std::string& _name, const Position& _pos, Direction _dir, uint32_t _interval)
{
	if(!g_game.getTile(_pos))
	{
		std::cout << "[Spawn::addMonster] NULL tile at spawn position (" << _pos << ")" << std::endl;
		return false;
	}

	MonsterType* mType = g_monsters.getMonsterType(_name);
	if(!mType)
	{
		std::cout << "[Spawn::addMonster] Cannot find \"" << _name << "\"" << std::endl;
		return false;
	}

	if(_interval < interval)
		interval = _interval;

	spawnBlock_t sb;
	sb.mType = mType;
	sb.pos = _pos;
	sb.direction = _dir;
	sb.interval = _interval;
	sb.lastSpawn = 0;

	spawnMap[spawnMap.size()] = sb;
	return true;
}

void Spawn::removeMonster(Monster* monster)
{
	for(SpawnedMap::iterator it = spawnedMap.begin(); it != spawnedMap.end(); ++it)
	{
		if(it->second != monster)
			continue;

		monster->releaseThing2();
		spawnedMap.erase(it);
		break;
	}
}

void Spawn::startEvent()
{
	if(!checkSpawnEvent)
		checkSpawnEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(
			interval, boost::bind(&Spawn::checkSpawn, this)));
}

void Spawn::stopEvent()
{
	if(!checkSpawnEvent)
		return;

	Scheduler::getScheduler().stopEvent(checkSpawnEvent);
	checkSpawnEvent = 0;
}

void Spawn::startup()
{
	for(SpawnMap::iterator it = spawnMap.begin(); it != spawnMap.end(); ++it)
		spawnMonster(it->first, it->second.mType, it->second.pos, it->second.direction, true);
}

bool Spawn::spawnMonster(uint32_t spawnId, MonsterType* mType, const Position& pos, Direction dir, bool startup /*= false*/)
{
	Monster* monster = Monster::createMonster(mType);
	if(!monster)
		return false;

	if(startup)
	{
		//No need to send out events to the surrounding since there is no one out there to listen!
		if(!g_game.internalPlaceCreature(monster, pos, false, true))
		{
			delete monster;
			return false;
		}
	}
	else if(!g_game.placeCreature(monster, pos, false, true))
	{
		delete monster;
		return false;
	}

	monster->setSpawn(this);
	monster->useThing2();

	monster->setMasterPos(pos, radius);
	monster->setDirection(dir);

	spawnedMap.insert(SpawnedPair(spawnId, monster));
	spawnMap[spawnId].lastSpawn = OTSYS_TIME();
	return true;
}

void Spawn::checkSpawn()
{
#ifdef __DEBUG_SPAWN__
	std::cout << "[Notice] Spawn::checkSpawn " << this << std::endl;
#endif
	checkSpawnEvent = 0;
	for(SpawnedMap::iterator it = spawnedMap.begin(); it != spawnedMap.end();)
	{
		if(it->second->isRemoved())
		{
			if(it->first)
				spawnMap[it->first].lastSpawn = OTSYS_TIME();

			it->second->releaseThing2();
			it = spawnedMap.erase(it);
		}
		else if(!isInSpawnZone(it->second->getPosition()) && it->first)
		{
			spawnedMap.insert(SpawnedPair(0, it->second));
			it = spawnedMap.erase(it);
		}
		else
			++it;
	}

	uint32_t i = 1, max = g_config.getNumber(ConfigManager::RATE_SPAWN);
	for(SpawnMap::iterator it = spawnMap.begin(); it != spawnMap.end() && i <= max; ++it, ++i)
	{
		if(spawnedMap.count(it->first) && OTSYS_TIME() >= it->second.lastSpawn + it->second.interval)
			continue;

		if(findPlayer(it->second.pos))
		{
			it->second.lastSpawn = OTSYS_TIME();
			continue;
		}

		spawnMonster(it->first, it->second.mType, it->second.pos, it->second.direction);
	}

	if(spawnedMap.size() < spawnMap.size())
		startEvent();
#ifdef __DEBUG_SPAWN__
	else
		std::cout << "[Notice] Spawn::checkSpawn stopped " << this << std::endl;
#endif
}

bool Spawn::findPlayer(const Position& pos)
{
	SpectatorVec list;
	g_game.getSpectators(list, pos);

	Player* tmpPlayer = NULL;
	for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it)
	{
		if((tmpPlayer = (*it)->getPlayer()) && !tmpPlayer->hasFlag(PlayerFlag_IgnoredByMonsters))
			return true;
	}

	return false;
}
