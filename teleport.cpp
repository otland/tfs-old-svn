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
#include "teleport.h"

#include "game.h"

extern Game g_game;

Attr_ReadValue Teleport::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if(attr != ATTR_TELE_DEST)
		return Item::readAttr(attr, propStream);

	TeleportDest* dest;
	if(!propStream.GET_STRUCT(dest))
		return ATTR_READ_ERROR;

	setDestination(Position(dest->_x, dest->_y, dest->_z));
	return ATTR_READ_CONTINUE;
}

bool Teleport::serializeAttr(PropWriteStream& propWriteStream) const
{
	bool ret = Item::serializeAttr(propWriteStream);
	propWriteStream.ADD_UCHAR(ATTR_TELE_DEST);

	TeleportDest dest;
	dest._x = destination.x;
	dest._y = destination.y;
	dest._z = destination.z;

	propWriteStream.ADD_VALUE(dest);
	return ret;
}

void Teleport::__addThing(Creature* actor, int32_t index, Thing* thing)
{
	Tile* destTile = g_game.getTile(destination);
	if(!destTile)
		return;

	if(Creature* creature = thing->getCreature())
	{
		creature->getTile()->moveCreature(actor, creature, destTile);
		g_game.addMagicEffect(destTile->getPosition(), MAGIC_EFFECT_TELEPORT, creature->isGhost());
	}
	else if(Item* item = thing->getItem())
		g_game.internalMoveItem(actor, getTile(), destTile, INDEX_WHEREEVER, item, item->getItemCount(), NULL);
}

void Teleport::postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent,
	int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(getParent())
		getParent()->postAddNotification(actor, thing, oldParent, index, LINK_PARENT);
}

void Teleport::postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent,
	int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(getParent())
		getParent()->postRemoveNotification(actor, thing, newParent,
			index, isCompleteRemoval, LINK_PARENT);
}
