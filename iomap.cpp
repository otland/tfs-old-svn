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

#include "iomap.h"
#include "fileloader.h"

#include "map.h"
#include "town.h"
#include "tile.h"
#include "item.h"
#include "container.h"
#include "depot.h"

#include "teleport.h"
#include "beds.h"

#include "game.h"
extern Game g_game;

typedef uint8_t attribute_t;
typedef uint32_t flags_t;

/*
	OTBM_ROOTV2
	|
	|--- OTBM_MAP_DATA
	|	|
	|	|--- OTBM_TILE_AREA
	|	|	|--- OTBM_TILE
	|	|	|--- OTBM_TILE_SQUARE (not implemented)
	|	|	|--- OTBM_TILE_REF (not implemented)
	|	|	|--- OTBM_HOUSETILE
	|	|
	|	|--- OTBM_SPAWNS (not implemented)
	|	|	|--- OTBM_SPAWN_AREA (not implemented)
	|	|	|--- OTBM_MONSTER (not implemented)
	|	|
	|	|--- OTBM_TOWNS
	|	|	|--- OTBM_TOWN
	|	|
	|	|--- OTBM_WAYPOINTS
	|		|--- OTBM_WAYPOINT
	|
	|--- OTBM_ITEM_DEF (not implemented)
*/

Tile* IOMap::createTile(Item*& ground, Item* item, uint16_t px, uint16_t py, uint16_t pz)
{
	Tile* tile = NULL;
	if(ground)
	{
		if((item && item->isBlocking(NULL)) || ground->isBlocking(NULL)) //tile is blocking with possibly some decoration, should be static
			tile = new StaticTile(px, py, pz);
		else //tile is not blocking with possibly multiple items, use dynamic
			tile = new DynamicTile(px, py, pz);

		tile->__internalAddThing(ground);
		if(ground->getDecaying() != DECAYING_TRUE)
		{
			ground->__startDecaying();
			ground->setLoadedFromMap(true);
		}

		ground = NULL;
	}
	else //no ground on this tile, so it will always block
		tile = new StaticTile(px, py, pz);

	return tile;
}

bool IOMap::loadMap(Map* map, const std::string& identifier)
{
	FileLoader f;
	if(!f.openFile(identifier.c_str(), false, true))
	{
		std::stringstream ss;
		ss << "Could not open the file " << identifier << ".";
		setLastErrorString(ss.str());
		return false;
	}

	uint32_t type = 0;
	NODE root = f.getChildNode((NODE)NULL, type);

	PropStream propStream;
	if(!f.getProps(root, propStream))
	{
		setLastErrorString("Could not read root property.");
		return false;
	}

	OTBM_root_header* rootHeader;
	if(!propStream.GET_STRUCT(rootHeader))
	{
		setLastErrorString("Could not read header.");
		return false;
	}

	uint32_t headerVersion = rootHeader->version;
	if(headerVersion <= 0)
	{
		//In otbm version 1 the count variable after splashes/fluidcontainers and stackables
		//are saved as attributes instead, this solves alot of problems with items
		//that is changed (stackable/charges/fluidcontainer/splash) during an update.
		setLastErrorString("This map needs to be upgraded by using the latest map editor version to be able to load correctly.");
		return false;
	}

	if(headerVersion > 2)
	{
		setLastErrorString("Unknown OTBM version detected.");
		return false;
	}

	uint32_t headerMajorItems = rootHeader->majorVersionItems;
	if(headerMajorItems < 3)
	{
		setLastErrorString("This map needs to be upgraded by using the latest map editor version to be able to load correctly.");
		return false;
	}

	if(headerMajorItems > (uint32_t)Items::dwMajorVersion)
	{
		setLastErrorString("The map was saved with a different items.otb version, an upgraded items.otb is required.");
		return false;
	}

	uint32_t headerMinorItems = rootHeader->minorVersionItems;
	if(headerMinorItems < CLIENT_VERSION_810)
	{
		setLastErrorString("This map needs an updated items.otb.");
		return false;
	}

	if(headerMinorItems > (uint32_t)Items::dwMinorVersion)
		setLastErrorString("This map needs an updated items.otb.");

	std::cout << "> Map size: " << rootHeader->width << "x" << rootHeader->height << "." << std::endl;
	map->mapWidth = rootHeader->width;
	map->mapHeight = rootHeader->height;

	NODE nodeMap = f.getChildNode(root, type);
	if(type != OTBM_MAP_DATA)
	{
		setLastErrorString("Could not read data node.");
		return false;
	}

	if(!f.getProps(nodeMap, propStream))
	{
		setLastErrorString("Could not read map data attributes.");
		return false;
	}

	std::string tmp;
	uint8_t attribute;
	while(propStream.GET_UCHAR(attribute))
	{
		switch(attribute)
		{
			case OTBM_ATTR_DESCRIPTION:
			{
				if(!propStream.GET_STRING(tmp))
				{
					setLastErrorString("Invalid description tag.");
					return false;
				}

				map->descriptions.push_back(tmp);
				break;
			}
			case OTBM_ATTR_EXT_SPAWN_FILE:
			{
				if(!propStream.GET_STRING(tmp))
				{
					setLastErrorString("Invalid spawnfile tag.");
					return false;
				}

				map->spawnfile = identifier.substr(0, identifier.rfind('/') + 1);
				map->spawnfile += tmp;
				break;
			}
			case OTBM_ATTR_EXT_HOUSE_FILE:
			{
				if(!propStream.GET_STRING(tmp))
				{
					setLastErrorString("Invalid housefile tag.");
					return false;
				}

				map->housefile = identifier.substr(0, identifier.rfind('/') + 1);
				map->housefile += tmp;
				break;
			}
			default:
			{
				setLastErrorString("Unknown header node.");
				return false;
			}
		}
	}

	std::cout << "> Map descriptions: " << std::endl;
	for(StringVec::iterator it = map->descriptions.begin(); it != map->descriptions.end(); ++it)
		std::cout << "\"" << (*it) << "\"" << std::endl;

	NODE nodeMapData = f.getChildNode(nodeMap, type);
	while(nodeMapData != NO_NODE)
	{
		if(f.getError() != ERROR_NONE)
		{
			setLastErrorString("Invalid map node.");
			return false;
		}

		if(type == OTBM_TILE_AREA)
		{
			if(!f.getProps(nodeMapData, propStream))
			{
				setLastErrorString("Invalid map node.");
				return false;
			}

			OTBM_Destination_coords* area_coord;
			if(!propStream.GET_STRUCT(area_coord))
			{
				setLastErrorString("Invalid map node.");
				return false;
			}

			int32_t base_x = area_coord->_x, base_y = area_coord->_y, base_z = area_coord->_z;
			NODE nodeTile = f.getChildNode(nodeMapData, type);
			while(nodeTile != NO_NODE)
			{
				if(f.getError() != ERROR_NONE)
				{
					setLastErrorString("Could not read node data.");
					return false;
				}

				if(type == OTBM_TILE || type == OTBM_HOUSETILE)
				{
					if(!f.getProps(nodeTile, propStream))
					{
						setLastErrorString("Could not read node data.");
						return false;
					}

					OTBM_Tile_coords* tileCoord;
					if(!propStream.GET_STRUCT(tileCoord))
					{
						setLastErrorString("Could not read tile position.");
						return false;
					}

					Tile* tile = NULL;
					Item* ground = NULL;
					uint32_t tileflags = 0;

					uint16_t px = base_x + tileCoord->_x, py = base_y + tileCoord->_y, pz = base_z;
					House* house = NULL;
					if(type == OTBM_HOUSETILE)
					{
						uint32_t _houseid;
						if(!propStream.GET_ULONG(_houseid))
						{
							std::stringstream ss;
							ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] Could not read house id.";

							setLastErrorString(ss.str());
							return false;
						}

						house = Houses::getInstance()->getHouse(_houseid, true);
						if(!house)
						{
							std::stringstream ss;
							ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] Could not create house id: " << _houseid;

							setLastErrorString(ss.str());
							return false;
						}

						tile = new HouseTile(px, py, pz, house);
						house->addTile(static_cast<HouseTile*>(tile));
					}

					//read tile attributes
					uint8_t attribute;
					while(propStream.GET_UCHAR(attribute))
					{
						switch(attribute)
						{
							case OTBM_ATTR_TILE_FLAGS:
							{
								uint32_t flags;
								if(!propStream.GET_ULONG(flags))
								{
									std::stringstream ss;
									ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] Failed to read tile flags.";

									setLastErrorString(ss.str());
									return false;
								}

								if((flags & TILESTATE_PROTECTIONZONE) == TILESTATE_PROTECTIONZONE)
									tileflags |= TILESTATE_PROTECTIONZONE;
								else if((flags & TILESTATE_NOPVPZONE) == TILESTATE_NOPVPZONE)
									tileflags |= TILESTATE_NOPVPZONE;
								else if((flags & TILESTATE_PVPZONE) == TILESTATE_PVPZONE)
									tileflags |= TILESTATE_PVPZONE;

								if((flags & TILESTATE_NOLOGOUT) == TILESTATE_NOLOGOUT)
									tileflags |= TILESTATE_NOLOGOUT;

								if((flags & TILESTATE_REFRESH) == TILESTATE_REFRESH)
								{
									if(house)
										std::cout << "[x:" << px << ", y:" << py << ", z:" << pz << "] House tile flagged as refreshing!";

									tileflags |= TILESTATE_REFRESH;
								}

								break;
							}

							case OTBM_ATTR_ITEM:
							{
								Item* item = Item::CreateItem(propStream);
								if(!item)
								{
									std::stringstream ss;
									ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] Failed to create item.";

									setLastErrorString(ss.str());
									return false;
								}

								if(house && item->isMoveable())
								{
									std::cout << "[Warning - IOMap::loadMap] Movable item in house: " << house->getId();
									std::cout << ", item type: " << item->getID() << ", at position " << px << "/" << py << "/";
									std::cout << pz << std::endl;

									delete item;
									item = NULL;
								}
								else if(tile)
								{
									tile->__internalAddThing(item);
									item->__startDecaying();
									item->setLoadedFromMap(true);
								}
								else if(item->isGroundTile())
								{
									if(ground)
										delete ground;

									ground = item;
								}
								else
								{
									tile = createTile(ground, item, px, py, pz);
									tile->__internalAddThing(item);

									item->__startDecaying();
									item->setLoadedFromMap(true);
								}

								break;
							}

							default:
							{
								std::stringstream ss;
								ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] Unknown tile attribute.";

								setLastErrorString(ss.str());
								return false;
							}
						}
					}

					NODE nodeItem = f.getChildNode(nodeTile, type);
					while(nodeItem)
					{
						if(type == OTBM_ITEM)
						{
							PropStream propStream;
							f.getProps(nodeItem, propStream);

							Item* item = Item::CreateItem(propStream);
							if(!item)
							{
								std::stringstream ss;
								ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] Failed to create item.";

								setLastErrorString(ss.str());
								return false;
							}

							if(item->unserializeItemNode(f, nodeItem, propStream))
							{
								if(house && item->isMoveable())
								{
									std::cout << "[Warning - IOMap::loadMap] Movable item in house: ";
									std::cout << house->getId() << ", item type: " << item->getID();
									std::cout << ", pos " << px << "/" << py << "/" << pz << std::endl;

									delete item;
									item = NULL;
								}
								else if(tile)
								{
									tile->__internalAddThing(item);
									item->__startDecaying();
									item->setLoadedFromMap(true);
								}
								else if(item->isGroundTile())
								{
									if(ground)
										delete ground;

									ground = item;
								}
								else
								{
									tile = createTile(ground, item, px, py, pz);
									tile->__internalAddThing(item);

									item->__startDecaying();
									item->setLoadedFromMap(true);
								}
							}
							else
							{
								std::stringstream ss;
								ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] Failed to load item " << item->getID() << ".";
								setLastErrorString(ss.str());

								delete item;
								item = NULL;
								return false;
							}
						}
						else
						{
							std::stringstream ss;
							ss << "[x:" << px << ", y:" << py << ", z:" << pz << "] Unknown node type.";
							setLastErrorString(ss.str());
						}

						nodeItem = f.getNextNode(nodeItem, type);
					}

					if(!tile)
						tile = createTile(ground, NULL, px, py, pz);

					tile->setFlag((tileflags_t)tileflags);
					map->setTile(px, py, pz, tile);
				}
				else
				{
					setLastErrorString("Unknown tile node.");
					return false;
				}

				nodeTile = f.getNextNode(nodeTile, type);
			}
		}
		else if(type == OTBM_TOWNS)
		{
			NODE nodeTown = f.getChildNode(nodeMapData, type);
			while(nodeTown != NO_NODE)
			{
				if(type == OTBM_TOWN)
				{
					if(!f.getProps(nodeTown, propStream))
					{
						setLastErrorString("Could not read town data.");
						return false;
					}

					uint32_t townId = 0;
					if(!propStream.GET_ULONG(townId))
					{
						setLastErrorString("Could not read town id.");
						return false;
					}

					Town* town = Towns::getInstance()->getTown(townId);
					if(!town)
					{
						town = new Town(townId);
						Towns::getInstance()->addTown(townId, town);
					}

					std::string townName;
					if(!propStream.GET_STRING(townName))
					{
						setLastErrorString("Could not read town name.");
						return false;
					}

					town->setName(townName);
					OTBM_Destination_coords *townCoords;
					if(!propStream.GET_STRUCT(townCoords))
					{
						setLastErrorString("Could not read town coordinates.");
						return false;
					}

					town->setPosition(Position(townCoords->_x, townCoords->_y, townCoords->_z));
				}
				else
				{
					setLastErrorString("Unknown town node.");
					return false;
				}

				nodeTown = f.getNextNode(nodeTown, type);
			}
		}
		else if(type == OTBM_WAYPOINTS && headerVersion > 1)
		{
			NODE nodeWaypoint = f.getChildNode(nodeMapData, type);
			while(nodeWaypoint != NO_NODE)
			{
				if(type == OTBM_WAYPOINT)
				{
					if(!f.getProps(nodeWaypoint, propStream))
					{
						setLastErrorString("Could not read waypoint data.");
						return false;
					}

					std::string name;
					if(!propStream.GET_STRING(name))
					{
						setLastErrorString("Could not read waypoint name.");
						return false;
					}

					OTBM_Destination_coords* waypoint_coords;
					if(!propStream.GET_STRUCT(waypoint_coords))
					{
						setLastErrorString("Could not read waypoint coordinates.");
						return false;
					}

					map->waypoints.addWaypoint(WaypointPtr(new Waypoint(name,
						Position(waypoint_coords->_x, waypoint_coords->_y, waypoint_coords->_z))));
				}
				else
				{
					setLastErrorString("Unknown waypoint node.");
					return false;
				}

				nodeWaypoint = f.getNextNode(nodeWaypoint, type);
			}
		}
		else
		{
			setLastErrorString("Unknown map node.");
			return false;
		}

		nodeMapData = f.getNextNode(nodeMapData, type);
	}

	return true;
}
