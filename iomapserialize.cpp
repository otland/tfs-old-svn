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
#include "iomapserialize.h"

#include "house.h"
#include "configmanager.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;

bool IOMapSerialize::loadMap(Map* map)
{
	if(g_config.getBool(ConfigManager::HOUSE_STORAGE))
		return loadMapBinary(map);

	return loadMapRelational(map);
}

bool IOMapSerialize::saveMap(Map* map)
{
	if(g_config.getBool(ConfigManager::HOUSE_STORAGE))
		return saveMapBinary(map);

	return saveMapRelational(map);
}


bool IOMapSerialize::loadHouseInfo(Map* map)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT * FROM `houses` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!(result = db->storeQuery(query.str())))
		return false;

	do
	{
		if(House* house = Houses::getInstance().getHouse(result->getDataInt("id")))
		{
			house->setHouseOwner(result->getDataInt("owner"));
			house->setPaidUntil(result->getDataInt("paid"));
			house->setPayRentWarnings(result->getDataInt("warnings"));
			house->setLastWarning(result->getDataInt("lastwarning"));
		}
	}
	while(result->next());
	result->free();

	query.str("");
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		House* house = it->second;
		if(house && house->getHouseOwner() && house->getHouseId())
		{
			query << "SELECT `listid`, `list` FROM `house_lists` WHERE `house_id` = " << house->getHouseId();
			query << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
			if((result = db->storeQuery(query.str())))
			{
				do
					house->setAccessList(result->getDataInt("listid"), result->getDataString("list"));
				while(result->next());
				result->free();
			}
		}

		query.str("");
	}

	return true;
}

bool IOMapSerialize::saveHouseInfo(Map* map)
{
	Database* db = Database::getInstance();
	DBTransaction trans(db);
	if(!trans.begin())
		return false;

	DBQuery query;
	query << "DELETE FROM `houses` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");
	query << "DELETE FROM `house_lists` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!db->executeQuery(query.str()))
		return false;

	DBInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `houses` (`id`, `world_id`, `owner`, `paid`, `warnings`, `lastwarning`, `name`, `town`, `size`, `price`, `rent`) VALUES ");
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		House* house = it->second;
		query.str("");

		query << house->getHouseId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", " << house->getHouseOwner() << ", "
		<< house->getPaidUntil() << ", " << house->getPayRentWarnings() << ", " << house->getLastWarning() << ", "
		<< db->escapeString(house->getName()) << ", " << house->getTownId() << ", " << house->getSize() << ", " << house->getPrice()
		<< ", " << house->getRent();
		if(!query_insert.addRow(query.str()))
			return false;
	}

	if(!query_insert.execute())
		return false;

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		bool saveLists = false;
		query_insert.setQuery("INSERT INTO `house_lists` (`house_id`, `world_id`, `listid`, `list`) VALUES ");

		House* house = it->second;
		query.str("");

		std::string listText;
		if(house->getAccessList(GUEST_LIST, listText) && listText != "")
		{
			query << house->getHouseId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", "
			<< GUEST_LIST << ", " << db->escapeString(listText);
			if(!query_insert.addRow(query.str()))
				return false;

			saveLists = true;
			query.str("");
		}

		if(house->getAccessList(SUBOWNER_LIST, listText) && listText != "")
		{
			query << house->getHouseId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", "
			<< SUBOWNER_LIST << ", " << db->escapeString(listText);
			if(!query_insert.addRow(query.str()))
				return false;

			saveLists = true;
			query.str("");
		}

		for(HouseDoorList::iterator it = house->getHouseDoorBegin(); it != house->getHouseDoorEnd(); ++it)
		{
			const Door* door = (*it);
			if(door->getAccessList(listText) && listText != "")
			{
				query << house->getHouseId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", " << door->getDoorId()
				<< ", " << db->escapeString(listText);
				if(!query_insert.addRow(query.str()))
					return false;

				saveLists = true;
				query.str("");
			}
		}

		if(saveLists)
		{
			if(!query_insert.execute())
				return false;
		}
	}

	return trans.commit();
}

bool IOMapSerialize::loadMapRelational(Map* map)
{
	Database* db = Database::getInstance();
	DBQuery query; //lock mutex - do we really need it here?
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		//load tile
		for(HouseTileList::iterator tit = it->second->getHouseTileBegin(); tit != it->second->getHouseTileEnd(); ++tit)
			loadTile(*db, *tit);
	}

	return true;
}

bool IOMapSerialize::saveMapRelational(Map* map)
{
	Database* db = Database::getInstance();
	//Start the transaction
	DBTransaction trans(db);
	if(!trans.begin())
		return false;

	//clear old tile data
	DBQuery query;
	query << "DELETE FROM `tile_items` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");
	query << "DELETE FROM `tiles` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!db->executeQuery(query.str()))
		return false;

	uint32_t tileId = 0;
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		//save house items
		for(HouseTileList::iterator tit = it->second->getHouseTileBegin(); tit != it->second->getHouseTileEnd(); ++tit)
			saveTile(db, ++tileId, *tit);
	}

	//End the transaction
	return trans.commit();
}

bool IOMapSerialize::loadMapBinary(Map* map)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `data` FROM `house_data` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!(result = db->storeQuery(query.str())))
		return false;

	do
	{
		uint64_t attrSize = 0;
		const char* attr = result->getDataStream("data", attrSize);

		PropStream propStream;
		propStream.init(attr, attrSize);
		while(propStream.size())
		{
			uint16_t x = 0, y = 0;
			uint8_t z = 0;

			propStream.GET_USHORT(x);
			propStream.GET_USHORT(y);
			propStream.GET_UCHAR(z);

			Tile* tile = map->getTile(x, y, (int16_t)z);
			if(!tile)
			{
				std::cout << "[Error - IOMapSerialize::loadMapBinary] Unserialization of invalid tile at position ";
				std::cout << x << "/" << y << "/" << (int16_t)z << std::endl;
				break;
			}

			uint32_t itemCount = 0;
			propStream.GET_ULONG(itemCount);
			while(itemCount--)
				loadItem(propStream, tile);
 		}
	}
	while(result->next());
	result->free();

 	return true;
}

bool IOMapSerialize::saveMapBinary(Map* map)
{
 	Database* db = Database::getInstance();
	//Start the transaction
 	DBTransaction transaction(db);
 	if(!transaction.begin())
 		return false;

	DBQuery query;
	query << "DELETE FROM `house_data` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!db->executeQuery(query.str()))
 		return false;

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `house_data` (`house_id`, `world_id`, `data`) VALUES ");
 	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
 		//save house items
		PropWriteStream stream;
		for(HouseTileList::iterator tit = it->second->getHouseTileBegin(); tit != it->second->getHouseTileEnd(); ++tit)
		{
			if(!saveTile(stream, *tit))
 				return false;
 		}

		uint32_t attributesSize = 0;
		const char* attributes = stream.getStream(attributesSize);

		query.str("");
		query << it->second->getHouseId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", " << db->escapeBlob(attributes, attributesSize);
		if(!stmt.addRow(query))
			return false;
 	}

	query.str("");
	if(!stmt.execute())
		return false;

 	//End the transaction
 	return transaction.commit();
}

bool IOMapSerialize::loadTile(Database& db, Tile* tile)
{
	DBResult* result;

	const Position& tilePos = tile->getPosition();
	ItemMap itemMap;

	DBQuery query;
	query << "SELECT `tiles`.`id` FROM `tiles` WHERE `x` = " << tilePos.x << " AND `y` = " << tilePos.y;
	query << " AND `z` = " << tilePos.z << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!(result = db.storeQuery(query.str())))
		return false;

	int32_t tileId = result->getDataInt("id");
	result->free();

	query.str("");
	query << "SELECT * FROM `tile_items` WHERE `tile_id` = " << tileId << " AND `world_id` = ";
	query << g_config.getNumber(ConfigManager::WORLD_ID) << " ORDER BY `sid` DESC;";
	if((result = db.storeQuery(query.str())))
	{
		Item* item = NULL;
		do
		{
			item = NULL;
			int32_t sid = result->getDataInt("sid"), pid = result->getDataInt("pid");
			int32_t type = result->getDataInt("itemtype"), count = result->getDataInt("count");

			uint64_t attrSize = 0;
			const char* attr = result->getDataStream("attributes", attrSize);
			PropStream propStream;
			propStream.init(attr, attrSize);

			const ItemType& iType = Item::items[type];
			if(iType.moveable || iType.forceSerialize || pid != 0)
			{
				//create a new item
				item = Item::CreateItem(type, count);
				if(!item)
					continue;

				if(!item->unserializeAttr(propStream))
					std::cout << "[Warning - IOMapSerialize::loadTile] Unserialization error [1]" << std::endl;

				if(pid == 0)
				{
					tile->__internalAddThing(item);
					item->__startDecaying();
				}
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

					if(iType.isDoor() && findItem->getDoor())
					{
						item = findItem;
						break;
					}

					if(iType.isBed() && findItem->getBed())
					{
						item = findItem;
						break;
					}
				}
			}

			if(item)
			{
				if(!item->unserializeAttr(propStream))
					std::cout << "[Warning - IOMapSerialize::loadTile] Unserialization error [0]" << std::endl;

				item = g_game.transformItem(item, type);
				itemMap[sid] = std::make_pair(item, pid);
			}
			else
			{
				std::cout << "[Warning - IOMapSerialize::loadTile] NULL item at " << tile->getPosition();
				std::cout << " (type = " << type << ", sid = " << sid << ", pid = " << pid << ")" << std::endl;
			}
		}
		while(result->next());
		result->free();
	}

	ItemMap::reverse_iterator rit;
	ItemMap::iterator it;
	for(rit = itemMap.rbegin(); rit != itemMap.rend(); ++rit)
	{
		Item* item = rit->second.first;
		int32_t pid = rit->second.second;

		it = itemMap.find(pid);
		if(it != itemMap.end())
		{
			if(Container* container = it->second.first->getContainer())
			{
				container->__internalAddThing(item);
				g_game.startDecay(item);
			}
		}
	}

	return true;
}

bool IOMapSerialize::saveTile(Database* db, uint32_t tileId, const Tile* tile)
{
	const Position& tilePosition = tile->getPosition();
	uint32_t tileCount = tile->getThingCount();
	if(!tileCount)
		return true;

	DBQuery query;
	query << "INSERT INTO `tiles` (`id`, `world_id`, `x`, `y`, `z`) VALUES " << "(" << tileId << ", " << g_config.getNumber(ConfigManager::WORLD_ID);
	query << ", " << tilePosition.x << ", " << tilePosition.y << ", " << tilePosition.z << ")";
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");
	ContainerStackList containerStackList;
	int32_t runningId = 0, parentId = 0;

	Item* item = NULL;
	Container* container = NULL;

	DBInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `tile_items` (`tile_id`, `world_id`, `sid`, `pid`, `itemtype`, `count`, `attributes`) VALUES ");
	for(uint32_t i = 0; i < tileCount; ++i)
	{
		item = tile->__getThing(i)->getItem();
		if(!item)
			continue;

		if(item->isNotMoveable() && !item->forceSerialize())
			continue;

		uint32_t attributesSize = 0;
		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		const char* attributes = propWriteStream.getStream(attributesSize);

		runningId++;
		query << tileId << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", " << runningId << ", " << parentId << ", "
			<< item->getID() << ", " << (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);
		if(!query_insert.addRow(query.str()))
			return false;

		query.str("");
		if(item->getContainer())
			containerStackList.push_back(std::make_pair(item->getContainer(), runningId));
	}

	for(ContainerStackList::iterator cit = containerStackList.begin(); cit != containerStackList.end(); ++cit)
	{
		container = (*cit).first;
		parentId = (*cit).second;
		for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it)
		{
			item = (*it);

			uint32_t attributesSize = 0;
			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			runningId++;
			query << tileId << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", " << runningId << ", " << parentId << ", "
				<< item->getID() << ", " << (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);
			if(!query_insert.addRow(query.str()))
				return false;

			query.str("");
			if(item->getContainer())
				containerStackList.push_back(std::make_pair(item->getContainer(), runningId));
		}
	}

	if(!query_insert.execute())
		return false;

	return true;
}

bool IOMapSerialize::saveTile(PropWriteStream& stream, const Tile* tile)
{
	int32_t tileCount = tile->getThingCount();
	if(!tileCount)
		return true;

	std::vector<Item*> items;
	for(; tileCount > 0; --tileCount)
	{
		Item* item = tile->__getThing(tileCount - 1)->getItem();
		if(!item)
			continue;

		if(item->isMoveable() || item->forceSerialize())
			items.push_back(item);
	}

	tileCount = items.size(); //lame, but at least we don't need new variable
	if(tileCount > 0)
	{
		stream.ADD_USHORT(tile->getPosition().x);
		stream.ADD_USHORT(tile->getPosition().y);
		stream.ADD_UCHAR(tile->getPosition().z);

		stream.ADD_ULONG(tileCount);
		for(std::vector<Item*>::iterator it = items.begin(); it != items.end(); ++it)
			saveItem(stream, (*it));
	}

	return true;
}

bool IOMapSerialize::loadItem(PropStream& propStream, Cylinder* parent)
{
	uint16_t id = 0;
	propStream.GET_USHORT(id);

	Item* item = NULL;
	const ItemType& iType = Item::items[id];
	if(iType.moveable || iType.forceSerialize)
	{
		//create a new item
		item = Item::CreateItem(id);
		if(!item)
			return false;

		bool ret = item->unserializeAttr(propStream);
		item = g_game.transformItem(item, id);
		if(!ret)
		{
			propStream.SKIP_N(-1);
			uint8_t prop = 0;
			propStream.GET_UCHAR(prop);
			if(prop == ATTR_CONTAINER_ITEMS)
			{
				Container* container = item->getContainer();

				uint32_t items = 0;
				propStream.GET_ULONG(items);
				while(items > 0)
				{
					if(!loadItem(propStream, container))
						std::cout << "[Warning - IOMapSerialize::loadItem] Unserialization error for container item [1]" << std::endl;

					--items;
				}

				ret = item->unserializeAttr(propStream);
			}
		}

		if(!ret)
			std::cout << "[Warning - IOMapSerialize::loadItem] Unserialization error [0]" << std::endl;

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
		// A static item (bed)
		// find this type in the tile
		Tile* tile = parent->getTile();
		if(!tile)
		{
			std::cout << "[Warning - IOMapSerialize::loadItem] Unserialization error [1]" << std::endl;
			return false;
		}

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

			if(iType.isDoor() && findItem->getDoor())
			{
				item = findItem;
				break;
			}

			if(iType.isBed() && findItem->getBed())
			{
				item = findItem;
				break;
			}
		}

		if(item)
		{
			bool ret = item->unserializeAttr(propStream);
			item = g_game.transformItem(item, id);
			if(!ret)
			{
				propStream.SKIP_N(-1);

				uint8_t prop = 0;
				propStream.GET_UCHAR(prop);
				if(prop == ATTR_CONTAINER_ITEMS)
				{
					Container* container = item->getContainer();

					uint32_t items = 0;
					propStream.GET_ULONG(items);
					while(items > 0)
					{
						if(!loadItem(propStream, container))
							std::cout << "[Warning - IOMapSerialize::loadItem] Unserialization error for container item [0]" << std::endl;

						--items;
					}

					ret = item->unserializeAttr(propStream);
				}
			}

			if(!ret)
				std::cout << "[Warning - IOMapSerialize::loadMapBinary] Unserialization error [2]" << std::endl;
		}
		else
		{
			std::cout << "[Warning - IOMapSerialize::loadItem] NULL item at " << tile->getPosition() << " with id " << id << std::endl;
			Item dummy(0);
			dummy.unserializeAttr(propStream);
		}
	}

	return true;
}

bool IOMapSerialize::saveItem(PropWriteStream& stream, const Item* item)
{
	stream.ADD_USHORT(item->getID());
	item->serializeAttr(stream);
	if(const Container* container = item->getContainer())
	{
		stream.ADD_UCHAR(ATTR_CONTAINER_ITEMS);
		stream.ADD_ULONG(container->size());
		for(ItemList::const_reverse_iterator rit = container->getReversedItems(); rit != container->getReversedEnd(); ++rit)
			saveItem(stream, (*rit));
	}

	stream.ADD_UCHAR(0x00);
	return true;
}
