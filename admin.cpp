//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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
#ifdef __REMOTE_CONTROL__
#include "otpch.h"
#include "admin.h"

#include <iostream>

#include "configmanager.h"
#include "game.h"
#include "tools.h"
#include "rsa.h"

#include "connection.h"
#include "outputmessage.h"
#include "networkmessage.h"

#include "house.h"
#include "iologindata.h"

extern Game g_game;
extern ConfigManager g_config;
Admin* g_admin = NULL;

Logger::Logger()
{
	m_file = fopen(getFilePath(FILE_TYPE_LOG, "ForgottenAdmin.log").c_str(), "a");
}

Logger::~Logger()
{
	if(m_file)
		fclose(m_file);
}

void Logger::logMessage(const char* channel, LogType_t type, int32_t level, std::string message, const char* func)
{
	char buffer[32];
	formatDate(time(NULL), buffer);
	fprintf(m_file, "%s", buffer);
	if(channel)
		fprintf(m_file, " [%s] ", channel);

	std::string typeStr = "unknown";
	switch(type)
	{
		case LOGTYPE_EVENT:
			typeStr = "event";
			break;

		case LOGTYPE_WARNING:
			typeStr = "warning";
			break;

		case LOGTYPE_ERROR:
			typeStr = "error";
			break;

		default:
			break;
	}

	fprintf(m_file, " %s:", typeStr.c_str());
	fprintf(m_file, " %s\n", message.c_str());
	fflush(m_file);
}

static void addLogLine(ProtocolAdmin* protocol, LogType_t type, int32_t level, std::string message);
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolAdmin::protocolAdminCount = 0;
#endif

void ProtocolAdmin::onRecvFirstMessage(NetworkMessage& msg)
{
	if(!g_admin->enabled())
	{
		getConnection()->close();
		return;
	}

	m_state = NO_CONNECTED;
	if(!g_admin->allowIP(getIP()))
	{
		addLogLine(this, LOGTYPE_EVENT, 1, "ip not allowed");
		getConnection()->close();
		return;
	}

	if(!g_admin->addConnection())
	{
		addLogLine(this, LOGTYPE_EVENT, 1, "cannot add new connection");
		getConnection()->close();
		return;
	}

	addLogLine(this, LOGTYPE_EVENT, 1, "sending HELLO");
	if(OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false))
	{
		TRACK_MESSAGE(output);
		output->AddByte(AP_MSG_HELLO);
		output->AddU32(1); //version
		output->AddString("OTADMIN");
		output->AddU16(g_admin->getProtocolPolicy()); //security policy
		output->AddU32(g_admin->getProtocolOptions()); //protocol options(encryption, ...)
		OutputMessagePool::getInstance()->send(output);
	}

	m_lastCommand = time(NULL);
	m_state = ENCRYPTION_NO_SET;
}

void ProtocolAdmin::parsePacket(NetworkMessage& msg)
{
	uint8_t recvbyte = msg.GetByte();
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(!output)
		return;

	TRACK_MESSAGE(output);
	switch(m_state)
	{
		case ENCRYPTION_NO_SET:
		{
			if(g_admin->requireEncryption())
			{
				if((time(NULL) - m_startTime) > 30000)
				{
					getConnection()->close();
					addLogLine(this, LOGTYPE_WARNING, 1, "encryption timeout");
					return;
				}

				if(recvbyte != AP_MSG_ENCRYPTION && recvbyte != AP_MSG_KEY_EXCHANGE)
				{
					output->AddByte(AP_MSG_ERROR);
					output->AddString("encryption needed");
					OutputMessagePool::getInstance()->send(output);

					getConnection()->close();
					addLogLine(this, LOGTYPE_WARNING, 1, "wrong command while ENCRYPTION_NO_SET");
					return;
				}
			}
			else
				m_state = NO_LOGGED_IN;

			break;
		}

		case NO_LOGGED_IN:
		{
			if(g_admin->requireLogin())
			{
				if((time(NULL) - m_startTime) > 30000)
				{
					//login timeout
					getConnection()->close();
					addLogLine(this, LOGTYPE_WARNING, 1, "login timeout");
					return;
				}

				if(m_loginTries > 3)
				{
					output->AddByte(AP_MSG_ERROR);
					output->AddString("too many login tries");
					OutputMessagePool::getInstance()->send(output);

					getConnection()->close();
					addLogLine(this, LOGTYPE_WARNING, 1, "too many login tries");
					return;
				}

				if(recvbyte != AP_MSG_LOGIN)
				{
					output->AddByte(AP_MSG_ERROR);
					output->AddString("you are not logged in");
					OutputMessagePool::getInstance()->send(output);

					getConnection()->close();
					addLogLine(this, LOGTYPE_WARNING, 1, "wrong command while NO_LOGGED_IN");
					return;
				}
			}
			else
				m_state = LOGGED_IN;

			break;
		}

		case LOGGED_IN:
			break;

		default:
		{
			getConnection()->close();
			addLogLine(this, LOGTYPE_ERROR, 1, "no valid connection state!!!");
			return;
		}
	}

	m_lastCommand = time(NULL);
	switch(recvbyte)
	{
		case AP_MSG_LOGIN:
		{
			if(m_state == NO_LOGGED_IN && g_admin->requireLogin())
			{
				std::string password = msg.GetString();
				if(g_admin->passwordMatch(password))
				{
					m_state = LOGGED_IN;
					output->AddByte(AP_MSG_LOGIN_OK);
					addLogLine(this, LOGTYPE_EVENT, 1, "login ok");
				}
				else
				{
					m_loginTries++;
					output->AddByte(AP_MSG_LOGIN_FAILED);
					output->AddString("wrong password");
					addLogLine(this, LOGTYPE_WARNING, 1, "login failed.("+ password + ")");
				}
			}
			else
			{
				output->AddByte(AP_MSG_LOGIN_FAILED);
				output->AddString("can not login");
				addLogLine(this, LOGTYPE_WARNING, 1, "wrong state at login");
			}

			break;
		}

		case AP_MSG_ENCRYPTION:
		{
			if(m_state == ENCRYPTION_NO_SET && g_admin->requireEncryption())
			{
				uint8_t keyType = msg.GetByte();
				switch(keyType)
				{
					case ENCRYPTION_RSA1024XTEA:
					{
						RSA* rsa = g_admin->getRSAKey(ENCRYPTION_RSA1024XTEA);
						if(!rsa)
						{
							output->AddByte(AP_MSG_ENCRYPTION_FAILED);
							addLogLine(this, LOGTYPE_WARNING, 1, "no valid server key type");
							break;
						}

						if(RSA_decrypt(rsa, msg))
						{
							m_state = NO_LOGGED_IN;
							uint32_t k[4]= {msg.GetU32(), msg.GetU32(), msg.GetU32(), msg.GetU32()};

							//use for in/out the new key we have
							enableXTEAEncryption();
							setXTEAKey(k);

							output->AddByte(AP_MSG_ENCRYPTION_OK);
							addLogLine(this, LOGTYPE_EVENT, 1, "encryption ok");
						}
						else
						{
							output->AddByte(AP_MSG_ENCRYPTION_FAILED);
							output->AddString("wrong encrypted packet");
							addLogLine(this, LOGTYPE_WARNING, 1, "wrong encrypted packet");
						}

						break;
					}

					default:
					{
						output->AddByte(AP_MSG_ENCRYPTION_FAILED);
						output->AddString("no valid key type");

						addLogLine(this, LOGTYPE_WARNING, 1, "no valid client key type");
						break;
					}
				}
			}
			else
			{
				output->AddByte(AP_MSG_ENCRYPTION_FAILED);
				output->AddString("can not set encryption");
				addLogLine(this, LOGTYPE_EVENT, 1, "can not set encryption");
			}

			break;
		}

		case AP_MSG_KEY_EXCHANGE:
		{
			if(m_state == ENCRYPTION_NO_SET && g_admin->requireEncryption())
			{
				uint8_t keyType = msg.GetByte();
				switch(keyType)
				{
					case ENCRYPTION_RSA1024XTEA:
					{
						RSA* rsa = g_admin->getRSAKey(ENCRYPTION_RSA1024XTEA);
						if(!rsa)
						{
							output->AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
							addLogLine(this, LOGTYPE_WARNING, 1, "no valid server key type");
							break;
						}

						output->AddByte(AP_MSG_KEY_EXCHANGE_OK);
						output->AddByte(ENCRYPTION_RSA1024XTEA);

						char RSAPublicKey[128];
						rsa->getPublicKey(RSAPublicKey);

						output->AddBytes(RSAPublicKey, 128);
						break;
					}

					default:
					{
						output->AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
						addLogLine(this, LOGTYPE_WARNING, 1, "no valid client key type");
						break;
					}
				}
			}
			else
			{
				output->AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
				output->AddString("can not get public key");
				addLogLine(this, LOGTYPE_WARNING, 1, "can not get public key");
			}

			break;
		}

		case AP_MSG_COMMAND:
		{
			if(m_state != LOGGED_IN)
			{
				addLogLine(this, LOGTYPE_ERROR, 1, "recvbyte == AP_MSG_COMMAND && m_state != LOGGED_IN !!!");
				break;
			}

			uint8_t command = msg.GetByte();
			switch(command)
			{
				case CMD_BROADCAST:
				{
					const std::string param = msg.GetString();
					addLogLine(this, LOGTYPE_EVENT, 1, "broadcasting: " + param);
					Dispatcher::getDispatcher().addTask(createTask(boost::bind(
						&Game::broadcastMessage, &g_game, param, MSG_STATUS_WARNING)));

					output->AddByte(AP_MSG_COMMAND_OK);
					break;
				}

				case CMD_CLOSE_SERVER:
				{
					addLogLine(this, LOGTYPE_EVENT, 1, "closing server");
					Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_CLOSED)));

					output->AddByte(AP_MSG_COMMAND_OK);
					break;
				}

				case CMD_PAY_HOUSES:
				{
					Dispatcher::getDispatcher().addTask(createTask(boost::bind(&ProtocolAdmin::adminCommandPayHouses, this)));
					break;
				}

				case CMD_OPEN_SERVER:
				{
					addLogLine(this, LOGTYPE_EVENT, 1, "opening server");
					Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_NORMAL)));

					output->AddByte(AP_MSG_COMMAND_OK);
					break;
				}

				case CMD_SHUTDOWN_SERVER:
				{
					addLogLine(this, LOGTYPE_EVENT, 1, "starting server shutdown");
					Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_SHUTDOWN)));

					output->AddByte(AP_MSG_COMMAND_OK);
					getConnection()->close();
					break;
				}

				case CMD_KICK:
				{
					const std::string param = msg.GetString();
					Dispatcher::getDispatcher().addTask(createTask(boost::bind(
						&ProtocolAdmin::adminCommandKickPlayer, this, param)));
					break;
				}

				case CMD_SETOWNER:
				{
					const std::string param = msg.GetString();
					Dispatcher::getDispatcher().addTask(createTask(boost::bind(
						&ProtocolAdmin::adminCommandSetOwner, this, param)));
					break;
				}

				default:
				{
					output->AddByte(AP_MSG_COMMAND_FAILED);
					output->AddString("not known server command");
					addLogLine(this, LOGTYPE_WARNING, 1, "not known server command");
				}
			}
			break;
		}

		case AP_MSG_PING:
			output->AddByte(AP_MSG_PING_OK);
			break;

		default:
		{
			output->AddByte(AP_MSG_ERROR);
			output->AddString("not known command byte");

			addLogLine(this, LOGTYPE_WARNING, 1, "not known command byte");
			break;
		}
	}

	if(output->getMessageLength() > 0)
		OutputMessagePool::getInstance()->send(output);
}

void ProtocolAdmin::deleteProtocolTask()
{
	addLogLine(NULL, LOGTYPE_EVENT, 1, "end connection");
	g_admin->removeConnection();
	Protocol::deleteProtocolTask();
}

void ProtocolAdmin::adminCommandPayHouses()
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(!output)
		return;

	TRACK_MESSAGE(output);
	if(Houses::getInstance().payHouses())
	{
		addLogLine(this, LOGTYPE_EVENT, 1, "pay houses ok");
		output->AddByte(AP_MSG_COMMAND_OK);
	}
	else
	{
		addLogLine(this, LOGTYPE_WARNING, 1, "pay houses failed");
		output->AddByte(AP_MSG_COMMAND_FAILED);
		output->AddString(" ");
	}

	OutputMessagePool::getInstance()->send(output);
}

void ProtocolAdmin::adminCommandKickPlayer(const std::string& name)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(!output)
		return;

	TRACK_MESSAGE(output);
	Player* player = NULL;
	if(g_game.getPlayerByNameWildcard(name, player) == RET_NOERROR)
	{
		player->kickPlayer(false);
		addLogLine(this, LOGTYPE_EVENT, 1, "kicked player " + player->getName());
		output->AddByte(AP_MSG_COMMAND_OK);
	}
	else
	{
		addLogLine(this, LOGTYPE_WARNING, 1, "Could not kick player: " + name);
		output->AddByte(AP_MSG_COMMAND_FAILED);
		output->AddString("player is not online");
	}

	OutputMessagePool::getInstance()->send(output);
}

void ProtocolAdmin::adminCommandSetOwner(const std::string& param)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(!output)
		return;

	TRACK_MESSAGE(output);
	StringVec params = explodeString(param, ";");
	std::string houseId = params[0], name = params[1];
	
	trimString(houseId);
	trimString(name);
	if(House* house = Houses::getInstance().getHouse(atoi(houseId.c_str())))
	{
		uint32_t guid;
		if(IOLoginData::getInstance()->getGuidByName(guid, name))
		{
			house->setHouseOwner(guid);
			addLogLine(this, LOGTYPE_EVENT, 1, "set " + name + " as new owner of house with id " + house->getName());
			output->AddByte(AP_MSG_COMMAND_OK);
		}
		else
		{
			addLogLine(this, LOGTYPE_WARNING, 1, "Could not find player with name: " + name);
			output->AddByte(AP_MSG_COMMAND_FAILED);
			output->AddString("such player does not exists");
		}
	}
	else
	{
		addLogLine(this, LOGTYPE_WARNING, 1, "Could not find house with id: " + houseId);
		output->AddByte(AP_MSG_COMMAND_FAILED);
		output->AddString("such house does not exists");
	}

	OutputMessagePool::getInstance()->send(output);
}

bool Admin::loadFromXml()
{
	xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_XML, "admin.xml").c_str());
	if(!doc)
		return false;

	xmlNodePtr root, p, q;
	root = xmlDocGetRootElement(doc);
	if(!xmlStrEqual(root->name,(const xmlChar*)"otadmin"))
	{
		xmlFreeDoc(doc);
		return false;
	}

	std::string strValue;
	if(readXMLString(root, "enabled", strValue))
		m_enabled = booleanString(strValue);

	int32_t intValue;
	p = root->children;
	while(p)
	{
		if(xmlStrEqual(p->name, (const xmlChar*)"security"))
		{
			if(readXMLString(p, "onlylocalhost", strValue))
				m_onlyLocalHost = booleanString(strValue);
			if(readXMLInteger(p, "maxconnections", intValue) && intValue > 0)
				m_maxConnections = intValue;
			if(readXMLString(p, "loginrequired", strValue))
				m_requireLogin = booleanString(strValue);
			if(readXMLString(p, "loginpassword", strValue))
				m_password = strValue;
			else if(m_requireLogin)
				std::cout << "[Warning - Admin::loadFromXml]: Login required, but no password specified - using default." << std::endl;
		}
		else if(xmlStrEqual(p->name, (const xmlChar*)"encryption"))
		{
			if(readXMLString(p, "required", strValue))
				m_requireEncryption = booleanString(strValue);

			q = p->children;
			while(q)
			{
				if(xmlStrEqual(q->name, (const xmlChar*)"key"))
				{
					if(readXMLString(q, "type", strValue))
					{
						if(asLowerCaseString(strValue) == "rsa1024xtea")
						{
							if(readXMLString(q, "file", strValue))
							{
								m_key_RSA1024XTEA = new RSA();
								if(!m_key_RSA1024XTEA->setKey(getFilePath(FILE_TYPE_XML, strValue)))
								{
									delete m_key_RSA1024XTEA;
									m_key_RSA1024XTEA = NULL;
									std::cout << "[Error - Admin::loadFromXml]: Could not load RSA key from file " << getFilePath(FILE_TYPE_XML, strValue) << std::endl;
								}
							}
							else
								std::cout << "[Error - Admin::loadFromXml]: Missing file for RSA1024XTEA key." << std::endl;
						}
						else
							std::cout << "[Warning - Admin::loadFromXml]: " << strValue << " is not a valid key type." << std::endl;
					}
				}

				q = q->next;
			}
		}

		p = p->next;
	}

	xmlFreeDoc(doc);
	return true;
}

bool Admin::addConnection()
{
	if(m_currrentConnections >= m_maxConnections)
		return false;

	m_currrentConnections++;
	return true;
}

void Admin::removeConnection()
{
	if(m_currrentConnections > 0)
		m_currrentConnections--;
}

uint16_t Admin::getProtocolPolicy()
{
	uint16_t policy = 0;
	if(requireLogin())
		policy = policy | REQUIRE_LOGIN;
	if(requireEncryption())
		policy = policy | REQUIRE_ENCRYPTION;

	return policy;
}

uint32_t Admin::getProtocolOptions()
{
	uint32_t ret = 0;
	if(requireEncryption() && m_key_RSA1024XTEA)
		ret = ret | ENCRYPTION_RSA1024XTEA;

	return ret;
}

RSA* Admin::getRSAKey(uint8_t type)
{
	switch(type)
	{
		case ENCRYPTION_RSA1024XTEA:
			return m_key_RSA1024XTEA;

		default:
			break;
	}

	return NULL;
}

bool Admin::allowIP(uint32_t ip)
{
	if(m_onlyLocalHost)
	{
		if(ip == 0x0100007F) //127.0.0.1
			return true;

		char buffer[32];
		formatIP(ip, buffer);
		addLogLine(NULL, LOGTYPE_WARNING, 1, std::string("forbidden connection try from ") + buffer);
		return false;
	}
	else
		return !ConnectionManager::getInstance()->isDisabled(ip, 0xFE);
}

bool Admin::passwordMatch(std::string& password)
{
	//prevent empty password login
	if(!m_password.length())
		return false;

	if(password == m_password)
		return true;

	return false;
}

static void addLogLine(ProtocolAdmin* protocol, LogType_t type, int32_t level, std::string message)
{
	if(g_config.getBool(ConfigManager::ADMIN_LOGS_ENABLED))
		return;

	std::string tmp;
	if(protocol)
	{
		char buffer[32];
		formatIP(protocol->getIP(), buffer);
		tmp += "[";
		tmp += buffer;
		tmp += "] - ";
	}

	tmp += message;
	LOG_MESSAGE("OTADMIN", type, level, tmp);
}
#endif
