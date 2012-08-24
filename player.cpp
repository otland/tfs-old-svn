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

#include <string>
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
#include "ban.h"
#include "configmanager.h"
#include "creatureevent.h"
#include "status.h"
#include "beds.h"
#include "mounts.h"
#include "quests.h"
#ifndef _CONSOLE
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
MuteCountMap Player::muteCountMap;
int32_t Player::maxMessageBuffer;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t Player::playerCount = 0;
#endif

Player::Player(const std::string& _name, ProtocolGame *p) :
Creature()
{
	client = p;
	isConnecting = false;

	if(client)
		client->setPlayer(this);

	depotChange = false;
	accountNumber = 0;
	name = _name;
	setVocation(0);
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
	accessLevel = false;
	groupName = "";
	groupId = 0;
	lastLoginSaved = 0;
	lastLogout = 0;
 	lastIP = 0;
	lastPing = OTSYS_TIME();
	lastPong = OTSYS_TIME();
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

	mayNotMove = false;

	marketDepotId = -1;

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
	}

	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i)
		varSkills[i] = 0;

	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i)
		varStats[i] = 0;

	maxDepotLimit = 1000;
	maxVipLimit = 20;
	groupFlags = 0;

	accountManager = NULL;

	sex = PLAYERSEX_FEMALE;
 	vocationId = 0;

 	town = 0;

	accountType = ACCOUNT_TYPE_NORMAL;
	premiumDays = 0;

	idleTime = 0;

	skullTicks = 0;
	skull = SKULL_NONE;
	setParty(NULL);

	groupId = 0;

	bankBalance = 0;

	offlineTrainingSkill = -1;
	offlineTrainingTime = 0;
	lastStatsTrainingTime = 0;

	ghostMode = false;
	requestedOutfit = false;
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	playerCount++;
#endif
}

Player::~Player()
{
	for(int i = 0; i < 11; ++i)
	{
		if(inventory[i])
		{
			inventory[i]->setParent(NULL);
			inventory[i]->releaseThing2();
			inventory[i] = NULL;
			inventoryAbilities[i] = false;
		}
	}

	for(DepotMap::iterator it = depots.begin(), end = depots.end(); it != end; ++it)
		it->second->releaseThing2();

	//std::cout << "Player destructor " << this << std::endl;

	setWriteItem(NULL);
	setEditHouse(NULL);

	delete accountManager;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	playerCount--;
#endif
}

void Player::setVocation(uint32_t vocId)
{
	vocationId = vocId;
	vocation = g_vocations.getVocation(vocId);

	Condition* condition = getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition)
	{
		condition->setParam(CONDITIONPARAM_HEALTHGAIN, vocation->getHealthGainAmount());
		condition->setParam(CONDITIONPARAM_HEALTHTICKS, vocation->getHealthGainTicks() * 1000);
		condition->setParam(CONDITIONPARAM_MANAGAIN, vocation->getManaGainAmount());
		condition->setParam(CONDITIONPARAM_MANATICKS, vocation->getManaGainTicks() * 1000);
	}

	soulMax = vocation->getSoulMax();

	sendBasicData();
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
	std::ostringstream s;
	std::string str;
	if(lookDistance == -1)
	{
		s << "yourself.";
		if(accessLevel)
			s << " You are " << groupName << ".";
		else if(vocationId != VOCATION_NONE)
			s << " You are " << vocation->getVocDescription() << ".";
		else
			s << " You have no vocation.";
	}
	else
	{
		s << name;
		if(!accessLevel)
			s << " (Level " << level << ")";
		s << ".";

		if(sex == PLAYERSEX_FEMALE)
			s << " She";
		else
			s << " He";

		if(accessLevel)
			s << " is " << groupName << ".";
		else if(vocationId != VOCATION_NONE)
			s << " is " << vocation->getVocDescription() << ".";
		else
			s << " has no vocation.";
	}

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
	str = s.str();
	return str;
}

Item* Player::getInventoryItem(slots_t slot) const
{
	if(slot >= SLOT_FIRST && slot < SLOT_LAST)
		return inventory[slot];

	return NULL;
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
		item = getInventoryItem((slots_t)slot);
		if(!item)
			continue;

		switch(item->getWeaponType())
		{
			case WEAPON_SWORD:
			case WEAPON_AXE:
			case WEAPON_CLUB:
			case WEAPON_WAND:
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
	Item* item = getWeapon();
	if(!item)
		return WEAPON_NONE;
	return item->getWeaponType();
}

int32_t Player::getWeaponSkill(const Item* item) const
{
	if(!item)
		return getSkill(SKILL_FIST, SKILL_LEVEL);

	WeaponType_t weaponType = item->getWeaponType();
	int32_t attackSkill;

	switch(weaponType)
	{
		case WEAPON_SWORD:
		{
			attackSkill = getSkill(SKILL_SWORD, SKILL_LEVEL);
			break;
		}

		case WEAPON_CLUB:
		{
			attackSkill = getSkill(SKILL_CLUB, SKILL_LEVEL);
			break;
		}

		case WEAPON_AXE:
		{
			attackSkill = getSkill(SKILL_AXE, SKILL_LEVEL);
			break;
		}

		case WEAPON_DIST:
		{
			attackSkill = getSkill(SKILL_DIST, SKILL_LEVEL);
			break;
		}

		default:
		{
			attackSkill = 0;
			break;
		}
	}
	return attackSkill;
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

	return int32_t(armor * vocation->armorMultipler);
}

void Player::getShieldAndWeapon(const Item* &shield, const Item* &weapon) const
{
	Item* item;
	shield = NULL;
	weapon = NULL;
	for(uint32_t slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++)
	{
		item = getInventoryItem((slots_t)slot);
		if(item)
		{
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
				default: // weapons that are not shields
				{
					weapon = item;
					break;
				}
			}
		}
	}
	return;
}

int32_t Player::getDefense() const
{
	int32_t baseDefense = 5;
	int32_t defenseValue = 0;
	int32_t defenseSkill = 0;
	int32_t extraDefense = 0;
	float defenseFactor = getDefenseFactor();
	const Item* weapon = NULL;
	const Item* shield = NULL;
	getShieldAndWeapon(shield, weapon);

	if(weapon)
	{
		defenseValue = baseDefense + weapon->getDefense();
		extraDefense = weapon->getExtraDefense();
		defenseSkill = getWeaponSkill(weapon);
	}

	if(shield && shield->getDefense() >= defenseValue)
	{
		defenseValue = baseDefense + shield->getDefense() + extraDefense;
		defenseSkill = getSkill(SKILL_SHIELD, SKILL_LEVEL);
	}

	defenseValue = int32_t(defenseValue * vocation->defenseMultipler);

	if(defenseSkill == 0)
		return 0;

	return ((int32_t)std::ceil(((float)(defenseSkill * (defenseValue * 0.015)) + (defenseValue * 0.1)) * defenseFactor));
}

float Player::getAttackFactor() const
{
	switch(fightMode)
	{
		case FIGHTMODE_ATTACK:
			return 1.0f;

		case FIGHTMODE_BALANCED:
			return 1.2f;

		case FIGHTMODE_DEFENSE:
			return 2.0f;

		default:
			return 1.0f;
	}
}

float Player::getDefenseFactor() const
{
	switch(fightMode)
	{
		case FIGHTMODE_ATTACK:
			return 1.0f;

		case FIGHTMODE_BALANCED:
			return 1.2f;

		case FIGHTMODE_DEFENSE:
		{
			if((OTSYS_TIME() - lastAttack) < getAttackSpeed())
				return 1.0f;

			return 2.0f;
		}

		default:
			return 1.0f;
	}
}

void Player::sendIcons() const
{
	if(client)
	{
		int32_t icons = 0;

		ConditionList::const_iterator it;
		for(it = conditions.begin(); it != conditions.end(); ++it)
		{
			if(!isSuppress((*it)->getType()))
				icons |= (*it)->getIcons();
		}

		if(pzLocked)
			icons |= ICON_REDSWORDS;

		if(_tile->hasFlag(TILESTATE_PROTECTIONZONE))
		{
			icons |= ICON_PIGEON;

			// Don't show ICON_SWORDS if player is in protection zone.
			if(hasBitSet(ICON_SWORDS, icons))
				icons &= ~ICON_SWORDS;
		}

		if(!getCondition(CONDITION_REGENERATION))
			icons |= ICON_HUNGRY;

		client->sendIcons(icons);
	}
}

void Player::updateInventoryWeight()
{
	if(!hasFlag(PlayerFlag_HasInfiniteCapacity))
	{
		inventoryWeight = 0.00;
		for(int i = SLOT_FIRST; i < SLOT_LAST; ++i)
		{
			Item* item = getInventoryItem((slots_t)i);
			if(item)
				inventoryWeight += item->getWeight();
		}
	}
}

int32_t Player::getPlayerInfo(playerinfo_t playerinfo) const
{
	switch(playerinfo)
	{
		case PLAYERINFO_LEVEL: return level;
		case PLAYERINFO_LEVELPERCENT: return levelPercent;
		case PLAYERINFO_MAGICLEVEL: return std::max((int32_t)0, ((int32_t)magLevel + varStats[STAT_MAGICPOINTS]));
		case PLAYERINFO_MAGICLEVELPERCENT: return magLevelPercent;
		case PLAYERINFO_HEALTH: return health;
		case PLAYERINFO_MAXHEALTH: return std::max((int32_t)1, ((int32_t)healthMax + varStats[STAT_MAXHITPOINTS]));
		case PLAYERINFO_MANA: return mana;
		case PLAYERINFO_MAXMANA: return std::max((int32_t)0, ((int32_t)manaMax + varStats[STAT_MAXMANAPOINTS]));
		case PLAYERINFO_SOUL: return std::max((int32_t)0, ((int32_t)soul + varStats[STAT_SOULPOINTS]));
		default: return 0;
	}
	return 0;
}

int32_t Player::getSkill(skills_t skilltype, skillsid_t skillinfo) const
{
	int32_t n = skills[skilltype][skillinfo];
	if(skillinfo == SKILL_LEVEL)
		n += varSkills[skilltype];

	return std::max((int32_t)0, (int32_t)n);
}

void Player::addSkillAdvance(skills_t skill, uint32_t count)
{
	if(count == 0)
		return;

	uint64_t currReqTries = vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL]);
	uint64_t nextReqTries = vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL] + 1);
	if(currReqTries > nextReqTries)
	{
		//player has reached max skill
		return;
	}

	bool sendUpdateSkills = false;
	count *= g_config.getNumber(ConfigManager::RATE_SKILL);
	while((skills[skill][SKILL_TRIES] + count) >= nextReqTries)
	{
		count -= nextReqTries - skills[skill][SKILL_TRIES];
		skills[skill][SKILL_LEVEL]++;
		skills[skill][SKILL_TRIES] = 0;
		skills[skill][SKILL_PERCENT] = 0;

		std::ostringstream ss;
		ss << "You advanced to " << getSkillName(skill) << " level " << skills[skill][SKILL_LEVEL] << ".";
		sendTextMessage(MSG_EVENT_ADVANCE, ss.str());

		g_creatureEvents->playerAdvance(this, skill, (skills[skill][SKILL_LEVEL] - 1), skills[skill][SKILL_LEVEL]);

		sendUpdateSkills = true;
		currReqTries = nextReqTries,
		nextReqTries = vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL] + 1);
		if(currReqTries > nextReqTries)
		{
			count = 0;
			break;
		}
	}
	skills[skill][SKILL_TRIES] += count;

	uint32_t newPercent;
	if(nextReqTries > currReqTries)
		newPercent = Player::getPercentLevel(skills[skill][SKILL_TRIES], nextReqTries);
	else
		newPercent = 0;

	if(skills[skill][SKILL_PERCENT] != newPercent)
	{
		skills[skill][SKILL_PERCENT] = newPercent;
		sendUpdateSkills = true;
	}

	if(sendUpdateSkills)
		sendSkills();
}

void Player::setVarStats(stats_t stat, int32_t modifier)
{
	varStats[stat] += modifier;
	switch(stat)
	{
		case STAT_MAXHITPOINTS:
		{
			if(getHealth() > getMaxHealth())
				Creature::changeHealth(getMaxHealth() - getHealth());
			else
				g_game.addCreatureHealth(this);
			break;
		}

		case STAT_MAXMANAPOINTS:
		{
			if(getMana() > getMaxMana())
				Creature::changeMana(getMaxMana() - getMana());
			break;
		}

		default:
		{
			break;
		}
	}
}

int32_t Player::getDefaultStats(stats_t stat)
{
	switch(stat)
	{
		case STAT_MAXHITPOINTS:
			return getMaxHealth() - getVarStats(STAT_MAXHITPOINTS);
		case STAT_MAXMANAPOINTS:
			return getMaxMana() - getVarStats(STAT_MAXMANAPOINTS);
		case STAT_SOULPOINTS:
			return getPlayerInfo(PLAYERINFO_SOUL) - getVarStats(STAT_SOULPOINTS);
		case STAT_MAGICPOINTS:
			return getMagicLevel() - getVarStats(STAT_MAGICPOINTS);
		default:
			return 0;
	}
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

	//id doesnt exist, create it
	containervector_pair cv;
	cv.first = cid;
	cv.second = container;

	containerVec.push_back(cv);
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
	return getID() == ownerId || (party && party->canOpenCorpse(ownerId));
}

uint16_t Player::getLookCorpse() const
{
	if(sex != 0)
		return ITEM_MALE_CORPSE;
	else
		return ITEM_FEMALE_CORPSE;
}

uint16_t Player::getDropPercent() const
{
	uint16_t dropPercent;
	std::bitset<5> bitset(blessings);
	switch(bitset.count())
	{
		case 1:
			dropPercent = 70;
			break;

		case 2:
			dropPercent = 45;
			break;

		case 3:
			dropPercent = 25;
			break;

		case 4:
			dropPercent = 10;
			break;

		case 5:
			dropPercent = 0;
			break;

		default:
			dropPercent = 100;
			break;
	}
	return dropPercent;
}

void Player::dropLoot(Container* corpse)
{
	if(corpse && lootDrop && vocationId != VOCATION_NONE)
	{
		if(inventory[SLOT_NECKLACE] && inventory[SLOT_NECKLACE]->getID() == ITEM_AMULETOFLOSS &&
			getSkull() != SKULL_RED && getSkull() != SKULL_BLACK && g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
		{
			g_game.internalRemoveItem(inventory[SLOT_NECKLACE], 1);
		}
		else
		{
			for(int32_t i = SLOT_FIRST; i < SLOT_LAST; ++i)
			{
				Item* item = inventory[i];
				if(item)
				{
					if(getSkull() == SKULL_RED || getSkull() == SKULL_BLACK || random_range(1, (item->getContainer() ? 100 : 1000)) <= getDropPercent())
					{
						g_game.internalMoveItem(this, corpse, INDEX_WHEREEVER, item, item->getItemCount(), 0);
						sendRemoveInventoryItem((slots_t)i, inventory[(slots_t)i]);
					}
				}
			}
		}
	}

	if(!inventory[SLOT_BACKPACK])
		__internalAddThing(SLOT_BACKPACK, Item::CreateItem(ITEM_BAG));
}

void Player::addStorageValue(const uint32_t key, const int32_t value, const bool isLogin/* = false*/)
{
	if(IS_IN_KEYRANGE(key, RESERVED_RANGE))
	{
		if(IS_IN_KEYRANGE(key, OUTFITS_RANGE))
		{
			Outfit outfit;
			outfit.looktype = value >> 16;
			outfit.addons = value & 0xFF;
			if(outfit.addons > 3)
				std::cout << "Warning: No valid addons value key:" << key << " value: " << (int32_t)value << " player: " << getName() << std::endl;
			else
				m_playerOutfits.addOutfit(outfit);

			return;
		}
		else if(IS_IN_KEYRANGE(key, MOUNTS_RANGE))
		{
			// do nothing
		}
		else
		{
			std::cout << "Warning: unknown reserved key: " << key << " player: " << getName() << std::endl;
			return;
		}
	}

	if(value == -1)
		storageMap.erase(key);
	else
	{
		storageMap[key] = value;
		if(!isLogin && Quests::getInstance()->isQuestStorage(key, value))
			sendTextMessage(MSG_EVENT_ADVANCE, "Your questlog has been updated.");
	}
}

bool Player::getStorageValue(const uint32_t key, int32_t& value) const
{
	StorageMap::const_iterator it;
	it = storageMap.find(key);
	if(it != storageMap.end())
	{
		value = it->second;
		return true;
	}
	else
	{
		value = -1;
		return false;
	}
}

bool Player::canSee(const Position& pos) const
{
	if(client)
		return client->canSee(pos);
	return false;
}

bool Player::canSeeCreature(const Creature* creature) const
{
	if(creature == this)
		return true;

	if(creature->isInGhostMode() && !accessLevel)
		return false;

	if(creature->isInvisible() && !creature->getPlayer() && !canSeeInvisibility())
		return false;

	return true;
}

void Player::onReceiveMail(uint32_t depotId)
{
	if(isNearDepotBox(depotId))
		sendTextMessage(MSG_INFO_DESCR, "New mail has arrived.");
}

bool Player::isNearDepotBox(uint32_t depotId)
{
	Position pos = getPosition();
	for(int32_t cx = -1; cx <= 1; ++cx)
	{
		for(int32_t cy = -1; cy <= 1; ++cy)
		{
			Tile* tile = g_game.getTile(pos.x + cx, pos.y + cy, pos.z);
			if(!tile)
				return false;

			if(!tile->hasFlag(TILESTATE_DEPOT))
				continue;

			for(uint32_t i = 0; i < tile->getThingCount(); ++i)
			{
				if(Item* item = tile->__getThing(i)->getItem())
				{
					const ItemType& it = Item::items[item->getID()];
					if(it.type == ITEM_TYPE_DEPOT)
					{
						Depot* depot = NULL;
						if(item->getContainer() && (depot = item->getContainer()->getDepot()))
						{
							if(depot->getDepotId() == depotId)
								return true;
						}
					}
				}
			}
		}
	}
	return false;
}

Depot* Player::getDepot(uint32_t depotId, bool autoCreateDepot)
{
	DepotMap::iterator it = depots.find(depotId);
	if(it != depots.end())
		return it->second;

	//depot does not yet exist

	//create a new depot?
	if(autoCreateDepot)
	{
		Depot* depot;
		Item* tmpDepot = Item::CreateItem(ITEM_LOCKER1);
		if(tmpDepot->getContainer() && (depot = tmpDepot->getContainer()->getDepot()))
		{
			Item* market = Item::CreateItem(ITEM_MARKET);
			depot->__internalAddThing(market);

			Item* inbox = Item::CreateItem(ITEM_INBOX);
			depot->__internalAddThing(inbox);
			depot->setInbox(inbox->getContainer());

			Item* depotChest = Item::CreateItem(ITEM_DEPOT);
			depot->__internalAddThing(depotChest);
			depot->setChest(depotChest->getContainer());

			addDepot(depot, depotId);
			return depot;
		}
		else
		{
			g_game.FreeThing(tmpDepot);
			std::cout << "Failure: Creating a new depot with id: " << depotId <<
				", for player: " << getName() << std::endl;
		}
	}
	return NULL;
}

bool Player::addDepot(Depot* depot, uint32_t depotId)
{
	if(getDepot(depotId, false))
		return false;

	for(ItemList::const_iterator it = depot->getItems(), end = depot->getEnd(); it != end; ++it)
	{
		Item* item = *it;
		if(item->getID() == ITEM_DEPOT)
		{
			depot->setChest(item->getContainer());
			if(depot->getInbox())
				break;
		}
		else if(item->getID() == ITEM_INBOX)
		{
			depot->setInbox(item->getContainer());
			if(depot->getChest())
				break;
		}
	}

	if(!depot->getChest())
	{
		Item* chest = Item::CreateItem(ITEM_DEPOT);
		depot->__internalAddThing(chest);
		depot->setChest(chest->getContainer());
		depotChange = true;
	}

	if(!depot->getInbox())
	{
		depot->__internalAddThing(Item::CreateItem(ITEM_MARKET));

		Item* inbox = Item::CreateItem(ITEM_INBOX);
		depot->__internalAddThing(inbox);
		depot->setInbox(inbox->getContainer());

		depot->moveChestToFront();
		depotChange = true;
	}

	depots[depotId] = depot;
	depot->setDepotId(depotId);
	depot->setMaxDepotLimit(maxDepotLimit);
	return true;
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
			sendCancel("Both hands need to be free.");
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
			sendCancel("You may only use one weapon.");
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
			sendCancel("This object is too heavy for you to carry.");
			break;

		case RET_CONTAINERNOTENOUGHROOM:
			sendCancel("You cannot put more objects in this container.");
			break;

		case RET_NEEDEXCHANGE:
		case RET_NOTENOUGHROOM:
			sendCancel("There is not enough room.");
			break;

		case RET_CANNOTPICKUP:
			sendCancel("You cannot take this object.");
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
			sendCancel("You cannot use this object.");
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
			sendCancel("This action is not permitted in a non pvp zone.");
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

		case RET_NAMEISTOOAMBIGIOUS:
			sendCancel("Name is too ambigious.");
			break;

		case RET_CANONLYUSEONESHIELD:
			sendCancel("You may use only one shield.");
			break;

		case RET_NOPARTYMEMBERSINRANGE:
			sendCancel("No party members in range.");
			break;

		case RET_YOUARENOTTHEOWNER:
			sendCancel("You are not the owner.");
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
	{
		client->sendStats();
		lastStatsTrainingTime = getOfflineTrainingTime() / 60 / 1000;
	}
}

void Player::sendPing()
{
	int64_t timeNow = OTSYS_TIME();
	if(timeNow - lastPing >= 5000)
	{
		lastPing = timeNow;
		if(client)
			client->sendPing();
	}

	if((timeNow - lastPong) >= 60000 && canLogout())
	{
		if(!client)
		{
			g_creatureEvents->playerLogout(this);
			g_game.removeCreature(this, true);
		}
		else
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

void Player::onAddTileItem(const Tile* tile, const Position& pos, const Item* item)
{
	Creature::onAddTileItem(tile, pos, item);
}

void Player::onUpdateTileItem(const Tile* tile, const Position& pos, const Item* oldItem,
	const ItemType& oldType, const Item* newItem, const ItemType& newType)
{
	Creature::onUpdateTileItem(tile, pos, oldItem, oldType, newItem, newType);

	if(oldItem != newItem)
		onRemoveTileItem(tile, pos, oldType, oldItem);

	if(tradeState != TRADE_TRANSFER)
	{
		if(tradeItem && oldItem == tradeItem)
			g_game.internalCloseTrade(this);
	}
}

void Player::onRemoveTileItem(const Tile* tile, const Position& pos, const ItemType& iType,
	const Item* item)
{
	Creature::onRemoveTileItem(tile, pos, iType, item);

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

void Player::onUpdateTile(const Tile* tile, const Position& pos)
{
	Creature::onUpdateTile(tile, pos);
}

void Player::onCreatureAppear(const Creature* creature, bool isLogin)
{
	Creature::onCreatureAppear(creature, isLogin);

	if(isLogin && creature == this)
	{
		Item* item;
		for(int32_t slot = SLOT_FIRST; slot < SLOT_LAST; ++slot)
		{
			if((item = getInventoryItem((slots_t)slot)))
			{
				item->__startDecaying();
				g_moveEvents->onPlayerEquip(this, item, (slots_t)slot, false);
			}
		}

		if(!storedConditionList.empty())
		{
			for(ConditionList::const_iterator it = storedConditionList.begin(); it != storedConditionList.end(); ++it)
				addCondition(*it);

			storedConditionList.clear();
		}

		BedItem* bed = Beds::getInstance().getBedBySleeper(getGUID());
		if(bed)
		{
			bed->wakeUp(this);
			#ifdef __DEBUG__
			std::cout << "Player " << getName() << " waking up." << std::endl;
			#endif
		}

		#ifndef _CONSOLE
		GUI::getInstance()->m_pBox.addPlayer(this);
		#endif
		std::cout << name << " has logged in." << std::endl;
		g_game.checkPlayersRecord();
		IOLoginData::getInstance()->updateOnlineStatus(guid, true);
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
	if(zone == ZONE_PROTECTION)
	{
		if(attackedCreature && !hasFlag(PlayerFlag_IgnoreProtectionZone))
		{
			setAttackedCreature(NULL);
			onAttackedCreatureDisappear(false);
		}

		if(!accessLevel && isMounted())
			dismount();
	}
	sendIcons();
}

void Player::onAttackedCreatureChangeZone(ZoneType_t zone)
{
	if(zone == ZONE_PROTECTION)
	{
		if(!hasFlag(PlayerFlag_IgnoreProtectionZone))
		{
			setAttackedCreature(NULL);
			onAttackedCreatureDisappear(false);
		}
	}
	else if(zone == ZONE_NOPVP)
	{
		if(attackedCreature->getPlayer())
		{
			if(!hasFlag(PlayerFlag_IgnoreProtectionZone))
			{
				setAttackedCreature(NULL);
				onAttackedCreatureDisappear(false);
			}
		}
	}
	else if(zone == ZONE_NORMAL)
	{
		//attackedCreature can leave a pvp zone if not pzlocked
		if(g_game.getWorldType() == WORLD_TYPE_NO_PVP)
		{
			if(attackedCreature->getPlayer())
			{
				setAttackedCreature(NULL);
				onAttackedCreatureDisappear(false);
			}
		}
	}
}

void Player::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	Creature::onCreatureDisappear(creature, stackpos, isLogout);

	if(creature == this)
	{
		if(isLogout)
			loginPosition = getPosition();

		lastLogout = time(NULL);

		if(eventWalk != 0)
			setFollowCreature(NULL);

		if(tradePartner)
			g_game.internalCloseTrade(this);

		closeShopWindow();

		clearPartyInvitations();
		if(getParty())
			getParty()->leaveParty(this);

		g_chat.removeUserFromAllChannels(this);

		#ifndef _CONSOLE
		GUI::getInstance()->m_pBox.removePlayer(this);
		#endif
		std::cout << getName() << " has logged out." << std::endl;
		IOLoginData::getInstance()->updateOnlineStatus(guid, false);

		bool saved = false;
		for(uint32_t tries = 0; tries < 3; ++tries)
		{
			if(IOLoginData::getInstance()->savePlayer(this, true))
			{
				saved = true;
				break;
			}
#ifdef __DEBUG_PLAYERS__
			else
				std::cout << "Error while saving player: " << getName() << ", strike " << tries << std::endl;
#endif
		}

		if(!saved)
#ifndef __DEBUG_PLAYERS__
			std::cout << "Error while saving player: " << getName() << std::endl;
#else
			std::cout << "Player " << getName() << " couldn't be saved." << std::endl;
#endif

#ifdef __DEBUG_PLAYERS__
		std::cout << (uint32_t)g_game.getPlayersOnline() << " players online." << std::endl;
#endif
	}
}

void Player::openShopWindow(Npc* npc, const std::list<ShopInfo>& shop)
{
	shopItemList = shop;
	sendShop(npc);
	sendSaleItemList();
}

void Player::closeShopWindow(bool sendCloseShopWindow /*= true*/)
{
	//unreference callbacks
	int32_t onBuy;
	int32_t onSell;

	Npc* npc = getShopOwner(onBuy, onSell);
	if(npc)
	{
		setShopOwner(NULL, -1, -1);
		npc->onPlayerEndTrade(this, onBuy, onSell);
		if(sendCloseShopWindow)
			sendCloseShop();
	}
	shopItemList.clear();
}

void Player::onWalk(Direction& dir)
{
	Creature::onWalk(dir);
	setNextActionTask(NULL);
	setNextAction(OTSYS_TIME() + getStepDuration(dir));
}

void Player::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	const Tile* oldTile, const Position& oldPos, bool teleport)
{
	Creature::onCreatureMove(creature, newTile, newPos, oldTile, oldPos, teleport);

	if(creature != this)
		return;

	if(tradeState != TRADE_TRANSFER)
	{
		//check if we should close trade
		if(tradeItem)
		{
			if(!Position::areInRange<1,1,0>(tradeItem->getPosition(), getPosition()))
				g_game.internalCloseTrade(this);
		}

		if(tradePartner)
		{
			if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), getPosition()))
				g_game.internalCloseTrade(this);
		}
	}

	if(getParty())
		getParty()->updateSharedExperience();

	if(teleport || oldPos.z != newPos.z)
	{
		int32_t ticks = g_config.getNumber(ConfigManager::STAIRHOP_DELAY);
		if(ticks > 0)
		{
			addCombatExhaust(ticks);
			addWeaponExhaust(ticks);
			if(Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_PACIFIED, ticks, 0))
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
	if(client)
	{
		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
		{
			if(cl->second == container)
				client->sendCloseContainer(cl->first);
		}
	}
}

void Player::onSendContainer(const Container* container)
{
	if(client)
	{
		bool hasParent = (dynamic_cast<const Container*>(container->getParent()) != NULL);
		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl)
		{
			if(cl->second == container)
				client->sendContainer(cl->first, container, hasParent);
		}
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

void Player::checkTradeState(const Item* item)
{
	if(tradeItem && tradeState != TRADE_TRANSFER)
	{
		if(tradeItem == item)
			g_game.internalCloseTrade(this);
		else
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
	}
}

void Player::setNextWalkActionTask(SchedulerTask* task)
{
	if(walkTaskEvent != 0)
	{
		g_scheduler.stopEvent(walkTaskEvent);
		walkTaskEvent = 0;
	}
	delete walkTask;
	walkTask = task;
}

void Player::setNextWalkTask(SchedulerTask* task)
{
	if(nextStepEvent != 0)
	{
		g_scheduler.stopEvent(nextStepEvent);
		nextStepEvent = 0;
	}

	if(task)
	{
		nextStepEvent = g_scheduler.addEvent(task);
		resetIdleTime();
	}
}

void Player::setNextActionTask(SchedulerTask* task)
{
	if(actionTaskEvent != 0)
	{
		g_scheduler.stopEvent(actionTaskEvent);
		actionTaskEvent = 0;
	}

	if(task)
	{
		actionTaskEvent = g_scheduler.addEvent(task);
		resetIdleTime();
	}
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

	sendPing();

	MessageBufferTicks += interval;
	if(MessageBufferTicks >= 1500)
	{
		MessageBufferTicks = 0;
		addMessageBuffer();
	}

	if(accountManager)
	{
		idleTime += interval;
		if(idleTime > 150000)
			kickPlayer(true);
		else if(client && idleTime == 120000)
			client->sendTextMessage(MSG_STATUS_WARNING, "You have been idle for two minutes, you will be disconnected in 30 seconds if you are still idle then.");
	}
	else if(!getTile()->hasFlag(TILESTATE_NOLOGOUT) && !mayNotMove && !isAccessPlayer())
	{
		idleTime += interval;
		if(idleTime > (g_config.getNumber(ConfigManager::KICK_AFTER_MINUTES) * 60000) + 60000)
			kickPlayer(true);
		else if(client && idleTime == 60000 * g_config.getNumber(ConfigManager::KICK_AFTER_MINUTES))
		{
			std::ostringstream ss;
			ss << "You have been idle for " << g_config.getNumber(ConfigManager::KICK_AFTER_MINUTES) << " minutes. You will be disconnected in one minute if you are still idle then.";
			client->sendTextMessage(MSG_STATUS_WARNING, ss.str());
		}
	}

	if(g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
		checkSkullTicks(interval);
}

uint32_t Player::isMuted()
{
	if(hasFlag(PlayerFlag_CannotBeMuted))
		return 0;

	int32_t muteTicks = 0;
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end(); ++it)
	{
		if((*it)->getType() == CONDITION_MUTED && (*it)->getTicks() > muteTicks)
			muteTicks = (*it)->getTicks();
	}
	return ((uint32_t)muteTicks / 1000);
}

void Player::addMessageBuffer()
{
	if(MessageBufferCount > 0 && Player::maxMessageBuffer != 0 && !hasFlag(PlayerFlag_CannotBeMuted))
		MessageBufferCount--;
}

void Player::removeMessageBuffer()
{
	if(!hasFlag(PlayerFlag_CannotBeMuted) && MessageBufferCount <= Player::maxMessageBuffer + 1 && Player::maxMessageBuffer != 0)
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
			Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_MUTED, muteTime * 1000, 0);
			addCondition(condition);

			std::ostringstream ss;
			ss << "You are muted for " << muteTime << " seconds.";
			sendTextMessage(MSG_STATUS_SMALL, ss.str());
		}
	}
}

void Player::drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage)
{
	Creature::drainHealth(attacker, combatType, damage);
	sendStats();
}

void Player::drainMana(Creature* attacker, int32_t manaLoss)
{
	Creature::drainMana(attacker, manaLoss);
	sendStats();
}

void Player::addManaSpent(uint64_t amount, bool withMultiplier /*= true*/)
{
	if(amount == 0 || hasFlag(PlayerFlag_NotGainMana))
		return;

	uint64_t currReqMana = vocation->getReqMana(magLevel);
	uint64_t nextReqMana = vocation->getReqMana(magLevel + 1);
	if(currReqMana > nextReqMana)
	{
		//player has reached max magic level
		return;
	}

	if(withMultiplier)
		amount *= g_config.getNumber(ConfigManager::RATE_MAGIC);

	bool sendUpdateStats = false;
	while((manaSpent + amount) >= nextReqMana)
	{
		amount -= nextReqMana - manaSpent;

		magLevel++;
		manaSpent = 0;

		std::ostringstream ss;
		ss << "You advanced to magic level " << magLevel << ".";
		sendTextMessage(MSG_EVENT_ADVANCE, ss.str());

		g_creatureEvents->playerAdvance(this, SKILL__MAGLEVEL, magLevel - 1, magLevel);

		sendUpdateStats = true;
		currReqMana = nextReqMana;
		nextReqMana = vocation->getReqMana(magLevel + 1);
		if(currReqMana > nextReqMana)
		{
			amount = 0;
			return;
		}
	}
	manaSpent += amount;

	uint32_t newPercent;
	if(nextReqMana > currReqMana)
		newPercent = Player::getPercentLevel(manaSpent, nextReqMana);
	else
		newPercent = 0;

	if(newPercent != magLevelPercent)
	{
		magLevelPercent = newPercent;
		sendUpdateStats = true;
	}

	if(sendUpdateStats)
		sendStats();
}

void Player::addExperience(uint64_t exp, bool useMult/* = false*/, bool sendText/* = false*/)
{
	int32_t newLevel = level;

	uint64_t nextLevelExp = Player::getExpForLevel(newLevel + 1);
	if(Player::getExpForLevel(newLevel) > nextLevelExp)
	{
		//player has reached max level
		levelPercent = 0;
		sendStats();
		return;
	}

	uint64_t gainExp = exp * (useMult ? g_game.getExperienceStage(level) : 1);
	experience += gainExp;
	if(sendText)
	{
		const Position& targetPos = getPosition();

		std::ostringstream ss;
		ss << "You gained " << gainExp << " experience points.";
		sendExperienceMessage(MSG_EXPERIENCE, ss.str(), targetPos, gainExp, TEXTCOLOR_WHITE_EXP);

		std::ostringstream ssExp;
		ssExp << getNameDescription() << " gained " << gainExp << " experience points.";
		std::string strExp = ssExp.str();

		SpectatorVec list;
		g_game.getSpectators(list, targetPos);
		Player* tmpPlayer = NULL;
		for(SpectatorVec::const_iterator it = list.begin(), end = list.end(); it != end; ++it)
		{
			if((tmpPlayer = (*it)->getPlayer()))
			{
				if(tmpPlayer != this)
					tmpPlayer->sendExperienceMessage(MSG_EXPERIENCE_OTHERS, strExp, targetPos, gainExp, TEXTCOLOR_WHITE_EXP);
			}
		}
	}

	while(experience >= nextLevelExp)
	{
		++newLevel;
		healthMax += vocation->getHPGain();
		health += vocation->getHPGain();
		manaMax += vocation->getManaGain();
		mana += vocation->getManaGain();
		capacity += vocation->getCapGain();
		nextLevelExp = Player::getExpForLevel(newLevel + 1);
		if(Player::getExpForLevel(newLevel) > nextLevelExp)
		{
			//player has reached max level
			break;
		}
	}

	int32_t prevLevel = level;
	if(prevLevel != newLevel)
	{
		level = newLevel;

		health = healthMax;
		mana = manaMax;

		updateBaseSpeed();

		int32_t newSpeed = getBaseSpeed();
		setBaseSpeed(newSpeed);

		g_game.changeSpeed(this, 0);
		g_game.addCreatureHealth(this);

		if(getParty())
			getParty()->updateSharedExperience();

		g_creatureEvents->playerAdvance(this, SKILL__LEVEL, prevLevel, newLevel);

		std::ostringstream ss;
		ss << "You advanced from Level " << prevLevel << " to Level " << newLevel << ".";
		sendTextMessage(MSG_EVENT_ADVANCE, ss.str());
	}

	uint64_t currLevelExp = Player::getExpForLevel(level);
	nextLevelExp = Player::getExpForLevel(level + 1);
	if(nextLevelExp > currLevelExp)
	{
		uint32_t newPercent = Player::getPercentLevel(experience - currLevelExp, Player::getExpForLevel(level + 1) - currLevelExp);
		levelPercent = newPercent;
	}
	else
		levelPercent = 0;

	sendStats();
}

uint32_t Player::getPercentLevel(uint64_t count, uint64_t nextLevelCount)
{
	if(nextLevelCount > 0)
	{
		uint32_t result = (count * 100) / nextLevelCount;
		if(result > 100)
			return 0;

		return result;
	}
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
	Item* item;

	item = getInventoryItem(SLOT_LEFT);
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
			if(it.abilities)
			{
				const int16_t& absorbPercent = it.abilities->absorbPercent[combatTypeToIndex(combatType)];
				if(absorbPercent != 0)
				{
					blocked += (int32_t)std::ceil((float)damage * absorbPercent / 100);
					if(item->hasCharges())
						g_game.transformItem(item, item->getID(), std::max((int32_t)0, (int32_t)item->getCharges() - 1));
				}
			}
		}

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

void Player::death()
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();)
	{
		if((*it)->isPersistent())
		{
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, CONDITIONEND_DEATH);
			onEndCondition(condition->getType());
			delete condition;
		}
		else
			++it;
	}

	loginPosition = masterPos;

	if(skillLoss)
	{
		//Magic level loss
		uint64_t sumMana = 0;
		uint64_t lostMana = 0;

		//sum up all the mana
		for(uint32_t i = 1; i <= magLevel; ++i)
			sumMana += vocation->getReqMana(i);

		sumMana += manaSpent;

		double lossPercent = getLostPercent();

		lostMana = (uint64_t)(sumMana * lossPercent);
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
		for(int16_t i = SKILL_FIRST; i <= SKILL_LAST; ++i) //for each skill
		{
			uint32_t sumSkillTries = 0;
			for(uint32_t c = 11; c <= skills[i][SKILL_LEVEL]; ++c) //sum up all required tries for all skill levels
				sumSkillTries += vocation->getReqSkillTries(i, c);

			sumSkillTries += skills[i][SKILL_TRIES];

			uint32_t lostSkillTries = (uint32_t)(sumSkillTries * lossPercent);
			while(lostSkillTries > skills[i][SKILL_TRIES])
			{
				lostSkillTries -= skills[i][SKILL_TRIES];
				skills[i][SKILL_TRIES] = vocation->getReqSkillTries(i, skills[i][SKILL_LEVEL]);
				if(skills[i][SKILL_LEVEL] > 10)
					skills[i][SKILL_LEVEL]--;
				else
				{
					skills[i][SKILL_LEVEL] = 10;
					skills[i][SKILL_TRIES] = 0;
					lostSkillTries = 0;
					break;
				}
			}
			skills[i][SKILL_TRIES] = std::max((int32_t)0, (int32_t)(skills[i][SKILL_TRIES] - lostSkillTries));
		}
		//

		//Level loss
		uint32_t newLevel = level;
		uint64_t newExperience = experience;
		if(vocationId == VOCATION_NONE || level > 7)
			newExperience -= getLostExperience();

		while(newExperience < Player::getExpForLevel(newLevel))
		{
			if(newLevel > 1)
				newLevel--;
			else
				break;
		}

		if(newLevel != level)
		{
			std::ostringstream ss;
			ss << "You were downgraded from Level " << level << " to Level " << newLevel << ".";
			sendTextMessage(MSG_EVENT_ADVANCE, ss.str());
		}

		uint64_t currLevelExp = Player::getExpForLevel(newLevel);
		uint64_t nextLevelExp = Player::getExpForLevel(newLevel + 1);
		if(nextLevelExp > currLevelExp)
			levelPercent = Player::getPercentLevel(experience - currLevelExp - uint64_t(experience * lossPercent), nextLevelExp - currLevelExp);
		else
			levelPercent = 0;

		sendReLoginWindow();
	}

	sendStats();
	sendSkills();
}

void Player::dropCorpse()
{
	if(getZone() == ZONE_PVP)
	{
		preSave();
		setDropLoot(true);
		setLossSkill(true);
		sendStats();
		g_game.internalTeleport(this, getTemplePosition(), true);
		g_game.addCreatureHealth(this);
		onThink(EVENT_CREATURE_THINK_INTERVAL);
		onIdleStatus();
	}
	else
		Creature::dropCorpse();
}

Item* Player::getCorpse()
{
	Item* corpse = Creature::getCorpse();
	if(corpse && corpse->getContainer())
	{
		Creature* lastHitCreature_ = NULL;
		Creature* mostDamageCreature = NULL;
		std::ostringstream ss;
		if(getKillers(&lastHitCreature_, &mostDamageCreature) && lastHitCreature_)
			ss << "You recognize " << getNameDescription() << ". " << (getSex() == PLAYERSEX_FEMALE ? "She" : "He") << " was killed by " << lastHitCreature_->getNameDescription() << ".";
		else
			ss << "You recognize " << getNameDescription() << ".";

		corpse->setSpecialDescription(ss.str());
	}
	return corpse;
}

void Player::preSave()
{
	if(health <= 0)
	{
		if(skillLoss)
		{
			if(vocationId == VOCATION_NONE || level > 7)
				experience -= getLostExperience();

			while(level > 1 && experience < Player::getExpForLevel(level))
			{
				--level;
				healthMax = std::max((int32_t)0, (healthMax - (int32_t)vocation->getHPGain()));
				manaMax = std::max((int32_t)0, (manaMax - (int32_t)vocation->getManaGain()));
				capacity = std::max((double)0, (capacity - (double)vocation->getCapGain()));
			}
			blessings = 0;

			mana = manaMax;
		}

		health = healthMax;

		if(getSkull() == SKULL_BLACK)
		{
			health = 40;
			mana = 0;
		}
	}
}

void Player::addWeaponExhaust(uint32_t ticks)
{
	Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_EXHAUST_WEAPON, ticks, 0);
	addCondition(condition);
}

void Player::addCombatExhaust(uint32_t ticks)
{
	Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_EXHAUST_COMBAT, ticks, 0);
	addCondition(condition);
}

void Player::addHealExhaust(uint32_t ticks)
{
	Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_EXHAUST_HEAL, ticks, 0);
	addCondition(condition);
}

void Player::addInFightTicks(bool pzlock /*= false*/)
{
	if(pzlock)
		pzLocked = true;

	Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_INFIGHT, g_game.getInFightTicks(), 0);
	addCondition(condition);
}

void Player::addDefaultRegeneration(uint32_t addTicks)
{
	Condition* condition = getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);

	if(condition)
		condition->setTicks(condition->getTicks() + addTicks);
	else
	{
		condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_REGENERATION, addTicks, 0);
		condition->setParam(CONDITIONPARAM_HEALTHGAIN, vocation->getHealthGainAmount());
		condition->setParam(CONDITIONPARAM_HEALTHTICKS, vocation->getHealthGainTicks() * 1000);
		condition->setParam(CONDITIONPARAM_MANAGAIN, vocation->getManaGainAmount());
		condition->setParam(CONDITIONPARAM_MANATICKS, vocation->getManaGainTicks() * 1000);
		addCondition(condition);
	}
}

void Player::removeList()
{
	listPlayer.removeList(getID());
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
		(*it).second->notifyLogOut(this);

	Status::getInstance()->removePlayer();
}

void Player::addList()
{
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
		(*it).second->notifyLogIn(this);
	listPlayer.addList(this);

	Status::getInstance()->addPlayer();
}

void Player::kickPlayer(bool displayEffect)
{
	if(client)
		client->logout(displayEffect, true);
	else
	{
		g_creatureEvents->playerLogout(this);
		g_game.removeCreature(this);
	}
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
	if(it != VIPList.end())
	{
		VIPList.erase(it);
		return true;
	}
	return false;
}

bool Player::addVIP(uint32_t _guid, std::string& name, bool isOnline, bool internal /*=false*/)
{
	if(guid == _guid)
	{
		if(!internal)
			sendTextMessage(MSG_STATUS_SMALL, "You cannot add yourself.");

		return false;
	}

	if(VIPList.size() > maxVipLimit || VIPList.size() == 200) // max number of buddies is 200 in 9.53
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
		Container* tmpcontainer = it->second;
		while(tmpcontainer != NULL)
		{
			if(tmpcontainer->isRemoved() || tmpcontainer == container)
			{
				closeList.push_back(it->first);
				break;
			}
			tmpcontainer = dynamic_cast<Container*>(tmpcontainer->getParent());
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

ReturnValue Player::__queryAdd(int32_t index, const Thing* thing, uint32_t count, uint32_t flags, Creature* actor/* = NULL*/) const
{
	const Item* item = thing->getItem();
	if(item == NULL)
		return RET_NOTPOSSIBLE;

	bool childIsOwner = hasBitSet(FLAG_CHILDISOWNER, flags);
	bool skipLimit = hasBitSet(FLAG_NOLIMIT, flags);
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
	const int32_t& slotPosition = item->getSlotPosition();
	if((slotPosition & SLOTP_HEAD) || (slotPosition & SLOTP_NECKLACE) ||
		(slotPosition & SLOTP_BACKPACK) || (slotPosition & SLOTP_ARMOR) ||
		(slotPosition & SLOTP_LEGS) || (slotPosition & SLOTP_FEET) ||
		(slotPosition & SLOTP_RING))
		ret = RET_CANNOTBEDRESSED;
	else if(slotPosition & SLOTP_TWO_HAND)
		ret = RET_PUTTHISOBJECTINBOTHHANDS;
	else if((slotPosition & SLOTP_RIGHT) || (slotPosition & SLOTP_LEFT))
		ret = RET_PUTTHISOBJECTINYOURHAND;

	switch(index)
	{
		case SLOT_HEAD:
		{
			if(slotPosition & SLOTP_HEAD)
				ret = RET_NOERROR;

			break;
		}

		case SLOT_NECKLACE:
		{
			if(slotPosition & SLOTP_NECKLACE)
				ret = RET_NOERROR;

			break;
		}

		case SLOT_BACKPACK:
		{
			if(slotPosition & SLOTP_BACKPACK)
				ret = RET_NOERROR;

			break;
		}

		case SLOT_ARMOR:
		{
			if(slotPosition & SLOTP_ARMOR)
				ret = RET_NOERROR;

			break;
		}

		case SLOT_RIGHT:
		{
			if(slotPosition & SLOTP_RIGHT)
			{
				//check if we already carry an item in the other hand
				if(slotPosition & SLOTP_TWO_HAND)
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
		}

		case SLOT_LEFT:
		{
			if(slotPosition & SLOTP_LEFT)
			{
				//check if we already carry an item in the other hand
				if(slotPosition & SLOTP_TWO_HAND)
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
		}

		case SLOT_LEGS:
		{
			if(slotPosition & SLOTP_LEGS)
				ret = RET_NOERROR;

			break;
		}

		case SLOT_FEET:
		{
			if(slotPosition & SLOTP_FEET)
				ret = RET_NOERROR;

			break;
		}

		case SLOT_RING:
		{
			if(slotPosition & SLOTP_RING)
				ret = RET_NOERROR;

			break;
		}

		case SLOT_AMMO:
		{
			if(slotPosition & SLOTP_AMMO)
				ret = RET_NOERROR;

			break;
		}

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

	if(index == INDEX_WHEREEVER)
	{
		uint32_t n = 0;
		for(int slotIndex = SLOT_FIRST; slotIndex < SLOT_LAST; ++slotIndex)
		{
			Item* inventoryItem = inventory[slotIndex];

			if(inventoryItem)
			{
				if(Container* subContainer = inventoryItem->getContainer())
				{
					uint32_t queryCount = 0;
					subContainer->__queryMaxCount(INDEX_WHEREEVER, item, item->getItemCount(), queryCount, flags);
					n += queryCount;

					//iterate through all items, including sub-containers (deep search)
					for(ContainerIterator cit = subContainer->begin(); cit != subContainer->end(); ++cit)
					{
						if(Container* tmpContainer = (*cit)->getContainer())
						{
							queryCount = 0;
							tmpContainer->__queryMaxCount(INDEX_WHEREEVER, item, item->getItemCount(), queryCount, flags);
							n += queryCount;
						}
					}
				}
				else if(inventoryItem->isStackable() && item->getID() == inventoryItem->getID() && inventoryItem->getItemCount() < 100)
				{
					uint32_t remainder = (100 - inventoryItem->getItemCount());
					if(__queryAdd(slotIndex, item, remainder, flags) == RET_NOERROR)
						n += remainder;
				}
			}
			else if(__queryAdd(slotIndex, item, item->getItemCount(), flags) == RET_NOERROR) //empty slot
			{
				if(item->isStackable())
					n += 100;
				else
					n += 1;
			}
		}

		maxQueryCount = n;
	}
	else
	{
		const Thing* destThing = __getThing(index);
		const Item* destItem = NULL;
		if(destThing)
			destItem = destThing->getItem();

		if(destItem)
		{
			if(destItem->isStackable() && item->getID() == destItem->getID() && destItem->getItemCount() < 100)
				maxQueryCount = 100 - destItem->getItemCount();
			else
				maxQueryCount = 0;
		}
		else if(__queryAdd(index, item, count, flags) == RET_NOERROR) //empty slot
		{
			if(item->isStackable())
				maxQueryCount = 100;
			else
				maxQueryCount = 1;

			return RET_NOERROR;
		}
	}

	if(maxQueryCount < count)
		return RET_NOTENOUGHROOM;
	else
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
		if(item == NULL)
			return this;

		bool autoStack = !((flags & FLAG_IGNOREAUTOSTACK) == FLAG_IGNOREAUTOSTACK);

		std::list<Container*> containerList;
		for(uint32_t slotIndex = SLOT_FIRST; slotIndex < SLOT_LAST; ++slotIndex)
		{
			Item* inventoryItem = inventory[slotIndex];
			if(inventoryItem)
			{
				if(inventoryItem == tradeItem)
					continue;

				if(inventoryItem == item)
					continue;

				if(autoStack && item->isStackable())
				{
					//try find an already existing item to stack with
					if(__queryAdd(slotIndex, item, item->getItemCount(), 0) == RET_NOERROR)
					{
						if(inventoryItem->getID() == item->getID() && inventoryItem->getItemCount() < 100)
						{
							index = slotIndex;
							*destItem = inventoryItem;
							return this;
						}
					}

					if(Container* subContainer = inventoryItem->getContainer())
						containerList.push_back(subContainer);
				}
				else if(Container* subContainer = inventoryItem->getContainer())
				{
					if(subContainer->__queryAdd(INDEX_WHEREEVER, item, item->getItemCount(), flags) == RET_NOERROR)
					{
						index = INDEX_WHEREEVER;
						*destItem = NULL;
						return subContainer;
					}

					containerList.push_back(subContainer);
				}
			}
			else if(__queryAdd(slotIndex, item, item->getItemCount(), flags) == RET_NOERROR) //empty slot
			{
				index = slotIndex;
				*destItem = NULL;
				return this;
			}
		}

		while(!containerList.empty())
		{
			Container* tmpContainer = containerList.front();
			containerList.pop_front();

			if(!(autoStack && item->isStackable()))
			{
				//we need to find first empty container as fast as we can for non-stackable items
				uint32_t n = tmpContainer->capacity() - tmpContainer->size();
				while(n)
				{
					if(tmpContainer->__queryAdd(tmpContainer->capacity() - n, item, item->getItemCount(), flags) == RET_NOERROR)
					{
						index = tmpContainer->capacity() - n;
						*destItem = NULL;
						return tmpContainer;
					}
					n--;
				}

				for(ItemList::const_iterator it = tmpContainer->getItems(), end = tmpContainer->getEnd(); it != end; ++it)
				{
					if(Container* subContainer = (*it)->getContainer())
						containerList.push_back(subContainer);
				}
				continue;
			}

			uint32_t n = 0;
			for(ItemList::const_iterator it = tmpContainer->getItems(), end = tmpContainer->getEnd(); it != end; ++it)
			{
				Item* tmpItem = *it;
				if(tmpItem == tradeItem)
					continue;

				if(tmpItem == item)
					continue;

				//try find an already existing item to stack with
				if(tmpItem != item && tmpItem->getID() == item->getID() && tmpItem->getItemCount() < 100)
				{
					index = n;
					*destItem = tmpItem;
					return tmpContainer;
				}

				if(Container* subContainer = tmpItem->getContainer())
					containerList.push_back(subContainer);

				n++;
			}

			if(n < tmpContainer->capacity() && tmpContainer->__queryAdd(n, item, item->getItemCount(), flags) == RET_NOERROR)
			{
				index = n;
				*destItem = NULL;
				return tmpContainer;
			}
		}

		return this;
	}

	Thing* destThing = __getThing(index);
	if(destThing)
		*destItem = destThing->getItem();

	Cylinder* subCylinder = dynamic_cast<Cylinder*>(destThing);
	if(subCylinder)
	{
		index = INDEX_WHEREEVER;
		*destItem = NULL;
		return subCylinder;
	}
	else
		return this;
}

void Player::__addThing(Thing* thing)
{
	__addThing(0, thing);
}

void Player::__addThing(int32_t index, Thing* thing)
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
	if(index > 11)
	{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__replaceThing], " << "player: " << getName() << ", index: " << index << ", index < 0 || index > 11" << std::endl;
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
			uint8_t newCount = (uint8_t)std::max((int32_t)0, (int32_t)(item->getItemCount() - count));
			item->setItemCount(newCount);

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
	for(int i = SLOT_FIRST; i < SLOT_LAST; ++i)
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

uint32_t Player::__getItemTypeCount(uint16_t itemId, int32_t subType /*= -1*/) const
{
	uint32_t count = 0;
	for(int i = SLOT_FIRST; i < SLOT_LAST; i++)
	{
		Item* item = inventory[i];
		if(!item)
			continue;

		if(item->getID() == itemId)
			count += Item::countByType(item, subType);
		else if(Container* container = item->getContainer())
		{
			for(ContainerIterator it = container->begin(), end = container->end(); it != end; ++it)
			{
				if((*it)->getID() == itemId)
					count += Item::countByType(*it, subType);
			}
		}
	}
	return count;
}

std::map<uint32_t, uint32_t>& Player::__getAllItemTypeCount(std::map<uint32_t, uint32_t>& countMap) const
{
	for(int i = SLOT_FIRST; i < SLOT_LAST; i++)
	{
		Item* item = inventory[i];
		if(!item)
			continue;

		countMap[item->getID()] += Item::countByType(item, -1);
		if(Container* container = item->getContainer())
		{
			for(ContainerIterator it = container->begin(), end = container->end(); it != end; ++it)
				countMap[(*it)->getID()] += Item::countByType(*it, -1);
		}
	}

	return countMap;
}

Thing* Player::__getThing(uint32_t index) const
{
	if(index >= SLOT_FIRST && index < SLOT_LAST)
		return inventory[index];

	return NULL;
}

void Player::postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(link == LINK_OWNER)
	{
		//calling movement scripts
		g_moveEvents->onPlayerEquip(this, thing->getItem(), (slots_t)index, false);
	}

	bool requireListUpdate = true;
	if(link == LINK_OWNER || link == LINK_TOPPARENT)
	{
		const Item* i = (oldParent? oldParent->getItem() : NULL);

		// Check if we owned the old container too, so we don't need to do anything,
		// as the list was updated in postRemoveNotification
		assert(i? i->getContainer() != NULL : true);

		if(i)
			requireListUpdate = i->getContainer()->getHoldingPlayer() != this;
		else
			requireListUpdate = oldParent != this;

		updateInventoryWeight();
		updateItemsLight();
		sendStats();
	}

	if(const Item* item = thing->getItem())
	{
		if(const Container* container = item->getContainer())
			onSendContainer(container);

		if(shopOwner && requireListUpdate)
			updateSaleShopList(item->getID());
	}
	else if(const Creature* creature = thing->getCreature())
	{
		if(creature == this)
		{
			//check containers
			std::vector<Container*> containers;
			for(ContainerVector::iterator it = containerVec.begin(); it != containerVec.end(); ++it)
			{
				if(!Position::areInRange<1,1,0>(it->second->getPosition(), getPosition()))
					containers.push_back(it->second);
			}

			for(std::vector<Container*>::const_iterator it = containers.begin(); it != containers.end(); ++it)
				autoCloseContainers(*it);
		}
	}
}

void Player::postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(link == LINK_OWNER)
	{
		//calling movement scripts
		g_moveEvents->onPlayerDeEquip(this, thing->getItem(), (slots_t)index, isCompleteRemoval);
	}

	bool requireListUpdate = true;
	if(link == LINK_OWNER || link == LINK_TOPPARENT)
	{
		const Item* i = (newParent? newParent->getItem() : NULL);

		// Check if we owned the old container too, so we don't need to do anything,
		// as the list was updated in postRemoveNotification
		assert(i? i->getContainer() != NULL : true);
		if(i)
			requireListUpdate = i->getContainer()->getHoldingPlayer() != this;
		else
			requireListUpdate = newParent != this;

		updateInventoryWeight();
		updateItemsLight();
		sendStats();
	}

	if(const Item* item = thing->getItem())
	{
		if(const Container* container = item->getContainer())
		{
			if(container->isRemoved() || !Position::areInRange<1,1,0>(getPosition(), container->getPosition()))
				autoCloseContainers(container);
			else if(container->getTopParent() == this)
				onSendContainer(container);
			else if(const Container* topContainer = dynamic_cast<const Container*>(container->getTopParent()))
			{
				if(const Depot* depot = dynamic_cast<const Depot*>(topContainer))
				{
					bool isOwner = false;
					for(DepotMap::iterator it = depots.begin(); it != depots.end(); ++it)
					{
						if(it->second == depot)
						{
							isOwner = true;
							onSendContainer(container);
						}
					}

					if(!isOwner)
						autoCloseContainers(container);
				}
				else
					onSendContainer(container);
			}
			else
				autoCloseContainers(container);
		}

		if(shopOwner && requireListUpdate)
			updateSaleShopList(item->getID());
	}
}

void Player::updateSaleShopList(uint32_t itemId)
{
	for(std::list<ShopInfo>::const_iterator it = shopItemList.begin(); it != shopItemList.end(); ++it)
	{
		if(it->itemId == itemId)
		{
			if(client)
				client->sendSaleItemList(shopItemList);

			break;
		}
	}
}

bool Player::hasShopItemForSale(uint32_t itemId, uint8_t subType)
{
	for(std::list<ShopInfo>::const_iterator it = shopItemList.begin(); it != shopItemList.end(); ++it)
	{
		if(it->itemId == itemId && (*it).buyPrice > 0)
		{
			const ItemType& iit = Item::items[itemId];
			if(iit.isFluidContainer() || iit.isSplash())
			{
				if(it->subType == subType)
					return true;
			}
			else
				return true;
		}
	}
	return false;
}

void Player::__internalAddThing(Thing* thing)
{
	__internalAddThing(0, thing);
}

void Player::__internalAddThing(uint32_t index, Thing* thing)
{
#ifdef __DEBUG__MOVESYS__NOTICE
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
		stopWalk();
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
	{
		g_dispatcher.addTask(createTask(
			boost::bind(&Game::checkCreatureAttack, &g_game, getID())));
	}
	return true;
}

void Player::goToFollowCreature()
{
	if(!walkTask)
		Creature::goToFollowCreature();
}

void Player::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	Creature::getPathSearchParams(creature, fpp);
	fpp.fullPathSearch = true;
}

void Player::onAttacking(uint32_t interval)
{
	Creature::onAttacking(interval);
}

void Player::doAttacking(uint32_t interval)
{
	if(lastAttack == 0)
		lastAttack = OTSYS_TIME() - getAttackSpeed() - 1;

	if(hasCondition(CONDITION_PACIFIED))
		return;

	if((OTSYS_TIME() - lastAttack) >= getAttackSpeed())
	{
		Item* tool = getWeapon();
		const Weapon* weapon = g_weapons->getWeapon(tool);

		bool result = false;
		if(weapon)
		{
			if(!weapon->interruptSwing())
				result = weapon->useWeapon(this, tool, attackedCreature);
			else if(!canDoAction())
			{
				uint32_t delay = getNextActionTime();
				SchedulerTask* task = createSchedulerTask(delay, boost::bind(&Game::checkCreatureAttack,
					&g_game, getID()));
				setNextActionTask(task);
			}
			else if(!hasCondition(CONDITION_EXHAUST_COMBAT) || !weapon->hasExhaustion())
				result = weapon->useWeapon(this, tool, attackedCreature);
		}
		else
			result = Weapon::useFist(this, attackedCreature);

		if(result)
			lastAttack = OTSYS_TIME();
	}
}

uint64_t Player::getGainedExperience(Creature* attacker) const
{
	if(g_config.getBoolean(ConfigManager::EXPERIENCE_FROM_PLAYERS))
	{
		Player* attackerPlayer = attacker->getPlayer();
		if(attackerPlayer && attackerPlayer != this && skillLoss)
		{
			/*
				Formula
				a = attackers level * 0.9
				b = victims level
				c = victims experience

				y = (1 - (a / b)) * 0.05 * c
			*/

			uint32_t a = (uint32_t)std::floor(attackerPlayer->getLevel() * 0.9);
			uint32_t b = getLevel();
			uint64_t c = getExperience();

			uint64_t result = std::max((uint64_t)0, (uint64_t)std::floor(getDamageRatio(attacker) * std::max((double)0, ((double)(1 - (((double)a / b))))) * 0.05 * c));
			return (result * g_config.getNumber(ConfigManager::RATE_EXPERIENCE));
		}
	}
	return 0;
}

void Player::onFollowCreature(const Creature* creature)
{
	if(!creature)
		stopWalk();
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
			cancelNextWalk = true;
		}
	}
}

void Player::setFightMode(fightMode_t mode)
{
	fightMode = mode;
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
		walkTaskEvent = g_scheduler.addEvent(walkTask);
		walkTask = NULL;
	}
}

void Player::stopWalk()
{
	cancelNextWalk = true;
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

		case CONDITION_BLEEDING:
			sendTextMessage(MSG_STATUS_DEFAULT, "You are bleeding.");
			break;

		default:
			break;
	}
}

void Player::onEndCondition(ConditionType_t type)
{
	Creature::onEndCondition(type);
	if(type == CONDITION_INFIGHT)
	{
		onIdleStatus();
		pzLocked = false;

		if(getSkull() != SKULL_RED && getSkull() != SKULL_BLACK)
		{
			clearAttacked();
			setSkull(SKULL_NONE);
			g_game.updateCreatureSkull(this);
		}
	}
	sendIcons();
}

void Player::onCombatRemoveCondition(const Creature* attacker, Condition* condition)
{
	//Creature::onCombatRemoveCondition(attacker, condition);
	bool remove = true;

	if(condition->getId() > 0)
	{
		remove = false;

		//Means the condition is from an item, id == slot
		if(g_game.getWorldType() == WORLD_TYPE_PVP_ENFORCED)
		{
			Item* item = getInventoryItem((slots_t)condition->getId());
			if(item)
			{
				//25% chance to destroy the item
				if(25 >= random_range(0, 100))
					g_game.internalRemoveItem(item);
			}
		}
	}

	if(remove)
	{
		if(!canDoAction())
		{
			int32_t delay = getNextActionTime();
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

	if(!hasFlag(PlayerFlag_NotGainInFight))
	{
		if(target != this)
		{
			if(Player* targetPlayer = target->getPlayer())
			{
				if(!isPartner(targetPlayer) && !targetPlayer->hasAttacked(this))
				{
					if(!pzLocked)
					{
						pzLocked = true;
						sendIcons();
					}

					if(!Combat::isInPvpZone(this, targetPlayer) && !isInWar(targetPlayer) && !isGuildMate(targetPlayer))
					{
						addAttacked(targetPlayer);
						if(targetPlayer->getSkull() == SKULL_NONE && getSkull() == SKULL_NONE)
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
		addInFightTicks();
	}
}

void Player::onAttacked()
{
	Creature::onAttacked();

	if(!hasFlag(PlayerFlag_NotGainInFight))
		addInFightTicks();
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
	if(target)
	{
		if(getParty() && !Combat::isPlayerCombat(target))
		{
			Monster* tmpMonster = target->getMonster();
			if(tmpMonster && tmpMonster->isHostile())
			{
				//We have fulfilled a requirement for shared experience
				getParty()->addPlayerDamageMonster(this, points);
			}
		}
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
		else if(Creature* targetMaster = target->getMaster())
		{
			if(Player* targetMasterPlayer = targetMaster->getPlayer())
				tmpPlayer = targetMasterPlayer;
		}

		if(isPartner(tmpPlayer))
			getParty()->addPlayerHealedMember(this, points);
	}
}

bool Player::onKilledCreature(Creature* target, bool lastHit/* = true*/)
{
	bool unjustified = false;
	if(hasFlag(PlayerFlag_NotGenerateLoot))
		target->setDropLoot(false);

	Creature::onKilledCreature(target, lastHit);

	if(Player* targetPlayer = target->getPlayer())
	{
		if(targetPlayer && targetPlayer->getZone() == ZONE_PVP)
		{
			targetPlayer->setDropLoot(false);
			targetPlayer->setLossSkill(false);
		}
		else if(!hasFlag(PlayerFlag_NotGainInFight) && !isPartner(targetPlayer))
		{
			if(!Combat::isInPvpZone(this, targetPlayer) &&
				!targetPlayer->hasAttacked(this) &&
				targetPlayer->getSkull() == SKULL_NONE &&
				!isInWar(targetPlayer) &&
				!isGuildMate(targetPlayer) &&
				targetPlayer != this)
			{
				addUnjustifiedDead(targetPlayer);
				unjustified = true;
			}

			if(lastHit && !Combat::isInPvpZone(this, targetPlayer) && hasCondition(CONDITION_INFIGHT))
			{
				pzLocked = true;
				Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_INFIGHT, g_config.getNumber(ConfigManager::WHITE_SKULL_TIME), 0);
				addCondition(condition);
			}
		}
	}
	return unjustified;
}

void Player::gainExperience(uint64_t gainExp)
{
	if(!hasFlag(PlayerFlag_NotGainExperience) && gainExp > 0)
	{
		//soul regeneration
		if(gainExp >= getLevel())
		{
			Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_SOUL, 4 * 60 * 1000, 0);
			condition->setParam(CONDITIONPARAM_SOULGAIN, 1);
			condition->setParam(CONDITIONPARAM_SOULTICKS, vocation->getSoulGainTicks() * 1000);
			addCondition(condition);
		}

		addExperience(gainExp);
	}
}

void Player::onGainExperience(uint64_t gainExp, Creature* target)
{
	if(hasFlag(PlayerFlag_NotGainExperience))
		gainExp = 0;

	Party* party = getParty();
	if(target && !target->getPlayer() && party && party->isSharedExperienceActive() && party->isSharedExperienceEnabled())
	{
		party->shareExperience(gainExp);
		//We will get a share of the experience through the sharing mechanism
		gainExp = 0;
	}

	Creature::onGainExperience(gainExp, target);
	gainExperience(gainExp);
}

void Player::onGainSharedExperience(uint64_t gainExp)
{
	Creature::onGainSharedExperience(gainExp);
	gainExperience(gainExp);
}

bool Player::isImmune(CombatType_t type) const
{
	if(hasFlag(PlayerFlag_CannotBeAttacked))
		return true;

	return Creature::isImmune(type);
}

bool Player::isImmune(ConditionType_t type) const
{
	if(hasFlag(PlayerFlag_CannotBeAttacked))
		return true;

	return Creature::isImmune(type);
}

bool Player::isAttackable() const
{
	return !hasFlag(PlayerFlag_CannotBeAttacked);
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
	if(soulChange > 0)
		soul += std::min(soulChange, soulMax - soul);
	else
		soul = std::max((int32_t)0, soul + soulChange);
	sendStats();
}

const OutfitListType& Player::getPlayerOutfits()
{
	return m_playerOutfits.getOutfits();
}

bool Player::canWear(uint32_t _looktype, uint32_t _addons)
{
	return m_playerOutfits.isInList(_looktype, _addons, isPremium(), getSex()) || accessLevel;
}

bool Player::canLogout()
{
	if(isConnecting)
		return false;

	if(getTile()->hasFlag(TILESTATE_NOLOGOUT))
		return false;

	if(getTile()->hasFlag(TILESTATE_PROTECTIONZONE))
		return true;

	return !isPzLocked() && !hasCondition(CONDITION_INFIGHT);
}

void Player::genReservedStorageRange()
{
	uint32_t base_key;
	//generate outfits range
	base_key = PSTRG_OUTFITS_RANGE_START + 1;

	const OutfitList& global_outfits = Outfits::getInstance()->getOutfitList(sex);

	const OutfitListType& outfits = m_playerOutfits.getOutfits();
	OutfitListType::const_iterator it;
	for(it = outfits.begin(); it != outfits.end(); ++it)
	{
		uint32_t looktype = (*it)->looktype;
		uint32_t addons = (*it)->addons;
		if(!global_outfits.isInList(looktype, addons, isPremium(), getSex()))
		{
			long value = (looktype << 16) | (addons & 0xFF);
			storageMap[base_key] = value;
			base_key++;
			if(base_key > PSTRG_OUTFITS_RANGE_START + PSTRG_OUTFITS_RANGE_SIZE)
			{
				std::cout << "Warning: [Player::genReservedStorageRange()] Player " << getName() << " with more than 500 outfits!" << std::endl;
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

uint32_t Player::getOutfitAddons(uint32_t looktype)
{
	const OutfitListType& outfits = m_playerOutfits.getOutfits();
	for(OutfitListType::const_iterator it = outfits.begin(); it != outfits.end(); ++it)
	{
		if((*it)->looktype == looktype)
			return (*it)->addons;
	}
	return 0;
}

void Player::setSex(PlayerSex_t newSex)
{
	sex = newSex;
	Outfits* outfits = Outfits::getInstance();
	const OutfitListType& global_outfits = outfits->getOutfits(sex);
	OutfitListType::const_iterator it;
	Outfit outfit;
	for(it = global_outfits.begin(); it != global_outfits.end(); ++it)
	{
		outfit.looktype = (*it)->looktype;
		outfit.addons = (*it)->addons;
		outfit.premium = (*it)->premium;
		m_playerOutfits.addOutfit(outfit);
	}
}

Skulls_t Player::getSkull() const
{
	if(hasFlag(PlayerFlag_NotGainInFight))
		return SKULL_NONE;

	return skull;
}

Skulls_t Player::getSkullClient(const Player* player) const
{
	if(!player || g_game.getWorldType() != WORLD_TYPE_PVP)
		return SKULL_NONE;

	if(player->getSkull() != SKULL_RED && player->getSkull() != SKULL_BLACK)
	{
		if(isInWar(player))
			return SKULL_YELLOW;

		if(getSkull() != SKULL_NONE)
		{
			if(player->hasAttacked(this))
				return SKULL_YELLOW;
		}
	}

	if(player->getSkull() == SKULL_NONE)
	{
		if(isPartner(player))
			return SKULL_GREEN;
	}
	return player->getSkull();
}

bool Player::hasAttacked(const Player* attacked) const
{
	if(hasFlag(PlayerFlag_NotGainInFight) || !attacked)
		return false;

	AttackedSet::const_iterator it;
	uint32_t attackedId = attacked->getID();
	it = attackedSet.find(attackedId);
	return it != attackedSet.end();
}

void Player::addAttacked(const Player* attacked)
{
	if(hasFlag(PlayerFlag_NotGainInFight) || !attacked || attacked == this)
		return;

	AttackedSet::iterator it;
	uint32_t attackedId = attacked->getID();
	it = attackedSet.find(attackedId);
	if(it == attackedSet.end())
		attackedSet.insert(attackedId);
}

void Player::clearAttacked()
{
	attackedSet.clear();
}

void Player::addUnjustifiedDead(const Player* attacked)
{
	if(hasFlag(PlayerFlag_NotGainInFight) || attacked == this || g_game.getWorldType() == WORLD_TYPE_PVP_ENFORCED)
		return;

	if(client)
	{
		std::ostringstream ss;
		ss << "Warning! The murder of " << attacked->getName() << " was not justified.";
		client->sendTextMessage(MSG_EVENT_ADVANCE, ss.str());
	}

	skullTicks += g_config.getNumber(ConfigManager::FRAG_TIME);
	if(g_config.getNumber(ConfigManager::KILLS_TO_BAN) != 0 && skullTicks > (g_config.getNumber(ConfigManager::KILLS_TO_BAN) - 1) * g_config.getNumber(ConfigManager::FRAG_TIME) && !IOBan::getInstance()->isAccountBanned(accountNumber))
	{
		IOBan::getInstance()->addAccountBan(accountNumber, time(NULL) + (g_config.getNumber(ConfigManager::BAN_DAYS) * 86400), 20, 2, "No comment.", 0);

		uint32_t playerId = getID();
		g_game.addMagicEffect(getPosition(), NM_ME_MAGIC_POISON);
		g_scheduler.addEvent(createSchedulerTask(500,
			boost::bind(&Game::kickPlayer, &g_game, playerId, false)));
	}
	else if(getSkull() != SKULL_BLACK && g_config.getNumber(ConfigManager::KILLS_TO_BLACK) != 0 && skullTicks > (g_config.getNumber(ConfigManager::KILLS_TO_BLACK) - 1) * g_config.getNumber(ConfigManager::FRAG_TIME))
	{
		setSkull(SKULL_BLACK);
		g_game.updateCreatureSkull(this);
	}
	else if(getSkull() != SKULL_RED && g_config.getNumber(ConfigManager::KILLS_TO_RED) != 0 && skullTicks > (g_config.getNumber(ConfigManager::KILLS_TO_RED) - 1) * g_config.getNumber(ConfigManager::FRAG_TIME))
	{
		setSkull(SKULL_RED);
		g_game.updateCreatureSkull(this);
	}
}

void Player::checkSkullTicks(int32_t ticks)
{
	int32_t newTicks = skullTicks - ticks;
	if(newTicks < 0)
		skullTicks = 0;
	else
		skullTicks = newTicks;

	if((skull == SKULL_RED || skull == SKULL_BLACK) && skullTicks < 1000 && !hasCondition(CONDITION_INFIGHT))
	{
		setSkull(SKULL_NONE);
		g_game.updateCreatureSkull(this);
	}
}

bool Player::isPromoted() const
{
	int32_t promotedVocation = g_vocations.getPromotedVocation(vocationId);
	return promotedVocation == 0 && vocationId != promotedVocation;
}

double Player::getLostPercent() const
{
	std::bitset<5> bitset(blessings);

	const int32_t deathLosePercent = g_config.getNumber(ConfigManager::DEATH_LOSE_PERCENT);
	if(deathLosePercent != -1)
	{
		int32_t lossPercent = deathLosePercent;
		if(isPromoted())
			lossPercent -= 3;

		lossPercent -= (int32_t)bitset.count();
		return (double)std::max(0, lossPercent) / 100;
	}
	else
	{
		double lossPercent;
		if(level >= 25)
		{
			double tmpLevel = level + (levelPercent / 100.);
			lossPercent = (double)((tmpLevel + 50) * 50 * ((tmpLevel * tmpLevel) - (5 * tmpLevel) + 8)) / experience;
		}
		else
			lossPercent = 10;

		if(isPromoted())
			lossPercent *= 0.7;

		return lossPercent * pow(0.92, (int32_t)bitset.count()) / 100;
	}
}

void Player::learnInstantSpell(const std::string& name)
{
	if(!hasLearnedInstantSpell(name))
		learnedInstantSpellList.push_back(name);
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
	std::ostringstream msg;
	msg << "Account Manager: ";
	if(accountManager->namelockedPlayerName != "")
	{
		if(accountManager->talkState == 0)
		{
			accountManager->accountManagerInput = text;
			trimString(accountManager->accountManagerInput);
			if(accountManager->accountManagerInput.length() < 4)
				msg << "The name you want is too short, please select a longer name.";
			else if(accountManager->accountManagerInput.length() > 20)
				msg << "The name you want is too long, please select a shorter name.";
			else
			{
				std::string lowerCaseInput = asLowerCaseString(accountManager->accountManagerInput);
				if(lowerCaseInput.substr(0, 4) == "god " || lowerCaseInput.substr(0, 3) == "gm " || lowerCaseInput.substr(0, 3) == "cm ")
					msg << "You are not a gamemaster, please pick another name.";
				else if(IOLoginData::getInstance()->playerExists(accountManager->accountManagerInput))
					msg << "A player with this name currently exists, please choose another name.";
				else if(!isValidName(accountManager->accountManagerInput))
					msg << "That name seems to contain invalid symbols tell me another name.";
				else
				{
					accountManager->talkState++;
					msg << accountManager->accountManagerInput << ", are you sure?";
				}
			}
		}
		else if(accountManager->talkState == 1 && checkText(text, "no"))
		{
			accountManager->talkState--;
			msg << "What else would you like to name your character?";
		}
		else if(accountManager->talkState == 1 && checkText(text, "yes"))
		{
			if(!IOLoginData::getInstance()->playerExists(accountManager->accountManagerInput))
			{
				uint32_t _guid;
				IOLoginData::getInstance()->getGuidByName(_guid, accountManager->namelockedPlayerName);
				if(IOLoginData::getInstance()->changeName(_guid, accountManager->accountManagerInput))
				{
					IOBan::getInstance()->removePlayerNamelock(_guid);

					accountManager->talkState++;
					msg << "Your character has successfully been renamed, you should now be able to login at it without any problems.";
				}
				else
				{
					accountManager->talkState--;
					msg << "Failed to change your name, please try another name.";
				}
			}
			else
			{
				accountManager->talkState--;
				msg << "A player with that name already exists, please pick another name.";
			}
		}
		else
			msg << "Sorry, but I can't understand you, please try to repeat that!";
	}
	else if(accountManager->managingAccount)
	{
		if(checkText(text, "cancel") || (accountManager->talkState == 0 && checkText(text, "account")))
		{
			accountManager->talkState = 1;
			msg << "Do you want to change your 'password', request a 'recovery key', add a 'character', or 'delete' a character?";
		}
		else if(accountManager->talkState == 1)
		{
			if(checkText(text, "delete"))
			{
				accountManager->talkState = 2;
				msg << "Which character would you like to delete?";
			}
			else if(checkText(text, "password"))
			{
				accountManager->talkState = 4;
				msg << "What would you like to change your password to?";
			}
			else if(checkText(text, "character"))
			{
				Account account = IOLoginData::getInstance()->loadAccount(accountManager->realAccount);
				if(account.charList.size() <= 15)
				{
					accountManager->talkState = 6;
					msg << "What would you like as your character name?";
				}
				else
					msg << "Your account has reached the limit of 15 players, you must 'delete' a character if you want to create a new one.";
			}
			else if(checkText(text, "recovery key"))
			{
				accountManager->talkState = 12;
				msg << "Would you like a recovery key?";
			}
			else
				msg << "Do you want to change your 'password', request a 'recovery key', add a 'character', or 'delete' a character?";
		}
		else if(accountManager->talkState == 2)
		{
			accountManager->accountManagerInput = text;
			trimString(accountManager->accountManagerInput);
			accountManager->talkState++;
			msg << "Do you really want to delete the character named " << accountManager->accountManagerInput << "?";
		}
		else if(accountManager->talkState == 3 && checkText(text, "yes"))
		{
			int32_t result = IOLoginData::getInstance()->deleteCharacter(accountManager->realAccount, accountManager->accountManagerInput);
			switch(result)
			{
				case 0:
					msg << "Either the character does not belong to you or it doesn't exist.";
					break;

				case 1:
					msg << "Your character has been deleted.";
					break;

				case 2:
					msg << "Your character is a houseowner, to make sure you really want to lose your house deleting your character you have to login and leave the house or pass it to someone else.";
					break;

				case 3:
					msg << "Your character is the leader of a guild, you need to disband or pass the leadership to delete your character.";
					break;

				case 4:
					msg << "A character with that name is currently online, to delete a character it has to be offline.";
					break;
			}
			accountManager->talkState = 1;
		}
		else if(accountManager->talkState == 3 && checkText(text, "no"))
		{
			accountManager->talkState--;
			msg << "Tell me what character you want to delete.";
		}
		else if(accountManager->talkState == 4)
		{
			std::string tmpStr = text;
			trimString(tmpStr);
			if(isValidPassword(tmpStr))
			{
				if(tmpStr.length() > 5)
				{
					accountManager->talkState++;
					msg << "Should '" << tmpStr << "' be your new password?";
					accountManager->accountManagerInput = tmpStr;
				}
				else
					msg << "That password is too short, please select a longer password.";
			}
			else
				msg << "That password contains invalid characters... tell me another one.";
		}
		else if(accountManager->talkState == 5)
		{
			if(checkText(text, "yes"))
			{
				accountManager->talkState = 1;
				IOLoginData::getInstance()->setNewPassword(accountManager->realAccount, accountManager->accountManagerInput);
				msg << "Your password has been changed.";
			}
			else if(checkText(text, "no"))
			{
				accountManager->talkState = 1;
				msg << "Then not.";
			}
			else
				msg << "I don't understand, should it be your new password, yes or no?";
		}
		else if(accountManager->talkState == 6)
		{
			accountManager->accountManagerInput = text;
			trimString(accountManager->accountManagerInput);
			if(accountManager->accountManagerInput.length() < 4)
				msg << "The name you want is too short, please select a longer name.";
			else if(accountManager->accountManagerInput.length() > 20)
				msg << "The name you want is too long, please select a shorter name.";
			else
			{
				std::string lowerCaseInput = asLowerCaseString(accountManager->accountManagerInput);
				if(lowerCaseInput.substr(0, 4) == "god " || lowerCaseInput.substr(0, 3) == "gm " || lowerCaseInput.substr(0, 3) == "cm ")
					msg << "You are not a gamemaster, please pick another name.";
				else if(!isValidName(accountManager->accountManagerInput))
					msg << "That name seems to contain invalid symbols tell me another name.";
				else if(IOLoginData::getInstance()->playerExists(accountManager->accountManagerInput))
					msg << "A player with this name currently exists, please choose another name.";
				else
				{
					accountManager->talkState++;
					msg << accountManager->accountManagerInput << ", are you sure?";
				}
			}
		}
		else if(accountManager->talkState == 7 && checkText(text, "no"))
		{
			accountManager->talkState--;
			msg << "What else would you like to name your character?";
		}
		else if(accountManager->talkState == 7 && checkText(text, "yes"))
		{
			accountManager->talkState++;
			msg << "Should your character be a 'male' or a 'female'?";
		}
		else if(accountManager->talkState == 8)
		{
			if(checkText(text, "female"))
			{
				msg << "A female, are you sure?";
				accountManager->talkState++;
				accountManager->newSex = PLAYERSEX_FEMALE;
			}
			else if(checkText(text, "male"))
			{
				msg << "A male, are you sure?";
				accountManager->talkState++;
				accountManager->newSex = PLAYERSEX_MALE;
			}
			else
				msg << "I don't understand, should your character be a 'male' or a 'female'?";
		}
		else if(accountManager->talkState == 9 && checkText(text, "no"))
		{
			accountManager->talkState--;
			msg << "Tell me.. would you like to be a 'male' or a 'female'?";
		}
		else if(accountManager->talkState == 9 && checkText(text, "yes"))
		{
			if(g_config.getBoolean(ConfigManager::START_CHOOSEVOC))
			{
				accountManager->talkState++;
				std::vector<std::string> vocations;
				for(VocationsMap::iterator it = g_vocations.getFirstVocation(), end = g_vocations.getLastVocation(); it != end; ++it)
				{
					if(it->first == VOCATION_NONE || it->first != it->second->getFromVocation())
						continue;

					vocations.push_back(it->second->getVocDescription());
				}

				if(!vocations.empty())
				{
					msg << "What do you want to be... " << vocations[0];
					std::vector<std::string>::size_type size = vocations.size() - 1;
					for (std::vector<std::string>::size_type i = 1; i < size; ++i) {
						msg << ", " << vocations[i];
					}
					msg << " or " << vocations[size] << "?";
				}
				else
					msg << "Something is wrong with vocations, please contact a gamemaster.";
			}
			else if(IOLoginData::getInstance()->createCharacter(accountManager->realAccount, accountManager->accountManagerInput, accountManager->newVocation, accountManager->newSex))
			{
				accountManager->talkState = 1;
				msg << "Your character has been created.";
			}
			else
			{
				accountManager->talkState = 6;
				msg << "A player with this name already exists, please choose another name.";
			}
		}
		else if(accountManager->talkState == 10)
		{
			for(VocationsMap::iterator it = g_vocations.getFirstVocation(), end = g_vocations.getLastVocation(); it != end; ++it)
			{
				Vocation* voc = it->second;
				std::string vocationName = voc->getVocName();
				std::transform(vocationName.begin(), vocationName.end(), vocationName.begin(), tolower);
				trimString(vocationName);
				if(it->first != VOCATION_NONE && it->first == voc->getFromVocation() && checkText(text, vocationName))
				{
					msg << "So you would like to be " << voc->getVocDescription() << "... are you sure?";
					accountManager->newVocation = it->first;
					accountManager->talkState++;
					break;
				}
			}

			if(msg.str().length() == 17)
				msg << "I don't understand what vocation you would like to be... could you please repeat it?";
		}
		else if(accountManager->talkState == 11 && checkText(text, "yes"))
		{
			if(IOLoginData::getInstance()->createCharacter(accountManager->realAccount, accountManager->accountManagerInput, accountManager->newVocation, accountManager->newSex))
			{
				accountManager->talkState = 1;
				msg << "Your character has been created.";
			}
			else
			{
				accountManager->talkState = 6;
				msg << "A player with this name already exists, please choose another name.";
			}
		}
		else if(accountManager->talkState == 11 && checkText(text, "no"))
		{
			accountManager->talkState = 10;
			msg << "No? Then what would you like to be?";
		}
		else if(accountManager->talkState == 12 && checkText(text, "yes"))
		{
			Account account = IOLoginData::getInstance()->loadAccount(accountManager->realAccount);
			if(account.recoveryKey != "0")
				msg << "Sorry, you already have a recovery key, for security reasons I may not give you a new one.";
			else
			{
				const std::string& recoveryKey = generateRecoveryKey(4, 4);
				IOLoginData::getInstance()->setRecoveryKey(accountManager->realAccount, recoveryKey);
				msg << "Your recovery key is: " << recoveryKey << ".";
			}
			accountManager->talkState = 1;
		}
		else if(accountManager->talkState == 12 && checkText(text, "no"))
		{
			msg << "Then not.";
			accountManager->talkState = 1;
		}
		else
			msg << "Please read the latest message that I have specified, I dont understand the current requested action.";
	}
	else
	{
		if(accountManager->talkState == 0 && checkText(text, "account"))
		{
			msg << "What would you like your password to be?";
			accountManager->talkState++;
		}
		else if(accountManager->talkState == 1)
		{
			std::string tmpStr = text;
			trimString(tmpStr);
			if(isValidPassword(tmpStr))
			{
				if(tmpStr.length() > 5)
				{
					msg << tmpStr << " is it? 'yes' or 'no'?";
					accountManager->accountManagerInput = tmpStr;
					accountManager->talkState++;
				}
				else
					msg << "That password is too short, please select a longer password.";
			}
			else
				msg << "That password contains invalid characters, try again.";
		}
		else if(accountManager->talkState == 2 && checkText(text, "yes"))
		{
			if(g_config.getBoolean(ConfigManager::GENERATE_ACCOUNT_NUMBER))
			{
				do
				{
					std::ostringstream ss;
					ss << random_range(2, 9) << random_range(2, 9) << random_range(2, 9) << random_range(2, 9) << random_range(2, 9) << random_range(2, 9) << random_range(2, 9);
					accountManager->newAccountName = ss.str();
				}
				while(IOLoginData::getInstance()->accountNameExists(accountManager->newAccountName));
				msg << "Your account has been created, you login with account name '" << accountManager->newAccountName << "' and password '" << accountManager->accountManagerInput << "', if the account name is hard to remember please write it down!";

				IOLoginData::getInstance()->createAccount(accountManager->newAccountName, accountManager->accountManagerInput);
				accountManager->talkState = -1;
			}
			else
			{
				msg << "What would you like your account name to be?";
				accountManager->talkState++;
			}
		}
		else if(accountManager->talkState == 2 && checkText(text, "no"))
		{
			accountManager->talkState--;
			msg << "What would you like your password to be then?";
		}
		else if(accountManager->talkState == 3)
		{
			std::string tmpStr = text;
			trimString(tmpStr);
			if(isValidAccountName(tmpStr))
			{
				if(tmpStr.length() >= 6)
				{
					if(tmpStr.length() <= 30)
					{
						msg << tmpStr << ", are you sure?";
						accountManager->newAccountName = tmpStr;
						accountManager->talkState++;
					}
					else
						msg << "That account name is too long.. an account name has to be atleast 6 characters but no more than 30, please pick another account name.";
				}
				else
					msg << "That account name is too short.. an account name has to be atleast 6 characters but no more than 30, please pick another account name.";
			}
			else
				msg << "Your account name may only contain characters from a to z and numbers, please pick another account name.";
		}
		else if(accountManager->talkState == 4 && checkText(text, "yes"))
		{
			if(!IOLoginData::getInstance()->accountNameExists(accountManager->newAccountName))
			{
				IOLoginData::getInstance()->createAccount(accountManager->newAccountName, accountManager->accountManagerInput);
				msg << "Your account has been created, you can login with account name: '" << accountManager->newAccountName << "' and password: '" << accountManager->accountManagerInput << "'.";
			}
			else
			{
				msg << "An account with that number combination already exists, please try another account name.";
				accountManager->talkState--;
			}
		}
		else if(accountManager->talkState == 4 && checkText(text, "no"))
		{
			accountManager->talkState--;
			msg << "What else would you like as your account name?";
		}
		else if(accountManager->talkState == 0 && checkText(text, "recover"))
		{
			accountManager->talkState = 5;
			msg << "What was your account name?";
		}
		else if(accountManager->talkState == 5)
		{
			accountManager->accountManagerInput = text;
			accountManager->talkState++;
			msg << "What was your recovery key?";
		}
		else if(accountManager->talkState == 6)
		{
			if(text != "0" && IOLoginData::getInstance()->validRecoveryKey(accountManager->accountManagerInput, text))
			{
				std::ostringstream ss;
				ss << g_config.getString(ConfigManager::SERVER_NAME) << random_range(100, 999);
				IOLoginData::getInstance()->setNewPassword(accountManager->accountManagerInput, ss.str());
				msg << "Correct! Your new password is: " << ss.str() << ".";
			}
			else
				msg << "Sorry, but that information you gave me did not match to any account :(.";

			accountManager->talkState = 0;
		}
		else
			msg << "Sorry, but I can't understand you, please try to repeat that.";
	}
	sendTextMessage(MSG_STATUS_CONSOLE_BLUE, msg.str());
	resetIdleTime();
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

bool Player::isInWar(const Player* player) const
{
	if(!player)
		return false;

	if(guildId != 0 && player->guildId != 0)
	{
		if(isInWarList(player->guildId) && player->isInWarList(guildId))
			return true;
	}
	return false;
}

bool Player::isInWarList(uint32_t guild_id) const
{
	for(GuildWarList::const_iterator it = guildWarList.begin(); it != guildWarList.end(); ++it)
	{
		if((*it) == guild_id)
			return true;
	}
	return false;
}

void Player::leaveGuild()
{
	sendClosePrivate(CHANNEL_GUILD);
	guildId = 0;
	guildName = "";
	guildRank = "";
	guildNick = "";
	guildLevel = 0;
	guildWarList.clear();
}

bool Player::isPremium() const
{
	if(g_config.getBoolean(ConfigManager::FREE_PREMIUM) || hasFlag(PlayerFlag_IsAlwaysPremium))
		return true;

	return premiumDays > 0;
}

void Player::setPremiumDays(int32_t v)
{
	premiumDays = v;
	sendBasicData();
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
		groupName = group->m_name;
		toLowerCaseString(groupName);
		setFlags(group->m_flags);
		accessLevel = (group->m_access >= 1);

		if(group->m_maxdepotitems > 0)
			maxDepotLimit = group->m_maxdepotitems;
		else if(isPremium())
			maxDepotLimit = 2000;

		if(group->m_maxviplist > 0)
			maxVipLimit = group->m_maxviplist;
		else if(isPremium())
			maxVipLimit = 100;
	}
}

PartyShields_t Player::getPartyShield(const Player* player) const
{
	if(!player)
		return SHIELD_NONE;

	Party* party = getParty();
	if(party)
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

bool Player::isGuildMate(const Player* player) const
{
	if(!player || guildId == 0 || player->getGuildId() == 0)
		return false;

	return guildId == player->getGuildId();
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

GuildEmblems_t Player::getGuildEmblem(const Player* player) const
{
	if(!player || player->getGuildWarList().empty())
		return EMBLEM_NONE;

	if(isInWar(player))
		return EMBLEM_RED;

	if(player->getGuildId() == guildId)
		return EMBLEM_GREEN;

	return EMBLEM_BLUE;
}

uint8_t Player::getCurrentMount() const
{
	int32_t value;
	if(getStorageValue(PSTRG_MOUNTS_CURRENTMOUNT, value))
		return value;

	return 0;
}

void Player::setCurrentMount(uint8_t mount)
{
	addStorageValue(PSTRG_MOUNTS_CURRENTMOUNT, mount);
}

bool Player::toggleMount(bool mount)
{
	if(mount)
	{
		if(isMounted())
			return false;

		if(_tile->hasFlag(TILESTATE_PROTECTIONZONE))
		{
			sendCancelMessage(RET_ACTIONNOTPERMITTEDINPROTECTIONZONE);
			return false;
		}

		uint8_t currentMountId = getCurrentMount();
		if(currentMountId == 0)
		{
			sendOutfitWindow();
			return false;
		}

		Mount* currentMount = Mounts::getInstance()->getMountByID(currentMountId);
		if(!currentMount)
			return false;

		if(!currentMount->isTamed(this))
		{
			setCurrentMount(0);
			sendOutfitWindow();
			return false;
		}

		if(currentMount->isPremium() && !isPremium())
		{
			sendCancelMessage(RET_YOUNEEDPREMIUMACCOUNT);
			return false;
		}

		defaultOutfit.lookMount = currentMount->getClientID();
		if(currentMount->getSpeed() != 0)
			g_game.changeSpeed(this, currentMount->getSpeed());

		g_game.internalCreatureChangeOutfit(this, defaultOutfit);
	}
	else
	{
		if(!isMounted())
			return false;

		dismount();
	}
	return true;
}

bool Player::tameMount(uint8_t mountId)
{
	if(!Mounts::getInstance()->getMountByID(mountId))
		return false;

	mountId--;
	int key = PSTRG_MOUNTS_RANGE_START + (mountId / 31);
	int32_t value = 0;
	if(getStorageValue(key, value))
		value |= (1 << (mountId % 31));
	else
		value = (1 << (mountId % 31));

	addStorageValue(key, value);
	return true;
}

bool Player::untameMount(uint8_t mountId)
{
	if(!Mounts::getInstance()->getMountByID(mountId))
		return false;

	mountId--;
	int key = PSTRG_MOUNTS_RANGE_START + (mountId / 31);
	int32_t value = 0;
	if(!getStorageValue(key, value))
		return true;

	value &= ~(1 << (mountId % 31));
	addStorageValue(key, value);

	if(getCurrentMount() == (mountId + 1))
	{
		if(isMounted())
			dismount();

		setCurrentMount(0);
	}
	return true;
}

void Player::dismount()
{
	Mount* mount = Mounts::getInstance()->getMountByID(getCurrentMount());
	if(mount && mount->getSpeed() > 0)
		g_game.changeSpeed(this, -mount->getSpeed());

	defaultOutfit.lookMount = 0;
	g_game.internalCreatureChangeOutfit(this, defaultOutfit);
}
