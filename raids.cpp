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

#include "raids.h"

#include "game.h"
#include "player.h"
#include "configmanager.h"

#include <algorithm>

extern Game g_game;
extern ConfigManager g_config;

Raids::Raids()
{
	running = NULL;
	loaded = started = false;
	lastRaidEnd = checkRaidsEvent = 0;
}

Raids::~Raids()
{
	clear();
}

bool Raids::loadFromXml()
{
	if(isLoaded())
		return true;

	if(xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_OTHER, "raids/raids.xml").c_str()))
	{
		xmlNodePtr root, raidNode;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"raids") != 0)
		{
			std::cout << "[Error Raids::loadFromXml]: Wrong root node." << std::endl;
			xmlFreeDoc(doc);
			return false;
		}

		int32_t intValue;
		std::string strValue;
		raidNode = root->children;
		while(raidNode)
		{
			if(xmlStrcmp(raidNode->name, (const xmlChar*)"raid") == 0)
			{
				std::string name, file;
				uint32_t interval;
				uint64_t margin;
				bool enabled = true;

				if(readXMLString(raidNode, "name", strValue))
					name = strValue;
				else
				{
					std::cout << "[Error Raids::loadFromXml]: name tag missing for raid." << std::endl;
					raidNode = raidNode->next;
					continue;
				}

				if(readXMLString(raidNode, "file", strValue))
					file = strValue;
				else
				{
					file = "raids/" + name + ".xml";
					std::cout << "[Warning Raids::loadFromXml]: file tag missing for raid " << name << ", using default: " << file << std::endl;
				}

				if(readXMLInteger(raidNode, "interval2", intValue))
					interval = intValue * 60;
				else
				{
					std::cout << "[Error Raids::loadFromXml]: interval2 tag missing for raid " << name << std::endl;
					raidNode = raidNode->next;
					continue;
				}

				if(readXMLInteger(raidNode, "margin", intValue))
					margin = intValue * 60 * 1000;
				else
				{
					margin = 0;
					std::cout << "[Warning Raids::loadFromXml]: margin tag missing for raid " << name << ", using default: " << margin << std::endl;
				}

				if(readXMLString(raidNode, "enabled", strValue))
					enabled = booleanString(strValue);

				Raid* newRaid = new Raid(name, interval, margin, enabled);
				if(newRaid && newRaid->loadFromXml(getFilePath(FILE_TYPE_OTHER, "raids/" + file)))
					raidList.push_back(newRaid);
				else
				{
					delete newRaid;
					std::cout << "[Error Raids::loadFromXml]: failed to load raid " << name << std::endl;
				}
					
			}

			raidNode = raidNode->next;
		}

		xmlFreeDoc(doc);

	}
	else
	{
		std::cout << "[Error Raids::loadFromXml]: Could not load " << getFilePath(FILE_TYPE_OTHER, "raids/raids.xml") << std::endl;
		return false;
	}

	loaded = true;
	return true;
}

bool Raids::startup()
{
	if(!isLoaded() || isStarted())
		return false;

	setLastRaidEnd(OTSYS_TIME());
	checkRaidsEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(CHECK_RAIDS_INTERVAL * 1000, boost::bind(&Raids::checkRaids, this)));

	started = true;
	return true;
}

void Raids::checkRaids()
{
	checkRaidsEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(CHECK_RAIDS_INTERVAL * 1000, boost::bind(&Raids::checkRaids, this)));
	if(getRunning())
		return;

	uint64_t now = OTSYS_TIME();
	for(RaidList::iterator it = raidList.begin(); it != raidList.end(); ++it)
	{
		if((*it)->isEnabled() && now >= (getLastRaidEnd() + (*it)->getMargin()))
		{
			if(MAX_RAND_RANGE * CHECK_RAIDS_INTERVAL / (*it)->getInterval() >= (uint32_t)random_range(0, MAX_RAND_RANGE))
			{
				#ifdef __DEBUG_RAID__
				char buffer[40];
				formatDate(now, buffer);
				std::cout << buffer << " Notice Raids::checkRaids]: Starting raid " << (*it)->getName() << std::endl;
				#endif
				setRunning(*it);
				(*it)->startRaid();
				break;
			}
		}
	}
}

void Raids::clear()
{
	Scheduler::getScheduler().stopEvent(checkRaidsEvent);
	checkRaidsEvent = lastRaidEnd = 0;
	loaded = started = false;

	for(RaidList::iterator it = raidList.begin(); it != raidList.end(); ++it)
		delete (*it);

	raidList.clear();
	running = NULL;
}

bool Raids::reload()
{
	clear();
	return loadFromXml();
}

Raid* Raids::getRaidByName(const std::string& name)
{
	RaidList::iterator it;
	for(it = raidList.begin(); it != raidList.end(); it++)
	{
		if(strcasecmp((*it)->getName().c_str(), name.c_str()) == 0)
			return (*it);
	}

	return NULL;
}

Raid::Raid(const std::string& _name, uint32_t _interval, uint64_t _margin, bool _enabled)
{
	loaded = false;
	name = _name;
	interval = _interval;
	margin = _margin;
	enabled = _enabled;
	state = RAIDSTATE_IDLE;
	nextEvent = nextEventEvent = 0;
}

Raid::~Raid()
{
	stopEvents();
	for(RaidEventVector::iterator it = raidEvents.begin(); it != raidEvents.end(); it++)
		delete (*it);

	raidEvents.clear();
}

bool Raid::loadFromXml(const std::string& _filename)
{
	if(isLoaded())
		return true;

	xmlDocPtr doc = xmlParseFile(_filename.c_str());
	if(!doc)
	{
		std::cout << "[Error Raid::loadFromXml]: Could not load " << _filename << "!" << std::endl;
		return false;
	}	

	xmlNodePtr root, eventNode;
	root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"raid") != 0)
	{
		std::cout << "[Error Raid::loadFromXml]: Wrong root node." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	std::string strValue;
	eventNode = root->children;
	while(eventNode)
	{
		RaidEvent* event;
		if(xmlStrcmp(eventNode->name, (const xmlChar*)"announce") == 0)
			event = new AnnounceEvent();
		else if(xmlStrcmp(eventNode->name, (const xmlChar*)"singlespawn") == 0)
			event = new SingleSpawnEvent();
		else if(xmlStrcmp(eventNode->name, (const xmlChar*)"areaspawn") == 0)
			event = new AreaSpawnEvent();
		else if(xmlStrcmp(eventNode->name, (const xmlChar*)"script") == 0)
			event = new ScriptEvent();
		else
		{
			eventNode = eventNode->next;
			continue;
		}

		if(event->configureRaidEvent(eventNode))
			raidEvents.push_back(event);
		else
		{
			std::cout << "[Error Raid::loadFromXml]: Malformed file " << _filename << ", eventNode - " << eventNode->name << std::endl;
			delete event;
		}

		eventNode = eventNode->next;
	}

	//sort by delay time
	std::sort(raidEvents.begin(), raidEvents.end(), RaidEvent::compareEvents);
	xmlFreeDoc(doc);
	loaded = true;
	return true;
}

void Raid::startRaid()
{
	if(RaidEvent* raidEvent = getNextRaidEvent())
	{
		state = RAIDSTATE_EXECUTING;
		nextEventEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(raidEvent->getDelay(), boost::bind(&Raid::executeRaidEvent, this, raidEvent)));
	}
}

void Raid::executeRaidEvent(RaidEvent* raidEvent)
{
	if(raidEvent->executeEvent())
	{
		nextEvent++;
		if(RaidEvent* newRaidEvent = getNextRaidEvent())
		{
			uint32_t ticks = (uint32_t)std::max(((uint32_t)RAID_MINTICKS), ((int32_t)newRaidEvent->getDelay() - raidEvent->getDelay()));
			nextEventEvent = Scheduler::getScheduler().addEvent(createSchedulerTask(ticks, boost::bind(&Raid::executeRaidEvent, this, newRaidEvent)));
			return;
		}
	}

	resetRaid();
}

void Raid::resetRaid()
{
#ifdef __DEBUG_RAID__
	std::cout << "[Notice Raid::resetRaid]: Resetting raid." << std::endl;
#endif
	nextEvent = 0;
	state = RAIDSTATE_IDLE;
	Raids::getInstance()->setRunning(NULL);
	Raids::getInstance()->setLastRaidEnd(OTSYS_TIME());
}

void Raid::stopEvents()
{
	if(nextEventEvent != 0)
	{
		Scheduler::getScheduler().stopEvent(nextEventEvent);
		nextEventEvent = 0;
	}
}

RaidEvent* Raid::getNextRaidEvent()
{
	if(nextEvent < raidEvents.size())
		return raidEvents[nextEvent];

	return NULL;
}

bool RaidEvent::configureRaidEvent(xmlNodePtr eventNode)
{
	int32_t intValue;
	if(readXMLInteger(eventNode, "delay", intValue))
	{
		m_delay = std::max(RAID_MINTICKS, intValue);
		return true;
	}

	std::cout << "[Error RaidEvent::configureRaidEvent]: delay tag missing." << std::endl;
	return false;
}

bool AnnounceEvent::configureRaidEvent(xmlNodePtr eventNode)
{
	if(!RaidEvent::configureRaidEvent(eventNode))
		return false;

	std::string strValue;
	if(readXMLString(eventNode, "message", strValue))
		m_message = strValue;
	else
	{
		std::cout << "[Error AnnounceEvent::configureRaidEvent]: Message tag missing for announce event." << std::endl;
		return false;
	}

	if(readXMLString(eventNode, "type", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "warning")
			m_messageType = MSG_STATUS_WARNING;
		else if(tmpStrValue == "event")
			m_messageType = MSG_EVENT_ADVANCE;
		else if(tmpStrValue == "default")
			m_messageType = MSG_EVENT_DEFAULT;
		else if(tmpStrValue == "description")
			m_messageType = MSG_INFO_DESCR;
		else if(tmpStrValue == "status")
			m_messageType = MSG_STATUS_SMALL;
		else if(tmpStrValue == "blue")
			m_messageType = MSG_STATUS_CONSOLE_BLUE;
		else if(tmpStrValue == "red")
			m_messageType = MSG_STATUS_CONSOLE_RED;
		else
		{
			m_messageType = MSG_EVENT_ADVANCE;
			std::cout << "[Notice AnnounceEvent::configureRaidEvent]: unknown type tag missing for announce event, using default: " << (int32_t)m_messageType << std::endl;
		}
	}
	else
	{
		m_messageType = MSG_EVENT_ADVANCE;
		std::cout << "[Notice AnnounceEvent::configureRaidEvent]: type tag missing for announce event. Using default: " << (int32_t)m_messageType << std::endl;
	}

	return true;
}

bool AnnounceEvent::executeEvent() const
{
	g_game.broadcastMessage(m_message, m_messageType);
	return true;
}

bool SingleSpawnEvent::configureRaidEvent(xmlNodePtr eventNode)
{
	if(!RaidEvent::configureRaidEvent(eventNode))
		return false;

	std::string strValue;
	int32_t intValue;

	if(readXMLString(eventNode, "name", strValue))
		m_monsterName = strValue;
	else
	{
		std::cout << "[Error SingleSpawnEvent::configureRaidEvent]: name tag missing for singlespawn event." << std::endl;
		return false;
	}

	if(readXMLString(eventNode, "pos", strValue))
	{
		std::vector<int32_t> posList = vectorAtoi(explodeString(strValue, ";"));
		if(posList.size() >= 3)
			m_position = Position(posList[0], posList[1], posList[2]);
		else
		{
			std::cout << "[Error SingleSpawnEvent::configureRaidEvent]: malformed pos tag for singlespawn event." << std::endl;
			return false;
		}
	}
	else
	{
		if(readXMLInteger(eventNode, "x", intValue))
			m_position.x = intValue;
		else
		{
			std::cout << "[Error SingleSpawnEvent::configureRaidEvent]: x tag missing for singlespawn event." << std::endl;
			return false;
		}

		if(readXMLInteger(eventNode, "y", intValue))
			m_position.y = intValue;
		else
		{
			std::cout << "[Error SingleSpawnEvent::configureRaidEvent]: y tag missing for singlespawn event." << std::endl;
			return false;
		}

		if(readXMLInteger(eventNode, "z", intValue))
			m_position.z = intValue;
		else
		{
			std::cout << "[Error SingleSpawnEvent::configureRaidEvent]: z tag missing for singlespawn event." << std::endl;
			return false;
		}
	}

	return true;
}

bool SingleSpawnEvent::executeEvent() const
{
	Monster* monster = Monster::createMonster(m_monsterName);
	if(!monster || !g_game.placeCreature(monster, m_position))
	{
		delete monster;
		std::cout << "[Error SingleSpawnEvent::executeEvent]: Cannot spawn monster " << m_monsterName << std::endl;
		return false;
	}

	return true;
}

bool AreaSpawnEvent::configureRaidEvent(xmlNodePtr eventNode)
{
	if(!RaidEvent::configureRaidEvent(eventNode))
		return false;

	std::string strValue;
	int32_t intValue;

	if(readXMLInteger(eventNode, "radius", intValue))
	{
		int32_t radius = intValue;
		Position centerPos;

		if(readXMLString(eventNode, "centerpos", strValue))
		{
			std::vector<int32_t> posList = vectorAtoi(explodeString(strValue, ";"));
			if(posList.size() >= 3)
				centerPos = Position(posList[0], posList[1], posList[2]);
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: malformed centerpos tag for areaspawn event." << std::endl;
				return false;
			}
		}
		else
		{
			if(readXMLInteger(eventNode, "centerx", intValue))
				centerPos.x = intValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: centerx tag missing for areaspawn event." << std::endl;
				return false;
			}

			if(readXMLInteger(eventNode, "centery", intValue))
				centerPos.y = intValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: centery tag missing for areaspawn event." << std::endl;
				return false;
			}

			if(readXMLInteger(eventNode, "centerz", intValue))
				centerPos.z = intValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: centerz tag missing for areaspawn event." << std::endl;
				return false;
			}
		}

		m_fromPos.x = centerPos.x - radius;
		m_fromPos.y = centerPos.y - radius;
		m_fromPos.z = centerPos.z;

		m_toPos.x = centerPos.x + radius;
		m_toPos.y = centerPos.y + radius;
		m_toPos.z = centerPos.z;
	}
	else
	{
		if(readXMLString(eventNode, "frompos", strValue))
		{
			std::vector<int32_t> posList = vectorAtoi(explodeString(strValue, ";"));
			if(posList.size() >= 3)
				m_fromPos = Position(posList[0], posList[1], posList[2]);
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: malformed frompos tag for areaspawn event." << std::endl;
				return false;
			}
		}
		else
		{
			if(readXMLInteger(eventNode, "fromx", intValue))
				m_fromPos.x = intValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: fromx tag missing for areaspawn event." << std::endl;
				return false;
			}

			if(readXMLInteger(eventNode, "fromy", intValue))
				m_fromPos.y = intValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: fromy tag missing for areaspawn event." << std::endl;
				return false;
			}

			if(readXMLInteger(eventNode, "fromz", intValue))
				m_fromPos.z = intValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: fromz tag missing for areaspawn event." << std::endl;
				return false;
			}
		}

		if(readXMLString(eventNode, "topos", strValue))
		{
			std::vector<int32_t> posList = vectorAtoi(explodeString(strValue, ";"));
			if(posList.size() >= 3)
				m_toPos = Position(posList[0], posList[1], posList[2]);
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: malformed topos tag for areaspawn event." << std::endl;
				return false;
			}
		}
		else
		{
			if(readXMLInteger(eventNode, "tox", intValue))
				m_toPos.x = intValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: tox tag missing for areaspawn event." << std::endl;
				return false;
			}

			if(readXMLInteger(eventNode, "toy", intValue))
				m_toPos.y = intValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: toy tag missing for areaspawn event." << std::endl;
				return false;
			}

			if(readXMLInteger(eventNode, "toz", intValue))
				m_toPos.z = intValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: toz tag missing for areaspawn event." << std::endl;
				return false;
			}
		}
	}

	xmlNodePtr monsterNode = eventNode->children;
	while(monsterNode)
	{
		if(xmlStrcmp(monsterNode->name, (const xmlChar*)"monster") == 0)
		{
			std::string name;
			int32_t minAmount = 0, maxAmount = 0;

			if(readXMLString(monsterNode, "name", strValue))
				name = strValue;
			else
			{
				std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: name tag missing for monster node." << std::endl;
				return false;
			}

			if(readXMLInteger(monsterNode, "minamount", intValue))
				minAmount = intValue;

			if(readXMLInteger(monsterNode, "maxamount", intValue))
				maxAmount = intValue;

			if(maxAmount == 0 && minAmount == 0)
			{
				if(readXMLInteger(monsterNode, "amount", intValue))
					maxAmount = minAmount = intValue;
				else
				{
					std::cout << "[Error AreaSpawnEvent::configureRaidEvent]: amount tag missing for monster node." << std::endl;
					return false;
				}
			}

			addMonster(name, minAmount, maxAmount);
		}

		monsterNode = monsterNode->next;
	}

	return true;
}

AreaSpawnEvent::~AreaSpawnEvent()
{
	for(MonsterSpawnList::iterator it = m_spawnList.begin(); it != m_spawnList.end(); it++)
		delete (*it);

	m_spawnList.clear();
}

void AreaSpawnEvent::addMonster(MonsterSpawn* monsterSpawn)
{
	m_spawnList.push_back(monsterSpawn);
}

void AreaSpawnEvent::addMonster(const std::string& monsterName, uint32_t minAmount, uint32_t maxAmount)
{
	MonsterSpawn* monsterSpawn = new MonsterSpawn();
	monsterSpawn->name = monsterName;
	monsterSpawn->minAmount = minAmount;
	monsterSpawn->maxAmount = maxAmount;
	addMonster(monsterSpawn);
}

bool AreaSpawnEvent::executeEvent() const
{
	for(MonsterSpawnList::const_iterator it = m_spawnList.begin(); it != m_spawnList.end(); it++)
	{
		MonsterSpawn* spawn = (*it);

		uint32_t amount = random_range(spawn->minAmount, spawn->maxAmount);
		for(uint32_t i = 0; i < amount; i++)
		{
			Monster* monster = Monster::createMonster(spawn->name);
			if(!monster)
			{
				std::cout << "[Error AreaSpawnEvent::executeEvent]: Cant create monster " << spawn->name << std::endl;
				return false;
			}

			bool success = false;
			for(int32_t tries = 0; tries < MAXIMUM_TRIES_PER_MONSTER; tries++)
			{
				if(g_game.placeCreature(monster, Position(random_range(m_fromPos.x, m_toPos.x),
					random_range(m_fromPos.y, m_toPos.y), random_range(m_fromPos.z, m_toPos.z))))
				{
					success = true;
					break;
				}
			}

			if(!success)
				delete monster;
		}
	}

	return true;
}

LuaScriptInterface ScriptEvent::m_scriptInterface("Raid Interface");

ScriptEvent::ScriptEvent():
Event(&m_scriptInterface)
{
	m_scriptInterface.initState();
}

bool ScriptEvent::configureRaidEvent(xmlNodePtr eventNode)
{
	if(!RaidEvent::configureRaidEvent(eventNode))
		return false;

	std::string strValue;
	if(readXMLString(eventNode, "file", strValue))
	{
		if(!loadScript(getFilePath(FILE_TYPE_OTHER, "raids/scripts/" + strValue)))
		{
			std::cout << "[Error ScriptEvent::configureRaidEvent]: Can not load raid script." << std::endl;
			return false;
		}
	}
	else
	{
		std::cout << "[Error ScriptEvent::configureRaidEvent]: No script file found for raid" << std::endl;
		return false;
	}

	return true;
}

std::string ScriptEvent::getScriptEventName()
{
	return "onRaid";
}

bool ScriptEvent::executeEvent() const
{
	//onRaid()
	if(m_scriptInterface.reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface.getScriptEnv();
		#ifdef __DEBUG_LUASCRIPTS__
		env->setEventDesc("Raid event");
		#endif
		env->setScriptId(m_scriptId, &m_scriptInterface);

		m_scriptInterface.pushFunction(m_scriptId);
		int32_t result = m_scriptInterface.callFunction(0);
		m_scriptInterface.releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error ScriptEvent::executeEvent]: Call stack overflow." << std::endl;
		return false;
	}
}
