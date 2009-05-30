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
#include "thing.h"

#include "cylinder.h"
#include "tile.h"

#include "item.h"
#include "creature.h"
#include "player.h"

Cylinder* Thing::getTopParent()
{
	//tile
	if(!getParent())
		return dynamic_cast<Cylinder*>(this);

	Cylinder* aux = getParent();
	Cylinder* prevaux = dynamic_cast<Cylinder*>(this);
	while(aux->getParent())
	{
		prevaux = aux;
		aux = aux->getParent();
	}

	if(dynamic_cast<Cylinder*>(prevaux))
		return prevaux;

	return aux;
}

const Cylinder* Thing::getTopParent() const
{
	//tile
	if(getParent() == NULL)
		return dynamic_cast<const Cylinder*>(this);

	const Cylinder* aux = getParent();
	const Cylinder* prevaux = dynamic_cast<const Cylinder*>(this);
	while(aux->getParent())
	{
		prevaux = aux;
		aux = aux->getParent();
	}

	if(dynamic_cast<const Cylinder*>(prevaux))
		return prevaux;

	return aux;
}

Tile* Thing::getTile()
{
	Cylinder* cylinder = getTopParent();
#ifdef __DEBUG_MOVESYS__
	if(!cylinder)
	{
		std::cout << "[Failure - Thing::getTile] NULL tile" << std::endl;
		DEBUG_REPORT
		return &(Tile::nullTile);
	}
#endif

	//get root cylinder
	if(cylinder->getParent())
		cylinder = cylinder->getParent();

	return dynamic_cast<Tile*>(cylinder);
}

const Tile* Thing::getTile() const
{
	const Cylinder* cylinder = getTopParent();
#ifdef __DEBUG_MOVESYS__
	if(!cylinder)
	{
		std::cout << "[Failure - Thing::getTile] NULL tile" << std::endl;
		DEBUG_REPORT
		return &(Tile::nullTile);
	}
#endif

	//get root cylinder
	if(cylinder->getParent())
		cylinder = cylinder->getParent();

	return dynamic_cast<const Tile*>(cylinder);
}

Position Thing::getPosition() const
{
	if(const Tile* tile = getTile())
		return tile->getTilePosition();

#ifdef __DEBUG_MOVESYS__
	std::cout << "[Failure - Thing::getTile] NULL tile" << std::endl;
	DEBUG_REPORT
#endif
	return Tile::nullTile.getTilePosition();
}

bool Thing::isRemoved() const
{
	if(!parent)
		return true;

	const Cylinder* aux = getParent();
	return aux->isRemoved();
}
