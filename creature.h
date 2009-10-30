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

#ifndef __CREATURE__
#define __CREATURE__
#include "otsystem.h"

#include "templates.h"
#include <boost/any.hpp>

#include "const.h"
#include "enums.h"

#include "map.h"
#include "condition.h"
#include "creatureevent.h"

enum slots_t
{
	SLOT_PRE_FIRST = 0,
	SLOT_WHEREEVER = SLOT_PRE_FIRST,
	SLOT_FIRST = 1,
	SLOT_HEAD = SLOT_FIRST,
	SLOT_NECKLACE = 2,
	SLOT_BACKPACK = 3,
	SLOT_ARMOR = 4,
	SLOT_RIGHT = 5,
	SLOT_LEFT = 6,
	SLOT_LEGS = 7,
	SLOT_FEET = 8,
	SLOT_RING = 9,
	SLOT_AMMO = 10,
	SLOT_DEPOT = 11,
	SLOT_LAST = SLOT_DEPOT,
	SLOT_HAND = 12,
	SLOT_TWO_HAND = SLOT_HAND
};

enum lootDrop_t
{
	LOOT_DROP_FULL = 0,
	LOOT_DROP_PREVENT,
	LOOT_DROP_NONE
};

enum killflags_t
{
	KILLFLAG_NONE = 0,
	KILLFLAG_LASTHIT = 1 << 0,
	KILLFLAG_JUSTIFY = 1 << 1,
	KILLFLAG_UNJUSTIFIED = 1 << 2
};

enum Visible_t
{
	VISIBLE_NONE = 0,
	VISIBLE_APPEAR = 1,
	VISIBLE_DISAPPEAR = 2,
	VISIBLE_GHOST_APPEAR = 3,
	VISIBLE_GHOST_DISAPPEAR = 4
};

struct FindPathParams
{
	bool fullPathSearch, clearSight, allowDiagonal, keepDistance;
	int32_t maxSearchDist, minTargetDist, maxTargetDist;
	FindPathParams()
	{
		fullPathSearch = clearSight = allowDiagonal = true;
		maxSearchDist = minTargetDist = maxTargetDist = -1;
		keepDistance = false;
	}
};

struct DeathLessThan;
struct DeathEntry
{
		DeathEntry(std::string name, int32_t dmg):
			data(name), damage(dmg), unjustified(false) {}
		DeathEntry(Creature* killer, int32_t dmg):
			data(killer), damage(dmg), unjustified(false) {}
		void setUnjustified(bool v) {unjustified = v;}

		bool isCreatureKill() const {return data.type() == typeid(Creature*);}
		bool isNameKill() const {return !isCreatureKill();}
		bool isUnjustified() const {return unjustified;}

		const std::type_info& getKillerType() const {return data.type();}
		Creature* getKillerCreature() const {return boost::any_cast<Creature*>(data);}
		std::string getKillerName() const {return boost::any_cast<std::string>(data);}

	protected:
		boost::any data;
		int32_t damage;
		bool unjustified;

		friend struct DeathLessThan;
};

struct DeathLessThan
{
	bool operator()(const DeathEntry& d1, const DeathEntry& d2) {return d1.damage > d2.damage;}
};

typedef std::vector<DeathEntry> DeathList;
typedef std::list<CreatureEvent*> CreatureEventList;
typedef std::list<Condition*> ConditionList;
typedef std::map<uint32_t, std::string> StorageMap;

class Map;
class Tile;
class Thing;

class Player;
class Monster;
class Npc;

class Item;
class Container;

#define EVENT_CREATURECOUNT 10
#define EVENT_CREATURE_THINK_INTERVAL 500
#define EVENT_CHECK_CREATURE_INTERVAL (EVENT_CREATURE_THINK_INTERVAL / EVENT_CREATURECOUNT)

class FrozenPathingConditionCall
{
	public:
		FrozenPathingConditionCall(const Position& _targetPos);
		virtual ~FrozenPathingConditionCall() {}

		virtual bool operator()(const Position& startPos, const Position& testPos,
			const FindPathParams& fpp, int32_t& bestMatchDist) const;

		bool isInRange(const Position& startPos, const Position& testPos,
			const FindPathParams& fpp) const;

	protected:
		Position targetPos;
};

class Creature : public AutoId, virtual public Thing
{
	protected:
		Creature();

	public:
		virtual ~Creature();

		virtual Creature* getCreature() {return this;}
		virtual const Creature* getCreature()const {return this;}
		virtual Player* getPlayer() {return NULL;}
		virtual const Player* getPlayer() const {return NULL;}
		virtual Npc* getNpc() {return NULL;}
		virtual const Npc* getNpc() const {return NULL;}
		virtual Monster* getMonster() {return NULL;}
		virtual const Monster* getMonster() const {return NULL;}

		virtual const std::string& getName() const = 0;
		virtual const std::string& getNameDescription() const = 0;
		virtual std::string getDescription(int32_t lookDistance) const;

		uint32_t getID() const {return id;}
		void setID()
		{
			/*
			 * 0x10000000 - Player
			 * 0x40000000 - Monster
			 * 0x80000000 - NPC
			 */
			if(!id)
				id = autoId | rangeId();
		}

		void setRemoved() {removed = true;}
		virtual bool isRemoved() const {return removed;}

		virtual uint32_t rangeId() = 0;
		virtual void removeList() = 0;
		virtual void addList() = 0;

		virtual bool canSee(const Position& pos) const;
		virtual bool canSeeCreature(const Creature* creature) const;
		virtual bool canWalkthrough(const Creature* creature) const {return creature->isWalkable() || creature->isGhost();}

		Direction getDirection() const {return direction;}
		void setDirection(Direction dir) {direction = dir;}

		bool getHideName() const {return hideName;}
		void setHideName(bool v) {hideName = v;}
		bool getHideHealth() const {return hideHealth;}
		void setHideHealth(bool v) {hideHealth = v;}

		SpeakClasses getSpeakType() const {return speakType;}
		void setSpeakType(SpeakClasses type) {speakType = type;}

		Position getMasterPosition() const {return masterPosition;}
		void setMasterPosition(const Position& pos, uint32_t radius = 1) {masterPosition = pos; masterRadius = radius;}

		virtual int32_t getThrowRange() const {return 1;}
		virtual RaceType_t getRace() const {return RACE_NONE;}

		virtual bool isPushable() const {return getWalkDelay() <= 0;}
		virtual bool canSeeInvisibility() const {return false;}

		int32_t getWalkDelay(Direction dir) const;
		int32_t getWalkDelay() const;
		int32_t getStepDuration(Direction dir) const;
		int32_t getStepDuration() const;

		void getPathToFollowCreature();
		int64_t getEventStepTicks() const;
		int64_t getTimeSinceLastMove() const;
		virtual int32_t getStepSpeed() const {return getSpeed();}

		int32_t getSpeed() const {return baseSpeed + varSpeed;}
		void setSpeed(int32_t varSpeedDelta)
		{
			int32_t oldSpeed = getSpeed();
			varSpeed = varSpeedDelta;
			if(getSpeed() <= 0)
				stopEventWalk();
			else if(oldSpeed <= 0 && !listWalkDir.empty())
				addEventWalk();
		}

		void setBaseSpeed(uint32_t newBaseSpeed) {baseSpeed = newBaseSpeed;}
		int32_t getBaseSpeed() {return baseSpeed;}

		virtual int32_t getHealth() const {return health;}
		virtual int32_t getMaxHealth() const {return healthMax;}
		virtual int32_t getMana() const {return mana;}
		virtual int32_t getMaxMana() const {return manaMax;}

		const Outfit_t getCurrentOutfit() const {return currentOutfit;}
		const void setCurrentOutfit(Outfit_t outfit) {currentOutfit = outfit;}
		const Outfit_t getDefaultOutfit() const {return defaultOutfit;}

		bool isInvisible() const {return hasCondition(CONDITION_INVISIBLE, -1, false);}
		virtual bool isGhost() const {return false;}
		virtual bool isWalkable() const {return false;}

		ZoneType_t getZone() const {return getTile()->getZone();}

		//walk functions
		bool startAutoWalk(std::list<Direction>& listDir);
		void addEventWalk();
		void stopEventWalk();

		//walk events
		virtual void onWalk(Direction& dir);
		virtual void onWalkAborted() {}
		virtual void onWalkComplete() {}

		//follow functions
		virtual Creature* getFollowCreature() const {return followCreature;}
		virtual bool setFollowCreature(Creature* creature, bool fullPathSearch = false);

		//follow events
		virtual void onFollowCreature(const Creature* creature) {}
		virtual void onFollowCreatureComplete(const Creature* creature) {}

		//combat functions
		Creature* getAttackedCreature() {return attackedCreature;}
		virtual bool setAttackedCreature(Creature* creature);
		virtual BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
			bool checkDefense = false, bool checkArmor = false);

		void setMaster(Creature* creature) {master = creature;}
		Creature* getMaster() {return master;}
		const Creature* getMaster() const {return master;}
		Player* getPlayerMaster() const {return isPlayerSummon() ? master->getPlayer() : NULL;}
		bool isSummon() const {return master != NULL;}
		bool isPlayerSummon() const {return master && master->getPlayer();}

		virtual void addSummon(Creature* creature);
		virtual void removeSummon(const Creature* creature);
		const std::list<Creature*>& getSummons() {return summons;}
		void destroySummons();
		uint32_t getSummonCount() const {return summons.size();}

		virtual int32_t getArmor() const {return 0;}
		virtual int32_t getDefense() const {return 0;}
		virtual float getAttackFactor() const {return 1.0f;}
		virtual float getDefenseFactor() const {return 1.0f;}

		bool addCondition(Condition* condition);
		bool addCombatCondition(Condition* condition);
		void removeCondition(ConditionType_t type);
		void removeCondition(ConditionType_t type, ConditionId_t id);
		void removeCondition(Condition* condition);
		void removeCondition(const Creature* attacker, ConditionType_t type);
		void removeConditions(ConditionEnd_t reason, bool onlyPersistent = true);
		Condition* getCondition(ConditionType_t type, ConditionId_t id, uint32_t subId = 0) const;
		void executeConditions(uint32_t interval);
		bool hasCondition(ConditionType_t type, int32_t subId = 0, bool checkTime = true) const;
		virtual bool isImmune(ConditionType_t type) const;
		virtual bool isImmune(CombatType_t type) const;
		virtual bool isSuppress(ConditionType_t type) const;
		virtual uint32_t getDamageImmunities() const {return 0;}
		virtual uint32_t getConditionImmunities() const {return 0;}
		virtual uint32_t getConditionSuppressions() const {return 0;}
		virtual bool isAttackable() const {return true;}
		virtual bool isAccountManager() const {return false;}

		virtual void changeHealth(int32_t healthChange);
		void changeMaxHealth(uint32_t healthChange) {healthMax = healthChange;}
		virtual void changeMana(int32_t manaChange);
		void changeMaxMana(uint32_t manaChange) {manaMax = manaChange;}

		virtual bool getStorage(const uint32_t key, std::string& value) const;
		virtual bool setStorage(const uint32_t key, const std::string& value);
		virtual void eraseStorage(const uint32_t key) {storageMap.erase(key);}

		inline StorageMap::const_iterator getStorageBegin() const {return storageMap.begin();}
		inline StorageMap::const_iterator getStorageEnd() const {return storageMap.end();}

		virtual void gainHealth(Creature* caster, int32_t amount);
		virtual void drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage);
		virtual void drainMana(Creature* attacker, CombatType_t combatType, int32_t damage);

		virtual bool challengeCreature(Creature* creature) {return false;}
		virtual bool convinceCreature(Creature* creature) {return false;}

		virtual bool onDeath();
		virtual double getGainedExperience(Creature* attacker) const {return getDamageRatio(attacker) * (double)getLostExperience();}
		void addDamagePoints(Creature* attacker, int32_t damagePoints);
		void addHealPoints(Creature* caster, int32_t healthPoints);
		bool hasBeenAttacked(uint32_t attackerId) const;

		//combat event functions
		virtual void onAddCondition(ConditionType_t type, bool hadCondition);
		virtual void onAddCombatCondition(ConditionType_t type, bool hadCondition) {}
		virtual void onEndCondition(ConditionType_t type);
		virtual void onTickCondition(ConditionType_t type, int32_t interval, bool& _remove);
		virtual void onCombatRemoveCondition(const Creature* attacker, Condition* condition);
		virtual void onAttackedCreature(Creature* target) {}
		virtual void onSummonAttackedCreature(Creature* summon, Creature* target) {}
		virtual void onAttacked() {}
		virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
		virtual void onSummonAttackedCreatureDrainHealth(Creature* summon, Creature* target, int32_t points) {}
		virtual void onAttackedCreatureDrainMana(Creature* target, int32_t points);
		virtual void onSummonAttackedCreatureDrainMana(Creature* summon, Creature* target, int32_t points) {}
		virtual void onAttackedCreatureDrain(Creature* target, int32_t points);
		virtual void onSummonAttackedCreatureDrain(Creature* summon, Creature* target, int32_t points) {}
		virtual void onTargetCreatureGainHealth(Creature* target, int32_t points);
		virtual void onAttackedCreatureKilled(Creature* target);
		virtual bool onKilledCreature(Creature* target, uint32_t& flags);
		virtual void onGainExperience(double& gainExp, bool fromMonster, bool multiplied);
		virtual void onGainSharedExperience(double& gainExp, bool fromMonster, bool multiplied);
		virtual void onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType) {}
		virtual void onBlockHit(BlockType_t blockType) {}
		virtual void onChangeZone(ZoneType_t zone);
		virtual void onAttackedCreatureChangeZone(ZoneType_t zone);
		virtual void onIdleStatus();

		virtual void getCreatureLight(LightInfo& light) const;
		virtual void setNormalCreatureLight();
		void setCreatureLight(LightInfo& light) {internalLight = light;}

		virtual void onThink(uint32_t interval);
		virtual void onAttacking(uint32_t interval);
		virtual void onWalk();
		virtual bool getNextStep(Direction& dir, uint32_t& flags);

		virtual void onAddTileItem(const Tile* tile, const Position& pos, const Item* item);
		virtual void onUpdateTileItem(const Tile* tile, const Position& pos, const Item* oldItem,
			const ItemType& oldType, const Item* newItem, const ItemType& newType);
		virtual void onRemoveTileItem(const Tile* tile, const Position& pos, const ItemType& iType, const Item* item);
		virtual void onUpdateTile(const Tile* tile, const Position& pos) {}

		virtual void onCreatureAppear(const Creature* creature);
		virtual void onCreatureDisappear(const Creature* creature, bool isLogout);
		virtual void onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
			const Tile* oldTile, const Position& oldPos, bool teleport);

		virtual void onAttackedCreatureDisappear(bool isLogout) {}
		virtual void onFollowCreatureDisappear(bool isLogout) {}

		virtual void onCreatureTurn(const Creature* creature) {}
		virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text,
			Position* pos = NULL) {}

		virtual void onCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit) {}
		virtual void onCreatureConvinced(const Creature* convincer, const Creature* creature) {}
		virtual void onCreatureChangeVisible(const Creature* creature, Visible_t visible) {}
		virtual void onPlacedCreature() {}
		virtual void onRemovedCreature();

		virtual WeaponType_t getWeaponType() {return WEAPON_NONE;}
		virtual bool getCombatValues(int32_t& min, int32_t& max) {return false;}

		virtual void setSkull(Skulls_t newSkull) {skull = newSkull;}
		virtual Skulls_t getSkull() const {return skull;}
		virtual Skulls_t getSkullClient(const Creature* creature) const {return creature->getSkull();}

		virtual void setShield(PartyShields_t newPartyShield) {partyShield = newPartyShield;}
		virtual PartyShields_t getShield() const {return partyShield;}
		virtual PartyShields_t getPartyShield(const Creature* creature) const {return creature->getShield();}

		void setDropLoot(lootDrop_t _lootDrop) {lootDrop = _lootDrop;}
		void setLossSkill(bool _skillLoss) {skillLoss = _skillLoss;}
		bool getLossSkill() const {return skillLoss;}
		void setNoMove(bool _cannotMove) {cannotMove = _cannotMove;}
		bool getNoMove() const {return cannotMove;}

		//creature script events
		bool registerCreatureEvent(const std::string& name);
		CreatureEventList getCreatureEvents(CreatureEventType_t type);

		virtual void setParent(Cylinder* cylinder)
		{
			_tile = dynamic_cast<Tile*>(cylinder);
			Thing::setParent(cylinder);
		}

		virtual Position getPosition() const {return _tile->getPosition();}
		virtual Tile* getTile() {return _tile;}
		virtual const Tile* getTile() const {return _tile;}
		int32_t getWalkCache(const Position& pos) const;

		const Position& getLastPosition() {return lastPosition;}
		void setLastPosition(Position newLastPos) {lastPosition = newLastPos;}
		static bool canSee(const Position& myPos, const Position& pos, uint32_t viewRangeX, uint32_t viewRangeY);

	protected:
		static const int32_t mapWalkWidth = Map::maxViewportX * 2 + 1;
		static const int32_t mapWalkHeight = Map::maxViewportY * 2 + 1;
		bool localMapCache[mapWalkHeight][mapWalkWidth];

		virtual bool useCacheMap() const {return false;}

		Tile* _tile;
		uint32_t id;
		bool removed;
		bool isMapLoaded;
		bool isUpdatingPath;
		bool checked;
		StorageMap storageMap;

		int32_t checkVector;
		int32_t health, healthMax;
		int32_t mana, manaMax;

		bool hideName, hideHealth, cannotMove;
		SpeakClasses speakType;

		Outfit_t currentOutfit;
		Outfit_t defaultOutfit;

		Position masterPosition;
		Position lastPosition;
		int32_t masterRadius;
		uint64_t lastStep;
		uint32_t lastStepCost;
		uint32_t baseSpeed;
		int32_t varSpeed;
		bool skillLoss;
		lootDrop_t lootDrop;
		Skulls_t skull;
		PartyShields_t partyShield;
		Direction direction;
		ConditionList conditions;
		LightInfo internalLight;

		//summon variables
		Creature* master;
		std::list<Creature*> summons;

		//follow variables
		Creature* followCreature;
		uint32_t eventWalk;
		std::list<Direction> listWalkDir;
		uint32_t walkUpdateTicks;
		bool hasFollowPath;
		bool forceUpdateFollowPath;

		//combat variables
		Creature* attackedCreature;
		struct CountBlock_t
		{
			uint32_t total;
			int64_t ticks, start;

			CountBlock_t(uint32_t points)
			{
				start = ticks = OTSYS_TIME();
				total = points;
			}

			CountBlock_t() {start = ticks = total = 0;}
		};

		typedef std::map<uint32_t, CountBlock_t> CountMap;
		CountMap damageMap;
		CountMap healMap;

		CreatureEventList eventsList;
		uint32_t scriptEventsBitField, blockCount, blockTicks, lastHitCreature;
		CombatType_t lastDamageSource;

		#ifdef __DEBUG__
		void validateMapCache();
		#endif
		void updateMapCache();

		void updateTileCache(const Tile* tile, int32_t dx, int32_t dy);
		void updateTileCache(const Tile* tile, const Position& pos);

		bool hasEventRegistered(CreatureEventType_t event) const {return (0 != (scriptEventsBitField & ((uint32_t)1 << event)));}
		virtual bool hasExtraSwing() {return false;}

		virtual uint16_t getLookCorpse() const {return 0;}
		virtual uint64_t getLostExperience() const {return 0;}

		virtual double getDamageRatio(Creature* attacker) const;
		virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;
		DeathList getKillers();

		virtual Item* createCorpse(DeathList deathList);
		virtual void dropLoot(Container* corpse) {}
		virtual void dropCorpse(DeathList deathList);

		virtual void doAttacking(uint32_t interval) {}
		void internalCreatureDisappear(const Creature* creature, bool isLogout);

		friend class Game;
		friend class Map;
		friend class LuaScriptInterface;
};
#endif
