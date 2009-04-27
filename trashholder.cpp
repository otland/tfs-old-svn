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
#include "trashholder.h"

#include "game.h"
extern Game g_game;

TrashHolder::TrashHolder(uint16_t _type, MagicEffectClasses _effect /*= NM_ME_NONE*/): Item(_type)
{
	effect = _effect;
}

ReturnValue TrashHolder::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	return RET_NOERROR;
}

ReturnValue TrashHolder::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
	uint32_t& maxQueryCount, uint32_t flags) const
{
	maxQueryCount = std::max((uint32_t)1, count);
	return RET_NOERROR;
}

ReturnValue TrashHolder::__queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const
{
	return RET_NOTPOSSIBLE;
}

Cylinder* TrashHolder::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	return this;
}

void TrashHolder::__addThing(Creature* actor, Thing* thing)
{
	return __addThing(actor, 0, thing);
}

void TrashHolder::__addThing(Creature* actor, int32_t index, Thing* thing)
{
	Item* item = thing->getItem();
	if(!item)
		return;

	if(item->getID() == ITEM_WATERBALL)
		return;

	if(item != this && item->hasProperty(MOVEABLE))
	{
		g_game.internalRemoveItem(actor, item);
		if(effect != NM_ME_NONE)
			g_game.addMagicEffect(getPosition(), effect);
	}
}

void TrashHolder::__updateThing(Thing* thing, uint16_t itemId, uint32_t count)
{
	//
}

void TrashHolder::__replaceThing(uint32_t index, Thing* thing)
{
	//
}

void TrashHolder::__removeThing(Thing* thing, uint32_t count)
{
	//
}

void TrashHolder::postAddNotification(Creature* actor, Thing* thing, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postAddNotification(actor, thing, index, LINK_PARENT);
}

void TrashHolder::postRemoveNotification(Creature* actor, Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postRemoveNotification(actor, thing, index, isCompleteRemoval, LINK_PARENT);
}
