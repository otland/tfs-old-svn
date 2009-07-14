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

#ifndef __TRASHHOLDER__
#define __TRASHHOLDER__
#include "const.h"

#include "tile.h"
class TrashHolder : public Item, public Cylinder
{
	public:
		TrashHolder(uint16_t _type, MagicEffectClasses _effect = NM_ME_NONE);
		virtual ~TrashHolder() {}

		virtual TrashHolder* getTrashHolder() {return this;}
		virtual const TrashHolder* getTrashHolder() const {return this;}

		//cylinder implementations
		virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
			uint32_t flags) const {return RET_NOERROR;}
		virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
			uint32_t& maxQueryCount, uint32_t flags) const {return RET_NOERROR;}
		virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count,
			uint32_t flags) const {return RET_NOTPOSSIBLE;}
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

		MagicEffectClasses getEffect() const {return effect;}

	private:
		MagicEffectClasses effect;
};
#endif
