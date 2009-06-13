//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// SQL map serialization
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

#include "iomapserialize.h"
#include "house.h"
#include "configmanager.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;

bool IOMapSerialize::loadMap(Map* map)
{
	int64_t start = OTSYS_TIME();
	bool s = false;

	if(g_config.getString(ConfigManager::MAP_STORAGE_TYPE) == "binary")
		s = loadMapBinary(map);
	else
		s = loadMapRelational(map);

	std::cout << "Notice: Map load (" << g_config.getString(ConfigManager::MAP_STORAGE_TYPE) << ") took : " <<
		(OTSYS_TIME() - start)/(1000.) << " s" << std::endl;

	return s;
}

bool IOMapSerialize::saveMap(Map* map)
{
	int64_t start = OTSYS_TIME();
	bool s = false;

	if(g_config.getString(ConfigManager::MAP_STORAGE_TYPE) == "binary")
		s = saveMapBinary(map);
	else
		s = saveMapRelational(map);

	std::cout << "Notice: Map save (" << g_config.getString(ConfigManager::MAP_STORAGE_TYPE) << ") took : " <<
		(OTSYS_TIME() - start)/(1000.) << " s" << std::endl;

	return s;
}

bool IOMapSerialize::loadMapRelational(Map* map)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		//load tile
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it)
			loadTile(*db, *it);
	}
	return true;
}

bool IOMapSerialize::loadTile(Database& db, Tile* tile)
{
	typedef std::map<int,std::pair<Item*,int> > ItemMap;
	ItemMap itemMap;

	const Position& tilePos = tile->getPosition();

	DBQuery query;

	query << "SELECT `id` FROM `tiles` WHERE `x` = " << tilePos.x
		<< " AND `y` = " << tilePos.y
		<< " AND `z` = " << tilePos.z;

	DBResult result;
	if(!db.storeQuery(query, result) || result.getNumRows() != 1)
		return false;

	int tileId = result.getDataInt("id");

	query.str("");
	query << "SELECT * FROM `tile_items` WHERE `tile_id` = "
		<< tileId
		<< " ORDER BY `sid` DESC";
	if(db.storeQuery(query, result) && (result.getNumRows() > 0))
	{
		Item* item = NULL;

		for(uint32_t i = 0; i < result.getNumRows(); ++i)
		{
			int sid = result.getDataInt("sid", i);
			int pid = result.getDataInt("pid", i);
			int type = result.getDataInt("itemtype", i);
			int count = result.getDataInt("count", i);
			item = NULL;

			unsigned long attrSize = 0;
			const char* attr = result.getDataBlob("attributes", attrSize, i);
			PropStream propStream;
			propStream.init(attr, attrSize);

			const ItemType& iType = Item::items[type];
			if(iType.moveable || /* or object in a container*/ pid != 0)
			{
				//create a new item
				item = Item::CreateItem(type, count);

				if(item)
				{
					if(item->unserializeAttr(propStream))
					{
						if(pid == 0)
						{
							tile->__internalAddThing(item);
							item->__startDecaying();
						}
					}
					else
						std::cout << "WARNING: Serialize error in IOMapSerialize::loadTile()" << std::endl;
				}
				else
					continue;
			}
			else
			{
				//find this type in the tile
				for(uint32_t i = 0; i < tile->getThingCount(); ++i)
				{
					Item* findItem = tile->__getThing(i)->getItem();

					if(!findItem)
						continue;

					if(findItem->getID() == type)
					{
						item = findItem;
						break;
					}
					else if(iType.isDoor() && findItem->getDoor())
					{
						item = findItem;
						break;
					}
					//[ added for beds system
					else if(iType.isBed() && findItem->getBed())
					{
						item = findItem;
						break;
					}
					//]
				}
			}

			if(item)
			{
				if(item->unserializeAttr(propStream))
				{
					item = g_game.transformItem(item, type);
					if(item)
					{
						std::pair<Item*, int> myPair(item, pid);
						itemMap[sid] = myPair;
					}
				}
				else
					std::cout << "WARNING: Serialize error in IOMapSerialize::loadTile()" << std::endl;
			}
			else
				std::cout << "WARNING: IOMapSerialize::loadTile(). NULL item at " << tile->getPosition() << ". type = " << type << ", sid = " << sid << ", pid = " << pid << std::endl;
		}
	}

	ItemMap::reverse_iterator it;
	ItemMap::iterator it2;

	for(it = itemMap.rbegin(); it != itemMap.rend(); ++it)
	{
		Item* item = it->second.first;
		int pid = it->second.second;

		it2 = itemMap.find(pid);
		if(it2 != itemMap.end())
		{
			if(Container* container = it2->second.first->getContainer())
			{
				container->__internalAddThing(item);
				g_game.startDecay(item);
			}
		}
	}
	return true;
}

bool IOMapSerialize::saveMapRelational(Map* map)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	//Start the transaction
	DBTransaction trans(db);
	if(!trans.start())
		return false;

	//clear old tile data
	DBQuery query;
	query << "DELETE FROM `tile_items`;";
	if(!db->executeQuery(query))
		return false;

	query << "DELETE FROM `tiles`;";
	if(!db->executeQuery(query))
		return false;
	uint32_t tileId = 0;
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		//save house items
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it)
		{
			++tileId;
			saveTile(db, tileId, *it);
		}
	}
	//End the transaction
	return trans.success();
}

bool IOMapSerialize::saveTile(Database* db, uint32_t tileId, const Tile* tile)
{
	typedef std::list<std::pair<Container*, int> > ContainerStackList;
	typedef ContainerStackList::value_type ContainerStackList_Pair;
	ContainerStackList containerStackList;

	bool storedTile = false;
	int32_t runningID = 0;
	Item* item = NULL;
	Container* container = NULL;

	int32_t parentid = 0;
	std::stringstream streamitems;
	std::string itemsstring;

	DBSplitInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `tile_items` (`tile_id`, `sid` , `pid` , `itemtype` , `count`, `attributes` ) VALUES ");
	for(uint32_t i = 0; i < tile->getThingCount(); ++i)
	{
		item = tile->__getThing(i)->getItem();
		if(!item)
			continue;

		if(!(!item->isNotMoveable() ||
			item->getDoor() ||
			(item->getContainer() && item->getContainer()->size() != 0) ||
			item->canWriteText() ||
			item->getBed()))
			continue;

		//only save beds in houses
		if(item->getBed() && !tile->hasFlag(TILESTATE_HOUSE))
			continue;

		if(!storedTile)
		{
			DBQuery tileListQuery;
			const Position& tilePos = tile->getPosition();
			tileListQuery << "INSERT INTO `tiles` (`id`, `x` , `y` , `z` ) VALUES";
			tileListQuery << "(" << tileId << "," << tilePos.x << "," << tilePos.y << "," << tilePos.z << ")";

			if(!db->executeQuery(tileListQuery))
				return false;

			storedTile = true;
		}
		++runningID;

		uint32_t attributesSize;

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		const char* attributes = propWriteStream.getStream(attributesSize);

		streamitems << "(" << tileId << "," << runningID << "," << parentid << "," << item->getID() << ","
			<< (int32_t)item->getSubType() << ",'" << Database::escapeString(attributes, attributesSize) << "')";

		if(!query_insert.addRow(streamitems.str()))
			return false;

		streamitems.str("");

		if(item->getContainer())
			containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));
	}

	while(containerStackList.size() > 0)
	{
		ContainerStackList_Pair csPair = containerStackList.front();
		container = csPair.first;
		parentid = csPair.second;
		containerStackList.pop_front();

		for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it)
		{
			item = (*it);
			++runningID;
			if(item->getContainer())
				containerStackList.push_back(ContainerStackList_Pair(item->getContainer(), runningID));

			uint32_t attributesSize;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			streamitems << "(" << tileId << "," << runningID << "," << parentid << "," << item->getID() << ","
				<< (int32_t)item->getSubType() << ",'" << Database::escapeString(attributes, attributesSize) << "')";

			if(!query_insert.addRow(streamitems.str()))
				return false;

			streamitems.str("");
		}
	}

	if(!query_insert.executeQuery())
		return false;

	return true;
}

bool IOMapSerialize::loadMapBinary(Map* map)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query.reset();
	query << "SELECT `data` FROM `map_store`;";

	DBResult result;

	if(db->storeQuery(query, result))
	{
		for(uint32_t i = 0; i < result.getNumRows(); ++i)
		{
			unsigned long attrSize = 0;
			const char* attr = result.getDataBlob("data", attrSize, i);
			PropStream propStream;
			propStream.init(attr, attrSize);

			while(propStream.size())
			{
				uint32_t item_count = 0;
				uint16_t x = 0, y = 0;
				uint8_t z = 0;

				propStream.GET_USHORT(x);
				propStream.GET_USHORT(y);
				propStream.GET_UCHAR(z);

				Tile* tile = map->getTile(x, y, z);
				if(!tile)
				{
					std::cout << "ERROR: Unserialization of invalid tile in IOMapSerialize::loadTile()" << std::endl;
					break;
				}

				propStream.GET_ULONG(item_count);
				while(item_count--)
					loadItem(propStream, tile);
 			}
		}
	}
 	return true;
}

bool IOMapSerialize::loadContainer(PropStream& propStream, Container* container)
{
	while(container->serializationCount > 0)
	{
		if(!loadItem(propStream, container))
		{
			std::cout << "WARNING: Unserialization error for containing item in IOMapSerialize::loadContainer() - " << container->getID() << std::endl;
			return false;
		}
		container->serializationCount--;
	}

	uint8_t endAttr = 0x00;
	propStream.GET_UCHAR(endAttr);

	if(endAttr != 0x00)
	{
		std::cout << "WARNING: Unserialization error for containing item in IOMapSerialize::loadContainer() - " << container->getID() << std::endl;
		return false;
	}
	return true;
}

bool IOMapSerialize::loadItem(PropStream& propStream, Cylinder* parent)
{
	Item* item = NULL;

	uint16_t id = 0;
	propStream.GET_USHORT(id);

	const ItemType& iType = Item::items[id];
	Tile* tile = NULL;
	if(parent->getParent() == NULL)
		tile = parent->getTile();

	if(iType.moveable || !tile)
	{
		//create a new item
		item = Item::CreateItem(id);
		if(item)
		{
			if(item->unserializeAttr(propStream))
			{
				Container* container = item->getContainer();
				if(container && !loadContainer(propStream, container))
				{
					delete item;
					return false;
				}

				if(parent)
				{
					parent->__internalAddThing(item);
					item->__startDecaying();
				}
				else
					delete item;
			}
			else
			{
				std::cout << "WARNING: Unserialization error in IOMapSerialize::loadItem()" << id << std::endl;
				delete item;
				return false;
			}
		}
	}
	else
	{
		// Stationary items like doors/beds/blackboards/bookcases
		for(uint32_t i = 0; i < tile->getThingCount(); ++i)
		{
			Item* findItem = tile->__getThing(i)->getItem();

			if(!findItem)
				continue;

			if(findItem->getID() == id)
			{
				item = findItem;
				break;
			}
			else if(iType.isDoor() && findItem->getDoor())
			{
				item = findItem;
				break;
			}
			else if(iType.isBed() && findItem->getBed())
			{
				item = findItem;
				break;
			}
		}

		if(item)
		{
			if(item->unserializeAttr(propStream))
			{
				Container* container = item->getContainer();
				if(container && !loadContainer(propStream, container))
					return false;

				item = g_game.transformItem(item, id);
			}
			else
				std::cout << "WARNING: Unserialization error in IOMapSerialize::loadItem()" << id << std::endl;
		}
		else
		{
			//The map changed since the last save, just read the attributes
			Item* dummy = Item::CreateItem(id);
			if(dummy)
			{
				dummy->unserializeAttr(propStream);
				Container* container = dummy->getContainer();
				if(container && !loadContainer(propStream, container))
				{
					delete dummy;
					return false;
				}
				delete dummy;
			}
		}
	}
	return true;
}

bool IOMapSerialize::saveMapBinary(Map* map)
{
 	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	//Start the transaction
	DBTransaction trans(db);
	if(!trans.start())
		return false;

 	DBQuery query;
	query << "DELETE FROM `map_store`;";
	if(!db->executeQuery(query))
 		return false;

	DBSplitInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `map_store` (`house_id`, `data`) VALUES ");

	//clear old tile data
 	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin();
		it != Houses::getInstance().getHouseEnd();
		++it)
	{
 		//save house items
 		House* house = it->second;
		PropWriteStream stream;
		for(HouseTileList::iterator tile_iter = house->getHouseTileBegin();
			tile_iter != house->getHouseTileEnd();
			++tile_iter)
		{
			if(!saveTile(stream, *tile_iter))
 				return false;
 		}

		uint32_t attributesSize;
		const char* attributes = stream.getStream(attributesSize);
		query << "(" << it->second->getHouseId() << ", " <<
			db->escapeBlob(attributes, attributesSize) << ")";

		if(!query_insert.addRow(query.str()))
			return false;

		query.str("");
 	}

	if(!query_insert.executeQuery())
		return false;

 	//End the transaction
 	return trans.success();
}

bool IOMapSerialize::saveItem(PropWriteStream& stream, const Item* item)
{
	const Container* container = item->getContainer();

	// Write ID & props
	stream.ADD_USHORT(item->getID());
	item->serializeAttr(stream);

	if(container)
	{
		// Hack our way into the attributes
		stream.ADD_UCHAR(ATTR_CONTAINER_ITEMS);
		stream.ADD_ULONG(container->size());
		for(ItemList::const_reverse_iterator i = container->getReversedItems(); i != container->getReversedEnd(); ++i)
			saveItem(stream, *i);
	}

	stream.ADD_UCHAR(0x00); // attr end
	return true;
}

bool IOMapSerialize::saveTile(PropWriteStream& stream, const Tile* tile)
{
	std::vector<Item*> items;
	for(int32_t i = tile->getThingCount(); i > 0; --i)
	{
		Item* item = tile->__getThing(i - 1)->getItem();
		if(!item)
			continue;

		// Note that these are NEGATED, ie. these are the items that will be saved.
		if(!(!item->isNotMoveable() || item->getDoor() || (item->getContainer() &&
			item->getContainer()->size() != 0) || item->canWriteText() || item->getBed()))
		{
			continue;
		}

		items.push_back(item);
	}

	if(items.size() > 0)
	{
		stream.ADD_USHORT(tile->getPosition().x);
		stream.ADD_USHORT(tile->getPosition().y);
		stream.ADD_UCHAR(tile->getPosition().z);
		stream.ADD_ULONG(items.size());

		for(std::vector<Item*>::iterator iter = items.begin();
			iter != items.end();
			++iter)
		{
			saveItem(stream, *iter);
		}
	}
	return true;
}

bool IOMapSerialize::loadHouseInfo(Map* map)
{
	Database* db = Database::getInstance();
	DBQuery query;
	DBResult result;

	if(!db->connect())
		return false;

	query << "SELECT * FROM `houses`";
	if(!db->storeQuery(query, result) || result.getNumRows() == 0)
		return false;

	for(uint32_t i = 0; i < result.getNumRows(); ++i)
	{
		int32_t houseid = result.getDataInt("id", i);
		House* house = Houses::getInstance().getHouse(houseid);
		if(house)
		{
			int32_t ownerid = result.getDataInt("owner", i);
			int32_t paid = result.getDataInt("paid", i);
			int32_t payRentWarnings = result.getDataInt("warnings", i);

			house->setHouseOwner(ownerid);
			house->setPaidUntil(paid);
			house->setPayRentWarnings(payRentWarnings);
		}
	}

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		query.reset();
		House* house = it->second;
		if(house->getHouseOwner() != 0 && house->getHouseId() != 0)
		{
			query << "SELECT `listid`, `list` FROM `house_lists` WHERE `house_id` = " << house->getHouseId();
			if(db->storeQuery(query, result) && result.getNumRows() != 0)
			{
				for(uint32_t i = 0; i < result.getNumRows(); ++i)
				{
					int32_t listid = result.getDataInt("listid", i);
					std::string list = result.getDataString("list", i);
					house->setAccessList(listid, list);
				}
			}
		}
	}
	return true;
}

bool IOMapSerialize::saveHouseInfo(Map* map)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	DBTransaction trans(db);
	if(!trans.start())
		return false;

	DBQuery query;
	query << "DELETE FROM `houses`;";
	if(!db->executeQuery(query))
		return false;

	query << "DELETE FROM `house_lists`;";
	if(!db->executeQuery(query))
		return false;

	std::stringstream housestream;

	DBSplitInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `houses` (`id` , `owner` , `paid`, `warnings`) VALUES ");

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		House* house = it->second;
		housestream << "(" << house->getHouseId() << "," << house->getHouseOwner() << "," << house->getPaidUntil() << "," << house->getPayRentWarnings() << ")";
		if(!query_insert.addRow(housestream.str()))
			return false;

		housestream.str("");
	}

	if(!query_insert.executeQuery())
		return false;

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		bool save_lists = false;
		query_insert.setQuery("INSERT INTO `house_lists` (`house_id` , `listid` , `list`) VALUES ");
		House* house = it->second;

		std::string listText;
		if(house->getAccessList(GUEST_LIST, listText) && listText != "")
		{
			housestream << "(" << house->getHouseId() << "," << GUEST_LIST << ",'" << Database::escapeString(listText) << "')";
			save_lists = true;

			if(!query_insert.addRow(housestream.str()))
				return false;

			housestream.str("");
		}

		if(house->getAccessList(SUBOWNER_LIST, listText) && listText != "")
		{
			housestream << "(" << house->getHouseId() << "," << SUBOWNER_LIST << ",'" << Database::escapeString(listText) << "')";
			save_lists = true;

			if(!query_insert.addRow(housestream.str()))
				return false;

			housestream.str("");
		}

		for(HouseDoorList::iterator it = house->getHouseDoorBegin(); it != house->getHouseDoorEnd(); ++it)
		{
			const Door* door = *it;
			if(door->getAccessList(listText) && listText != "")
			{
				housestream << "(" << house->getHouseId() << "," << door->getDoorId() << ",'" << Database::escapeString(listText) << "')";
				save_lists = true;

				if(!query_insert.addRow(housestream.str()))
					return false;

				housestream.str("");
			}
		}

		if(save_lists)
		{
			if(!query_insert.executeQuery())
				return false;
		}
	}
	return trans.success();
}
