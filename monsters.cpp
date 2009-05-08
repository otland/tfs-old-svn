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

#include "monsters.h"
#include "tools.h"
#include "monster.h"

#include "luascript.h"
#include "container.h"
#include "weapons.h"

#include "spells.h"
#include "combat.h"

#include "configmanager.h"
#include "game.h"

extern Game g_game;
extern Spells* g_spells;
extern Monsters g_monsters;
extern ConfigManager g_config;

void MonsterType::reset()
{
	canPushItems = canPushCreatures = isSummonable = isIllusionable = isConvinceable = isLureable = hideName = hideHealth = false;
	pushable = isAttackable = isHostile = true;

	outfit.lookHead = outfit.lookBody = outfit.lookLegs = outfit.lookFeet = outfit.lookType = outfit.lookTypeEx = outfit.lookAddons = 0;
	experience = defense = armor = lookCorpse = conditionImmunities = damageImmunities = 0;
	maxSummons = runAwayHealth = manaCost = lightLevel = lightColor = 0;
	yellSpeedTicks = yellChance = changeTargetSpeed = changeTargetChance = 0;

	targetDistance = 1;
	staticAttackChance = 95;
	health = healthMax = 100;
	baseSpeed = 200;

	race = RACE_BLOOD;
	skull = SKULL_NONE;
	partyShield = SHIELD_NONE;

	for(SpellList::iterator it = spellAttackList.begin(); it != spellAttackList.end(); ++it)
	{
		if(it->combatSpell)
		{
			delete it->spell;
			it->spell = NULL;
		}
	}

	spellAttackList.clear();
	for(SpellList::iterator it = spellDefenseList.begin(); it != spellDefenseList.end(); ++it)
	{
		if(it->combatSpell)
		{
			delete it->spell;
			it->spell = NULL;
		}
	}

	spellDefenseList.clear();
	voiceVector.clear();
	scriptList.clear();
	summonList.clear();
	lootItems.clear();
	elementMap.clear();
}

uint32_t Monsters::getLootRandom()
{
	return (uint32_t)std::ceil((double)random_range(0, MAX_LOOTCHANCE) / g_config.getDouble(ConfigManager::RATE_LOOT));
}

void MonsterType::createLoot(Container* corpse)
{
	ItemVector itemVector;
	for(LootItems::const_iterator it = lootItems.begin(); it != lootItems.end() && (corpse->capacity() - corpse->size() > 0); it++)
	{
		if(Item* tmpItem = createLootItem(*it))
		{
			if(Container* container = tmpItem->getContainer())
			{
				if(createLootContainer(container, (*it), itemVector))
				{
					corpse->__internalAddThing(tmpItem);
					itemVector.push_back(tmpItem);
				}
				else
					delete container;
			}
			else
			{
				corpse->__internalAddThing(tmpItem);
				itemVector.push_back(tmpItem);
			}
		}
	}

	corpse->__startDecaying();
	uint32_t ownerId = corpse->getCorpseOwner();
	if(ownerId)
	{
		Player* owner = NULL;
		if((owner = g_game.getPlayerByID(ownerId)) && owner->getParty())
			owner->getParty()->broadcastPartyLoot(nameDescription, itemVector);
	}
}

Item* MonsterType::createLootItem(const LootBlock& lootBlock)
{
	Item* tmpItem = NULL;
	if(Item::items[lootBlock.id].stackable)
	{
		uint32_t randvalue = Monsters::getLootRandom();
		if(randvalue < lootBlock.chance)
		{
			uint32_t n = randvalue % lootBlock.countmax + 1;
			tmpItem = Item::CreateItem(lootBlock.id, n);
		}
	}
	else
	{
		if(Monsters::getLootRandom() < lootBlock.chance)
			tmpItem = Item::CreateItem(lootBlock.id, 0);
	}

	if(tmpItem)
	{
		if(lootBlock.subType != -1)
			tmpItem->setSubType(lootBlock.subType);

		if(lootBlock.actionId != -1)
			tmpItem->setActionId(lootBlock.actionId);

		if(lootBlock.uniqueId != -1)
			tmpItem->setUniqueId(lootBlock.uniqueId);

		if(lootBlock.text != "")
			tmpItem->setText(lootBlock.text);

		return tmpItem;
	}

	return NULL;
}

bool MonsterType::createLootContainer(Container* parent, const LootBlock& lootblock, ItemVector& itemVector)
{
	LootItems::const_iterator it = lootblock.childLoot.begin();
	if(it == lootblock.childLoot.end())
		return true;

	for(; it != lootblock.childLoot.end() && parent->size() < parent->capacity(); ++it)
	{
		if(Item* tmpItem = createLootItem((*it)))
		{
			if(Container* container = tmpItem->getContainer())
			{
				if(createLootContainer(container, (*it), itemVector))
				{
					parent->__internalAddThing(container);
					itemVector.push_back(tmpItem);
				}
				else
					delete container;
			}
			else
			{
				parent->__internalAddThing(tmpItem);
				itemVector.push_back(tmpItem);
			}
		}
	}

	return parent->size() != 0;
}

bool Monsters::loadFromXml(bool reloading /*= false*/)
{
	loaded = false;
	xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_OTHER, "monster/monsters.xml").c_str());
	if(!doc)
	{
		std::cout << "[Warning - Monsters::loadFromXml] Cannot load monsters file." << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr p, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"monsters"))
	{
		std::cout << "[Error - Monsters::loadFromXml] Malformed monsters file." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	p = root->children;
	while(p)
	{
		if(p->type != XML_ELEMENT_NODE)
		{
			p = p->next;
			continue;
		}

		if(xmlStrcmp(p->name, (const xmlChar*)"monster"))
		{
			std::cout << "[Warning - Monsters::loadFromXml] Unknown node name (" << p->name << ")." << std::endl;
			p = p->next;
			continue;
		}

		std::string file, name;
		if(readXMLString(p, "file", file) && readXMLString(p, "name", name))
		{
			file = getFilePath(FILE_TYPE_OTHER, "monster/" + file);
			loadMonster(file, name, reloading);
		}

		p = p->next;
	}

	xmlFreeDoc(doc);
	loaded = true;
	return loaded;
}

ConditionDamage* Monsters::getDamageCondition(ConditionType_t conditionType,
	int32_t maxDamage, int32_t minDamage, int32_t startDamage, uint32_t tickInterval)
{
	if(ConditionDamage* condition = dynamic_cast<ConditionDamage*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, 0)))
	{
		condition->setParam(CONDITIONPARAM_TICKINTERVAL, tickInterval);
		condition->setParam(CONDITIONPARAM_MINVALUE, minDamage);
		condition->setParam(CONDITIONPARAM_MAXVALUE, maxDamage);
		condition->setParam(CONDITIONPARAM_STARTVALUE, startDamage);
		condition->setParam(CONDITIONPARAM_DELAYED, 1);
		return condition;
	}

	return NULL;
}

bool Monsters::deserializeSpell(xmlNodePtr node, spellBlock_t& sb, const std::string& description)
{
	sb.chance = 100;
	sb.speed = 2000;
	sb.range = 0;
	sb.minCombatValue = 0;
	sb.maxCombatValue = 0;
	sb.combatSpell = false;
	sb.isMelee = false;

	std::string name = "", scriptName = "";
	bool isScripted = false;

	if(readXMLString(node, "script", scriptName))
		isScripted = true;
	else if(!readXMLString(node, "name", name))
		return false;

	int32_t intValue;
	std::string strValue;
	if(readXMLInteger(node, "speed", intValue) || readXMLInteger(node, "interval", intValue))
		sb.speed = std::max(1, intValue);

	if(readXMLInteger(node, "chance", intValue))
	{
		if(intValue < 0 || intValue > 100)
			intValue = 100;

		sb.chance = intValue;
	}

	if(readXMLInteger(node, "range", intValue))
	{
		if(intValue < 0 )
			intValue = 0;

		if(intValue > Map::maxViewportX * 2)
			intValue = Map::maxViewportX * 2;

		sb.range = intValue;
	}

	if(readXMLInteger(node, "min", intValue))
		sb.minCombatValue = intValue;

	if(readXMLInteger(node, "max", intValue))
	{
		sb.maxCombatValue = intValue;

		//normalize values
		if(std::abs(sb.minCombatValue) > std::abs(sb.maxCombatValue))
		{
			int32_t value = sb.maxCombatValue;
			sb.maxCombatValue = sb.minCombatValue;
			sb.minCombatValue = value;
		}
	}

	if((sb.spell = g_spells->getSpellByName(name)))
		return true;

	CombatSpell* combatSpell = NULL;
	bool needTarget = false, needDirection = false;
	if(isScripted)
	{
		if(readXMLString(node, "direction", strValue))
			needDirection = booleanString(strValue);

		if(readXMLString(node, "target", strValue))
			needTarget = booleanString(strValue);

		combatSpell = new CombatSpell(NULL, needTarget, needDirection);
		if(!combatSpell->loadScript(getFilePath(FILE_TYPE_OTHER, g_spells->getScriptBaseName() + "/scripts/" + scriptName), true))
		{
			delete combatSpell;
			return false;
		}

		if(!combatSpell->loadScriptCombat())
		{
			delete combatSpell;
			return false;

		}

		combatSpell->getCombat()->setPlayerCombatValues(FORMULA_VALUE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
	}
	else
	{
		Combat* combat = new Combat;
		sb.combatSpell = true;
		if(readXMLInteger(node, "length", intValue))
		{
			int32_t length = intValue;
			if(length > 0)
			{
				int32_t spread = 3;

				//need direction spell
				if(readXMLInteger(node, "spread", intValue))
					spread = std::max(0, intValue);

				AreaCombat* area = new AreaCombat();
				area->setupArea(length, spread);
				combat->setArea(area);

				needDirection = true;
			}
		}

		if(readXMLInteger(node, "radius", intValue))
		{
			int32_t radius = intValue;

			//target spell
			if(readXMLInteger(node, "target", intValue))
				needTarget = (intValue != 0);

			AreaCombat* area = new AreaCombat();
			area->setupArea(radius);
			combat->setArea(area);
		}

		std::string tmpName = asLowerCaseString(name);
		if(tmpName == "melee")
		{
			int32_t attack = 0, skill = 0;
			sb.isMelee = true;
			if(readXMLInteger(node, "attack", attack) && readXMLInteger(node, "skill", skill))
			{
				sb.minCombatValue = 0;
				sb.maxCombatValue = -Weapons::getMaxMeleeDamage(skill, attack);
			}

			ConditionType_t conditionType = CONDITION_NONE;
			int32_t startDamage = 0, minDamage = 0, maxDamage = 0;
			uint32_t tickInterval = 2000;
			if(readXMLInteger(node, "fire", intValue))
			{
				conditionType = CONDITION_FIRE;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 10000;
			}
			else if(readXMLInteger(node, "energy", intValue))
			{
				conditionType = CONDITION_ENERGY;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 10000;
			}
			else if(readXMLInteger(node, "poison", intValue) || readXMLInteger(node, "earth", intValue))
			{
				conditionType = CONDITION_POISON;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "freeze", intValue))
			{
				conditionType = CONDITION_FREEZING;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 10000;
			}
			else if(readXMLInteger(node, "dazzle", intValue))
			{
				conditionType = CONDITION_DAZZLED;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 10000;
			}
			else if(readXMLInteger(node, "curse", intValue))
			{
				conditionType = CONDITION_CURSED;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 10000;
			}
			else if(readXMLInteger(node, "drown", intValue))
			{
				conditionType = CONDITION_DROWN;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 10000;
			}
			else if(readXMLInteger(node, "physical", intValue))
			{
				conditionType = CONDITION_PHYSICAL;

				minDamage = intValue;
				maxDamage = intValue;
				tickInterval = 10000;
			}

			if(readXMLInteger(node, "tick", intValue) && intValue > 0)
				tickInterval = intValue;

			if(conditionType != CONDITION_NONE)
			{
				Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, startDamage, tickInterval);
				if(condition)
					combat->setCondition(condition);
			}

			sb.range = 1;
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
			combat->setParam(COMBATPARAM_BLOCKEDBYSHIELD, 1);
		}
		else if(tmpName == "physical")
		{
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
		}
		else if(tmpName == "drown")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_DROWNDAMAGE);
		else if(tmpName == "fire")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_FIREDAMAGE);
		else if(tmpName == "energy")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_ENERGYDAMAGE);
		else if(tmpName == "poison" || tmpName == "earth")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_EARTHDAMAGE);
		else if(tmpName == "ice")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_ICEDAMAGE);
		else if(tmpName == "holy")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_HOLYDAMAGE);
		else if(tmpName == "death")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_DEATHDAMAGE);
		else if(tmpName == "lifedrain")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_LIFEDRAIN);
		else if(tmpName == "manadrain")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_MANADRAIN);
		else if(tmpName == "healing")
		{
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_HEALING);
			combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
		}
		else if(tmpName == "undefined")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_UNDEFINEDDAMAGE);
		else if(tmpName == "speed")
		{
			int32_t speedChange = 0, duration = 10000;
			if(readXMLInteger(node, "duration", intValue))
				duration = intValue;

			if(readXMLInteger(node, "speedchange", intValue))
			{
				speedChange = intValue;
				if(speedChange < -1000)
				{
					//cant be slower than 100%
					speedChange = -1000;
				}
			}

			ConditionType_t conditionType;
			if(speedChange > 0)
			{
				conditionType = CONDITION_HASTE;
				combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
			}
			else
				conditionType = CONDITION_PARALYZE;

			if(ConditionSpeed* condition = dynamic_cast<ConditionSpeed*>(Condition::createCondition(
				CONDITIONID_COMBAT, conditionType, duration)))
			{
				condition->setFormulaVars(speedChange / 1000.0, 0, speedChange / 1000.0, 0);
				combat->setCondition(condition);
			}
		}
		else if(tmpName == "outfit")
		{
			int32_t duration = 10000;
			if(readXMLInteger(node, "duration", intValue))
				duration = intValue;

			if(readXMLString(node, "monster", strValue))
			{
				MonsterType* mType = g_monsters.getMonsterType(strValue);
				if(mType)
				{
					if(ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(
						CONDITIONID_COMBAT, CONDITION_OUTFIT, duration)))
					{
						condition->addOutfit(mType->outfit);
						combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
						combat->setCondition(condition);
					}
				}
			}
			else if(readXMLInteger(node, "item", intValue))
			{
				Outfit_t outfit;
				outfit.lookTypeEx = intValue;

				if(ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(
					CONDITIONID_COMBAT, CONDITION_OUTFIT, duration)))
				{
					condition->addOutfit(outfit);
					combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
					combat->setCondition(condition);
				}
			}
		}
		else if(tmpName == "invisible")
		{
			int32_t duration = 10000;
			if(readXMLInteger(node, "duration", intValue))
				duration = intValue;

			if(Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_INVISIBLE, duration))
			{
				combat->setParam(COMBATPARAM_AGGRESSIVE, 0);
				combat->setCondition(condition);
			}
		}
		else if(tmpName == "drunk")
		{
			int32_t duration = 10000;
			if(readXMLInteger(node, "duration", intValue))
				duration = intValue;

			if(Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_DRUNK, duration))
				combat->setCondition(condition);
		}
		else if(tmpName == "firefield")
			combat->setParam(COMBATPARAM_CREATEITEM, 1492);
		else if(tmpName == "poisonfield")
			combat->setParam(COMBATPARAM_CREATEITEM, 1496);
		else if(tmpName == "energyfield")
			combat->setParam(COMBATPARAM_CREATEITEM, 1495);
		else if(tmpName == "firecondition" || tmpName == "energycondition" || tmpName == "drowncondition" ||
			tmpName == "poisoncondition" || tmpName == "earthcondition" || tmpName == "freezecondition" ||
			tmpName == "cursecondition" || tmpName == "dazzlecondition")
		{
			ConditionType_t conditionType = CONDITION_NONE;
			uint32_t tickInterval = 2000;
			if(tmpName == "firecondition")
			{
				conditionType = CONDITION_FIRE;
				tickInterval = 10000;
			}
			else if(tmpName == "energycondition")
			{
				conditionType = CONDITION_ENERGY;
				tickInterval = 10000;
			}
			else if(tmpName == "poisoncondition" || tmpName == "earthcondition")
			{
				conditionType = CONDITION_POISON;
				tickInterval = 5000;
			}
			else if(tmpName == "freezecondition")
			{
				conditionType = CONDITION_FREEZING;
				tickInterval = 10000;
			}
			else if(tmpName == "cursecondition")
			{
				conditionType = CONDITION_CURSED;
				tickInterval = 10000;
			}
			else if(tmpName == "dazzlecondition")
			{
				conditionType = CONDITION_CURSED;
				tickInterval = 10000;
			}
			else if(tmpName == "drowncondition")
			{
				conditionType = CONDITION_DROWN;
				tickInterval = 5000;
			}
			else if(tmpName == "physicalcondition")
			{
				conditionType = CONDITION_PHYSICAL;
				tickInterval = 5000;
			}

			if(readXMLInteger(node, "tick", intValue) && intValue > 0)
				tickInterval = intValue;

			int32_t startDamage = 0, minDamage = std::abs(sb.minCombatValue), maxDamage = std::abs(sb.maxCombatValue);
			if(readXMLInteger(node, "start", intValue))
			{
				intValue = std::abs(intValue);
				if(intValue <= minDamage)
					startDamage = intValue;
			}

			Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, startDamage, tickInterval);
			if(condition)
				combat->setCondition(condition);
		}
		else if(tmpName == "strength")
		{
			//TODO?
		}
		else
		{
			std::cout << "Error: [Monsters::deserializeSpell] - " << description <<  " - Unknown spell name: " << name << std::endl;
			delete combat;
			return false;
		}

		combat->setPlayerCombatValues(FORMULA_VALUE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
		combatSpell = new CombatSpell(combat, needTarget, needDirection);

		xmlNodePtr attributeNode = node->children;
		while(attributeNode)
		{
			if(xmlStrcmp(attributeNode->name, (const xmlChar*)"attribute") == 0)
			{
				if(readXMLString(attributeNode, "key", strValue))
				{
					std::string tmpStrValue = asLowerCaseString(strValue);
					if(tmpStrValue == "shooteffect")
					{
						if(readXMLString(attributeNode, "value", strValue))
						{
							ShootType_t shoot = getShootType(strValue);
							if(shoot != NM_SHOOT_UNK)
								combat->setParam(COMBATPARAM_DISTANCEEFFECT, shoot);
							else
								std::cout << "Warning: [Monsters::deserializeSpell] - "  << description << " - Unknown shootEffect: " << strValue << std::endl;
						}
					}
					else if(tmpStrValue == "areaeffect")
					{
						if(readXMLString(attributeNode, "value", strValue))
						{
							MagicEffectClasses effect = getMagicEffect(strValue);
							if(effect != NM_ME_UNK)
								combat->setParam(COMBATPARAM_EFFECT, effect);
							else
								std::cout << "Warning: [Monsters::deserializeSpell] - "  << description << " - Unknown areaEffect: " << strValue << std::endl;
						}
					}
					else
						std::cout << "[Warning - Monsters::deserializeSpells] Effect type \"" << strValue << "\" does not exist." << std::endl;
				}
			}
			attributeNode = attributeNode->next;
		}
	}

	sb.spell = combatSpell;
	return true;
}

#define SHOW_XML_WARNING(desc) std::cout << "[Warning - Monsters::loadMonster] " << desc << ". (" << file << ")" << std::endl;
#define SHOW_XML_ERROR(desc) std::cout << "[Error - Monsters::loadMonster] " << desc << ". (" << file << ")" << std::endl;

bool Monsters::loadMonster(const std::string& file, const std::string& monsterName, bool reloading /*= false*/)
{
	if(getIdByName(monsterName) != 0 && !reloading)
	{
		std::cout << "[Warning - Monsters::loadMonster] Duplicate registered monster with name: " << monsterName << std::endl;
		return true;
	}

	bool monsterLoad, new_mType = true;
	MonsterType* mType = NULL;
	if(reloading)
	{
		uint32_t id = getIdByName(monsterName);
		if(id != 0)
		{
			mType = getMonsterType(id);
			if(mType != NULL)
			{
				new_mType = false;
				mType->reset();
			}
		}
	}

	if(new_mType)
		mType = new MonsterType();

	xmlDocPtr doc = xmlParseFile(file.c_str());
	if(!doc)
	{
		std::cout << "[Warning - Monsters::loadMonster] Cannot load monster (" << monsterName << ") file (" << file << ")." << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	monsterLoad = true;
	xmlNodePtr p, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"monster"))
	{
		std::cout << "[Error - Monsters::loadMonster] Malformed monster (" << monsterName << ") file (" << file << ")." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	int32_t intValue;
	std::string strValue;
	if(readXMLString(root, "name", strValue))
		mType->name = strValue;
	else
		monsterLoad = false;

	if(readXMLString(root, "nameDescription", strValue))
		mType->nameDescription = strValue;
	else
	{
		mType->nameDescription = "a " + mType->name;
		toLowerCaseString(mType->nameDescription);
	}

	if(readXMLString(root, "race", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "venom" || atoi(strValue.c_str()) == 1)
			mType->race = RACE_VENOM;
		else if(tmpStrValue == "blood" || atoi(strValue.c_str()) == 2)
			mType->race = RACE_BLOOD;
		else if(tmpStrValue == "undead" || atoi(strValue.c_str()) == 3)
			mType->race = RACE_UNDEAD;
		else if(tmpStrValue == "fire" || atoi(strValue.c_str()) == 4)
			mType->race = RACE_FIRE;
		else if(tmpStrValue == "energy" || atoi(strValue.c_str()) == 5)
			mType->race = RACE_ENERGY;
		else
			SHOW_XML_WARNING("Unknown race type " << strValue);
	}

	if(readXMLInteger(root, "experience", intValue))
		mType->experience = intValue;

	if(readXMLInteger(root, "speed", intValue))
		mType->baseSpeed = intValue;

	if(readXMLInteger(root, "manacost", intValue))
		mType->manaCost = intValue;

	if(readXMLString(root, "skull", strValue))
		mType->skull = getSkull(strValue);

	if(readXMLString(root, "shield", strValue))
		mType->partyShield = getPartyShield(strValue);

	p = root->children;
	while(p && monsterLoad)
	{
		if(p->type != XML_ELEMENT_NODE)
		{
			p = p->next;
			continue;
		}

		if(!xmlStrcmp(p->name, (const xmlChar*)"health"))
		{
			if(readXMLInteger(p, "now", intValue))
				mType->health = intValue;
			else
			{
				SHOW_XML_ERROR("Missing health.now");
				monsterLoad = false;
			}

			if(readXMLInteger(p, "max", intValue))
				mType->healthMax = intValue;
			else
			{
				SHOW_XML_ERROR("Missing health.max");
				monsterLoad = false;
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"flags"))
		{
			xmlNodePtr tmpNode = p->children;
			while(tmpNode)
			{
				if(xmlStrcmp(tmpNode->name, (const xmlChar*)"flag") == 0)
				{
					if(readXMLString(tmpNode, "summonable", strValue))
						mType->isSummonable = booleanString(strValue);

					if(readXMLString(tmpNode, "attackable", strValue))
						mType->isAttackable = booleanString(strValue);

					if(readXMLString(tmpNode, "hostile", strValue))
						mType->isHostile = booleanString(strValue);

					if(readXMLString(tmpNode, "illusionable", strValue))
						mType->isIllusionable = booleanString(strValue);

					if(readXMLString(tmpNode, "convinceable", strValue))
						mType->isConvinceable = booleanString(strValue);

					if(readXMLString(tmpNode, "pushable", strValue))
						mType->pushable = booleanString(strValue);

					if(readXMLString(tmpNode, "canpushitems", strValue))
						mType->canPushItems = booleanString(strValue);

					if(readXMLString(tmpNode, "canpushcreatures", strValue))
						mType->canPushCreatures = booleanString(strValue);

					if(readXMLString(tmpNode, "hidename", strValue))
						mType->hideName = booleanString(strValue);

					if(readXMLString(tmpNode, "hidehealth", strValue))
						mType->hideHealth = booleanString(strValue);

					if(readXMLInteger(tmpNode, "staticattack", intValue))
					{
						if(intValue < 0 || intValue > 100)
						{
							SHOW_XML_WARNING("staticattack lower than 0 or greater than 100");
							intValue = 0;
						}

						mType->staticAttackChance = intValue;
					}

					if(readXMLInteger(tmpNode, "lightlevel", intValue))
						mType->lightLevel = intValue;

					if(readXMLInteger(tmpNode, "lightcolor", intValue))
						mType->lightColor = intValue;

					if(readXMLInteger(tmpNode, "targetdistance", intValue))
					{
						if(intValue > Map::maxViewportX)
							SHOW_XML_WARNING("targetdistance greater than maxViewportX");

						mType->targetDistance = std::max(1, intValue);
					}

					if(readXMLInteger(tmpNode, "runonhealth", intValue))
						mType->runAwayHealth = intValue;

					if(readXMLString(tmpNode, "lureable", strValue))
						mType->isLureable = booleanString(strValue);

					if(readXMLString(tmpNode, "skull", strValue))
						mType->skull = getSkull(strValue);

					if(readXMLString(tmpNode, "shield", strValue))
						mType->partyShield = getPartyShield(strValue);
				}

				tmpNode = tmpNode->next;
			}

			//if a monster can push creatures, it should not be pushable
			if(mType->canPushCreatures && mType->pushable)
				mType->pushable = false;
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"targetchange"))
		{
			if(readXMLInteger(p, "speed", intValue) || readXMLInteger(p, "interval", intValue))
				mType->changeTargetSpeed = std::max(1, intValue);
			else
				SHOW_XML_WARNING("Missing targetchange.speed");

			if(readXMLInteger(p, "chance", intValue))
				mType->changeTargetChance = intValue;
			else
				SHOW_XML_WARNING("Missing targetchange.chance");
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"strategy"))
		{
			if(readXMLInteger(p, "attack", intValue)) {}
				//mType->attackStrength = intValue;

			if(readXMLInteger(p, "defense", intValue)) {}
				//mType->defenseStrength = intValue;
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"look"))
		{
			if(readXMLInteger(p, "type", intValue))
			{
				mType->outfit.lookType = intValue;
				if(readXMLInteger(p, "head", intValue))
					mType->outfit.lookHead = intValue;

				if(readXMLInteger(p, "body", intValue))
					mType->outfit.lookBody = intValue;

				if(readXMLInteger(p, "legs", intValue))
					mType->outfit.lookLegs = intValue;

				if(readXMLInteger(p, "feet", intValue))
					mType->outfit.lookFeet = intValue;

				if(readXMLInteger(p, "addons", intValue))
					mType->outfit.lookAddons = intValue;
			}
			else if(readXMLInteger(p, "typeex", intValue))
				mType->outfit.lookTypeEx = intValue;
			else
				SHOW_XML_WARNING("Missing look type/typeex");

			if(readXMLInteger(p, "corpse", intValue))
				mType->lookCorpse = intValue;
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"attacks"))
		{
			xmlNodePtr tmpNode = p->children;
			while(tmpNode)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"attack"))
				{
					spellBlock_t sb;
					if(deserializeSpell(tmpNode, sb, monsterName))
						mType->spellAttackList.push_back(sb);
					else
						SHOW_XML_WARNING("Cant load spell");
				}

				tmpNode = tmpNode->next;
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"defenses"))
		{
			if(readXMLInteger(p, "defense", intValue))
				mType->defense = intValue;

			if(readXMLInteger(p, "armor", intValue))
				mType->armor = intValue;

			xmlNodePtr tmpNode = p->children;
			while(tmpNode)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"defense"))
				{
					spellBlock_t sb;
					if(deserializeSpell(tmpNode, sb, monsterName))
						mType->spellDefenseList.push_back(sb);
					else
						SHOW_XML_WARNING("Cant load spell");
				}

				tmpNode = tmpNode->next;
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"immunities"))
		{
			xmlNodePtr tmpNode = p->children;
			while(tmpNode)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"immunity"))
				{
					if(readXMLString(tmpNode, "name", strValue))
					{
						std::string tmpStrValue = asLowerCaseString(strValue);
						if(tmpStrValue == "physical")
						{
							mType->damageImmunities |= COMBAT_PHYSICALDAMAGE;
							mType->conditionImmunities |= CONDITION_PHYSICAL;
						}
						else if(tmpStrValue == "energy")
						{
							mType->damageImmunities |= COMBAT_ENERGYDAMAGE;
							mType->conditionImmunities |= CONDITION_ENERGY;
						}
						else if(tmpStrValue == "fire")
						{
							mType->damageImmunities |= COMBAT_FIREDAMAGE;
							mType->conditionImmunities |= CONDITION_FIRE;
						}
						else if(tmpStrValue == "poison" || tmpStrValue == "earth")
						{
							mType->damageImmunities |= COMBAT_EARTHDAMAGE;
							mType->conditionImmunities |= CONDITION_POISON;
						}
						else if(tmpStrValue == "ice")
						{
							mType->damageImmunities |= COMBAT_ICEDAMAGE;
							mType->conditionImmunities |= CONDITION_FREEZING;
						}
						else if(tmpStrValue == "holy")
						{
							mType->damageImmunities |= COMBAT_HOLYDAMAGE;
							mType->conditionImmunities |= CONDITION_DAZZLED;
						}
						else if(tmpStrValue == "death")
						{
							mType->damageImmunities |= COMBAT_DEATHDAMAGE;
							mType->conditionImmunities |= CONDITION_CURSED;
						}
						else if(tmpStrValue == "drown")
						{
							mType->damageImmunities |= COMBAT_DROWNDAMAGE;
							mType->conditionImmunities |= CONDITION_DROWN;
						}
						else if(tmpStrValue == "lifedrain")
							mType->damageImmunities |= COMBAT_LIFEDRAIN;
						else if(tmpStrValue == "manadrain")
							mType->damageImmunities |= COMBAT_MANADRAIN;
						else if(tmpStrValue == "paralyze")
							mType->conditionImmunities |= CONDITION_PARALYZE;
						else if(tmpStrValue == "outfit")
							mType->conditionImmunities |= CONDITION_OUTFIT;
						else if(tmpStrValue == "drunk")
							mType->conditionImmunities |= CONDITION_DRUNK;
						else if(tmpStrValue == "invisible")
							mType->conditionImmunities |= CONDITION_INVISIBLE;
						else
							SHOW_XML_WARNING("Unknown immunity name " << strValue);
					}
					else if(readXMLString(tmpNode, "physical", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_PHYSICALDAMAGE;
						//mType->conditionImmunities |= CONDITION_PHYSICAL;
					}
					else if(readXMLString(tmpNode, "energy", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_ENERGYDAMAGE;
						mType->conditionImmunities |= CONDITION_ENERGY;
					}
					else if(readXMLString(tmpNode, "fire", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_FIREDAMAGE;
						mType->conditionImmunities |= CONDITION_FIRE;
					}
					else if((readXMLString(tmpNode, "poison", strValue) || readXMLString(tmpNode, "earth", strValue))
						&& booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_EARTHDAMAGE;
						mType->conditionImmunities |= CONDITION_POISON;
					}
					else if(readXMLString(tmpNode, "drown", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_DROWNDAMAGE;
						mType->conditionImmunities |= CONDITION_DROWN;
					}
					else if(readXMLString(tmpNode, "ice", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_ICEDAMAGE;
						mType->conditionImmunities |= CONDITION_FREEZING;
					}
					else if(readXMLString(tmpNode, "holy", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_HOLYDAMAGE;
						mType->conditionImmunities |= CONDITION_DAZZLED;
					}
					else if(readXMLString(tmpNode, "death", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_DEATHDAMAGE;
						mType->conditionImmunities |= CONDITION_CURSED;
					}
					else if(readXMLString(tmpNode, "lifedrain", strValue) && booleanString(strValue))
						mType->damageImmunities |= COMBAT_LIFEDRAIN;
					else if(readXMLString(tmpNode, "manadrain", strValue) && booleanString(strValue))
						mType->damageImmunities |= COMBAT_LIFEDRAIN;
					else if(readXMLString(tmpNode, "paralyze", strValue) && booleanString(strValue))
						mType->conditionImmunities |= CONDITION_PARALYZE;
					else if(readXMLString(tmpNode, "outfit", strValue) && booleanString(strValue))
						mType->conditionImmunities |= CONDITION_OUTFIT;
					else if(readXMLString(tmpNode, "drunk", strValue) && booleanString(strValue))
						mType->conditionImmunities |= CONDITION_DRUNK;
					else if(readXMLString(tmpNode, "invisible", strValue) && booleanString(strValue))
						mType->conditionImmunities |= CONDITION_INVISIBLE;
				}

				tmpNode = tmpNode->next;
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"voices"))
		{
			if(readXMLInteger(p, "speed", intValue) || readXMLInteger(p, "interval", intValue))
				mType->yellSpeedTicks = intValue;
			else
				SHOW_XML_WARNING("Missing voices.speed");

			if(readXMLInteger(p, "chance", intValue))
				mType->yellChance = intValue;
			else
				SHOW_XML_WARNING("Missing voices.chance");

			xmlNodePtr tmpNode = p->children;
			while(tmpNode)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"voice"))
				{
					voiceBlock_t vb;
					vb.text = "";
					vb.yellText = false;

					if(readXMLString(tmpNode, "sentence", strValue))
						vb.text = strValue;
					else
						SHOW_XML_WARNING("Missing voice.sentence");

					if(readXMLString(tmpNode, "yell", strValue))
						vb.yellText = booleanString(strValue);

					mType->voiceVector.push_back(vb);
				}

				tmpNode = tmpNode->next;
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"loot"))
		{
			xmlNodePtr tmpNode = p->children;
			while(tmpNode)
			{
				if(tmpNode->type != XML_ELEMENT_NODE)
				{
					tmpNode = tmpNode->next;
					continue;
				}

				LootBlock lootBlock;
				if(loadLootItem(tmpNode, lootBlock))
					mType->lootItems.push_back(lootBlock);
				else
					SHOW_XML_WARNING("Cant load loot");

				tmpNode = tmpNode->next;
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"elements"))
		{
			xmlNodePtr tmpNode = p->children;
			while(tmpNode)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"element"))
				{
					if(readXMLInteger(tmpNode, "firePercent", intValue))
						mType->elementMap[COMBAT_FIREDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "energyPercent", intValue))
						mType->elementMap[COMBAT_ENERGYDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "icePercent", intValue))
						mType->elementMap[COMBAT_ICEDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "poisonPercent", intValue) || readXMLInteger(tmpNode, "earthPercent", intValue))
						mType->elementMap[COMBAT_EARTHDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "holyPercent", intValue))
						mType->elementMap[COMBAT_HOLYDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "deathPercent", intValue))
						mType->elementMap[COMBAT_DEATHDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "drownPercent", intValue))
						mType->elementMap[COMBAT_DROWNDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "physicalPercent", intValue))
						mType->elementMap[COMBAT_PHYSICALDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "lifeDrainPercent", intValue))
						mType->elementMap[COMBAT_LIFEDRAIN] = intValue;
					else if(readXMLInteger(tmpNode, "manaDrainPercent", intValue))
						mType->elementMap[COMBAT_MANADRAIN] = intValue;
					else if(readXMLInteger(tmpNode, "healingPercent", intValue))
						mType->elementMap[COMBAT_HEALING] = intValue;
					else if(readXMLInteger(tmpNode, "undefinedPercent", intValue))
						mType->elementMap[COMBAT_UNDEFINEDDAMAGE] = intValue;
				}

				tmpNode = tmpNode->next;
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"summons"))
		{
			if(readXMLInteger(p, "maxSummons", intValue))
				mType->maxSummons = std::min(intValue, 100);
			else
				SHOW_XML_WARNING("Missing summons.maxSummons");

			xmlNodePtr tmpNode = p->children;
			while(tmpNode)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"summon"))
				{
					uint32_t chance = 100, interval = 1000, amount = 1;
					if(readXMLInteger(tmpNode, "speed", intValue) || readXMLInteger(tmpNode, "interval", intValue))
						interval = intValue;

					if(readXMLInteger(tmpNode, "chance", intValue))
						chance = intValue;

					if(readXMLInteger(tmpNode, "amount", intValue) || readXMLInteger(tmpNode, "max", intValue))
						amount = intValue;

					if(readXMLString(tmpNode, "name", strValue))
					{
						summonBlock_t sb;
						sb.name = strValue;
						sb.interval = interval;
						sb.chance = chance;
						sb.amount = amount;

						mType->summonList.push_back(sb);
					}
					else
						SHOW_XML_WARNING("Missing summon.name");
				}

				tmpNode = tmpNode->next;
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"script"))
		{
			xmlNodePtr tmpNode = p->children;
			while(tmpNode)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"event"))
				{
					if(readXMLString(tmpNode, "name", strValue))
						mType->scriptList.push_back(strValue);
					else
						SHOW_XML_WARNING("Missing name for script event");
				}

				tmpNode = tmpNode->next;
			}
		}
		else
			SHOW_XML_WARNING("Unknown attribute type - " << p->name);

		p = p->next;
	}

	xmlFreeDoc(doc);
	if(monsterLoad)
	{
		static uint32_t id = 0;
		if(new_mType)
		{
			id++;
			monsterNames[asLowerCaseString(monsterName)] = id;
			monsters[id] = mType;
		}

		return true;
	}

	if(new_mType)
		delete mType;

	return false;
}

bool Monsters::loadLootItem(xmlNodePtr node, LootBlock& lootBlock)
{
	int32_t intValue;
	std::string strValue;
	if(readXMLInteger(node, "id", intValue))
		lootBlock.id = intValue;

	if(lootBlock.id == 0)
		return false;

	if(readXMLInteger(node, "countmax", intValue))
	{
		lootBlock.countmax = intValue;
		if(lootBlock.countmax > 100)
			lootBlock.countmax = 100;
	}
	else
		lootBlock.countmax = 1;

	if(readXMLInteger(node, "chance", intValue) || readXMLInteger(node, "chance1", intValue))
	{
		lootBlock.chance = intValue;
		if(lootBlock.chance > MAX_LOOTCHANCE)
			lootBlock.chance = MAX_LOOTCHANCE;
	}
	else
		lootBlock.chance = MAX_LOOTCHANCE;

	if(Item::items[lootBlock.id].isContainer())
		loadLootContainer(node, lootBlock);

	//optional
	if(readXMLInteger(node, "subtype", intValue) || readXMLInteger(node, "subType", intValue))
		lootBlock.subType = intValue;

	if(readXMLInteger(node, "actionId", intValue) || readXMLInteger(node, "actionid", intValue)
		|| readXMLInteger(node, "aid", intValue))
			lootBlock.actionId = intValue;

	if(readXMLInteger(node, "uniqueId", intValue) || readXMLInteger(node, "uniqueid", intValue)
		|| readXMLInteger(node, "uid", intValue))
			lootBlock.uniqueId = intValue;

	if(readXMLString(node, "text", strValue))
		lootBlock.text = strValue;

	return true;
}

bool Monsters::loadLootContainer(xmlNodePtr node, LootBlock& lBlock)
{
	if(node == NULL)
		return false;

	xmlNodePtr p, tmpNode = node->children;
	if(tmpNode == NULL)
		return false;

	while(tmpNode)
	{
		if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"inside"))
		{
			p = tmpNode->children;
			while(p)
			{
				LootBlock lootBlock;
				if(loadLootItem(p, lootBlock))
					lBlock.childLoot.push_back(lootBlock);
				p = p->next;
			}

			return true;
		}

		tmpNode = tmpNode->next;
	}

	return false;
}

MonsterType* Monsters::getMonsterType(const std::string& name)
{
	uint32_t mId = getIdByName(name);
	if(mId != 0)
		return getMonsterType(mId);

	return NULL;
}

MonsterType* Monsters::getMonsterType(uint32_t mid)
{
	MonsterMap::iterator it = monsters.find(mid);
	if(it != monsters.end())
		return it->second;

	return NULL;
}

uint32_t Monsters::getIdByName(const std::string& name)
{
	std::string tmp = name;
	MonsterNameMap::iterator it = monsterNames.find(asLowerCaseString(tmp));
	if(it != monsterNames.end())
		return it->second;

	return 0;
}

Monsters::~Monsters()
{
	loaded = false;
	for(MonsterMap::iterator it = monsters.begin(); it != monsters.end(); it++)
		delete it->second;
}
