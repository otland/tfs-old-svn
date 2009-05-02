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

#ifndef __OTSERV_MONSTERS_H__
#define __OTSERV_MONSTERS_H__
#include "creature.h"

#define MAX_LOOTCHANCE 100000
#define MAX_STATICWALK 100

struct LootBlock;
typedef std::list<LootBlock> LootItems;

struct LootBlock
{
	uint16_t id, countmax;
	int32_t subType, actionId, uniqueId;
	uint32_t chance;
	std::string text;
	LootItems childLoot;

	LootBlock()
	{
		id = countmax = chance = 0;
		subType = actionId = uniqueId = -1;
		text = "";
	}
};

struct summonBlock_t
{
	std::string name;
	uint32_t chance, interval, amount;
};

class BaseSpell;

struct spellBlock_t
{
	bool combatSpell, isMelee;
	int32_t minCombatValue, maxCombatValue;
	uint32_t chance, speed, range;
	BaseSpell* spell;
};

struct voiceBlock_t
{
	bool yellText;
	std::string text;
};

typedef std::list<summonBlock_t> SummonList;
typedef std::list<spellBlock_t> SpellList;
typedef std::vector<voiceBlock_t> VoiceVector;
typedef std::list<std::string> MonsterScriptList;
typedef std::map<CombatType_t, int32_t> ElementMap;
typedef std::vector<Item*> ItemVector;

class MonsterType
{
	public:
		MonsterType() {reset();}
		virtual ~MonsterType() {reset();}

		void reset();

		void createLoot(Container* corpse);
		bool createLootContainer(Container* parent, const LootBlock& lootblock, ItemVector& itemVector);
		Item* createLootItem(const LootBlock& lootblock);

		bool isSummonable, isIllusionable, isConvinceable, isAttackable, isHostile, isLureable,
			canPushItems, canPushCreatures, pushable, hideName, hideHealth;

		Outfit_t outfit;
		RaceType_t race;
		Skulls_t skull;
		PartyShields_t partyShield;

		int32_t defense, armor, health, healthMax, baseSpeed, lookCorpse, maxSummons, targetDistance, runAwayHealth,
			conditionImmunities, damageImmunities, lightLevel, lightColor, changeTargetSpeed, changeTargetChance;
		uint32_t yellChance, yellSpeedTicks, staticAttackChance, manaCost;
		uint64_t experience;

		std::string name;
		std::string nameDescription;

		SummonList summonList;
		LootItems lootItems;
		ElementMap elementMap;
		SpellList spellAttackList;
		SpellList spellDefenseList;
		VoiceVector voiceVector;
		MonsterScriptList scriptList;
};

class Monsters
{
	public:
		Monsters(): loaded(false) {}
		virtual ~Monsters();

		bool reload() {return loadFromXml(true);}
		bool loadFromXml(bool reloading = false);

		MonsterType* getMonsterType(const std::string& name);
		MonsterType* getMonsterType(uint32_t mid);

		uint32_t getIdByName(const std::string& name);
		bool isLoaded(){return loaded;}
		static uint32_t getLootRandom();

	private:
		bool loaded;

		bool loadMonster(const std::string& file, const std::string& monster_name, bool reloading = false);
		bool loadLootContainer(xmlNodePtr, LootBlock&);
		bool loadLootItem(xmlNodePtr, LootBlock&);

		ConditionDamage* getDamageCondition(ConditionType_t conditionType,
			int32_t maxDamage, int32_t minDamage, int32_t startDamage, uint32_t tickInterval);
		bool deserializeSpell(xmlNodePtr node, spellBlock_t& sb, const std::string& description = "");

		typedef std::map<std::string, uint32_t> MonsterNameMap;
		MonsterNameMap monsterNames;

		typedef std::map<uint32_t, MonsterType*> MonsterMap;
		MonsterMap monsters;
};

#endif
