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
#include "mailbox.h"

#include "configmanager.h"

#include "player.h"
#include "iologindata.h"
#include "depot.h"
#include "town.h"

#include "game.h"
extern Game g_game;
extern ConfigManager g_config;

ReturnValue Mailbox::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	if(const Item* item = thing->getItem())
	{
		if(canSend(item))
			return RET_NOERROR;
	}

	return RET_NOTPOSSIBLE;
}

ReturnValue Mailbox::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
	uint32_t flags) const
{
	maxQueryCount = std::max((uint32_t)1, count);
	return RET_NOERROR;
}

ReturnValue Mailbox::__queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const
{
	return RET_NOTPOSSIBLE;
}

Cylinder* Mailbox::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	return this;
}

void Mailbox::__addThing(Creature* actor, Thing* thing)
{
	return __addThing(actor, 0, thing);
}

void Mailbox::__addThing(Creature* actor, int32_t index, Thing* thing)
{
	if(Item* item = thing->getItem())
	{
		if(canSend(item))
			sendItem(actor, item);
	}
}

void Mailbox::postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent,
	int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postAddNotification(actor, thing, oldParent, index, LINK_PARENT);
}

void Mailbox::postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent,
	int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postRemoveNotification(actor, thing, newParent,
		index, isCompleteRemoval, LINK_PARENT);
}

bool Mailbox::sendItem(Creature* actor, Item* item)
{
	uint32_t dp = 0;
	std::string receiver;
	if(!getReceiver(item, receiver, dp))
		return false;

	if(receiver.empty() || !dp)
		return false;

	Player* player = g_game.getPlayerByName(receiver);
	if(!player)
	{
		player = new Player(receiver, NULL);
		if(!IOLoginData::getInstance()->loadPlayer(player, receiver))
		{
			delete player;
			player = NULL;
		}
	}

	if(!player)
		return false;

	Depot* depot = player->getDepot(dp, true);
	if(!depot || g_game.internalMoveItem(actor, item->getParent(), depot, INDEX_WHEREEVER,
		item, item->getItemCount(), NULL, FLAG_NOLIMIT) != RET_NOERROR)
	{
		if(player->isVirtual())
			delete player;

		return false;
	}

	g_game.transformItem(item, item->getID() + 1);
	bool result = true, opened = player->getContainerID(depot) != -1;
	if(Player* tmp = actor->getPlayer())
	{
		CreatureEventList mailEvents = tmp->getCreatureEvents(CREATURE_EVENT_MAIL_SEND);
		for(CreatureEventList::iterator it = mailEvents.begin(); it != mailEvents.end(); ++it)
		{
			if(!(*it)->executeMailSend(tmp, player, item, opened) && result)
				result = false;
		}

		mailEvents = player->getCreatureEvents(CREATURE_EVENT_MAIL_RECEIVE);
		for(CreatureEventList::iterator it = mailEvents.begin(); it != mailEvents.end(); ++it)
		{
			if(!(*it)->executeMailReceive(player, tmp, item, opened) && result)
				result = false;
		}
	}

	if(player->isVirtual())
	{
		IOLoginData::getInstance()->savePlayer(player);
		delete player;
	}

	return result;
}

bool Mailbox::getReceiver(Item* item, std::string& name, uint32_t& dp)
{
	if(!item)
		return false;

	if(item->getID() == ITEM_PARCEL) /**We need to get the text from the label incase its a parcel**/
	{
		if(Container* parcel = item->getContainer())
		{
			for(ItemList::const_iterator cit = parcel->getItems(); cit != parcel->getEnd(); ++cit)
			{
				if((*cit)->getID() == ITEM_LABEL && !(*cit)->getText().empty())
				{
					item = (*cit);
					break;
				}
			}
		}
	}
	else if(item->getID() != ITEM_LETTER) /**The item is somehow not a parcel or letter**/
	{
		std::cout << "[Error - Mailbox::getReciver] Trying to get receiver from unkown item with id: " << item->getID() << "!" << std::endl;
		return false;
	}

	if(!item || item->getText().empty()) /**No label/letter found or its empty.**/
		return false;

	std::istringstream iss(item->getText(), std::istringstream::in);
	uint32_t curLine = 0;

	std::string tmp, strTown;
	while(getline(iss, tmp, '\n') && curLine < 2)
	{
		if(curLine == 0)
			name = tmp;
		else if(curLine == 1)
			strTown = tmp;

		++curLine;
	}

	Town* town = Towns::getInstance().getTown(strTown);
	if(!town)
		return false;

	IntegerVec disabledTowns = vectorAtoi(explodeString(g_config.getString(ConfigManager::MAILBOX_DISABLED_TOWNS), ","));
	if(disabledTowns[0] != -1)
	{
		for(IntegerVec::iterator it = disabledTowns.begin(); it != disabledTowns.end(); ++it)
		{
			if(town->getTownID() == (uint32_t)(*it))
				return false;
		}
	}

	dp = town->getTownID();
	return true;
}
