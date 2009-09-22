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

#ifndef __TELEPORT__
#define __TELEPORT__

#include "tile.h"

class Teleport : public Item, public Cylinder
{
	public:
		Teleport(uint16_t type): Item(type) {}
		virtual ~Teleport() {}

		virtual Teleport* getTeleport() {return this;}
		virtual const Teleport* getTeleport() const {return this;}

		//serialization
		virtual Attr_ReadValue readAttr(AttrTypes_t attr, PropStream& propStream);
		virtual bool serializeAttr(PropWriteStream& propWriteStream) const;

		void setDestination(const Position& pos) {destination = pos;}
		Position getDestination() const {return destination;}

		//cylinder implementations
		virtual Cylinder* getParent() {return Item::getParent();}
		virtual const Cylinder* getParent() const {return Item::getParent();}
		virtual bool isRemoved() const {return Item::isRemoved();}
		virtual Position getPosition() const {return Item::getPosition();}
		virtual Tile* getTile() {return Item::getTile();}
		virtual const Tile* getTile() const {return Item::getTile();}
		virtual Item* getItem() {return this;}
		virtual const Item* getItem() const {return this;}
		virtual Creature* getCreature() {return NULL;}
		virtual const Creature* getCreature() const {return NULL;}

		virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
			uint32_t flags) const {return RET_NOTPOSSIBLE;}
		virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
			uint32_t& maxQueryCount, uint32_t flags) const {return RET_NOTPOSSIBLE;}
		virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count,
			uint32_t flags) const {return RET_NOERROR;}
		virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
			uint32_t& flags) {return this;}

		virtual void __addThing(Creature* actor, Thing* thing) {return __addThing(actor, 0, thing);}
		virtual void __addThing(Creature* actor, int32_t index, Thing* thing);

		virtual void __updateThing(Thing* thing, uint16_t itemId, uint32_t count) {}
		virtual void __replaceThing(uint32_t index, Thing* thing) {}

		virtual void __removeThing(Thing* thing, uint32_t count) {}

		virtual void postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent,
			int32_t index, cylinderlink_t link = LINK_OWNER);
		virtual void postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent,
			int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

	private:
		Position destination;
};
#endif
