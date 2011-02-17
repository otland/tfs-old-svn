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

#ifndef __MOUNTS__
#define __MOUNTS__
#include "otsystem.h"

#include "networkmessage.h"
#include "player.h"

class Mount
{
	public:
		Mount(std::string _name, uint16_t _id, uint16_t _clientId, int32_t _speed)
		{
			name = _name;
			id = _id;
			speed = _speed;
			clientId = _clientId;
		}

		bool isTamed(Player* player) const;

		uint16_t getId() const {return id;}
		const std::string& getName() const {return name;}
		uint32_t getSpeed() const {return speed;}
		uint16_t getClientId() const {return clientId;}
		
		
	private:
		std::string name;
		uint8_t id;
		uint16_t clientId;
		int32_t speed;
};

typedef std::list<Mount*> MountList;
class Mounts
{
	public:
		virtual ~Mounts() {clear();}
		static Mounts* getInstance()
		{
			static Mounts instance;
			return &instance;
		}

		void clear();
		bool reload();

		bool loadFromXml();
		bool parseMountNode(xmlNodePtr p);

		inline MountList::const_iterator getFirstMount() const {return mounts.begin();}
		inline MountList::const_iterator getLastMount() const {return mounts.end();}

		Mount* getMountById(uint16_t id) const;
		Mount* getMountByCid(uint16_t id) const;
		uint8_t getMountCount() const {return mountCount;}
		
	private:
		MountList mounts;
		uint8_t mountCount;
};
#endif
