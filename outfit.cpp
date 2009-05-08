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
#include "condition.h"

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
		newOutfit->access = outfit.access;
		newOutfit->quest = outfit.quest;
		newOutfit->premium = outfit.premium;
		newOutfit->manaShield = outfit.manaShield;
		newOutfit->invisible = outfit.invisible;
		newOutfit->regeneration = outfit.regeneration;
		newOutfit->speed = outfit.speed;
		newOutfit->healthGain = outfit.healthGain;
		newOutfit->healthTicks = outfit.healthTicks;
		newOutfit->manaGain = outfit.manaGain;
		newOutfit->manaTicks = outfit.manaTicks;
		newOutfit->conditionSuppressions = outfit.conditionSuppressions;

		for(uint32_t i = SKILL_FIRST; i <= SKILL_LAST; i++)
			newOutfit->skills[(skills_t)i] = outfit.skills[(skills_t)i];

		for(uint32_t i = COMBAT_FIRST; i <= COMBAT_LAST; i++)
			newOutfit->absorbPercent[(CombatType_t)i] = outfit.absorbPercent[(CombatType_t)i];

		for(uint32_t i = STAT_FIRST; i <= STAT_LAST; i++)
		{
			newOutfit->stats[(stats_t)i] = outfit.stats[(stats_t)i];
			newOutfit->statsPercent[(stats_t)i] = outfit.stats[(stats_t)i];
		}

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

			if(player->getAccess() < (*it)->access)
				return false;

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

bool Outfits::parseOutfitNode(xmlNodePtr p)
{
	if(xmlStrcmp(p->name, (const xmlChar*)"outfit"))
		return false;

	std::string strValue, strTypes;
	int32_t intValue;

	if(!readXMLString(p, "type", strTypes))
	{
		std::cout << "[Warning - Outfits::loadFromXml] Outfit type not specified" << std::endl;
		return false;
	}

	if(!readXMLInteger(p, "looktype", intValue))
	{
		std::cout << "[Error - Outfits::loadFromXml] Missing looktype, skipping" << std::endl;
		return false;
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

	if(readXMLInteger(p, "addons", intValue))
		outfit.addons = intValue;

	if(readXMLInteger(p, "access", intValue))
		outfit.access = intValue;

	if(readXMLInteger(p, "quest", intValue) || readXMLInteger(p, "storage", intValue))
		outfit.quest = intValue;

	if(readXMLString(p, "premium", strValue))
		outfit.premium = booleanString(strValue);

	if(readXMLString(p, "manaShield", strValue))
		outfit.manaShield = booleanString(strValue);

	if(readXMLString(p, "invisible", strValue))
		outfit.invisible = booleanString(strValue);

	if(readXMLInteger(p, "healthGain", intValue))
	{
		outfit.healthGain = intValue;
		outfit.regeneration = true;
	}

	if(readXMLInteger(p, "healthTicks", intValue))
	{
		outfit.healthTicks = intValue;
		outfit.regeneration = true;
	}

	if(readXMLInteger(p, "manaGain", intValue))
	{
		outfit.manaGain = intValue;
		outfit.regeneration = true;
	}

	if(readXMLInteger(p, "manaTicks", intValue))
	{
		outfit.manaTicks = intValue;
		outfit.regeneration = true;
	}

	if(readXMLInteger(p, "speed", intValue))
		outfit.speed = intValue;

	xmlNodePtr configNode = p->children;
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
		else if(!xmlStrcmp(configNode->name, (const xmlChar*)"skills"))
		{
			if(readXMLInteger(configNode, "fist", intValue))
				outfit.skills[SKILL_FIST] += intValue;
			else if(readXMLInteger(configNode, "club", intValue))
				outfit.skills[SKILL_CLUB] += intValue;
			else if(readXMLInteger(configNode, "sword", intValue))
				outfit.skills[SKILL_SWORD] += intValue;
			else if(readXMLInteger(configNode, "axe", intValue))
				outfit.skills[SKILL_AXE] += intValue;
			else if(readXMLInteger(configNode, "distance", intValue) || readXMLInteger(configNode, "dist", intValue))
				outfit.skills[SKILL_DIST] += intValue;
			else if(readXMLInteger(configNode, "shielding", intValue) || readXMLInteger(configNode, "shield", intValue))
				outfit.skills[SKILL_SHIELD] = intValue;
			else if(readXMLInteger(configNode, "fishing", intValue) || readXMLInteger(configNode, "fish", intValue))
				outfit.skills[SKILL_FISH] = intValue;
			else if(readXMLInteger(configNode, "melee", intValue))
			{
				outfit.skills[SKILL_FIST] += intValue;
				outfit.skills[SKILL_CLUB] += intValue;
				outfit.skills[SKILL_SWORD] += intValue;
				outfit.skills[SKILL_AXE] += intValue;
			}
			else if(readXMLInteger(configNode, "weapon", intValue) || readXMLInteger(configNode, "weapons", intValue))
			{
				outfit.skills[SKILL_CLUB] += intValue;
				outfit.skills[SKILL_SWORD] += intValue;
				outfit.skills[SKILL_AXE] += intValue;
				outfit.skills[SKILL_DIST] += intValue;
			}
			else if(readXMLInteger(configNode, "fistPercent", intValue))
				outfit.skillsPercent[SKILL_FIST] += intValue;
			else if(readXMLInteger(configNode, "clubPercent", intValue))
				outfit.skillsPercent[SKILL_CLUB] += intValue;
			else if(readXMLInteger(configNode, "swordPercent", intValue))
				outfit.skillsPercent[SKILL_SWORD] += intValue;
			else if(readXMLInteger(configNode, "axePercent", intValue))
				outfit.skillsPercent[SKILL_AXE] += intValue;
			else if(readXMLInteger(configNode, "distancePercent", intValue) || readXMLInteger(configNode, "distPercent", intValue))
				outfit.skillsPercent[SKILL_DIST] += intValue;
			else if(readXMLInteger(configNode, "shieldingPercent", intValue) || readXMLInteger(configNode, "shieldPercent", intValue))
				outfit.skillsPercent[SKILL_SHIELD] = intValue;
			else if(readXMLInteger(configNode, "fishingPercent", intValue) || readXMLInteger(configNode, "fishPercent", intValue))
				outfit.skillsPercent[SKILL_FISH] = intValue;
			else if(readXMLInteger(configNode, "meleePercent", intValue))
			{
				outfit.skillsPercent[SKILL_FIST] += intValue;
				outfit.skillsPercent[SKILL_CLUB] += intValue;
				outfit.skillsPercent[SKILL_SWORD] += intValue;
				outfit.skillsPercent[SKILL_AXE] += intValue;
			}
			else if(readXMLInteger(configNode, "weaponPercent", intValue) || readXMLInteger(configNode, "weaponsPercent", intValue))
			{
				outfit.skillsPercent[SKILL_CLUB] += intValue;
				outfit.skillsPercent[SKILL_SWORD] += intValue;
				outfit.skillsPercent[SKILL_AXE] += intValue;
				outfit.skillsPercent[SKILL_DIST] += intValue;
			}
		}
		else if(!xmlStrcmp(configNode->name, (const xmlChar*)"stats"))
		{
			if(readXMLInteger(configNode, "maxHealth", intValue))
				outfit.stats[STAT_MAXHEALTH] = intValue;
			else if(readXMLInteger(configNode, "maxMana", intValue))
				outfit.stats[STAT_MAXMANA] = intValue;
			else if(readXMLInteger(configNode, "soul", intValue))
				outfit.stats[STAT_SOUL] = intValue;
			else if(readXMLInteger(configNode, "level", intValue))
				outfit.stats[STAT_LEVEL] = intValue;
			else if(readXMLInteger(configNode, "magLevel", intValue))
				outfit.stats[STAT_MAGICLEVEL] = intValue;
			else if(readXMLInteger(configNode, "maxHealthPercent", intValue))
				outfit.statsPercent[STAT_MAXHEALTH] = intValue;
			else if(readXMLInteger(configNode, "maxManaPercent", intValue))
				outfit.statsPercent[STAT_MAXMANA] = intValue;
			else if(readXMLInteger(configNode, "soulPercent", intValue))
				outfit.statsPercent[STAT_SOUL] = intValue;
			else if(readXMLInteger(configNode, "levelPercent", intValue))
				outfit.statsPercent[STAT_LEVEL] = intValue;
			else if(readXMLInteger(configNode, "magLevelPercent", intValue))
				outfit.statsPercent[STAT_MAGICLEVEL] = intValue;
		}
		else if(!xmlStrcmp(configNode->name, (const xmlChar*)"suppress"))
		{
			if(readXMLString(configNode, "poison", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_POISON;
			else if(readXMLString(configNode, "fire", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_FIRE;
			else if(readXMLString(configNode, "energy", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_ENERGY;
			else if(readXMLString(configNode, "physical", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_PHYSICAL;
			else if(readXMLString(configNode, "haste", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_HASTE;
			else if(readXMLString(configNode, "paralyze", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_PARALYZE;
			else if(readXMLString(configNode, "outfit", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_OUTFIT;
			else if(readXMLString(configNode, "invisible", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_INVISIBLE;
			else if(readXMLString(configNode, "light", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_LIGHT;
			else if(readXMLString(configNode, "manaShield", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_MANASHIELD;
			else if(readXMLString(configNode, "infight", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_INFIGHT;
			else if(readXMLString(configNode, "drunk", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_DRUNK;
			else if(readXMLString(configNode, "exhaust", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_EXHAUST;
			else if(readXMLString(configNode, "regeneration", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_REGENERATION;
			else if(readXMLString(configNode, "soul", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_SOUL;
			else if(readXMLString(configNode, "drown", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_DROWN;
			else if(readXMLString(configNode, "muted", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_MUTED;
			else if(readXMLString(configNode, "attributes", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_ATTRIBUTES;
			else if(readXMLString(configNode, "freezing", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_FREEZING;
			else if(readXMLString(configNode, "dazzled", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_DAZZLED;
			else if(readXMLString(configNode, "cursed", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_CURSED;
			else if(readXMLString(configNode, "pacified", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_PACIFIED;
			else if(readXMLString(configNode, "gamemaster", strValue) && booleanString(strValue))
				outfit.conditionSuppressions |= CONDITION_GAMEMASTER;
		}

		configNode = configNode->next;
	}

	if(!readXMLString(p, "enabled", strValue) || !booleanString(strValue))
		return true;

	IntegerVec intVector;
	if(!parseIntegerVec(strTypes, intVector))
		return false;

	size_t size = intVector.size();
	for(size_t i = 0; i < size; ++i)
	{
		intValue = intVector[i];
		if(intValue > 9 || intValue < 0)
		{
			std::cout << "[Warning - Outfits::loadFromXml] No valid outfit type " << intValue << std::endl;
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

		list->addOutfit(outfit);
	}

	return true;
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

	xmlNodePtr p, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"outfits") != 0)
	{
		std::cout << "[Error - Outfits::loadFromXml] Malformed outfits file, using defaults." << std::endl;
		xmlFreeDoc(doc);
		return true;
	}

	p = root->children;
	while(p)
	{
		parseOutfitNode(p);
		p = p->next;
	}

	xmlFreeDoc(doc);
	return true;
}

bool Outfits::addAttributes(uint32_t playerId, uint32_t lookType)
{
	Player* player = g_game.getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Outfit* outfit = NULL;
	const OutfitListType& globalOutfits = getOutfits((uint32_t)player->getSex());
	for(OutfitListType::const_iterator it = globalOutfits.begin(); it != globalOutfits.end(); ++it)
	{
		if((*it)->looktype == lookType)
			outfit = (*it);
	}

	if(!outfit || !outfit->looktype)
		return false;

	if(outfit->invisible)
	{
		Condition* condition = Condition::createCondition(CONDITIONID_OUTFIT, CONDITION_INVISIBLE, -1, 0);
		player->addCondition(condition);
	}

	if(outfit->manaShield)
	{
		Condition* condition = Condition::createCondition(CONDITIONID_OUTFIT, CONDITION_MANASHIELD, -1, 0);
		player->addCondition(condition);
	}

	if(outfit->speed)
		g_game.changeSpeed(player, outfit->speed);

	if(outfit->conditionSuppressions)
	{
		player->setConditionSuppressions(outfit->conditionSuppressions, false);
		player->sendIcons();
	}

	if(outfit->regeneration)
	{
		Condition* condition = Condition::createCondition(CONDITIONID_OUTFIT, CONDITION_REGENERATION, -1, 0);
		if(outfit->healthGain)
			condition->setParam(CONDITIONPARAM_HEALTHGAIN, outfit->healthGain);

		if(outfit->healthTicks)
			condition->setParam(CONDITIONPARAM_HEALTHTICKS, outfit->healthTicks);

		if(outfit->manaGain)
			condition->setParam(CONDITIONPARAM_MANAGAIN, outfit->manaGain);

		if(outfit->manaTicks)
			condition->setParam(CONDITIONPARAM_MANATICKS, outfit->manaTicks);

		player->addCondition(condition);
	}

	bool needUpdateSkills = false;
	for(uint32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
	{
		if(outfit->skills[i])
		{
			needUpdateSkills = true;
			player->setVarSkill((skills_t)i, outfit->skills[i]);
		}

		if(outfit->skillsPercent[i])
		{
			needUpdateSkills = true;
			player->setVarSkill((skills_t)i, (int32_t)(player->getSkill((skills_t)i, SKILL_LEVEL) * ((outfit->skillsPercent[i] - 100) / 100.f)));
		}
	}

	if(needUpdateSkills)
		player->sendSkills();

	bool needUpdateStats = false;
	for(uint32_t s = STAT_FIRST; s <= STAT_LAST; ++s)
	{
		if(outfit->stats[s])
		{
			needUpdateStats = true;
			player->setVarStats((stats_t)s, outfit->stats[s]);
		}

		if(outfit->statsPercent[s])
		{
			needUpdateStats = true;
			player->setVarStats((stats_t)s, (int32_t)(player->getDefaultStats((stats_t)s) * ((outfit->statsPercent[s] - 100) / 100.f)));
		}
	}

	if(needUpdateStats)
		player->sendStats();

	return true;
}

bool Outfits::removeAttributes(uint32_t playerId, uint32_t lookType)
{
	Player* player = g_game.getPlayerByID(playerId);
	if(!player || player->isRemoved())
		return false;

	Outfit* outfit = NULL;
	const OutfitListType& globalOutfits = getOutfits((uint32_t)player->getSex());
	for(OutfitListType::const_iterator it = globalOutfits.begin(); it != globalOutfits.end(); ++it)
	{
		if((*it)->looktype == lookType)
			outfit = (*it);
	}

	if(!outfit || !outfit->looktype)
		return false;

	if(outfit->invisible)
		player->removeCondition(CONDITION_INVISIBLE, CONDITIONID_OUTFIT);

	if(outfit->manaShield)
		player->removeCondition(CONDITION_MANASHIELD, CONDITIONID_OUTFIT);

	if(outfit->speed)
		g_game.changeSpeed(player, -outfit->speed);

	if(outfit->conditionSuppressions)
	{
		player->setConditionSuppressions(outfit->conditionSuppressions, true);
		player->sendIcons();
	}

	if(outfit->regeneration)
		player->removeCondition(CONDITION_REGENERATION, CONDITIONID_OUTFIT);

	bool needUpdateSkills = false;
	for(uint32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
	{
		if(outfit->skills[i])
		{
			needUpdateSkills = true;
			player->setVarSkill((skills_t)i, -outfit->skills[i]);
		}

		if(outfit->skillsPercent[i])
		{
			needUpdateSkills = true;
			player->setVarSkill((skills_t)i, -(int32_t)(player->getSkill((skills_t)i, SKILL_LEVEL) * ((outfit->skillsPercent[i] - 100) / 100.f)));
		}
	}

	if(needUpdateSkills)
		player->sendSkills();

	bool needUpdateStats = false;
	for(uint32_t s = STAT_FIRST; s <= STAT_LAST; ++s)
	{
		if(outfit->stats[s])
		{
			needUpdateStats = true;
			player->setVarStats((stats_t)s, -outfit->stats[s]);
		}

		if(outfit->statsPercent[s])
		{
			needUpdateStats = true;
			player->setVarStats((stats_t)s, -(int32_t)(player->getDefaultStats((stats_t)s) * ((outfit->statsPercent[s] - 100) / 100.f)));
		}
	}

	if(needUpdateStats)
		player->sendStats();

	return true;
}

int16_t Outfits::getOutfitAbsorb(uint32_t lookType, uint32_t type, CombatType_t combat)
{
	const OutfitListType& globalOutfits = getOutfitList(type).getOutfits();
	for(OutfitListType::const_iterator it = globalOutfits.begin(); it != globalOutfits.end(); ++it)
	{
		if((*it)->looktype == lookType)
			return (*it)->absorbPercent[combat];
	}

	return 0;
}

const std::string& Outfits::getOutfitName(uint32_t lookType) const
{
	OutfitNamesMap::const_iterator it = outfitNamesMap.find(lookType);
	if(it != outfitNamesMap.end())
		return it->second;

	const std::string& tmp = "Outfit";
	return tmp;
}
