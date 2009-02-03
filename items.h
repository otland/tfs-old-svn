//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The database of items.
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

#ifndef __OTSERV_ITEMS_H__
#define __OTSERV_ITEMS_H__

#include "definitions.h"
#include "const.h"
#include "enums.h"
#include "itemloader.h"
#include "position.h"
#include <map>

#define SLOTP_WHEREEVER 0xFFFFFFFF
#define SLOTP_HEAD 1 << 0
#define	SLOTP_NECKLACE 1 << 1
#define	SLOTP_BACKPACK 1 << 2
#define	SLOTP_ARMOR 1 << 3
#define	SLOTP_RIGHT 1 << 4
#define	SLOTP_LEFT 1 << 5
#define	SLOTP_LEGS 1 << 6
#define	SLOTP_FEET 1 << 7
#define	SLOTP_RING 1 << 8
#define	SLOTP_AMMO 1 << 9
#define	SLOTP_DEPOT 1 << 10
#define	SLOTP_TWO_HAND 1 << 11

enum ItemTypes_t
{
	ITEM_TYPE_NONE = 0,
	ITEM_TYPE_DEPOT,
	ITEM_TYPE_MAILBOX,
	ITEM_TYPE_TRASHHOLDER,
	ITEM_TYPE_CONTAINER,
	ITEM_TYPE_DOOR,
	ITEM_TYPE_MAGICFIELD,
	ITEM_TYPE_TELEPORT,
	ITEM_TYPE_BED,
	ITEM_TYPE_KEY,
	ITEM_TYPE_LAST
};

struct Abilities
{
	Abilities()
	{
		memset(skills, 0, sizeof(skills));
		memset(stats, 0 , sizeof(stats));
		memset(statsPercent, 0, sizeof(statsPercent));
		memset(absorbPercent, 0, sizeof(absorbPercent));

		healthGain = healthTicks = manaGain = manaTicks = 0;

		elementType = COMBAT_NONE;
		elementDamage = 0;

		conditionImmunities = conditionSuppressions = 0;

		speed = 0;
		manaShield = invisible = regeneration = preventLoss = false;
	};

	//extra skill modifiers
	int32_t skills[SKILL_LAST + 1];

	//stats modifiers
	int32_t stats[STAT_LAST + 1], statsPercent[STAT_LAST + 1];

	//damage abilities modifiers
	int16_t absorbPercent[COMBAT_LAST + 1];

	//regeneration
	uint32_t healthGain, healthTicks, manaGain, manaTicks;

	//elements
	CombatType_t elementType;
	int16_t elementDamage;

	//conditions
	uint32_t conditionImmunities, conditionSuppressions;

	//other
	int32_t speed;
	bool manaShield, invisible, regeneration, preventLoss;
};

class Condition;

class ItemType
{
	private:
		ItemType(const ItemType& it){}

	public:
		ItemType();
		virtual ~ItemType();

		itemgroup_t group;
		ItemTypes_t type;

		bool isGroundTile() const {return (group == ITEM_GROUP_GROUND);}
		bool isContainer() const {return (group == ITEM_GROUP_CONTAINER);}
		bool isSplash() const {return (group == ITEM_GROUP_SPLASH);}
		bool isFluidContainer() const {return (group == ITEM_GROUP_FLUID);}

		bool isDoor() const {return (type == ITEM_TYPE_DOOR);}
		bool isMagicField() const {return (type == ITEM_TYPE_MAGICFIELD);}
		bool isTeleport() const {return (type == ITEM_TYPE_TELEPORT);}
		bool isKey() const {return (type == ITEM_TYPE_KEY);}
		bool isDepot() const {return (type == ITEM_TYPE_DEPOT);}
		bool isMailbox() const {return (type == ITEM_TYPE_MAILBOX);}
		bool isTrashHolder() const {return (type == ITEM_TYPE_TRASHHOLDER);}
		bool isBed() const {return (type == ITEM_TYPE_BED);}

		bool isRune() const {return clientCharges;}
		bool hasSubType() const {return (isFluidContainer() || isSplash() || stackable || charges != 0);}

		Direction bedPartnerDir;
		uint16_t transformToOnUse[2];
		uint16_t transformToFree;
		uint32_t levelDoor;

		uint16_t id;
		uint16_t clientId;

		std::string name;
		std::string article;
		std::string pluralName;
		std::string description;
		uint16_t maxItems;
		float weight;
		bool showCount;
		WeaponType_t weaponType;
		Ammo_t ammoType;
		ShootType_t shootType;
		MagicEffectClasses magicEffect;
		int32_t attack;
		int32_t extraAttack;
		int32_t defense;
		int32_t extraDefense;
		int32_t armor;
		uint32_t attackSpeed;
		uint16_t slotPosition;
		bool isVertical;
		bool isHorizontal;
		bool isHangable;
		bool allowDistRead;
		bool clientCharges;
		uint16_t speed;
		int32_t decayTo;
		uint32_t decayTime;
		bool stopTime;
		RaceType_t corpseType;

		bool canReadText;
		bool canWriteText;
		uint16_t maxTextLen;
		uint16_t writeOnceItemId;

		bool stackable;
		bool useable;
		bool moveable;
		bool alwaysOnTop;
		int32_t alwaysOnTopOrder;
		bool pickupable;
		bool rotable;
		int32_t rotateTo;
		bool forceSerialize;

		std::string runeSpellName;
		int32_t runeLevel;
		int32_t runeMagLevel;
		std::string vocationString;
		uint32_t wieldInfo;
		uint32_t minReqLevel;
		uint32_t minReqMagicLevel;

		int32_t lightLevel;
		int32_t lightColor;

		bool floorChangeDown;
		bool floorChangeNorth;
		bool floorChangeSouth;
		bool floorChangeEast;
		bool floorChangeWest;
		bool hasHeight;

		bool blockSolid;
		bool blockPickupable;
		bool blockProjectile;
		bool blockPathFind;
		bool allowPickupable;

		uint16_t transformEquipTo;
		uint16_t transformDeEquipTo;
		bool showDuration;
		bool showCharges;
		uint32_t charges;
		int32_t breakChance;
		int32_t hitChance;
		int32_t maxHitChance;
		uint32_t shootRange;
		AmmoAction_t ammoAction;
		FluidTypes_t fluidSource;

		uint32_t worth;
		Abilities abilities;

		Condition* condition;
		CombatType_t combatType;
		bool replaceable;
};

template<typename A>
class Array
{
	public:
		Array(uint32_t n);
		virtual ~Array();

		A getElement(uint32_t id);
		const A getElement(uint32_t id) const;
		void addElement(A a, uint32_t pos);

		uint32_t size() {return m_size;}

	private:
		A* m_data;
		uint32_t m_size;
};

template<typename A>
Array<A>::Array(uint32_t n)
{
	m_data = (A*)malloc(sizeof(A) * n);
	memset(m_data, 0, sizeof(A) * n);
	m_size = n;
}

template<typename A>
Array<A>::~Array()
{
	free(m_data);
}

template<typename A>
A Array<A>::getElement(uint32_t id)
{
	if(id < m_size)
		return m_data[id];

	return 0;
}

template<typename A>
const A Array<A>::getElement(uint32_t id) const
{
	if(id < m_size)
		return m_data[id];

	return 0;
}

template<typename A>
void Array<A>::addElement(A a, uint32_t pos)
{
	#define INCREMENT 5000
	if(pos >= m_size)
	{
		m_data = (A*)realloc(m_data, sizeof(A) * (pos + INCREMENT));
		memset(m_data + m_size, 0, sizeof(A) * (pos + INCREMENT - m_size));
		m_size = pos + INCREMENT;
	}

	m_data[pos] = a;
}

class Items
{
	public:
		Items();
		virtual ~Items();

		bool reload();
		void clear();

		int32_t loadFromOtb(std::string);

		const ItemType& operator[](int32_t id) const {return getItemType(id);}
		const ItemType& getItemType(int32_t id) const;
		ItemType& getItemType(int32_t id);
		const ItemType& getItemIdByClientId(int32_t spriteId) const;

		int32_t getItemIdByName(const std::string& name);

		static uint32_t dwMajorVersion;
		static uint32_t dwMinorVersion;
		static uint32_t dwBuildNumber;

		bool loadFromXml();

		void addItemType(ItemType* iType);

		const ItemType* getElement(uint32_t id) const {return items.getElement(id);}
		uint32_t size() {return items.size();}

	protected:
		typedef std::map<int32_t, int32_t> ReverseItemMap;
		ReverseItemMap reverseItemMap;

		Array<ItemType*> items;
};

#endif
