//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Game Servers handler
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
#ifndef __GAMESERVER_H__
#define __GAMESERVER_H__

#include <string>
#include <map>

class GameServer
{
	public:
		GameServer();
		GameServer(std::string, std::string, uint32_t);
		virtual ~GameServer() {}

		std::string getName() const {return name;}
		std::string getAddress() const {return ip;}
		uint32_t getPort() const {return port;}

		std::string getError() const {return error;}
		void setError(std::string _error) {error = _error;}

	protected:
		std::string name;
		std::string ip;
		uint32_t port;

		std::string error;
};

typedef std::map< uint32_t, GameServer* > GameServersMap;

class GameServers
{
	public:
		GameServers() {}
		static GameServers* getInstance()
		{
			static GameServers instance;
			return &instance;
		}

		bool loadFromXml(bool showResult = false);
		bool reload(bool showResult = false);

		bool addServer(uint32_t, GameServer*);

		GameServer* getServerByID(uint32_t id) const;
		GameServer* getServerByName(std::string name) const;
		GameServer* getServerByAddress(std::string address) const;
		GameServer* getServerByPort(uint32_t port) const;

	protected:
		virtual ~GameServers() {clear();}
		void clear();

		GameServersMap serverList;
};
#endif
