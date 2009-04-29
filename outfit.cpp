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

#include "outfit.h"
#include "tools.h"

#include "player.h"
#include "game.h"
extern Game g_game;

OutfitList::~OutfitList()
{
	for(OutfitListType::iterator it = m_list.begin(); it != m_list.end(); it++)
		delete (*it);

	m_list.clear();
}

void OutfitList::addOutfit(const Outfit& outfit)
{
	for(OutfitListType::iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		if((*it)->looktype == outfit.looktype)
		{
			(*it)->addons = (*it)->addons | outfit.addons;
			return;
		}
	}

	//adding a new outfit
	if(Outfit* newOutfit = new Outfit)
	{
		newOutfit->looktype = outfit.looktype;
		newOutfit->addons = outfit.addons;
		newOutfit->quest = outfit.quest;
		newOutfit->premium = outfit.premium;

		for(uint32_t i = COMBAT_FIRST; i <= COMBAT_LAST; i++)
			newOutfit->absorbPercent[(CombatType_t)i] = outfit.absorbPercent[(CombatType_t)i];

		m_list.push_back(newOutfit);
	}
}

bool OutfitList::remOutfit(const Outfit& outfit)
{
	for(OutfitListType::iterator it = m_list.begin(); it != m_list.end(); ++it)
	{
		if((*it)->looktype == outfit.looktype)
		{
			if(outfit.addons == 0xFF)
			{
				delete (*it);
				m_list.erase(it);
			}
			else
				(*it)->addons = (*it)->addons & (~outfit.addons);

			return true;
		}
	}

	return false;
}

bool OutfitList::isInList(uint32_t playerId, uint32_t lookType, uint32_t addons) const
{
	Player* player = g_game.getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	OutfitListType::const_iterator git, it;
	const OutfitListType& globalOutfits = Outfits::getInstance()->getOutfits(player->getSex());
	for(git = globalOutfits.begin(); git != globalOutfits.end(); ++git)
	{
		if((*git)->looktype != lookType)
			continue;

		for(it = m_list.begin(); it != m_list.end(); ++it)
		{
			if((*it)->looktype != lookType)
				continue;

			if(((*it)->addons & addons) != addons || ((*git)->premium && !player->isPremium()))
				return false;

			if((*git)->quest)
			{
				std::string value;
				if(!player->getStorageValue((*git)->quest, value) || atoi(value.c_str()) != OUTFITS_QUEST_VALUE)
					return false;
			}

			return true;
		}
	}

	return false;
}

Outfits::Outfits()
{
	Outfit outfit;
	outfit.premium = false;
	outfit.addons = outfit.quest = 0;
	memset(outfit.absorbPercent, 0, sizeof(outfit.absorbPercent));

	for(int32_t i = PLAYER_FEMALE_1; i <= PLAYER_FEMALE_7; i++)
	{
		outfit.looktype = i;
		m_femaleList.addOutfit(outfit);
	}

	for(int32_t i = PLAYER_MALE_1; i <= PLAYER_MALE_7; i++)
	{
		outfit.looktype = i;
		m_maleList.addOutfit(outfit);
	}

	m_list.resize(10, NULL);
}

Outfits::~Outfits()
{
	for(OutfitsListVector::iterator it = m_list.begin(); it != m_list.end(); it++)
		delete (*it);

	m_list.clear();
}

bool Outfits::loadFromXml()
{
	xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_XML, "outfits.xml").c_str());
	if(!doc)
	{
		std::cout << "[Warning - Outfits::loadFromXml] Cannot load outfits file, using defaults." << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr p, configNode, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"outfits") != 0)
	{
		std::cout << "[Error - Outfits::loadFromXml] Malformed outfits file, using defaults." << std::endl;
		xmlFreeDoc(doc);
		return true;
	}

	std::string strValue;
	int32_t intValue;

	p = root->children;
	while(p)
	{
		if(xmlStrcmp(p->name, (const xmlChar*)"outfit") != 0)
		{
			p = p->next;
			continue;
		}

		if(!readXMLInteger(p, "type", intValue) || intValue > 9 || intValue < 0)
		{
			std::cout << "[Warning - Outfits::loadFromXml] No valid outfit type " << intValue << std::endl;
			p = p->next;
			continue;
		}

		OutfitList* list;
		if(!m_list[intValue])
		{
			list = new OutfitList;
			m_list[intValue] = list;
		}
		else
			list = m_list[intValue];

		if(!readXMLInteger(p, "looktype", intValue))
		{
			std::cout << "[Error - Outfits::loadFromXml] Missing looktype, skipping" << std::endl;
			p = p->next;
			continue;
		}

		Outfit outfit;
		outfit.looktype = intValue;

		std::string name;
		if(!readXMLString(p, "name", strValue))
		{
			std::cout << "[Warning - Outfits::loadFromXml] Missing name for outfit " << outfit.looktype << ", using default" << std::endl;
			name = "Outfit #" + outfit.looktype;
		}
		else
			name = strValue;

		outfitNamesMap[outfit.looktype] = name;
		outfit.addons = outfit.quest = 0;

		if(readXMLInteger(p, "addons", intValue))
			outfit.addons = intValue;

		if(readXMLInteger(p, "quest", intValue) || readXMLInteger(p, "storage", intValue))
			outfit.quest = intValue;

		if(readXMLString(p, "premium", strValue))
			outfit.premium = booleanString(strValue);

		configNode = p->children;
		while(configNode)
		{
			if(!xmlStrcmp(configNode->name, (const xmlChar*)"absorb"))
			{
				if(readXMLInteger(configNode, "percentAll", intValue))
				{
					for(uint32_t i = COMBAT_FIRST; i <= COMBAT_LAST; i++)
						outfit.absorbPercent[(CombatType_t)i] += intValue;
				}
				else if(readXMLInteger(configNode, "percentElements", intValue))
				{
					outfit.absorbPercent[COMBAT_ENERGYDAMAGE] += intValue;
					outfit.absorbPercent[COMBAT_FIREDAMAGE] += intValue;
					outfit.absorbPercent[COMBAT_EARTHDAMAGE] += intValue;
					outfit.absorbPercent[COMBAT_ICEDAMAGE] += intValue;
				}
				else if(readXMLInteger(configNode, "percentMagic", intValue))
				{
					outfit.absorbPercent[COMBAT_ENERGYDAMAGE] += intValue;
					outfit.absorbPercent[COMBAT_FIREDAMAGE] += intValue;
					outfit.absorbPercent[COMBAT_EARTHDAMAGE] += intValue;
					outfit.absorbPercent[COMBAT_ICEDAMAGE] += intValue;
					outfit.absorbPercent[COMBAT_HOLYDAMAGE] += intValue;
					outfit.absorbPercent[COMBAT_DEATHDAMAGE] += intValue;
				}
				else if(readXMLInteger(configNode, "percentEnergy", intValue))
					outfit.absorbPercent[COMBAT_ENERGYDAMAGE] += intValue;
				else if(readXMLInteger(configNode, "percentFire", intValue))
					outfit.absorbPercent[COMBAT_FIREDAMAGE] += intValue;
				else if(readXMLInteger(configNode, "percentPoison", intValue) || readXMLInteger(configNode, "percentEarth", intValue))
					outfit.absorbPercent[COMBAT_EARTHDAMAGE] += intValue;
				else if(readXMLInteger(configNode, "percentIce", intValue))
					outfit.absorbPercent[COMBAT_ICEDAMAGE] += intValue;
				else if(readXMLInteger(configNode, "percentHoly", intValue))
					outfit.absorbPercent[COMBAT_HOLYDAMAGE] += intValue;
				else if(readXMLInteger(configNode, "percentDeath", intValue))
					outfit.absorbPercent[COMBAT_DEATHDAMAGE] += intValue;
				else if(readXMLInteger(configNode, "percentLifeDrain", intValue))
					outfit.absorbPercent[COMBAT_LIFEDRAIN] += intValue;
				else if(readXMLInteger(configNode, "percentManaDrain", intValue))
					outfit.absorbPercent[COMBAT_MANADRAIN] += intValue;
				else if(readXMLInteger(configNode, "percentDrown", intValue))
					outfit.absorbPercent[COMBAT_DROWNDAMAGE] += intValue;
				else if(readXMLInteger(configNode, "percentPhysical", intValue))
					outfit.absorbPercent[COMBAT_PHYSICALDAMAGE] += intValue;
				else if(readXMLInteger(configNode, "percentHealing", intValue))
					outfit.absorbPercent[COMBAT_HEALING] += intValue;
				else if(readXMLInteger(configNode, "percentUndefined", intValue))
					outfit.absorbPercent[COMBAT_UNDEFINEDDAMAGE] += intValue;
			}

			configNode = configNode->next;
		}

		if(readXMLString(p, "enabled", strValue) && booleanString(strValue))
			list->addOutfit(outfit);

		p = p->next;
	}

	xmlFreeDoc(doc);
	return true;
}

void Outfits::addAttributes(uint32_t playerId, uint32_t lookType)
{
	//TODO
}

void Outfits::removeAttributes(uint32_t playerId, uint32_t lookType)
{
	//TODO
}

int16_t Outfits::getOutfitAbsorb(uint32_t lookType, uint32_t type, CombatType_t combat)
{
	const OutfitListType& globalOutfits = Outfits::getInstance()->getOutfits(type);
	for(OutfitListType::const_iterator it = globalOutfits.begin(); it != globalOutfits.end(); ++it)
	{
		if((*it)->looktype != lookType)
			continue;

		return (*it)->absorbPercent[combat];
	}

	return 0;
}
