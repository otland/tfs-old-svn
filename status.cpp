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
#include "resources.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "status.h"
#include "tools.h"

#include "connection.h"
#include "networkmessage.h"
#include "outputmessage.h"

#include "configmanager.h"
#include "game.h"

#ifndef WINDOWS
	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
#endif

extern ConfigManager g_config;
extern Game g_game;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolStatus::protocolStatusCount = 0;
#endif
IpConnectMap ProtocolStatus::ipConnectMap;

void ProtocolStatus::onRecvFirstMessage(NetworkMessage& msg)
{
	for(StringVec::const_iterator it = g_game.blacklist.begin(); it != g_game.blacklist.end(); ++it)
	{
		if((*it) == convertIPAddress(getIP()))
		{
			getConnection()->close();
			return;
		}
	}

	IpConnectMap::const_iterator it = ipConnectMap.find(getIP());
	if(it != ipConnectMap.end() && OTSYS_TIME() < it->second + g_config.getNumber(ConfigManager::STATUSQUERY_TIMEOUT))
	{
		getConnection()->close();
		return;
	}

	ipConnectMap[getIP()] = OTSYS_TIME();
	switch(msg.GetByte())
	{
		case 0xFF:
		{
			if(msg.GetString(4) == "info")
			{
				if(OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false))
				{
					TRACK_MESSAGE(output);
					if(Status* status = Status::getInstance())
					{
						bool sendPlayers = false;
						if(msg.getMessageLength() > msg.getReadPos())
							sendPlayers = msg.GetByte() == 0x01;

						std::string str = status->getStatusString(sendPlayers);
						output->AddBytes(str.c_str(), str.size());
					}

					setRawMessages(true); // we dont want the size header, nor encryption
					OutputMessagePool::getInstance()->send(output);
				}
			}

			break;
		}

		case 0x01:
		{
			uint32_t requestedInfo = msg.GetU16(); //Only a Byte is necessary, though we could add new infos here
			if(OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false))
			{
				TRACK_MESSAGE(output);
				if(Status* status = Status::getInstance())
					status->getInfo(requestedInfo, output, msg);

				OutputMessagePool::getInstance()->send(output);
			}

			break;
		}

		default:
			break;
	}

	getConnection()->close();
}

void ProtocolStatus::deleteProtocolTask()
{
#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Deleting ProtocolStatus" << std::endl;
#endif
	Protocol::deleteProtocolTask();
}

std::string Status::getStatusString(bool sendPlayers) const
{
	char buffer[90];
	xmlDocPtr doc;
	xmlNodePtr p, root;

	doc = xmlNewDoc((const xmlChar*)"1.0");
	doc->children = xmlNewDocNode(doc, NULL, (const xmlChar*)"tsqp", NULL);
	root = doc->children;

	xmlSetProp(root, (const xmlChar*)"version", (const xmlChar*)"1.0");

	p = xmlNewNode(NULL,(const xmlChar*)"serverinfo");
	sprintf(buffer, "%u", (uint32_t)getUptime());
	xmlSetProp(p, (const xmlChar*)"uptime", (const xmlChar*)buffer);
	xmlSetProp(p, (const xmlChar*)"ip", (const xmlChar*)g_config.getString(ConfigManager::IP).c_str());
	xmlSetProp(p, (const xmlChar*)"servername", (const xmlChar*)g_config.getString(ConfigManager::SERVER_NAME).c_str());
	sprintf(buffer, "%d", g_config.getNumber(ConfigManager::LOGIN_PORT));
	xmlSetProp(p, (const xmlChar*)"port", (const xmlChar*)buffer);
	xmlSetProp(p, (const xmlChar*)"location", (const xmlChar*)g_config.getString(ConfigManager::LOCATION).c_str());
	xmlSetProp(p, (const xmlChar*)"url", (const xmlChar*)g_config.getString(ConfigManager::URL).c_str());
	xmlSetProp(p, (const xmlChar*)"server", (const xmlChar*)STATUS_SERVER_NAME);
	xmlSetProp(p, (const xmlChar*)"version", (const xmlChar*)STATUS_SERVER_VERSION);
	xmlSetProp(p, (const xmlChar*)"client", (const xmlChar*)STATUS_SERVER_PROTOCOL);
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"owner");
	xmlSetProp(p, (const xmlChar*)"name", (const xmlChar*)g_config.getString(ConfigManager::OWNER_NAME).c_str());
	xmlSetProp(p, (const xmlChar*)"email", (const xmlChar*)g_config.getString(ConfigManager::OWNER_EMAIL).c_str());
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"players");
	sprintf(buffer, "%d", g_game.getPlayersOnline());
	xmlSetProp(p, (const xmlChar*)"online", (const xmlChar*)buffer);
	sprintf(buffer, "%d", g_config.getNumber(ConfigManager::MAX_PLAYERS));
	xmlSetProp(p, (const xmlChar*)"max", (const xmlChar*)buffer);
	sprintf(buffer, "%d", g_game.getPlayersRecord());
	xmlSetProp(p, (const xmlChar*)"peak", (const xmlChar*)buffer);
	if(sendPlayers)
	{
		std::stringstream ss;
		for(AutoList<Player>::iterator it = Player::autoList.begin(); it != Player::autoList.end(); ++it)
		{
			if(it->second->isRemoved() || it->second->isGhost())
				continue;

			if(!ss.str().empty())
				ss << ";";

			ss << it->second->getName() << "," << it->second->getVocationId() << "," << it->second->getLevel();
		}

		xmlNodeSetContent(p, (const xmlChar*)ss.str().c_str());
	}

	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"monsters");
	sprintf(buffer, "%d", g_game.getMonstersOnline());
	xmlSetProp(p, (const xmlChar*)"total", (const xmlChar*)buffer);
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"npcs");
	sprintf(buffer, "%d", g_game.getNpcsOnline());
	xmlSetProp(p, (const xmlChar*)"total", (const xmlChar*)buffer);
	xmlAddChild(root, p);

	p = xmlNewNode(NULL,(const xmlChar*)"map");
	xmlSetProp(p, (const xmlChar*)"name", (const xmlChar*)m_mapName.c_str());
	xmlSetProp(p, (const xmlChar*)"author", (const xmlChar*)g_config.getString(ConfigManager::MAP_AUTHOR).c_str());

	uint32_t mapWidth, mapHeight;
	g_game.getMapDimensions(mapWidth, mapHeight);
	sprintf(buffer, "%u", mapWidth);
	xmlSetProp(p, (const xmlChar*)"width", (const xmlChar*)buffer);
	sprintf(buffer, "%u", mapHeight);

	xmlSetProp(p, (const xmlChar*)"height", (const xmlChar*)buffer);
	xmlAddChild(root, p);

	xmlNewTextChild(root, NULL, (const xmlChar*)"motd", (const xmlChar*)g_config.getString(ConfigManager::MOTD).c_str());

	xmlChar* s = NULL;
	int32_t len = 0;
	xmlDocDumpMemory(doc, (xmlChar**)&s, &len);

	std::string xml;
	if(s)
		xml = std::string((char*)s, len);

	xmlFree(s);
	xmlFreeDoc(doc);
	return xml;
}

void Status::getInfo(uint32_t requestedInfo, OutputMessage_ptr output, NetworkMessage& msg) const
{
	if(requestedInfo & REQUEST_BASIC_SERVER_INFO)
	{
		output->AddByte(0x10);
		output->AddString(g_config.getString(ConfigManager::SERVER_NAME).c_str());
		output->AddString(g_config.getString(ConfigManager::IP).c_str());

		char buffer[10];
		sprintf(buffer, "%d", g_config.getNumber(ConfigManager::LOGIN_PORT));
		output->AddString(buffer);
	}

	if(requestedInfo & REQUEST_SERVER_OWNER_INFO)
	{
		output->AddByte(0x11);
		output->AddString(g_config.getString(ConfigManager::OWNER_NAME).c_str());
		output->AddString(g_config.getString(ConfigManager::OWNER_EMAIL).c_str());
	}

	if(requestedInfo & REQUEST_MISC_SERVER_INFO)
	{
		output->AddByte(0x12);
		output->AddString(g_config.getString(ConfigManager::MOTD).c_str());
		output->AddString(g_config.getString(ConfigManager::LOCATION).c_str());
		output->AddString(g_config.getString(ConfigManager::URL).c_str());

		uint64_t uptime = getUptime();
		output->AddU32((uint32_t)(uptime >> 32));
		output->AddU32((uint32_t)(uptime));
	}

	if(requestedInfo & REQUEST_PLAYERS_INFO)
	{
		output->AddByte(0x20);
		output->AddU32(g_game.getPlayersOnline());
		output->AddU32(g_config.getNumber(ConfigManager::MAX_PLAYERS));
		output->AddU32(g_game.getPlayersRecord());
	}

	if(requestedInfo & REQUEST_SERVER_MAP_INFO)
	{
		output->AddByte(0x30);
		output->AddString(m_mapName.c_str());
		output->AddString(g_config.getString(ConfigManager::MAP_AUTHOR).c_str());

		uint32_t mapWidth, mapHeight;
		g_game.getMapDimensions(mapWidth, mapHeight);
		output->AddU16(mapWidth);
		output->AddU16(mapHeight);
	}

	if(requestedInfo & REQUEST_EXT_PLAYERS_INFO)
	{
		output->AddByte(0x21);
		std::list<std::pair<std::string, uint32_t> > players;
		for(AutoList<Player>::iterator it = Player::autoList.begin(); it != Player::autoList.end(); ++it)
		{
			if(!it->second->isRemoved() && !it->second->isGhost())
				players.push_back(std::make_pair(it->second->getName(), it->second->getLevel()));
		}

		output->AddU32(players.size());
		for(std::list<std::pair<std::string, uint32_t> >::iterator it = players.begin(); it != players.end(); ++it)
		{
			output->AddString(it->first);
			output->AddU32(it->second);
		}
	}

	if(requestedInfo & REQUEST_PLAYER_STATUS_INFO)
	{
		output->AddByte(0x22);
		const std::string name = msg.GetString();

		Player* p = NULL;
		if(g_game.getPlayerByNameWildcard(name, p) == RET_NOERROR && !p->isGhost())
			output->AddByte(0x01);
		else
			output->AddByte(0x00);
	}

	if(requestedInfo & REQUEST_SERVER_SOFTWARE_INFO)
	{
		output->AddByte(0x23);
		output->AddString(STATUS_SERVER_NAME);
		output->AddString(STATUS_SERVER_VERSION);
		output->AddString(STATUS_SERVER_PROTOCOL);
	}
}
