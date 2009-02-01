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

#include "teleport.h"
#include "game.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern Game g_game;

Teleport::Teleport(uint16_t _type):
Item(_type)
{
	destPos = Position();
}

Teleport::~Teleport()
{
	//
}

bool Teleport::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if(ATTR_TELE_DEST == attr)
	{
		TeleportDest* teleDest;
		if(!propStream.GET_STRUCT(teleDest))
			return false;

		setDestPos(Position(teleDest->_x, teleDest->_y, teleDest->_z));
		return true;
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

ReturnValue Teleport::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	return RET_NOTPOSSIBLE;
}

ReturnValue Teleport::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
	uint32_t& maxQueryCount, uint32_t flags) const
{
	return RET_NOTPOSSIBLE;
}

ReturnValue Teleport::__queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const
{
	return RET_NOERROR;
}

Cylinder* Teleport::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	return this;
}

void Teleport::__addThing(Creature* actor, Thing* thing)
{
	return __addThing(actor, 0, thing);
}

void Teleport::__addThing(Creature* actor, int32_t index, Thing* thing)
{
	if(Tile* destTile = g_game.getTile(getDestPos().x, getDestPos().y, getDestPos().z))
	{
		if(Creature* creature = thing->getCreature())
		{
			getTile()->moveCreature(creature, destTile, true);
			g_game.addMagicEffect(destTile->getPosition(), NM_ME_TELEPORT, creature->isInGhostMode());
		}
		else if(Item* item = thing->getItem())
			g_game.internalMoveItem(actor, getTile(), destTile, INDEX_WHEREEVER, item, item->getItemCount(), NULL);
	}
}

void Teleport::__updateThing(Thing* thing, uint16_t itemId, uint32_t count)
{
	//
}

void Teleport::__replaceThing(uint32_t index, Thing* thing)
{
	//
}

void Teleport::__removeThing(Thing* thing, uint32_t count)
{
	//
}

void Teleport::postAddNotification(Creature* actor, Thing* thing, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postAddNotification(actor, thing, index, LINK_PARENT);
}

void Teleport::postRemoveNotification(Creature* actor, Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postRemoveNotification(actor, thing, index, isCompleteRemoval, LINK_PARENT);
}
