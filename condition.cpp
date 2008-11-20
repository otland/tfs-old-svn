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

#include "condition.h"
#include "game.h"
#include "creature.h"
#include "tools.h"
#include "combat.h"

#include <utility>

extern Game g_game;

Condition::Condition(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
id(_id), ticks(_ticks), endTime(0), conditionType(_type)
{
	//
}

bool Condition::setParam(ConditionParam_t param, int32_t value)
{
	switch(param)
	{
		case CONDITIONPARAM_TICKS:
			ticks = value;
			return true;

		default:
			break;
	}

	return false;
}

bool Condition::unserialize(xmlNodePtr p)
{
	return true;
}

xmlNodePtr Condition::serialize()
{
	xmlNodePtr nodeCondition = xmlNewNode(NULL,(const xmlChar*)"condition");

	char buffer[20];
	sprintf(buffer, "%d", conditionType);
	xmlSetProp(nodeCondition, (const xmlChar*)"type", (const xmlChar*)buffer);

	sprintf(buffer, "%d", id);
	xmlSetProp(nodeCondition, (const xmlChar*)"id", (const xmlChar*)buffer);

	sprintf(buffer, "%d", ticks);
	xmlSetProp(nodeCondition, (const xmlChar*)"ticks", (const xmlChar*)buffer);
	return nodeCondition;
}

bool Condition::unserialize(PropStream& propStream)
{
	uint8_t attrType;
	while(propStream.GET_UCHAR(attrType) && attrType != CONDITIONATTR_END)
	{
		if(!unserializeProp((ConditionAttr_t)attrType, propStream))
			return false;
	}

	return true;
}

bool Condition::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case CONDITIONATTR_TYPE:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			conditionType = (ConditionType_t)value;
			return true;
		}

		case CONDITIONATTR_ID:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			id = (ConditionId_t)value;
			return true;
		}

		case CONDITIONATTR_TICKS:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			ticks = value;
			return true;
		}

		case CONDITIONATTR_END:
			return true;

		default:
			break;
	}

	return false;
}

bool Condition::serialize(PropWriteStream& propWriteStream)
{
	propWriteStream.ADD_UCHAR(CONDITIONATTR_TYPE);
	propWriteStream.ADD_VALUE((int32_t)conditionType);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_ID);
	propWriteStream.ADD_VALUE((int32_t)id);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_TICKS);
	propWriteStream.ADD_VALUE((int32_t)ticks);
	return true;
}

void Condition::setTicks(int32_t _ticks)
{
	ticks = _ticks;
	if(_ticks > 0)
		endTime = OTSYS_TIME() + _ticks;
}

bool Condition::startCondition(Creature* creature)
{
	if(ticks > 0)
		endTime = OTSYS_TIME() + ticks;

	return true;
}

bool Condition::executeCondition(Creature* creature, int32_t interval)
{
	if(ticks == -1)
		return true;

	ticks = std::max((int32_t)0, (ticks - interval));
	return endTime >= OTSYS_TIME();
}

Condition* Condition::createCondition(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, int32_t param)
{
	switch((int32_t)_type)
	{
		case CONDITION_POISON:
		case CONDITION_FIRE:
		case CONDITION_ENERGY:
		case CONDITION_DROWN:
		case CONDITION_FREEZING:
		case CONDITION_DAZZLED:
		case CONDITION_CURSED:
			return new ConditionDamage(_id, _type);

		case CONDITION_HASTE:
		case CONDITION_PARALYZE:
			return new ConditionSpeed(_id, _type, _ticks, param);

		case CONDITION_INVISIBLE:
			return new ConditionInvisible(_id, _type, _ticks);

		case CONDITION_OUTFIT:
			return new ConditionOutfit(_id, _type, _ticks);

		case CONDITION_LIGHT:
			return new ConditionLight(_id, _type, _ticks, param & 0xFF, (param & 0xFF00) >> 8);

		case CONDITION_REGENERATION:
			return new ConditionRegeneration(_id, _type, _ticks);

		case CONDITION_SOUL:
			return new ConditionSoul(_id, _type, _ticks);

		case CONDITION_MANASHIELD:
			return new ConditionManaShield(_id, _type,_ticks);

		case CONDITION_ATTRIBUTES:
			return new ConditionAttributes(_id, _type,_ticks);

		case CONDITION_INFIGHT:
		case CONDITION_DRUNK:
		case CONDITION_EXHAUST_WEAPON:
		case CONDITION_EXHAUST_COMBAT:
		case CONDITION_EXHAUST_HEAL:
		case CONDITION_MUTED:
		case CONDITION_TRADETICKS:
		case CONDITION_YELLTICKS:
			return new ConditionGeneric(_id, _type, _ticks);

		default:
			break;
	}

	return NULL;
}


Condition* Condition::createCondition(PropStream& propStream)
{
	uint8_t attr = 0;
	if(!propStream.GET_UCHAR(attr) || attr != CONDITIONATTR_TYPE)
		return NULL;

	uint32_t _type = 0;
	if(!propStream.GET_ULONG(_type))
		return NULL;

	if(!propStream.GET_UCHAR(attr) || attr != CONDITIONATTR_ID)
		return NULL;

	uint32_t _id = 0;
	if(!propStream.GET_ULONG(_id))
		return NULL;

	if(!propStream.GET_UCHAR(attr) || attr != CONDITIONATTR_TICKS)
		return NULL;

	uint32_t _ticks = 0;
	if(!propStream.GET_ULONG(_ticks))
		return NULL;

	return createCondition((ConditionId_t)_id, (ConditionType_t)_type, _ticks, 0);
}

bool Condition::isPersistent() const
{
	if(ticks == -1)
		return false;

	if(!(id == CONDITIONID_DEFAULT || id == CONDITIONID_COMBAT))
		return false;

	return true;
}

bool Condition::updateCondition(const Condition* addCondition)
{
	if(conditionType != addCondition->getType())
		return false;

	if(addCondition->getTicks() > 0 && (ticks == -1 || addCondition->getTicks() <= ticks))
		return false;

	return true;
}

ConditionGeneric::ConditionGeneric(ConditionId_t _id, ConditionType_t _type, int32_t _ticks):
Condition(_id, _type, _ticks)
{
	//
}

bool ConditionGeneric::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
}

void ConditionGeneric::endCondition(Creature* creature, ConditionEnd_t reason)
{
	//
}

void ConditionGeneric::addCondition(Creature* creature, const Condition* addCondition)
{
	if(updateCondition(addCondition))
		setTicks(addCondition->getTicks());
}

uint32_t ConditionGeneric::getIcons() const
{
	switch(conditionType)
	{
		case CONDITION_MANASHIELD:
			return ICON_MANASHIELD;

		case CONDITION_INFIGHT:
			return ICON_SWORDS;

		case CONDITION_DRUNK:
			return ICON_DRUNK;

		default:
			break;
	}
	return 0;
}

ConditionAttributes::ConditionAttributes(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
ConditionGeneric(_id, _type, _ticks)
{
	currentSkill = currentStat = 0;
	memset(skills, 0, sizeof(skills));
	memset(stats, 0, sizeof(stats));
	memset(statsPercent, 0, sizeof(statsPercent));
}

void ConditionAttributes::addCondition(Creature* creature, const Condition* addCondition)
{
	if(updateCondition(addCondition))
	{
		setTicks(addCondition->getTicks());
		const ConditionAttributes& conditionAttrs = static_cast<const ConditionAttributes&>(*addCondition);
		endCondition(creature, CONDITIONEND_ABORT);

		//Apply the new one
		memcpy(skills, conditionAttrs.skills, sizeof(skills));
		memcpy(stats, conditionAttrs.stats, sizeof(stats));
		memcpy(statsPercent, conditionAttrs.statsPercent, sizeof(statsPercent));

		if(Player* player = creature->getPlayer())
		{
			updateSkills(player);
			updatePercentStats(player);
			updateStats(player);
		}
	}
}

xmlNodePtr ConditionAttributes::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	char buffer[20], buffer2[20];
	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
	{
		sprintf(buffer, "%d", skills[i]);
		sprintf(buffer2, "skill%d", i);
		xmlSetProp(nodeCondition, (const xmlChar*)buffer2, (const xmlChar*)buffer);
	}

	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i)
	{
		sprintf(buffer, "%d", stats[i]);
		sprintf(buffer2, "stat%d", i);
		xmlSetProp(nodeCondition, (const xmlChar*)buffer2, (const xmlChar*)buffer);
	}

	return nodeCondition;
}

bool ConditionAttributes::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p))
		return false;

	char buffer[20];
	int32_t intValue;

	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
	{
		sprintf(buffer, "skill%d", i);
		if(readXMLInteger(p, buffer, intValue))
			skills[i] = intValue;
	}

	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i)
	{
		sprintf(buffer, "stat%d", i);
		if(readXMLInteger(p, buffer, intValue))
			stats[i] = intValue;
	}

	return true;
}

bool ConditionAttributes::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case CONDITIONATTR_SKILLS:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			skills[currentSkill] = value;
			++currentSkill;
			return true;
		}

		case CONDITIONATTR_STATS:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			stats[currentStat] = value;
			++currentStat;
			return true;
		}

		default:
			break;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionAttributes::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream))
		return false;

	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
	{
		propWriteStream.ADD_UCHAR(CONDITIONATTR_SKILLS);
		propWriteStream.ADD_VALUE(skills[i]);
	}

	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i)
	{
		propWriteStream.ADD_UCHAR(CONDITIONATTR_STATS);
		propWriteStream.ADD_VALUE(stats[i]);
	}

	return true;
}

bool ConditionAttributes::startCondition(Creature* creature)
{
	if(Player* player = creature->getPlayer())
	{
		updateSkills(player);
		updatePercentStats(player);
		updateStats(player);
	}

	return Condition::startCondition(creature);
}

void ConditionAttributes::updatePercentStats(Player* player)
{
	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i)
	{
		if(statsPercent[i] == 0)
			continue;

		switch(i)
		{
			case STAT_MAXHEALTH:
				stats[i] = (int32_t)(player->getMaxHealth() * ((statsPercent[i] - 100) / 100.f));
				break;

			case STAT_MAXMANA:
				stats[i] = (int32_t)(player->getMaxMana() * ((statsPercent[i] - 100) / 100.f));
				break;

			case STAT_SOUL:
				stats[i] = (int32_t)(player->getSoul() * ((statsPercent[i] - 100) / 100.f));
				break;

			case STAT_MAGICLEVEL:
				stats[i] = (int32_t)(player->getMagicLevel() * ((statsPercent[i] - 100) / 100.f));
				break;
		}
	}
}

void ConditionAttributes::updateSkills(Player* player)
{
	bool needUpdateSkills = false;
	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
	{
		if(skills[i])
		{
			needUpdateSkills = true;
			player->setVarSkill((skills_t)i, skills[i]);
		}
	}

	if(needUpdateSkills)
		player->sendSkills();
}

void ConditionAttributes::updateStats(Player* player)
{
	bool needUpdateStats = false;
	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i)
	{
		if(stats[i])
		{
			needUpdateStats = true;
			player->setVarStats((stats_t)i, stats[i]);
		}
	}

	if(needUpdateStats)
		player->sendStats();
}

bool ConditionAttributes::executeCondition(Creature* creature, int32_t interval)
{
	return ConditionGeneric::executeCondition(creature, interval);
}

void ConditionAttributes::endCondition(Creature* creature, ConditionEnd_t reason)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		bool needUpdateSkills = false;
		for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
		{
			if(skills[i])
			{
				needUpdateSkills = true;
				player->setVarSkill((skills_t)i, -skills[i]);
			}
		}

		if(needUpdateSkills)
			player->sendSkills();

		bool needUpdateStats = false;
		for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i)
		{
			if(stats[i])
			{
				needUpdateStats = true;
				player->setVarStats((stats_t)i, -stats[i]);
			}
		}

		if(needUpdateStats)
			player->sendStats();
	}
}

bool ConditionAttributes::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);
	switch(param)
	{
		case CONDITIONPARAM_SKILL_MELEE:
		{
			skills[SKILL_CLUB] = value;
			skills[SKILL_AXE] = value;
			skills[SKILL_SWORD] = value;
			return true;
		}

		case CONDITIONPARAM_SKILL_FIST:
			skills[SKILL_FIST] = value;
			return true;

		case CONDITIONPARAM_SKILL_CLUB:
			skills[SKILL_CLUB] = value;
			return true;

		case CONDITIONPARAM_SKILL_SWORD:
			skills[SKILL_SWORD] = value;
			return true;

		case CONDITIONPARAM_SKILL_AXE:
			skills[SKILL_AXE] = value;
			return true;

		case CONDITIONPARAM_SKILL_DISTANCE:
			skills[SKILL_DIST] = value;
			return true;

		case CONDITIONPARAM_SKILL_SHIELD:
			skills[SKILL_SHIELD] = value;
			return true;

		case CONDITIONPARAM_SKILL_FISHING:
			skills[SKILL_FISH] = value;
			return true;

		case CONDITIONPARAM_STAT_MAXHEALTH:
			stats[STAT_MAXHEALTH] = value;
			return true;

		case CONDITIONPARAM_STAT_MAXMANA:
			stats[STAT_MAXMANA] = value;
			return true;

		case CONDITIONPARAM_STAT_SOUL:
			stats[STAT_SOUL] = value;
			return true;

		case CONDITIONPARAM_STAT_MAGICLEVEL:
			stats[STAT_MAGICLEVEL] = value;
			return true;

		case CONDITIONPARAM_STAT_MAXHEALTHPERCENT:
			statsPercent[STAT_MAXHEALTH] = std::max((int32_t)0, value);
			return true;

		case CONDITIONPARAM_STAT_MAXMANAPERCENT:
			statsPercent[STAT_MAXMANA] = std::max((int32_t)0, value);
			return true;

		case CONDITIONPARAM_STAT_SOULPERCENT:
			statsPercent[STAT_SOUL] = std::max((int32_t)0, value);
			return true;

		case CONDITIONPARAM_STAT_MAGICLEVELPERCENT:
			statsPercent[STAT_MAGICLEVEL] = std::max((int32_t)0, value);
			return true;

		default:
			break;
	}

	return ret;
}

ConditionRegeneration::ConditionRegeneration(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
ConditionGeneric(_id, _type, _ticks)
{
	internalHealthTicks = internalManaTicks = healthGain = manaGain = 0;
	healthTicks = manaTicks = 1000;
}

void ConditionRegeneration::addCondition(Creature* creature, const Condition* addCondition)
{
	if(updateCondition(addCondition))
	{
		setTicks(addCondition->getTicks());
		const ConditionRegeneration& conditionRegen = static_cast<const ConditionRegeneration&>(*addCondition);

		healthTicks = conditionRegen.healthTicks;
		manaTicks = conditionRegen.manaTicks;

		healthGain = conditionRegen.healthGain;
		manaGain = conditionRegen.manaGain;
	}
}

xmlNodePtr ConditionRegeneration::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	char buffer[20];
	sprintf(buffer, "%d", healthTicks);
	xmlSetProp(nodeCondition, (const xmlChar*)"healthticks", (const xmlChar*)buffer);
	sprintf(buffer, "%d", healthGain);
	xmlSetProp(nodeCondition, (const xmlChar*)"healthgain", (const xmlChar*)buffer);
	sprintf(buffer, "%d", manaTicks);
	xmlSetProp(nodeCondition, (const xmlChar*)"manaticks", (const xmlChar*)buffer);
	sprintf(buffer, "%d", manaGain);
	xmlSetProp(nodeCondition, (const xmlChar*)"managain", (const xmlChar*)buffer);
	return nodeCondition;
}

bool ConditionRegeneration::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p))
		return false;

	int32_t intValue;

	if(readXMLInteger(p, "healthticks", intValue))
		healthTicks = intValue;

	if(readXMLInteger(p, "healthgain", intValue))
		healthGain = intValue;

	if(readXMLInteger(p, "manaticks", intValue))
		manaTicks = intValue;

	if(readXMLInteger(p, "managain", intValue))
		manaGain = intValue;

	return true;
}

bool ConditionRegeneration::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case CONDITIONATTR_HEALTHTICKS:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			healthTicks = value;
			return true;
		}

		case CONDITIONATTR_HEALTHGAIN:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			healthGain = value;
			return true;
		}

		case CONDITIONATTR_MANATICKS:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			manaTicks = value;
			return true;
		}

		case CONDITIONATTR_MANAGAIN:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			manaGain = value;
			return true;
		}

		default:
			break;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionRegeneration::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream))
		return false;

	propWriteStream.ADD_UCHAR(CONDITIONATTR_HEALTHTICKS);
	propWriteStream.ADD_VALUE(healthTicks);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_HEALTHGAIN);
	propWriteStream.ADD_VALUE(healthGain);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_MANATICKS);
	propWriteStream.ADD_VALUE(manaTicks);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_MANAGAIN);
	propWriteStream.ADD_VALUE(manaGain);
	return true;
}

bool ConditionRegeneration::executeCondition(Creature* creature, int32_t interval)
{
	internalHealthTicks += interval;
	internalManaTicks += interval;
	if(creature->getZone() != ZONE_PROTECTION)
	{
		if(internalHealthTicks >= healthTicks)
		{
			internalHealthTicks = 0;
			creature->changeHealth(healthGain);
		}

		if(internalManaTicks >= manaTicks)
		{
			internalManaTicks = 0;
			creature->changeMana(manaGain);
		}
	}

	return ConditionGeneric::executeCondition(creature, interval);
}

bool ConditionRegeneration::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);
	switch(param)
	{
		case CONDITIONPARAM_HEALTHGAIN:
			healthGain = value;
			return true;

		case CONDITIONPARAM_HEALTHTICKS:
			healthTicks = value;
			return true;

		case CONDITIONPARAM_MANAGAIN:
			manaGain = value;
			return true;

		case CONDITIONPARAM_MANATICKS:
			manaTicks = value;
			return true;

		default:
			break;
	}

	return ret;
}

ConditionSoul::ConditionSoul(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
ConditionGeneric(_id, _type, _ticks)
{
	internalSoulTicks = soulTicks = soulGain = 0;
}

void ConditionSoul::addCondition(Creature* creature, const Condition* addCondition)
{
	if(updateCondition(addCondition))
	{
		setTicks(addCondition->getTicks());
		const ConditionSoul& conditionSoul = static_cast<const ConditionSoul&>(*addCondition);

		soulTicks = conditionSoul.soulTicks;
		soulGain = conditionSoul.soulGain;
	}
}

xmlNodePtr ConditionSoul::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	char buffer[20];
	sprintf(buffer, "%d", soulGain);
	xmlSetProp(nodeCondition, (const xmlChar*)"soulgain", (const xmlChar*)buffer);
	sprintf(buffer, "%d", soulTicks);
	xmlSetProp(nodeCondition, (const xmlChar*)"soulticks", (const xmlChar*)buffer);
	return nodeCondition;
}

bool ConditionSoul::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p))
		return false;

	int32_t intValue;

	if(readXMLInteger(p, "soulgain", intValue))
		soulGain = intValue;

	if(readXMLInteger(p, "soulticks", intValue))
		soulTicks = intValue;

	return true;
}

bool ConditionSoul::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case CONDITIONATTR_SOULGAIN:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			soulGain = value;
			return true;
		}

		case CONDITIONATTR_SOULTICKS:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			soulTicks = value;
			return true;
		}

		default:
			break;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionSoul::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream))
		return false;

	propWriteStream.ADD_UCHAR(CONDITIONATTR_SOULGAIN);
	propWriteStream.ADD_VALUE(soulGain);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_SOULTICKS);
	propWriteStream.ADD_VALUE(soulTicks);

	return true;
}

bool ConditionSoul::executeCondition(Creature* creature, int32_t interval)
{
	internalSoulTicks += interval;
	if(Player* player = creature->getPlayer())
	{
		if(player->getZone() != ZONE_PROTECTION)
		{
			if(internalSoulTicks >= soulTicks)
			{
				internalSoulTicks = 0;
				player->changeSoul(soulGain);
			}
		}
	}

	return ConditionGeneric::executeCondition(creature, interval);
}

bool ConditionSoul::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);
	switch(param)
	{
		case CONDITIONPARAM_SOULGAIN:
			soulGain = value;
			return true;

		case CONDITIONPARAM_SOULTICKS:
			soulTicks = value;
			return true;

		default:
			break;
	}

	return ret;
}

ConditionDamage::ConditionDamage(ConditionId_t _id, ConditionType_t _type) :
Condition(_id, _type, 0)
{
	tickInterval = 2000;
	delayed = forceUpdate = false;
	owner = minDamage = maxDamage = startDamage = periodDamage = periodDamageTick = 0;
}

bool ConditionDamage::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);
	switch(param)
	{
		case CONDITIONPARAM_OWNER:
			owner = value;
			return true;

		case CONDITIONPARAM_FORCEUPDATE:
			forceUpdate = (value != 0);
			return true;

		case CONDITIONPARAM_DELAYED:
			delayed = (value != 0);
			return true;

		case CONDITIONPARAM_MAXVALUE:
			maxDamage = std::abs(value);
			break;

		case CONDITIONPARAM_MINVALUE:
			minDamage = std::abs(value);
			break;

		case CONDITIONPARAM_STARTVALUE:
			startDamage = std::abs(value);
			break;

		case CONDITIONPARAM_TICKINTERVAL:
			tickInterval = std::abs(value);
			break;

		case CONDITIONPARAM_PERIODICDAMAGE:
			periodDamage = value;
			break;

		default:
			break;
	}

	return ret;
}

xmlNodePtr ConditionDamage::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	char buffer[20];
	sprintf(buffer, "%d", delayed);
	xmlSetProp(nodeCondition, (const xmlChar*)"delayed", (const xmlChar*)buffer);
	sprintf(buffer, "%d", periodDamage);
	xmlSetProp(nodeCondition, (const xmlChar*)"perioddamage", (const xmlChar*)buffer);
	sprintf(buffer, "%d", owner);
	xmlSetProp(nodeCondition, (const xmlChar*)"owner", (const xmlChar*)buffer);
	for(DamageList::const_iterator it = damageList.begin(); it != damageList.end(); ++it)
	{
		xmlNodePtr nodeValueListNode = xmlNewNode(NULL, (const xmlChar*)"damage");
		sprintf(buffer, "%d", (*it).timeLeft);
		xmlSetProp(nodeValueListNode, (const xmlChar*)"duration", (const xmlChar*)buffer);
		sprintf(buffer, "%d", (*it).value);
		xmlSetProp(nodeValueListNode, (const xmlChar*)"value", (const xmlChar*)buffer);
		sprintf(buffer, "%d", (*it).interval);
		xmlSetProp(nodeValueListNode, (const xmlChar*)"interval", (const xmlChar*)buffer);
		xmlAddChild(nodeCondition, nodeValueListNode);
	}

	return nodeCondition;
}

bool ConditionDamage::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p))
		return false;

	setTicks(0);

	int32_t intValue;

	if(readXMLInteger(p, "delayed", intValue))
		delayed = (intValue == 1);

	if(readXMLInteger(p, "perioddamage", intValue))
		periodDamage = intValue;

	if(readXMLInteger(p, "owner", intValue))
		owner = intValue;

	xmlNodePtr nodeList = p->children;
	while(nodeList)
	{
		if(xmlStrcmp(nodeList->name, (const xmlChar*)"damage") == 0)
		{
			IntervalInfo damageInfo;
			damageInfo.interval = 0;
			damageInfo.timeLeft = 0;
			damageInfo.value = 0;

			if(readXMLInteger(nodeList, "duration", intValue))
				damageInfo.timeLeft = intValue;

			if(readXMLInteger(nodeList, "value", intValue))
				damageInfo.value = intValue;

			if(readXMLInteger(nodeList, "interval", intValue))
				damageInfo.interval = intValue;

			damageList.push_back(damageInfo);
			if(getTicks() != -1)
				setTicks(getTicks() + damageInfo.interval);
		}

		nodeList = nodeList->next;
	}

	return true;
}

bool ConditionDamage::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case CONDITIONATTR_DELAYED:
		{
			bool value = false;
				if(!propStream.GET_VALUE(value))
				return false;

			delayed = value;
			return true;
		}

		case CONDITIONATTR_PERIODDAMAGE:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			periodDamage = value;
			return true;
		}

		case CONDITIONATTR_OWNER:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			owner = value;
			return true;
		}

		case CONDITIONATTR_INTERVALDATA:
		{
			IntervalInfo damageInfo;
			if(!propStream.GET_VALUE(damageInfo))
				return false;

			damageList.push_back(damageInfo);
			if(getTicks() != -1)
				setTicks(getTicks() + damageInfo.interval);

			return true;
		}

		default:
			break;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionDamage::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream))
		return false;

	propWriteStream.ADD_UCHAR(CONDITIONATTR_DELAYED);
	propWriteStream.ADD_VALUE(delayed);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_PERIODDAMAGE);
	propWriteStream.ADD_VALUE(periodDamage);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_OWNER);
	propWriteStream.ADD_VALUE(owner);

	for(DamageList::const_iterator it = damageList.begin(); it != damageList.end(); ++it)
	{
		propWriteStream.ADD_UCHAR(CONDITIONATTR_INTERVALDATA);
		propWriteStream.ADD_VALUE((*it));
	}

	return true;
}

bool ConditionDamage::updateCondition(const ConditionDamage* addCondition)
{
	if(addCondition->doForceUpdate())
		return true;

	if(getTicks() == -1 && addCondition->getTicks() > 0)
		return false;

	if(addCondition->getTicks() <= getTicks())
		return false;

	if(addCondition->getTotalDamage() < getTotalDamage())
		return false;

	if(addCondition->periodDamage < periodDamage)
		return false;

	return true;
}

bool ConditionDamage::addDamage(int32_t rounds, int32_t time, int32_t value)
{
	if(rounds == -1)
	{
		//periodic damage
		periodDamage = value;
		setParam(CONDITIONPARAM_TICKINTERVAL, time);
		setParam(CONDITIONPARAM_TICKS, -1);
		return true;
	}

	if(periodDamage > 0)
		return false;

	//rounds, time, damage
	for(int32_t i = 0; i < rounds; ++i)
	{
		IntervalInfo damageInfo;
		damageInfo.interval = time;
		damageInfo.timeLeft = time;
		damageInfo.value = value;

		damageList.push_back(damageInfo);
		if(getTicks() != -1)
			setTicks(getTicks() + damageInfo.interval);
	}

	return true;
}

bool ConditionDamage::init()
{
	if(periodDamage != 0)
		return true;

	if(damageList.empty())
	{
		setTicks(0);

		int32_t amount = random_range(minDamage, maxDamage);
		if(amount != 0)
		{
			if(startDamage > maxDamage)
				startDamage = maxDamage;
			else if(startDamage == 0)
				startDamage = std::max((int32_t)1, (int32_t)std::ceil(((float)amount / 20.0)));

			std::list<int32_t> list;
			ConditionDamage::generateDamageList(amount, startDamage, list);

			for(std::list<int32_t>::iterator it = list.begin(); it != list.end(); ++it)
				addDamage(1, tickInterval, -*it);
		}
	}

	return (!damageList.empty());
}

bool ConditionDamage::startCondition(Creature* creature)
{
	if(Condition::startCondition(creature) && init() && !delayed)
	{
		int32_t damage = 0;
		if(getNextDamage(damage))
			return doDamage(creature, damage);
	}

	return false;
}

bool ConditionDamage::executeCondition(Creature* creature, int32_t interval)
{
	if(periodDamage != 0)
	{
		periodDamageTick += interval;
		if(periodDamageTick >= tickInterval)
		{
			periodDamageTick = 0;
			doDamage(creature, periodDamage);
		}
	}
	else if(!damageList.empty())
	{
		IntervalInfo& damageInfo = damageList.front();

		bool remove = (getTicks() != -1);
		creature->onTickCondition(getType(), remove);
		damageInfo.timeLeft -= interval;

		if(damageInfo.timeLeft <= 0)
		{
			int32_t damage = damageInfo.value;

			if(remove)
				damageList.pop_front();
			else
				damageInfo.timeLeft = damageInfo.interval;

			doDamage(creature, damage);
		}

		if(!remove)
			interval = 0;
	}

	return Condition::executeCondition(creature, interval);
}

bool ConditionDamage::getNextDamage(int32_t& damage)
{
	if(periodDamage != 0)
	{
		damage = periodDamage;
		return true;
	}
	else if(!damageList.empty())
	{
		IntervalInfo& damageInfo = damageList.front();

		damage = damageInfo.value;
		if(getTicks() != -1)
			damageList.pop_front();

		return true;
	}
	return false;
}

bool ConditionDamage::doDamage(Creature* creature, int32_t damage)
{
	if(creature->isSuppress(getType()))
		return true;

	CombatType_t combatType = Combat::ConditionToDamageType(conditionType);
	Creature* attacker = g_game.getCreatureByID(owner);

	if(g_game.combatBlockHit(combatType, attacker, creature, damage, false, false))
		return false;

	return g_game.combatChangeHealth(combatType, attacker, creature, damage);
}

void ConditionDamage::endCondition(Creature* creature, ConditionEnd_t reason)
{
	//
}

void ConditionDamage::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType)
	{
		const ConditionDamage& conditionDamage = static_cast<const ConditionDamage&>(*addCondition);
		if(updateCondition(&conditionDamage))
		{
			setTicks(addCondition->getTicks());
			owner = conditionDamage.owner;
			maxDamage = conditionDamage.maxDamage;
			minDamage = conditionDamage.minDamage;
			startDamage = conditionDamage.startDamage;
			tickInterval = conditionDamage.tickInterval;
			periodDamage = conditionDamage.periodDamage;
			int32_t nextTimeLeft = tickInterval;

			if(!damageList.empty())
			{
				//save previous timeLeft
				IntervalInfo& damageInfo = damageList.front();
				nextTimeLeft = damageInfo.timeLeft;
				damageList.clear();
			}

			damageList = conditionDamage.damageList;
			if(init())
			{
				if(!damageList.empty())
				{
					//restore last timeLeft
					IntervalInfo& damageInfo = damageList.front();
					damageInfo.timeLeft = nextTimeLeft;
				}

				if(!delayed)
				{
					int32_t damage = 0;
					if(getNextDamage(damage))
						doDamage(creature, damage);
				}
			}
		}
	}
}

int32_t ConditionDamage::getTotalDamage() const
{
	int32_t result = 0;

	if(!damageList.empty())
	{
		for(DamageList::const_iterator it = damageList.begin(); it != damageList.end(); ++it)
			result += it->value;
	}
	else
		result = maxDamage + (maxDamage - minDamage) / 2;

	return std::abs(result);
}

uint32_t ConditionDamage::getIcons() const
{
	switch(conditionType)
	{
		case CONDITION_FIRE:
			return ICON_BURN;

		case CONDITION_ENERGY:
			return ICON_ENERGY;

		case CONDITION_DROWN:
			return ICON_DROWNING;

		case CONDITION_POISON:
			return ICON_POISON;

		case CONDITION_FREEZING:
			return ICON_FREEZING;

		case CONDITION_DAZZLED:
			return ICON_DAZZLED;

		case CONDITION_CURSED:
			return ICON_CURSED;

		default:
			break;
	}
	return 0;
}

void ConditionDamage::generateDamageList(int32_t amount, int32_t start, std::list<int32_t>& list)
{
	amount = std::abs(amount);
	int32_t sum = 0, med = 0;
	float x1, x2;

	for(int32_t i = start; i > 0; --i)
	{
		int32_t n = start + 1 - i;
		med = (n * amount) / start;
		do
		{
			sum += i;
			list.push_back(i);

			x1 = std::fabs(1.0 - (((float)sum) + i) / med);
			x2 = std::fabs(1.0 - (((float)sum) / med));
		}
		while(x1 < x2);
	}
}

ConditionSpeed::ConditionSpeed(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, int32_t changeSpeed) :
Condition(_id, _type, _ticks)
{
	speedDelta = changeSpeed;
	mina = minb = maxa = maxb = 0.0f;
}

void ConditionSpeed::setFormulaVars(float _mina, float _minb, float _maxa, float _maxb)
{
	mina = _mina;
	minb = _minb;
	maxa = _maxa;
	maxb = _maxb;
}

void ConditionSpeed::getFormulaValues(int32_t var, int32_t& min, int32_t& max) const
{
	min = (int32_t)std::ceil(var * 1.f * mina + minb);
	max = (int32_t)std::ceil(var * 1.f * maxa + maxb);
}

bool ConditionSpeed::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);
	if(param == CONDITIONPARAM_SPEED)
	{
		speedDelta = value;
		if(value > 0)
			conditionType = CONDITION_HASTE;
		else
			conditionType = CONDITION_PARALYZE;

		return true;
	}
	else
		return false;

	return ret;
}

xmlNodePtr ConditionSpeed::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	char buffer[20];
	sprintf(buffer, "%d", speedDelta);
	xmlSetProp(nodeCondition, (const xmlChar*)"delta", (const xmlChar*)buffer);
	sprintf(buffer, "%f", mina);
	xmlSetProp(nodeCondition, (const xmlChar*)"mina", (const xmlChar*)buffer);
	sprintf(buffer, "%f", minb);
	xmlSetProp(nodeCondition, (const xmlChar*)"minb", (const xmlChar*)buffer);
	sprintf(buffer, "%f", maxa);
	xmlSetProp(nodeCondition, (const xmlChar*)"maxa", (const xmlChar*)buffer);
	sprintf(buffer, "%f", maxb);
	xmlSetProp(nodeCondition, (const xmlChar*)"maxb", (const xmlChar*)buffer);
	return nodeCondition;
}

bool ConditionSpeed::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p))
		return false;

	int32_t intValue;

	if(readXMLInteger(p, "delta", intValue))
		speedDelta = intValue;

	float floatValue;

	if(readXMLFloat(p, "mina", floatValue))
		mina = floatValue;

	if(readXMLFloat(p, "minb", floatValue))
		minb = floatValue;

	if(readXMLFloat(p, "maxa", floatValue))
		maxa = floatValue;

	if(readXMLFloat(p, "maxb", floatValue))
		maxb = floatValue;

	return true;
}

bool ConditionSpeed::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case CONDITIONATTR_SPEEDDELTA:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			speedDelta = value;
			return true;
		}

		case CONDITIONATTR_FORMULA_MINA:
		{
			float value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			mina = value;
			return true;
		}

		case CONDITIONATTR_FORMULA_MINB:
		{
			float value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			minb = value;
			return true;
		}

		case CONDITIONATTR_FORMULA_MAXA:
		{
			float value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			maxa = value;
			return true;
		}

		case CONDITIONATTR_FORMULA_MAXB:
		{
			float value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			maxb = value;
			return true;
		}

		default:
			break;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionSpeed::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream))
		return false;

	propWriteStream.ADD_UCHAR(CONDITIONATTR_SPEEDDELTA);
	propWriteStream.ADD_VALUE(speedDelta);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_FORMULA_MINA);
	propWriteStream.ADD_VALUE(mina);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_FORMULA_MINB);
	propWriteStream.ADD_VALUE(minb);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_FORMULA_MAXA);
	propWriteStream.ADD_VALUE(maxa);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_FORMULA_MAXB);
	propWriteStream.ADD_VALUE(maxb);
	return true;
}

bool ConditionSpeed::startCondition(Creature* creature)
{
	if(speedDelta == 0)
	{
		int32_t min, max;
		getFormulaValues(creature->getBaseSpeed(), min, max);
		speedDelta = random_range(min, max);
	}

	g_game.changeSpeed(creature, speedDelta);
	return Condition::startCondition(creature);
}

bool ConditionSpeed::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
}

void ConditionSpeed::endCondition(Creature* creature, ConditionEnd_t reason)
{
	g_game.changeSpeed(creature, -speedDelta);
}

void ConditionSpeed::addCondition(Creature* creature, const Condition* addCondition)
{
	if(updateCondition(addCondition))
	{
		setTicks(addCondition->getTicks());

		const ConditionSpeed& conditionSpeed = static_cast<const ConditionSpeed&>(*addCondition);
		int32_t oldSpeedDelta = speedDelta;
		speedDelta = conditionSpeed.speedDelta;
		mina = conditionSpeed.mina;
		maxa = conditionSpeed.maxa;
		minb = conditionSpeed.minb;
		maxb = conditionSpeed.maxb;

		if(speedDelta == 0)
		{
			int32_t min;
			int32_t max;
			getFormulaValues(creature->getBaseSpeed(), min, max);
			speedDelta = random_range(min, max);
		}

		int32_t newSpeedChange = (speedDelta - oldSpeedDelta);
		if(newSpeedChange != 0)
			g_game.changeSpeed(creature, newSpeedChange);
	}
}

uint32_t ConditionSpeed::getIcons() const
{
	switch(conditionType)
	{
		case CONDITION_HASTE:
			return ICON_HASTE;

		case CONDITION_PARALYZE:
			return ICON_PARALYZE;

		default:
			break;
	}

	return 0;
}

ConditionInvisible::ConditionInvisible(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
ConditionGeneric(_id, _type, _ticks)
{
	//
}

bool ConditionInvisible::startCondition(Creature* creature)
{
	g_game.internalCreatureChangeVisible(creature, false);
	return Condition::startCondition(creature);
}

void ConditionInvisible::endCondition(Creature* creature, ConditionEnd_t reason)
{
	if(!creature->isInvisible())
		g_game.internalCreatureChangeVisible(creature, true);
}

ConditionOutfit::ConditionOutfit(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
Condition(_id, _type, _ticks)
{
	//
}

void ConditionOutfit::addOutfit(Outfit_t outfit)
{
	outfits.push_back(outfit);
}

xmlNodePtr ConditionOutfit::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();
	char buffer[20];
	for(std::vector<Outfit_t>::const_iterator it = outfits.begin(); it != outfits.end(); ++it)
	{
		xmlNodePtr nodeValueListNode = xmlNewNode(NULL, (const xmlChar*)"outfit");
		sprintf(buffer, "%d", (*it).lookType);
		xmlSetProp(nodeValueListNode, (const xmlChar*)"looktype", (const xmlChar*)buffer);
		sprintf(buffer, "%d", (*it).lookHead);
		xmlSetProp(nodeValueListNode, (const xmlChar*)"lookhead", (const xmlChar*)buffer);
		sprintf(buffer, "%d", (*it).lookBody);
		xmlSetProp(nodeValueListNode, (const xmlChar*)"lookbody", (const xmlChar*)buffer);
		sprintf(buffer, "%d", (*it).lookLegs);
		xmlSetProp(nodeValueListNode, (const xmlChar*)"looklegs", (const xmlChar*)buffer);
		sprintf(buffer, "%d", (*it).lookFeet);
		xmlSetProp(nodeValueListNode, (const xmlChar*)"lookfeet", (const xmlChar*)buffer);
		sprintf(buffer, "%d", (*it).lookAddons);
		xmlSetProp(nodeValueListNode, (const xmlChar*)"lookaddons", (const xmlChar*)buffer);
		xmlAddChild(nodeCondition, nodeValueListNode);
	}

	return nodeCondition;
}

bool ConditionOutfit::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p))
		return false;

	xmlNodePtr nodeList = p->children;
	while(nodeList)
	{
		if(xmlStrcmp(nodeList->name, (const xmlChar*)"outfit") == 0)
		{
			Outfit_t outfit;
			int32_t intValue;

			if(readXMLInteger(nodeList, "looktype", intValue))
				outfit.lookType = intValue;

			if(readXMLInteger(nodeList, "lookhead", intValue))
				outfit.lookHead = intValue;

			if(readXMLInteger(nodeList, "lookbody", intValue))
				outfit.lookBody = intValue;

			if(readXMLInteger(nodeList, "looklegs", intValue))
				outfit.lookLegs = intValue;

			if(readXMLInteger(nodeList, "lookfeet", intValue))
				outfit.lookFeet = intValue;

			if(readXMLInteger(nodeList, "lookaddons", intValue))
				outfit.lookAddons = intValue;

			outfits.push_back(outfit);
		}
		nodeList = nodeList->next;
	}
	return true;
}

bool ConditionOutfit::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	if(attr == CONDITIONATTR_OUTFIT)
	{
		Outfit_t outfit;
		if(!propStream.GET_VALUE(outfit))
			return false;

		outfits.push_back(outfit);
		return true;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionOutfit::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream))
		return false;

	for(std::vector<Outfit_t>::const_iterator it = outfits.begin(); it != outfits.end(); ++it)
	{
		propWriteStream.ADD_UCHAR(CONDITIONATTR_OUTFIT);
		propWriteStream.ADD_VALUE(*it);
	}

	return true;
}

bool ConditionOutfit::startCondition(Creature* creature)
{
	changeOutfit(creature);
	return Condition::startCondition(creature);
}

bool ConditionOutfit::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
}

void ConditionOutfit::changeOutfit(Creature* creature, int32_t index /*= -1*/)
{
	if(!outfits.empty())
	{
		if(index == -1)
			index = random_range(0, outfits.size() - 1);

		Outfit_t outfit = outfits[index];
		g_game.internalCreatureChangeOutfit(creature, outfit);
	}
}

void ConditionOutfit::endCondition(Creature* creature, ConditionEnd_t reason)
{
	g_game.internalCreatureChangeOutfit(creature, creature->getDefaultOutfit());
}

void ConditionOutfit::addCondition(Creature* creature, const Condition* addCondition)
{
	if(updateCondition(addCondition))
	{
		setTicks(addCondition->getTicks());

		const ConditionOutfit& conditionOutfit = static_cast<const ConditionOutfit&>(*addCondition);
		outfits = conditionOutfit.outfits;

		changeOutfit(creature);
	}
}

uint32_t ConditionOutfit::getIcons() const
{
	return 0;
}

ConditionLight::ConditionLight(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, int32_t _lightlevel, int32_t _lightcolor) :
Condition(_id, _type, _ticks)
{
	lightInfo.level = _lightlevel;
	lightInfo.color = _lightcolor;
	internalLightTicks = 0;
	lightChangeInterval = 0;
}

bool ConditionLight::startCondition(Creature* creature)
{
	internalLightTicks = 0;
	lightChangeInterval = ticks / lightInfo.level;
	creature->setCreatureLight(lightInfo);
	g_game.changeLight(creature);
	return Condition::startCondition(creature);
}

bool ConditionLight::executeCondition(Creature* creature, int32_t interval)
{
	internalLightTicks += interval;
	if(internalLightTicks >= lightChangeInterval)
	{
		internalLightTicks = 0;
		LightInfo creatureLight;
		creature->getCreatureLight(creatureLight);
		if(creatureLight.level > 0)
		{
			--creatureLight.level;
			creature->setCreatureLight(creatureLight);
			g_game.changeLight(creature);
		}
	}
	return Condition::executeCondition(creature, interval);
}

void ConditionLight::endCondition(Creature* creature, ConditionEnd_t reason)
{
	creature->setNormalCreatureLight();
	g_game.changeLight(creature);
}

void ConditionLight::addCondition(Creature* creature, const Condition* addCondition)
{
	if(updateCondition(addCondition))
	{
		setTicks(addCondition->getTicks());

		const ConditionLight& conditionLight = static_cast<const ConditionLight&>(*addCondition);
		lightInfo.level = conditionLight.lightInfo.level;
		lightInfo.color = conditionLight.lightInfo.color;
		lightChangeInterval = getTicks() / lightInfo.level;
		internalLightTicks = 0;
		creature->setCreatureLight(lightInfo);
		g_game.changeLight(creature);
	}
}

bool ConditionLight::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);
	if(!ret)
	{
		switch(param)
		{
			case CONDITIONPARAM_LIGHT_LEVEL:
				lightInfo.level = value;
				return true;

			case CONDITIONPARAM_LIGHT_COLOR:
				lightInfo.color = value;
				return true;

			default:
				break;
		}
	}

	return false;
}

xmlNodePtr ConditionLight::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	char buffer[20];
	sprintf(buffer, "%d", lightInfo.color);
	xmlSetProp(nodeCondition, (const xmlChar*)"lightcolor", (const xmlChar*)buffer);
	sprintf(buffer, "%d", lightInfo.level);
	xmlSetProp(nodeCondition, (const xmlChar*)"lightlevel", (const xmlChar*)buffer);
	sprintf(buffer, "%d", internalLightTicks);
	xmlSetProp(nodeCondition, (const xmlChar*)"lightticks", (const xmlChar*)buffer);
	sprintf(buffer, "%d", lightChangeInterval);
	xmlSetProp(nodeCondition, (const xmlChar*)"lightinterval", (const xmlChar*)buffer);
	return nodeCondition;
}

bool ConditionLight::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p))
		return false;

	int32_t intValue;

	if(readXMLInteger(p, "lightcolor", intValue))
		lightInfo.color = intValue;

	if(readXMLInteger(p, "lightlevel", intValue))
		lightInfo.level = intValue;

	if(readXMLInteger(p, "lightticks", intValue))
		internalLightTicks = intValue;

	if(readXMLInteger(p, "lightinterval", intValue))
		lightChangeInterval = intValue;

	return true;
}

bool ConditionLight::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case CONDITIONATTR_LIGHTCOLOR:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			lightInfo.color = value;
			return true;
		}

		case CONDITIONATTR_LIGHTLEVEL:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			lightInfo.level = value;
			return true;
		}

		case CONDITIONATTR_LIGHTTICKS:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			internalLightTicks = value;
			return true;
		}

		case CONDITIONATTR_LIGHTINTERVAL:
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value))
				return false;

			lightChangeInterval = value;
			return true;
		}

		default:
			break;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionLight::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream))
		return false;

	propWriteStream.ADD_UCHAR(CONDITIONATTR_LIGHTCOLOR);
	propWriteStream.ADD_VALUE(lightInfo.color);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_LIGHTLEVEL);
	propWriteStream.ADD_VALUE(lightInfo.level);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_LIGHTTICKS);
	propWriteStream.ADD_VALUE(internalLightTicks);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_LIGHTINTERVAL);
	propWriteStream.ADD_VALUE(lightChangeInterval);

	return true;
}

uint32_t ConditionLight::getIcons() const
{
	return 0;
}
