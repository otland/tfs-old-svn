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

#include "outfit.h"
#include "player.h"
#include "game.h"
#include "tools.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

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

bool OutfitList::isInList(int32_t playerId, uint32_t looktype, uint32_t addons) const
{
	Player* player = g_game.getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	OutfitListType::const_iterator git, it;
	const OutfitListType& globalOutfits = Outfits::getInstance()->getOutfits(player->getSex());
	for(git = globalOutfits.begin(); git != globalOutfits.end(); ++git)
	{
		if((*git)->looktype != looktype)
			continue;

		for(it = m_list.begin(); it != m_list.end(); ++it)
		{
			if((*it)->looktype != looktype)
				continue;

			if(((*it)->addons & addons) != addons)
				return false;

			if((*git)->premium && !player->isPremium())
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
	if(xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_XML, "outfits.xml").c_str()))
	{
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		if(xmlStrcmp(root->name,(const xmlChar*)"outfits") != 0)
		{
			xmlFreeDoc(doc);
			std::cout << "[Warning - Outfits::loadFromXml] Malformed outfits file, using defaults." << std::endl;
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
			if(m_list[intValue] != NULL)
				list = m_list[intValue];
			else
			{
				list = new OutfitList;
				m_list[intValue] = list;
			}

			Outfit outfit;
			if(readXMLInteger(p, "looktype", intValue))
				outfit.looktype = intValue;
			else
			{
				std::cout << "[Error - Outfits::loadFromXml] Missing looktype, skipping" << std::endl;
				p = p->next;
				continue;
			}

			std::string name;
			if(readXMLString(p, "name", strValue))
				name = strValue;
			else
			{
				std::cout << "[Warning - Outfits::loadFromXml] Missing name for outfit " << outfit.looktype << ", using default" << std::endl;
				name = "Outfit #" + outfit.looktype;
			}

			outfitNamesMap[outfit.looktype] = name;
			if(readXMLInteger(p, "addons", intValue))
				outfit.addons = intValue;
			else
				outfit.addons = 0;

			if(readXMLInteger(p, "quest", intValue))
				outfit.quest = intValue;
			else
				outfit.quest = 0;

			if(readXMLString(p, "premium", strValue))
				outfit.premium = booleanString(strValue);

			bool enabled = true;
			if(readXMLString(p, "enabled", strValue))
				enabled = booleanString(strValue);

			if(enabled) //This way you can add names for outfits without adding them to default list
				list->addOutfit(outfit);

			p = p->next;
		}

		xmlFreeDoc(doc);
	}

	return true;
}
