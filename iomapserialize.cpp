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
#include "luascript.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;

bool IOMapSerialize::loadMap(Map* map)
{
	Database* db = Database::getInstance();
	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		//load tile
		for(HouseTileList::iterator tit = it->second->getHouseTileBegin(); tit != it->second->getHouseTileEnd(); ++tit)
			loadTile(*db, *tit);
	}

	return true;
}

bool IOMapSerialize::saveMap(Map* map)
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

bool IOMapSerialize::saveTile(Database* db, uint32_t tileId, const Tile* tile)
{
	const Position& tilePosition = tile->getPosition();
	uint32_t tileCount = tile->getThingCount();
	if(!tileCount)
		return true;

	DBQuery query;
	query << "INSERT INTO `tiles` (`id`, `world_id`, `x`, `y`, `z`) VALUES " << "(" << tileId << ", " << g_config.getNumber(ConfigManager::WORLD_ID)
		<< ", " << tilePosition.x << ", " << tilePosition.y << ", " << tilePosition.z << ")";
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

		if(item->isNotMoveable() && !item->getDoor() && !item->getBed() && (!item->getContainer() || item->getContainer()->size() == 0) && !item->canWriteText())
			continue;

		uint32_t attributesSize;
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

			uint32_t attributesSize;
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

bool IOMapSerialize::loadTile(Database& db, Tile* tile)
{
	typedef std::map<int32_t, std::pair<Item*, int32_t> > ItemMap;
	ItemMap itemMap;

	const Position& tilePos = tile->getPosition();

	DBQuery query;
	query.str("");
	query << "SELECT `tiles`.`id` FROM `tiles` WHERE `x` = " << tilePos.x << " AND `y` = " << tilePos.y << " AND `z` = " << tilePos.z << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);

	DBResult* result;
	if(!(result = db.storeQuery(query.str())))
		return false;

	int32_t tileId = result->getDataInt("id");
	db.freeResult(result);
	query.str("");
	query << "SELECT * FROM `tile_items` WHERE `tile_id` = " << tileId << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " ORDER BY `sid` DESC;";
	if((result = db.storeQuery(query.str())))
	{
		Item* item = NULL;
		do
		{
			int32_t sid = result->getDataInt("sid");
			int32_t pid = result->getDataInt("pid");
			int32_t type = result->getDataInt("itemtype");
			int32_t count = result->getDataInt("count");

			item = NULL;

			uint64_t attrSize = 0;
			const char* attr = result->getDataStream("attributes", attrSize);
			PropStream propStream;
			propStream.init(attr, attrSize);

			const ItemType& iType = Item::items[type];
			if(iType.moveable || /* or object in a container*/ pid != 0)
			{
				//create a new item
				item = Item::CreateItem(type, count);
				if(item)
				{
					if(!item->unserializeAttr(propStream))
						std::cout << "WARNING: Serialize error in IOMapSerialize::loadTile() [1]" << std::endl;

					if(pid == 0)
					{
						tile->__internalAddThing(item);
						item->__startDecaying();
					}
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

					if(findItem->getID() == type || (iType.isDoor() && findItem->getDoor()) ||
						(iType.isBed() && findItem->getBed()))
					{
						item = findItem;
						break;
					}
				}
			}

			if(item)
			{
				if(!item->unserializeAttr(propStream))
					std::cout << "WARNING: Serialize error in IOMapSerialize::loadTile() [0]" << std::endl;

				item = g_game.transformItem(item, type);

				std::pair<Item*, int32_t> myPair(item, pid);
				itemMap[sid] = myPair;
			}
			else
				std::cout << "WARNING: IOMapSerialize::loadTile() - NULL item at " << tile->getPosition() << " (type = " << type << ", sid = " << sid << ", pid = " << pid << ")." << std::endl;
		}
		while(result->next());
		db.freeResult(result);
	}

	ItemMap::reverse_iterator it;
	ItemMap::iterator it2;
	for(it = itemMap.rbegin(); it != itemMap.rend(); ++it)
	{
		Item* item = it->second.first;
		int32_t pid = it->second.second;
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
	db->freeResult(result);

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it)
	{
		query.str("");
		House* house = it->second;
		if(house->getHouseOwner() != 0 && house->getHouseId() != 0)
		{
			query << "SELECT `listid`, `list` FROM `house_lists` WHERE `house_id` = " << house->getHouseId() << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
			if((result = db->storeQuery(query.str())))
			{
				do
					house->setAccessList(result->getDataInt("listid"), result->getDataString("list"));
				while(result->next());
				db->freeResult(result);
			}
		}
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
