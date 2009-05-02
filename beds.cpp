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

#include "beds.h"
#include "house.h"

#include "player.h"
#include "iologindata.h"

#include "game.h"
#include "configmanager.h"

extern Game g_game;
extern ConfigManager g_config;

BedItem::BedItem(uint16_t _id) : Item(_id)
{
	house = NULL;
	internalRemoveSleeper();
}

bool BedItem::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case ATTR_SLEEPERGUID:
		{
			uint32_t _guid;
			if(!propStream.GET_ULONG(_guid))
				return false;

			if(_guid != 0)
			{
				std::string name;
				if(!IOLoginData::getInstance()->getNameByGuid(_guid, name))
					return false;

				setSpecialDescription(name + " is sleeping there.");
				Beds::getInstance().setBedSleeper(this, _guid);
			}

			sleeperGUID = _guid;
			return true;
		}

		case ATTR_SLEEPSTART:
		{
			uint32_t sleep_start;
			if(!propStream.GET_ULONG(sleep_start))
				return false;

			sleepStart = (uint64_t)sleep_start;
			return true;
		}

		default:
			break;
	}

	return Item::readAttr(attr, propStream);
}

bool BedItem::serializeAttr(PropWriteStream& propWriteStream) const
{
	if(sleeperGUID != 0)
	{
		propWriteStream.ADD_UCHAR(ATTR_SLEEPERGUID);
		propWriteStream.ADD_ULONG(sleeperGUID);
	}

	if(sleepStart != 0)
	{
		propWriteStream.ADD_UCHAR(ATTR_SLEEPSTART);
		propWriteStream.ADD_ULONG((int32_t)sleepStart);
	}

	return true;
}

BedItem* BedItem::getNextBedItem()
{
	if(Tile* tile = g_game.getMap()->getTile(getNextPosition(Item::items[getID()].bedPartnerDir, getPosition())))
		return tile->getBedItem();

	return NULL;
}

bool BedItem::canUse(Player* player)
{
	if(!house || !player || player->isRemoved())
		return false;

	if(!player->isPremium() && g_config.getBool(ConfigManager::BED_REQUIRE_PREMIUM))
		return false;

	if(player->hasCondition(CONDITION_INFIGHT))
		return false;

	if(sleeperGUID != 0 && house->getHouseAccessLevel(player) != HOUSE_OWNER)
	{
		std::string name;
		if(IOLoginData::getInstance()->getNameByGuid(sleeperGUID, name))
		{
			Player* sleeper = new Player(name, NULL);
			if(IOLoginData::getInstance()->loadPlayer(sleeper, name) && house->getHouseAccessLevel(sleeper) > house->getHouseAccessLevel(player))
				return false;

			delete sleeper;
		}
	}

	return isBed();
}

void BedItem::sleep(Player* player)
{
	if(!house || !player || player->isRemoved())
		return;

	if(sleeperGUID != 0)
	{
		if(house->getHouseOwner() != player->getGUID() || Item::items[getID()].transformToFree == 0)
		{
			player->sendCancelMessage(RET_NOTPOSSIBLE);
			return;
		}

		wakeUp(NULL);
		g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
	}
	else
	{
		Beds::getInstance().setBedSleeper(this, player->getGUID());
		BedItem* nextBedItem = getNextBedItem();

		internalSetSleeper(player);
		if(nextBedItem)
			nextBedItem->internalSetSleeper(player);

		updateAppearance(player);
		if(nextBedItem)
			nextBedItem->updateAppearance(player);

		player->getTile()->moveCreature(player, getTile());
		g_game.addMagicEffect(player->getPosition(), NM_ME_SLEEP);
		Scheduler::getScheduler().addEvent(createSchedulerTask(SCHEDULER_MINTICKS, boost::bind(&Game::kickPlayer, &g_game, player->getID(), false)));
	}
}

void BedItem::wakeUp(Player* player)
{
	if(!house)
		return;

	if(sleeperGUID != 0)
	{
		if(player)
		{
			regeneratePlayer(player);
			g_game.addCreatureHealth(player);
		}
		else
		{
			std::string name;
			if(IOLoginData::getInstance()->getNameByGuid(sleeperGUID, name))
			{
				Player* _player = new Player(name, NULL);
				if(IOLoginData::getInstance()->loadPlayer(_player, name))
				{
					regeneratePlayer(_player);
					IOLoginData::getInstance()->savePlayer(_player);
				}

				delete _player;
			}
		}
	}

	Beds::getInstance().setBedSleeper(NULL, sleeperGUID);
	BedItem* nextBedItem = getNextBedItem();

	internalRemoveSleeper();
	if(nextBedItem)
		nextBedItem->internalRemoveSleeper();

	updateAppearance(NULL);
	if(nextBedItem)
		nextBedItem->updateAppearance(NULL);
}

void BedItem::regeneratePlayer(Player* player) const
{
	int32_t sleptTime = int32_t(time(NULL) - sleepStart);
	if(Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT))
	{
		int32_t amount = sleptTime / 30;
		if(condition->getTicks() != -1)
		{
			amount = std::min((condition->getTicks()/1000), sleptTime) / 30;
			int32_t tmp = condition->getTicks() - (amount * 30000);
			if(tmp > 0)
				condition->setTicks(tmp);
			else
			{
				player->removeCondition(condition);
				condition = NULL;
			}
		}

		player->changeHealth(amount);
		player->changeMana(amount);
	}

	player->changeSoul((int32_t)std::max((float)0, ((float)sleptTime / (60 * 15))));
}

void BedItem::updateAppearance(const Player* player)
{
	const ItemType& it = Item::items[getID()];
	if(it.type != ITEM_TYPE_BED)
		return;

	if(player && it.transformToOnUse[player->getSex()] != 0)
	{
		const ItemType& newType = Item::items[it.transformToOnUse[player->getSex()]];
		if(newType.type == ITEM_TYPE_BED)
			g_game.transformItem(this, it.transformToOnUse[player->getSex()]);
	}
	else if(it.transformToFree != 0)
	{
		const ItemType& newType = Item::items[it.transformToFree];
		if(newType.type == ITEM_TYPE_BED)
			g_game.transformItem(this, it.transformToFree);
	}
}

void BedItem::internalSetSleeper(const Player* player)
{
	setSleeper(player->getGUID());
	setSleepStart(time(NULL));
	setSpecialDescription(std::string(player->getName() + " is sleeping there."));
}

void BedItem::internalRemoveSleeper()
{
	setSleeper(0);
	setSleepStart(0);
	setSpecialDescription("Nobody is sleeping there.");
}

BedItem* Beds::getBedBySleeper(uint32_t guid)
{
	std::map<uint32_t, BedItem*>::iterator it = BedSleepersMap.find(guid);
	if(it != BedSleepersMap.end())
		return it->second;

	return NULL;
}

void Beds::setBedSleeper(BedItem* bed, uint32_t guid)
{
	BedSleepersMap[guid] = bed;
}
