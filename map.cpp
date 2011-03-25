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
#include <iomanip>

#include <boost/config.hpp>
#include <boost/bind.hpp>

#include "iomap.h"
#include "map.h"
#include "tile.h"

#include "creature.h"
#include "player.h"
#include "combat.h"

#include "iomapserialize.h"
#include "items.h"

#include "game.h"

extern Game g_game;

Map::Map()
{
	mapWidth = 0;
	mapHeight = 0;
}

bool Map::loadMap(const std::string& identifier)
{
	int64_t start = OTSYS_TIME();
	IOMap* loader = new IOMap();
	if(!loader->loadMap(this, identifier))
	{
		std::clog << "> FATAL: OTBM Loader - " << loader->getLastErrorString() << std::endl;
		return false;
	}

	std::clog << "> Map loading time: " << (OTSYS_TIME() - start) / (1000.) << " seconds." << std::endl;
	start = OTSYS_TIME();
	if(!loader->loadSpawns(this))
		std::clog << "> WARNING: Could not load spawn data." << std::endl;

	if(!loader->loadHouses(this))
		std::clog << "> WARNING: Could not load house data." << std::endl;

	delete loader;
	std::clog << "> Data parsing time: " << (OTSYS_TIME() - start) / (1000.) << " seconds." << std::endl;
	start = OTSYS_TIME();

	IOMapSerialize::getInstance()->updateHouses();
	IOMapSerialize::getInstance()->updateAuctions();
	std::clog << "> Houses synchronization time: " << (OTSYS_TIME() - start) / (1000.) << " seconds." << std::endl;

	start = OTSYS_TIME();
	IOMapSerialize::getInstance()->loadHouses();
	IOMapSerialize::getInstance()->loadMap(this);

	std::clog << "> Content unserialization time: " << (OTSYS_TIME() - start) / (1000.) << " seconds." << std::endl;
	return true;
}

bool Map::saveMap()
{
	IOMapSerialize* IOLoader = IOMapSerialize::getInstance();
	bool saved = false;
	for(uint32_t tries = 0; tries < 3; ++tries)
	{
		if(!IOLoader->saveHouses())
			continue;

		saved = true;
		break;
	}

	if(!saved)
		return false;

	saved = false;
	for(uint32_t tries = 0; tries < 3; ++tries)
	{
		if(!IOLoader->saveMap(this))
			continue;

		saved = true;
		break;
	}

	return saved;
}

Tile* Map::getTile(int32_t x, int32_t y, int32_t z)
{
	if(x < 0 || x > 0xFFFF || y < 0 || y > 0xFFFF || z < 0 || z >= MAP_MAX_LAYERS)
		return NULL;

	QTreeLeafNode* leaf = QTreeNode::getLeafStatic(&root, x, y);
	if(!leaf)
		return NULL;

	Floor* floor = leaf->getFloor(z);
	if(!floor)
		return NULL;

	return floor->tiles[x & FLOOR_MASK][y & FLOOR_MASK];
}

void Map::setTile(uint16_t x, uint16_t y, uint16_t z, Tile* newTile)
{
	if(z >= MAP_MAX_LAYERS)
	{
		std::clog << "[Error - Map::setTile]: Attempt to set tile on invalid Z coordinate - " << z << "!" << std::endl;
		return;
	}

	QTreeLeafNode::newLeaf = false;
	QTreeLeafNode* leaf = root.createLeaf(x, y, 15);
	if(QTreeLeafNode::newLeaf)
	{
		//update north
		QTreeLeafNode* northLeaf = root.getLeaf(x, y - FLOOR_SIZE);
		if(northLeaf)
			northLeaf->m_leafS = leaf;

		//update west leaf
		QTreeLeafNode* westLeaf = root.getLeaf(x - FLOOR_SIZE, y);
		if(westLeaf)
			westLeaf->m_leafE = leaf;

		//update south
		QTreeLeafNode* southLeaf = root.getLeaf(x, y + FLOOR_SIZE);
		if(southLeaf)
			leaf->m_leafS = southLeaf;

		//update east
		QTreeLeafNode* eastLeaf = root.getLeaf(x + FLOOR_SIZE, y);
		if(eastLeaf)
			leaf->m_leafE = eastLeaf;
	}

	uint32_t offsetX = x & FLOOR_MASK, offsetY = y & FLOOR_MASK;
	Floor* floor = leaf->createFloor(z);
	if(!floor->tiles[offsetX][offsetY])
	{
		floor->tiles[offsetX][offsetY] = newTile;
		newTile->qt_node = leaf;
	}
	else
		std::clog << "[Error - Map::setTile] Tile already exists - pos " << offsetX << "/" << offsetY << "/" << z << std::endl;

	if(newTile->hasFlag(TILESTATE_REFRESH))
	{
		RefreshBlock_t rb;
		if(TileItemVector* tileItems = newTile->getItemList())
		{
			for(ItemVector::iterator it = tileItems->getBeginDownItem(); it != tileItems->getEndDownItem(); ++it)
				rb.list.push_back((*it)->clone());
		}

		rb.lastRefresh = OTSYS_TIME();
		g_game.addRefreshTile(newTile, rb);
	}
}

bool Map::placeCreature(const Position& centerPos, Creature* creature, bool extendedPos /*= false*/, bool forced /*= false*/)
{
	bool foundTile = false, placeInPz = false;
	Tile* tile = getTile(centerPos);
	if(tile)
	{
		placeInPz = tile->hasFlag(TILESTATE_PROTECTIONZONE);
		uint32_t flags = FLAG_IGNOREBLOCKITEM;
		if(creature->isAccountManager())
			flags |= FLAG_IGNOREBLOCKCREATURE;

		ReturnValue ret = tile->__queryAdd(0, creature, 1, flags);
		if(forced || ret == RET_NOERROR || ret == RET_PLAYERISNOTINVITED)
			foundTile = true;
	}

	size_t shufflePos = 0;
	PairVector relList;
	if(extendedPos)
	{
		shufflePos = 8;
		relList.push_back(PositionPair(-2, 0));
		relList.push_back(PositionPair(0, -2));
		relList.push_back(PositionPair(0, 2));
		relList.push_back(PositionPair(2, 0));
		std::random_shuffle(relList.begin(), relList.end());
	}

	relList.push_back(PositionPair(-1, -1));
	relList.push_back(PositionPair(-1, 0));
	relList.push_back(PositionPair(-1, 1));
	relList.push_back(PositionPair(0, -1));
	relList.push_back(PositionPair(0, 1));
	relList.push_back(PositionPair(1, -1));
	relList.push_back(PositionPair(1, 0));
	relList.push_back(PositionPair(1, 1));
	std::random_shuffle(relList.begin() + shufflePos, relList.end());

	uint32_t radius = 1;
	Position tryPos;
	for(uint32_t n = 1; n <= radius && !foundTile; ++n)
	{
		for(PairVector::iterator it = relList.begin(); it != relList.end() && !foundTile; ++it)
		{
			int32_t dx = it->first * n, dy = it->second * n;
			tryPos = centerPos;

			tryPos.x = tryPos.x + dx;
			tryPos.y = tryPos.y + dy;
			if(!(tile = getTile(tryPos)) || (placeInPz && !tile->hasFlag(TILESTATE_PROTECTIONZONE)))
				continue;

			if(tile->__queryAdd(0, creature, 1, 0) == RET_NOERROR)
			{
				if(!extendedPos)
				{
					foundTile = true;
					break;
				}

				if(isSightClear(centerPos, tryPos, false))
				{
					foundTile = true;
					break;
				}
			}
		}
	}

	if(!foundTile)
		return false;

	int32_t index = 0;
	uint32_t flags = 0;

	Item* toItem = NULL;
	if(Cylinder* toCylinder = tile->__queryDestination(index, creature, &toItem, flags))
	{
		toCylinder->__internalAddThing(creature);
		if(Tile* toTile = toCylinder->getTile())
			toTile->qt_node->addCreature(creature);
	}

	return true;
}

bool Map::removeCreature(Creature* creature)
{
	Tile* tile = creature->getTile();
	if(!tile)
		return false;

	tile->qt_node->removeCreature(creature);
	tile->__removeThing(creature, 0);
	return true;
}

void Map::getSpectatorsInternal(SpectatorVec& list, const Position& centerPos, bool checkForDuplicate,
	int32_t minRangeX, int32_t maxRangeX, int32_t minRangeY, int32_t maxRangeY,
	int32_t minRangeZ, int32_t maxRangeZ)
{
	int32_t minoffset = centerPos.z - maxRangeZ, maxoffset = centerPos.z - minRangeZ,
		x1 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.x + minRangeX + minoffset))),
		y1 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.y + minRangeY + minoffset))),
		x2 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.x + maxRangeX + maxoffset))),
		y2 = std::min((int32_t)0xFFFF, std::max((int32_t)0, (centerPos.y + maxRangeY + maxoffset))),
		startx1 = x1 - (x1 % FLOOR_SIZE), starty1 = y1 - (y1 % FLOOR_SIZE),
		endx2 = x2 - (x2 % FLOOR_SIZE), endy2 = y2 - (y2 % FLOOR_SIZE);

	QTreeLeafNode* startLeaf = getLeaf(startx1, starty1);
	QTreeLeafNode* leafS = startLeaf;

	QTreeLeafNode* leafE;
	for(int32_t ny = starty1; ny <= endy2; ny += FLOOR_SIZE)
	{
		leafE = leafS;
		for(int32_t nx = startx1; nx <= endx2; nx += FLOOR_SIZE)
		{
			if(leafE)
			{
				CreatureVector& nodeList = leafE->creatureList;
				CreatureVector::const_iterator it = nodeList.begin();
				if(it != nodeList.end())
				{
					do
					{
						Creature* creature = (*it);
						const Position& pos = creature->getPosition();

						int32_t offsetZ = centerPos.z - pos.z;
						if(pos.z < minRangeZ || pos.z > maxRangeZ)
							continue;

						if(pos.y < (centerPos.y + minRangeY + offsetZ) || pos.y > (centerPos.y + maxRangeY + offsetZ))
							continue;

						if(pos.x < (centerPos.x + minRangeX + offsetZ) || pos.x > (centerPos.x + maxRangeX + offsetZ))
							continue;

						if(!checkForDuplicate || std::find(list.begin(), list.end(), creature) == list.end())
							list.push_back(creature);
					}
					while(++it != nodeList.end());
				}

				leafE = leafE->stepEast();
			}
			else
				leafE = getLeaf(nx + FLOOR_SIZE, ny);
		}

		if(leafS)
			leafS = leafS->stepSouth();
		else
			leafS = getLeaf(startx1, ny + FLOOR_SIZE);
	}
}

void Map::getSpectators(SpectatorVec& list, const Position& centerPos, bool checkforduplicate /*= false*/, bool multifloor /*= false*/,
	int32_t minRangeX /*= 0*/, int32_t maxRangeX /*= 0*/, int32_t minRangeY /*= 0*/, int32_t maxRangeY /*= 0*/)
{
	if(centerPos.z >= MAP_MAX_LAYERS)
		return;

	bool foundCache = false, cacheResult = false;
	if(!minRangeX && !maxRangeX && !minRangeY && !maxRangeY && multifloor && !checkforduplicate)
	{
		SpectatorCache::iterator it = spectatorCache.find(centerPos);
		if(it != spectatorCache.end())
		{
			list = *it->second;
			foundCache = true;
		}
		else
			cacheResult = true;
	}

	if(!foundCache)
	{
		minRangeX = (!minRangeX ? -maxViewportX : -minRangeX);
		maxRangeX = (!maxRangeX ? maxViewportX : maxRangeX);
		minRangeY = (!minRangeY ? -maxViewportY : -minRangeY);
		maxRangeY = (!maxRangeY ? maxViewportY : maxRangeY);

		int32_t minRangeZ, maxRangeZ;
		if(multifloor)
		{
			if(centerPos.z > 7)
			{
				//underground, 8->15
				minRangeZ = std::max(centerPos.z - 2, 0);
				maxRangeZ = std::min(centerPos.z + 2, MAP_MAX_LAYERS - 1);
			}
			//above ground
			else if(centerPos.z == 6)
			{
				minRangeZ = 0;
				maxRangeZ = 8;
			}
			else if(centerPos.z == 7)
			{
				minRangeZ = 0;
				maxRangeZ = 9;
			}
			else
			{
				minRangeZ = 0;
				maxRangeZ = 7;
			}
		}
		else
		{
			minRangeZ = centerPos.z;
			maxRangeZ = centerPos.z;
		}

		getSpectatorsInternal(list, centerPos, true, minRangeX,
			maxRangeX, minRangeY, maxRangeY, minRangeZ, maxRangeZ);

		if(cacheResult)
			spectatorCache[centerPos].reset(new SpectatorVec(list));
	}
}

const SpectatorVec& Map::getSpectators(const Position& centerPos)
{
	if(centerPos.z >= MAP_MAX_LAYERS)
	{
		boost::shared_ptr<SpectatorVec> p(new SpectatorVec());
		SpectatorVec& list = *p;
		return list;
	}

	SpectatorCache::iterator it = spectatorCache.find(centerPos);
	if(it != spectatorCache.end())
		return *it->second;

	boost::shared_ptr<SpectatorVec> p(new SpectatorVec());
	spectatorCache[centerPos] = p;
	SpectatorVec& list = *p;

	int32_t minRangeX = -maxViewportX, maxRangeX = maxViewportX, minRangeY = -maxViewportY,
		maxRangeY = maxViewportY, minRangeZ, maxRangeZ;
	if(centerPos.z > 7)
	{
		//underground, 8->15
		minRangeZ = std::max(centerPos.z - 2, 0);
		maxRangeZ = std::min(centerPos.z + 2, MAP_MAX_LAYERS - 1);
	}
	//above ground
	else if(centerPos.z == 6)
	{
		minRangeZ = 0;
		maxRangeZ = 8;
	}
	else if(centerPos.z == 7)
	{
		minRangeZ = 0;
		maxRangeZ = 9;
	}
	else
	{
		minRangeZ = 0;
		maxRangeZ = 7;
	}

	getSpectatorsInternal(list, centerPos, false, minRangeX, maxRangeX, minRangeY, maxRangeY, minRangeZ, maxRangeZ);
	return list;
}

bool Map::canThrowObjectTo(const Position& fromPos, const Position& toPos, bool checkLineOfSight /*= true*/,
	int32_t rangex /*= Map::maxClientViewportX*/, int32_t rangey /*= Map::maxClientViewportY*/)
{
	//z checks
	//underground 8->15
	//ground level and above 7->0
	if((fromPos.z >= 8 && toPos.z < 8) || (toPos.z >= 8 &&
		fromPos.z < 8) || fromPos.z - fromPos.z > 2)
		return false;

	int32_t deltax = std::abs(fromPos.x - toPos.x), deltay = std::abs(
		fromPos.y - toPos.y), deltaz = std::abs(fromPos.z - toPos.z);
	if(deltax - deltaz > rangex || deltay - deltaz > rangey)
		return false;

	if(!checkLineOfSight)
		return true;

	return isSightClear(fromPos, toPos, false);
}

bool Map::checkSightLine(const Position& fromPos, const Position& toPos) const
{
	Position start = fromPos;
	Position end = toPos;

	int32_t x, y, z, dx = std::abs(start.x - end.x), dy = std::abs(start.y - end.y),
		dz = std::abs(start.z - end.z), sx, sy, sz, ey, ez, max = dx, dir = 0;
	if(dy > max)
	{
		max = dy;
		dir = 1;
	}

	if(dz > max)
	{
		max = dz;
		dir = 2;
	}

	switch(dir)
	{
		case 1:
			//x -> y
			//y -> x
			//z -> z
			std::swap(start.x, start.y);
			std::swap(end.x, end.y);
			std::swap(dx, dy);
			break;
		case 2:
			//x -> z
			//y -> y
			//z -> x
			std::swap(start.x, start.z);
			std::swap(end.x, end.z);
			std::swap(dx, dz);
			break;
		default:
			//x -> x
			//y -> y
			//z -> z
			break;
	}

	sx = ((start.x < end.x) ? 1 : -1);
	sy = ((start.y < end.y) ? 1 : -1);
	sz = ((start.z < end.z) ? 1 : -1);

	ey = ez = 0;
	x = start.x;
	y = start.y;
	z = start.z;

	int32_t lastrx = 0, lastry = 0, lastrz = 0;
	for(; x != end.x + sx; x += sx)
	{
		int32_t rx, ry, rz;
		switch(dir)
		{
			case 1:
				rx = y; ry = x; rz = z;
				break;
			case 2:
				rx = z; ry = y; rz = x;
				break;
			default:
				rx = x; ry = y; rz = z;
				break;
		}

		if(!lastrx && !lastry && !lastrz)
		{
			lastrx = rx;
			lastry = ry;
			lastrz = rz;
		}

		if(lastrz != rz || ((toPos.x != rx || toPos.y != ry || toPos.z != rz) && (fromPos.x != rx || fromPos.y != ry || fromPos.z != rz)))
		{
			if(lastrz != rz && const_cast<Map*>(this)->getTile(lastrx, lastry, std::min(lastrz, rz)))
				return false;

			lastrx = rx; lastry = ry; lastrz = rz;
			const Tile* tile = const_cast<Map*>(this)->getTile(rx, ry, rz);
			if(tile && tile->hasProperty(BLOCKPROJECTILE))
				return false;
		}

		ey += dy;
		ez += dz;
		if(2 * ey >= dx)
		{
			y += sy;
			ey -= dx;
		}

		if(2 * ez >= dx)
		{
			z += sz;
			ez -= dx;
		}
	}

	return true;
}

bool Map::isSightClear(const Position& fromPos, const Position& toPos, bool floorCheck) const
{
	if(floorCheck && fromPos.z != toPos.z)
		return false;

	// Cast two converging rays and see if either yields a result.
	return checkSightLine(fromPos, toPos) || checkSightLine(toPos, fromPos);
}

const Tile* Map::canWalkTo(const Creature* creature, const Position& pos)
{
	switch(creature->getWalkCache(pos))
	{
		case 0:
			return NULL;
		case 1:
			return getTile(pos);
		default:
			break;
	}

	//used for none-cached tiles
	Tile* tile = getTile(pos);
	if(creature->getTile() != tile && (!tile || tile->__queryAdd(0, creature, 1,
		FLAG_PATHFINDING | FLAG_IGNOREFIELDDAMAGE) != RET_NOERROR))
		return NULL;

	return tile;
}

bool Map::getPathTo(const Creature* creature, const Position& destPos,
	std::list<Direction>& listDir, int32_t maxSearchDist /*= -1*/)
{
	if(!canWalkTo(creature, destPos))
		return false;

	Position startPos = destPos;
	Position endPos = creature->getPosition();

	listDir.clear();
	if(startPos.z != endPos.z)
		return false;

	AStarNodes nodes;
	AStarNode* startNode = nodes.createOpenNode();

	startNode->x = startPos.x;
	startNode->y = startPos.y;

	startNode->g = 0;
	startNode->h = nodes.getEstimatedDistance(startPos.x, startPos.y, endPos.x, endPos.y);

	startNode->f = startNode->g + startNode->h;
	startNode->parent = NULL;

	Position pos;
	pos.z = startPos.z;
	static int32_t neighbourOrderList[8][2] =
	{
		{-1, 0},
		{0, 1},
		{1, 0},
		{0, -1},

		//diagonal
		{-1, -1},
		{1, -1},
		{1, 1},
		{-1, 1},
	};

	AStarNode* found = NULL;
	AStarNode* n = NULL;

	const Tile* tile = NULL;
	while(maxSearchDist != -1 || nodes.countClosedNodes() < 100)
	{
		if(!(n = nodes.getBestNode()))
		{
			listDir.clear();
			return false; //no path found
		}

		if(n->x == endPos.x && n->y == endPos.y)
		{
			found = n;
			break;
		}

		for(uint8_t i = 0; i < 8; ++i)
		{
			pos.x = n->x + neighbourOrderList[i][0];
			pos.y = n->y + neighbourOrderList[i][1];

			bool outOfRange = false;
			if(maxSearchDist != -1 && (std::abs(endPos.x - pos.x) > maxSearchDist ||
				std::abs(endPos.y - pos.y) > maxSearchDist))
				outOfRange = true;

			if(!outOfRange && (tile = canWalkTo(creature, pos)))
			{
				//The cost (g) for this neighbour
				int32_t cost = nodes.getMapWalkCost(creature, n, tile, pos),
					extraCost = nodes.getTileWalkCost(creature, tile),
					newg = n->g + cost + extraCost;
				//Check if the node is already in the closed/open list
				//If it exists and the nodes already on them has a lower cost (g) then we can ignore this neighbour node
				AStarNode* neighbourNode = nodes.getNodeInList(pos.x, pos.y);
				if(neighbourNode)
				{
					if(neighbourNode->g <= newg) //The node on the closed/open list is cheaper than this one
						continue;

					nodes.openNode(neighbourNode);
				}
				else if(!(neighbourNode = nodes.createOpenNode())) //Does not exist in the open/closed list, create a new node
				{
					//seems we ran out of nodes
					listDir.clear();
					return false;
				}

				//This node is the best node so far with this state
				neighbourNode->x = pos.x;
				neighbourNode->y = pos.y;

				neighbourNode->g = newg;
				neighbourNode->h = nodes.getEstimatedDistance(neighbourNode->x, neighbourNode->y, endPos.x, endPos.y);

				neighbourNode->f = neighbourNode->g + neighbourNode->h;
				neighbourNode->parent = n;
			}
		}

		nodes.closeNode(n);
	}

	int32_t prevx = endPos.x, prevy = endPos.y, dx, dy;
	while(found)
	{
		pos.x = found->x;
		pos.y = found->y;

		dx = pos.x - prevx;
		dy = pos.y - prevy;

		prevx = pos.x;
		prevy = pos.y;

		found = found->parent;
		if(dx == -1 && dy == -1)
			listDir.push_back(NORTHWEST);
		else if(dx == 1 && dy == -1)
			listDir.push_back(NORTHEAST);
		else if(dx == -1 && dy == 1)
			listDir.push_back(SOUTHWEST);
		else if(dx == 1 && dy == 1)
			listDir.push_back(SOUTHEAST);
		else if(dx == -1)
			listDir.push_back(WEST);
		else if(dx == 1)
			listDir.push_back(EAST);
		else if(dy == -1)
			listDir.push_back(NORTH);
		else if(dy == 1)
			listDir.push_back(SOUTH);
	}

	return !listDir.empty();
}

bool Map::getPathMatching(const Creature* creature, std::list<Direction>& dirList,
	const FrozenPathingConditionCall& pathCondition, const FindPathParams& fpp)
{
	Position startPos = creature->getPosition();
	Position endPos;

	AStarNodes nodes;
	AStarNode* startNode = nodes.createOpenNode();

	startNode->x = startPos.x;
	startNode->y = startPos.y;

	startNode->f = 0;
	startNode->parent = NULL;

	dirList.clear();
	int32_t bestMatch = 0;

	Position pos;
	pos.z = startPos.z;
	static int32_t neighbourOrderList[8][2] =
	{
		{-1, 0},
		{0, 1},
		{1, 0},
		{0, -1},

		//diagonal
		{-1, -1},
		{1, -1},
		{1, 1},
		{-1, 1},
	};

	AStarNode* found = NULL;
	AStarNode* n = NULL;

	const Tile* tile = NULL;
	while(fpp.maxSearchDist != -1 || nodes.countClosedNodes() < fpp.maxClosedNodes)
	{
		if(!(n = nodes.getBestNode()))
		{
			if(found) //not quite what we want, but we found something
				break;

			dirList.clear();
			return false; //no path found
		}

		if(pathCondition(startPos, Position(n->x, n->y, startPos.z), fpp, bestMatch))
		{
			found = n;
			endPos = Position(n->x, n->y, startPos.z);
			if(!bestMatch)
				break;
		}

		int32_t dirCount = (fpp.allowDiagonal ? 8 : 4);
		for(int32_t i = 0; i < dirCount; ++i)
		{
			pos.x = n->x + neighbourOrderList[i][0];
			pos.y = n->y + neighbourOrderList[i][1];

			bool inRange = true;
			if(fpp.maxSearchDist != -1 && (std::abs(startPos.x - pos.x) > fpp.maxSearchDist ||
				std::abs(startPos.y - pos.y) > fpp.maxSearchDist))
				inRange = false;

			if(fpp.keepDistance)
			{
				if(!pathCondition.isInRange(startPos, pos, fpp))
					inRange = false;
			}

			if(inRange && (tile = canWalkTo(creature, pos)))
			{
				//The cost (g) for this neighbour
				int32_t cost = nodes.getMapWalkCost(creature, n, tile, pos),
					extraCost = nodes.getTileWalkCost(creature, tile),
					newf = n->f + cost + extraCost;

				//Check if the node is already in the closed/open list
				//If it exists and the nodes already on them has a lower cost (g) then we can ignore this neighbour node
				AStarNode* neighbourNode = nodes.getNodeInList(pos.x, pos.y);
				if(neighbourNode)
				{
					if(neighbourNode->f <= newf) //The node on the closed/open list is cheaper than this one
						continue;

					nodes.openNode(neighbourNode);
				}
				else if(!(neighbourNode = nodes.createOpenNode())) //Does not exist in the open/closed list, create a new node
				{
					if(found) //not quite what we want, but we found something
						break;

					//seems we ran out of nodes
					dirList.clear();
					return false;
				}

				//This node is the best node so far with this state
				neighbourNode->x = pos.x;
				neighbourNode->y = pos.y;

				neighbourNode->parent = n;
				neighbourNode->f = newf;
			}
		}

		nodes.closeNode(n);
	}

	if(!found)
		return false;

	int32_t prevx = endPos.x, prevy = endPos.y, dx, dy;
	found = found->parent;
	while(found)
	{
		pos.x = found->x;
		pos.y = found->y;

		dx = pos.x - prevx;
		dy = pos.y - prevy;

		prevx = pos.x;
		prevy = pos.y;

		found = found->parent;
		if(dx == 1 && dy == 1)
			dirList.push_front(NORTHWEST);
		else if(dx == -1 && dy == 1)
			dirList.push_front(NORTHEAST);
		else if(dx == 1 && dy == -1)
			dirList.push_front(SOUTHWEST);
		else if(dx == -1 && dy == -1)
			dirList.push_front(SOUTHEAST);
		else if(dx == 1)
			dirList.push_front(WEST);
		else if(dx == -1)
			dirList.push_front(EAST);
		else if(dy == 1)
			dirList.push_front(NORTH);
		else if(dy == -1)
			dirList.push_front(SOUTH);
	}

	return true;
}

//*********** AStarNodes *************

AStarNodes::AStarNodes()
{
	curNode = 0;
	openNodes.reset();
}

AStarNode* AStarNodes::createOpenNode()
{
	if(curNode >= MAX_NODES)
		return NULL;

	uint32_t retNode = curNode;
	curNode++;

	openNodes[retNode] = 1;
	return &nodes[retNode];
}

AStarNode* AStarNodes::getBestNode()
{
	if(!curNode)
		return NULL;

	int32_t bestNodeF = 100000;
	uint32_t bestNode = 0;

	bool found = false;
	for(uint32_t i = 0; i < curNode; i++)
	{
		if(nodes[i].f < bestNodeF && openNodes[i] == 1)
		{
			found = true;
			bestNodeF = nodes[i].f;
			bestNode = i;
		}
	}

	if(found)
		return &nodes[bestNode];

	return NULL;
}

void AStarNodes::closeNode(AStarNode* node)
{
	uint32_t pos = GET_NODE_INDEX(node);
	if(pos < MAX_NODES)
	{
		openNodes[pos] = 0;
		return;
	}

	assert(pos >= MAX_NODES);
	std::clog << "AStarNodes. trying to close node out of range" << std::endl;
	return;
}

void AStarNodes::openNode(AStarNode* node)
{
	uint32_t pos = GET_NODE_INDEX(node);
	if(pos < MAX_NODES)
	{
		openNodes[pos] = 1;
		return;
	}

	assert(pos >= MAX_NODES);
	std::clog << "AStarNodes. trying to open node out of range" << std::endl;
	return;
}

uint32_t AStarNodes::countClosedNodes()
{
	uint32_t counter = 0;
	for(uint32_t i = 0; i < curNode; i++)
	{
		if(!openNodes[i])
			counter++;
	}

	return counter;
}

uint32_t AStarNodes::countOpenNodes()
{
	uint32_t counter = 0;
	for(uint32_t i = 0; i < curNode; i++)
	{
		if(openNodes[i] == 1)
			counter++;
	}

	return counter;
}

bool AStarNodes::isInList(uint16_t x, uint16_t y)
{
	for(uint32_t i = 0; i < curNode; i++)
	{
		if(nodes[i].x == x && nodes[i].y == y)
			return true;
	}

	return false;
}

AStarNode* AStarNodes::getNodeInList(uint16_t x, uint16_t y)
{
	for(uint32_t i = 0; i < curNode; i++)
	{
		if(nodes[i].x == x && nodes[i].y == y)
			return &nodes[i];
	}

	return NULL;
}

int32_t AStarNodes::getMapWalkCost(const Creature*, AStarNode* node,
	const Tile*, const Position& neighbourPos)
{
	if(std::abs(node->x - neighbourPos.x) == std::abs(node->y - neighbourPos.y)) //diagonal movement extra cost
		return MAP_DIAGONALWALKCOST;

	return MAP_NORMALWALKCOST;
}

int32_t AStarNodes::getTileWalkCost(const Creature* creature, const Tile* tile)
{
	int32_t cost = 0;
	if(tile->getTopVisibleCreature(creature)) //destroy creature cost
		cost += MAP_NORMALWALKCOST * 3;

	if(const MagicField* field = tile->getFieldItem())
	{
		if(!creature->isImmune(field->getCombatType()))
			cost += MAP_NORMALWALKCOST * 3;
	}

	return cost;
}

int32_t AStarNodes::getEstimatedDistance(uint16_t x, uint16_t y, uint16_t xGoal, uint16_t yGoal)
{
	int32_t diagonal = std::min(std::abs(x - xGoal), std::abs(y - yGoal));
	return (MAP_DIAGONALWALKCOST * diagonal) + (MAP_NORMALWALKCOST * ((std::abs(
		x - xGoal) + std::abs(y - yGoal)) - (2 * diagonal)));
}

//*********** Floor constructor **************

Floor::Floor()
{
	for(int32_t i = 0; i < FLOOR_SIZE; ++i)
	{
		for(int32_t j = 0; j < FLOOR_SIZE; ++j)
			tiles[i][j] = 0;
	}
}

//**************** QTreeNode **********************
QTreeNode::QTreeNode()
{
	m_isLeaf = false;
	for(int32_t i = 0; i < 4; ++i)
		m_child[i] = NULL;
}

QTreeNode::~QTreeNode()
{
	for(int32_t i = 0; i < 4; ++i)
		delete m_child[i];
}

QTreeLeafNode* QTreeNode::getLeaf(uint16_t x, uint16_t y)
{
	if(isLeaf())
		return static_cast<QTreeLeafNode*>(this);

	uint32_t index = ((x & 0x8000) >> 15) | ((y & 0x8000) >> 14);
	if(m_child[index])
		return m_child[index]->getLeaf(x * 2, y * 2);

	return NULL;
}

QTreeLeafNode* QTreeNode::getLeafStatic(QTreeNode* root, uint16_t x, uint16_t y)
{
	QTreeNode* currentNode = root;
	uint32_t currentX = x, currentY = y;
	while(currentNode)
	{
		if(currentNode->isLeaf())
			return static_cast<QTreeLeafNode*>(currentNode);

		uint32_t index = ((currentX & 0x8000) >> 15) | ((currentY & 0x8000) >> 14);
		if(!currentNode->m_child[index])
			return NULL;

		currentNode = currentNode->m_child[index];
		currentX = currentX * 2;
		currentY = currentY * 2;
	}

	return NULL;
}

QTreeLeafNode* QTreeNode::createLeaf(uint16_t x, uint16_t y, uint16_t level)
{
	if(!isLeaf())
	{
		uint32_t index = ((x & 0x8000) >> 15) | ((y & 0x8000) >> 14);
		if(!m_child[index])
		{
			if(level != FLOOR_BITS)
				m_child[index] = new QTreeNode();
			else
			{
				m_child[index] = new QTreeLeafNode();
				QTreeLeafNode::newLeaf = true;
			}
		}

		return m_child[index]->createLeaf(x * 2, y * 2, level - 1);
	}

	return static_cast<QTreeLeafNode*>(this);
}


//************ LeafNode  ************************
bool QTreeLeafNode::newLeaf = false;
QTreeLeafNode::QTreeLeafNode()
{
	for(int32_t i = 0; i < MAP_MAX_LAYERS; ++i)
		m_array[i] = NULL;

	m_isLeaf = true;
	m_leafS = NULL;
	m_leafE = NULL;
}

QTreeLeafNode::~QTreeLeafNode()
{
	for(int32_t i = 0; i < MAP_MAX_LAYERS; ++i)
		delete m_array[i];
}

Floor* QTreeLeafNode::createFloor(uint16_t z)
{
	if(!m_array[z])
		m_array[z] = new Floor();

	return m_array[z];
}
