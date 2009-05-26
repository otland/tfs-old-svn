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
		std::cout << "[Error - Vocations::parseVocationNode] Missing vocation id." << std::endl;
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
		voc->setGain(GAIN_HEALTH, intValue);

	if(readXMLInteger(p, "gainmana", intValue))
		voc->setGain(GAIN_MANA, intValue);

	if(readXMLInteger(p, "gainhpticks", intValue) || readXMLInteger(p, "gainhealthticks", intValue))
		voc->setGainTicks(GAIN_HEALTH, intValue);

	if(readXMLInteger(p, "gainhpamount", intValue) || readXMLInteger(p, "gainhealthamount", intValue))
		voc->setGainAmount(GAIN_HEALTH, intValue);

	if(readXMLInteger(p, "gainmanaticks", intValue))
		voc->setGainTicks(GAIN_MANA, intValue);

	if(readXMLInteger(p, "gainmanaamount", intValue))
		voc->setGainAmount(GAIN_MANA, intValue);

	if(readXMLFloat(p, "manamultiplier", floatValue))
		voc->setMultiplier(MULTIPLIER_MANA, floatValue);

	if(readXMLInteger(p, "attackspeed", intValue))
		voc->setAttackSpeed(intValue);

	if(readXMLInteger(p, "basespeed", intValue))
		voc->setBaseSpeed(intValue);

	if(readXMLInteger(p, "soulmax", intValue))
		voc->setGain(intValue);

	if(readXMLInteger(p, "gainsoulamount", intValue))
		voc->setGainAmount(GAIN_SOUL, intValue);

	if(readXMLInteger(p, "gainsoulticks", intValue))
		voc->setGainTicks(GAIN_SOUL, intValue);

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
			if(readXMLFloat(configNode, "fist", floatValue))
				voc->setSkillMultiplier(SKILL_FIST, floatValue);

			if(readXMLFloat(configNode, "club", floatValue))
				voc->setSkillMultiplier(SKILL_CLUB, floatValue);

			if(readXMLFloat(configNode, "axe", floatValue))
				voc->setSkillMultiplier(SKILL_AXE, floatValue);

			if(readXMLFloat(configNode, "sword", floatValue))
				voc->setSkillMultiplier(SKILL_SWORD, floatValue);

			if(readXMLFloat(configNode, "distance", floatValue) || readXMLFloat(configNode, "dist", floatValue))
				voc->setSkillMultiplier(SKILL_DIST, floatValue);

			if(readXMLFloat(configNode, "shielding", floatValue) || readXMLFloat(configNode, "shield", floatValue))
				voc->setSkillMultiplier(SKILL_SHIELD, floatValue);

			if(readXMLFloat(configNode, "fishing", floatValue) || readXMLFloat(configNode, "fish", floatValue))
				voc->setSkillMultiplier(SKILL_FISH, floatValue);

			if(readXMLFloat(configNode, "experience", floatValue) || readXMLFloat(configNode, "exp", floatValue))
				voc->setSkillMultiplier(SKILL__LEVEL, floatValue);

			if(readXMLInteger(configNode, "id", intValue))
			{
				if(intValue < SKILL_FIRST || intValue >= SKILL__LAST)
					std::cout << "[Error - Vocations::parseVocationNode] No valid skill id (" << intValue << ")." << std::endl;
				else if(readXMLFloat(configNode, "multiplier", floatValue))
					voc->setSkillMultiplier((skills_t)intValue, floatValue);
			}
		}
		else if(!xmlStrcmp(configNode->name, (const xmlChar*)"formula"))
		{
			if(readXMLFloat(configNode, "meleeDamage", floatValue))
				voc->setMultiplier(MULTIPLIER_MELEE, floatValue);

			if(readXMLFloat(configNode, "distDamage", floatValue) || readXMLFloat(configNode, "distanceDamage", floatValue))
				voc->setMultiplier(MULTIPLIER_DISTANCE, floatValue);

			if(readXMLFloat(configNode, "wandDamage", floatValue) || readXMLFloat(configNode, "rodDamage", floatValue))
				voc->setMultiplier(MULTIPLIER_WAND, floatValue);

			if(readXMLFloat(configNode, "magDamage", floatValue) || readXMLFloat(configNode, "magicDamage", floatValue))
				voc->setMultiplier(MULTIPLIER_MAGIC, floatValue);

			if(readXMLFloat(configNode, "magHealingDamage", floatValue) || readXMLFloat(configNode, "magicHealingDamage", floatValue))
				voc->setMultiplier(MULTIPLIER_MAGICHEALING, floatValue);

			if(readXMLFloat(configNode, "defense", floatValue))
				voc->setMultiplier(MULTIPLIER_DEFENSE, floatValue);

			if(readXMLFloat(configNode, "armor", floatValue))
				voc->setMultiplier(MULTIPLIER_ARMOR, floatValue);
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
	memset(absorbPercent, 0, sizeof(absorbPercent));
	needPremium = false;
	attackable = true;
	lessLoss = fromVocation = 0;
	gain[GAIN_SOUL] = 100;
	gainTicks[GAIN_SOUL] = 120;
	baseSpeed = 220;
	attackSpeed = 1500;
	name = description = "";

	gainAmount[GAIN_HEALTH] = gainAmount[GAIN_MANA] = gainAmount[GAIN_SOUL] = 1;
	gain[GAIN_HEALTH] = gain[GAIN_MANA] = capGain = 5;
	gainTicks[GAIN_HEALTH] = gainTicks[GAIN_MANA] = 6;

	skillMultipliers[0] = 1.5f;
	skillMultipliers[6] = 1.1f;
	skillMultipliers[7] = 1.0f;
	for(int32_t i = 1; i < 6; i++)
		skillMultipliers[i] = 2.0f;

	formulaMultipliers[MULTIPLIER_MANA] = 4.0f;
	for(int32_t i = MULTIPLIER_FIRST; i < MULTIPLIER_LAST; i++)
		formulaMultipliers[i] = 1.0f;
}

uint32_t Vocation::getReqSkillTries(int32_t skill, int32_t level)
{
	if(skill < SKILL_FIRST || skill > SKILL_LAST)
		return 0;

	cacheMap& skillMap = cacheSkill[skill];
	cacheMap::iterator it = skillMap.find(level);
	if(it != cacheSkill[skill].end())
		return it->second;

	skillMap[level] = (uint32_t)(skillBase[skill] * std::pow((float)skillMultipliers[skill], (float)(level - 11)));
	return skillMap[level];
}

uint64_t Vocation::getReqMana(uint32_t magLevel)
{
	cacheMap::iterator it = cacheMana.find(magLevel);
	if(it != cacheMana.end())
		return it->second;

	uint64_t reqMana = (uint64_t)(400 * pow(formulaMultipliers[MULTIPLIER_MANA], magLevel - 1));
	if(reqMana % 20 < 10)
		reqMana = reqMana - (reqMana % 20);
	else
		reqMana = reqMana - (reqMana % 20) + 20;

	cacheMana[magLevel] = reqMana;
	return reqMana;
}
