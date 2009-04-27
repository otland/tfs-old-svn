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

MissionState::MissionState(std::string _description, uint32_t _missionId)
{
	description = _description;
 	missionId = _missionId;
}

Mission::Mission(std::string _missionName, uint32_t _storageId, int32_t _startValue, int32_t _endValue)
{
	missionName = _missionName;
	endValue = _endValue;
	startValue = _startValue;
	storageId = _storageId;
}

Mission::~Mission()
{
	for(uint32_t it = 0; it != state.size(); it++)
		delete state[it];

	state.clear();
}

std::string Mission::getDescription(Player* player)
{
	for(int32_t i = endValue; i >= startValue; i--)
	{
		std::string value;
		if(player->getStorageValue(storageId, value) && atoi(value.c_str()) == i)
		{
			StateList::const_iterator sit = state.find(i);
			if(sit != state.end())
				return sit->second->getMissionDescription();
		}
	}

	return "An error has occurred, please contact a gamemaster.";
}

bool Mission::isStarted(Player* player) const
{
	if(!player)
		return false;

	std::string value;
	if(player->getStorageValue(storageId, value) && atoi(value.c_str()) >= startValue && atoi(value.c_str()) <= endValue)
		return true;

	return false;
}

bool Mission::isCompleted(Player* player) const
{
	if(!player)
		return false;

	std::string value;
	if(player->getStorageValue(storageId, value) && atoi(value.c_str()) >= endValue)
		return true;

	return false;
}

std::string Mission::getName(Player* player)
{
	if(isCompleted(player))
		return missionName + " (completed)";

	return missionName;
}

Quest::Quest(std::string _name, uint16_t _id, uint32_t _startStorageId, int32_t _startStorageValue)
{
	name = _name;
	id = _id;
	startStorageId = _startStorageId;
	startStorageValue = _startStorageValue;
}

Quest::~Quest()
{
	for(MissionsList::iterator it = missions.begin(); it != missions.end(); it++)
		delete (*it);

	missions.clear();
}

bool Quest::isCompleted(Player* player)
{
	for(MissionsList::iterator it = missions.begin(); it != missions.end(); it++)
	{
		if(!(*it)->isCompleted(player))
			return false;
	}

	return true;
}

bool Quest::isStarted(Player* player) const
{
	if(!player)
		return false;

	std::string value;
	if(player->getStorageValue(startStorageId, value) && atoi(value.c_str()) >= startStorageValue)
		return true;

	return false;
}

uint16_t Quest::getMissionsCount(Player* player)
{
	uint16_t count = 0;
	for(MissionsList::iterator it = missions.begin(); it != missions.end(); it++)
	{
		if((*it)->isStarted(player))
			count++;
	}

	return count;
}

void Quest::getMissionList(Player* player, NetworkMessage_ptr msg)
{
	msg->AddByte(0xF1);
	msg->AddU16(id);
	msg->AddByte(getMissionsCount(player));
	for(MissionsList::iterator it = missions.begin(); it != missions.end(); it++)
	{
		if((*it)->isStarted(player))
		{
			msg->AddString((*it)->getName(player));
			msg->AddString((*it)->getDescription(player));
		}
	}
}

Quests::~Quests()
{
	for(QuestsList::iterator it = quests.begin(); it != quests.end(); it++)
		delete (*it);

	quests.clear();
}

bool Quests::reload()
{
	for(QuestsList::iterator it = quests.begin(); it != quests.end(); it++)
		delete (*it);

	quests.clear();
	return loadFromXml();
}

bool Quests::parseQuestNode(xmlNodePtr p)
{
	if(xmlStrcmp(p->name, (const xmlChar*)"quest"))
		return false;

	int32_t intValue;
	std::string strValue;

	std::string name;
	uint32_t startStorageId = 0;
	int32_t startStorageValue = 0;
	if(readXMLString(p, "name", strValue))
		name = strValue;

	if(readXMLInteger(p, "startstorageid", intValue))
		startStorageId = intValue;

	if(readXMLInteger(p, "startstoragevalue", intValue))
		startStorageValue = intValue;

	Quest* quest = new Quest(name, ++m_lastId, startStorageId, startStorageValue);
	if(!quest)
		return false;

	xmlNodePtr missionNode = p->children;
	while(missionNode)
	{
		if(xmlStrcmp(missionNode->name, (const xmlChar*)"mission") != 0)
		{
			missionNode = missionNode->next;
			continue;
		}

		std::string missionName;
		uint32_t storageId = 0;
		int32_t startValue = 0, endValue = 0;
		if(readXMLString(missionNode, "name", strValue))
			missionName = strValue;

		if(readXMLInteger(missionNode, "storageid", intValue))
			storageId = intValue;

		if(readXMLInteger(missionNode, "startvalue", intValue))
			startValue = intValue;

		if(readXMLInteger(missionNode, "endvalue", intValue))
			endValue = intValue;

		if(Mission *mission = new Mission(missionName, storageId, startValue, endValue))
		{
			xmlNodePtr stateNode = missionNode->children;
			while(stateNode)
			{
				if(xmlStrcmp(stateNode->name, (const xmlChar*)"missionstate") != 0)
				{
					stateNode = stateNode->next;
					continue;
				}

				uint32_t missionId;
				if(readXMLInteger(stateNode, "id", intValue))
					missionId = intValue;
				else
				{
					std::cout << "[Warning - Quests::parseQuestNode]: Missing missionId for mission state" << std::endl;
					stateNode = stateNode->next;
					continue;
				}

				std::string description;
				if(readXMLString(stateNode, "description", strValue))
					description = strValue;

				mission->state[missionId] = new MissionState(description, missionId);
				stateNode = stateNode->next;
			}

			quest->missions.push_back(mission);
		}

		missionNode = missionNode->next;
	}

	quests.push_back(quest);
	return true;
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
		parseQuestNode(p);
		p = p->next;
	}

	xmlFreeDoc(doc);
	return true;
}

uint16_t Quests::getQuestsCount(Player* player)
{
	uint16_t count = 0;
	for(QuestsList::iterator it = quests.begin(); it != quests.end(); it++)
	{
		if((*it)->isStarted(player))
			count++;
	}

	return count;
}

void Quests::getQuestsList(Player* player, NetworkMessage_ptr msg)
{
	msg->AddByte(0xF0);
	msg->AddU16(getQuestsCount(player));
	for(QuestsList::iterator it = quests.begin(); it != quests.end(); it++)
	{
		if((*it)->isStarted(player))
		{
			msg->AddU16((*it)->getId());
			msg->AddString((*it)->getName());
			msg->AddByte((*it)->isCompleted(player));
		}
	}
}

Quest* Quests::getQuestById(uint16_t id)
{
	for(QuestsList::iterator it = quests.begin(); it != quests.end(); it++)
	{
		if((*it)->getId() == id)
			return (*it);
	}

	return NULL;
}
