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

#ifndef __IOMAPSERIALIZE_H__
#define __IOMAPSERIALIZE_H__

#include "database.h"
#include "map.h"

#include <string>
#include <list>

typedef std::map<int32_t, std::pair<Item*, int32_t> > ItemMap;
typedef std::list<std::pair<Container*, int32_t> > ContainerStackList;

class IOMapSerialize
{
	public:
		IOMapSerialize() {}
		virtual ~IOMapSerialize() {}

		bool loadMap(Map* map);
		bool saveMap(Map* map);

		bool loadHouseInfo(Map* map);
		bool saveHouseInfo(Map* map);

	protected:
		// Relational storage uses a row for each item/tile
		bool loadMapRelational(Map* map);
		bool saveMapRelational(Map* map);
	
		// Binary storage uses a giant BLOB field for storing everything
		bool loadMapBinary(Map* map);
		bool saveMapBinary(Map* map);

		bool loadTile(Database& db, Tile* tile);
		bool saveTile(Database* db, uint32_t tileId, const Tile* tile);

		bool saveTile(PropWriteStream& stream, const Tile* tile);

		bool loadItem(PropStream& propStream, Cylinder* parent);
		bool saveItem(PropWriteStream& stream, const Item* item);
};

#endif
