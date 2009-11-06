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

void ProtocolManager::onRecvFirstMessage(NetworkMessage& msg)
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
		msg->AddByte(MP_MSG_HELLO);

		msg->AddU32(1); //version
		msg->AddString("TFMANAGER");
	}

	m_lastCommand = time(NULL);
	m_state = NO_LOGGED_IN;
}

void ProtocolManager::parsePacket(NetworkMessage& msg)
{
	if(g_game.getGameState() == GAME_STATE_SHUTDOWN)
	{
		getConnection()->close();
		return;
	}

	uint8_t recvbyte = msg.GetByte();
	NetworkMessage_ptr output = getOutputBuffer();
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
				output->AddByte(MP_MSG_ERROR);
				output->AddString("Too many login attempts");
				OutputMessagePool::getInstance()->sendAll();

				getConnection()->close();
				addLogLine(LOGTYPE_WARNING, "Too many login attempts");
				return;
			}

			if(recvbyte != MP_MSG_LOGIN)
			{
				output->AddByte(MP_MSG_ERROR);
				output->AddString("You are not logged in");
				OutputMessagePool::getInstance()->sendAll();

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
				std::string pass = msg.GetString(), word = g_config.getString(ConfigManager::MANAGER_PASSWORD);
				_encrypt(word, false);
				if(pass == word)
				{
					if(!Manager::getInstance()->loginConnection(this))
					{
						output->AddByte(MP_MSG_FAILURE);
						output->AddString("Unknown connection");
						OutputMessagePool::getInstance()->sendAll();

						getConnection()->close();
						addLogLine(LOGTYPE_ERROR, "Login failed due to unknown connection");
						return;
					}
					else
					{
						m_state = LOGGED_IN;
						output->AddByte(MP_MSG_USERS);
						addLogLine(LOGTYPE_EVENT, "Logged in, sending users");

						std::map<uint32_t, std::string> users;
						for(AutoList<Player>::iterator it = Player::autoList.begin(); it != Player::autoList.end(); ++it)
						{
							if(!it->second->isRemoved())
								users[it->first] = it->second->getName();
						}

						output->AddU16(users.size());
						for(std::map<uint32_t, std::string>::iterator it = users.begin(); it != users.end(); ++it)
						{
							output->AddU32(it->first);
							output->AddString(it->second);
						}
					}
				}
				else
				{
					output->AddByte(MP_MSG_FAILURE);
					output->AddString("Wrong password");

					m_loginTries++;
					addLogLine(LOGTYPE_EVENT, "Login failed due to wrong password (" + pass + ")");
				}
			}
			else
			{
				output->AddByte(MP_MSG_FAILURE);
				output->AddString("Cannot login right now!");
				addLogLine(LOGTYPE_ERROR, "Wrong state at login");
			}

			break;
		}

		case MP_MSG_LOGOUT:
		{
			output->AddByte(MP_MSG_BYE);
			output->AddString("Bye, bye!");
			OutputMessagePool::getInstance()->sendAll();

			addLogLine(LOGTYPE_EVENT, "Logout");
			getConnection()->close();
			return;
		}

		case MP_MSG_KEEP_ALIVE:
			break;

		case MP_MSG_PING:
			output->AddByte(MP_MSG_PONG);
			break;

		case MP_MSG_LUA:
		{
			std::string script = msg.GetString();
			if(!Manager::getInstance()->execute(script))
			{
				output->AddByte(MP_MSG_FAILURE);
				output->AddString("Unable to reserve enviroment for Lua script");
				addLogLine(LOGTYPE_ERROR, "Unable to reserve enviroment for Lua script");
			}
			else
			{
				output->AddByte(MP_MSG_SUCCESS);
				addLogLine(LOGTYPE_EVENT, "Executed Lua script");
			}

			break;
		}

		case MP_MSG_USER_INFO:
		{
			uint32_t playerId = msg.GetU32();
			if(Player* player = g_game.getPlayerByID(playerId))
			{
				output->AddByte(MP_MSG_USER_DATA);
				output->AddU32(playerId);

				output->AddU32(player->getGroupId());
				output->AddU32(player->getVocationId());

				output->AddU32(player->getLevel());
				output->AddU32(player->getMagicLevel());
				// TODO?
			}
			else
			{
				output->AddByte(MP_MSG_FAILURE);
				output->AddString("Player not found");
			}
		}

		case MP_MSG_CHAT_REQUEST:
		{
			output->AddByte(MP_MSG_CHAT_LIST);
			ChannelList list = g_chat.getPublicChannels();

			output->AddU16(list.size());
			for(ChannelList::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				output->AddU16((*it)->getId());
				output->AddString((*it)->getName());

				output->AddU16((*it)->getFlags());
				output->AddU16((*it)->getUsers().size());
			}

			break;
		}

		case MP_MSG_CHAT_OPEN:
		{
			ChatChannel* channel = NULL;
			uint16_t channelId = msg.GetU16();
			if((channel = g_chat.getChannelById(channelId)) && g_chat.isPublicChannel(channelId))
			{
				m_channels |= (uint32_t)channelId;
				output->AddByte(MP_MSG_CHAT_USERS);
				UsersMap users = channel->getUsers();

				output->AddU16(users.size());
				for(UsersMap::const_iterator it = users.begin(); it != users.end(); ++it)
					output->AddU32(it->first);
			}
			else
			{
				output->AddByte(MP_MSG_FAILURE);
				output->AddString("Invalid channel");
			}

			break;
		}

		case MP_MSG_CHAT_CLOSE:
		{
			uint16_t channelId = msg.GetU16();
			if(g_chat.getChannelById(channelId) && g_chat.isPublicChannel(channelId))
			{
				m_channels &= ~(uint32_t)channelId;
				output->AddByte(MP_MSG_SUCCESS);
			}
			else
			{
				output->AddByte(MP_MSG_FAILURE);
				output->AddString("Invalid channel");
			}

			break;
		}

		case MP_MSG_CHAT_TALK:
		{
			std::string name = msg.GetString();
			uint16_t channelId = msg.GetU16();
			SpeakClasses type = (SpeakClasses)msg.GetByte();
			std::string message = msg.GetString();

			ChatChannel* channel = NULL;
			if((channel = g_chat.getChannelById(channelId)) && g_chat.isPublicChannel(channelId))
			{
				if(!channel->talk(name, type, message))
				{
					output->AddByte(MP_MSG_FAILURE);
					output->AddString("Could not talk to channel");
				}
				else
					output->AddByte(MP_MSG_SUCCESS);
			}
			else
			{
				output->AddByte(MP_MSG_FAILURE);
				output->AddString("Invalid channel");
			}

			break;
		}

		default:
		{
			output->AddByte(MP_MSG_ERROR);
			output->AddString("Unknown command");

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

	TRACK_MESSAGE(msg)
	msg->AddByte(MP_MSG_OUTPUT);
	msg->AddString(message);
}

void ProtocolManager::addUser(Player* player)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg)
	msg->AddByte(MP_MSG_USER_ADD);

	msg->AddU32(player->getID());
	msg->AddString(player->getName());
}

void ProtocolManager::removeUser(uint32_t playerId)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg)
	msg->AddByte(MP_MSG_USER_REMOVE);
	msg->AddU32(playerId);
}

void ProtocolManager::talk(uint32_t playerId, uint16_t channelId, SpeakClasses type, const std::string& message)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg)
	msg->AddByte(MP_MSG_CHAT_MESSAGE);
	msg->AddU32(playerId);

	msg->AddU16(channelId);
	msg->AddByte(type);
	msg->AddString(message);
}

void ProtocolManager::addUser(uint32_t playerId, uint16_t channelId)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg)
	msg->AddByte(MP_MSG_CHAT_USER_ADD);

	msg->AddU32(playerId);
	msg->AddU16(channelId);
}

void ProtocolManager::removeUser(uint32_t playerId, uint16_t channelId)
{
	NetworkMessage_ptr msg = getOutputBuffer();
	if(!msg)
		return;

	TRACK_MESSAGE(msg)
	msg->AddByte(MP_MSG_CHAT_USER_REMOVE);

	msg->AddU32(playerId);
	msg->AddU16(channelId);
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
	if(!m_interface.reserveEnv())
		return false;

	m_interface.loadBuffer(script);
	m_interface.releaseEnv();
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
