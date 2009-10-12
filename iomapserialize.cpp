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
#include "iologindata.h"

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

bool IOMapSerialize::updateAuctions()
{
	Database* db = Database::getInstance();
	DBQuery query;

	time_t now = time(NULL);
	query << "SELECT `house_id`, `player_id`, `bid` FROM `house_auctions` WHERE `endtime` < " << now;

	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return true;

	bool success = true;
	House* house = NULL;
	do
	{
		query.str("");
		query << "DELETE FROM `house_auctions` WHERE `house_id` = " << result->getDataInt("house_id");
		if(!(house = Houses::getInstance()->getHouse(result->getDataInt(
			"house_id"))) || !db->executeQuery(query.str()))
		{
			success = false;
			continue;
		}

		house->setOwner(result->getDataInt("player_id"));
		Houses::getInstance()->payHouse(house, now, result->getDataInt("bid"));
	}
	while(result->next());
	result->free();
	return success;
}

bool IOMapSerialize::loadHouses()
{
	Database* db = Database::getInstance();
	DBQuery query;

	query << "SELECT * FROM `houses` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	House* house = NULL;
	do
	{
		if(!(house = Houses::getInstance()->getHouse(result->getDataInt("id"))))
			continue;

		house->setRentWarnings(result->getDataInt("warnings"));
		house->setLastWarning(result->getDataInt("lastwarning"));
		house->setPaidUntil(result->getDataInt("paid"));

		house->setOwner(result->getDataInt("owner"));
		if(result->getDataInt("clear"))
			house->setPendingTransfer(true);
	}
	while(result->next());
	result->free();

	for(HouseMap::iterator it = Houses::getInstance()->getHouseBegin(); it != Houses::getInstance()->getHouseEnd(); ++it)
	{
		if(!(house = it->second) || !house->getId() || !house->getOwner())
			continue;

		query.str("");
		query << "SELECT `listid`, `list` FROM `house_lists` WHERE `house_id` = " << house->getId();
		query << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
		if(!(result = db->storeQuery(query.str())))
			continue;

		do
			house->setAccessList(result->getDataInt("listid"), result->getDataString("list"));
		while(result->next());
		result->free();
	}

	return true;
}

bool IOMapSerialize::updateHouses()
{
	Database* db = Database::getInstance();
	DBQuery query;

	House* house = NULL;
	for(HouseMap::iterator it = Houses::getInstance()->getHouseBegin(); it != Houses::getInstance()->getHouseEnd(); ++it)
	{
		if(!(house = it->second))
			continue;

		query << "SELECT `id` FROM `houses` WHERE `id` = " << house->getId() << " AND `world_id` = "
			<< g_config.getNumber(ConfigManager::WORLD_ID) << " LIMIT 1";
		if(DBResult* result = db->storeQuery(query.str()))
		{
			result->free();
			query.str("");

			query << "UPDATE `houses` SET ";
			if(house->hasSyncFlag(House::HOUSE_SYNC_NAME))
				query << "`name` = " << db->escapeString(house->getName()) << ", ";

			if(house->hasSyncFlag(House::HOUSE_SYNC_TOWN))
				query << "`town` = " << house->getTownId() << ", ";

			if(house->hasSyncFlag(House::HOUSE_SYNC_SIZE))
				query << "`size` = " << house->getSize() << ", ";

			if(house->hasSyncFlag(House::HOUSE_SYNC_PRICE))
				query << "`price` = " << house->getPrice() << ", ";

			if(house->hasSyncFlag(House::HOUSE_SYNC_RENT))
				query << "`rent` = " << house->getRent() << ", ";

			query << "`doors` = " << house->getDoorsCount() << ", `beds` = "
				<< house->getBedsCount() << ", `tiles` = " << house->getTilesCount();
			if(house->hasSyncFlag(House::HOUSE_SYNC_GUILD))
				query << ", `guild` = " << house->isGuild();

			query << " WHERE `id` = " << house->getId() << " AND `world_id` = "
				<< g_config.getNumber(ConfigManager::WORLD_ID) << db->getUpdateLimiter();
		}
		else
		{
			query.str("");
			query << "INSERT INTO `houses` (`id`, `world_id`, `owner`, `name`, `town`, `size`, `price`, `rent`, `doors`, `beds`, `tiles`, `guild`) VALUES ("
				<< house->getId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", 0, "
				//we need owner for compatibility reasons (field doesn't have a default value)
				<< db->escapeString(house->getName()) << ", " << house->getTownId() << ", "
				<< house->getSize() << ", " << house->getPrice() << ", " << house->getRent() << ", "
				<< house->getDoorsCount() << ", " << house->getBedsCount() << ", "
				<< house->getTilesCount() << ", " << house->isGuild() << ")";
		}

		if(!db->executeQuery(query.str()))
			return false;

		query.str("");
	}

	return true;
}

bool IOMapSerialize::saveHouses()
{
	Database* db = Database::getInstance();
	DBTransaction trans(db);
	if(!trans.begin())
		return false;

	for(HouseMap::iterator it = Houses::getInstance()->getHouseBegin(); it != Houses::getInstance()->getHouseEnd(); ++it)
		saveHouse(db, it->second);

	return trans.commit();
}

bool IOMapSerialize::saveHouse(Database* db, House* house)
{
	DBQuery query;
	query << "UPDATE `houses` SET `owner` = " << house->getOwner() << ", `paid` = "
		<< house->getPaidUntil() << ", `warnings` = " << house->getRentWarnings() << ", `lastwarning` = "
		<< house->getLastWarning() << ", `clear` = 0 WHERE `id` = " << house->getId() << " AND `world_id` = "
		<< g_config.getNumber(ConfigManager::WORLD_ID) << db->getUpdateLimiter();
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");
	query << "DELETE FROM `house_lists` WHERE `house_id` = " << house->getId() << " AND `world_id` = "
		<< g_config.getNumber(ConfigManager::WORLD_ID);
	if(!db->executeQuery(query.str()))
		return false;

	DBInsert queryInsert(db);
	queryInsert.setQuery("INSERT INTO `house_lists` (`house_id`, `world_id`, `listid`, `list`) VALUES ");

	std::string listText;
	if(house->getAccessList(GUEST_LIST, listText) && !listText.empty())
	{
		query.str("");
		query << house->getId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", "
			<< GUEST_LIST << ", " << db->escapeString(listText);
		if(!queryInsert.addRow(query.str()))
			return false;
	}

	if(house->getAccessList(SUBOWNER_LIST, listText) && !listText.empty())
	{
		query.str("");
		query << house->getId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", "
			<< SUBOWNER_LIST << ", " << db->escapeString(listText);
		if(!queryInsert.addRow(query.str()))
			return false;
	}

	for(HouseDoorList::iterator it = house->getHouseDoorBegin(); it != house->getHouseDoorEnd(); ++it)
	{
		const Door* door = (*it);
		if(!door)
			continue;

		if(door->getAccessList(listText) && !listText.empty())
		{
			query.str("");
			query << house->getId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", "
				<< door->getDoorId() << ", " << db->escapeString(listText);
			if(!queryInsert.addRow(query.str()))
				return false;
		}
	}

	return query.str().empty() || queryInsert.execute();
}

bool IOMapSerialize::loadMapRelational(Map* map)
{
	Database* db = Database::getInstance();
	DBQuery query; //lock mutex!

	House* house = NULL;
	for(HouseMap::iterator it = Houses::getInstance()->getHouseBegin(); it != Houses::getInstance()->getHouseEnd(); ++it)
	{
		if(!(house = it->second))
			continue;

		query.str("");
		query << "SELECT * FROM `tiles` WHERE `house_id` = " << house->getId() <<
			" AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
		if(DBResult* result = db->storeQuery(query.str()))
		{
			do
			{
				query.str("");
				query << "SELECT * FROM `tile_items` WHERE `tile_id` = " << result->getDataInt("id") << " AND `world_id` = "
					<< g_config.getNumber(ConfigManager::WORLD_ID) << " ORDER BY `sid` DESC";
				if(DBResult* itemsResult = db->storeQuery(query.str()))
				{
					if(house->hasPendingTransfer())
					{
						if(Player* player = g_game.getPlayerByGuidEx(house->getOwner()))
						{
							Depot* depot = player->getDepot(house->getTownId(), true);
							loadItems(db, itemsResult, depot, true);
							if(player->isVirtual())
							{
								IOLoginData::getInstance()->savePlayer(player);
								delete player;
							}
						}
					}
					else
					{
						Position pos(result->getDataInt("x"), result->getDataInt("y"), result->getDataInt("z"));
						if(Tile* tile = map->getTile(pos))
							loadItems(db, itemsResult, tile, false);
						else
							std::cout << "[Error - IOMapSerialize::loadMapRelational] Unserialization"
								<< " of invalid tile at position "<< pos << std::endl;
					}

					itemsResult->free();
				}
			}
			while(result->next());
			result->free();
		}
		else //backward compatibility
		{
			for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it)
			{
				query.str("");
				query << "SELECT `id` FROM `tiles` WHERE `x` = " << (*it)->getPosition().x << " AND `y` = "
					<< (*it)->getPosition().y << " AND `z` = " << (*it)->getPosition().z << " AND `world_id` = "
					<< g_config.getNumber(ConfigManager::WORLD_ID) << " LIMIT 1";
				if(DBResult* result = db->storeQuery(query.str()))
				{
					query.str("");
					query << "SELECT * FROM `tile_items` WHERE `tile_id` = " << result->getDataInt("id") << " AND `world_id` = "
						<< g_config.getNumber(ConfigManager::WORLD_ID) << " ORDER BY `sid` DESC";
					if(DBResult* itemsResult = db->storeQuery(query.str()))
					{
						if(house->hasPendingTransfer())
						{
							if(Player* player = g_game.getPlayerByGuidEx(house->getOwner()))
							{
								Depot* depot = player->getDepot(house->getTownId(), true);
								loadItems(db, itemsResult, depot, true);
								if(player->isVirtual())
								{
									IOLoginData::getInstance()->savePlayer(player);
									delete player;
								}
							}
						}
						else
							loadItems(db, itemsResult, (*it), false);

						itemsResult->free();
					}

					result->free();
				}
			}
		}
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
	for(HouseMap::iterator it = Houses::getInstance()->getHouseBegin(); it != Houses::getInstance()->getHouseEnd(); ++it)
	{
		//save house items
		for(HouseTileList::iterator tit = it->second->getHouseTileBegin(); tit != it->second->getHouseTileEnd(); ++tit)
			saveItems(db, tileId, it->second->getId(), (*tit));
	}

	//End the transaction
	return trans.commit();
}

bool IOMapSerialize::loadMapBinary(Map* map)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `house_id`, `data` FROM `house_data` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!(result = db->storeQuery(query.str())))
		return false;

	House* house = NULL;
	do
	{
		int32_t houseId = result->getDataInt("house_id");
		house = Houses::getInstance()->getHouse(houseId);

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

			uint32_t itemCount = 0;
			propStream.GET_ULONG(itemCount);

			Position pos(x, y, (int16_t)z);
			if(house && house->hasPendingTransfer())
			{
				if(Player* player = g_game.getPlayerByGuidEx(house->getOwner()))
				{
					Depot* depot = player->getDepot(player->getTown(), true);
					while(itemCount--)
						loadItem(propStream, depot, true);

					if(player->isVirtual())
					{
						IOLoginData::getInstance()->savePlayer(player);
						delete player;
					}
				}
			}
			else if(Tile* tile = map->getTile(pos))
			{
				while(itemCount--)
					loadItem(propStream, tile, false);
			}
			else
			{
				std::cout << "[Error - IOMapSerialize::loadMapBinary] Unserialization of invalid tile"
					<< " at position " << pos << std::endl;
				break;
			}
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
 	for(HouseMap::iterator it = Houses::getInstance()->getHouseBegin(); it != Houses::getInstance()->getHouseEnd(); ++it)
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

		query << it->second->getId() << ", " << g_config.getNumber(ConfigManager::WORLD_ID)
			<< ", " << db->escapeBlob(attributes, attributesSize);
		if(!stmt.addRow(query))
			return false;
 	}

	query.str("");
	if(!stmt.execute())
		return false;

 	//End the transaction
 	return transaction.commit();
}

bool IOMapSerialize::loadItems(Database* db, DBResult* result, Cylinder* parent, bool depotTransfer/* = false*/)
{
	ItemMap itemMap;
	Tile* tile = NULL;
	if(!parent->getItem())
		tile = parent->getTile();


	Item* item = NULL;
	int32_t sid, pid, id, count;
	do
	{
		sid = result->getDataInt("sid");
		pid = result->getDataInt("pid");
		id = result->getDataInt("itemtype");
		count = result->getDataInt("count");

		item = NULL;
		uint64_t attrSize = 0;
		const char* attr = result->getDataStream("attributes", attrSize);

		PropStream propStream;
		propStream.init(attr, attrSize);

		const ItemType& iType = Item::items[id];
		if(iType.moveable || iType.forceSerialize || pid)
		{
			if(!(item = Item::CreateItem(id, count)))
				continue;

			if(item->unserializeAttr(propStream))
			{
				if(!pid)
				{
					parent->__internalAddThing(item);
					item->__startDecaying();
				}
			}
			else
				std::cout << "[Warning - IOMapSerialize::loadItems] Unserialization error [0] for item type " << id << std::endl;
		}
		else if(tile)
		{
			//find this type in the tile
			Item* findItem = NULL;
			for(uint32_t i = 0; i < tile->getThingCount(); ++i)
			{
				if(!(findItem = tile->__getThing(i)->getItem()))
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
		}

		if(item)
		{
			if(item->unserializeAttr(propStream))
			{
				if((item = g_game.transformItem(item, id)))
					itemMap[sid] = std::make_pair(item, pid);
			}
			else
				std::cout << "[Warning - IOMapSerialize::loadItems] Unserialization error [1] for item type " << id << std::endl;
		}
		else if((item = Item::CreateItem(id)))
		{
			item->unserializeAttr(propStream);
			if(!depotTransfer)
				std::cout << "[Warning - IOMapSerialize::loadItems] NULL item at "
				<< tile->getPosition() << " (type = " << id << ", sid = "
				<< sid << ", pid = " << pid << ")" << std::endl;
			else
				itemMap[sid] = std::make_pair(parent->getItem(), pid);

			delete item;
			item = NULL;
		}
	}
	while(result->next());

	ItemMap::iterator it;
	for(ItemMap::reverse_iterator rit = itemMap.rbegin(); rit != itemMap.rend(); ++rit)
	{
		if(!(item = rit->second.first))
			continue;

		int32_t pid = rit->second.second;
		it = itemMap.find(pid);
		if(it == itemMap.end())
			continue;

		if(Container* container = it->second.first->getContainer())
		{
			container->__internalAddThing(item);
			g_game.startDecay(item);
		}
	}

	return true;
}

bool IOMapSerialize::saveItems(Database* db, uint32_t& tileId, uint32_t houseId, const Tile* tile)
{
	int32_t thingCount = tile->getThingCount();
	if(!thingCount)
		return true;

	Item* item = NULL;
	int32_t runningId = 0, parentId = 0;
	ContainerStackList containerStackList;

	bool stored = false;
	DBInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `tile_items` (`tile_id`, `world_id`, `sid`, `pid`, `itemtype`, `count`, `attributes`) VALUES ");

	DBQuery query;
	for(int32_t i = 0; i < thingCount; ++i)
	{
		if(!(item = tile->__getThing(i)->getItem()) || (item->isNotMoveable() && !item->forceSerialize()))
			continue;

		if(!stored)
		{
			Position tilePosition = tile->getPosition();
			query << "INSERT INTO `tiles` (`id`, `world_id`, `house_id`, `x`, `y`, `z`) VALUES ("
			<< tileId << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", " << houseId << ", "
			<< tilePosition.x << ", " << tilePosition.y << ", " << tilePosition.z << ")";
			if(!db->executeQuery(query.str()))
				return false;

			stored = true;
			query.str("");
		}

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);

		uint32_t attributesSize = 0;
		const char* attributes = propWriteStream.getStream(attributesSize);

		query << tileId << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", " << ++runningId << ", " << parentId << ", "
			<< item->getID() << ", " << (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);
		if(!query_insert.addRow(query.str()))
			return false;

		query.str("");
		if(item->getContainer())
			containerStackList.push_back(std::make_pair(item->getContainer(), runningId));
	}

	Container* container = NULL;
	for(ContainerStackList::iterator cit = containerStackList.begin(); cit != containerStackList.end(); ++cit)
	{
		container = cit->first;
		parentId = cit->second;
		for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it)
		{
			if(!(item = (*it)))
				continue;

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);

			uint32_t attributesSize = 0;
			const char* attributes = propWriteStream.getStream(attributesSize);

			query << tileId << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", " << ++runningId << ", " << parentId << ", "
				<< item->getID() << ", " << (int32_t)item->getSubType() << ", " << db->escapeBlob(attributes, attributesSize);
			if(!query_insert.addRow(query.str()))
				return false;

			query.str("");
			if(item->getContainer())
				containerStackList.push_back(std::make_pair(item->getContainer(), runningId));
		}
	}

	if(stored)
		++tileId;

	return query_insert.execute();
}

bool IOMapSerialize::loadContainer(PropStream& propStream, Container* container)
{
	while(container->serializationCount > 0)
	{
		if(!loadItem(propStream, container, false))
		{
			std::cout << "[Warning - IOMapSerialize::loadContainer] Unserialization error [0] for item in container " << container->getID() << std::endl;
			return false;
		}

		container->serializationCount--;
	}

	uint8_t endAttr = ATTR_END;
	propStream.GET_UCHAR(endAttr);
	if(endAttr == ATTR_END)
		return true;

	std::cout << "[Warning - IOMapSerialize::loadContainer] Unserialization error [1] for item in container " << container->getID() << std::endl;
	return false;
}

bool IOMapSerialize::loadItem(PropStream& propStream, Cylinder* parent, bool depotTransfer/* = false*/)
{
	Tile* tile = NULL;
	if(!parent->getItem())
		tile = parent->getTile();

	uint16_t id = 0;
	propStream.GET_USHORT(id);
	Item* item = NULL;

	const ItemType& iType = Item::items[id];
	if(iType.moveable || iType.forceSerialize || (!depotTransfer && !tile))
	{
		if(!(item = Item::CreateItem(id)))
			return true;

		if(!item->unserializeAttr(propStream))
		{
			std::cout << "[Warning - IOMapSerialize::loadItem] Unserialization error [0] for item type " << id << std::endl;
			delete item;
			return false;
		}

		if(Container* container = item->getContainer())
		{
			if(!loadContainer(propStream, container))
			{
				delete item;
				return false;
			}
		}

		if(parent)
		{
			parent->__internalAddThing(item);
			item->__startDecaying();
		}
		else
			delete item;

		return true;
	}

	if(tile)
	{
		//Stationary items
		Item* findItem = NULL;
		for(uint32_t i = 0; i < tile->getThingCount(); ++i)
		{
			if(!(findItem = tile->__getThing(i)->getItem()))
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
			std::cout << "[Warning - IOMapSerialize::loadItem] Unserialization error [1] for item type " << id << std::endl;

		return true;
	}

	//The map changed since the last save, just read the attributes
	if(!(item = Item::CreateItem(id)))
		return true;

	item->unserializeAttr(propStream);
	if(Container* container = item->getContainer())
	{
		if(!loadContainer(propStream, container))
		{
			delete item;
			return false;
		}

		if(depotTransfer)
		{
			for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it)
				parent->__addThing(NULL, (*it));

			container->itemlist.clear();
		}
	}

	delete item;
	return true;
}

bool IOMapSerialize::saveTile(PropWriteStream& stream, const Tile* tile)
{
	int32_t tileCount = tile->getThingCount();
	if(!tileCount)
		return true;

	std::vector<Item*> items;
	Item* item = NULL;
	for(; tileCount > 0; --tileCount)
	{
		if((item = tile->__getThing(tileCount - 1)->getItem()) &&
			(item->isMoveable() || item->forceSerialize()))
			items.push_back(item);
	}

	tileCount = items.size(); //lame, but at least we don't need a new variable
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

	stream.ADD_UCHAR(ATTR_END);
	return true;
}
