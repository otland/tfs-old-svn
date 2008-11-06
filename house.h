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

#ifndef __OTSERV_HOUSE_H__
#define __OTSERV_HOUSE_H__

#include <string>
#include <list>
#include <map>

#include <boost/regex.hpp>

#include "definitions.h"
#include "position.h"
#include "housetile.h"
#include "player.h"

class House;
class BedItem;

enum AccessList_t
{
	GUEST_LIST = 0x100,
	SUBOWNER_LIST = 0x101
};

enum AccessHouseLevel_t
{
	HOUSE_NO_INVITED = 0,
	HOUSE_GUEST = 1,
	HOUSE_SUBOWNER = 2,
	HOUSE_OWNER = 3
};

enum RentPeriod_t
{
	RENTPERIOD_DAILY,
	RENTPERIOD_WEEKLY,
	RENTPERIOD_MONTHLY,
	RENTPERIOD_YEARLY,
	RENTPERIOD_NEVER
};

typedef std::list<HouseTile*> HouseTileList;
typedef std::list<Door*> HouseDoorList;
typedef std::list<BedItem*> HouseBedItemList;
typedef std::map<uint32_t, House*> HouseMap;

class AccessList
{
	public:
		AccessList();
		virtual ~AccessList();

		bool parseList(const std::string& _list);
		bool addPlayer(std::string& name);
		bool addGuild(const std::string& guildName, const std::string& rankName);
		bool addExpression(const std::string& expression);

		bool isInList(const Player* player);

		void getList(std::string& _list) const;

	private:
		typedef OTSERV_HASH_SET<uint32_t> PlayerList;
		typedef std::list<std::pair<uint32_t, int32_t> > GuildList;
		typedef std::list<std::string> ExpressionList;
		typedef std::list<std::pair<boost::regex, bool> > RegExList;

		std::string list;
		PlayerList playerList;
		GuildList guildList;
		ExpressionList expressionList;
		RegExList regExList;
};

class Door : public Item
{
	public:
		Door(uint16_t _type);
		virtual ~Door();

		virtual Door* getDoor() {return this;}
		virtual const Door* getDoor() const {return this;}

		House* getHouse() {return house;}

		//serialization
		virtual bool unserialize(xmlNodePtr p);
		virtual xmlNodePtr serialize();

		virtual bool readAttr(AttrTypes_t attr, PropStream& propStream);
		virtual bool serializeAttr(PropWriteStream& propWriteStream);

		void setDoorId(uint32_t _doorId){ setIntAttr(ATTR_ITEM_DOORID, (uint32_t)_doorId); }
		uint32_t getDoorId() const{ return getIntAttr(ATTR_ITEM_DOORID); }

		bool canUse(const Player* player);

		void setAccessList(const std::string& textlist);
		bool getAccessList(std::string& list) const;

		//overrides
		virtual void onRemoved();
		void copyAttributes(Item* item);

	protected:
		void setHouse(House* _house);

	private:
		House* house;
		AccessList* accessList;
		friend class House;
};

class HouseTransferItem : public Item
{
	public:
		static HouseTransferItem* createHouseTransferItem(House* house);

		HouseTransferItem(House* _house) : Item(0) {house = _house;}
		virtual ~HouseTransferItem(){}

		virtual bool onTradeEvent(TradeEvents_t event, Player* owner);

		House* getHouse() {return house;}
		virtual bool canTransform() const {return false;}

	protected:
		House* house;
};

class House
{
	public:
		House(uint32_t _houseid);
		virtual ~House();
		uint32_t getHouseId() const {return houseid;}

		void setEntryPos(const Position& pos) {posEntry = pos;}
		const Position& getEntryPosition() const {return posEntry;}

		void setName(const std::string& _houseName) {houseName = _houseName;}
		const std::string& getName() const {return houseName;}

		void setHouseOwner(uint32_t guid, bool cleaned = true);
		uint32_t getHouseOwner() const {return houseOwner;}

		void setPaidUntil(uint32_t paid) {paidUntil = paid;}
		uint32_t getPaidUntil() const {return paidUntil;}

		void setRent(uint32_t _rent) {rent = _rent;}
		uint32_t getRent() const {return rent;}

		void setPrice(uint32_t _price, bool update = false);
		uint32_t getPrice() const {return price;}

		void setLastWarning(uint32_t _lastWarning) {lastWarning = _lastWarning;}
		uint32_t getLastWarning() {return lastWarning;}

		void setPayRentWarnings(uint32_t warnings) {rentWarnings = warnings;}
		uint32_t getPayRentWarnings() const {return rentWarnings;}

		void setTownId(uint32_t _town) {townid = _town;}
		uint32_t getTownId() const {return townid;}

		void setSize(uint32_t _size) {size = _size;}
		uint32_t getSize() const {return size;}

		bool canEditAccessList(uint32_t listId, const Player* player);
		void setAccessList(uint32_t listId, const std::string& textlist, bool teleport = true);
		bool getAccessList(uint32_t listId, std::string& list) const;

		Door* getDoorByNumber(uint32_t doorId);
		Door* getDoorByNumber(uint32_t doorId) const;
		Door* getDoorByPosition(const Position& pos);

		HouseTransferItem* getTransferItem();
		void resetTransferItem();
		bool executeTransfer(HouseTransferItem* item, Player* player);

		bool kickPlayer(Player* player, Player* target);
		bool isInvited(const Player* player);
		AccessHouseLevel_t getHouseAccessLevel(const Player* player);
		uint32_t getHouseTileSize() {return houseTiles.size();}
		void updateDoorDescription(std::string name = "");
		void clean();

		void addDoor(Door* door);
		void removeDoor(Door* door);
		HouseDoorList::iterator getHouseDoorBegin() {return doorList.begin();}
		HouseDoorList::iterator getHouseDoorEnd() {return doorList.end();}

		void addBed(BedItem* bed);
		HouseBedItemList::iterator getHouseBedsBegin() {return bedsList.begin();}
		HouseBedItemList::iterator getHouseBedsEnd() {return bedsList.end();}

		void addTile(HouseTile* tile);
		HouseTileList::iterator getHouseTileBegin() {return houseTiles.begin();}
		HouseTileList::iterator getHouseTileEnd() {return houseTiles.end();}

	private:
		bool transferToDepot();
		void removePlayer(Player* player, bool ignoreRights);
		void removePlayers(bool ignoreInvites);

		bool isLoaded;
		std::string houseName;
		uint32_t houseid, houseOwner, paidUntil, rentWarnings, lastWarning, rent, price, townid, size;
		AccessList guestList, subOwnerList;
		Position posEntry;
		HouseTileList houseTiles;
		HouseDoorList doorList;
		HouseBedItemList bedsList;

		HouseTransferItem* transferItem;
		Container transfer_container;
};

class Houses
{
	public:
		Houses();
		virtual ~Houses();
		static Houses& getInstance()
		{
			static Houses instance;
			return instance;
		}

		bool loadHousesXML(std::string filename);
		bool reloadPrices();
		bool payHouses();

		House* getHouse(uint32_t houseid, bool add = false);
		House* getHouseByPlayer(Player* player);
		House* getHouseByPlayerId(uint32_t playerId);

		uint32_t getHousesCount(uint32_t accId) const;

		HouseMap::iterator getHouseBegin() {return houseMap.begin();}
		HouseMap::iterator getHouseEnd() {return houseMap.end();}

	private:
		RentPeriod_t rentPeriod;
		HouseMap houseMap;
};

#endif
