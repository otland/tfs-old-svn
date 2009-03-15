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

#include "mailbox.h"
#include "game.h"
#include "player.h"
#include "iologindata.h"
#include "depot.h"
#include "town.h"

extern Game g_game;

Mailbox::Mailbox(uint16_t _type) : Item(_type)
{
	//
}

Mailbox::~Mailbox()
{
	//
}

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

void Mailbox::__updateThing(Thing* thing, uint16_t itemId, uint32_t count)
{
	//
}

void Mailbox::__replaceThing(uint32_t index, Thing* thing)
{
	//
}

void Mailbox::__removeThing(Thing* thing, uint32_t count)
{
	//
}

void Mailbox::postAddNotification(Creature* actor, Thing* thing, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postAddNotification(actor, thing, index, LINK_PARENT);
}

void Mailbox::postRemoveNotification(Creature* actor, Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postRemoveNotification(actor, thing, index, isCompleteRemoval, LINK_PARENT);
}

bool Mailbox::sendItem(Creature* actor, Item* item)
{
	std::string receiver = "";
	uint32_t dp = 0;

	if(!getReceiver(item, receiver, dp))
		return false;

	/**No need to continue if its still empty**/
	if(receiver == "" || dp == 0)
		return false;

	bool tmp = IOLoginData::getInstance()->playerExists(receiver);
	if(Player* player = g_game.getPlayerByName(receiver))
	{
		Depot* depot = player->getDepot(dp, true);
		if(depot && g_game.internalMoveItem(actor, item->getParent(), depot, INDEX_WHEREEVER,
			item, item->getItemCount(), NULL, FLAG_NOLIMIT) == RET_NOERROR)
		{
			g_game.transformItem(item, item->getID() + 1);
			bool result = true, opened = (player->getContainerID(depot) != -1);
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

			return result;
		}
	}
	else if(tmp)
	{
		Player* player = new Player(receiver, NULL);
		if(IOLoginData::getInstance()->loadPlayer(player, receiver))
		{
			Depot* depot = player->getDepot(dp, true);
			if(depot && g_game.internalMoveItem(actor, item->getParent(), depot, INDEX_WHEREEVER,
				item, item->getItemCount(), NULL, FLAG_NOLIMIT) == RET_NOERROR)
			{
				g_game.transformItem(item, item->getID() + 1);
				bool result = true;
				if(Player* tmp = actor->getPlayer())
				{
					CreatureEventList mailEvents = tmp->getCreatureEvents(CREATURE_EVENT_MAIL_SEND);
					for(CreatureEventList::iterator it = mailEvents.begin(); it != mailEvents.end(); ++it)
					{
						if(!(*it)->executeMailSend(tmp, player, item, false) && result)
							result = false;
					}

					mailEvents = player->getCreatureEvents(CREATURE_EVENT_MAIL_RECEIVE);
					for(CreatureEventList::iterator it = mailEvents.begin(); it != mailEvents.end(); ++it)
					{
						if(!(*it)->executeMailReceive(player, tmp, item, false) && result)
							result = false;
					}
				}

				if(IOLoginData::getInstance()->savePlayer(player))
				{
					delete player;
					return result;
				}
			}
		}

		delete player;
	}

	return false;
}

bool Mailbox::getReceiver(Item* item, std::string& name, uint32_t& dp)
{
	if(!item)
		return false;

	if(item->getID() == ITEM_PARCEL) /**We need to get the text from the label incase its a parcel**/
	{
		Container* parcel = item->getContainer();
		for(ItemList::const_iterator cit = parcel->getItems(); cit != parcel->getEnd(); cit++)
		{
			if((*cit)->getID() == ITEM_LABEL)
			{
				item = (*cit);
				if(item->getText() != "")
					break;
			}
		}
	}
	else if(item->getID() != ITEM_LETTER) /**The item is somehow not a parcel or letter**/
	{
		std::cout << "[Error - Mailbox::getReciver] Trying to get receiver from unkown item with id: " << item->getID() << "!" << std::endl;
		return false;
	}

	if(!item || item->getText() == "") /**No label/letter found or its empty.**/
		return false;

	std::string tmp, strTown = "";
	std::istringstream iss(item->getText(), std::istringstream::in);

	uint32_t curLine = 1;
	while(getline(iss, tmp, '\n'))
	{
		if(curLine == 1)
			name = tmp;
		else if(curLine == 2)
			strTown = tmp;
		else
			break;

		++curLine;
	}

	Town* town = Towns::getInstance().getTown(strTown);
	if(!town)
		return false;

	dp = town->getTownID();
	return true;
}
