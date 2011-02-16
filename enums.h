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

#ifndef __ENUMS__
#define __ENUMS__

#include <string>
#include <list>

#ifdef _MSC_VER
	#pragma warning(disable:4244)
#endif

enum DatabaseEngine_t
{
	DATABASE_ENGINE_NONE = 0,
	DATABASE_ENGINE_MYSQL,
	DATABASE_ENGINE_SQLITE,
	DATABASE_ENGINE_POSTGRESQL
};

enum Encryption_t
{
	ENCRYPTION_PLAIN = 0,
	ENCRYPTION_MD5,
	ENCRYPTION_SHA1,
	ENCRYPTION_SHA256,
	ENCRYPTION_SHA512,
	ENCRYPTION_VAHASH
};

enum GuildLevel_t
{
	GUILDLEVEL_NONE = 0,
	GUILDLEVEL_MEMBER,
	GUILDLEVEL_VICE,
	GUILDLEVEL_LEADER
};

enum OperatingSystem_t
{
	CLIENTOS_LINUX = 0x01,
	CLIENTOS_WINDOWS = 0x02
};

enum Channels_t
{
	CHANNEL_GUILD = 0x00,
	CHANNEL_PARTY = 0x01,
	CHANNEL_HELP = 0x09,
	CHANNEL_DEFAULT = 0xFFFE, //internal usage only, there is no such channel
	CHANNEL_PRIVATE = 0xFFFF
};

enum ViolationAction_t
{
	ACTION_NOTATION = 0,
	ACTION_NAMEREPORT,
	ACTION_BANISHMENT,
	ACTION_BANREPORT,
	ACTION_BANFINAL,
	ACTION_BANREPORTFINAL,
	ACTION_STATEMENT,
	//internal use
	ACTION_DELETION,
	ACTION_NAMELOCK,
	ACTION_BANLOCK,
	ACTION_BANLOCKFINAL,
	ACTION_PLACEHOLDER
};

enum RaceType_t
{
	RACE_NONE = 0,
	RACE_VENOM,
	RACE_BLOOD,
	RACE_UNDEAD,
	RACE_FIRE,
	RACE_ENERGY
};

enum CombatType_t
{
	COMBAT_FIRST		= 0,
	COMBAT_NONE		= COMBAT_FIRST,
	COMBAT_PHYSICALDAMAGE	= 1 << 0,
	COMBAT_ENERGYDAMAGE	= 1 << 1,
	COMBAT_EARTHDAMAGE	= 1 << 2,
	COMBAT_FIREDAMAGE	= 1 << 3,
	COMBAT_UNDEFINEDDAMAGE	= 1 << 4,
	COMBAT_LIFEDRAIN	= 1 << 5,
	COMBAT_MANADRAIN	= 1 << 6,
	COMBAT_HEALING		= 1 << 7,
	COMBAT_DROWNDAMAGE	= 1 << 8,
	COMBAT_ICEDAMAGE	= 1 << 9,
	COMBAT_HOLYDAMAGE	= 1 << 10,
	COMBAT_DEATHDAMAGE	= 1 << 11,
	COMBAT_LAST		= COMBAT_DEATHDAMAGE
};

enum CombatParam_t
{
	COMBATPARAM_NONE = 0,
	COMBATPARAM_COMBATTYPE,
	COMBATPARAM_EFFECT,
	COMBATPARAM_DISTANCEEFFECT,
	COMBATPARAM_BLOCKEDBYSHIELD,
	COMBATPARAM_BLOCKEDBYARMOR,
	COMBATPARAM_TARGETCASTERORTOPMOST,
	COMBATPARAM_CREATEITEM,
	COMBATPARAM_AGGRESSIVE,
	COMBATPARAM_DISPEL,
	COMBATPARAM_USECHARGES,
	COMBATPARAM_TARGETPLAYERSORSUMMONS,
	COMBATPARAM_DIFFERENTAREADAMAGE,
	COMBATPARAM_HITEFFECT,
	COMBATPARAM_HITCOLOR
};

enum CallBackParam_t
{
	CALLBACKPARAM_NONE = 0,
	CALLBACKPARAM_LEVELMAGICVALUE,
	CALLBACKPARAM_SKILLVALUE,
	CALLBACKPARAM_TARGETTILECALLBACK,
	CALLBACKPARAM_TARGETCREATURECALLBACK
};

enum ConditionParam_t
{
	CONDITIONPARAM_OWNER = 1,
	CONDITIONPARAM_TICKS,
	CONDITIONPARAM_OUTFIT,
	CONDITIONPARAM_HEALTHGAIN,
	CONDITIONPARAM_HEALTHTICKS,
	CONDITIONPARAM_MANAGAIN,
	CONDITIONPARAM_MANATICKS,
	CONDITIONPARAM_DELAYED,
	CONDITIONPARAM_SPEED,
	CONDITIONPARAM_LIGHT_LEVEL,
	CONDITIONPARAM_LIGHT_COLOR,
	CONDITIONPARAM_SOULGAIN,
	CONDITIONPARAM_SOULTICKS,
	CONDITIONPARAM_MINVALUE,
	CONDITIONPARAM_MAXVALUE,
	CONDITIONPARAM_STARTVALUE,
	CONDITIONPARAM_TICKINTERVAL,
	CONDITIONPARAM_FORCEUPDATE,
	CONDITIONPARAM_SKILL_MELEE,
	CONDITIONPARAM_SKILL_FIST,
	CONDITIONPARAM_SKILL_CLUB,
	CONDITIONPARAM_SKILL_SWORD,
	CONDITIONPARAM_SKILL_AXE,
	CONDITIONPARAM_SKILL_DISTANCE,
	CONDITIONPARAM_SKILL_SHIELD,
	CONDITIONPARAM_SKILL_FISHING,
	CONDITIONPARAM_STAT_MAXHEALTH,
	CONDITIONPARAM_STAT_MAXMANA,
	CONDITIONPARAM_STAT_SOUL,
	CONDITIONPARAM_STAT_MAGICLEVEL,
	CONDITIONPARAM_STAT_MAXHEALTHPERCENT,
	CONDITIONPARAM_STAT_MAXMANAPERCENT,
	CONDITIONPARAM_STAT_SOULPERCENT,
	CONDITIONPARAM_STAT_MAGICLEVELPERCENT,
	CONDITIONPARAM_SKILL_MELEEPERCENT,
	CONDITIONPARAM_SKILL_FISTPERCENT,
	CONDITIONPARAM_SKILL_CLUBPERCENT,
	CONDITIONPARAM_SKILL_SWORDPERCENT,
	CONDITIONPARAM_SKILL_AXEPERCENT,
	CONDITIONPARAM_SKILL_DISTANCEPERCENT,
	CONDITIONPARAM_SKILL_SHIELDPERCENT,
	CONDITIONPARAM_SKILL_FISHINGPERCENT,
	CONDITIONPARAM_PERIODICDAMAGE,
	CONDITIONPARAM_BUFF,
	CONDITIONPARAM_SUBID,
	CONDITIONPARAM_FIELD
};

enum BlockType_t
{
	BLOCK_NONE = 0,
	BLOCK_DEFENSE,
	BLOCK_ARMOR,
	BLOCK_IMMUNITY
};

enum Reflect_t
{
	REFLECT_FIRST = 0,
	REFLECT_PERCENT = REFLECT_FIRST,
	REFLECT_CHANCE,
	REFLECT_LAST = REFLECT_CHANCE
};

enum Increment_t
{
	INCREMENT_FIRST = 0,
	HEALING_VALUE = INCREMENT_FIRST,
	HEALING_PERCENT,
	MAGIC_VALUE,
	MAGIC_PERCENT,
	INCREMENT_LAST = MAGIC_PERCENT
};

enum skills_t
{
	SKILL_FIRST = 0,
	SKILL_FIST = SKILL_FIRST,
	SKILL_CLUB,
	SKILL_SWORD,
	SKILL_AXE,
	SKILL_DIST,
	SKILL_SHIELD,
	SKILL_FISH,
	SKILL__MAGLEVEL,
	SKILL__LEVEL,
	SKILL_LAST = SKILL_FISH,
	SKILL__LAST = SKILL__LEVEL
};

enum stats_t
{
	STAT_FIRST = 0,
	STAT_MAXHEALTH = STAT_FIRST,
	STAT_MAXMANA,
	STAT_SOUL,
	STAT_LEVEL,
	STAT_MAGICLEVEL,
	STAT_LAST = STAT_MAGICLEVEL
};

enum lossTypes_t
{
	LOSS_FIRST = 0,
	LOSS_EXPERIENCE = LOSS_FIRST,
	LOSS_MANA,
	LOSS_SKILLS,
	LOSS_CONTAINERS,
	LOSS_ITEMS,
	LOSS_LAST = LOSS_ITEMS
};

enum formulaType_t
{
	FORMULA_UNDEFINED = 0,
	FORMULA_LEVELMAGIC,
	FORMULA_SKILL,
	FORMULA_VALUE
};

enum ConditionId_t
{
	CONDITIONID_DEFAULT = -1,
	CONDITIONID_COMBAT = 0,
	CONDITIONID_HEAD,
	CONDITIONID_NECKLACE,
	CONDITIONID_BACKPACK,
	CONDITIONID_ARMOR,
	CONDITIONID_RIGHT,
	CONDITIONID_LEFT,
	CONDITIONID_LEGS,
	CONDITIONID_FEET,
	CONDITIONID_RING,
	CONDITIONID_AMMO,
	CONDITIONID_OUTFIT
};

enum Spells_t {
	SPELL_NONE                    = 0x00,
	SPELL_LIGHT_HEALING           = 0x01,
	SPELL_INTENSE_HEALING         = 0x02,
	SPELL_ULTIMATE_HEALING        = 0x03,
	SPELL_INTENSE_HEALING_RUNE    = 0x04,
	SPELL_ULTIMATE_HEALING_RUNE   = 0x05,
	SPELL_HASTE                   = 0x06,
	SPELL_LIGHT_MAGIC_MISSILE     = 0x07,
	SPELL_HEAVY_MAGIC_MISSILE     = 0x08,
	SPELL_SUMMON_CREATURE         = 0x09,
	SPELL_LIGHT                   = 0x0A,
	SPELL_GREAT_LIGHT             = 0x0B,
	SPELL_CONVINCE_CREATURE       = 0x0C,
	SPELL_ENERGY_WAVE             = 0x0D,
	SPELL_CHAMELEON               = 0x0E,
	SPELL_FIREBALL                = 0x0F,
	SPELL_GREAT_FIREBALL          = 0x10,
	SPELL_FIREBOMB                = 0x11,
	SPELL_EXPLOSION               = 0x12,
	SPELL_FIRE_WAVE               = 0x13,
	SPELL_FIND_PERSON             = 0x14,
	SPELL_SUDDEN_DEATH            = 0x15,
	SPELL_ENERGY_BEAM             = 0x16,
	SPELL_GREAT_ENERGY_BEAM       = 0x17,
	SPELL_HELLS_CORE              = 0x18,
	SPELL_FIRE_FIELD              = 0x19,
	SPELL_POISON_FIELD            = 0x1A,
	SPELL_ENERGY_FIELD            = 0x1B,
	SPELL_FIRE_WALL               = 0x1C,
	SPELL_CURE_POISON             = 0x1D,
	SPELL_DESTROY_FIELD           = 0x1E,
	SPELL_ANTIDOTE_RUNE           = 0x1F,
	SPELL_POISON_WALL             = 0x20,
	SPELL_ENERGY_WALL             = 0x21,
	SPELL_UNKNOWN_1               = 0x22,
	SPELL_UNKNOWN_2               = 0x23,
	SPELL_SALVATION               = 0x24,
	SPELL_MOVE                    = 0x25,
	SPELL_CREATURE_ILLUSION       = 0x26,
	SPELL_STRONG_HASTE            = 0x27,
	SPELL_UNKNOWN_3               = 0x28,
	SPELL_UNKNOWN_4               = 0x29,
	SPELL_FOOD                    = 0x2A,
	SPELL_STRONG_ICE_WAVE         = 0x2B,
	SPELL_MAGIC_SHIELD            = 0x2C,
	SPELL_INVISIBLE               = 0x2D,
	SPELL_UNKNOWN_5               = 0x2E,
	SPELL_UNKNOWN_6               = 0x2F,
	SPELL_POISONED_ARROW          = 0x30,
	SPELL_EXPLOSIVE_ARROW         = 0x31,
	SPELL_SOULFIRE                = 0x32,
	SPELL_CONJURE_ARROW           = 0x33,
	SPELL_RETRIEVE_FRIEND         = 0x34,
	SPELL_UNKNOWN_7               = 0x35,
	SPELL_PARALYZE                = 0x36,
	SPELL_ENERGYBOMB              = 0x37,
	SPELL_WRATH_OF_NATURE         = 0x38,
	SPELL_STRONG_ETHEREAL_SPEAR   = 0x39,
	SPELL_UNKNOWN_8               = 0x3A,
	SPELL_FRONT_SWEEP             = 0x3B,
	SPELL_UNKNOWN_9               = 0x3C,
	SPELL_BRUTAL_STRIKE           = 0x3D,
	SPELL_ANNIHILATION            = 0x3E,
	SPELL_UNKNOWN_10              = 0x3F,
	SPELL_UNKNOWN_11              = 0x40,
	SPELL_UNKNOWN_12              = 0x41,
	SPELL_UNKNOWN_13              = 0x42,
	SPELL_UNKNOWN_14              = 0x43,
	SPELL_UNKNOWN_15              = 0x44,
	SPELL_UNKNOWN_16              = 0x45,
	SPELL_UNKNOWN_17              = 0x46,
	SPELL_INVITE_GUESTS           = 0x47,
	SPELL_INVITE_SUBOWNERS        = 0x48,
	SPELL_KICK_GUEST              = 0x49,
	SPELL_EDIT_DOOR               = 0x4A,
	SPELL_ULTIMATE_LIGHT          = 0x4B,
	SPELL_MAGIC_ROPE              = 0x4C,
	SPELL_STALAGMITE              = 0x4D,
	SPELL_DESINTEGRATE            = 0x4E,
	SPELL_CONJURE_BOLT            = 0x4F,
	SPELL_BERSERK                 = 0x50,
	SPELL_LEVITATE                = 0x51,
	SPELL_MASS_HEALING            = 0x52,
	SPELL_ANIMATE_DEAD            = 0x53,
	SPELL_HEAL_FRIEND             = 0x54,
	SPELL_UNDEAD_LEGION           = 0x55,
	SPELL_MAGIC_WALL              = 0x56,
	SPELL_DEATH_STRIKE            = 0x57,
	SPELL_ENERGY_STRIKE           = 0x58,
	SPELL_FLAME_STRIKE            = 0x59,
	SPELL_CANCEL_INVISIBILITY     = 0x5A,
	SPELL_POISONBOMB              = 0x5B,
	SPELL_ENCHANT_STAFF           = 0x5C,
	SPELL_CHALLENGE               = 0x5D,
	SPELL_WILD_GROWTH             = 0x5E,
	SPELL_POWER_BOLT              = 0x5F,
	SPELL_UNKNOWN_18              = 0x60,
	SPELL_UNKNOWN_19              = 0x61,
	SPELL_UNKNOWN_20              = 0x62,
	SPELL_UNKNOWN_21              = 0x63,
	SPELL_UNKNOWN_22              = 0x64,
	SPELL_UNKNOWN_23              = 0x65,
	SPELL_UNKNOWN_24              = 0x66,
	SPELL_UNKNOWN_25              = 0x67,
	SPELL_UNKNOWN_26              = 0x68,
	SPELL_FIERCE_BERSERK          = 0x69,
	SPELL_GROUNDSHAKER            = 0x6A,
	SPELL_WHIRLWIND_THROW         = 0x6B,
	SPELL_SNIPER_ARROW            = 0x6C,
	SPELL_PIERCING_BOLT           = 0x6D,
	SPELL_ENCHANT_SPEAR           = 0x6E,
	SPELL_ETHEREAL_SPEAR          = 0x6F,
	SPELL_ICE_STRIKE              = 0x70,
	SPELL_TERRA_STRIKE            = 0x71,
	SPELL_ICICLE                  = 0x72,
	SPELL_AVALANCHE               = 0x73,
	SPELL_STONE_SHOWER            = 0x74,
	SPELL_THUNDERSTORM            = 0x75,
	SPELL_ETERNAL_WINTER          = 0x76,
	SPELL_RAGE_OF_THE_SKIES       = 0x77,
	SPELL_TERRA_WAVE              = 0x78,
	SPELL_ICE_WAVE                = 0x79,
	SPELL_DIVINE_MISSILE          = 0x7A,
	SPELL_WOUND_CLEANSING         = 0x7B,
	SPELL_DIVINE_CALDERA          = 0x7C,
	SPELL_DIVINE_HEALING          = 0x7D,
	SPELL_TRAIN_PARTY             = 0x7E,
	SPELL_PROTECT_PARTY           = 0x7F,
	SPELL_HEAL_PARTY              = 0x80,
	SPELL_ENCHANT_PARTY           = 0x81,
	SPELL_HOLY_MISSILE            = 0x82,
	SPELL_CHARGE                  = 0x83,
	SPELL_PROTECTOR               = 0x84,
	SPELL_BLOOD_RAGE              = 0x85,
	SPELL_SWIFT_FOOT              = 0x86,
	SPELL_SHARPSHOOTER            = 0x87,
	SPELL_UNKNOWN_27              = 0x88,
	SPELL_UNKNOWN_28              = 0x89,
	SPELL_IGNITE                  = 0x8A,
	SPELL_CURSE                   = 0x8B,
	SPELL_ELECTRIFY               = 0x8C,
	SPELL_INFLICT_WOUND           = 0x8D,
	SPELL_ENVENOM                 = 0x8E,
	SPELL_HOLY_FLASH              = 0x8F,
	SPELL_CURE_BLEEDING           = 0x90,
	SPELL_CURE_BURNING            = 0x91,
	SPELL_CURE_ELECTRIFICATION    = 0x92,
	SPELL_CURE_CURSE              = 0x93,
	SPELL_PHYSICAL_STRIKE         = 0x94,
	SPELL_LIGHTNING               = 0x95,
	SPELL_STRONG_FLAME_STRIKE     = 0x96,
	SPELL_STRONG_ENERGY_STRIKE    = 0x97,
	SPELL_STRONG_ICE_STRIKE       = 0x98,
	SPELL_STRONG_TERRA_STRIKE     = 0x99,
	SPELL_ULTIMATE_FLAME_STRIKE   = 0x9A,
	SPELL_ULTIMATE_ENERGY_STRIKE  = 0x9B,
	SPELL_ULTIMATE_ICE_STRIKE     = 0x9C,
	SPELL_ULTIMATE_TERRA_STRIKE   = 0x9D,
	SPELL_INTENSE_WOUND_CLEANSING = 0x9E,
	SPELL_RECOVERY                = 0x9F,
	SPELL_INTENSE_RECOVERY        = 0xA0
};

enum SpellGroup_t
{
	SPELLGROUP_NONE = 0,
	SPELLGROUP_ATTACK = 1,
	SPELLGROUP_HEALING = 2,
	SPELLGROUP_SUPPORT = 3,
	SPELLGROUP_SPECIAL = 4
};

enum PlayerSex_t
{
	PLAYERSEX_FEMALE = 0,
	PLAYERSEX_MALE
	// DO NOT ADD HERE! Every higher sex is only for your
	// own use- each female should be even and male odd.
};
#ifdef __WAR_SYSTEM__

enum WarType_t
{
	WAR_FIRST = 0,
	WAR_GUILD = WAR_FIRST,
	WAR_ENEMY,
	WAR_LAST = WAR_ENEMY
};

struct War_t
{
	War_t()
	{
		war = 0;
		type = WAR_FIRST;

		memset(ids, 0, sizeof(ids));
		memset(frags, 0, sizeof(frags));

		limit = payment = 0;
	}

	uint32_t war;
	WarType_t type;

	uint32_t ids[WAR_LAST + 1];
	std::string names[WAR_LAST + 1];
	uint16_t frags[WAR_LAST + 1];

	uint16_t limit;
	uint64_t payment;
};
#endif

struct Outfit_t
{
	Outfit_t() {lookHead = lookBody = lookLegs = lookFeet = lookType = lookTypeEx = lookAddons = lookMount = 0;}
	uint16_t lookType, lookMount, lookTypeEx;
	uint8_t lookHead, lookBody, lookLegs, lookFeet, lookAddons;

	bool operator==(const Outfit_t& o) const
	{
		return (o.lookAddons == lookAddons && o.lookMount == lookMount
			&& o.lookType == lookType && o.lookTypeEx == lookTypeEx
			&& o.lookHead == lookHead && o.lookBody == lookBody
			&& o.lookLegs == lookLegs && o.lookFeet == lookFeet);
	}

	bool operator!=(const Outfit_t& o) const
	{
		return !(*this == o);
	}
};

struct LightInfo
{
	uint32_t level, color;

	LightInfo() {level = color = 0;}
	LightInfo(uint32_t _level, uint32_t _color):
		level(_level), color(_color) {}
};

struct ShopInfo
{
	uint32_t itemId;
	int32_t subType, buyPrice, sellPrice;
	std::string itemName;

	ShopInfo()
	{
		itemId = 0;
		subType = 1;
		buyPrice = sellPrice = -1;
		itemName = "";
	}
	ShopInfo(uint32_t _itemId, int32_t _subType = 1, int32_t _buyPrice = -1, int32_t _sellPrice = -1,
		const std::string& _itemName = ""): itemId(_itemId), subType(_subType), buyPrice(_buyPrice),
		sellPrice(_sellPrice), itemName(_itemName) {}
};

typedef std::list<ShopInfo> ShopInfoList;
#endif
