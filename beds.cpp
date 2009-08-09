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

Attr_ReadValue BedItem::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch(attr)
	{
		case ATTR_SLEEPERGUID:
		{
			uint32_t _sleeper;
			if(!propStream.GET_ULONG(_sleeper))
				return ATTR_READ_ERROR;

			if(_sleeper)
			{
				std::string name;
				if(IOLoginData::getInstance()->getNameByGuid(_sleeper, name))
				{
					setSpecialDescription(name + " is sleeping there.");
					Beds::getInstance().setBedSleeper(this, _sleeper);
				}
			}

			sleeper = _sleeper;
			return ATTR_READ_CONTINUE;
		}

		case ATTR_SLEEPSTART:
		{
			uint32_t _sleepStart;
			if(!propStream.GET_ULONG(_sleepStart))
				return ATTR_READ_ERROR;

			sleepStart = (time_t)_sleepStart;
			return ATTR_READ_CONTINUE;
		}

		default:
			break;
	}

	return Item::readAttr(attr, propStream);
}

bool BedItem::serializeAttr(PropWriteStream& propWriteStream) const
{
	if(sleeper)
	{
		propWriteStream.ADD_UCHAR(ATTR_SLEEPERGUID);
		propWriteStream.ADD_ULONG(sleeper);
	}

	if(sleepStart)
	{
		propWriteStream.ADD_UCHAR(ATTR_SLEEPSTART);
		propWriteStream.ADD_ULONG((int32_t)sleepStart);
	}

	return true;
}

BedItem* BedItem::getNextBedItem()
{
	if(Tile* tile = g_game.getTile(getNextPosition(Item::items[getID()].bedPartnerDir, getPosition())))
		return tile->getBedItem();

	return NULL;
}

bool BedItem::canUse(Player* player)
{
	if(!house || !player || player->isRemoved() || (!player->isPremium() && g_config.getBool(
		ConfigManager::BED_REQUIRE_PREMIUM)) || player->hasCondition(CONDITION_INFIGHT))
		return false;

	if(!sleeper || house->getHouseAccessLevel(player) == HOUSE_OWNER)
		return isBed();

	Player* _player = g_game.getPlayerByGuidEx(sleeper);
	if(!_player)
		return isBed();

	bool ret = house->getHouseAccessLevel(_player) <= house->getHouseAccessLevel(player);
	if(_player->isVirtual())
		delete _player;

	return ret;
}

void BedItem::sleep(Player* player)
{
	if(!house || !player || player->isRemoved())
		return;

	if(!sleeper)
	{
		Beds::getInstance().setBedSleeper(this, player->getGUID());
		internalSetSleeper(player);

		BedItem* nextBedItem = getNextBedItem();
		if(nextBedItem)
			nextBedItem->internalSetSleeper(player);

		updateAppearance(player);
		if(nextBedItem)
			nextBedItem->updateAppearance(player);

		player->getTile()->moveCreature(NULL, player, getTile());
		g_game.addMagicEffect(player->getPosition(), NM_ME_SLEEP);
		Scheduler::getScheduler().addEvent(createSchedulerTask(SCHEDULER_MINTICKS, boost::bind(&Game::kickPlayer, &g_game, player->getID(), false)));
	}
	else if(Item::items[getID()].transformToFree)
	{
		wakeUp();
		g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
	}
	else
		player->sendCancelMessage(RET_NOTPOSSIBLE);
}

void BedItem::wakeUp()
{
	if(!house)
		return;

	if(sleeper)
	{
		if(Player* player = g_game.getPlayerByGuidEx(sleeper))
		{
			regeneratePlayer(player);
			if(player->isVirtual())
			{
				IOLoginData::getInstance()->savePlayer(player);
				delete player;
			}
			else
				g_game.addCreatureHealth(player);
		}
	}

	Beds::getInstance().setBedSleeper(NULL, sleeper);
	internalRemoveSleeper();

	BedItem* nextBedItem = getNextBedItem();
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
			amount = std::min((condition->getTicks() / 1000), sleptTime) / 30;
			int32_t tmp = condition->getTicks() - (amount * 30000);
			if(tmp <= 0)
				player->removeCondition(condition);
			else
				condition->setTicks(tmp);
		}

		player->changeHealth(amount);
		player->changeMana(amount);
	}

	player->changeSoul((int32_t)std::max((float)0, (float)sleptTime / (60 * 15)));
}

void BedItem::updateAppearance(const Player* player)
{
	const ItemType& it = Item::items[getID()];
	if(it.type != ITEM_TYPE_BED)
		return;

	if(player && it.transformToOnUse[player->getSex(false)])
	{
		const ItemType& newType = Item::items[it.transformToOnUse[player->getSex(false)]];
		if(newType.type == ITEM_TYPE_BED)
			g_game.transformItem(this, it.transformToOnUse[player->getSex(false)]);
	}
	else if(it.transformToFree)
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
