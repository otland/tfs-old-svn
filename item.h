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

#ifndef __ITEM__
#define __ITEM__
#include "otsystem.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "thing.h"
#include "itemattributes.h"

#include "items.h"
#include "raids.h"

class Creature;
class Player;

class Container;
class Depot;

class TrashHolder;
class Mailbox;

class Teleport;
class MagicField;

class Door;
class BedItem;

enum ITEMPROPERTY
{
	BLOCKSOLID = 0,
	HASHEIGHT,
	BLOCKPROJECTILE,
	BLOCKPATH,
	ISVERTICAL,
	ISHORIZONTAL,
	MOVEABLE,
	IMMOVABLEBLOCKSOLID,
	IMMOVABLEBLOCKPATH,
	IMMOVABLENOFIELDBLOCKPATH,
	NOFIELDBLOCKPATH,
	SUPPORTHANGABLE
};

enum TradeEvents_t
{
	ON_TRADE_TRANSFER,
	ON_TRADE_CANCEL,
};

enum ItemDecayState_t
{
	DECAYING_FALSE = 0,
	DECAYING_TRUE,
	DECAYING_PENDING
};

enum AttrTypes_t
{
	ATTR_END = 0,
	//ATTR_DESCRIPTION = 1,
	//ATTR_EXT_FILE = 2,
	ATTR_TILE_FLAGS = 3,
	ATTR_ACTION_ID = 4,
	ATTR_UNIQUE_ID = 5,
	ATTR_TEXT = 6,
	ATTR_DESC = 7,
	ATTR_TELE_DEST = 8,
	ATTR_ITEM = 9,
	ATTR_DEPOT_ID = 10,
	//ATTR_EXT_SPAWN_FILE = 11,
	ATTR_RUNE_CHARGES = 12,
	//ATTR_EXT_HOUSE_FILE = 13,
	ATTR_HOUSEDOORID = 14,
	ATTR_COUNT = 15,
	ATTR_DURATION = 16,
	ATTR_DECAYING_STATE = 17,
	ATTR_WRITTENDATE = 18,
	ATTR_WRITTENBY = 19,
	ATTR_SLEEPERGUID = 20,
	ATTR_SLEEPSTART = 21,
	ATTR_CHARGES = 22,
	ATTR_CONTAINER_ITEMS = 23,
	ATTR_NAME = 30,
	ATTR_PLURALNAME = 31,
	ATTR_ATTACK = 33,
	ATTR_EXTRAATTACK = 34,
	ATTR_DEFENSE = 35,
	ATTR_EXTRADEFENSE = 36,
	ATTR_ARMOR = 37,
	ATTR_ATTACKSPEED = 38,
	ATTR_HITCHANCE = 39,
	ATTR_SHOOTRANGE = 40,
	ATTR_ARTICLE = 41,
	ATTR_SCRIPTPROTECTED = 42,
	ATTR_ATTRIBUTE_MAP = 128
};

enum Attr_ReadValue
{
	ATTR_READ_CONTINUE,
	ATTR_READ_ERROR,
	ATTR_READ_END
};

// from iomap.h
#pragma pack(1)
struct TeleportDest
{
	uint16_t _x, _y;
	uint8_t _z;
};
#pragma pack()

typedef std::list<Item*> ItemList;

class Item : virtual public Thing, public ItemAttributes
{
	public:
		static Items items;

		//Factory member to create item of right type based on type
		static Item* CreateItem(const uint16_t type, uint16_t amount = 1);
		static Item* CreateItem(PropStream& propStream);

		static bool loadItem(xmlNodePtr node, Container* parent);
		static bool loadContainer(xmlNodePtr node, Container* parent);

		// Constructor for items
		Item(const uint16_t type, uint16_t amount = 0);
		Item(const Item &i): Thing(), ItemAttributes(i), id(i.id), count(i.count) {}
		virtual ~Item() {}

		virtual Item* clone() const;
		virtual void copyAttributes(Item* item);

		virtual Item* getItem() {return this;}
		virtual const Item* getItem() const {return this;}

		virtual Container* getContainer() {return NULL;}
		virtual const Container* getContainer() const {return NULL;}

		virtual Teleport* getTeleport() {return NULL;}
		virtual const Teleport* getTeleport() const {return NULL;}

		virtual TrashHolder* getTrashHolder() {return NULL;}
		virtual const TrashHolder* getTrashHolder() const {return NULL;}

		virtual Mailbox* getMailbox() {return NULL;}
		virtual const Mailbox* getMailbox() const {return NULL;}

		virtual Door* getDoor() {return NULL;}
		virtual const Door* getDoor() const {return NULL;}

		virtual MagicField* getMagicField() {return NULL;}
		virtual const MagicField* getMagicField() const {return NULL;}

		virtual BedItem* getBed() {return NULL;}
		virtual const BedItem* getBed() const {return NULL;}

		uint16_t getID() const {return id;}
		void setID(uint16_t newid);
		uint16_t getClientID() const {return items[id].clientId;}

		static std::string getDescription(const ItemType& it, int32_t lookDistance, const Item* item = NULL, int32_t subType = -1, bool addArticle = true);
		static std::string getNameDescription(const ItemType& it, const Item* item = NULL, int32_t subType = -1, bool addArticle = true);
		static std::string getWeightDescription(double weight, bool stackable, uint32_t count = 1);

		virtual std::string getDescription(int32_t lookDistance) const {return getDescription(items[id], lookDistance, this);}
		std::string getNameDescription() const {return getNameDescription(items[id], this);}
		std::string getWeightDescription() const {return getWeightDescription(getWeight(), items[id].stackable, count);}

		Player* getHoldingPlayer();
		const Player* getHoldingPlayer() const;

		//serialization
		virtual Attr_ReadValue readAttr(AttrTypes_t attr, PropStream& propStream);
		virtual bool unserializeAttr(PropStream& propStream);
		virtual bool serializeAttr(PropWriteStream& propWriteStream) const;
		virtual bool unserializeItemNode(FileLoader& f, NODE node, PropStream& propStream) {return unserializeAttr(propStream);}

		// Item attributes
		void setDuration(int32_t time) {setAttribute("duration", time);}
		void decreaseDuration(int32_t time);
		int32_t getDuration() const;

		void setSpecialDescription(const std::string& description) {setAttribute("description", description);}
		void resetSpecialDescription() {eraseAttribute("description");}
		std::string getSpecialDescription() const;

		void setText(const std::string& text) {setAttribute("text", text);}
		void resetText() {eraseAttribute("text");}
		std::string getText() const;

		void setDate(time_t date) {setAttribute("date", (int32_t)date);}
		void resetDate() {eraseAttribute("date");}
		time_t getDate() const;

		void setWriter(std::string writer) {setAttribute("writer", writer);}
		void resetWriter() {eraseAttribute("writer");}
		std::string getWriter() const;

		void setActionId(int32_t aid);
		void resetActionId();
		int32_t getActionId() const;

		void setUniqueId(int32_t uid);
		int32_t getUniqueId() const;

		void setCharges(uint16_t charges) {setAttribute("charges", charges);}
		uint16_t getCharges() const;

		void setFluidType(uint16_t fluidType) {setAttribute("fluidtype", fluidType);}
		uint16_t getFluidType() const;

		void setOwner(uint32_t owner) {setAttribute("owner", (int32_t)owner);}
		uint32_t getOwner() const;

		void setCorpseOwner(uint32_t corpseOwner) {setAttribute("corpseowner", (int32_t)corpseOwner);}
		uint32_t getCorpseOwner();

		void setDecaying(ItemDecayState_t state) {setAttribute("decaying", (int32_t)state);}
		ItemDecayState_t getDecaying() const;

		std::string getName() const;
		std::string getPluralName() const;
		std::string getArticle() const;
		bool isScriptProtected() const;

		int32_t getAttack() const;
		int32_t getExtraAttack() const;
		int32_t getDefense() const;
		int32_t getExtraDefense() const;

		int32_t getArmor() const;
		int32_t getAttackSpeed() const;
		int32_t getHitChance() const;
		int32_t getShootRange() const;

		Ammo_t getAmmoType() const {return items[id].ammoType;}
		WeaponType_t getWeaponType() const {return items[id].weaponType;}
		int32_t getSlotPosition() const {return items[id].slotPosition;}
		int32_t getWieldPosition() const {return items[id].wieldPosition;}

		virtual double getWeight() const;
		void getLight(LightInfo& lightInfo);

		int32_t getMaxWriteLength() const {return items[id].maxTextLen;}
		int32_t getWorth() const {return getItemCount() * items[id].worth;}
		virtual int32_t getThrowRange() const {return (isPickupable() ? 15 : 2);}

		bool floorChange(FloorChange_t change = CHANGE_NONE) const;
		bool forceSerialize() const {return items[id].forceSerialize || canWriteText() || isContainer() || isBed() || isDoor();}

		bool hasProperty(enum ITEMPROPERTY prop) const;
		bool hasSubType() const {return items[id].hasSubType();}
		bool hasCharges() const {return items[id].charges;}

		bool canDecay();
		virtual bool canRemove() const {return true;}
		virtual bool canTransform() const {return true;}
		bool canWriteText() const {return items[id].canWriteText;}

		virtual bool isPushable() const {return !isNotMoveable();}
		bool isGroundTile() const {return items[id].isGroundTile();}
		bool isContainer() const {return items[id].isContainer();}
		bool isSplash() const {return items[id].isSplash();}
		bool isFluidContainer() const {return (items[id].isFluidContainer());}
		bool isDoor() const {return items[id].isDoor();}
		bool isMagicField() const {return items[id].isMagicField();}
		bool isTeleport() const {return items[id].isTeleport();}
		bool isKey() const {return items[id].isKey();}
		bool isDepot() const {return items[id].isDepot();}
		bool isMailbox() const {return items[id].isMailbox();}
		bool isTrashHolder() const {return items[id].isTrashHolder();}
		bool isBed() const {return items[id].isBed();}
		bool isRune() const {return items[id].isRune();}
		bool isBlocking(const Creature* creature) const {return items[id].blockSolid;}
		bool isStackable() const {return items[id].stackable;}
		bool isAlwaysOnTop() const {return items[id].alwaysOnTop;}
		bool isNotMoveable() const {return !items[id].moveable;}
		bool isMoveable() const {return items[id].moveable;}
		bool isPickupable() const {return items[id].pickupable;}
		bool isUseable() const {return items[id].useable;}
		bool isHangable() const {return items[id].isHangable;}
		bool isRoteable() const {const ItemType& it = items[id]; return it.rotable && it.rotateTo;}
		bool isWeapon() const {return (items[id].weaponType != WEAPON_NONE);}
		bool isReadable() const {return items[id].canReadText;}

		bool isLoadedFromMap() const {return loadedFromMap;}
		void setLoadedFromMap(bool value) {loadedFromMap = value;}

		uint16_t getItemCount() const {return count;}
		void setItemCount(uint16_t n) {count = n;}

		uint16_t getSubType() const;
		void setSubType(uint16_t n);

		uint32_t getDefaultDuration() const {return items[id].decayTime * 1000;}
		void setDefaultDuration()
		{
			uint32_t duration = getDefaultDuration();
			if(duration)
				setDuration(duration);
		}

		Raid* getRaid() {return raid;}
		void setRaid(Raid* _raid) {raid = _raid;}

		virtual void onRemoved();
		virtual bool onTradeEvent(TradeEvents_t event, Player* owner, Player* seller) {return true;}

		void setDefaultSubtype();
		virtual void __startDecaying();
		static uint32_t countByType(const Item* item, int32_t checkType, bool multiCount);

	protected:
		uint16_t id;
		uint8_t count;

		Raid* raid;
		bool loadedFromMap;
};

inline std::string Item::getName() const
{
	const std::string* v = getStringAttribute("name");
	if(v)
		return *v;

	return items[id].name;
}

inline std::string Item::getPluralName() const
{
	const std::string* v = getStringAttribute("pluralname");
	if(v)
		return *v;

	return items[id].pluralName;
}

inline std::string Item::getArticle() const
{
	const std::string* v = getStringAttribute("article");
	if(v)
		return *v;

	return items[id].article;
}

inline bool Item::isScriptProtected() const
{
	const bool* v = getBooleanAttribute("scriptprotected");
	if(v)
		return *v;

	return false;
}

inline int32_t Item::getAttack() const
{
	const int32_t* v = getIntegerAttribute("attack");
	if(v)
		return *v;

	return items[id].attack;
}

inline int32_t Item::getExtraAttack() const
{
	const int32_t* v = getIntegerAttribute("extraattack");
	if(v)
		return *v;

	return items[id].extraAttack;
}

inline int32_t Item::getDefense() const
{
	const int32_t* v = getIntegerAttribute("defense");
	if(v)
		return *v;

	return items[id].defense;
}

inline int32_t Item::getExtraDefense() const
{
	const int32_t* v = getIntegerAttribute("extradefense");
	if(v)
		return *v;

	return items[id].extraDefense;
}

inline int32_t Item::getArmor() const
{
	const int32_t* v = getIntegerAttribute("armor");
	if(v)
		return *v;

	return items[id].armor;
}

inline int32_t Item::getAttackSpeed() const
{
	const int32_t* v = getIntegerAttribute("attackspeed");
	if(v)
		return *v;

	return items[id].attackSpeed;
}

inline int32_t Item::getHitChance() const
{
	const int32_t* v = getIntegerAttribute("hitchance");
	if(v)
		return *v;

	return items[id].hitChance;
}

inline int32_t Item::getShootRange() const
{
	const int32_t* v = getIntegerAttribute("shootrange");
	if(v)
		return *v;

	return items[id].shootRange;
}

inline void Item::decreaseDuration(int32_t time)
{
	const int32_t* v = getIntegerAttribute("duration");
	if(v)
		setAttribute("duration", *v - time);
}

inline int32_t Item::getDuration() const
{
	const int32_t* v = getIntegerAttribute("duration");
	if(v)
		return *v;

	return 0;
}

inline std::string Item::getSpecialDescription() const
{
	const std::string* v = getStringAttribute("description");
	if(v)
		return *v;

	return "";
}

inline std::string Item::getText() const
{
	const std::string* v = getStringAttribute("text");
	if(v)
		return *v;

	return "";
}

inline time_t Item::getDate() const
{
	const int32_t* v = getIntegerAttribute("date");
	if(v)
		return (time_t)*v;

	return 0;
}

inline std::string Item::getWriter() const
{
	const std::string* v = getStringAttribute("writer");
	if(v)
		return *v;

	return "";
}

inline int32_t Item::getActionId() const
{
	const int32_t* v = getIntegerAttribute("aid");
	if(v)
		return *v;

	return 0;
}

inline int32_t Item::getUniqueId() const
{
	const int32_t* v = getIntegerAttribute("uid");
	if(v)
		return *v;

	return 0;
}

inline uint16_t Item::getCharges() const
{
	const int32_t* v = getIntegerAttribute("charges");
	if(v && *v >= 0)
		return (uint16_t)*v;

	return 0;
}

inline uint16_t Item::getFluidType() const
{
	const int32_t* v = getIntegerAttribute("fluidtype");
	if(v && *v >= 0)
		return (uint16_t)*v;

	return 0;
}

inline uint32_t Item::getOwner() const
{
	const int32_t* v = getIntegerAttribute("owner");
	if(v)
		return (uint32_t)*v;

	return 0;
}

inline uint32_t Item::getCorpseOwner()
{
	const int32_t* v = getIntegerAttribute("corpseowner");
	if(v)
		return (uint32_t)*v;

	return 0;
}

inline ItemDecayState_t Item::getDecaying() const
{
	const int32_t* v = getIntegerAttribute("decaying");
	if(v)
		return (ItemDecayState_t)*v;

	return DECAYING_FALSE;
}

inline uint32_t Item::countByType(const Item* item, int32_t checkType, bool multiCount)
{
	if(checkType != -1 && checkType != (int32_t)item->getSubType())
		return 0;

	if(multiCount)
		return item->getItemCount();

	if(item->isRune())
		return item->getCharges();

	return item->getItemCount();
}
#endif
