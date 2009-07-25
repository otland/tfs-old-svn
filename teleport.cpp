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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "game.h"
extern Game g_game;

Teleport::Teleport(uint16_t _type):
	Item(_type)
{
	destPos = Position();
}

Attr_ReadValue Teleport::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if(ATTR_TELE_DEST == attr)
	{
		TeleportDest* teleDest;
		if(!propStream.GET_STRUCT(teleDest))
			return ATTR_READ_ERROR;

		setDestPos(Position(teleDest->_x, teleDest->_y, teleDest->_z));
		return ATTR_READ_CONTINUE;
	}

	return Item::readAttr(attr, propStream);
}

bool Teleport::serializeAttr(PropWriteStream& propWriteStream) const
{
	bool ret = Item::serializeAttr(propWriteStream);
	propWriteStream.ADD_UCHAR(ATTR_TELE_DEST);

	TeleportDest tmpDest;
	tmpDest._x = destPos.x;
	tmpDest._y = destPos.y;
	tmpDest._z = destPos.z;

	propWriteStream.ADD_VALUE(tmpDest);
	return ret;
}

void Teleport::__addThing(Creature* actor, int32_t index, Thing* thing)
{
	Tile* destTile = g_game.getTile(getDestPos());
	if(!destTile)
		return;

	if(Creature* creature = thing->getCreature())
	{
		getTile()->moveCreature(actor, creature, destTile);
		g_game.addMagicEffect(destTile->getPosition(), NM_ME_TELEPORT, creature->isGhost());
	}
	else if(Item* item = thing->getItem())
		g_game.internalMoveItem(actor, getTile(), destTile, INDEX_WHEREEVER, item, item->getItemCount(), NULL);
}

void Teleport::postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent,
	int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postAddNotification(actor, thing, oldParent, index, LINK_PARENT);
}

void Teleport::postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent,
	int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postRemoveNotification(actor, thing, newParent,
		index, isCompleteRemoval, LINK_PARENT);
}
