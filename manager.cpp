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
#include <iostream>

#include "manager.h"
#include "player.h"
#include "tools.h"

#include "configmanager.h"
#include "game.h"
#include "chat.h"

#include "connection.h"
#include "outputmessage.h"
#include "networkmessage.h"

extern ConfigManager g_config;
extern Game g_game;
extern Chat g_chat;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolManager::protocolManagerCount = 0;
#endif

void ProtocolManager::onRecvFirstMessage(NetworkMessage&)
{
	m_state = NO_CONNECTED;
	if(g_config.getString(ConfigManager::MANAGER_PASSWORD).empty())
	{
		addLogLine(LOGTYPE_WARNING, "Connection attempt on disabled protocol");
		getConnection()->close();
		return;
	}

	if(!Manager::getInstance()->allow(getIP()))
	{
		addLogLine(LOGTYPE_WARNING, "IP not allowed");
		getConnection()->close();
		return;
	}

	if(!Manager::getInstance()->addConnection(this))
	{
		addLogLine(LOGTYPE_WARNING, "Cannot add more connections");
		getConnection()->close();
		return;
	}

	if(NetworkMessage_ptr msg = getOutputBuffer())
	{
		TRACK_MESSAGE(msg);
		msg->put<char>(MP_MSG_HELLO);

		msg->put<uint32_t>(1); //version
		msg->putString("TFMANAGER");
	}

	m_lastCommand = time(NULL);
	m_state = NO_LOGGED_IN;
}

void ProtocolManager::parsePacket(NetworkMessage& msg)
{
	if(g_game.getGameState() == GAMESTATE_SHUTDOWN)
	{
		getConnection()->close();
		return;
	}

	uint8_t recvbyte = msg.get<char>();
	OutputMessage_ptr output = getOutputBuffer();
	if(!output)
		return;

	TRACK_MESSAGE(output);
	switch(m_state)
	{
		case NO_LOGGED_IN:
		{
			if((time(NULL) - m_startTime) > 30000)
			{
				//login timeout
				getConnection()->close();
				addLogLine(LOGTYPE_WARNING, "Login timeout");
				return;
			}

			if(m_loginTries > 3)
			{
				output->put<char>(MP_MSG_ERROR);
				output->putString("Too many login attempts");
				getConnection()->send(output);

				getConnection()->close();
				addLogLine(LOGTYPE_WARNING, "Too many login attempts");
				return;
			}

			if(recvbyte != MP_MSG_LOGIN)
			{
				output->put<char>(MP_MSG_ERROR);
				output->putString("You are not logged in");
				getConnection()->send(output);

				getConnection()->close();
				addLogLine(LOGTYPE_WARNING, "Wrong command while not logged in");
				return;
			}
			break;
		}

		case LOGGED_IN:
			break;

		default:
		{
			getConnection()->close();
			addLogLine(LOGTYPE_ERROR, "No valid connection state");
			return;
		}
	}

	m_lastCommand = time(NULL);
	switch(recvbyte)
	{
		case MP_MSG_LOGIN:
		{
			if(m_state == NO_LOGGED_IN)
			{
				std::string pass = msg.getString(), word = g_config.getString(ConfigManager::MANAGER_PASSWORD);
				_encrypt(word, false);
				if(pass == word)
				{
					if(!Manager::getInstance()->loginConnection(this))
					{
						output->put<char>(MP_MSG_FAILURE);
						output->putString("Unknown connection");
						getConnection()->send(output);

						getConnection()->close();
						addLogLine(LOGTYPE_ERROR, "Login failed due to unknown connection");
						return;
					}
					else
					{
						m_state = LOGGED_IN;
						output->put<char>(MP_MSG_USERS);
						addLogLine(LOGTYPE_EVENT, "Logged in, sending users");

						std::map<uint32_t, std::string> users;
						for(AutoList<Player>::iterator it = Player::autoList.begin(); it != Player::autoList.end(); ++it)
						{
							if(!it->second->isRemoved())
								users[it->first] = it->second->getName();
						}

						output->put<uint16_t>(users.size());
						for(std::map<uint32_t, std::string>::iterator it = users.begin(); it != users.end(); ++it)
						{
							output->put<uint32_t>(it->first);
							output->putString(it->second);
						}
					}
				}
				else
				{
					output->put<char>(MP_MSG_FAILURE);
					output->putString("Wrong password");

					m_loginTries++;
					addLogLine(LOGTYPE_EVENT, "Login failed due to wrong password (" + pass + ")");
				}
			}
			else
			{
				output->put<char>(MP_MSG_FAILURE);
				output->putString("Cannot login right now!");
				addLogLine(LOGTYPE_ERROR, "Wrong state at login");
			}

			break;
		}

		case MP_MSG_LOGOUT:
		{
			output->put<char>(MP_MSG_BYE);
			output->putString("Bye, bye!");
			getConnection()->send(output);

			getConnection()->close();
			addLogLine(LOGTYPE_EVENT, "Logout");
			return;
		}

		case MP_MSG_KEEP_ALIVE:
			break;

		case MP_MSG_PING:
			output->put<char>(MP_MSG_PONG);
			break;

		case MP_MSG_LUA:
		{
			std::string script = msg.getString();
			if(!Manager::getInstance()->execute(script))
			{
				output->put<char>(MP_MSG_FAILURE);
				output->putString("Unable to reserve enviroment for Lua script");
				addLogLine(LOGTYPE_ERROR, "Unable to reserve enviroment for Lua script");
			}
			else
			{
				output->put<char>(MP_MSG_SUCCESS);
				addLogLine(LOGTYPE_EVENT, "Executed Lua script");
			}

			break;
		}

		case MP_MSG_USER_INFO:
		{
			uint32_t playerId = msg.get<uint32_t>();
			if(Player* player = g_game.getPlayerByID(playerId))
			{
				output->put<char>(MP_MSG_USER_DATA);
				output->put<uint32_t>(playerId);

				output->put<uint32_t>(player->getGroupId());
				output->put<uint32_t>(player->getVocationId());

				output->put<uint32_t>(player->getLevel());
				output->put<uint32_t>(player->getMagicLevel());
				// TODO?
			}
			else
			{
				output->put<char>(MP_MSG_FAILURE);
				output->putString("Player not found");
			}
		}

		case MP_MSG_CHAT_REQUEST:
		{
			output->put<char>(MP_MSG_CHAT_LIST);
			ChannelList list = g_chat.getPublicChannels();

			output->put<uint16_t>(list.size());
			for(ChannelList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				output->put<uint16_t>((*it)->getId());
				output->putString((*it)->getName());

				output->put<uint16_t>((*it)->getFlags());
				output->put<uint16_t>((*it)->getUsers().size());
			}

			break;
		}

		case MP_MSG_CHAT_OPEN:
		{
			ChatChannel* channel = NULL;
			uint16_t channelId = msg.get<uint16_t>();
			if((channel = g_chat.getChannelById(channelId)) && g_chat.isPublicChannel(channelId))
			{
				m_channels |= (uint32_t)channelId;
				output->put<char>(MP_MSG_CHAT_USERS);
				UsersMap users = channel->getUsers();

				output->put<uint16_t>(users.size());
				for(UsersMap::const_iterator it = users.begin(); it != users.end(); ++it)
					output->put<uint32_t>(it->first);
			}
			else
			{
				output->put<char>(MP_MSG_FAILURE);
				output->putString("Invalid channel");
			}

			break;
		}

		case MP_MSG_CHAT_CLOSE:
		{
			uint16_t channelId = msg.get<uint16_t>();
			if(g_chat.getChannelById(channelId) && g_chat.isPublicChannel(channelId))
			{
				m_channels &= ~(uint32_t)channelId;
				output->put<char>(MP_MSG_SUCCESS);
			}
			else
			{
				output->put<char>(MP_MSG_FAILURE);
				output->putString("Invalid channel");
			}

			break;
		}

		case MP_MSG_CHAT_TALK:
		{
			std::string name = msg.getString();
			uint16_t channelId = msg.get<uint16_t>();
			SpeakClasses type = (SpeakClasses)msg.get<char>();
			std::string message = msg.getString();

			ChatChannel* channel = NULL;
			if((channel = g_chat.getChannelById(channelId)) && g_chat.isPublicChannel(channelId))
			{
				if(!channel->talk(name, type, message))
				{
					output->put<char>(MP_MSG_FAILURE);
					output->putString("Could not talk to channel");
				}
				else
					output->put<char>(MP_MSG_SUCCESS);
			}
			else
			{
				output->put<char>(MP_MSG_FAILURE);
				output->putString("Invalid channel");
			}

			break;
		}

		default:
		{
			output->put<char>(MP_MSG_ERROR);
			output->putString("Unknown command");

			addLogLine(LOGTYPE_WARNING, "Unknown command");
			break;
		}
	}
}

void ProtocolManager::deleteProtocolTask()
{
	addLogLine(LOGTYPE_EVENT, "Closing protocol");
	Manager::getInstance()->removeConnection(this);
	Protocol::deleteProtocolTask();
}

void ProtocolManager::output(const std::string& message)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg);
	msg->put<char>(MP_MSG_OUTPUT);
	msg->putString(message);
}

void ProtocolManager::addUser(Player* player)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg);
	msg->put<char>(MP_MSG_USER_ADD);

	msg->put<uint32_t>(player->getID());
	msg->putString(player->getName());
}

void ProtocolManager::removeUser(uint32_t playerId)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg);
	msg->put<char>(MP_MSG_USER_REMOVE);
	msg->put<uint32_t>(playerId);
}

void ProtocolManager::talk(uint32_t playerId, uint16_t channelId, SpeakClasses type, const std::string& message)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg);
	msg->put<char>(MP_MSG_CHAT_MESSAGE);
	msg->put<uint32_t>(playerId);

	msg->put<uint16_t>(channelId);
	msg->put<char>(type);
	msg->putString(message);
}

void ProtocolManager::addUser(uint32_t playerId, uint16_t channelId)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg);
	msg->put<char>(MP_MSG_CHAT_USER_ADD);

	msg->put<uint32_t>(playerId);
	msg->put<uint16_t>(channelId);
}

void ProtocolManager::removeUser(uint32_t playerId, uint16_t channelId)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg);
	msg->put<char>(MP_MSG_CHAT_USER_REMOVE);

	msg->put<uint32_t>(playerId);
	msg->put<uint16_t>(channelId);
}

bool Manager::addConnection(ProtocolManager* client)
{
	if(g_config.getNumber(ConfigManager::MANAGER_CONNECTIONS_LIMIT) > 0 && m_clients.size()
		>= (size_t)g_config.getNumber(ConfigManager::MANAGER_CONNECTIONS_LIMIT))
		return false;

	m_clients[client] = false;
	return true;
}

bool Manager::loginConnection(ProtocolManager* client)
{
	ClientMap::iterator it = m_clients.find(client);
	if(it == m_clients.end())
		return false;

	it->second = true;
	return true;
}

void Manager::removeConnection(ProtocolManager* client)
{
	ClientMap::iterator it = m_clients.find(client);
	if(it != m_clients.end())
		m_clients.erase(it);
}

bool Manager::allow(uint32_t ip) const
{
	if(!g_config.getBool(ConfigManager::MANAGER_LOCALHOST_ONLY))
		return !ConnectionManager::getInstance()->isDisabled(ip, 0xFE);

	if(ip == 0x0100007F) //127.0.0.1
		return true;

	if(g_config.getBool(ConfigManager::MANAGER_LOGS))
		LOG_MESSAGE(LOGTYPE_EVENT, "forbidden connection try", "MANAGER " + convertIPAddress(ip));

	return false;
}

bool Manager::execute(const std::string& script)
{
	if(!m_interface)
	{
		// create upon first execution to save some memory and
		// avoid any startup crashes related to creation on load
		m_interface = new LuaInterface("Manager Interface");
		m_interface->initState();
	}

	if(!m_interface->reserveEnv())
		return false;

	m_interface->loadBuffer(script);
	m_interface->releaseEnv();
	return true;
}

// should we run all these above on dispatcher thread?
void Manager::output(const std::string& message)
{
	if(m_clients.empty())
		return;

	for(ClientMap::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it)
	{
		if(it->second)
			it->first->output(message);
	}
}

void Manager::addUser(Player* player)
{
	if(m_clients.empty())
		return;

	for(ClientMap::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it)
	{
		if(it->second)
			it->first->addUser(player);
	}
}

void Manager::removeUser(uint32_t playerId)
{
	if(m_clients.empty())
		return;

	for(ClientMap::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it)
	{
		if(it->second)
			it->first->removeUser(playerId);
	}
}

void Manager::talk(uint32_t playerId, uint16_t channelId, SpeakClasses type, const std::string& message)
{
	if(m_clients.empty())
		return;

	for(ClientMap::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it)
	{
		if(it->second && it->first->checkChannel(channelId))
			it->first->talk(playerId, channelId, type, message);
	}
}

void Manager::addUser(uint32_t playerId, uint16_t channelId)
{
	if(m_clients.empty())
		return;

	for(ClientMap::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it)
	{
		if(it->second && it->first->checkChannel(channelId))
			it->first->addUser(playerId, channelId);
	}
}

void Manager::removeUser(uint32_t playerId, uint16_t channelId)
{
	if(m_clients.empty())
		return;

	for(ClientMap::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it)
	{
		if(it->second && it->first->checkChannel(channelId))
			it->first->removeUser(playerId, channelId);
	}
}

void ProtocolManager::addLogLine(LogType_t type, std::string message)
{
	if(!g_config.getBool(ConfigManager::MANAGER_LOGS))
		return;

	std::string tmp = "MANAGER";
	if(getIP())
		tmp += " " + convertIPAddress(getIP());

	LOG_MESSAGE(type, message, tmp)
}
