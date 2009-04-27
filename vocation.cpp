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
#include <iostream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "vocation.h"
#include "tools.h"

Vocation Vocations::defVoc = Vocation();

void Vocations::clear()
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it)
		delete it->second;

	vocationsMap.clear();
}

bool Vocations::reload()
{
	clear();
	return loadFromXml();
}

bool Vocations::parseVocationNode(xmlNodePtr p)
{
	std::string strValue;
	int32_t intValue;
	float floatValue;
	if(xmlStrcmp(p->name, (const xmlChar*)"vocation") != 0)
		return false;

	if(!readXMLInteger(p, "id", intValue))
	{
		std::cout << "Missing vocation id." << std::endl;
		return false;
	}

	Vocation* voc = new Vocation(intValue);
	if(readXMLString(p, "name", strValue))
		voc->setName(strValue);

	if(readXMLString(p, "description", strValue))
		voc->setDescription(strValue);

	if(readXMLString(p, "needpremium", strValue))
		voc->setNeedPremium(booleanString(strValue));

	if(readXMLInteger(p, "gaincap", intValue) || readXMLInteger(p, "gaincapacity", intValue))
		voc->setGainCap(intValue);

	if(readXMLInteger(p, "gainhp", intValue) || readXMLInteger(p, "gainhealth", intValue))
		voc->setGainHealth(intValue);

	if(readXMLInteger(p, "gainmana", intValue))
		voc->setGainMana(intValue);

	if(readXMLInteger(p, "gainhpticks", intValue) || readXMLInteger(p, "gainhealthticks", intValue))
		voc->setGainHealthTicks(intValue);

	if(readXMLInteger(p, "gainhpamount", intValue) || readXMLInteger(p, "gainhealthamount", intValue))
		voc->setGainHealthAmount(intValue);

	if(readXMLInteger(p, "gainmanaticks", intValue))
		voc->setGainManaTicks(intValue);

	if(readXMLInteger(p, "gainmanaamount", intValue))
		voc->setGainManaAmount(intValue);

	if(readXMLFloat(p, "manamultiplier", floatValue))
		voc->setManaMultiplier(floatValue);

	if(readXMLInteger(p, "attackspeed", intValue))
		voc->setAttackSpeed(intValue);

	if(readXMLInteger(p, "basespeed", intValue))
		voc->setBaseSpeed(intValue);

	if(readXMLInteger(p, "soulmax", intValue))
		voc->setSoulMax(intValue);

	if(readXMLInteger(p, "gainsoulticks", intValue))
		voc->setSoulGainTicks(intValue);

	if(readXMLString(p, "attackable", strValue))
		voc->setAttackable(booleanString(strValue));

	if(readXMLInteger(p, "fromvoc", intValue) || readXMLInteger(p, "fromvocation", intValue))
		voc->setFromVocation(intValue);

	if(readXMLInteger(p, "lessloss", intValue))
		voc->setLessLoss(intValue);

	xmlNodePtr configNode = p->children;
	while(configNode)
	{
		if(!xmlStrcmp(configNode->name, (const xmlChar*)"skill"))
		{
			uint32_t skillId;
			if(readXMLInteger(configNode, "id", intValue))
			{
				skillId = intValue;
				if(skillId < SKILL_FIRST || skillId > SKILL_LAST)
					std::cout << "No valid skill id. " << skillId << std::endl;
				else
				{
					if(readXMLFloat(configNode, "multiplier", floatValue))
						voc->setSkillMultiplier((skills_t)skillId, floatValue);
				}
			}
			else
				std::cout << "Missing skill id." << std::endl;
		}
		else if(!xmlStrcmp(configNode->name, (const xmlChar*)"formula"))
		{
			if(readXMLFloat(configNode, "meleeDamage", floatValue))
				voc->setMeleeMultiplier(floatValue);

			if(readXMLFloat(configNode, "distDamage", floatValue) || readXMLFloat(configNode, "distanceDamage", floatValue))
				voc->setDistanceMultiplier(floatValue);

			if(readXMLFloat(configNode, "wandDamage", floatValue) || readXMLFloat(configNode, "rodDamage", floatValue))
				voc->setWandMultiplier(floatValue);

			if(readXMLFloat(configNode, "magDamage", floatValue) || readXMLFloat(configNode, "magicDamage", floatValue))
				voc->setMagicMultiplier(floatValue);

			if(readXMLFloat(configNode, "magHealingDamage", floatValue) || readXMLFloat(configNode, "magicHealingDamage", floatValue))
				voc->setMagicHealingMultiplier(floatValue);

			if(readXMLFloat(configNode, "defense", floatValue))
				voc->setDefenseMultiplier(floatValue);

			if(readXMLFloat(configNode, "armor", floatValue))
				voc->setArmorMultiplier(floatValue);
		}
		else if(!xmlStrcmp(configNode->name, (const xmlChar*)"absorb"))
		{
			if(readXMLInteger(configNode, "percentAll", intValue))
			{
				for(uint32_t i = COMBAT_FIRST; i <= COMBAT_LAST; i++)
					voc->increaseAbsorbPercent((CombatType_t)i, intValue);
			}
			else if(readXMLInteger(configNode, "percentElements", intValue))
			{
				voc->increaseAbsorbPercent(COMBAT_ENERGYDAMAGE, intValue);
				voc->increaseAbsorbPercent(COMBAT_FIREDAMAGE, intValue);
				voc->increaseAbsorbPercent(COMBAT_EARTHDAMAGE, intValue);
				voc->increaseAbsorbPercent(COMBAT_ICEDAMAGE, intValue);
			}
			else if(readXMLInteger(configNode, "percentMagic", intValue))
			{
				voc->increaseAbsorbPercent(COMBAT_ENERGYDAMAGE, intValue);
				voc->increaseAbsorbPercent(COMBAT_FIREDAMAGE, intValue);
				voc->increaseAbsorbPercent(COMBAT_EARTHDAMAGE, intValue);
				voc->increaseAbsorbPercent(COMBAT_ICEDAMAGE, intValue);
				voc->increaseAbsorbPercent(COMBAT_HOLYDAMAGE, intValue);
				voc->increaseAbsorbPercent(COMBAT_DEATHDAMAGE, intValue);
			}
			else if(readXMLInteger(configNode, "percentEnergy", intValue))
				voc->increaseAbsorbPercent(COMBAT_ENERGYDAMAGE, intValue);
			else if(readXMLInteger(configNode, "percentFire", intValue))
				voc->increaseAbsorbPercent(COMBAT_FIREDAMAGE, intValue);
			else if(readXMLInteger(configNode, "percentPoison", intValue) || readXMLInteger(configNode, "percentEarth", intValue))
				voc->increaseAbsorbPercent(COMBAT_EARTHDAMAGE, intValue);
			else if(readXMLInteger(configNode, "percentIce", intValue))
				voc->increaseAbsorbPercent(COMBAT_ICEDAMAGE, intValue);
			else if(readXMLInteger(configNode, "percentHoly", intValue))
				voc->increaseAbsorbPercent(COMBAT_HOLYDAMAGE, intValue);
			else if(readXMLInteger(configNode, "percentDeath", intValue))
				voc->increaseAbsorbPercent(COMBAT_DEATHDAMAGE, intValue);
			else if(readXMLInteger(configNode, "percentLifeDrain", intValue))
				voc->increaseAbsorbPercent(COMBAT_LIFEDRAIN, intValue);
			else if(readXMLInteger(configNode, "percentManaDrain", intValue))
				voc->increaseAbsorbPercent(COMBAT_MANADRAIN, intValue);
			else if(readXMLInteger(configNode, "percentDrown", intValue))
				voc->increaseAbsorbPercent(COMBAT_DROWNDAMAGE, intValue);
			else if(readXMLInteger(configNode, "percentPhysical", intValue))
				voc->increaseAbsorbPercent(COMBAT_PHYSICALDAMAGE, intValue);
			else if(readXMLInteger(configNode, "percentHealing", intValue))
				voc->increaseAbsorbPercent(COMBAT_HEALING, intValue);
			else if(readXMLInteger(configNode, "percentUndefined", intValue))
				voc->increaseAbsorbPercent(COMBAT_UNDEFINEDDAMAGE, intValue);
		}

		configNode = configNode->next;
	}

	vocationsMap[voc->getId()] = voc;
	return true;
}

bool Vocations::loadFromXml()
{
	xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_XML,"vocations.xml").c_str());
	if(!doc)
	{
		std::cout << "[Warning - Vocations::loadFromXml] Cannot load vocations file." << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr p, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"vocations"))
	{
		std::cout << "[Error - Vocations::loadFromXml] Malformed vocations file." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	p = root->children;
	while(p)
	{
		parseVocationNode(p);
		p = p->next;
	}

	xmlFreeDoc(doc);
	return true;
}

Vocation* Vocations::getVocation(uint32_t vocId)
{
	VocationsMap::iterator it = vocationsMap.find(vocId);
	if(it != vocationsMap.end())
		return it->second;

	std::cout << "[Warning - Vocations::getVocation] Vocation " << vocId << " not found." << std::endl;
	return &Vocations::defVoc;
}

int32_t Vocations::getVocationId(const std::string& name)
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it)
	{
		if(!strcasecmp(it->second->getName().c_str(), name.c_str()))
			return it->first;
	}

	return -1;
}

int32_t Vocations::getPromotedVocation(uint32_t vocationId)
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it)
	{
		if(it->second->getFromVocation() == vocationId && it->first != vocationId)
			return it->first;
	}

	return -1;
}

uint32_t Vocation::skillBase[SKILL_LAST + 1] = {50, 50, 50, 50, 30, 100, 20};

Vocation::~Vocation()
{
	cacheMana.clear();
	for(int32_t i = SKILL_FIRST; i < SKILL_LAST; ++i)
		cacheSkill[i].clear();
}

void Vocation::reset()
{
	needPremium = false;
	attackable = true;
	skillMultipliers[6] = 1.1f;
	manaMultiplier = 4.0;
	soulMax = 100;
	soulGainTicks = 120;
	baseSpeed = 220;
	attackSpeed = 1500;
	name = description = "";
	lessLoss = fromVocation = 0;
	gainHealthAmount = gainManaAmount = 1;
	gainHealth = gainMana = gainCap = 5;
	gainHealthTicks = gainManaTicks = 6;
	meleeMultiplier = distanceMultiplier = wandMultiplier = magicMultiplier = magicHealingMultiplier = defenseMultiplier = armorMultiplier = 1.0;
	memset(skillMultipliers, 2.0f, sizeof(skillMultipliers) - 1);
	skillMultipliers[0] = 1.5f;
	memset(absorbPercent, 0, sizeof(absorbPercent));
}

uint32_t Vocation::getReqSkillTries(int32_t skill, int32_t level)
{
	if(skill < SKILL_FIRST || skill > SKILL_LAST)
		return 0;

	cacheMap& skillMap = cacheSkill[skill];
	cacheMap::iterator it = skillMap.find(level);
	if(it != cacheSkill[skill].end())
		return it->second;

	uint32_t tries = (uint32_t)(skillBase[skill] * std::pow((float)skillMultipliers[skill], (float)(level - 11)));
	skillMap[level] = tries;
	return tries;
}

uint64_t Vocation::getReqMana(uint32_t magLevel)
{
	cacheMap::iterator it = cacheMana.find(magLevel);
	if(it != cacheMana.end())
		return it->second;

	uint64_t reqMana = (uint64_t)(400 * pow(manaMultiplier, magLevel-1));
	if(reqMana % 20 < 10)
		reqMana = reqMana - (reqMana % 20);
	else
		reqMana = reqMana - (reqMana % 20) + 20;

	cacheMana[magLevel] = reqMana;
	return reqMana;
}
