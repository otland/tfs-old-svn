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
#include "otpch.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "tools.h"
#include "gameservers.h"

#include <iostream>

void GameServers::clear()
{
	for(GameServersMap::iterator it = serverList.begin(); it != serverList.end(); ++it)
		delete it->second;

	serverList.clear();
}

bool GameServers::reload(bool showResult /*= true*/)
{
	clear();
	return loadFromXml(showResult);
}

bool GameServers::loadFromXml(bool showResult /*= true*/)
{
	std::string filename = "data/XML/servers.xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc)
	{
		xmlNodePtr root, p;
		std::string strValue, name, ip;
		int32_t intValue, id, port;

		root = xmlDocGetRootElement(doc);
		if(xmlStrcmp(root->name,(const xmlChar*)"servers") != 0)
		{
			xmlFreeDoc(doc);
			return false;
		}

		p = root->children;
		while(p)
		{

			if(!xmlStrcmp(p->name, (const xmlChar*)"server"))
			{
				if(readXMLInteger(p, "id", intValue))
					id = intValue;
				else
				{
					std::cout << "[Error - GameServers::load] Missing server id" << std::endl;
					continue;
				}

				if(readXMLString(p, "name", strValue))
					name = strValue;
				else
				{
					std::cout << "[Error - GameServers::load] Missing server name" << std::endl;
					continue;
				}

				if(readXMLString(p, "ip", strValue))
					ip = strValue;
				else
				{
					std::cout << "[Error - GameServers::load] Missing server ip" << std::endl;
					continue;
				}

				if(readXMLInteger(p, "port", intValue))
					port = intValue;
				else
				{
					std::cout << "[Error - GameServers::load] Missing server port" << std::endl;
					continue;
				}

				GameServer* server = new GameServer(name, ip, port);
				if(server && !addServer(id, server))
					std::cout << "[Error - GameServers::load] Couldn't add server - " << server->getError() << std::endl;
			}

			p = p->next;
		}

		xmlFreeDoc(doc);
	}

	if(showResult)
	{
		std::cout << "> Servers loaded:" << std::endl;
		for(GameServersMap::iterator it = serverList.begin(); it != serverList.end(); it++)
			std::cout << it->second->getName() << " (" << it->second->getAddress() << ":" << it->second->getPort() << ")" << std::endl;
	}
	return true;
}

bool GameServers::addServer(uint32_t id, GameServer* server)
{
	if(getServerByID(id))
	{
		char buf[200];
		sprintf(buf, "[Warning - GameServers::addServer] Duplicated id - %i", id);
		server->setError(buf);
		return false;
	}

	serverList[id] = server;
	return true;
}

GameServer::GameServer()
{
	ip = "127.0.0.1";
	port = 7171;
	name = "ForgottenServer";
}

GameServer::GameServer(std::string _name, std::string _ip, uint32_t _port)
{
	name = _name;
	ip = _ip;
	port = _port;
}

GameServer* GameServers::getServerByID(uint32_t id) const
{
	GameServersMap::const_iterator it = serverList.find(id);
	if(it != serverList.end())
		return it->second;

	return NULL;
}

GameServer* GameServers::getServerByName(std::string name) const
{
	for(GameServersMap::const_iterator it = serverList.begin(); it != serverList.end(); ++it)
	{
		if(it->second->getName() == name)
			return it->second;
	}
	return NULL;
}

GameServer* GameServers::getServerByAddress(std::string address) const
{
	for(GameServersMap::const_iterator it = serverList.begin(); it != serverList.end(); ++it)
	{
		if(it->second->getAddress() == address)
			return it->second;
	}
	return NULL;
}

GameServer* GameServers::getServerByPort(uint32_t port) const
{
	for(GameServersMap::const_iterator it = serverList.begin(); it != serverList.end(); ++it)
	{
		if(it->second->getPort() == port)
			return it->second;
	}
	return NULL;
}
