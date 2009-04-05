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

#include <iostream>
#include <algorithm>

#include <stdlib.h>

#include "player.h"
#include "iologindata.h"
#include "chat.h"
#include "house.h"
#include "combat.h"
#include "movement.h"
#include "weapons.h"
#include "town.h"
#include "ioban.h"
#include "configmanager.h"
#include "creatureevent.h"
#include "status.h"
#include "beds.h"
#ifndef __CONSOLE__
#include "gui.h"
#endif

extern ConfigManager g_config;
extern Game g_game;
extern Chat g_chat;
extern Vocations g_vocations;
extern MoveEvents* g_moveEvents;
extern Weapons* g_weapons;
extern CreatureEvents* g_creatureEvents;

AutoList<Player> Player::listPlayer;
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t Player::playerCount = 0;
#endif
MuteCountMap Player::muteCountMap;
int32_t Player::maxMessageBuffer;

Player::Player(const std::string& _name, ProtocolGame *p) :
Creature()
{
	client = p;
	isConnecting = false;

	if(client)
		client->setPlayer(this);

	name = _name;
	nameDescription = _name;
	setVocation(0);
	promotionLevel = 0;
	capacity = 400.00;
	mana = 0;
	manaMax = 0;
	manaSpent = 0;
	soul = 0;
	soulMax = 100;
	guildId = 0;
	guildLevel = 0;

	level = 1;
	levelPercent = 0;
	magLevelPercent = 0;
	magLevel = 0;
	experience = 0;

	damageImmunities = 0;
	conditionImmunities = 0;
	conditionSuppressions = 0;
	accessLevel = 0;
	violationAccess = 0;
	groupName = "";
	groupId = 0;
	lastLogin = OTSYS_TIME();
	lastLoginSaved = 0;
	lastLogout = 0;
 	lastIP = 0;
	npings = 0;
	internalPing = 0;
	MessageBufferTicks = 0;
	MessageBufferCount = 0;
	nextAction = 0;

	windowTextId = 0;
	writeItem = NULL;
	maxWriteLen = 0;

	editHouse = NULL;
	editListId = 0;

	shopOwner = NULL;
	purchaseCallback = -1;
	saleCallback = -1;

	pzLocked = false;
	bloodHitCount = 0;
	shieldBlockCount = 0;
	lastAttackBlockType = BLOCK_NONE;
	addAttackSkillPoint = false;
	lastAttack = 0;
	shootRange = 1;

	blessings = 0;
	balance = 0;
	stamina = STAMINA_MAX;
	premiumDays = 0;

	idleTime = 0;
	marriage = 0;
	rates[SKILL__MAGLEVEL] = rates[SKILL__LEVEL] = 1.0f;

	chaseMode = CHASEMODE_STANDSTILL;
	fightMode = FIGHTMODE_ATTACK;

	tradePartner = NULL;
	tradeState = TRADE_NONE;
	tradeItem = NULL;

	walkTask = NULL;
	walkTaskEvent = 0;
	actionTaskEvent = 0;
	nextStepEvent = 0;

	for(int32_t i = 0; i < 11; i++)
	{
		inventory[i] = NULL;
		inventoryAbilities[i] = false;
	}

	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
	{
		skills[i][SKILL_LEVEL]= 10;
		skills[i][SKILL_TRIES]= 0;
		skills[i][SKILL_PERCENT] = 0;
		rates[i] = 1.0f;
	}

	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
		varSkills[i] = 0;

	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i)
		varStats[i] = 0;

	for(int32_t i = LOSS_FIRST; i <= LOSS_LAST; ++i)
		lossPercent[i] = 100;

	maxDepotLimit = 1000;
	maxVipLimit = 20;
	groupFlags = 0;
	groupCustomFlags = 0;

	accountManager = MANAGER_NONE;
	managerNumber2 = 0;
	managerString2 = "";
	for(int8_t i = 0; i <= 13; i++)
		talkState[i] = false;

	groupId = 0;
	groupOutfit = 0;
 	vocation_id = 0;
 	town = 0;

	redSkullTicks = 0;
	setParty(NULL);

	requestedOutfit = false;
	saving = true;
#ifdef __ENABLE_SERVER_DIAGNOSTIC__

	playerCount++;
#endif
}

Player::~Player()
{
	for(int32_t i = 0; i < 11; i++)
	{
		if(inventory[i])
		{
			inventory[i]->setParent(NULL);
			inventory[i]->releaseThing2();
			inventory[i] = NULL;
			inventoryAbilities[i] = false;
		}
	}

	for(DepotMap::iterator it = depots.begin(); it != depots.end(); it++)
		it->second.first->releaseThing2();

	setWriteItem(NULL);
	setEditHouse(NULL);
#ifdef __ENABLE_SERVER_DIAGNOSTIC__

	playerCount--;
#endif
}

void Player::setVocation(uint32_t vocId)
{
	vocation_id = vocId;
	vocation = g_vocations.getVocation(vocId);
	if(Condition* condition = getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT))
	{
		condition->setParam(CONDITIONPARAM_HEALTHGAIN, vocation->getHealthGainAmount());
		condition->setParam(CONDITIONPARAM_HEALTHTICKS, vocation->getHealthGainTicks() * 1000);
		condition->setParam(CONDITIONPARAM_MANAGAIN, vocation->getManaGainAmount());
		condition->setParam(CONDITIONPARAM_MANATICKS, vocation->getManaGainTicks() * 1000);
	}

	soulMax = vocation->getSoulMax();
}

bool Player::isPushable() const
{
	bool ret = Creature::isPushable();
	if(hasFlag(PlayerFlag_CannotBePushed))
		return false;

	return ret;
}

std::string Player::getDescription(int32_t lookDistance) const
{
	std::stringstream s;
	if(lookDistance == -1)
	{
		s << "yourself.";
		if(hasCustomFlag(PlayerCustomFlag_DescriptionGroupInsteadVocation))
			s << " You are " << groupName;
		else if(vocation_id != 0)
			s << " You are " << vocation->getVocDescription();
		else
			s << " You have no vocation";
	}
	else
	{
		s << nameDescription;
		if(!hasCustomFlag(PlayerCustomFlag_HideLevel))
			s << " (Level " << level << ")";

		s << ".";
		if(sex == PLAYERSEX_FEMALE)
			s << " She";
		else
			s << " He";

		if(hasCustomFlag(PlayerCustomFlag_DescriptionGroupInsteadVocation))
			s << " is " << groupName;
		else if(vocation_id != 0)
			s << " is " << vocation->getVocDescription();
		else
			s << " has no vocation";
	}

	std::string tmp;
	if(IOLoginData::getInstance()->getNameByGuid(marriage, tmp))
	{
		s << ", ";
		if(vocation_id == 0)
		{
			if(lookDistance == -1)
				s << "and you are";
			else
				s << "and is";

			s << " ";
		}

		if(sex == PLAYERSEX_FEMALE)
			s << "wife";
		else
			s << "husband";

		s << " of " << tmp;
	}

	s << ".";
	if(guildId)
	{
		if(lookDistance == -1)
			s << " You are ";
		else
		{
			if(sex == PLAYERSEX_FEMALE)
				s << " She is ";
			else
				s << " He is ";
		}

		if(guildRank.length())
			s << guildRank;
		else
			s << "a member";

		s << " of the " << guildName;
		if(guildNick.length())
			s << " (" << guildNick << ")";

		s << ".";
	}

	return s.str();
}

Item* Player::getInventoryItem(slots_t slot) const
{
	if(slot > SLOT_WHEREEVER && slot < SLOT_LAST)
		return inventory[slot];

	if(slot == SLOT_HAND)
		return inventory[SLOT_LEFT] ? inventory[SLOT_LEFT] : inventory[SLOT_RIGHT];

	return NULL;
}

Item* Player::getEquippedItem(slots_t slot) const
{
	Item* item = getInventoryItem(slot);
	if(!item)
		return NULL;

	switch(slot)
	{
		case SLOT_LEFT:
		case SLOT_RIGHT:
			return item->getWieldPosition() == SLOT_HAND ? item : NULL;

		default:
			break;
	}

	return slot == item->getWieldPosition() ? item : NULL;
}

void Player::setConditionSuppressions(uint32_t conditions, bool remove)
{
	if(!remove)
		conditionSuppressions |= conditions;
	else
		conditionSuppressions &= ~conditions;
}

Item* Player::getWeapon(bool ignoreAmmo /*= false*/)
{
	Item* item;
	for(uint32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
	{
		item = getEquippedItem((slots_t)slot);
		if(!item)
			continue;

		switch(item->getWeaponType())
		{
			case WEAPON_SWORD:
			case WEAPON_AXE:
			case WEAPON_CLUB:
			case WEAPON_WAND:
			case WEAPON_FIST:
			{
				const Weapon* weapon = g_weapons->getWeapon(item);
				if(weapon)
					return item;
				break;
			}

			case WEAPON_DIST:
			{
				if(!ignoreAmmo && item->getAmmoType() != AMMO_NONE)
				{
					Item* ammoItem = getInventoryItem(SLOT_AMMO);
					if(ammoItem && ammoItem->getAmmoType() == item->getAmmoType())
					{
						const Weapon* weapon = g_weapons->getWeapon(ammoItem);
						if(weapon)
						{
							shootRange = item->getShootRange();
							return ammoItem;
						}
					}
				}
				else
				{
					const Weapon* weapon = g_weapons->getWeapon(item);
					if(weapon)
					{
						shootRange = item->getShootRange();
						return item;
					}
				}
				break;
			}

			default:
				break;
		}
	}

	return NULL;
}

WeaponType_t Player::getWeaponType()
{
	if(Item* item = getWeapon())
		return item->getWeaponType();

	return WEAPON_NONE;
}

int32_t Player::getWeaponSkill(const Item* item) const
{
	if(!item)
		return getSkill(SKILL_FIST, SKILL_LEVEL);

	switch(item->getWeaponType())
	{
		case WEAPON_SWORD:
			return getSkill(SKILL_SWORD, SKILL_LEVEL);

		case WEAPON_CLUB:
			return getSkill(SKILL_CLUB, SKILL_LEVEL);

		case WEAPON_AXE:
			return getSkill(SKILL_AXE, SKILL_LEVEL);

		case WEAPON_FIST:
			return getSkill(SKILL_FIST, SKILL_LEVEL);

		case WEAPON_DIST:
			return getSkill(SKILL_DIST, SKILL_LEVEL);

		default:
			break;
	}

	return 0;
}

int32_t Player::getArmor() const
{
	int32_t armor = 0;
	if(getInventoryItem(SLOT_HEAD))
		armor += getInventoryItem(SLOT_HEAD)->getArmor();
	if(getInventoryItem(SLOT_NECKLACE))
		armor += getInventoryItem(SLOT_NECKLACE)->getArmor();
	if(getInventoryItem(SLOT_ARMOR))
		armor += getInventoryItem(SLOT_ARMOR)->getArmor();
	if(getInventoryItem(SLOT_LEGS))
		armor += getInventoryItem(SLOT_LEGS)->getArmor();
	if(getInventoryItem(SLOT_FEET))
		armor += getInventoryItem(SLOT_FEET)->getArmor();
	if(getInventoryItem(SLOT_RING))
		armor += getInventoryItem(SLOT_RING)->getArmor();

	if(vocation->armorMultipler != 1.0)
		return int32_t(armor * vocation->armorMultipler);

	return armor;
}

void Player::getShieldAndWeapon(const Item* &shield, const Item* &weapon) const
{
	shield = weapon = NULL;
	for(uint32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
	{
		Item* item = getInventoryItem((slots_t)slot);
		if(!item)
			continue;

		switch(item->getWeaponType())
		{
			case WEAPON_NONE:
				break;

			case WEAPON_SHIELD:
			{
				if(!shield || (shield && item->getDefense() > shield->getDefense()))
					shield = item;

				break;
			}

			default: //weapons that are not shields
			{
				weapon = item;
				break;
			}
		}
	}
}

int32_t Player::getDefense() const
{
	int32_t baseDefense = 5, defenseValue = 0, defenseSkill = 0, extraDefense = 0;
	float defenseFactor = getDefenseFactor();

	const Item* weapon = NULL;
	const Item* shield = NULL;
	getShieldAndWeapon(shield, weapon);
	if(weapon)
	{
		extraDefense = weapon->getExtraDefense();
		defenseValue = baseDefense + weapon->getDefense();
		defenseSkill = getWeaponSkill(weapon);
	}

	if(shield && shield->getDefense() > defenseValue)
	{
		if(shield->getExtraDefense() > extraDefense)
			extraDefense = shield->getExtraDefense();

		defenseValue = baseDefense + shield->getDefense();
		defenseSkill = getSkill(SKILL_SHIELD, SKILL_LEVEL);
	}

	if(!defenseSkill)
		return 0;

	defenseValue += extraDefense;
	if(vocation->defenseMultipler != 1.0)
		defenseValue = int32_t(defenseValue * vocation->defenseMultipler);

	return ((int32_t)std::ceil(((float)(defenseSkill * (defenseValue * 0.015)) + (defenseValue * 0.1)) * defenseFactor));
}

float Player::getAttackFactor() const
{
	switch(fightMode)
	{
		case FIGHTMODE_BALANCED:
			return 1.2f;

		case FIGHTMODE_DEFENSE:
			return 2.0f;

		case FIGHTMODE_ATTACK:
		default:
			break;
	}

	return 1.0f;
}

float Player::getDefenseFactor() const
{
	switch(fightMode)
	{
		case FIGHTMODE_BALANCED:
			return 1.2f;

		case FIGHTMODE_DEFENSE:
		{
			if((OTSYS_TIME() - lastAttack) < const_cast<Player*>(this)->getAttackSpeed()) //Attacking will cause us to get into normal defense
				return 1.0f;

			return 2.0f;
		}

		case FIGHTMODE_ATTACK:
		default:
			break;
	}

	return 1.0f;
}

void Player::sendIcons() const
{
	if(client)
	{
		int32_t icons = 0;
		for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it)
		{
			if(!isSuppress((*it)->getType()))
				icons |= (*it)->getIcons();
		}

		client->sendIcons(icons);
	}
}

void Player::updateInventoryWeigth()
{
	inventoryWeight = 0.00;
	if(!hasFlag(PlayerFlag_HasInfiniteCapacity))
	{
		for(int32_t i = SLOT_FIRST; i < SLOT_LAST; ++i)
		{
			if(Item* item = getInventoryItem((slots_t)i))
				inventoryWeight += item->getWeight();
		}
	}
}

void Player::updateInventoryGoods(uint32_t itemId)
{
	if(Item::items[itemId].worth)
	{
		sendGoods();
		return;
	}

	bool send = false;
	for(ShopInfoList::iterator it = shopOffer.begin(); it != shopOffer.end(); ++it)
	{
		if((*it).sellPrice > 0 && (*it).itemId == itemId)
		{
			uint32_t itemCount = __getItemTypeCount((*it).itemId, ((*it).subType ? (*it).subType : -1));
			if(itemCount > 0)
				goodsMap[(*it).itemId] = itemCount;
			else
				goodsMap.erase((*it).itemId);

			if(!send)
				send = true;
		}
	}

	if(send)
		sendGoods();
}

int32_t Player::getPlayerInfo(playerinfo_t playerinfo) const
{
	switch(playerinfo)
	{
		case PLAYERINFO_LEVEL:
			return level;
		case PLAYERINFO_LEVELPERCENT:
			return levelPercent;
		case PLAYERINFO_MAGICLEVEL:
			return std::max((int32_t)0, ((int32_t)magLevel + varStats[STAT_MAGICLEVEL]));
		case PLAYERINFO_MAGICLEVELPERCENT:
			return magLevelPercent;
		case PLAYERINFO_HEALTH:
			return health;
		case PLAYERINFO_MAXHEALTH:
			return std::max((int32_t)1, ((int32_t)healthMax + varStats[STAT_MAXHEALTH]));
		case PLAYERINFO_MANA:
			return mana;
		case PLAYERINFO_MAXMANA:
			return std::max((int32_t)0, ((int32_t)manaMax + varStats[STAT_MAXMANA]));
		case PLAYERINFO_SOUL:
			return std::max((int32_t)0, ((int32_t)soul + varStats[STAT_SOUL]));
		default:
			break;
	}

	return 0;
}

int32_t Player::getSkill(skills_t skilltype, skillsid_t skillinfo) const
{
	int32_t ret = skills[skilltype][skillinfo];
	if(skillinfo == SKILL_LEVEL)
		ret += varSkills[skilltype];

	return std::max((int32_t)0, ret);
}

void Player::addSkillAdvance(skills_t skill, uint32_t count, bool useMultiplier/* = true*/)
{
	//player has reached max skill
	if(vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL]) > vocation->getReqSkillTries(
		skill, skills[skill][SKILL_LEVEL] + 1))
		return;

	if(useMultiplier)
		count += uint32_t((double)count * rates[skill]);

	count = uint32_t((double)count * g_config.getDouble(ConfigManager::RATE_SKILL));
	bool advance = false;
	while(skills[skill][SKILL_TRIES] + count >= vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL] + 1))
	{
		count -= vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL] + 1) - skills[skill][SKILL_TRIES];
	 	skills[skill][SKILL_LEVEL]++;
	 	skills[skill][SKILL_TRIES] = 0;
		skills[skill][SKILL_PERCENT] = 0;

		char buffer[10];
		if(g_config.getBool(ConfigManager::ADVANCING_SKILL_LEVEL))
			sprintf(buffer, " [%u]", skills[skill][SKILL_LEVEL]);
		else
			sprintf(buffer, "%s", "");

		char advMsg[45];
		sprintf(advMsg, "You advanced in %s%s.", getSkillName(skill).c_str(), buffer);
		sendTextMessage(MSG_EVENT_ADVANCE, advMsg);

		advance = true;
		CreatureEventList advanceEvents = getCreatureEvents(CREATURE_EVENT_ADVANCE);
		for(CreatureEventList::iterator it = advanceEvents.begin(); it != advanceEvents.end(); ++it)
			(*it)->executeAdvance(this, skill, (skills[skill][SKILL_LEVEL] - 1), skills[skill][SKILL_LEVEL]);

		if(vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL]) > vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL] + 1))
		{
			count = 0;
			break;
		}
	}

	skills[skill][SKILL_TRIES] += count;
	//update percent
	uint32_t newPercent = Player::getPercentLevel(skills[skill][SKILL_TRIES], vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL] + 1));
 	if(skills[skill][SKILL_PERCENT] != newPercent)
	{
		skills[skill][SKILL_PERCENT] = newPercent;
		sendSkills();
 	}
	else if(advance)
		sendSkills();
}

void Player::setVarStats(stats_t stat, int32_t modifier)
{
	varStats[stat] += modifier;
	switch(stat)
	{
		case STAT_MAXHEALTH:
		{
			if(getHealth() > getMaxHealth())
				Creature::changeHealth(getMaxHealth() - getHealth());
			else
				g_game.addCreatureHealth(this);
			break;
		}

		case STAT_MAXMANA:
		{
			if(getMana() > getMaxMana())
				Creature::changeMana(getMaxMana() - getMana());
			break;
		}

		default:
			break;
	}
}

int32_t Player::getDefaultStats(stats_t stat)
{
	switch(stat)
	{
		case STAT_MAXHEALTH:
			return getMaxHealth() - getVarStats(STAT_MAXHEALTH);
		case STAT_MAXMANA:
			return getMaxMana() - getVarStats(STAT_MAXMANA);
		case STAT_SOUL:
			return getSoul() - getVarStats(STAT_SOUL);
		case STAT_MAGICLEVEL:
			return getMagicLevel() - getVarStats(STAT_MAGICLEVEL);
		default:
			break;
	}

	return 0;
}

Container* Player::getContainer(uint32_t cid)
{
	for(ContainerVector::iterator it = containerVec.begin(); it != containerVec.end(); ++it)
	{
		if(it->first == cid)
			return it->second;
	}

	return NULL;
}

int32_t Player::getContainerID(const Container* container) const
{
	for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
	{
		if(cl->second == container)
			return cl->first;
	}

	return -1;
}

void Player::addContainer(uint32_t cid, Container* container)
{
#ifdef __DEBUG__
	std::cout << getName() << ", addContainer: " << (int32_t)cid << std::endl;
#endif
	if(cid > 0xF)
		return;

	for(ContainerVector::iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
	{
		if(cl->first == cid)
		{
			cl->second = container;
			return;
		}
	}

	containerVec.push_back(std::make_pair(cid, container));
}

void Player::closeContainer(uint32_t cid)
{
	for(ContainerVector::iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
	{
		if(cl->first == cid)
		{
			containerVec.erase(cl);
			break;
		}
	}
#ifdef __DEBUG__

	std::cout << getName() << ", closeContainer: " << (int32_t)cid << std::endl;
#endif
}

bool Player::canOpenCorpse(uint32_t ownerId)
{
	return getID() == ownerId || (party && party->canOpenCorpse(ownerId)) || hasCustomFlag(PlayerCustomFlag_GamemasterPrivileges);
}

uint16_t Player::getLookCorpse() const
{
	if(sex != 0)
		return ITEM_MALE_CORPSE;

	return ITEM_FEMALE_CORPSE;
}

void Player::dropLoot(Container* corpse)
{
	if(!corpse || lootDrop != LOOT_DROP_FULL)
		return;

	uint32_t start = g_config.getNumber(ConfigManager::BLESS_REDUCTION_BASE), loss = lossPercent[LOSS_CONTAINERS], bless = getBlessings();
	while(bless > 0 && loss > 0)
	{
		loss -= start;
		start -= g_config.getNumber(ConfigManager::BLESS_REDUCTION_DECREAMENT);
		bless--;
	}

	uint32_t itemLoss = (uint32_t)std::floor((float)(loss + 5) * lossPercent[LOSS_ITEMS] / 1000.);
	for(uint8_t i = SLOT_FIRST; i < SLOT_LAST; ++i)
	{
		if(Item* item = inventory[i])
		{
			uint32_t rand = random_range(1, 100);
			if(getSkull() == SKULL_RED || (item->getContainer() && rand <= loss) || (!item->getContainer() && rand <= itemLoss))
			{
				g_game.internalMoveItem(NULL, this, corpse, INDEX_WHEREEVER, item, item->getItemCount(), 0);
				sendRemoveInventoryItem((slots_t)i, inventory[(slots_t)i]);
			}
		}
	}
}

bool Player::getStorageValue(const uint32_t key, std::string& value) const
{
	StorageMap::const_iterator it = storageMap.find(key);
	if(it != storageMap.end())
	{
		value = it->second;
		return true;
	}

	value = "-1";
	return false;
}

bool Player::addStorageValue(const uint32_t key, const std::string& value)
{
	if(IS_IN_KEYRANGE(key, RESERVED_RANGE))
	{
		if(IS_IN_KEYRANGE(key, OUTFITS_RANGE))
		{
			Outfit outfit;
			outfit.looktype = atoi(value.c_str()) >> 16;
			outfit.addons = atoi(value.c_str()) & 0xFF;
			if(outfit.addons <= 3)
			{
				m_playerOutfits.addOutfit(outfit);
				return true;
			}
			else
				std::cout << "[Warning - Player::addStorageValue]: Invalid addons value key: " << key << ", value: " << value << " for player: " << getName() << std::endl;
		}
		else
			std::cout << "[Warning - Player::addStorageValue]: Unknown reserved key: " << key << " for player: " << getName() << std::endl;
	}
	else
	{
		storageMap[key] = value;
		return true;
	}

	return false;
}

bool Player::eraseStorageValue(const uint32_t key)
{
	if(IS_IN_KEYRANGE(key, RESERVED_RANGE))
		std::cout << "[Warning - Player::eraseStorageValue]: Unknown reserved key: " << key << " for player: " << getName() << std::endl;

	return storageMap.erase(key);
}

bool Player::canSee(const Position& pos) const
{
	if(client)
		return client->canSee(pos);

	return false;
}

bool Player::canSeeCreature(const Creature* creature) const
{
	return !(creature->isInvisible() && !creature->getPlayer() && !canSeeInvisibility());
}

Depot* Player::getDepot(uint32_t depotId, bool autoCreateDepot)
{
	DepotMap::iterator it = depots.find(depotId);
	if(it != depots.end())
		return it->second.first;

	//create a new depot?
	if(autoCreateDepot)
	{
		Item* locker = Item::CreateItem(ITEM_LOCKER1);
		if(Container* container = locker->getContainer())
		{
			if(Depot* depot = container->getDepot())
			{
				container->__internalAddThing(Item::CreateItem(ITEM_DEPOT));
				addDepot(depot, depotId);
				return depot;
			}
		}

		g_game.FreeThing(locker);
		std::cout << "Failure: Creating a new depot with id: " << depotId <<
			", for player: " << getName() << std::endl;
	}

	return NULL;
}

bool Player::addDepot(Depot* depot, uint32_t depotId)
{
	if(getDepot(depotId, false))
		return false;

	depots[depotId] = std::make_pair(depot, false);
	depot->setMaxDepotLimit(maxDepotLimit);
	return true;
}

void Player::useDepot(uint32_t depotId, bool value)
{
	DepotMap::iterator it = depots.find(depotId);
	if(it != depots.end())
		depots[depotId] = std::make_pair(it->second.first, value);
}

void Player::sendCancelMessage(ReturnValue message) const
{
	switch(message)
	{
		case RET_DESTINATIONOUTOFREACH:
			sendCancel("Destination is out of reach.");
			break;

		case RET_NOTMOVEABLE:
			sendCancel("You cannot move this object.");
			break;

		case RET_DROPTWOHANDEDITEM:
			sendCancel("Drop the double-handed object first.");
			break;

		case RET_BOTHHANDSNEEDTOBEFREE:
			sendCancel("Both hands needs to be free.");
			break;

		case RET_CANNOTBEDRESSED:
			sendCancel("You cannot dress this object there.");
			break;

		case RET_PUTTHISOBJECTINYOURHAND:
			sendCancel("Put this object in your hand.");
			break;

		case RET_PUTTHISOBJECTINBOTHHANDS:
			sendCancel("Put this object in both hands.");
			break;

		case RET_CANONLYUSEONEWEAPON:
			sendCancel("You may use only one weapon.");
			break;

		case RET_TOOFARAWAY:
			sendCancel("Too far away.");
			break;

		case RET_FIRSTGODOWNSTAIRS:
			sendCancel("First go downstairs.");
			break;

		case RET_FIRSTGOUPSTAIRS:
			sendCancel("First go upstairs.");
			break;

		case RET_NOTENOUGHCAPACITY:
			sendCancel("This object is too heavy.");
			break;

		case RET_CONTAINERNOTENOUGHROOM:
			sendCancel("You cannot put more objects in this container.");
			break;

		case RET_NEEDEXCHANGE:
		case RET_NOTENOUGHROOM:
			sendCancel("There is not enough room.");
			break;

		case RET_CANNOTPICKUP:
			sendCancel("You cannot pickup this object.");
			break;

		case RET_CANNOTTHROW:
			sendCancel("You cannot throw there.");
			break;

		case RET_THEREISNOWAY:
			sendCancel("There is no way.");
			break;

		case RET_THISISIMPOSSIBLE:
			sendCancel("This is impossible.");
			break;

		case RET_PLAYERISPZLOCKED:
			sendCancel("You can not enter a protection zone after attacking another player.");
			break;

		case RET_PLAYERISNOTINVITED:
			sendCancel("You are not invited.");
			break;

		case RET_CREATUREDOESNOTEXIST:
			sendCancel("Creature does not exist.");
			break;

		case RET_DEPOTISFULL:
			sendCancel("You cannot put more items in this depot.");
			break;

		case RET_CANNOTUSETHISOBJECT:
			sendCancel("You can not use this object.");
			break;

		case RET_PLAYERWITHTHISNAMEISNOTONLINE:
			sendCancel("A player with this name is not online.");
			break;

		case RET_NOTREQUIREDLEVELTOUSERUNE:
			sendCancel("You do not have the required magic level to use this rune.");
			break;

		case RET_YOUAREALREADYTRADING:
			sendCancel("You are already trading.");
			break;

		case RET_THISPLAYERISALREADYTRADING:
			sendCancel("This player is already trading.");
			break;

		case RET_YOUMAYNOTLOGOUTDURINGAFIGHT:
			sendCancel("You may not logout during or immediately after a fight!");
			break;

		case RET_DIRECTPLAYERSHOOT:
			sendCancel("You are not allowed to shoot directly on players.");
			break;

		case RET_NOTENOUGHLEVEL:
			sendCancel("You do not have enough level.");
			break;

		case RET_NOTENOUGHMAGICLEVEL:
			sendCancel("You do not have enough magic level.");
			break;

		case RET_NOTENOUGHMANA:
			sendCancel("You do not have enough mana.");
			break;

		case RET_NOTENOUGHSOUL:
			sendCancel("You do not have enough soul.");
			break;

		case RET_YOUAREEXHAUSTED:
			sendCancel("You are exhausted.");
			break;

		case RET_CANONLYUSETHISRUNEONCREATURES:
			sendCancel("You can only use this rune on creatures.");
			break;

		case RET_PLAYERISNOTREACHABLE:
			sendCancel("Player is not reachable.");
			break;

		case RET_CREATUREISNOTREACHABLE:
			sendCancel("Creature is not reachable.");
			break;

		case RET_ACTIONNOTPERMITTEDINPROTECTIONZONE:
			sendCancel("This action is not permitted in a protection zone.");
			break;

		case RET_YOUMAYNOTATTACKTHISPLAYER:
			sendCancel("You may not attack this player.");
			break;

		case RET_YOUMAYNOTATTACKTHISCREATURE:
			sendCancel("You may not attack this creature.");
			break;

		case RET_YOUMAYNOTATTACKAPERSONINPROTECTIONZONE:
			sendCancel("You may not attack a person in a protection zone.");
			break;

		case RET_YOUMAYNOTATTACKAPERSONWHILEINPROTECTIONZONE:
			sendCancel("You may not attack a person while you are in a protection zone.");
			break;

		case RET_YOUCANONLYUSEITONCREATURES:
			sendCancel("You can only use it on creatures.");
			break;

		case RET_TURNSECUREMODETOATTACKUNMARKEDPLAYERS:
			sendCancel("Turn secure mode off if you really want to attack unmarked players.");
			break;

		case RET_YOUNEEDPREMIUMACCOUNT:
			sendCancel("You need a premium account.");
			break;

		case RET_YOUNEEDTOLEARNTHISSPELL:
			sendCancel("You need to learn this spell first.");
			break;

		case RET_YOURVOCATIONCANNOTUSETHISSPELL:
			sendCancel("Your vocation cannot use this spell.");
			break;

		case RET_YOUNEEDAWEAPONTOUSETHISSPELL:
			sendCancel("You need to equip a weapon to use this spell.");
			break;

		case RET_PLAYERISPZLOCKEDLEAVEPVPZONE:
			sendCancel("You can not leave a pvp zone after attacking another player.");
			break;

		case RET_PLAYERISPZLOCKEDENTERPVPZONE:
			sendCancel("You can not enter a pvp zone after attacking another player.");
			break;

		case RET_ACTIONNOTPERMITTEDINANOPVPZONE:
			sendCancel("This action is not permitted in a non-pvp zone.");
			break;

		case RET_YOUCANNOTLOGOUTHERE:
			sendCancel("You can not logout here.");
			break;

		case RET_YOUNEEDAMAGICITEMTOCASTSPELL:
			sendCancel("You need a magic item to cast this spell.");
			break;

		case RET_CANNOTCONJUREITEMHERE:
			sendCancel("You cannot conjure items here.");
			break;

		case RET_YOUNEEDTOSPLITYOURSPEARS:
			sendCancel("You need to split your spears first.");
			break;

		case RET_NAMEISTOOAMBIGUOUS:
			sendCancel("Name is too ambiguous.");
			break;

		case RET_CANONLYUSEONESHIELD:
			sendCancel("You may use only one shield.");
			break;

		case RET_YOUARENOTTHEOWNER:
			sendCancel("You are not the owner.");
			break;

		case RET_NOPARTYMEMBERSINRANGE:
			sendCancel("No party members in range.");
			break;

		case RET_NOTPOSSIBLE:
		default:
			sendCancel("Sorry, not possible.");
			break;
	}
}

void Player::sendStats()
{
	if(client)
		client->sendStats();
}

void Player::sendPing(uint32_t interval)
{
	internalPing += interval;
	if(internalPing >= 5000) //1 ping each 5 seconds
	{
		internalPing = 0;
		npings++;
		if(client)
			client->sendPing();
		else if(g_config.getBool(ConfigManager::STOP_ATTACK_AT_EXIT))
			setAttackedCreature(NULL);
	}

	if(canLogout())
	{
		if(!client)
		{
			g_creatureEvents->playerLogout(this);
			g_game.removeCreature(this, true);
		}
		else if(npings > 24)
			client->logout(true, true);
	}
}

Item* Player::getWriteItem(uint32_t& _windowTextId, uint16_t& _maxWriteLen)
{
	_windowTextId = windowTextId;
	_maxWriteLen = maxWriteLen;
	return writeItem;
}

void Player::setWriteItem(Item* item, uint16_t _maxWriteLen /*= 0*/)
{
	windowTextId++;
	if(writeItem)
		writeItem->releaseThing2();

	if(item)
	{
		writeItem = item;
		maxWriteLen = _maxWriteLen;
		writeItem->useThing2();
	}
	else
	{
		writeItem = NULL;
		maxWriteLen = 0;
	}
}

House* Player::getEditHouse(uint32_t& _windowTextId, uint32_t& _listId)
{
	_windowTextId = windowTextId;
	_listId = editListId;
	return editHouse;
}

void Player::setEditHouse(House* house, uint32_t listId /*= 0*/)
{
	windowTextId++;
	editHouse = house;
	editListId = listId;
}

void Player::sendHouseWindow(House* house, uint32_t listId) const
{
	if(client)
	{
		std::string text;
		if(house->getAccessList(listId, text))
			client->sendHouseWindow(windowTextId, house, listId, text);
	}
}

//container
void Player::sendAddContainerItem(const Container* container, const Item* item)
{
	if(client)
	{
		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
		{
			if(cl->second == container)
				client->sendAddContainerItem(cl->first, item);
		}
	}
}

void Player::sendUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem)
{
	if(client)
	{
		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
		{
			if(cl->second == container)
				client->sendUpdateContainerItem(cl->first, slot, newItem);
		}
	}
}

void Player::sendRemoveContainerItem(const Container* container, uint8_t slot, const Item* item)
{
	if(client)
	{
		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
		{
			if(cl->second == container)
				client->sendRemoveContainerItem(cl->first, slot);
		}
	}
}

void Player::onUpdateTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
	const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType)
{
	Creature::onUpdateTileItem(tile, pos, stackpos, oldItem, oldType, newItem, newType);
	if(oldItem != newItem)
		onRemoveTileItem(tile, pos, stackpos, oldType, oldItem);

	if(tradeState != TRADE_TRANSFER && tradeItem && oldItem == tradeItem)
		g_game.internalCloseTrade(this);
}

void Player::onRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
	const ItemType& iType, const Item* item)
{
	Creature::onRemoveTileItem(tile, pos, stackpos, iType, item);
	if(tradeState != TRADE_TRANSFER)
	{
		checkTradeState(item);
		if(tradeItem)
		{
			const Container* container = item->getContainer();
			if(container && container->isHoldingItem(tradeItem))
				g_game.internalCloseTrade(this);
		}
	}
}

void Player::onCreatureAppear(const Creature* creature, bool isLogin)
{
	Creature::onCreatureAppear(creature, isLogin);
	if(isLogin && creature == this)
	{
		for(int32_t slot = SLOT_FIRST; slot < SLOT_LAST; ++slot)
		{
			if(Item* item = getInventoryItem((slots_t)slot))
			{
				item->__startDecaying();
				g_moveEvents->onPlayerEquip(this, item, (slots_t)slot, false);
			}
		}

		if(BedItem* bed = Beds::getInstance().getBedBySleeper(getGUID()))
		{
			bed->wakeUp(this);
			#ifdef __DEBUG__
			std::cout << "Player " << getName() << " waking up." << std::endl;
			#endif
		}

		if(lastLogout)
		{
			int64_t period = (int32_t)time(NULL) - lastLogout;
			if(period - 600 > 0)
			{
				period = (int64_t)std::ceil((double)period * g_config.getDouble(ConfigManager::RATE_STAMINA_GAIN));
				int64_t rated = stamina + period, tmp = g_config.getNumber(
					ConfigManager::STAMINA_LIMIT_TOP) * STAMINA_MUL;
				if(rated >= tmp)
					rated -= tmp;
				else
					rated = 0;

				addStamina(period - rated);
				if(rated > 0)
				{
					tmp = (int64_t)std::ceil((double)rated / g_config.getDouble(ConfigManager::RATE_STAMINA_THRESHOLD));
					if(stamina + tmp > STAMINA_MAX)
						tmp = STAMINA_MAX;

					addStamina(tmp);
				}
			}
				
		}

		g_game.checkPlayersRecord();
		if(!isInGhostMode())
			IOLoginData::getInstance()->updateOnlineStatus(guid, true);

		#ifndef __CONSOLE__
		GUI::getInstance()->m_pBox.addPlayer(this);
		#endif
		if(g_config.getBool(ConfigManager::DISPLAY_LOGGING))
			std::cout << name << " has logged in." << std::endl;
	}
}

void Player::onAttackedCreatureDisappear(bool isLogout)
{
	sendCancelTarget();
	if(!isLogout)
		sendTextMessage(MSG_STATUS_SMALL, "Target lost.");
}

void Player::onFollowCreatureDisappear(bool isLogout)
{
	sendCancelTarget();
	if(!isLogout)
		sendTextMessage(MSG_STATUS_SMALL, "Target lost.");
}

void Player::onChangeZone(ZoneType_t zone)
{
	if(attackedCreature && zone == ZONE_PROTECTION && !hasFlag(PlayerFlag_IgnoreProtectionZone))
	{
		setAttackedCreature(NULL);
		onAttackedCreatureDisappear(false);
	}
}

void Player::onAttackedCreatureChangeZone(ZoneType_t zone)
{
	if(zone == ZONE_PROTECTION && !hasFlag(PlayerFlag_IgnoreProtectionZone))
	{
		setAttackedCreature(NULL);
		onAttackedCreatureDisappear(false);
	}
	else if(zone == ZONE_NOPVP && attackedCreature->getPlayer() && !hasFlag(PlayerFlag_IgnoreProtectionZone))
	{
		setAttackedCreature(NULL);
		onAttackedCreatureDisappear(false);
	}
	else if(zone == ZONE_NORMAL && g_game.getWorldType() == WORLD_TYPE_NO_PVP && attackedCreature->getPlayer())
	{
		//attackedCreature can leave a pvp zone if not pzlocked
		setAttackedCreature(NULL);
		onAttackedCreatureDisappear(false);
	}
}

void Player::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	Creature::onCreatureDisappear(creature, stackpos, isLogout);
	if(creature != this)
		return;

	if(isLogout)
	{
		loginPosition = getPosition();
		lastLogout = time(NULL);
	}

	if(eventWalk != 0)
		setFollowCreature(NULL);

	if(tradePartner)
		g_game.internalCloseTrade(this);

	clearPartyInvitations();
	if(getParty())
		getParty()->leaveParty(this);

	g_game.cancelRuleViolation(this);
	if(hasFlag(PlayerFlag_CanAnswerRuleViolations))
	{
		PlayerVector closeReportList;
		for(RuleViolationsMap::const_iterator it = g_game.getRuleViolations().begin(); it != g_game.getRuleViolations().end(); ++it)
		{
			if(it->second->gamemaster == this)
				closeReportList.push_back(it->second->reporter);
		}

		for(PlayerVector::iterator it = closeReportList.begin(); it != closeReportList.end(); ++it)
			g_game.closeRuleViolation(*it);
	}

	g_chat.removeUserFromAllChannels(this);
	if(!isInGhostMode())
		IOLoginData::getInstance()->updateOnlineStatus(guid, false);

	#ifndef __CONSOLE__
	GUI::getInstance()->m_pBox.removePlayer(this);
	#endif
	if(g_config.getBool(ConfigManager::DISPLAY_LOGGING))
		std::cout << getName() << " has logged out." << std::endl;

	bool saved = false;
	for(uint32_t tries = 0; !saved && tries < 3; ++tries)
	{
		if(IOLoginData::getInstance()->savePlayer(this))
			saved = true;
#ifdef __DEBUG__
		else
			std::cout << "Error while saving player: " << getName() << ", strike " << tries << "." << std::endl;
#endif
	}

	if(!saved)
#ifndef __DEBUG__
		std::cout << "Error while saving player: " << getName() << "." << std::endl;
#else
		std::cout << "Player " << getName() << " couldn't be saved." << std::endl;
#endif
}

void Player::openShopWindow()
{
	for(ShopInfoList::iterator it = shopOffer.begin(); it != shopOffer.end(); ++it)
	{
		if((*it).sellPrice > 0)
		{
			uint32_t itemCount = __getItemTypeCount((*it).itemId, ((*it).subType ? (*it).subType : -1));
			if(itemCount > 0)
				goodsMap[(*it).itemId] = itemCount;
		}
	}

	sendShop();
	sendGoods();
}

void Player::closeShopWindow(Npc* npc/* = NULL*/, int32_t onBuy/* = -1*/, int32_t onSell/* = -1*/)
{
	if(npc || (npc = getShopOwner(onBuy, onSell)))
		npc->onPlayerEndTrade(this, onBuy, onSell);

	if(shopOwner)
		sendCloseShop();

	shopOwner = NULL;
	purchaseCallback = saleCallback = -1;
	shopOffer.clear();
	goodsMap.clear();
}

bool Player::canShopItem(uint32_t itemId, ShopEvent_t event)
{
	for(ShopInfoList::iterator it = shopOffer.begin(); it != shopOffer.end(); ++it)
	{
		if((*it).itemId == itemId && ((event == SHOPEVENT_BUY && (*it).buyPrice > -1) || (event == SHOPEVENT_SELL && (*it).sellPrice > -1)))
			return true;
	}

	return false;
}

void Player::onWalk(Direction& dir)
{
	Creature::onWalk(dir);
	setNextActionTask(NULL);
	setNextAction(OTSYS_TIME() + getStepDuration());
}

void Player::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	Creature::onCreatureMove(creature, newTile, newPos, oldTile, oldPos, oldStackPos, teleport);
	if(creature != this)
		return;

	if(getParty())
		getParty()->updateSharedExperience();

	//check if we should close trade
	if(tradeState != TRADE_TRANSFER && ((tradeItem && !Position::areInRange<1,1,0>(tradeItem->getPosition(), getPosition()))
		|| (tradePartner && !Position::areInRange<2,2,0>(tradePartner->getPosition(), getPosition()))))
		g_game.internalCloseTrade(this);

	if(teleport || oldPos.z != newPos.z)
	{
		int32_t ticks = g_config.getNumber(ConfigManager::STAIRHOP_DELAY);
		if(ticks > 0)
		{
			addExhaust(g_config.getNumber(ConfigManager::STAIRHOP_DELAY), 1);
			addExhaust(g_config.getNumber(ConfigManager::STAIRHOP_DELAY), 3);
			if(Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_PACIFIED,
				g_config.getNumber(ConfigManager::STAIRHOP_DELAY)))
				addCondition(condition);
		}
	}
}

//container
void Player::onAddContainerItem(const Container* container, const Item* item)
{
	checkTradeState(item);
}

void Player::onUpdateContainerItem(const Container* container, uint8_t slot,
	const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType)
{
	if(oldItem != newItem)
		onRemoveContainerItem(container, slot, oldItem);

	if(tradeState != TRADE_TRANSFER)
		checkTradeState(oldItem);
}

void Player::onRemoveContainerItem(const Container* container, uint8_t slot, const Item* item)
{
	if(tradeState != TRADE_TRANSFER)
	{
		checkTradeState(item);
		if(tradeItem)
		{
			if(tradeItem->getParent() != container && container->isHoldingItem(tradeItem))
				g_game.internalCloseTrade(this);
		}
	}
}

void Player::onCloseContainer(const Container* container)
{
	if(!client)
		return;

	for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
	{
		if(cl->second == container)
			client->sendCloseContainer(cl->first);
	}
}

void Player::onSendContainer(const Container* container)
{
	if(!client)
		return;

	bool hasParent = dynamic_cast<const Container*>(container->getParent()) != NULL;
	for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
	{
		if(cl->second == container)
			client->sendContainer(cl->first, container, hasParent);
	}
}

//inventory
void Player::onAddInventoryItem(slots_t slot, Item* item)
{
	//
}

void Player::onUpdateInventoryItem(slots_t slot, Item* oldItem, const ItemType& oldType,
	Item* newItem, const ItemType& newType)
{
	if(oldItem != newItem)
		onRemoveInventoryItem(slot, oldItem);

	if(tradeState != TRADE_TRANSFER)
		checkTradeState(oldItem);
}

void Player::onRemoveInventoryItem(slots_t slot, Item* item)
{
	if(tradeState == TRADE_TRANSFER)
		return;

	checkTradeState(item);
	if(tradeItem)
	{
		const Container* container = item->getContainer();
		if(container && container->isHoldingItem(tradeItem))
			g_game.internalCloseTrade(this);
	}
}

void Player::checkTradeState(const Item* item)
{
	if(!tradeItem || tradeState == TRADE_TRANSFER)
		return;

	if(tradeItem != item)
	{
		const Container* container = dynamic_cast<const Container*>(item->getParent());
		while(container != NULL)
		{
			if(container == tradeItem)
			{
				g_game.internalCloseTrade(this);
				break;
			}

			container = dynamic_cast<const Container*>(container->getParent());
		}
	}
	else
		g_game.internalCloseTrade(this);
}

void Player::setNextWalkActionTask(SchedulerTask* task)
{
	if(walkTaskEvent != 0)
	{
		Scheduler::getScheduler().stopEvent(walkTaskEvent);
		walkTaskEvent = 0;
	}

	delete walkTask;
	walkTask = task;
}

void Player::setNextWalkTask(SchedulerTask* task)
{
	if(nextStepEvent != 0)
	{
		Scheduler::getScheduler().stopEvent(nextStepEvent);
		nextStepEvent = 0;
	}

	if(task)
		nextStepEvent = Scheduler::getScheduler().addEvent(task);
}

void Player::setNextActionTask(SchedulerTask* task)
{
	if(actionTaskEvent != 0)
	{
		Scheduler::getScheduler().stopEvent(actionTaskEvent);
		actionTaskEvent = 0;
	}

	if(task)
		actionTaskEvent = Scheduler::getScheduler().addEvent(task);
}

uint32_t Player::getNextActionTime() const
{
	int64_t time = nextAction - OTSYS_TIME();
	if(time < SCHEDULER_MINTICKS)
		return SCHEDULER_MINTICKS;

	return time;
}

void Player::onThink(uint32_t interval)
{
	Creature::onThink(interval);
	sendPing(interval);

	MessageBufferTicks += interval;
	if(MessageBufferTicks >= 1500)
	{
		MessageBufferTicks = 0;
		addMessageBuffer();
	}

	if(!getTile()->hasFlag(TILESTATE_NOLOGOUT) && !getNoMove() && !hasFlag(PlayerFlag_NotGainInFight))
	{
		idleTime += interval;
		if(idleTime > (g_config.getNumber(ConfigManager::KICK_AFTER_MINUTES) * 60000) + 60000)
			kickPlayer(true);
		else if(client && idleTime == g_config.getNumber(ConfigManager::KICK_AFTER_MINUTES) * 60000)
		{
			char buffer[130];
			sprintf(buffer, "You have been idle for %d minutes, you will be disconnected in one minute if you are still idle.",
				g_config.getNumber(ConfigManager::KICK_AFTER_MINUTES));
			client->sendTextMessage(MSG_STATUS_WARNING, buffer);
		}
	}

	if(g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
		checkRedSkullTicks(interval);
}

uint32_t Player::isMuted()
{
	if(hasFlag(PlayerFlag_CannotBeMuted))
		return 0;

	int32_t muteTicks = 0;
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end(); ++it)
	{
		if((*it)->getType() == CONDITION_MUTED && (*it)->getSubId() == 0 && (*it)->getTicks() > muteTicks)
			muteTicks = (*it)->getTicks();
	}

	return ((uint32_t)muteTicks / 1000);
}

void Player::addMessageBuffer()
{
	if(!hasFlag(PlayerFlag_CannotBeMuted) && Player::maxMessageBuffer != 0 && MessageBufferCount > 0)
		MessageBufferCount--;
}

void Player::removeMessageBuffer()
{
	if(!hasFlag(PlayerFlag_CannotBeMuted) && Player::maxMessageBuffer != 0 && MessageBufferCount <= Player::maxMessageBuffer + 1)
	{
		MessageBufferCount++;
		if(MessageBufferCount > Player::maxMessageBuffer)
		{
			uint32_t muteCount = 1;
			MuteCountMap::iterator it = muteCountMap.find(getGUID());
			if(it != muteCountMap.end())
				muteCount = it->second;

			uint32_t muteTime = 5 * muteCount * muteCount;
			muteCountMap[getGUID()] = muteCount + 1;
			if(Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_MUTED, muteTime * 1000))
				addCondition(condition);

			char buffer[50];
			sprintf(buffer, "You are muted for %d seconds.", muteTime);
			sendTextMessage(MSG_STATUS_SMALL, buffer);
		}
	}
}

void Player::drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage)
{
	Creature::drainHealth(attacker, combatType, damage);
	char buffer[150];
	if(attacker)
		sprintf(buffer, "You lose %d hitpoint%s due to an attack by %s.", damage, (damage != 1 ? "s" : ""), attacker->getNameDescription().c_str());
	else
		sprintf(buffer, "You lose %d hitpoint%s.", damage, (damage != 1 ? "s" : ""));

	sendStats();
	sendTextMessage(MSG_EVENT_DEFAULT, buffer);
}

void Player::drainMana(Creature* attacker, int32_t manaLoss)
{
	Creature::drainMana(attacker, manaLoss);
	char buffer[150];
	if(attacker)
		sprintf(buffer, "You lose %d mana blocking an attack by %s.", manaLoss, attacker->getNameDescription().c_str());
	else
		sprintf(buffer, "You lose %d mana.", manaLoss);

	sendStats();
	sendTextMessage(MSG_EVENT_DEFAULT, buffer);
}

void Player::addManaSpent(uint64_t amount, bool ignoreFlag/* = false*/, bool useMultiplier/* = true*/)
{
	if(!amount || (!ignoreFlag && hasFlag(PlayerFlag_NotGainMana)))
		return;

	uint64_t currReqMana = vocation->getReqMana(magLevel), nextReqMana = vocation->getReqMana(magLevel + 1);
	if(currReqMana > nextReqMana)
	{
		//player has reached max magic level
		return;
	}

	if(useMultiplier)
		amount = uint64_t((double)amount * rates[SKILL__MAGLEVEL]);

	amount = uint64_t((double)amount * g_config.getDouble(ConfigManager::RATE_MAGIC));
	while(manaSpent + amount >= nextReqMana)
	{
		amount -= nextReqMana - manaSpent;
		manaSpent = 0;
		magLevel++;
		char advMsg[50];
		sprintf(advMsg, "You advanced to magic level %d.", magLevel);
		sendTextMessage(MSG_EVENT_ADVANCE, advMsg);

		CreatureEventList advanceEvents = getCreatureEvents(CREATURE_EVENT_ADVANCE);
		for(CreatureEventList::iterator it = advanceEvents.begin(); it != advanceEvents.end(); ++it)
			(*it)->executeAdvance(this, SKILL__MAGLEVEL, (magLevel - 1), magLevel);

		currReqMana = nextReqMana;
		nextReqMana = vocation->getReqMana(magLevel + 1);
		if(currReqMana > nextReqMana)
		{
			amount = 0;
			break;
		}
	}

	manaSpent += amount;
	if(nextReqMana > currReqMana)
		magLevelPercent = Player::getPercentLevel(manaSpent, nextReqMana);
	else
		magLevelPercent = 0;

	sendStats();
}

void Player::addExperience(uint64_t exp)
{
	uint32_t prevLevel = level;
	uint64_t nextLevelExp = Player::getExpForLevel(level + 1);
	if(Player::getExpForLevel(level) > nextLevelExp)
	{
		//player has reached max level
		levelPercent = 0;
		sendStats();
		return;
	}

	experience += exp;
	while(experience >= nextLevelExp)
	{
		++level;
		healthMax += vocation->getHealthGain();
		health += vocation->getHealthGain();
		manaMax += vocation->getManaGain();
		mana += vocation->getManaGain();
		capacity += vocation->getCapGain();

		nextLevelExp = Player::getExpForLevel(level + 1);
		if(Player::getExpForLevel(level) > nextLevelExp) //player has reached max level
			break;
	}

	if(prevLevel != level)
	{
		updateBaseSpeed();
		setBaseSpeed(getBaseSpeed());

		g_game.changeSpeed(this, 0);
		g_game.addCreatureHealth(this);
		if(getParty())
			getParty()->updateSharedExperience();

		char advMsg[60];
		sprintf(advMsg, "You advanced from Level %d to Level %d.", prevLevel, level);
		sendTextMessage(MSG_EVENT_ADVANCE, advMsg);

		CreatureEventList advanceEvents = getCreatureEvents(CREATURE_EVENT_ADVANCE);
		for(CreatureEventList::iterator it = advanceEvents.begin(); it != advanceEvents.end(); ++it)
			(*it)->executeAdvance(this, SKILL__LEVEL, prevLevel, level);
	}

	uint64_t currLevelExp = Player::getExpForLevel(level);
	nextLevelExp = Player::getExpForLevel(level + 1);
	if(nextLevelExp > currLevelExp)
		levelPercent = Player::getPercentLevel(experience - currLevelExp, nextLevelExp - currLevelExp);
	else
		levelPercent = 0;

	sendStats();
}

void Player::removeExperience(uint64_t exp, bool updateStats/* = true*/)
{
	uint32_t prevLevel = level;
	experience -= std::min(exp, experience);
	while(level > 1 && experience < Player::getExpForLevel(level))
	{
		level--;
		healthMax = std::max((int32_t)0, (healthMax - (int32_t)vocation->getHealthGain()));
		manaMax = std::max((int32_t)0, (manaMax - (int32_t)vocation->getManaGain()));
		capacity = std::max((double)0, (capacity - (double)vocation->getCapGain()));
	}

	if(prevLevel != level)
	{
		if(updateStats)
		{
			updateBaseSpeed();
			setBaseSpeed(getBaseSpeed());

			g_game.changeSpeed(this, 0);
			g_game.addCreatureHealth(this);
		}

		char advMsg[90];
		sprintf(advMsg, "You were downgraded from Level %d to Level %d.", prevLevel, level);
		sendTextMessage(MSG_EVENT_ADVANCE, advMsg);
	}

	uint64_t currLevelExp = Player::getExpForLevel(level);
	uint64_t nextLevelExp = Player::getExpForLevel(level + 1);
	if(nextLevelExp > currLevelExp)
		levelPercent = Player::getPercentLevel(experience - currLevelExp, nextLevelExp - currLevelExp);
	else
		levelPercent = 0;

	if(updateStats)
		sendStats();
}

uint32_t Player::getPercentLevel(uint64_t count, uint64_t nextLevelCount)
{
	if(nextLevelCount > 0)
		return std::min((uint32_t)100, std::max((uint32_t)0, uint32_t(count * 100 / nextLevelCount)));

	return 0;
}

void Player::onBlockHit(BlockType_t blockType)
{
	if(shieldBlockCount > 0)
	{
		--shieldBlockCount;
		if(hasShield())
			addSkillAdvance(SKILL_SHIELD, 1);
	}
}

void Player::onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType)
{
	Creature::onAttackedCreatureBlockHit(target, blockType);
	lastAttackBlockType = blockType;
	switch(blockType)
	{
		case BLOCK_NONE:
		{
			addAttackSkillPoint = true;
			bloodHitCount = 30;
			shieldBlockCount = 30;
			break;
		}

		case BLOCK_DEFENSE:
		case BLOCK_ARMOR:
		{
			//need to draw blood every 30 hits
			if(bloodHitCount > 0)
			{
				addAttackSkillPoint = true;
				--bloodHitCount;
			}
			else
				addAttackSkillPoint = false;

			break;
		}

		default:
		{
			addAttackSkillPoint = false;
			break;
		}
	}
}

bool Player::hasShield() const
{
	bool result = false;
	Item* item = getInventoryItem(SLOT_LEFT);
	if(item && item->getWeaponType() == WEAPON_SHIELD)
		result = true;

	item = getInventoryItem(SLOT_RIGHT);
	if(item && item->getWeaponType() == WEAPON_SHIELD)
		result = true;

	return result;
}

BlockType_t Player::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
	bool checkDefense /* = false*/, bool checkArmor /* = false*/)
{
	BlockType_t blockType = Creature::blockHit(attacker, combatType, damage, checkDefense, checkArmor);
	if(attacker)
		sendCreatureSquare(attacker, SQ_COLOR_BLACK);

	if(blockType != BLOCK_NONE)
		return blockType;

	if(damage > 0)
	{
		int32_t blocked = 0;
		for(int32_t slot = SLOT_FIRST; slot < SLOT_LAST; ++slot)
		{
			if(!isItemAbilityEnabled((slots_t)slot))
				continue;

			Item* item = getInventoryItem((slots_t)slot);
			if(!item)
				continue;

			const ItemType& it = Item::items[item->getID()];
			if(it.abilities.absorbPercent[combatType] != 0)
			{
				blocked += (int32_t)std::ceil((float)damage * it.abilities.absorbPercent[combatType] / 100);
				if(item->hasCharges())
					g_game.transformItem(item, item->getID(), std::max((int32_t)0, (int32_t)item->getCharges() - 1));
			}
		}

		if(vocation->getAbsorbPercent(combatType) != 0)
			blocked += (int32_t)std::ceil((float)damage * vocation->getAbsorbPercent(combatType) / 100);

		damage -= blocked;
		if(damage <= 0)
		{
			damage = 0;
			blockType = BLOCK_DEFENSE;
		}
	}

	return blockType;
}

uint32_t Player::getIP() const
{
	if(client)
		return client->getIP();

	return 0;
}

bool Player::onDeath()
{
	Item* preventLoss = NULL;
	Item* preventDrop = NULL;
	if(getZone() == ZONE_PVP)
	{
		setDropLoot(LOOT_DROP_NONE);
		setLossSkill(false);
	}
	else if(getSkull() != SKULL_RED && g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
	{
		for(uint8_t i = SLOT_FIRST; ((skillLoss || lootDrop == LOOT_DROP_FULL) && i < SLOT_LAST); ++i)
		{
			if(Item* item = getInventoryItem((slots_t)i))
			{
				const ItemType& it = Item::items[item->getID()];
				if(lootDrop == LOOT_DROP_FULL && it.abilities.preventDrop)
				{
					setDropLoot(LOOT_DROP_PREVENT);
					preventDrop = item;
				}

				if(skillLoss && !preventLoss && it.abilities.preventLoss)
					preventLoss = item;
			}
		}
	}

	if(!Creature::onDeath())
	{
		if(preventDrop)
			setDropLoot(LOOT_DROP_FULL);

		return false;
	}

	if(preventLoss)
	{
		setLossSkill(false);
		if(preventLoss->getCharges() > 1) //weird, but transform failed to remove for some hosters
			g_game.transformItem(preventLoss, preventLoss->getID(), std::max(0, ((int32_t)preventLoss->getCharges() - 1)));
		else
			g_game.internalRemoveItem(NULL, preventDrop);
	}

	if(preventDrop && preventDrop != preventLoss)
	{
		if(preventDrop->getCharges() > 1) //weird, but transform failed to remve for some hosters
			g_game.transformItem(preventDrop, preventDrop->getID(), std::max(0, ((int32_t)preventDrop->getCharges() - 1)));
		else
			g_game.internalRemoveItem(NULL, preventDrop);
	}

	removeConditions(CONDITIONEND_DEATH);
	if(skillLoss)
	{
		uint64_t lossExperience = getLostExperience();
		removeExperience(lossExperience, false);
		float percent = 1.0f - ((float)(experience - lossExperience) / (float)experience);

		//Magic level loss
		uint32_t sumMana = 0;
		uint64_t lostMana = 0;
		for(uint32_t i = 1; i <= magLevel; ++i)
			sumMana += vocation->getReqMana(i);

		sumMana += manaSpent;
		lostMana = (uint64_t)std::ceil(sumMana * (percent * (float)lossPercent[LOSS_MANA] / 100.0f));
		while(lostMana > manaSpent && magLevel > 0)
		{
			lostMana -= manaSpent;
			manaSpent = vocation->getReqMana(magLevel);
			magLevel--;
		}

		manaSpent -= std::max((int32_t)0, (int32_t)lostMana);
		uint64_t nextReqMana = vocation->getReqMana(magLevel + 1);
		if(nextReqMana > vocation->getReqMana(magLevel))
			magLevelPercent = Player::getPercentLevel(manaSpent, nextReqMana);
		else
			magLevelPercent = 0;

		//Skill loss
		uint32_t lostSkillTries, sumSkillTries;
		for(int16_t i = 0; i < 7; ++i) //for each skill
		{
			lostSkillTries = sumSkillTries = 0;
			for(uint32_t c = 11; c <= skills[i][SKILL_LEVEL]; ++c) //sum up all required tries for all skill levels
				sumSkillTries += vocation->getReqSkillTries(i, c);

			sumSkillTries += skills[i][SKILL_TRIES];
			lostSkillTries = (uint32_t)std::ceil(sumSkillTries * (percent * (float)lossPercent[LOSS_SKILLS] / 100.0f));
			while(lostSkillTries > skills[i][SKILL_TRIES])
			{
				lostSkillTries -= skills[i][SKILL_TRIES];
				skills[i][SKILL_TRIES] = vocation->getReqSkillTries(i, skills[i][SKILL_LEVEL]);
				if(skills[i][SKILL_LEVEL] < 11)
				{
					skills[i][SKILL_LEVEL] = 10;
					skills[i][SKILL_TRIES] = lostSkillTries = 0;
					break;
				}
				else
					skills[i][SKILL_LEVEL]--;
			}

			skills[i][SKILL_TRIES] = std::max((int32_t)0, (int32_t)(skills[i][SKILL_TRIES] - lostSkillTries));
		}

		blessings = 0;
		loginPosition = masterPos;
		if(!inventory[SLOT_BACKPACK])
			__internalAddThing(SLOT_BACKPACK, Item::CreateItem(1987));

		sendStats();
		sendSkills();

		sendReLoginWindow();
		g_game.removeCreature(this, false);
	}
	else
	{
		setLossSkill(true);
		if(preventLoss)
		{
			sendReLoginWindow();
			g_game.removeCreature(this, false);
		}
	}

	return true;
}

void Player::dropCorpse()
{
	if(lootDrop == LOOT_DROP_NONE)
	{
		setDropLoot(LOOT_DROP_FULL);
		onIdleStatus();
		if(health <= 0)
		{
			health = healthMax;
			mana = manaMax;
		}

		sendStats();
		onThink(EVENT_CREATURE_THINK_INTERVAL);
		g_game.internalTeleport(this, getTemplePosition(), true);
	}
	else
		Creature::dropCorpse();
}

Item* Player::getCorpse()
{
	Item* corpse = Creature::getCorpse();
	if(corpse && corpse->getContainer())
	{
		std::stringstream ss;
		ss << "You recognize " << getNameDescription();
		if(lastHitCreature || mostDamageCreature)
			ss << ". " << (getSex() == PLAYERSEX_FEMALE ? "She" : "He") << " was killed by ";

		Creature* lastHitCreatureMaster = NULL;
		Creature* mostDamageCreatureMaster = NULL;
		if(lastHitCreature)
		{
			ss << lastHitCreature->getNameDescription();
			lastHitCreatureMaster = lastHitCreature->getMaster();
		}

		if(mostDamageCreature)
		{
			mostDamageCreatureMaster = mostDamageCreature->getMaster();
			bool isNotLastHitMaster = mostDamageCreature != lastHitCreatureMaster;
			bool isNotMostDamageMaster = lastHitCreature != mostDamageCreatureMaster;
			bool isNotSameMaster = !lastHitCreatureMaster || (mostDamageCreatureMaster != lastHitCreatureMaster);
			bool isNotSameName = lastHitCreature && lastHitCreature->getName() != mostDamageCreature->getName();
			if(mostDamageCreature != lastHitCreature && isNotLastHitMaster && isNotMostDamageMaster && isNotSameMaster && isNotSameName)
			{
				if(lastHitCreature)
					ss << " and by ";

				ss << mostDamageCreature->getNameDescription();
			}
		}

		ss << ".";
		corpse->setSpecialDescription(ss.str().c_str());
	}

	return corpse;
}

void Player::addExhaust(uint32_t ticks, uint32_t type)
{
	if(Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_EXHAUST, ticks, 0, false, type))
		addCondition(condition);
}

void Player::addInFightTicks(bool pzLock/* = false*/)
{
	if(hasFlag(PlayerFlag_NotGainInFight))
		return;

	if(Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_INFIGHT, g_game.getInFightTicks()))
		addCondition(condition);

	if(pzLock)
		pzLocked = true;
}

void Player::addDefaultRegeneration(uint32_t addTicks)
{
	Condition* condition = getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition)
		condition->setTicks(condition->getTicks() + addTicks);
	else if((condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_REGENERATION, addTicks)))
	{
		condition->setParam(CONDITIONPARAM_HEALTHGAIN, vocation->getHealthGainAmount());
		condition->setParam(CONDITIONPARAM_HEALTHTICKS, vocation->getHealthGainTicks() * 1000);
		condition->setParam(CONDITIONPARAM_MANAGAIN, vocation->getManaGainAmount());
		condition->setParam(CONDITIONPARAM_MANATICKS, vocation->getManaGainTicks() * 1000);
		addCondition(condition);
	}
}

void Player::removeList()
{
	Status::getInstance()->removePlayer();
	listPlayer.removeList(getID());
	if(!isInGhostMode())
	{
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
			(*it).second->notifyLogOut(this);
	}
}

void Player::addList()
{
	if(!isInGhostMode())
	{
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
			(*it).second->notifyLogIn(this);
	}

	listPlayer.addList(this);
	Status::getInstance()->addPlayer();
}

void Player::kickPlayer(bool displayEffect)
{
	if(!client)
	{
		g_creatureEvents->playerLogout(this);
		g_game.removeCreature(this);
	}
	else
		client->logout(displayEffect, true);
}

void Player::notifyLogIn(Player* login_player)
{
	if(client)
	{
		VIPListSet::iterator it = VIPList.find(login_player->getGUID());
		if(it != VIPList.end())
		{
			client->sendVIPLogIn(login_player->getGUID());
			client->sendTextMessage(MSG_STATUS_SMALL, (login_player->getName() + " has logged in."));
		}
	}
}

void Player::notifyLogOut(Player* logout_player)
{
	if(client)
	{
		VIPListSet::iterator it = VIPList.find(logout_player->getGUID());
		if(it != VIPList.end())
		{
			client->sendVIPLogOut(logout_player->getGUID());
			client->sendTextMessage(MSG_STATUS_SMALL, (logout_player->getName() + " has logged out."));
		}
	}
}

bool Player::removeVIP(uint32_t _guid)
{
	VIPListSet::iterator it = VIPList.find(_guid);
	if(it == VIPList.end())
		return false;

	VIPList.erase(it);
	return true;
}

bool Player::addVIP(uint32_t _guid, std::string& name, bool isOnline, bool internal /*=false*/)
{
	if(guid == _guid)
	{
		if(!internal)
			sendTextMessage(MSG_STATUS_SMALL, "You cannot add yourself.");

		return false;
	}

	if(VIPList.size() > maxVipLimit)
	{
		if(!internal)
			sendTextMessage(MSG_STATUS_SMALL, "You cannot add more buddies.");

		return false;
	}

	VIPListSet::iterator it = VIPList.find(_guid);
	if(it != VIPList.end())
	{
		if(!internal)
			sendTextMessage(MSG_STATUS_SMALL, "This player is already in your list.");

		return false;
	}

	VIPList.insert(_guid);
	if(client && !internal)
		client->sendVIP(_guid, name, isOnline);

	return true;
}

//close container and its child containers
void Player::autoCloseContainers(const Container* container)
{
	typedef std::vector<uint32_t> CloseList;
	CloseList closeList;
	for(ContainerVector::iterator it = containerVec.begin(); it != containerVec.end(); ++it)
	{
		Container* tmp = it->second;
		while(tmp != NULL)
		{
			if(tmp->isRemoved() || tmp == container)
			{
				closeList.push_back(it->first);
				break;
			}

			tmp = dynamic_cast<Container*>(tmp->getParent());
		}
	}

	for(CloseList::iterator it = closeList.begin(); it != closeList.end(); ++it)
	{
		closeContainer(*it);
		if(client)
			client->sendCloseContainer(*it);
	}
}

bool Player::hasCapacity(const Item* item, uint32_t count) const
{
	if(hasFlag(PlayerFlag_CannotPickupItem))
		return false;

	if(!hasFlag(PlayerFlag_HasInfiniteCapacity) && item->getTopParent() != this)
	{
		double itemWeight = 0;
		if(item->isStackable())
			itemWeight = Item::items[item->getID()].weight * count;
		else
			itemWeight = item->getWeight();

		return (itemWeight < getFreeCapacity());
	}

	return true;
}

ReturnValue Player::__queryAdd(int32_t index, const Thing* thing, uint32_t count, uint32_t flags) const
{
	const Item* item = thing->getItem();
	if(item == NULL)
		return RET_NOTPOSSIBLE;

	bool childIsOwner = ((flags & FLAG_CHILDISOWNER) == FLAG_CHILDISOWNER), skipLimit = ((flags & FLAG_NOLIMIT) == FLAG_NOLIMIT);
	if(childIsOwner)
	{
		//a child container is querying the player, just check if enough capacity
		if(skipLimit || hasCapacity(item, count))
			return RET_NOERROR;

		return RET_NOTENOUGHCAPACITY;
	}

	if(!item->isPickupable())
		return RET_CANNOTPICKUP;

	ReturnValue ret = RET_NOERROR;
	if((item->getSlotPosition() & SLOTP_HEAD) || (item->getSlotPosition() & SLOTP_NECKLACE) ||
		(item->getSlotPosition() & SLOTP_BACKPACK) || (item->getSlotPosition() & SLOTP_ARMOR) ||
		(item->getSlotPosition() & SLOTP_LEGS) || (item->getSlotPosition() & SLOTP_FEET) ||
		(item->getSlotPosition() & SLOTP_RING))
		ret = RET_CANNOTBEDRESSED;
	else if(item->getSlotPosition() & SLOTP_TWO_HAND)
		ret = RET_PUTTHISOBJECTINBOTHHANDS;
	else if((item->getSlotPosition() & SLOTP_RIGHT) || (item->getSlotPosition() & SLOTP_LEFT))
		ret = RET_PUTTHISOBJECTINYOURHAND;

	switch(index)
	{
		case SLOT_HEAD:
			if(item->getSlotPosition() & SLOTP_HEAD)
				ret = RET_NOERROR;
			break;
		case SLOT_NECKLACE:
			if(item->getSlotPosition() & SLOTP_NECKLACE)
				ret = RET_NOERROR;
			break;
		case SLOT_BACKPACK:
			if(item->getSlotPosition() & SLOTP_BACKPACK)
				ret = RET_NOERROR;
			break;
		case SLOT_ARMOR:
			if(item->getSlotPosition() & SLOTP_ARMOR)
				ret = RET_NOERROR;
			break;
		case SLOT_RIGHT:
			if(item->getSlotPosition() & SLOTP_RIGHT)
			{
				//check if we already carry an item in the other hand
				if(item->getSlotPosition() & SLOTP_TWO_HAND)
				{
					if(inventory[SLOT_LEFT] && inventory[SLOT_LEFT] != item)
						ret = RET_BOTHHANDSNEEDTOBEFREE;
					else
						ret = RET_NOERROR;
				}
				else if(inventory[SLOT_LEFT])
				{
					const Item* leftItem = inventory[SLOT_LEFT];
					WeaponType_t type = item->getWeaponType(), leftType = leftItem->getWeaponType();
					if(leftItem->getSlotPosition() & SLOTP_TWO_HAND)
						ret = RET_DROPTWOHANDEDITEM;
					else if(item == leftItem && count == item->getItemCount())
						ret = RET_NOERROR;
					else if(leftType == WEAPON_SHIELD && type == WEAPON_SHIELD)
						ret = RET_CANONLYUSEONESHIELD;
					else if(!leftItem->isWeapon() || !item->isWeapon() ||
						leftType == WEAPON_SHIELD || leftType == WEAPON_AMMO
						|| type == WEAPON_SHIELD || type == WEAPON_AMMO)
						ret = RET_NOERROR;
					else
						ret = RET_CANONLYUSEONEWEAPON;
				}
				else
					ret = RET_NOERROR;
			}
			break;
		case SLOT_LEFT:
			if(item->getSlotPosition() & SLOTP_LEFT)
			{
				//check if we already carry an item in the other hand
				if(item->getSlotPosition() & SLOTP_TWO_HAND)
				{
					if(inventory[SLOT_RIGHT] && inventory[SLOT_RIGHT] != item)
						ret = RET_BOTHHANDSNEEDTOBEFREE;
					else
						ret = RET_NOERROR;
				}
				else if(inventory[SLOT_RIGHT])
				{
					const Item* rightItem = inventory[SLOT_RIGHT];
					WeaponType_t type = item->getWeaponType(), rightType = rightItem->getWeaponType();
					if(rightItem->getSlotPosition() & SLOTP_TWO_HAND)
						ret = RET_DROPTWOHANDEDITEM;
					else if(item == rightItem && count == item->getItemCount())
						ret = RET_NOERROR;
					else if(rightType == WEAPON_SHIELD && type == WEAPON_SHIELD)
						ret = RET_CANONLYUSEONESHIELD;
					else if(!rightItem->isWeapon() || !item->isWeapon() ||
						rightType == WEAPON_SHIELD || rightType == WEAPON_AMMO
						|| type == WEAPON_SHIELD || type == WEAPON_AMMO)
						ret = RET_NOERROR;
					else
						ret = RET_CANONLYUSEONEWEAPON;
				}
				else
					ret = RET_NOERROR;
			}
			break;
		case SLOT_LEGS:
			if(item->getSlotPosition() & SLOTP_LEGS)
				ret = RET_NOERROR;
			break;
		case SLOT_FEET:
			if(item->getSlotPosition() & SLOTP_FEET)
				ret = RET_NOERROR;
			break;
		case SLOT_RING:
			if(item->getSlotPosition() & SLOTP_RING)
				ret = RET_NOERROR;
			break;
		case SLOT_AMMO:
			if(item->getSlotPosition() & SLOTP_AMMO)
				ret = RET_NOERROR;
			break;
		case SLOT_WHEREEVER:
		case -1:
			ret = RET_NOTENOUGHROOM;
			break;
		default:
			ret = RET_NOTPOSSIBLE;
			break;
	}

	if(ret == RET_NOERROR || ret == RET_NOTENOUGHROOM)
	{
		//need an exchange with source?
		if(getInventoryItem((slots_t)index) != NULL && (!getInventoryItem((slots_t)index)->isStackable()
			|| getInventoryItem((slots_t)index)->getID() != item->getID()))
			return RET_NEEDEXCHANGE;

		if(!g_moveEvents->onPlayerEquip(const_cast<Player*>(this), const_cast<Item*>(item), (slots_t)index, true))
			return RET_CANNOTBEDRESSED;

		//check if enough capacity
		if(!hasCapacity(item, count))
			return RET_NOTENOUGHCAPACITY;
	}

	return ret;
}

ReturnValue Player::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
	uint32_t flags) const
{
	const Item* item = thing->getItem();
	if(item == NULL)
	{
		maxQueryCount = 0;
		return RET_NOTPOSSIBLE;
	}

	const Thing* destThing = __getThing(index);
	const Item* destItem = NULL;
	if(destThing)
		destItem = destThing->getItem();

	if(destItem)
	{
		if(destItem->isStackable() && item->getID() == destItem->getID())
			maxQueryCount = 100 - destItem->getItemCount();
		else
			maxQueryCount = 0;
	}
	else
	{
		if(item->isStackable())
			maxQueryCount = 100;
		else
			maxQueryCount = 1;

		return RET_NOERROR;
	}

	if(maxQueryCount < count)
		return RET_NOTENOUGHROOM;

	return RET_NOERROR;
}

ReturnValue Player::__queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const
{
	int32_t index = __getIndexOfThing(thing);
	if(index == -1)
		return RET_NOTPOSSIBLE;

	const Item* item = thing->getItem();
	if(item == NULL)
		return RET_NOTPOSSIBLE;

	if(count == 0 || (item->isStackable() && count > item->getItemCount()))
		return RET_NOTPOSSIBLE;

	 if(item->isNotMoveable() && !hasBitSet(FLAG_IGNORENOTMOVEABLE, flags))
		return RET_NOTMOVEABLE;

	return RET_NOERROR;
}

Cylinder* Player::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	if(index == 0 /*drop to capacity window*/ || index == INDEX_WHEREEVER)
	{
		*destItem = NULL;
		const Item* item = thing->getItem();
		if(!item)
			return this;

		//find a appropiate slot
		for(int32_t i = SLOT_FIRST; i < SLOT_LAST; ++i)
		{
			if(!inventory[i] && __queryAdd(i, item, item->getItemCount(), 0) == RET_NOERROR)
			{
				index = i;
				return this;
			}
		}

		//try containers
		std::list<std::pair<Container*, int32_t> > deepList;
		for(int32_t i = SLOT_FIRST; i < SLOT_LAST; ++i)
		{
			if(inventory[i] == tradeItem)
				continue;

			if(Container* container = dynamic_cast<Container*>(inventory[i]))
			{
				if(container->__queryAdd(-1, item, item->getItemCount(), 0) == RET_NOERROR)
				{
					index = INDEX_WHEREEVER;
					*destItem = NULL;
					return container;
				}

				deepList.push_back(std::make_pair(container, 0));
			}
		}

		//check deeper in the containers
		int32_t deepness = g_config.getNumber(ConfigManager::PLAYER_DEEPNESS);
		for(std::list<std::pair<Container*, int32_t> >::iterator dit = deepList.begin(); dit != deepList.end(); ++dit)
		{
			Container* c = (*dit).first;
			if(!c || c->empty())
				continue;

			int32_t level = (*dit).second;
			for(ItemList::const_iterator it = c->getItems(); it != c->getEnd(); ++it)
			{
				if((*it) == tradeItem)
					continue;

				if(Container* subContainer = dynamic_cast<Container*>(*it))
				{
					if(subContainer->__queryAdd(-1, item, item->getItemCount(), 0) == RET_NOERROR)
					{
						index = INDEX_WHEREEVER;
						*destItem = NULL;
						return subContainer;
					}

					if(deepness < 0 || level < deepness)
						deepList.push_back(std::make_pair(subContainer, (level + 1)));
				}
			}
		}

		return this;
	}

	Thing* destThing = __getThing(index);
	if(destThing)
		*destItem = destThing->getItem();

	if(Cylinder* subCylinder = dynamic_cast<Cylinder*>(destThing))
	{
		index = INDEX_WHEREEVER;
		*destItem = NULL;
		return subCylinder;
	}

	return this;
}

void Player::__addThing(Creature* actor, Thing* thing)
{
	__addThing(actor, 0, thing);
}

void Player::__addThing(Creature* actor, int32_t index, Thing* thing)
{
	if(index < 0 || index > 11)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__addThing], " << "player: " << getName() << ", index: " << index << ", index < 0 || index > 11" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	if(index == 0)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__addThing], " << "player: " << getName() << ", index == 0" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTENOUGHROOM*/;
	}

	Item* item = thing->getItem();
	if(item == NULL)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__addThing], " << "player: " << getName() << ", item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	item->setParent(this);
	inventory[index] = item;

	//send to client
	sendAddInventoryItem((slots_t)index, item);

	//event methods
	onAddInventoryItem((slots_t)index, item);
}

void Player::__updateThing(Thing* thing, uint16_t itemId, uint32_t count)
{
	int32_t index = __getIndexOfThing(thing);
	if(index == -1)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__updateThing], " << "player: " << getName() << ", index == -1" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* item = thing->getItem();
	if(item == NULL)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__updateThing], " << "player: " << getName() << ", item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	const ItemType& oldType = Item::items[item->getID()];
	const ItemType& newType = Item::items[itemId];

	item->setID(itemId);
	item->setSubType(count);

	//send to client
	sendUpdateInventoryItem((slots_t)index, item, item);

	//event methods
	onUpdateInventoryItem((slots_t)index, item, oldType, item, newType);
}

void Player::__replaceThing(uint32_t index, Thing* thing)
{
	if(index < 0 || index > 11)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__replaceThing], " << "player: " << getName() << ", index: " << index << ",  index < 0 || index > 11" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* oldItem = getInventoryItem((slots_t)index);
	if(!oldItem)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__updateThing], " << "player: " << getName() << ", oldItem == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* item = thing->getItem();
	if(item == NULL)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__updateThing], " << "player: " << getName() << ", item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	const ItemType& oldType = Item::items[oldItem->getID()];
	const ItemType& newType = Item::items[item->getID()];

	//send to client
	sendUpdateInventoryItem((slots_t)index, oldItem, item);

	//event methods
	onUpdateInventoryItem((slots_t)index, oldItem, oldType, item, newType);

	item->setParent(this);
	inventory[index] = item;
}

void Player::__removeThing(Thing* thing, uint32_t count)
{
	Item* item = thing->getItem();
	if(item == NULL)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__removeThing], " << "player: " << getName() << ", item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	int32_t index = __getIndexOfThing(thing);
	if(index == -1)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__removeThing], " << "player: " << getName() << ", index == -1" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	if(item->isStackable())
	{
		if(count == item->getItemCount())
		{
			//send change to client
			sendRemoveInventoryItem((slots_t)index, item);

			//event methods
			onRemoveInventoryItem((slots_t)index, item);

			item->setParent(NULL);
			inventory[index] = NULL;
		}
		else
		{
			item->setItemCount(std::max(0, (int32_t)(item->getItemCount() - count)));

			const ItemType& it = Item::items[item->getID()];

			//send change to client
			sendUpdateInventoryItem((slots_t)index, item, item);

			//event methods
			onUpdateInventoryItem((slots_t)index, item, it, item, it);
		}
	}
	else
	{
		//send change to client
		sendRemoveInventoryItem((slots_t)index, item);

		//event methods
		onRemoveInventoryItem((slots_t)index, item);

		item->setParent(NULL);
		inventory[index] = NULL;
	}
}

int32_t Player::__getIndexOfThing(const Thing* thing) const
{
	for(int32_t i = SLOT_FIRST; i < SLOT_LAST; ++i)
	{
		if(inventory[i] == thing)
			return i;
	}

	return -1;
}

int32_t Player::__getFirstIndex() const
{
	return SLOT_FIRST;
}

int32_t Player::__getLastIndex() const
{
	return SLOT_LAST;
}

uint32_t Player::__getItemTypeCount(uint16_t itemId, int32_t subType /*= -1*/, bool itemCount /*= true*/) const
{
	uint32_t count = 0;
	std::list<const Container*> listContainer;
	Container* tmpContainer = NULL;

	Item* item = NULL;
	for(int32_t i = SLOT_FIRST; i < SLOT_LAST; i++)
	{
		if((item = inventory[i]))
		{
			if(item->getID() == itemId && (subType == -1 || subType == item->getSubType()))
			{
				if(itemCount)
					count += item->getItemCount();
				else if(item->isRune())
					count += item->getCharges();
				else
					count += item->getItemCount();
			}

			if((tmpContainer = item->getContainer()))
				listContainer.push_back(tmpContainer);
		}
	}

	ItemList::const_iterator cit;
	while(listContainer.size() > 0)
	{
		const Container* container = listContainer.front();
		listContainer.pop_front();

		count += container->__getItemTypeCount(itemId, subType, itemCount);
		for(cit = container->getItems(); cit != container->getEnd(); ++cit)
		{
			if((tmpContainer = (*cit)->getContainer()))
				listContainer.push_back(tmpContainer);
		}
	}

	return count;
}

Thing* Player::__getThing(uint32_t index) const
{
	if(index >= SLOT_FIRST && index < SLOT_LAST)
		return inventory[index];

	return NULL;
}

void Player::postAddNotification(Creature* actor, Thing* thing, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(link == LINK_OWNER) //calling movement scripts
		g_moveEvents->onPlayerEquip(this, thing->getItem(), (slots_t)index, false);

	if(link == LINK_OWNER || link == LINK_TOPPARENT)
	{
		updateItemsLight();
		updateInventoryWeigth();
		sendStats();
	}

	if(const Item* item = thing->getItem())
	{
		if(const Container* container = item->getContainer())
			onSendContainer(container);

		if(shopOwner)
			updateInventoryGoods(item->getID());
	}
	else if(const Creature* creature = thing->getCreature())
	{
		if(creature == this)
		{
			typedef std::vector<Container*> Containers;
			Containers containers;
			for(ContainerVector::iterator it = containerVec.begin(); it != containerVec.end(); ++it)
			{
				if(!Position::areInRange<1,1,0>(it->second->getPosition(), getPosition()))
					containers.push_back(it->second);
			}

			for(Containers::const_iterator it = containers.begin(); it != containers.end(); ++it)
				autoCloseContainers(*it);
		}
	}
}

void Player::postRemoveNotification(Creature* actor, Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(link == LINK_OWNER) //calling movement scripts
		g_moveEvents->onPlayerDeEquip(this, thing->getItem(), (slots_t)index, isCompleteRemoval);

	if(link == LINK_OWNER || link == LINK_TOPPARENT)
	{
		updateItemsLight();
		updateInventoryWeigth();
		sendStats();
	}

	if(const Item* item = thing->getItem())
	{
		if(const Container* container = item->getContainer())
		{
			if(!container->isRemoved() && (container->getTopParent() == this || (dynamic_cast<const Container*>(container->getTopParent())))
				&& Position::areInRange<1,1,0>(getPosition(), container->getPosition()))
				onSendContainer(container);
			else
				autoCloseContainers(container);
		}

		if(shopOwner)
			updateInventoryGoods(item->getID());
	}
}

void Player::__internalAddThing(Thing* thing)
{
	__internalAddThing(0, thing);
}

void Player::__internalAddThing(uint32_t index, Thing* thing)
{
#ifdef __DEBUG__MOVESYS__
	std::cout << "[Player::__internalAddThing] index: " << index << std::endl;
#endif

	Item* item = thing->getItem();
	if(item == NULL)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__internalAddThing] item == NULL" << std::endl;
#endif
		return;
	}

	//index == 0 means we should equip this item at the most appropiate slot
	if(index == 0)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__internalAddThing] index == 0" << std::endl;
		DEBUG_REPORT
#endif
		return;
	}

	if(index > 0 && index < 11)
	{
		if(inventory[index])
		{
#ifdef __DEBUG__MOVESYS__
			std::cout << "Warning: [Player::__internalAddThing], player: " << getName() << ", items[index] is not empty." << std::endl;
			//DEBUG_REPORT
#endif
			return;
		}

		inventory[index] = item;
		item->setParent(this);
	}
}

bool Player::setFollowCreature(Creature* creature, bool fullPathSearch /*= false*/)
{
	if(!Creature::setFollowCreature(creature, fullPathSearch))
	{
		setFollowCreature(NULL);
		setAttackedCreature(NULL);

		sendCancelMessage(RET_THEREISNOWAY);
		sendCancelTarget();
		stopEventWalk();
		return false;
	}

	return true;
}

bool Player::setAttackedCreature(Creature* creature)
{
	if(!Creature::setAttackedCreature(creature))
	{
		sendCancelTarget();
		return false;
	}

	if(chaseMode == CHASEMODE_FOLLOW && creature)
	{
		if(followCreature != creature)
		{
			//chase opponent
			setFollowCreature(creature);
		}
	}
	else
		setFollowCreature(NULL);

	if(creature)
		Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Game::checkCreatureAttack, &g_game, getID())));

	return true;
}

void Player::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	Creature::getPathSearchParams(creature, fpp);
	fpp.fullPathSearch = true;
}

void Player::doAttacking(uint32_t interval)
{
	if(lastAttack == 0)
		lastAttack = OTSYS_TIME() - getAttackSpeed() - 1;

	if((OTSYS_TIME() - lastAttack) < getAttackSpeed())
		return;

	if(hasCondition(CONDITION_PACIFIED) && !hasCustomFlag(PlayerCustomFlag_IgnorePacification))
		return;

	Item* tool = getWeapon();
	if(const Weapon* weapon = g_weapons->getWeapon(tool))
	{
		if(weapon->interruptSwing() && !canDoAction())
		{
			SchedulerTask* task = createSchedulerTask(getNextActionTime(), boost::bind(&Game::checkCreatureAttack, &g_game, getID()));
			setNextActionTask(task);
		}
		else if((!hasCondition(CONDITION_EXHAUST, 3) || !weapon->hasExhaustion()) && weapon->useWeapon(this, tool, attackedCreature))
			lastAttack = OTSYS_TIME();
	}
	else if(Weapon::useFist(this, attackedCreature))
		lastAttack = OTSYS_TIME();
}

uint64_t Player::getGainedExperience(Creature* attacker, bool useMultiplier/* = true*/)
{
	if(!g_config.getBool(ConfigManager::EXPERIENCE_FROM_PLAYERS))
		return 0;

	Player* attackerPlayer = attacker->getPlayer();
	if(attackerPlayer && attackerPlayer != this && skillLoss)
	{
		uint32_t a = (uint32_t)std::floor(attackerPlayer->getLevel() * 0.9);
		if(getLevel() >= a)
		{
			/*
				Formula
				a = attackers level * 0.9
				b = victims level
				c = victims experience

				result = (1 - (a / b)) * 0.05 * c
			*/

			uint32_t b = getLevel();
			uint64_t c = getExperience();

			uint64_t result = std::max((uint64_t)0, (uint64_t)std::floor(getDamageRatio(attacker) * std::max((double)0,
				((double)(1 - (((double)a / b))))) * 0.05 * c));
			if(useMultiplier)
				result = uint64_t((double)result * attackerPlayer->rates[SKILL__LEVEL]);

			return std::min((uint64_t)getLostExperience(), uint64_t(result * g_game.getExperienceStage(attackerPlayer->getLevel())));
		}
	}

	return 0;
}

void Player::onFollowCreature(const Creature* creature)
{
	if(!creature)
		stopEventWalk();
}

void Player::setChaseMode(chaseMode_t mode)
{
	chaseMode_t prevChaseMode = chaseMode;
	chaseMode = mode;

	if(prevChaseMode != chaseMode)
	{
		if(chaseMode == CHASEMODE_FOLLOW)
		{
			if(!followCreature && attackedCreature)
			{
				//chase opponent
				setFollowCreature(attackedCreature);
			}
		}
		else if(attackedCreature)
		{
			setFollowCreature(NULL);
			stopEventWalk();
		}
	}
}

void Player::onWalkAborted()
{
	setNextWalkActionTask(NULL);
	sendCancelWalk();
}

void Player::onWalkComplete()
{
	if(walkTask)
	{
		walkTaskEvent = Scheduler::getScheduler().addEvent(walkTask);
		walkTask = NULL;
	}
}

void Player::stopWalk()
{
	if(!listWalkDir.empty())
	{
		extraStepDuration = getStepDuration();
		stopEventWalk();
	}
}

void Player::getCreatureLight(LightInfo& light) const
{
	if(internalLight.level > itemsLight.level)
		light = internalLight;
	else
		light = itemsLight;
}

void Player::updateItemsLight(bool internal /*=false*/)
{
	LightInfo maxLight;
	LightInfo curLight;
	for(int32_t i = SLOT_FIRST; i < SLOT_LAST; ++i)
	{
		Item* item = getInventoryItem((slots_t)i);
		if(item)
		{
			item->getLight(curLight);
			if(curLight.level > maxLight.level)
				maxLight = curLight;
		}
	}
	if(itemsLight.level != maxLight.level || itemsLight.color != maxLight.color)
	{
		itemsLight = maxLight;
		if(!internal)
			g_game.changeLight(this);
	}
}

void Player::onAddCondition(ConditionType_t type)
{
	Creature::onAddCondition(type);
	sendIcons();
}

void Player::onAddCombatCondition(ConditionType_t type)
{
	switch(type)
	{
		case CONDITION_POISON:
			sendTextMessage(MSG_STATUS_DEFAULT, "You are poisoned.");
			break;
		case CONDITION_DROWN:
			sendTextMessage(MSG_STATUS_DEFAULT, "You are drowning.");
			break;
		case CONDITION_PARALYZE:
			sendTextMessage(MSG_STATUS_DEFAULT, "You are paralyzed.");
			break;
		case CONDITION_DRUNK:
			sendTextMessage(MSG_STATUS_DEFAULT, "You are drunk.");
			break;
		case CONDITION_CURSED:
			sendTextMessage(MSG_STATUS_DEFAULT, "You are cursed.");
			break;
		case CONDITION_FREEZING:
			sendTextMessage(MSG_STATUS_DEFAULT, "You are freezing.");
			break;
		case CONDITION_DAZZLED:
			sendTextMessage(MSG_STATUS_DEFAULT, "You are dazzled.");
			break;
		default:
			break;
	}
}

void Player::onEndCondition(ConditionType_t type)
{
	Creature::onEndCondition(type);
	sendIcons();
	if(type == CONDITION_INFIGHT)
	{
		onIdleStatus();
		pzLocked = false;
		if(getSkull() != SKULL_RED)
		{
			clearAttacked();
			setSkull(SKULL_NONE);
			g_game.updateCreatureSkull(this);
		}
	}
}

void Player::onCombatRemoveCondition(const Creature* attacker, Condition* condition)
{
	//Creature::onCombatRemoveCondition(attacker, condition);
	bool remove = true;
	if(condition->getId() != 0)
	{
		remove = false;
		//Means the condition is from an item, id == slot
		if(g_game.getWorldType() == WORLD_TYPE_PVP_ENFORCED)
		{
			if(Item* item = getInventoryItem((slots_t)condition->getId()))
			{
				//25% chance to destroy the item
				if(25 >= random_range(0, 100))
					g_game.internalRemoveItem(NULL, item);
			}
		}
	}

	if(remove)
	{
		if(!canDoAction())
		{
			uint32_t delay = getNextActionTime();
			delay -= (delay % EVENT_CREATURE_THINK_INTERVAL);
			condition->setTicks(delay);
		}
		else
			removeCondition(condition);
	}
}

void Player::onAttackedCreature(Creature* target)
{
	Creature::onAttackedCreature(target);
	if(target == this)
		return;

	if(!hasFlag(PlayerFlag_NotGainInFight))
	{
		addInFightTicks();
		if(Player* targetPlayer = target->getPlayer())
		{
			pzLocked = true;
			if(!isPartner(targetPlayer) && !Combat::isInPvpZone(this, targetPlayer) && !targetPlayer->hasAttacked(this))
			{
				addAttacked(targetPlayer);
				if(targetPlayer->getSkull() == SKULL_NONE && getSkull() == SKULL_NONE && !hasCustomFlag(PlayerCustomFlag_NotGainSkull))
				{
					setSkull(SKULL_WHITE);
					g_game.updateCreatureSkull(this);
				}

				if(getSkull() == SKULL_NONE)
					targetPlayer->sendCreatureSkull(this);
			}
		}
	}
}

void Player::onAttacked()
{
	Creature::onAttacked();
	addInFightTicks();
}

bool Player::checkLoginDelay(uint32_t playerId) const
{
	return (OTSYS_TIME() <= (lastLogin + g_config.getNumber(ConfigManager::LOGIN_PROTECTION)) && !hasBeenAttacked(playerId));
}

void Player::onIdleStatus()
{
	Creature::onIdleStatus();
	if(getParty())
		getParty()->clearPlayerPoints(this);
}

void Player::onPlacedCreature()
{
	//scripting event - onLogin
	if(!g_creatureEvents->playerLogin(this))
		kickPlayer(true);
}

void Player::onRemovedCreature()
{
	//
}

void Player::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	Creature::onAttackedCreatureDrainHealth(target, points);
	if(getParty() && target && !target->getPlayer() && (!target->getMaster() || !target->getMaster()->getPlayer()))
	{
		Monster* tmpMonster = target->getMonster();
		if(tmpMonster && tmpMonster->isHostile()) //We have fulfilled a requirement for shared experience
			getParty()->addPlayerDamageMonster(this, points);
	}
}

void Player::onTargetCreatureGainHealth(Creature* target, int32_t points)
{
	Creature::onTargetCreatureGainHealth(target, points);
	if(target && getParty())
	{
		Player* tmpPlayer = NULL;
		if(target->getPlayer())
			tmpPlayer = target->getPlayer();
		else if(target->getMaster() && target->getMaster()->getPlayer())
			tmpPlayer = target->getMaster()->getPlayer();

		if(isPartner(tmpPlayer))
			getParty()->addPlayerHealedMember(this, points);
	}
}

bool Player::onKilledCreature(Creature* target)
{
	if(hasFlag(PlayerFlag_NotGenerateLoot))
		target->setDropLoot(LOOT_DROP_NONE);

	if(!Creature::onKilledCreature(target))
		return false;

	Player* targetPlayer = target->getPlayer();
	if(!hasFlag(PlayerFlag_NotGainInFight) && targetPlayer && !Combat::isInPvpZone(this, targetPlayer))
	{
		if(!isPartner(targetPlayer) && !targetPlayer->hasAttacked(this) && targetPlayer->getSkull() == SKULL_NONE)
			addUnjustifiedDead(targetPlayer);

		if(hasCondition(CONDITION_INFIGHT))
		{
			pzLocked = true;
			if(Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_INFIGHT,
				g_config.getNumber(ConfigManager::WHITE_SKULL_TIME)))
				addCondition(condition);
		}
	}

	return true;
}

void Player::gainExperience(uint64_t gainExp)
{
	if(!hasFlag(PlayerFlag_NotGainExperience) && gainExp > 0)
	{
		//soul regeneration
		if(gainExp >= getLevel())
		{
			if(Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_SOUL, 4 * 60 * 1000))
			{
				condition->setParam(CONDITIONPARAM_SOULGAIN, 1);
				condition->setParam(CONDITIONPARAM_SOULTICKS, vocation->getSoulGainTicks() * 1000);
				addCondition(condition);
			}
		}

		addExperience(gainExp);
	}
}

void Player::onGainExperience(uint64_t gainExp)
{
	if(hasFlag(PlayerFlag_NotGainExperience))
		gainExp = 0;

	Party* party = getParty();
	if(party && party->isSharedExperienceActive() && party->isSharedExperienceEnabled())
	{
		party->shareExperience(gainExp);
		//We will get a share of the experience through the sharing mechanism
		gainExp = 0;
	}

	Creature::onGainExperience(gainExp);
	gainExperience(gainExp);
}

void Player::onGainSharedExperience(uint64_t gainExp)
{
	Creature::onGainSharedExperience(gainExp);
	gainExperience(gainExp);
}

bool Player::isImmune(CombatType_t type) const
{
	if(hasCustomFlag(PlayerCustomFlag_IsImmune))
		return true;

	return Creature::isImmune(type);
}

bool Player::isImmune(ConditionType_t type) const
{
	if(hasCustomFlag(PlayerCustomFlag_IsImmune))
		return true;

	return Creature::isImmune(type);
}

bool Player::isAttackable() const
{
	return (!hasFlag(PlayerFlag_CannotBeAttacked) && !isAccountManager());
}

void Player::changeHealth(int32_t healthChange)
{
	Creature::changeHealth(healthChange);
	sendStats();
}

void Player::changeMana(int32_t manaChange)
{
	if(!hasFlag(PlayerFlag_HasInfiniteMana))
		Creature::changeMana(manaChange);

	sendStats();
}

void Player::changeSoul(int32_t soulChange)
{
	if(!hasFlag(PlayerFlag_HasInfiniteSoul))
		soul = std::max((int32_t)0, std::min((int32_t)soulMax, (int32_t)soul + soulChange));

	sendStats();
}

const OutfitListType& Player::getPlayerOutfits()
{
	return m_playerOutfits.getOutfits();
}

bool Player::canWear(uint32_t _looktype, uint32_t _addons)
{
	return m_playerOutfits.isInList(getID(), _looktype, _addons);
}

bool Player::canLogout()
{
	return !isConnecting && !hasCondition(CONDITION_INFIGHT) && !getTile()->hasFlag(TILESTATE_NOLOGOUT);
}

void Player::genReservedStorageRange()
{
	uint32_t baseKey = PSTRG_OUTFITS_RANGE_START + 1;
	const OutfitListType& outfits = m_playerOutfits.getOutfits();

	const OutfitList& globalOutfits = Outfits::getInstance()->getOutfitList(sex);
	for(OutfitListType::const_iterator it = outfits.begin(); it != outfits.end(); ++it)
	{
		if(!globalOutfits.isInList(getID(), (*it)->looktype, (*it)->addons))
		{
			std::stringstream ss;
			ss << (int64_t((*it)->looktype << 16) | ((*it)->addons & 0xFF));
			storageMap[baseKey] = ss.str();

			baseKey++;
			if(baseKey > PSTRG_OUTFITS_RANGE_START + PSTRG_OUTFITS_RANGE_SIZE)
			{
				std::cout << "[Warning - Player::genReservedStorageRange] Player " << getName() << " with more than 500 outfits!." << std::endl;
				break;
			}
		}
	}
}

void Player::addOutfit(uint32_t _looktype, uint32_t _addons)
{
	Outfit outfit;
	outfit.looktype = _looktype;
	outfit.addons = _addons;
	m_playerOutfits.addOutfit(outfit);
}

bool Player::remOutfit(uint32_t _looktype, uint32_t _addons)
{
	Outfit outfit;
	outfit.looktype = _looktype;
	outfit.addons = _addons;
	return m_playerOutfits.remOutfit(outfit);
}

void Player::setSex(PlayerSex_t newSex)
{
	sex = newSex;
	if(Outfits* outfits = Outfits::getInstance())
	{
		const OutfitListType& global_outfits = outfits->getOutfits(sex);
		Outfit outfit;
		for(OutfitListType::const_iterator it = global_outfits.begin(); it != global_outfits.end(); ++it)
		{
			outfit.looktype = (*it)->looktype;
			outfit.addons = (*it)->addons;
			outfit.quest = (*it)->quest;
			outfit.premium = (*it)->premium;
			m_playerOutfits.addOutfit(outfit);
		}
	}
}

Skulls_t Player::getSkull() const
{
	if(hasFlag(PlayerFlag_NotGainInFight) || hasCustomFlag(PlayerCustomFlag_NotGainSkull))
		return SKULL_NONE;

	return skull;
}

Skulls_t Player::getSkullClient(const Creature* creature) const
{
	if(const Player* player = creature->getPlayer())
	{
		if(g_game.getWorldType() != WORLD_TYPE_PVP)
			return SKULL_NONE;

		if(skull != SKULL_NONE && player->getSkull() != SKULL_RED && player->hasAttacked(this))
			return SKULL_YELLOW;

		if(player->getSkull() == SKULL_NONE && isPartner(player) && g_game.getWorldType() != WORLD_TYPE_NO_PVP)
			return SKULL_GREEN;
	}

	return Creature::getSkullClient(creature);
}

bool Player::hasAttacked(const Player* attacked) const
{
	if(hasFlag(PlayerFlag_NotGainInFight) || !attacked)
		return false;

	return attackedSet.find(attacked->getID()) != attackedSet.end();
}

void Player::addAttacked(const Player* attacked)
{
	if(hasFlag(PlayerFlag_NotGainInFight) || !attacked || attacked == this)
		return;

	uint32_t attackedId = attacked->getID();
	if(attackedSet.find(attackedId) == attackedSet.end())
		attackedSet.insert(attackedId);
}

void Player::clearAttacked()
{
	attackedSet.clear();
}

void Player::addUnjustifiedDead(const Player* attacked)
{
	if(g_game.getWorldType() == WORLD_TYPE_PVP_ENFORCED || attacked == this || hasFlag(
		PlayerFlag_NotGainInFight) || hasCustomFlag(PlayerCustomFlag_NotGainSkull))
		return;

	if(client)
	{
		char buffer[90];
		sprintf(buffer, "Warning! The murder of %s was not justified.", attacked->getName().c_str());
		client->sendTextMessage(MSG_STATUS_WARNING, buffer);
	}

	redSkullTicks += g_config.getNumber(ConfigManager::FRAG_TIME);
	if(g_config.getNumber(ConfigManager::KILLS_TO_RED) != 0 && getSkull() != SKULL_RED &&
		redSkullTicks >= ((g_config.getNumber(ConfigManager::KILLS_TO_RED) - 1) * g_config.getNumber(ConfigManager::FRAG_TIME)))
	{
		setSkull(SKULL_RED);
		g_game.updateCreatureSkull(this);
	}
	else if(g_config.getNumber(ConfigManager::KILLS_TO_BAN) != 0 && redSkullTicks >= (g_config.getNumber(
		ConfigManager::KILLS_TO_BAN) - 1) * g_config.getNumber(ConfigManager::FRAG_TIME))
	{
		int32_t warnings = IOLoginData::getInstance()->loadAccount(accountId, true).warnings;
		bool success = false;
		if(warnings >= g_config.getNumber(ConfigManager::WARNINGS_TO_DELETION))
			success = IOBan::getInstance()->addDeletion(accountId, 20, 7, "Unjustified player killing.", 0);
		else if(warnings >= g_config.getNumber(ConfigManager::WARNINGS_TO_FINALBAN))
			success = IOBan::getInstance()->addBanishment(accountId, (time(NULL) + g_config.getNumber(ConfigManager::FINALBAN_LENGTH)), 20, 4, "Unjustified player killing.", 0);
		else
			success = IOBan::getInstance()->addBanishment(accountId, (time(NULL) + g_config.getNumber(ConfigManager::BAN_LENGTH)), 20, 2, "Unjustified player killing.", 0);

		if(success)
		{
			g_game.addMagicEffect(getPosition(), NM_ME_MAGIC_POISON);
			Scheduler::getScheduler().addEvent(createSchedulerTask(500, boost::bind(&Game::kickPlayer, &g_game, getID(), false)));
		}
	}
}

void Player::checkRedSkullTicks(int32_t ticks)
{
	if((redSkullTicks - ticks) > 0)
		redSkullTicks -= ticks;

	if(redSkullTicks < 1000 && !hasCondition(CONDITION_INFIGHT) && skull == SKULL_RED)
	{
		setSkull(SKULL_NONE);
		g_game.updateCreatureSkull(this);
	}
}

void Player::setPromotionLevel(uint32_t pLevel)
{
	uint32_t tmpLevel = 0, currentVoc = vocation_id;
	for(uint32_t i = promotionLevel; i < pLevel; i++)
	{
		currentVoc = g_vocations.getPromotedVocation(currentVoc);
		if(currentVoc == 0)
			break;

		tmpLevel++;
		Vocation *voc = g_vocations.getVocation(currentVoc);
		if(voc->isPremiumNeeded() && !isPremium() && g_config.getBool(ConfigManager::PREMIUM_FOR_PROMOTION))
			continue;

		vocation_id = currentVoc;
	}

	setVocation(vocation_id);
	promotionLevel += tmpLevel;
}

uint16_t Player::getBlessings() const
{
	if(!isPremium() && g_config.getBool(ConfigManager::BLESSING_ONLY_PREMIUM))
		return 0;

	uint16_t count = 0;
	for(int16_t i = 0; i < 16; ++i)
	{
		if(hasBlessing(i))
			count++;
	}

	return count;
}

uint64_t Player::getLostExperience() const
{
	if(!skillLoss)
		return 0;

	float percent = (float)(lossPercent[LOSS_EXPERIENCE] - vocation->getLessLoss() - (getBlessings() * 8)) / 100.0f;
	if(level <= 25)
		return (uint64_t)std::floor((float)experience * (percent / 10.0f));

	int32_t base = level;
	float levels = (float)(base + 50) / 100.0f;

	uint64_t lost = 0;
	while(levels > 1.0f)
	{
		lost += (getExpForLevel(base) - getExpForLevel(base - 1));
		base--;
		levels -= 1.0f;
	}

	if(levels > 0.0f)
		lost += (uint64_t)std::floor((float)(getExpForLevel(base) - getExpForLevel(base - 1)) * levels);

	return (uint64_t)std::floor((float)lost * percent);
}

uint32_t Player::getAttackSpeed()
{
	Item* weapon = getWeapon();
	if(weapon && weapon->getAttackSpeed() != 0)
		return weapon->getAttackSpeed();

	return vocation->getAttackSpeed();
}

void Player::learnInstantSpell(const std::string& name)
{
	if(!hasLearnedInstantSpell(name))
		learnedInstantSpellList.push_back(name);
}

void Player::unlearnInstantSpell(const std::string& name)
{
	if(hasLearnedInstantSpell(name))
	{
		LearnedInstantSpellList::iterator it = std::find(learnedInstantSpellList.begin(), learnedInstantSpellList.end(), name);
		if(it != learnedInstantSpellList.end())
			learnedInstantSpellList.erase(it);
	}
}

bool Player::hasLearnedInstantSpell(const std::string& name) const
{
	if(hasFlag(PlayerFlag_CannotUseSpells))
		return false;

	if(hasFlag(PlayerFlag_IgnoreSpellCheck))
		return true;

	for(LearnedInstantSpellList::const_iterator it = learnedInstantSpellList.begin(); it != learnedInstantSpellList.end(); ++it)
	{
		if(strcasecmp((*it).c_str(), name.c_str()) == 0)
			return true;
	}

	return false;
}

void Player::manageAccount(const std::string &text)
{
	std::stringstream msg;
	msg << "Account Manager: ";
	switch(accountManager)
	{
		case MANAGER_NAMELOCK:
		{
			if(!talkState[1])
			{
				managerString = text;
				trimString(managerString);
				if(managerString.length() < 4)
					msg << "Your name you want is too short, please select a longer name.";
				else if(managerString.length() > 20)
					msg << "The name you want is too long, please select a shorter name.";
				else if(!isValidName(managerString))
					msg << "That name seems to contain invalid symbols, please choose another name.";
				else if(IOLoginData::getInstance()->playerExists(managerString, true))
					msg << "A player with such name already exists, please choose another name.";
				else
				{
					std::string tmp = asLowerCaseString(managerString);
					if(tmp.substr(0, 4) != "god " && tmp.substr(0, 3) != "cm " && tmp.substr(0, 3) != "gm ")
					{
						talkState[1] = true;
						talkState[2] = true;
						msg << managerString << ", are you sure?";
					}
					else
						msg << "Your character is not a staff member, please tell me another name!";
				}
			}
			else if(checkText(text, "no") && talkState[2])
			{
				talkState[1] = talkState[2] = false;
				msg << "What else would you like to name your character?";
			}
			else if(checkText(text, "yes") && talkState[2])
			{
				if(!IOLoginData::getInstance()->playerExists(managerString, true))
				{
					uint32_t tmp;
					if(IOLoginData::getInstance()->getGuidByName(tmp, managerString2) &&
						IOLoginData::getInstance()->changeName(tmp, managerString, managerString2) &&
						IOBan::getInstance()->removeNamelock(tmp))
					{
						if(House* house = Houses::getInstance().getHouseByPlayerId(tmp))
							house->updateDoorDescription(managerString);

						talkState[1] = true;
						talkState[2] = false;
						msg << "Your character has been successfully renamed, you should now be able to login at it without any problems.";
					}
					else
					{
						talkState[1] = talkState[2] = false;
						msg << "Failed to change your name, please try again.";
					}
				}
				else
				{
					talkState[1] = talkState[2] = false;
					msg << "A player with that name already exists, please pick another name.";
				}
			}
			else
				msg << "Sorry, but I can't understand you, please try to repeat that!";

			break;
		}
		case MANAGER_ACCOUNT:
		{
			Account account = IOLoginData::getInstance()->loadAccount(managerNumber);
			if(checkText(text, "cancel") || (checkText(text, "account") && !talkState[1]))
			{
				talkState[1] = true;
				for(int8_t i = 2; i <= 12; i++)
					talkState[i] = false;

				msg << "Do you want to change your 'password', request a 'recovery key', add a 'character', or 'delete' a character?";
			}
			else if(checkText(text, "delete") && talkState[1])
			{
				talkState[1] = false;
				talkState[2] = true;
				msg << "Which character would you like to delete?";
			}
			else if(talkState[2])
			{
				std::string tmp = text;
				trimString(tmp);
				if(!isValidName(tmp, false))
					msg << "That name contains invalid characters, try to say your name again, you might have typed it wrong.";
				else
				{
					talkState[2] = false;
					talkState[3] = true;
					managerString = tmp;
					msg << "Do you really want to delete the character named " << managerString << "?";
				}
			}
			else if(checkText(text, "yes") && talkState[3])
			{
				switch(IOLoginData::getInstance()->deleteCharacter(managerNumber, managerString))
				{
					case DELETE_INTERNAL:
						msg << "An error occured while deleting your character. Either the character does not belong to you or it doesn't exist.";
						break;

					case DELETE_SUCCESS:
						msg << "Your character has been deleted.";
						break;

					case DELETE_HOUSE:
						msg << "Your character owns a house. To make sure you really want to lose your house by deleting your character, you have to login and leave the house or pass it to someone else first.";
						break;

					case DELETE_LEADER:
						msg << "Your character is the leader of a guild. You need to disband or pass the leadership someone else to delete your character.";
						break;

					case DELETE_ONLINE:
						msg << "A character with that name is currently online, to delete a character it has to be offline.";
						break;
				}

				talkState[1] = true;
				for(int8_t i = 2; i <= 12; i++)
					talkState[i] = false;
			}
			else if(checkText(text, "no") && talkState[3])
			{
				talkState[1] = true;
				talkState[3] = false;
				msg << "Tell me what character you want to delete.";
			}
			else if(checkText(text, "password") && talkState[1])
			{
				talkState[1] = false;
				talkState[4] = true;
				msg << "Tell me your new password please.";
			}
			else if(talkState[4])
			{
				std::string tmp = text;
				trimString(tmp);
				if(tmp.length() < 6)
					msg << "That password is too short, at least 6 digits are required. Please select a longer password.";
				else if(!isValidPassword(tmp))
					msg << "Your password contains invalid characters... please tell me another one.";
				else
				{
					talkState[4] = false;
					talkState[5] = true;
					managerString = tmp;
					msg << "Should '" << managerString << "' be your new password?";
				}
			}
			else if(checkText(text, "yes") && talkState[5])
			{
				talkState[1] = true;
				for(int8_t i = 2; i <= 12; i++)
					talkState[i] = false;

				IOLoginData::getInstance()->setNewPassword(managerNumber, managerString);
				msg << "Your password has been changed.";
			}
			else if(checkText(text, "no") && talkState[5])
			{
				talkState[1] = true;
				for(int8_t i = 2; i <= 12; i++)
					talkState[i] = false;

				msg << "Then not.";
			}
			else if(checkText(text, "character") && talkState[1])
			{
				if(account.charList.size() <= 15)
				{
					talkState[1] = false;
					talkState[6] = true;
					msg << "What would you like as your character name?";
				}
				else
				{
					talkState[1] = true;
					for(int8_t i = 2; i <= 12; i++)
						talkState[i] = false;

					msg << "Your account reach the limit of 15 players, you can 'delete' a character if you want to create a new one.";
				}
			}
			else if(talkState[6])
			{
				managerString = text;
				trimString(managerString);
				if(managerString.length() < 4)
					msg << "Your name you want is too short, please select a longer name.";
				else if(managerString.length() > 20)
					msg << "The name you want is too long, please select a shorter name.";
				else if(!isValidName(managerString))
					msg << "That name seems to contain invalid symbols, please choose another name.";
				else if(IOLoginData::getInstance()->playerExists(managerString, true))
					msg << "A player with such name already exists, please choose another name.";
				else
				{
					std::string tmp = asLowerCaseString(managerString);
					if(tmp.substr(0, 4) != "god " && tmp.substr(0, 3) != "cm " && tmp.substr(0, 3) != "gm ")
					{
						talkState[6] = false;
						talkState[7] = true;
						msg << managerString << ", are you sure?";
					}
					else
						msg << "Your character is not a staff member, please tell me another name!";
				}
			}
			else if(checkText(text, "no") && talkState[7])
			{
				talkState[6] = true;
				talkState[7] = false;
				msg << "What else would you like to name your character?";
			}
			else if(checkText(text, "yes") && talkState[7])
			{
				talkState[7] = false;
				talkState[8] = true;
				msg << "Should your character be a 'male' or a 'female'.";
			}
			else if(talkState[8] && (checkText(text, "female") || checkText(text, "male")))
			{
				talkState[8] = false;
				talkState[9] = true;
				if(checkText(text, "female"))
				{
					msg << "A female, are you sure?";
					managerSex = PLAYERSEX_FEMALE;
				}
				else
				{
					msg << "A male, are you sure?";
					managerSex = PLAYERSEX_MALE;
				}
			}
			else if(checkText(text, "no") && talkState[9])
			{
				talkState[8] = true;
				talkState[9] = false;
				msg << "Tell me... would you like to be a 'male' or a 'female'?";
			}
			else if(checkText(text, "yes") && talkState[9])
			{
				if(g_config.getBool(ConfigManager::START_CHOOSEVOC))
				{
					talkState[9] = false;
					talkState[11] = true;

					bool firstPart = true;
					for(VocationsMap::iterator it = g_vocations.getFirstVocation(); it != g_vocations.getLastVocation(); ++it)
					{
						if(it->first == it->second->getFromVocation() && it->first != 0)
						{
							if(firstPart)
							{
								msg << "What do you want to be... " << it->second->getVocDescription();
								firstPart = false;
							}
							else if(it->first - 1 != 0)
								msg << ", " << it->second->getVocDescription();
							else
								msg << " or " << it->second->getVocDescription() << ".";
						}
					}
				}
				else
				{
					if(!IOLoginData::getInstance()->playerExists(managerString, true))
					{
						talkState[1] = true;
						for(int8_t i = 2; i <= 12; i++)
							talkState[i] = false;

						if(IOLoginData::getInstance()->createCharacter(managerNumber, managerString, managerNumber2, managerSex))
							msg << "Your character has been created.";
						else
							msg << "Your character couldn't be created, please try again.";
					}
					else
					{
						talkState[6] = true;
						talkState[9] = false;
						msg << "A player with this name currently exists, please choose another name.";
					}
				}
			}
			else if(talkState[11])
			{
				for(VocationsMap::iterator it = g_vocations.getFirstVocation(); it != g_vocations.getLastVocation(); ++it)
				{
					std::string tmp = asLowerCaseString(it->second->getVocName());
					if(checkText(text, tmp) && it != g_vocations.getLastVocation() && it->first == it->second->getFromVocation() && it->first != 0)
					{
						msg << "So you would like to be " << it->second->getVocDescription() << "... are you sure?";
						managerNumber2 = it->first;
						talkState[11] = false;
						talkState[12] = true;
					}
				}

				if(msg.str().length() == 17)
					msg << "I don't understand what vocation you would like to be... could you please repeat it?";
			}
			else if(checkText(text, "yes") && talkState[12])
			{
				if(!IOLoginData::getInstance()->playerExists(managerString, true))
				{
					talkState[1] = true;
					for(int8_t i = 2; i <= 12; i++)
						talkState[i] = false;

					if(IOLoginData::getInstance()->createCharacter(managerNumber, managerString, managerNumber2, managerSex))
						msg << "Your character has been created.";
					else
						msg << "Your character couldn't be created, please try again.";
				}
				else
				{
					talkState[6] = true;
					talkState[9] = false;
					msg << "A player with this name currently exists, please choose another name.";
				}
			}
			else if(checkText(text, "no") && talkState[12])
			{
				talkState[11] = true;
				talkState[12] = false;
				msg << "No? Then what would you like to be?";
			}
			else if(checkText(text, "recovery key") && talkState[1])
			{
				talkState[1] = false;
				talkState[10] = true;
				msg << "Would you like a recovery key?";
			}
			else if(checkText(text, "yes") && talkState[10])
			{
				if(account.recoveryKey != "0")
					msg << "Sorry, you already have a recovery key, for security reasons I may not give you a new one.";
				else
				{
					managerString = generateRecoveryKey(4, 4);
					IOLoginData::getInstance()->setRecoveryKey(managerNumber, managerString);
					msg << "Your recovery key is: " << managerString << ".";
				}

				talkState[1] = true;
				for(int8_t i = 2; i <= 12; i++)
					talkState[i] = false;
			}
			else if(checkText(text, "no") && talkState[10])
			{
				msg << "Then not.";
				talkState[1] = true;
				for(int8_t i = 2; i <= 12; i++)
					talkState[i] = false;
			}
			else
				msg << "Please read the latest message that I have specified, I don't understand the current requested action.";

			break;
		}
		case MANAGER_NEW:
		{
			if(checkText(text, "account") && !talkState[1])
			{
				msg << "What would you like your password to be?";
				talkState[1] = true;
				talkState[2] = true;
			}
			else if(talkState[2])
			{
				std::string tmp = text;
				trimString(tmp);
				if(tmp.length() < 6)
					msg << "That password is too short, at least 6 digits are required. Please select a longer password.";
				else if(!isValidPassword(tmp))
					msg << "Your password contains invalid characters... please tell me another one.";
				else
				{
					talkState[3] = true;
					talkState[2] = false;
					managerString = tmp;
					msg << managerString << " is it? 'yes' or 'no'?";
				}
			}
			else if(checkText(text, "yes") && talkState[3])
			{
				if(g_config.getBool(ConfigManager::GENERATE_ACCOUNT_NUMBER))
				{
					do
						sprintf(managerChar, "%d%d%d%d%d%d%d", random_range(2, 9), random_range(2, 9), random_range(2, 9), random_range(2, 9), random_range(2, 9), random_range(2, 9), random_range(2, 9));
					while(IOLoginData::getInstance()->accountNameExists(managerChar));
					msg << "Your account has been created, you can login now with name: '" << managerChar << "', and password: '" << managerString << "'! If the account name is too hard to remember, please note it somewhere.";

					IOLoginData::getInstance()->createAccount(managerChar, managerString);
					for(int8_t i = 2; i <= 5; i++)
						talkState[i] = false;
				}
				else
				{
					msg << "What would you like your account name to be?";
					talkState[3] = false;
					talkState[4] = true;
				}
			}
			else if(checkText(text, "no") && talkState[3])
			{
				talkState[2] = true;
				talkState[3] = false;
				msg << "What would you like your password to be then?";
			}
			else if(talkState[4])
			{
				std::string tmp = text;
				trimString(tmp);
				if(tmp.length() < 3)
					msg << "That account name is too short, at least 3 digits are required. Please select a longer account name.";
				else if(tmp.length() > 25)
					msg << "That account name is too long, not more than 25 digits are required. Please select a shorter account name.";
				else if(!isValidAccountName(tmp))
					msg << "Your account name contains invalid characters, please choose another one.";
				else
				{
					sprintf(managerChar, "%s", tmp.c_str());
					msg << managerChar << ", are you sure?";
					talkState[4] = false;
					talkState[5] = true;
				}
			}
			else if(checkText(text, "yes") && talkState[5])
			{
				if(!IOLoginData::getInstance()->accountNameExists(managerChar))
				{
					IOLoginData::getInstance()->createAccount(managerChar, managerString);
					msg << "Your account has been created, you can login now with name: '" << managerChar << "', and password: '" << managerString << "'!";
					for(int8_t i = 2; i <= 5; i++)
						talkState[i] = false;
				}
				else
				{
					msg << "An account with that name already exists, please try another account name.";
					talkState[4] = true;
					talkState[5] = false;
				}
			}
			else if(checkText(text, "no") && talkState[5])
			{
				talkState[5] = false;
				talkState[4] = true;
				msg << "What else would you like as your account name?";
			}
			else if(checkText(text, "recover") && !talkState[6])
			{
				talkState[6] = true;
				talkState[7] = true;
				msg << "What was your account name?";
			}
			else if(talkState[7])
			{
				managerString = text;
				if(IOLoginData::getInstance()->getAccountId(managerString, (uint32_t&)managerNumber))
				{
					talkState[7] = false;
					talkState[8] = true;
					msg << "What was your recovery key?";
				}
				else
				{
					msg << "Sorry, but account with such name doesn't exists.";
					talkState[6] = talkState[7] = false;
				}
			}
			else if(talkState[8])
			{
				managerString2 = text;
				if(IOLoginData::getInstance()->validRecoveryKey(managerNumber, managerString2) && managerString2 != "0")
				{
					sprintf(managerChar, "%s%d", g_config.getString(ConfigManager::SERVER_NAME).c_str(), random_range(100, 999));
					IOLoginData::getInstance()->setNewPassword(managerNumber, managerChar);
					msg << "Correct! Your new password is: " << managerChar << ".";
				}
				else
					msg << "Sorry, but this key doesn't match to account you gave me.";

				talkState[7] = talkState[8] = false;
			}
			else
				msg << "Sorry, but I can't understand you, please try to repeat that.";

			break;
		}
		default:
			return;
			break;
	}

	sendTextMessage(MSG_STATUS_CONSOLE_BLUE, msg.str().c_str());
}

bool Player::isInvitedToGuild(uint32_t guild_id) const
{
	for(InvitedToGuildsList::const_iterator it = invitedToGuildsList.begin(); it != invitedToGuildsList.end(); ++it)
	{
		if((*it) == guild_id)
			return true;
	}
	return false;
}

void Player::resetGuildInformation()
{
	sendClosePrivate(0x00);
	guildId = 0;
	guildName = "";
	guildRank = "";
	guildRankId = 0;
	guildNick = "";
	guildLevel = 0;
}

bool Player::isPremium() const
{
	if(g_config.getBool(ConfigManager::FREE_PREMIUM) || hasFlag(PlayerFlag_IsAlwaysPremium))
		return true;

	return premiumDays;
}

void Player::setGuildLevel(GuildLevel_t newGuildLevel)
{
	guildLevel = newGuildLevel;
	setGuildRank(IOGuild::getInstance()->getRankName(guildLevel, guildId));
}

void Player::setGroupId(int32_t newId)
{
	const PlayerGroup* group = IOLoginData::getInstance()->getPlayerGroup(newId);
	if(group)
	{
		groupId = newId;
		groupName = asLowerCaseString(group->m_name);
		groupOutfit = group->m_outfit;
		setFlags(group->m_flags);
		setCustomFlags(group->m_customflags);
		accessLevel = group->m_access;
		violationAccess = group->m_violationaccess;

		if(group->m_maxdepotitems > 0)
			maxDepotLimit = group->m_maxdepotitems;
		else if(isPremium())
			maxDepotLimit = 2000;

		if(group->m_maxviplist > 0)
			maxVipLimit = group->m_maxviplist;
		else if(isPremium())
			maxVipLimit = 100;
	}
	else if(isPremium())
	{
		maxDepotLimit = 2000;
		maxVipLimit = 100;
	}
}

PartyShields_t Player::getPartyShield(const Creature* creature) const
{
	const Player* player = creature->getPlayer();
	if(!player)
		return Creature::getPartyShield(creature);

	if(Party* party = getParty())
	{
		if(party->getLeader() == player)
		{
			if(party->isSharedExperienceActive())
			{
				if(party->isSharedExperienceEnabled())
					return SHIELD_YELLOW_SHAREDEXP;

				if(party->canUseSharedExperience(player))
					return SHIELD_YELLOW_NOSHAREDEXP;

				return SHIELD_YELLOW_NOSHAREDEXP_BLINK;
			}

			return SHIELD_YELLOW;
		}

		if(party->isPlayerMember(player))
		{
			if(party->isSharedExperienceActive())
			{
				if(party->isSharedExperienceEnabled())
					return SHIELD_BLUE_SHAREDEXP;

				if(party->canUseSharedExperience(player))
					return SHIELD_BLUE_NOSHAREDEXP;

				return SHIELD_BLUE_NOSHAREDEXP_BLINK;
			}

			return SHIELD_BLUE;
		}

		if(isInviting(player))
			return SHIELD_WHITEBLUE;
	}

	if(player->isInviting(this))
		return SHIELD_WHITEYELLOW;

	return SHIELD_NONE;
}

bool Player::isInviting(const Player* player) const
{
	if(!player || !getParty() || getParty()->getLeader() != this)
		return false;

	return getParty()->isPlayerInvited(player);
}

bool Player::isPartner(const Player* player) const
{
	if(!player || !getParty() || !player->getParty())
		return false;

	return (getParty() == player->getParty());
}

void Player::sendPlayerPartyIcons(Player* player)
{
	sendCreatureShield(player);
	sendCreatureSkull(player);
}

bool Player::addPartyInvitation(Party* party)
{
	if(!party)
		return false;

	PartyList::iterator it = std::find(invitePartyList.begin(), invitePartyList.end(), party);
	if(it != invitePartyList.end())
		return false;

	invitePartyList.push_back(party);
	return true;
}

bool Player::removePartyInvitation(Party* party)
{
	if(!party)
		return false;

	PartyList::iterator it = std::find(invitePartyList.begin(), invitePartyList.end(), party);
	if(it != invitePartyList.end())
	{
		invitePartyList.erase(it);
		return true;
	}
	return false;
}

void Player::clearPartyInvitations()
{
	if(!invitePartyList.empty())
	{
		PartyList list;
		for(PartyList::iterator it = invitePartyList.begin(); it != invitePartyList.end(); ++it)
			list.push_back(*it);

		invitePartyList.clear();

		for(PartyList::iterator it = list.begin(); it != list.end(); ++it)
			(*it)->removeInvite(this);
	}
}

bool Player::withdrawMoney(uint64_t amount)
{
	if(!g_config.getBool(ConfigManager::BANK_SYSTEM))
		return false;

	if(amount > balance)
		return false;

	g_game.addMoney(this, amount);
	balance -= amount;
	return true;
}

bool Player::depositMoney(uint64_t amount)
{
	if(!g_config.getBool(ConfigManager::BANK_SYSTEM))
		return false;

	bool ret = g_game.removeMoney(this, amount);
	if(ret)
		balance += amount;

	return ret;
}

bool Player::transferMoneyTo(const std::string& name, uint64_t amount)
{
	if(!g_config.getBool(ConfigManager::BANK_SYSTEM))
		return false;

	if(amount > balance)
		return false;

	Player* target = g_game.getPlayerByName(name);
	if(!target)
	{
		target = new Player(name, NULL);
		if(!IOLoginData::getInstance()->loadPlayer(target, name))
		{
#ifdef __DEBUG__
			std::cout << "Failure: [Player::transferMoneyTo], can not load player: " << name << std::endl;
#endif
			delete target;
			return false;
		}
	}

	balance -= amount;
	target->balance += amount;
	if(target->isVirtual())
	{
		IOLoginData::getInstance()->savePlayer(target);
		delete target;
	}

	return true;
}

void Player::sendCriticalHit() const
{
	if(g_config.getBool(ConfigManager::DISPLAY_CRITICAL_HIT))
		g_game.addAnimatedText(getPosition(), TEXTCOLOR_DARKRED, "CRITICAL!");
}
