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

#ifndef __GAMESERVER__
#define __GAMESERVER__

#include "otsystem.h"
#include "const.h"

class GameServer
{
	public:
		GameServer(): name("TheForgottenServer"), address(LOCALHOST), port(7172), 
			versionMin(CLIENT_VERSION_MIN), versionMax(CLIENT_VERSION_MAX) {}
		GameServer(std::string _name, uint32_t _versionMin, uint32_t _versionMax, uint32_t _address, uint32_t _port):
			name(_name), address(_address), port(_port), versionMin(_versionMin), versionMax(_versionMax) {}
		virtual ~GameServer() {}

		std::string getName() const {return name;}
		uint32_t getVersionMin() const {return versionMin;}
		uint32_t getVersionMax() const {return versionMax;}

		uint32_t getAddress() const {return address;}
		uint32_t getPort() const {return port;}

	protected:
		std::string name;
		uint32_t address, port, versionMin, versionMax;
};

typedef std::map<uint32_t, GameServer*> GameServersMap;
class GameServers
{
	public:
		GameServers() {}
		virtual ~GameServers() {clear();}

		static GameServers* getInstance()
		{
			static GameServers instance;
			return &instance;
		}

		bool loadFromXml(bool result);
		bool reload();

		GameServer* getServerById(uint32_t id) const;
		GameServer* getServerByName(std::string name) const;
		GameServer* getServerByAddress(uint32_t address) const;
		GameServer* getServerByPort(uint32_t port) const;

	protected:
		void clear();

		GameServersMap serverList;
};
#endif
