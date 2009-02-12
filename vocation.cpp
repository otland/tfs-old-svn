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

#include "definitions.h"
#include "vocation.h"
#include <iostream>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <cmath>

#include "tools.h"

Vocations::Vocations()
{
	//
}

Vocations::~Vocations()
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it)
		delete it->second;

	vocationsMap.clear();
}

bool Vocations::loadFromXml()
{
	xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_XML,"vocations.xml").c_str());
	if(doc)
	{
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		if(xmlStrcmp(root->name,(const xmlChar*)"vocations") != 0)
		{
			xmlFreeDoc(doc);
			return false;
		}

		p = root->children;
		while(p)
		{
			std::string strVal;
			int32_t intVal;
			float floatVal;
			if(xmlStrcmp(p->name, (const xmlChar*)"vocation") == 0)
			{
				Vocation* voc = new Vocation();
				xmlNodePtr configNode;
				if(readXMLInteger(p, "id", intVal))
				{
					voc->id = intVal;
					if(readXMLString(p, "name", strVal))
						voc->name = strVal;

					if(readXMLString(p, "description", strVal))
						voc->description = strVal;

					if(readXMLString(p, "needpremium", strVal))
						voc->needPremium = booleanString(strVal);

					if(readXMLInteger(p, "gaincap", intVal) || readXMLInteger(p, "gaincapacity", intVal))
						voc->gainCap = intVal;

					if(readXMLInteger(p, "gainhp", intVal) || readXMLInteger(p, "gainhealth", intVal))
						voc->gainHP = intVal;

					if(readXMLInteger(p, "gainmana", intVal))
						voc->gainMana = intVal;

					if(readXMLInteger(p, "gainhpticks", intVal) || readXMLInteger(p, "gainhealthticks", intVal))
						voc->gainHealthTicks = intVal;

					if(readXMLInteger(p, "gainhpamount", intVal) || readXMLInteger(p, "gainhealthamount", intVal))
						voc->gainHealthAmount = intVal;

					if(readXMLInteger(p, "gainmanaticks", intVal))
						voc->gainManaTicks = intVal;

					if(readXMLInteger(p, "gainmanaamount", intVal))
						voc->gainManaAmount = intVal;

					if(readXMLFloat(p, "manamultiplier", floatVal))
						voc->manaMultiplier = floatVal;

					if(readXMLInteger(p, "attackspeed", intVal))
						voc->attackSpeed = intVal;

					if(readXMLInteger(p, "basespeed", intVal))
						voc->baseSpeed = intVal;

					if(readXMLInteger(p, "soulmax", intVal))
						voc->soulMax = intVal;

					if(readXMLInteger(p, "gainsoulticks", intVal))
						voc->gainSoulTicks = intVal;

					if(readXMLString(p, "attackable", strVal))
						voc->attackable = booleanString(strVal);

					if(readXMLInteger(p, "fromvoc", intVal) || readXMLInteger(p, "fromvocation", intVal))
						voc->fromVocation = intVal;

					if(readXMLInteger(p, "lessloss", intVal))
						voc->lessLoss = intVal;

					configNode = p->children;
					while(configNode)
					{
						if(xmlStrcmp(configNode->name, (const xmlChar*)"skill") == 0)
						{
							uint32_t skillId;
							if(readXMLInteger(configNode, "id", intVal))
							{
								skillId = intVal;
								if(skillId < SKILL_FIRST || skillId > SKILL_LAST)
									std::cout << "No valid skill id. " << skillId << std::endl;
								else
								{
									if(readXMLFloat(configNode, "multiplier", floatVal))
										voc->skillMultipliers[skillId] = floatVal;
								}
							}
							else
								std::cout << "Missing skill id." << std::endl;
						}
						else if(xmlStrcmp(configNode->name, (const xmlChar*)"formula") == 0)
						{
							if(readXMLFloat(configNode, "meleeDamage", floatVal))
								voc->meleeDamageMultipler = floatVal;

							if(readXMLFloat(configNode, "distDamage", floatVal) || readXMLFloat(configNode, "distanceDamage", floatVal))
								voc->distDamageMultipler = floatVal;

							if(readXMLFloat(configNode, "defense", floatVal))
								voc->defenseMultipler = floatVal;

							if(readXMLFloat(configNode, "armor", floatVal))
								voc->armorMultipler = floatVal;
						}
						else if(xmlStrcmp(configNode->name, (const xmlChar*)"absorb") == 0)
						{
							if(readXMLInteger(configNode, "percentAll", intVal))
							{
								for(uint32_t i = COMBAT_FIRST; i <= COMBAT_LAST; i++)
									voc->absorbPercent[i] += intVal;
							}
							else if(readXMLInteger(configNode, "percentElements", intVal))
							{
								voc->absorbPercent[COMBAT_ENERGYDAMAGE] += intVal;
								voc->absorbPercent[COMBAT_FIREDAMAGE] += intVal;
								voc->absorbPercent[COMBAT_EARTHDAMAGE] += intVal;
								voc->absorbPercent[COMBAT_ICEDAMAGE] += intVal;
							}
							else if(readXMLInteger(configNode, "percentMagic", intVal))
							{
								voc->absorbPercent[COMBAT_ENERGYDAMAGE] += intVal;
								voc->absorbPercent[COMBAT_FIREDAMAGE] += intVal;
								voc->absorbPercent[COMBAT_EARTHDAMAGE] += intVal;
								voc->absorbPercent[COMBAT_ICEDAMAGE] += intVal;
								voc->absorbPercent[COMBAT_HOLYDAMAGE] += intVal;
								voc->absorbPercent[COMBAT_DEATHDAMAGE] += intVal;
							}
							else if(readXMLInteger(configNode, "percentEnergy", intVal))
								voc->absorbPercent[COMBAT_ENERGYDAMAGE] += intVal;
							else if(readXMLInteger(configNode, "percentFire", intVal))
								voc->absorbPercent[COMBAT_FIREDAMAGE] += intVal;
							else if(readXMLInteger(configNode, "percentPoison", intVal) || readXMLInteger(configNode, "percentEarth", intVal))
								voc->absorbPercent[COMBAT_EARTHDAMAGE] += intVal;
							else if(readXMLInteger(configNode, "percentIce", intVal))
								voc->absorbPercent[COMBAT_ICEDAMAGE] += intVal;
							else if(readXMLInteger(configNode, "percentHoly", intVal))
								voc->absorbPercent[COMBAT_HOLYDAMAGE] += intVal;
							else if(readXMLInteger(configNode, "percentDeath", intVal))
								voc->absorbPercent[COMBAT_DEATHDAMAGE] += intVal;
							else if(readXMLInteger(configNode, "percentLifeDrain", intVal))
								voc->absorbPercent[COMBAT_LIFEDRAIN] += intVal;
							else if(readXMLInteger(configNode, "percentManaDrain", intVal))
								voc->absorbPercent[COMBAT_MANADRAIN] += intVal;
							else if(readXMLInteger(configNode, "percentDrown", intVal))
								voc->absorbPercent[COMBAT_DROWNDAMAGE] += intVal;
							else if(readXMLInteger(configNode, "percentPhysical", intVal))
								voc->absorbPercent[COMBAT_PHYSICALDAMAGE] += intVal;
							else if(readXMLInteger(configNode, "percentHealing", intVal))
								voc->absorbPercent[COMBAT_HEALING] += intVal;
							else if(readXMLInteger(configNode, "percentUndefined", intVal))
								voc->absorbPercent[COMBAT_UNDEFINEDDAMAGE] += intVal;
						}

						configNode = configNode->next;
					}

					vocationsMap[voc->id] = voc;
				}
				else
					std::cout << "Missing vocation id." << std::endl;
			}

			p = p->next;
		}

		xmlFreeDoc(doc);
	}

	return true;
}

Vocation* Vocations::getVocation(uint32_t vocId)
{
	VocationsMap::iterator it = vocationsMap.find(vocId);
	if(it != vocationsMap.end())
		return it->second;

	std::cout << "Warning: [Vocations::getVocation] Vocation " << vocId << " not found." << std::endl;
	return &defVoc;
}

int32_t Vocations::getVocationId(const std::string& name)
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it)
	{
		if(strcasecmp(it->second->name.c_str(), name.c_str()) == 0)
			return it->first;
	}

	return -1;
}

int32_t Vocations::getPromotedVocation(uint32_t vocationId)
{
	for(VocationsMap::iterator it = vocationsMap.begin(); it != vocationsMap.end(); ++it)
	{
		if(it->second->fromVocation == vocationId && it->first != vocationId)
			return it->first;
	}

	return -1;
}

uint32_t Vocation::skillBase[SKILL_LAST + 1] = {50, 50, 50, 50, 30, 100, 20};

Vocation::Vocation()
{
	name = "none";
	description = "";
	needPremium = false;
	gainHealthTicks = 6;
	gainHealthAmount = 1;
	gainManaTicks = 6;
	gainManaAmount = 1;
	gainSoulTicks = 120;
	soulMax = 100;

	attackable = true;
	fromVocation = 0;

	gainCap = 5;
	gainMana = 5;
	gainHP = 5;
	lessLoss = 0;
	attackSpeed = 1500;
	baseSpeed = 220;
	manaMultiplier = 4.0;
	meleeDamageMultipler = 1.0;
	distDamageMultipler = 1.0;
	defenseMultipler = 1.0;
	armorMultipler = 1.0;
	skillMultipliers[0] = 1.5f;
	skillMultipliers[1] = 2.0f;
	skillMultipliers[2] = 2.0f;
	skillMultipliers[3] = 2.0f;
	skillMultipliers[4] = 2.0f;
	skillMultipliers[5] = 1.5f;
	skillMultipliers[6] = 1.1f;

	memset(absorbPercent, 0, sizeof(absorbPercent));
}

Vocation::~Vocation()
{
	cacheMana.clear();
	for(int32_t i = SKILL_FIRST; i < SKILL_LAST; ++i)
		cacheSkill[i].clear();
}

uint32_t Vocation::getReqSkillTries(int32_t skill, int32_t level)
{
	if(skill < SKILL_FIRST || skill > SKILL_LAST)
		return 0;

	cacheMap& skillMap = cacheSkill[skill];
	cacheMap::iterator it = skillMap.find(level);
	if(it != cacheSkill[skill].end())
		return it->second;

	uint32_t tries = (uint32_t)(skillBase[skill] * pow((float)skillMultipliers[skill], (float)(level - 11)));
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
