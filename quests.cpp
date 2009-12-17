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

#include "quests.h"
#include "tools.h"

bool Mission::isStarted(Player* player)
{
	if(!player)
		return false;

	std::string value;
	return player->getStorage(storageId, value) && atoi(value.c_str()) >= startValue;
}

bool Mission::isCompleted(Player* player)
{
	if(!player)
		return false;

	std::string value;
	return player->getStorage(storageId, value) && atoi(value.c_str()) >= endValue;
}

std::string Mission::getDescription(Player* player)
{
	std::string value;
	if(!player->getStorage(storageId, value))
		return "Couldn't retrieve a valid player storage, please report to a gamemaster.";

	if(state.size())
	{
		std::string ret = state;
		replaceString(ret, "|STATE|", value);
		return ret;
	}

	if(atoi(value.c_str()) >= endValue)
		return states.rbegin()->second;

	for(int32_t i = endValue; i >= startValue; --i)
	{
		if(!player->getStorage(storageId, value) || atoi(value.c_str()) != i)
			continue;

		std::string ret = states[i - startValue];
		replaceString(ret, "|STATE|", value);
		return ret;

	}

	return "Couldn't retrieve any mission description, please report to a gamemaster.";
}

Quest::~Quest()
{
	for(MissionList::iterator it = missions.begin(); it != missions.end(); it++)
		delete (*it);

	missions.clear();
}

bool Quest::isStarted(Player* player)
{
	if(!player)
		return false;

	std::string value;
	return player->getStorage(storageId, value) && atoi(value.c_str()) >= storageValue;
}

bool Quest::isCompleted(Player* player) const
{
	for(MissionList::const_iterator it = missions.begin(); it != missions.end(); it++)
	{
		if(!(*it)->isCompleted(player))
			return false;
	}

	return true;
}

uint16_t Quest::getMissionCount(Player* player)
{
	uint16_t count = 0;
	for(MissionList::iterator it = missions.begin(); it != missions.end(); it++)
	{
		if((*it)->isStarted(player))
			count++;
	}

	return count;
}

void Quests::clear()
{
	for(QuestList::iterator it = quests.begin(); it != quests.end(); it++)
		delete (*it);

	quests.clear();
}

bool Quests::reload()
{
	clear();
	return loadFromXml();
}

bool Quests::loadFromXml()
{
	xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_XML, "quests.xml").c_str());
	if(!doc)
	{
		std::cout << "[Warning - Quests::loadFromXml] Cannot load quests file." << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr p, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"quests"))
	{
		std::cout << "[Error - Quests::loadFromXml] Malformed quests file." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	p = root->children;
	while(p)
	{
		parseQuestNode(p, false);
		p = p->next;
	}

	xmlFreeDoc(doc);
	return true;
}

bool Quests::parseQuestNode(xmlNodePtr p, bool checkDuplicate)
{
	if(xmlStrcmp(p->name, (const xmlChar*)"quest"))
		return false;

	int32_t intValue;
	std::string strValue;

	uint32_t id = m_lastId;
	if(readXMLInteger(p, "id", intValue) && id > 0)
	{
		id = intValue;
		if(id > m_lastId)
			m_lastId = id;
	}

	std::string name;
	if(readXMLString(p, "name", strValue))
		name = strValue;

	uint32_t startStorageId = 0;
	if(readXMLInteger(p, "startstorageid", intValue) || readXMLInteger(p, "storageId", intValue))
		startStorageId = intValue;

	int32_t startStorageValue = 0;
	if(readXMLInteger(p, "startstoragevalue", intValue) || readXMLInteger(p, "storageValue", intValue))
		startStorageValue = intValue;

	Quest* quest = new Quest(name, id, startStorageId, startStorageValue);
	if(!quest)
		return false;

	for(xmlNodePtr missionNode = p->children; missionNode; missionNode = missionNode->next)
	{
		if(xmlStrcmp(missionNode->name, (const xmlChar*)"mission"))
			continue;

		std::string missionName, missionState;
		if(readXMLString(missionNode, "name", strValue))
			missionName = strValue;

		if(readXMLString(missionNode, "state", strValue) || readXMLString(missionNode, "description", strValue))
			missionState = strValue;

		uint32_t storageId = 0;
		if(readXMLInteger(missionNode, "storageid", intValue) || readXMLInteger(p, "storageId", intValue))
			storageId = intValue;

		int32_t startValue = 0, endValue = 0;
		if(readXMLInteger(missionNode, "startvalue", intValue) || readXMLInteger(p, "startValue", intValue))
			startValue = intValue;

		if(readXMLInteger(missionNode, "endvalue", intValue) || readXMLInteger(p, "endValue", intValue))
			endValue = intValue;

		if(Mission* mission = new Mission(missionName, missionState, storageId, startValue, endValue))
		{
			if(missionState.empty())
			{
				// parse sub-states only if main is not set
				for(xmlNodePtr stateNode = missionNode->children; stateNode; stateNode = stateNode->next)
				{
					if(xmlStrcmp(stateNode->name, (const xmlChar*)"missionstate"))
						continue;

					uint32_t missionId;
					if(!readXMLInteger(stateNode, "id", intValue))
					{
						std::cout << "[Warning - Quests::parseQuestNode] Missing missionId for mission state" << std::endl;
						continue;
					}

					missionId = intValue;
					std::string description;
					if(readXMLString(stateNode, "description", strValue))
						description = strValue;

					mission->newState(missionId, description);
				}
			}

			quest->newMission(mission);
		}
	}

	if(checkDuplicate)
	{
		for(QuestList::iterator it = quests.begin(); it != quests.end(); ++it)
		{
			if((*it)->getName() == name)
				delete *it;
		}
	}

	m_lastId++;
	quests.push_back(quest);
	return true;
}

uint16_t Quests::getQuestCount(Player* player)
{
	uint16_t count = 0;
	for(QuestList::iterator it = quests.begin(); it != quests.end(); it++)
	{
		if((*it)->isStarted(player))
			count++;
	}

	return count;
}

Quest* Quests::getQuestById(uint16_t id) const
{
	for(QuestList::const_iterator it = quests.begin(); it != quests.end(); it++)
	{
		if((*it)->getId() == id)
			return (*it);
	}

	return NULL;
}
