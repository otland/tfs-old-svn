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
		addLogLine(LOGTYPE_EVENT, "connection attempt on disabled protocol");
		getConnection()->close();
		return;
	}

	if(!Manager::getInstance()->allow(getIP()))
	{
		addLogLine(LOGTYPE_EVENT, "ip not allowed");
		getConnection()->close();
		return;
	}

	if(!Manager::getInstance()->addConnection())
	{
		addLogLine(LOGTYPE_EVENT, "cannot add new connection");
		getConnection()->close();
		return;
	}

	addLogLine(LOGTYPE_EVENT, "sending HELLO");
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
				addLogLine(LOGTYPE_EVENT, "login timeout");
				return;
			}

			if(m_loginTries > 3)
			{
				output->AddByte(MP_MSG_ERROR);
				output->AddString("too many login tries");
				OutputMessagePool::getInstance()->send(output);

				getConnection()->close();
				addLogLine(LOGTYPE_EVENT, "too many login tries");
				return;
			}

			if(recvbyte != MP_MSG_LOGIN)
			{
				output->AddByte(MP_MSG_ERROR);
				output->AddString("you are not logged in");
				OutputMessagePool::getInstance()->send(output);

				getConnection()->close();
				addLogLine(LOGTYPE_EVENT, "wrong command while NO_LOGGED_IN");
				return;
			}
			break;
		}

		case LOGGED_IN:
			break;

		default:
		{
			getConnection()->close();
			addLogLine(LOGTYPE_EVENT, "no valid connection state!!!");
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
					m_state = LOGGED_IN;
					output->AddByte(MP_MSG_LOGIN_OK);
					addLogLine(LOGTYPE_EVENT, "login ok");
				}
				else
				{
					m_loginTries++;
					output->AddByte(MP_MSG_LOGIN_FAILED);
					output->AddString("wrong password");
					addLogLine(LOGTYPE_EVENT, "login failed.("+ pass + ")");
				}
			}
			else
			{
				output->AddByte(MP_MSG_LOGIN_FAILED);
				output->AddString("cannot login");
				addLogLine(LOGTYPE_EVENT, "wrong state at login");
			}

			break;
		}

		case MP_MSG_COMMAND:
		{
			if(m_state != LOGGED_IN)
			{
				addLogLine(LOGTYPE_EVENT, "recvbyte == MP_MSG_COMMAND && m_state != LOGGED_IN !!!");
				break;
			}

			uint8_t command = msg.GetByte();
			switch(command)
			{
				case CMD_TEST:
				{
					const std::string param = msg.GetString();
					addLogLine(LOGTYPE_EVENT, "test command: " + param);

					output->AddByte(MP_MSG_COMMAND_OK);
					break;
				}

				default:
				{
					output->AddByte(MP_MSG_COMMAND_FAILED);
					output->AddString("not known server command");
					addLogLine(LOGTYPE_EVENT, "not known server command");
				}
			}
			break;
		}

		case MP_MSG_PING:
			output->AddByte(MP_MSG_PING_OK);
			break;

		case MP_MSG_KEEP_ALIVE:
			break;

		default:
		{
			output->AddByte(MP_MSG_ERROR);
			output->AddString("not known command byte");

			addLogLine(LOGTYPE_EVENT, "not known command byte");
			break;
		}
	}

	if(output->getMessageLength() > 0)
		OutputMessagePool::getInstance()->send(output);
}

void ProtocolManager::deleteProtocolTask()
{
	addLogLine(LOGTYPE_EVENT, "end connection");
	Manager::getInstance()->removeConnection();
	Protocol::deleteProtocolTask();
}

bool Manager::addConnection()
{
	if(m_currrentConnections >= g_config.getNumber(ConfigManager::MANAGER_CONNECTIONS_LIMIT))
		return false;

	m_currrentConnections++;
	return true;
}

void Manager::removeConnection()
{
	if(m_currrentConnections > 0)
		m_currrentConnections--;
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

void ProtocolManager::addLogLine(LogType_t type, std::string message)
{
	if(g_config.getBool(ConfigManager::MANAGER_LOGS))
		LOG_MESSAGE(type, message, "MANAGER " + convertIPAddress(getIP()))
}
