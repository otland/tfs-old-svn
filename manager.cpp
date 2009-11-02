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
#include "tools.h"

#include "configmanager.h"
#include "game.h"

#include "connection.h"
#include "outputmessage.h"
#include "networkmessage.h"

extern ConfigManager g_config;
extern Game g_game;

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

	if(OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false))
	{
		TRACK_MESSAGE(output);
		output->AddByte(MP_MSG_HELLO);

		output->AddU32(1); //version
		output->AddString("TFADMIN");
		OutputMessagePool::getInstance()->send(output);
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
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
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
				OutputMessagePool::getInstance()->send(output);

				getConnection()->close();
				addLogLine(LOGTYPE_WARNING, "Too many login attempts");
				return;
			}

			if(recvbyte != MP_MSG_LOGIN)
			{
				output->AddByte(MP_MSG_ERROR);
				output->AddString("You are not logged in");
				OutputMessagePool::getInstance()->send(output);

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
					if(!Manager::getInstance()->acceptConnection(this))
					{
						output->AddByte(MP_MSG_FAILURE);
						output->AddString("Unknown connection");
						OutputMessagePool::getInstance()->send(output);

						getConnection()->close();
						addLogLine(LOGTYPE_ERROR, "Login failed due to unknown connection");
						return;
					}
					else
					{
						m_state = LOGGED_IN;
						output->AddByte(MP_MSG_SUCCESS);
						addLogLine(LOGTYPE_EVENT, "Login ok");
					}
				}
				else
				{
					output->AddByte(MP_MSG_FAILURE);
					output->AddString("Wrong password");

					m_loginTries++;
					addLogLine(LOGTYPE_EVENT, "Login failed (" + pass + ")");
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

		case MP_MSG_KEEP_ALIVE:
			break;

		case MP_MSG_PING:
			output->AddByte(MP_MSG_PONG);
			break;

		case MP_MSG_LUA:
		{
			LuaInterface* interface = Manager::getInstance()->getInterface();
			if(interface->reserveEnv())
			{
				interface->loadBuffer(msg.GetString(), NULL);
				interface->releaseEnv();
				output->AddByte(MP_MSG_SUCCESS);
			}
			else
			{
				output->AddByte(MP_MSG_FAILURE);
				output->AddString("Unable to reserve script environment");
				addLogLine(LOGTYPE_ERROR, "Unable to reserve script environment");
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

	if(output->getMessageLength() > 0)
		OutputMessagePool::getInstance()->send(output);
}

void ProtocolManager::deleteProtocolTask()
{
	addLogLine(LOGTYPE_EVENT, "Ending connection");
	Manager::getInstance()->removeConnection(this);
	Protocol::deleteProtocolTask();
}

void ProtocolManager::output(const std::string& message)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(!output)
		return;

	TRACK_MESSAGE(output)
	output->AddByte(MP_MSG_OUTPUT);
	output->AddString(message);
	OutputMessagePool::getInstance()->send(output);
}

bool Manager::addConnection(ProtocolManager* client)
{
	if(g_config.getNumber(ConfigManager::MANAGER_CONNECTIONS_LIMIT) > 0 && m_clients.size()
		>= (size_t)g_config.getNumber(ConfigManager::MANAGER_CONNECTIONS_LIMIT))
		return false;

	m_clients[client] = false;
	return true;
}

bool Manager::acceptConnection(ProtocolManager* client)
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

void ProtocolManager::addLogLine(LogType_t type, std::string message)
{
	if(g_config.getBool(ConfigManager::MANAGER_LOGS))
		LOG_MESSAGE(type, message, "MANAGER " + convertIPAddress(getIP()))
}
