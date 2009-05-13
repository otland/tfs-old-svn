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

#include "creature.h"
#include "player.h"
#include "npc.h"
#include "monster.h"

#include "condition.h"
#include "combat.h"

#include "container.h"
#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif

#include "configmanager.h"
#include "game.h"

OTSYS_THREAD_LOCKVAR AutoID::autoIDLock;
uint32_t AutoID::count = 1000;
AutoID::list_type AutoID::list;

extern Game g_game;
extern ConfigManager g_config;
extern CreatureEvents* g_creatureEvents;

Creature::Creature() :
isInternalRemoved(false)
{
	id = 0;
	_tile = NULL;
	direction = SOUTH;
	master = NULL;
	lootDrop = LOOT_DROP_FULL;
	skillLoss = true;
	hideName = hideHealth = cannotMove = false;
	skull = SKULL_NONE;
	partyShield = SHIELD_NONE;

	health = 1000;
	healthMax = 1000;
	mana = 0;
	manaMax = 0;

	lastStep = 0;
	lastStepCost = 1;
	baseSpeed = 220;
	varSpeed = 0;

	masterRadius = -1;
	masterPos.x = 0;
	masterPos.y = 0;
	masterPos.z = 0;

	followCreature = NULL;
	hasFollowPath = false;
	eventWalk = 0;
	forceUpdateFollowPath = false;
	isMapLoaded = false;
	isUpdatingPath = false;
	memset(localMapCache, false, sizeof(localMapCache));

	attackedCreature = NULL;
	lastHitCreatureId = 0;
	lastHitCreature = NULL;
	mostDamageCreature = NULL;
	blockCount = 0;
	blockTicks = 0;
	walkUpdateTicks = 0;
	checkCreatureVectorIndex = -1;

	scriptEventsBitField = 0;
	onIdleStatus();
}

Creature::~Creature()
{
	for(std::list<Creature*>::iterator cit = summons.begin(); cit != summons.end(); ++cit)
	{
		(*cit)->setAttackedCreature(NULL);
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
	}

	summons.clear();
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end(); ++it)
	{
		(*it)->endCondition(this, CONDITIONEND_CLEANUP);
		delete *it;
	}

	conditions.clear();
	attackedCreature = NULL;
	eventsList.clear();
}

bool Creature::canSee(const Position& myPos, const Position& pos, uint32_t viewRangeX, uint32_t viewRangeY)
{
	if(myPos.z <= 7)
	{
		//we are on ground level or above (7 -> 0)
		//view is from 7 -> 0
		if(pos.z > 7)
			return false;
	}
	else if(myPos.z >= 8)
	{
		//we are underground (8 -> 15)
		//view is +/- 2 from the floor we stand on
		if(std::abs(myPos.z - pos.z) > 2)
			return false;
	}

	int32_t offsetz = myPos.z - pos.z;
	return (((uint32_t)pos.x >= myPos.x - viewRangeX + offsetz) && ((uint32_t)pos.x <= myPos.x + viewRangeX + offsetz) &&
		((uint32_t)pos.y >= myPos.y - viewRangeY + offsetz) && ((uint32_t)pos.y <= myPos.y + viewRangeY + offsetz));
}

bool Creature::canSee(const Position& pos) const
{
	return canSee(getPosition(), pos, Map::maxViewportX, Map::maxViewportY);
}

bool Creature::canSeeCreature(const Creature* creature) const
{
	return canSeeInvisibility() || !creature->isInvisible();
}

int64_t Creature::getTimeSinceLastMove() const
{
	if(lastStep)
		return OTSYS_TIME() - lastStep;

	return 0x7FFFFFFFFFFFFFFFLL;
}

int64_t Creature::getSleepTicks() const
{
	if(!lastStep)
		return 0;

	int64_t ct = OTSYS_TIME(), stepDuration = getStepDuration();
	return stepDuration - (ct - lastStep);
}

int32_t Creature::getWalkDelay(Direction dir, uint32_t resolution) const
{
	if(!lastStep)
		return 0;

	float mul = 1.0f;
	if(dir == NORTHWEST || dir == NORTHEAST || dir == SOUTHWEST || dir == SOUTHEAST)
		mul = 3.0f;

	return ((int64_t)std::ceil(((double)getStepDuration(false) * mul) / resolution) * resolution) - (OTSYS_TIME() - lastStep);
}

void Creature::onThink(uint32_t interval)
{
	if(!isMapLoaded && useCacheMap())
	{
		isMapLoaded = true;
		updateMapCache();
	}

	if(followCreature && getMaster() != followCreature && !canSeeCreature(followCreature))
		onCreatureDisappear(followCreature, false);

	if(attackedCreature && getMaster() != attackedCreature && !canSeeCreature(attackedCreature))
		onCreatureDisappear(attackedCreature, false);

	blockTicks += interval;
	if(blockTicks >= 1000)
	{
		blockCount = std::min((uint32_t)blockCount + 1, (uint32_t)2);
		blockTicks = 0;
	}

	if(followCreature)
	{
		walkUpdateTicks += interval;
		if(forceUpdateFollowPath || walkUpdateTicks >= 2000)
		{
			walkUpdateTicks = 0;
			forceUpdateFollowPath = false;
			isUpdatingPath = true;
		}
	}

	if(isUpdatingPath)
	{
		isUpdatingPath = false;
		getPathToFollowCreature();
	}

	onAttacking(interval);
	executeConditions(interval);

	CreatureEventList thinkEvents = getCreatureEvents(CREATURE_EVENT_THINK);
	for(CreatureEventList::iterator it = thinkEvents.begin(); it != thinkEvents.end(); ++it)
		(*it)->executeThink(this, interval);
}

void Creature::onAttacking(uint32_t interval)
{
	if(!attackedCreature)
		return;

	CreatureEventList attackEvents = getCreatureEvents(CREATURE_EVENT_ATTACK);
	for(CreatureEventList::iterator it = attackEvents.begin(); it != attackEvents.end(); ++it)
	{
		if(!(*it)->executeAttack(this, attackedCreature) && attackedCreature)
			setAttackedCreature(NULL);
	}

	if(!attackedCreature)
		return;

	onAttacked();
	attackedCreature->onAttacked();
	if(g_game.isSightClear(getPosition(), attackedCreature->getPosition(), true))
		doAttacking(interval);
}

void Creature::onWalk()
{
	if(getSleepTicks() <= 0)
	{
		Direction dir;
		if(getNextStep(dir) && g_game.internalMoveCreature(this, dir, FLAG_IGNOREFIELDDAMAGE) != RET_NOERROR)
			forceUpdateFollowPath = true;
	}

	if(listWalkDir.empty())
		onWalkComplete();

	if(eventWalk)
	{
		eventWalk = 0;
		addEventWalk();
	}
}

void Creature::onWalk(Direction& dir)
{
	if(!hasCondition(CONDITION_DRUNK))
		return;

	uint32_t r = random_range(0, 16);
	if(r > 4)
		return;

	switch(r)
	{
		case 0:
			dir = NORTH;
			break;
		case 1:
			dir = WEST;
			break;
		case 3:
			dir = SOUTH;
			break;
		case 4:
			dir = EAST;
			break;
	}

	g_game.internalCreatureSay(this, SPEAK_MONSTER_SAY, "Hicks!");
}

bool Creature::getNextStep(Direction& dir)
{
	if(listWalkDir.empty())
		return false;

	dir = listWalkDir.front();
	listWalkDir.pop_front();
	onWalk(dir);
	return true;
}

bool Creature::startAutoWalk(std::list<Direction>& listDir)
{
	if(getPlayer() && getPlayer()->getNoMove())
	{
		getPlayer()->sendCancelWalk();
		return false;
	}

	listWalkDir = listDir;
	addEventWalk();
	return true;
}

void Creature::addEventWalk()
{
	if(eventWalk)
		return;

	int64_t ticks = getEventStepTicks();
	if(ticks > 0)
		eventWalk = Scheduler::getScheduler().addEvent(createSchedulerTask(ticks,
			boost::bind(&Game::checkCreatureWalk, &g_game, getID())));
}

void Creature::stopEventWalk()
{
	if(!eventWalk)
		return;

	Scheduler::getScheduler().stopEvent(eventWalk);
	eventWalk = 0;
	if(!listWalkDir.empty())
	{
		listWalkDir.clear();
		onWalkAborted();
	}
}

void Creature::updateMapCache()
{
	const Position& myPos = getPosition();
	Position pos(0, 0, myPos.z);

	Tile* tile;
	for(int32_t y = -((mapWalkHeight - 1) / 2); y <= ((mapWalkHeight - 1) / 2); ++y)
	{
		for(int32_t x = -((mapWalkWidth - 1) / 2); x <= ((mapWalkWidth - 1) / 2); ++x)
		{
			pos.x = myPos.x + x;
			pos.y = myPos.y + y;
			if((tile = g_game.getTile(pos.x, pos.y, myPos.z)))
				updateTileCache(tile, pos);
		}
	}
}

#ifdef __DEBUG__
void Creature::validateMapCache()
{
	const Position& myPos = getPosition();
	for(int32_t y = -((mapWalkHeight - 1) / 2); y <= ((mapWalkHeight - 1) / 2); ++y)
	{
		for(int32_t x = -((mapWalkWidth - 1) / 2); x <= ((mapWalkWidth - 1) / 2); ++x)
			getWalkCache(Position(myPos.x + x, myPos.y + y, myPos.z));
	}
}
#endif

void Creature::updateTileCache(const Tile* tile, int32_t dx, int32_t dy)
{
	if((std::abs(dx) <= (mapWalkWidth - 1) / 2) && (std::abs(dy) <= (mapWalkHeight - 1) / 2))
	{
		int32_t x = (mapWalkWidth - 1) / 2 + dx, y = (mapWalkHeight - 1) / 2 + dy;
		localMapCache[y][x] = (tile && tile->__queryAdd(0, this, 1,
			FLAG_PATHFINDING | FLAG_IGNOREFIELDDAMAGE) == RET_NOERROR);
	}
#ifdef __DEBUG__
	else
		std::cout << "Creature::updateTileCache out of range." << std::endl;
#endif
}

void Creature::updateTileCache(const Tile* tile, const Position& pos)
{
	const Position& myPos = getPosition();
	if(pos.z == myPos.z)
		updateTileCache(tile, pos.x - myPos.x, pos.y - myPos.y);
}

int32_t Creature::getWalkCache(const Position& pos) const
{
	if(!useCacheMap())
		return 2;

	const Position& myPos = getPosition();
	if(myPos.z != pos.z)
		return 0;

	if(pos == myPos)
		return 1;

	int32_t dx = pos.x - myPos.x, dy = pos.y - myPos.y;
	if((std::abs(dx) <= (mapWalkWidth - 1) / 2) && (std::abs(dy) <= (mapWalkHeight - 1) / 2))
	{
		int32_t x = (mapWalkWidth - 1) / 2 + dx, y = (mapWalkHeight - 1) / 2 + dy;
#ifdef __DEBUG__
		//testing
		Tile* tile = g_game.getTile(pos);
		if(tile && (tile->__queryAdd(0, this, 1, FLAG_PATHFINDING | FLAG_IGNOREFIELDDAMAGE) == RET_NOERROR))
		{
			if(!localMapCache[y][x])
				std::cout << "Wrong cache value" << std::endl;
		}
		else
		{
			if(localMapCache[y][x])
				std::cout << "Wrong cache value" << std::endl;
		}

#endif
		if(localMapCache[y][x])
			return 1;

		return 0;
	}

	//out of range
	return 2;
}

void Creature::onAddTileItem(const Tile* tile, const Position& pos, const Item* item)
{
	if(isMapLoaded && pos.z == getPosition().z)
		updateTileCache(tile, pos);
}

void Creature::onUpdateTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
	const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType)
{
	if(isMapLoaded && (oldType.blockSolid || oldType.blockPathFind || newType.blockPathFind
		|| newType.blockSolid) && pos.z == getPosition().z)
		updateTileCache(tile, pos);
}

void Creature::onRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos,
	const ItemType& iType, const Item* item)
{
	if(isMapLoaded && (iType.blockSolid || iType.blockPathFind ||
		iType.isGroundTile()) && pos.z == getPosition().z)
		updateTileCache(tile, pos);
}

void Creature::onCreatureAppear(const Creature* creature, bool isLogin)
{
	if(creature == this)
	{
		if(useCacheMap())
		{
			isMapLoaded = true;
			updateMapCache();
		}

		if(isLogin)
			setLastPosition(getPosition());
	}
	else if(isMapLoaded && creature->getPosition().z == getPosition().z)
		updateTileCache(creature->getTile(), creature->getPosition());
}

void Creature::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	onCreatureDisappear(creature, true);
	if(creature == this)
	{
		if(getMaster() && !getMaster()->isRemoved())
			getMaster()->removeSummon(this);
	}
	else if(isMapLoaded && creature->getPosition().z == getPosition().z)
		updateTileCache(creature->getTile(), creature->getPosition());
}

void Creature::onCreatureDisappear(const Creature* creature, bool isLogout)
{
	if(attackedCreature == creature)
	{
		setAttackedCreature(NULL);
		onAttackedCreatureDisappear(isLogout);
	}

	if(followCreature == creature)
	{
		setFollowCreature(NULL);
		onFollowCreatureDisappear(isLogout);
	}
}

void Creature::onChangeZone(ZoneType_t zone)
{
	if(attackedCreature && zone == ZONE_PROTECTION)
		onCreatureDisappear(attackedCreature, false);
}

void Creature::onAttackedCreatureChangeZone(ZoneType_t zone)
{
	if(zone == ZONE_PROTECTION)
		onCreatureDisappear(attackedCreature, false);
}

void Creature::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	if(creature == this)
	{
		lastStep = OTSYS_TIME();
		lastStepCost = 1;
		if(!teleport)
		{
			if(oldPos.z != newPos.z)
				lastStepCost = 2;
			else if(std::abs(newPos.x - oldPos.x) >= 1 && std::abs(newPos.y - oldPos.y) >= 1)
				lastStepCost = 3;
		}
		else
			stopEventWalk();

		if(!summons.empty())
		{
			std::list<Creature*>::iterator cit;
			std::list<Creature*> despawnList;
			for(cit = summons.begin(); cit != summons.end(); ++cit)
			{
				const Position pos = (*cit)->getPosition();
				if((std::abs(pos.z - newPos.z) > 2) ||
					(std::max(std::abs((newPos.x) - pos.x), std::abs((newPos.y - 1) - pos.y)) > 30))
				{
					despawnList.push_back((*cit));
				}
			}

			for(cit = despawnList.begin(); cit != despawnList.end(); ++cit)
				g_game.removeCreature((*cit), true);
		}

		onChangeZone(getZone());
		//update map cache
		if(isMapLoaded)
		{
			if(!teleport && oldPos.z == newPos.z)
			{
				Tile* tile;
				const Position& myPos = getPosition();
				if(oldPos.y > newPos.y) //north
				{
					//shift y south
					for(int32_t y = mapWalkHeight - 1 - 1; y >= 0; --y)
						memcpy(localMapCache[y + 1], localMapCache[y], sizeof(localMapCache[y]));

					//update 0
					for(int32_t x = -((mapWalkWidth - 1) / 2); x <= ((mapWalkWidth - 1) / 2); ++x)
					{
						tile = g_game.getTile(myPos.x + x, myPos.y - ((mapWalkHeight - 1) / 2), myPos.z);
						updateTileCache(tile, x, -((mapWalkHeight - 1) / 2));
					}
				}
				else if(oldPos.y < newPos.y) // south
				{
					//shift y north
					for(int32_t y = 0; y <= mapWalkHeight - 1 - 1; ++y)
						memcpy(localMapCache[y], localMapCache[y + 1], sizeof(localMapCache[y]));

					//update mapWalkHeight - 1
					for(int32_t x = -((mapWalkWidth - 1) / 2); x <= ((mapWalkWidth - 1) / 2); ++x)
					{
						tile = g_game.getTile(myPos.x + x, myPos.y + ((mapWalkHeight - 1) / 2), myPos.z);
						updateTileCache(tile, x, (mapWalkHeight - 1) / 2);
					}
				}

				if(oldPos.x < newPos.x) // east
				{
					//shift y west
					int32_t starty = 0, endy = mapWalkHeight - 1, dy = (oldPos.y - newPos.y);
					if(dy < 0)
						endy = endy + dy;
					else if(dy > 0)
						starty = starty + dy;

					for(int32_t y = starty; y <= endy; ++y)
					{
						for(int32_t x = 0; x <= mapWalkWidth - 1 - 1; ++x)
							localMapCache[y][x] = localMapCache[y][x + 1];
					}

					//update mapWalkWidth - 1
					for(int32_t y = -((mapWalkHeight - 1) / 2); y <= ((mapWalkHeight - 1) / 2); ++y)
					{
						tile = g_game.getTile(myPos.x + ((mapWalkWidth - 1) / 2), myPos.y + y, myPos.z);
						updateTileCache(tile, (mapWalkWidth - 1) / 2, y);
					}
				}
				else if(oldPos.x > newPos.x) // west
				{
					//shift y east
					int32_t starty = 0, endy = mapWalkHeight - 1, dy = (oldPos.y - newPos.y);
					if(dy < 0)
						endy = endy + dy;
					else if(dy > 0)
						starty = starty + dy;

					for(int32_t y = starty; y <= endy; ++y)
					{
						for(int32_t x = mapWalkWidth - 1 - 1; x >= 0; --x)
							localMapCache[y][x + 1] = localMapCache[y][x];
					}

					//update 0
					for(int32_t y = -((mapWalkHeight - 1) / 2); y <= ((mapWalkHeight - 1) / 2); ++y)
					{
						tile = g_game.getTile(myPos.x - ((mapWalkWidth - 1) / 2), myPos.y + y, myPos.z);
						updateTileCache(tile, -((mapWalkWidth - 1) / 2), y);
					}
				}

				updateTileCache(oldTile, oldPos);
#ifdef __DEBUG__
				validateMapCache();
#endif
			}
			else
				updateMapCache();
		}
	}
	else if(isMapLoaded)
	{
		const Position& myPos = getPosition();
		if(newPos.z == myPos.z)
			updateTileCache(newTile, newPos);

		if(oldPos.z == myPos.z)
			updateTileCache(oldTile, oldPos);
	}

	if(creature == followCreature || (creature == this && followCreature))
	{
		if(hasFollowPath)
		{
			isUpdatingPath = true;
			Dispatcher::getDispatcher().addTask(createTask(
				boost::bind(&Game::updateCreatureWalk, &g_game, getID())));
		}

		if(newPos.z != oldPos.z || !canSee(followCreature->getPosition()))
			onCreatureDisappear(followCreature, false);
	}

	if(creature == attackedCreature || (creature == this && attackedCreature))
	{
		if(newPos.z == oldPos.z && canSee(attackedCreature->getPosition()))
		{
			if(hasExtraSwing())
			{
				//our target is moving lets see if we can get in hit
				Dispatcher::getDispatcher().addTask(createTask(
					boost::bind(&Game::checkCreatureAttack, &g_game, getID())));
			}

			onAttackedCreatureChangeZone(attackedCreature->getZone());
		}
		else
			onCreatureDisappear(attackedCreature, false);
	}
}

bool Creature::onDeath()
{
	bool lastHitKiller = false, mostDamageKiller = false;
	if(getKillers(&lastHitCreature, &mostDamageCreature))
	{
		Creature* lastHitCreatureMaster = NULL;
		if(lastHitCreature)
		{
			lastHitCreatureMaster = lastHitCreature->getMaster();
			lastHitKiller = true;
		}

		Creature* mostDamageCreatureMaster = NULL;
		if(mostDamageCreature)
		{
			mostDamageCreatureMaster = mostDamageCreature->getMaster();
			if(mostDamageCreature != lastHitCreature && mostDamageCreature != lastHitCreatureMaster
				&& lastHitCreature != mostDamageCreatureMaster && (!lastHitCreatureMaster ||
				mostDamageCreatureMaster != lastHitCreatureMaster))
				mostDamageKiller = true;
		}
	}

	bool deny = false;
	CreatureEventList prepareDeathEvents = getCreatureEvents(CREATURE_EVENT_PREPAREDEATH);
	for(CreatureEventList::iterator it = prepareDeathEvents.begin(); it != prepareDeathEvents.end(); ++it)
	{
		if(!(*it)->executePrepareDeath(this, lastHitCreature, mostDamageCreature))
			deny = true;
	}

	if(deny)
		return false;

	if(lastHitKiller && lastHitCreature &&
		!lastHitCreature->onKilledCreature(this))
			deny = true;

	if(mostDamageKiller && mostDamageCreature &&
		!mostDamageCreature->onKilledCreature(this))
			deny = true;

	if(deny)
		return false;

	for(CountMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it)
	{
		if(Creature* attacker = g_game.getCreatureByID((*it).first))
			attacker->onAttackedCreatureKilled(this);
	}

	if(getMaster())
		getMaster()->removeSummon(this);

	dropCorpse();
	return true;
}

void Creature::dropCorpse()
{
	Item* corpse = getCorpse();
	if(Tile* tile = getTile())
	{
		Item* splash = NULL;
		switch(getRace())
		{
			case RACE_VENOM:
				splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_GREEN);
				break;

			case RACE_BLOOD:
				splash = Item::CreateItem(ITEM_FULLSPLASH, FLUID_BLOOD);
				break;

			case RACE_UNDEAD:
			case RACE_FIRE:
			case RACE_ENERGY:
			default:
				break;
		}

		if(splash)
		{
			g_game.internalAddItem(NULL, tile, splash, INDEX_WHEREEVER, FLAG_NOLIMIT);
			g_game.startDecay(splash);
		}

		if(corpse)
		{
			g_game.internalAddItem(NULL, tile, corpse, INDEX_WHEREEVER, FLAG_NOLIMIT);
			dropLoot(corpse->getContainer());
			g_game.startDecay(corpse);
		}
	}

	CreatureEventList deathEvents = getCreatureEvents(CREATURE_EVENT_DEATH);
	for(CreatureEventList::iterator it = deathEvents.begin(); it != deathEvents.end(); ++it)
		(*it)->executeDeath(this, corpse, lastHitCreature, mostDamageCreature);
}

bool Creature::getKillers(Creature** _lastHitCreature, Creature** _mostDamageCreature)
{
	uint32_t mostDamage = 0;

	CountBlock_t cb;
	for(CountMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it)
	{
		cb = it->second;
		if((cb.total > mostDamage && (OTSYS_TIME() - cb.ticks <= g_game.getInFightTicks())))
		{
			if((*_mostDamageCreature = g_game.getCreatureByID((*it).first)))
				mostDamage = cb.total;
		}
	}

	*_lastHitCreature = g_game.getCreatureByID(lastHitCreatureId);
	return (*_lastHitCreature || *_mostDamageCreature);
}

bool Creature::hasBeenAttacked(uint32_t attackerId) const
{
	CountMap::const_iterator it = damageMap.find(attackerId);
	if(it == damageMap.end())
		return false;

	return (OTSYS_TIME() - it->second.ticks <= g_game.getInFightTicks());
}

Item* Creature::getCorpse()
{
	return Item::CreateItem(getLookCorpse());
}

void Creature::changeHealth(int32_t healthChange)
{
	if(healthChange > 0)
		health += std::min(healthChange, getMaxHealth() - health);
	else
		health = std::max((int32_t)0, health + healthChange);

	g_game.addCreatureHealth(this);
}

void Creature::changeMana(int32_t manaChange)
{
	if(manaChange > 0)
		mana += std::min(manaChange, getMaxMana() - mana);
	else
		mana = std::max((int32_t)0, mana + manaChange);
}

void Creature::gainHealth(Creature* caster, int32_t healthGain)
{
	if(healthGain > 0)
	{
		int32_t prevHealth = getHealth();
		changeHealth(healthGain);

		int32_t effectiveGain = getHealth() - prevHealth;
		if(caster)
			caster->onTargetCreatureGainHealth(this, effectiveGain);
	}
	else
		changeHealth(healthGain);
}

void Creature::drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage)
{
	changeHealth(-damage);
	if(attacker)
		attacker->onAttackedCreatureDrainHealth(this, damage);
}

void Creature::drainMana(Creature* attacker, int32_t manaLoss)
{
	onAttacked();
	changeMana(-manaLoss);
}

BlockType_t Creature::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
	bool checkDefense/* = false*/, bool checkArmor/* = false*/)
{
	BlockType_t blockType = BLOCK_NONE;
	if(isImmune(combatType))
	{
		damage = 0;
		blockType = BLOCK_IMMUNITY;
	}
	else if(checkDefense || checkArmor)
	{
		bool hasDefense = false;
		if(blockCount > 0)
		{
			--blockCount;
			hasDefense = true;
		}

		if(checkDefense && hasDefense)
		{
			int32_t maxDefense = getDefense(), minDefense = maxDefense / 2;
			damage -= random_range(minDefense, maxDefense);
			if(damage <= 0)
			{
				damage = 0;
				blockType = BLOCK_DEFENSE;
				checkArmor = false;
			}
		}

		if(checkArmor)
		{
			int32_t armorValue = getArmor(), minArmorReduction = 0, maxArmorReduction = 0;
			if(armorValue > 1)
			{
				minArmorReduction = (int32_t)std::ceil(armorValue * 0.475);
				maxArmorReduction = (int32_t)std::ceil(((armorValue * 0.475) - 1) + minArmorReduction);
			}
			else if(armorValue == 1)
			{
				minArmorReduction = 1;
				maxArmorReduction = 1;
			}

			damage -= random_range(minArmorReduction, maxArmorReduction);
			if(damage <= 0)
			{
				damage = 0;
				blockType = BLOCK_ARMOR;
			}
		}

		if(hasDefense && blockType != BLOCK_NONE)
			onBlockHit(blockType);
	}

	if(attacker)
	{
		attacker->onAttackedCreature(this);
		attacker->onAttackedCreatureBlockHit(this, blockType);
	}

	onAttacked();
	return blockType;
}

bool Creature::setAttackedCreature(Creature* creature)
{
	if(creature)
	{
		const Position& creaturePos = creature->getPosition();
		if(creaturePos.z != getPosition().z || !canSee(creaturePos))
		{
			attackedCreature = NULL;
			return false;
		}
	}

	attackedCreature = creature;
	if(attackedCreature)
	{
		onAttackedCreature(attackedCreature);
		attackedCreature->onAttacked();
	}

	for(std::list<Creature*>::iterator cit = summons.begin(); cit != summons.end(); ++cit)
		(*cit)->setAttackedCreature(creature);

	return true;
}

void Creature::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	fpp.fullPathSearch = !hasFollowPath;
	fpp.clearSight = true;
	fpp.maxSearchDist = 12;
	fpp.minTargetDist = fpp.maxTargetDist = 1;
}

void Creature::getPathToFollowCreature()
{
	if(followCreature)
	{
		FindPathParams fpp;
		getPathSearchParams(followCreature, fpp);
		if(g_game.getPathToEx(this, followCreature->getPosition(), listWalkDir, fpp))
		{
			hasFollowPath = true;
			startAutoWalk(listWalkDir);
		}
		else
			hasFollowPath = false;
	}

	onFollowCreatureComplete(followCreature);
}

bool Creature::setFollowCreature(Creature* creature, bool fullPathSearch /*= false*/)
{
	if(creature)
	{
		if(followCreature == creature)
			return true;

		const Position& creaturePos = creature->getPosition();
		if(creaturePos.z != getPosition().z || !canSee(creaturePos))
		{
			followCreature = NULL;
			return false;
		}

		if(!listWalkDir.empty())
		{
			listWalkDir.clear();
			onWalkAborted();
		}

		hasFollowPath = forceUpdateFollowPath = false;
		followCreature = creature;
		isUpdatingPath = true;
	}
	else
	{
		isUpdatingPath = false;
		followCreature = NULL;
	}

	onFollowCreature(creature);
	return true;
}

double Creature::getDamageRatio(Creature* attacker) const
{
	int32_t totalDamage = 0, attackerDamage = 0;
	for(CountMap::const_iterator it = damageMap.begin(); it != damageMap.end(); ++it)
	{
		totalDamage += it->second.total;
		if(it->first == attacker->getID())
			attackerDamage += it->second.total;
	}

	return ((double)attackerDamage / totalDamage);
}

uint64_t Creature::getGainedExperience(Creature* attacker, bool useMultiplier/* = true*/)
{
	Player* player = attacker->getPlayer();
	if(!player && attacker->getMaster())
		player = attacker->getMaster()->getPlayer();

	double baseExperience = getDamageRatio(attacker) * getLostExperience();
	if(!player)
		return (uint64_t)std::floor(baseExperience * g_config.getDouble(ConfigManager::RATE_EXPERIENCE));

	if(player->hasFlag(PlayerFlag_NotGainExperience))
		return 0;

	if(useMultiplier)
		baseExperience *= player->rates[SKILL__LEVEL];

	baseExperience *= g_game.getExperienceStage(player->getLevel());
	if(!player->hasFlag(PlayerFlag_HasInfiniteStamina))
	{
		int64_t totalTime = 0;
		for(CountMap::const_iterator it = damageMap.begin(); it != damageMap.end(); ++it)
		{
			if(it->first == attacker->getID())
				totalTime += it->second.ticks - it->second.start;
		}

		player->removeStamina(totalTime * g_config.getNumber(ConfigManager::RATE_STAMINA_LOSS));
	}

	int32_t minutes = player->getStaminaMinutes();
	if(!getPlayer() && minutes >= g_config.getNumber(ConfigManager::STAMINA_LIMIT_TOP) &&
		(player->isPremium() || !g_config.getNumber(ConfigManager::STAMINA_BONUS_PREMIUM)))
		baseExperience *= g_config.getDouble(ConfigManager::RATE_STAMINA_ABOVE);
	else if(minutes < (g_config.getNumber(ConfigManager::STAMINA_LIMIT_BOTTOM)) && minutes > 0)
		baseExperience *= g_config.getDouble(ConfigManager::RATE_STAMINA_UNDER);
	else if(minutes <= 0)
		baseExperience = 0;

	return (uint64_t)std::floor(baseExperience);
}

void Creature::addDamagePoints(Creature* attacker, int32_t damagePoints)
{
	uint32_t attackerId = (attacker ? attacker->getID() : 0);
	CountMap::iterator it = damageMap.find(attackerId);
	if(it == damageMap.end())
	{
		CountBlock_t cb;
		cb.ticks = cb.start = OTSYS_TIME();

		cb.total = damagePoints;
		damageMap[attackerId] = cb;
	}
	else
	{
		it->second.ticks = OTSYS_TIME();
		if(damagePoints > 0)
			it->second.total += damagePoints;
	}

	if(damagePoints > 0)
		lastHitCreatureId = attackerId;
}

void Creature::addHealPoints(Creature* caster, int32_t healthPoints)
{
	if(healthPoints <= 0)
		return;

	uint32_t casterId = (caster ? caster->getID() : 0);
	CountMap::iterator it = healMap.find(casterId);
	if(it == healMap.end())
	{
		CountBlock_t cb;
		cb.ticks = cb.start = OTSYS_TIME();

		cb.total = healthPoints;
		healMap[casterId] = cb;
	}
	else
	{
		it->second.ticks = OTSYS_TIME();
		it->second.total += healthPoints;
	}
}

void Creature::onAddCondition(ConditionType_t type)
{
	if(type == CONDITION_PARALYZE && hasCondition(CONDITION_HASTE))
		removeCondition(CONDITION_HASTE);
	else if(type == CONDITION_HASTE && hasCondition(CONDITION_PARALYZE))
		removeCondition(CONDITION_PARALYZE);
}

void Creature::onTickCondition(ConditionType_t type, int32_t interval, bool& _remove)
{
	if(const MagicField* field = getTile()->getFieldItem())
	{
		switch(type)
		{
			case CONDITION_FIRE:
				_remove = (field->getCombatType() != COMBAT_FIREDAMAGE);
				break;
			case CONDITION_ENERGY:
				_remove = (field->getCombatType() != COMBAT_ENERGYDAMAGE);
				break;
			case CONDITION_POISON:
				_remove = (field->getCombatType() != COMBAT_EARTHDAMAGE);
				break;
			case CONDITION_DROWN:
				_remove = (field->getCombatType() != COMBAT_DROWNDAMAGE);
				break;
			default:
				break;
		}
	}
}

void Creature::onCombatRemoveCondition(const Creature* attacker, Condition* condition)
{
	removeCondition(condition);
}

void Creature::onIdleStatus()
{
	if(getHealth() > 0)
	{
		healMap.clear();
		damageMap.clear();
	}
}

void Creature::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	target->addDamagePoints(this, points);
}

void Creature::onTargetCreatureGainHealth(Creature* target, int32_t points)
{
	target->addHealPoints(this, points);
}

void Creature::onAttackedCreatureKilled(Creature* target)
{
	if(target != this)
		onGainExperience(target->getGainedExperience(this));
}

bool Creature::onKilledCreature(Creature* target)
{
	bool ret = true;
	if(getMaster())
		ret = getMaster()->onKilledCreature(target);

	CreatureEventList killEvents = getCreatureEvents(CREATURE_EVENT_KILL);
	for(CreatureEventList::iterator it = killEvents.begin(); it != killEvents.end(); ++it)
	{
		if(!(*it)->executeKill(this, target))
			ret = false;
	}

	return ret;
}

void Creature::onGainExperience(uint64_t gainExp)
{
	if(gainExp <= 0)
		return;

	if(getMaster())
	{
		gainExp = gainExp / 2;
		getMaster()->onGainExperience(gainExp);
	}

	std::stringstream ss;
	ss << gainExp;
	g_game.addAnimatedText(getPosition(), g_config.getNumber(ConfigManager::EXPERIENCE_COLOR), ss.str());
}

void Creature::onGainSharedExperience(uint64_t gainExp)
{
	if(gainExp <= 0)
		return;

	std::stringstream ss;
	ss << gainExp;
	g_game.addAnimatedText(getPosition(), g_config.getNumber(ConfigManager::EXPERIENCE_COLOR), ss.str());
}

void Creature::addSummon(Creature* creature)
{
	creature->setDropLoot(LOOT_DROP_NONE);
	creature->setLossSkill(false);

	creature->setMaster(this);
	creature->useThing2();

	summons.push_back(creature);
}

void Creature::removeSummon(const Creature* creature)
{
	std::list<Creature*>::iterator cit = std::find(summons.begin(), summons.end(), creature);
	if(cit != summons.end())
	{
		(*cit)->setDropLoot(LOOT_DROP_NONE);
		(*cit)->setLossSkill(false);

		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();

		summons.erase(cit);
	}
}

bool Creature::addCondition(Condition* condition)
{
	if(condition == NULL)
		return false;

	if(Condition* previous = getCondition(condition->getType(), condition->getId(), condition->getSubId()))
	{
		previous->addCondition(this, condition);
		delete condition;
		return true;
	}

	if(condition->startCondition(this))
	{
		conditions.push_back(condition);
		onAddCondition(condition->getType());
		return true;
	}

	delete condition;
	return false;
}

bool Creature::addCombatCondition(Condition* condition)
{
	//Caution: condition variable could be deleted after the call to addCondition
	ConditionType_t type = condition->getType();
	if(!addCondition(condition))
		return false;

	onAddCombatCondition(type);
	return true;
}

void Creature::removeCondition(ConditionType_t type)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();)
	{
		if((*it)->getType() != type)
		{
			++it;
			continue;
		}

		Condition* condition = *it;
		it = conditions.erase(it);

		condition->endCondition(this, CONDITIONEND_ABORT);
		onEndCondition(condition->getType());
		delete condition;
	}
}

void Creature::removeCondition(ConditionType_t type, ConditionId_t id)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();)
	{
		if((*it)->getType() != type || (*it)->getId() != id)
		{
			++it;
			continue;
		}

		Condition* condition = *it;
		it = conditions.erase(it);

		condition->endCondition(this, CONDITIONEND_ABORT);
		onEndCondition(condition->getType());
		delete condition;
	}
}

void Creature::removeCondition(Condition* condition)
{
	ConditionList::iterator it = std::find(conditions.begin(), conditions.end(), condition);
	if(it != conditions.end())
	{
		Condition* condition = *it;
		it = conditions.erase(it);

		condition->endCondition(this, CONDITIONEND_ABORT);
		onEndCondition(condition->getType());
		delete condition;
	}
}

void Creature::removeCondition(const Creature* attacker, ConditionType_t type)
{
	ConditionList tmpList = conditions;
	for(ConditionList::iterator it = tmpList.begin(); it != tmpList.end(); ++it)
	{
		if((*it)->getType() == type)
			onCombatRemoveCondition(attacker, *it);
	}
}

void Creature::removeConditions(ConditionEnd_t reason, bool onlyPersistent/* = true*/)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();)
	{
		if(onlyPersistent && !(*it)->isPersistent())
		{
			++it;
			continue;
		}

		Condition* condition = *it;
		it = conditions.erase(it);

		condition->endCondition(this, reason);
		onEndCondition(condition->getType());
		delete condition;
	}
}

Condition* Creature::getCondition(ConditionType_t type, ConditionId_t id, uint32_t subId/* = 0*/) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it)
	{
		if((*it)->getType() == type && (*it)->getId() == id && (*it)->getSubId() == subId)
			return *it;
	}

	return NULL;
}

void Creature::executeConditions(uint32_t interval)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();)
	{
		if((*it)->executeCondition(this, interval))
		{
			++it;
			continue;
		}

		Condition* condition = *it;
		it = conditions.erase(it);

		condition->endCondition(this, CONDITIONEND_TICKS);
		onEndCondition(condition->getType());
		delete condition;
	}
}

bool Creature::hasCondition(ConditionType_t type, uint32_t subId/* = 0*/, bool checkTime/* = true*/) const
{
	if(type == CONDITION_EXHAUST && g_game.getStateDelay() == 0)
		return true;

	if(isSuppress(type))
		return false;

	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it)
	{
		if((*it)->getType() != type || (*it)->getSubId() != subId)
			continue;

		if(!checkTime || g_config.getBool(ConfigManager::OLD_CONDITION_ACCURACY))
			return true;

		if((*it)->getEndTime() == 0)
			return true;

		int64_t seekTime = g_game.getStateDelay();
		if(seekTime == 0)
			return true;

		if((*it)->getEndTime() > seekTime)
			seekTime = (*it)->getEndTime();

		if(seekTime >= OTSYS_TIME())
			return true;
	}

	return false;
}

bool Creature::isImmune(CombatType_t type) const
{
	return ((getDamageImmunities() & (uint32_t)type) == (uint32_t)type);
}

bool Creature::isImmune(ConditionType_t type) const
{
	return ((getConditionImmunities() & (uint32_t)type) == (uint32_t)type);
}

bool Creature::isSuppress(ConditionType_t type) const
{
	return ((getConditionSuppressions() & (uint32_t)type) == (uint32_t)type);
}

std::string Creature::getDescription(int32_t lookDistance) const
{
	return "a creature";
}

int32_t Creature::getStepDuration(bool addLastStepCost/* = true*/) const
{
	if(isRemoved())
		return 0;

	const Tile* tile = getTile();
	if(!tile || !tile->ground)
		return 0;

	uint32_t stepSpeed = getStepSpeed();
	if(!stepSpeed)
		return 0;

	return (1000 * Item::items[tile->ground->getID()].speed) / stepSpeed * (addLastStepCost ? lastStepCost : 1);
}

int64_t Creature::getEventStepTicks() const
{
	int64_t ret = getSleepTicks();
	if(ret > 0)
		return ret;

	return getStepDuration();
}

void Creature::getCreatureLight(LightInfo& light) const
{
	light = internalLight;
}

void Creature::setNormalCreatureLight()
{
	internalLight.level = internalLight.color = 0;
}

bool Creature::registerCreatureEvent(const std::string& name)
{
	CreatureEvent* event = g_creatureEvents->getEventByName(name);
	if(!event)
		return false;

	if(!hasEventRegistered(event->getEventType())) //wasn't added, so set the bit in the bitfield
		scriptEventsBitField = scriptEventsBitField | ((uint32_t)1 << event->getEventType());

	eventsList.push_back(event);
	return true;
}

CreatureEventList Creature::getCreatureEvents(CreatureEventType_t type)
{
	CreatureEventList retList;
	if(hasEventRegistered(type))
	{
		for(CreatureEventList::iterator it = eventsList.begin(); it != eventsList.end(); ++it)
		{
			if((*it)->getEventType() == type)
				retList.push_back(*it);
		}
	}

	return retList;
}

FrozenPathingConditionCall::FrozenPathingConditionCall(const Position& _targetPos)
{
	targetPos = _targetPos;
}

bool FrozenPathingConditionCall::isInRange(const Position& startPos, const Position& testPos,
	const FindPathParams& fpp) const
{
	int32_t dxMin = ((fpp.fullPathSearch || (startPos.x - targetPos.x) <= 0) ? fpp.maxTargetDist : 0),
	dxMax = ((fpp.fullPathSearch || (startPos.x - targetPos.x) >= 0) ? fpp.maxTargetDist : 0),
	dyMin = ((fpp.fullPathSearch || (startPos.y - targetPos.y) <= 0) ? fpp.maxTargetDist : 0),
	dyMax = ((fpp.fullPathSearch || (startPos.y - targetPos.y) >= 0) ? fpp.maxTargetDist : 0);
	if(testPos.x > targetPos.x + dxMax || testPos.x < targetPos.x - dxMin)
		return false;

	if(testPos.y > targetPos.y + dyMax || testPos.y < targetPos.y - dyMin)
		return false;

	return true;
}

bool FrozenPathingConditionCall::operator()(const Position& startPos, const Position& testPos,
	const FindPathParams& fpp, int32_t& bestMatchDist) const
{
	if(!isInRange(startPos, testPos, fpp))
		return false;

	if(fpp.clearSight && !g_game.isSightClear(testPos, targetPos, true))
		return false;

	int32_t testDist = std::max(std::abs(targetPos.x - testPos.x), std::abs(targetPos.y - testPos.y));
	if(fpp.maxTargetDist == 1)
		return (testDist >= fpp.minTargetDist && testDist <= fpp.maxTargetDist);

	if(testDist <= fpp.maxTargetDist)
	{
		if(testDist < fpp.minTargetDist)
			return false;

		if(testDist == fpp.maxTargetDist)
		{
			bestMatchDist = 0;
			return true;
		}
		else if(testDist > bestMatchDist)
		{
			//not quite what we want, but the best so far
			bestMatchDist = testDist;
			return true;
		}
	}

	return false;
}
