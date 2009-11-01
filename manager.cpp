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
#ifdef __REMOTE_CONTROL__
#include "otpch.h"
#include <iostream>

#include "manager.h"
#include "rsa.h"
#include "tools.h"

#include "configmanager.h"
#include "game.h"

#include "connection.h"
#include "outputmessage.h"
#include "networkmessage.h"

#include "house.h"
#include "town.h"
#include "iologindata.h"

extern Game g_game;
extern ConfigManager g_config;
Manager* g_manager = NULL;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolManager::protocolManagerCount = 0;
#endif

void ProtocolManager::onRecvFirstMessage(NetworkMessage& msg)
{
	if(!g_manager->enabled())
	{
		getConnection()->close();
		return;
	}

	m_state = NO_CONNECTED;
	if(!g_manager->allowIP(getIP()))
	{
		addLogLine(LOGTYPE_EVENT, "ip not allowed");
		getConnection()->close();
		return;
	}

	if(!g_manager->addConnection())
	{
		addLogLine(LOGTYPE_EVENT, "cannot add new connection");
		getConnection()->close();
		return;
	}

	addLogLine(LOGTYPE_EVENT, "sending HELLO");
	if(OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false))
	{
		TRACK_MESSAGE(output);
		output->AddByte(AP_MSG_HELLO);
		output->AddU32(1); //version
		output->AddString("TFManager");
		output->AddU16(g_manager->getProtocolPolicy()); //security policy
		output->AddU32(g_manager->getProtocolOptions()); //protocol options(encryption, ...)
		OutputMessagePool::getInstance()->send(output);
	}

	m_lastCommand = time(NULL);
	m_state = ENCRYPTION_NO_SET;
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
		case ENCRYPTION_NO_SET:
		{
			if(g_manager->requireEncryption())
			{
				if((time(NULL) - m_startTime) > 30000)
				{
					getConnection()->close();
					addLogLine(LOGTYPE_EVENT, "encryption timeout");
					return;
				}

				if(recvbyte != AP_MSG_ENCRYPTION && recvbyte != AP_MSG_KEY_EXCHANGE)
				{
					output->AddByte(AP_MSG_ERROR);
					output->AddString("encryption needed");
					OutputMessagePool::getInstance()->send(output);

					getConnection()->close();
					addLogLine(LOGTYPE_EVENT, "wrong command while ENCRYPTION_NO_SET");
					return;
				}
			}
			else
				m_state = NO_LOGGED_IN;

			break;
		}

		case NO_LOGGED_IN:
		{
			if(g_manager->requireLogin())
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
					output->AddByte(AP_MSG_ERROR);
					output->AddString("too many login tries");
					OutputMessagePool::getInstance()->send(output);

					getConnection()->close();
					addLogLine(LOGTYPE_EVENT, "too many login tries");
					return;
				}

				if(recvbyte != AP_MSG_LOGIN)
				{
					output->AddByte(AP_MSG_ERROR);
					output->AddString("you are not logged in");
					OutputMessagePool::getInstance()->send(output);

					getConnection()->close();
					addLogLine(LOGTYPE_EVENT, "wrong command while NO_LOGGED_IN");
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
			addLogLine(LOGTYPE_EVENT, "no valid connection state!!!");
			return;
		}
	}

	m_lastCommand = time(NULL);
	switch(recvbyte)
	{
		case AP_MSG_LOGIN:
		{
			if(m_state == NO_LOGGED_IN && g_manager->requireLogin())
			{
				std::string password = msg.GetString();
				if(g_manager->passwordMatch(password))
				{
					m_state = LOGGED_IN;
					output->AddByte(AP_MSG_LOGIN_OK);
					addLogLine(LOGTYPE_EVENT, "login ok");
				}
				else
				{
					m_loginTries++;
					output->AddByte(AP_MSG_LOGIN_FAILED);
					output->AddString("wrong password");
					addLogLine(LOGTYPE_EVENT, "login failed.("+ password + ")");
				}
			}
			else
			{
				output->AddByte(AP_MSG_LOGIN_FAILED);
				output->AddString("cannot login");
				addLogLine(LOGTYPE_EVENT, "wrong state at login");
			}

			break;
		}

		case AP_MSG_ENCRYPTION:
		{
			if(m_state == ENCRYPTION_NO_SET && g_manager->requireEncryption())
			{
				uint8_t keyType = msg.GetByte();
				switch(keyType)
				{
					case ENCRYPTION_RSA1024XTEA:
					{
						RSA* rsa = g_manager->getRSAKey(ENCRYPTION_RSA1024XTEA);
						if(!rsa)
						{
							output->AddByte(AP_MSG_ENCRYPTION_FAILED);
							addLogLine(LOGTYPE_EVENT, "no valid server key type");
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
							addLogLine(LOGTYPE_EVENT, "encryption ok");
						}
						else
						{
							output->AddByte(AP_MSG_ENCRYPTION_FAILED);
							output->AddString("wrong encrypted packet");
							addLogLine(LOGTYPE_EVENT, "wrong encrypted packet");
						}

						break;
					}

					default:
					{
						output->AddByte(AP_MSG_ENCRYPTION_FAILED);
						output->AddString("no valid key type");

						addLogLine(LOGTYPE_EVENT, "no valid client key type");
						break;
					}
				}
			}
			else
			{
				output->AddByte(AP_MSG_ENCRYPTION_FAILED);
				output->AddString("cannot set encryption");
				addLogLine(LOGTYPE_EVENT, "cannot set encryption");
			}

			break;
		}

		case AP_MSG_KEY_EXCHANGE:
		{
			if(m_state == ENCRYPTION_NO_SET && g_manager->requireEncryption())
			{
				uint8_t keyType = msg.GetByte();
				switch(keyType)
				{
					case ENCRYPTION_RSA1024XTEA:
					{
						RSA* rsa = g_manager->getRSAKey(ENCRYPTION_RSA1024XTEA);
						if(!rsa)
						{
							output->AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
							addLogLine(LOGTYPE_EVENT, "no valid server key type");
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
						addLogLine(LOGTYPE_EVENT, "no valid client key type");
						break;
					}
				}
			}
			else
			{
				output->AddByte(AP_MSG_KEY_EXCHANGE_FAILED);
				output->AddString("cannot get public key");
				addLogLine(LOGTYPE_EVENT, "cannot get public key");
			}

			break;
		}

		case AP_MSG_COMMAND:
		{
			if(m_state != LOGGED_IN)
			{
				addLogLine(LOGTYPE_EVENT, "recvbyte == AP_MSG_COMMAND && m_state != LOGGED_IN !!!");
				break;
			}

			uint8_t command = msg.GetByte();
			switch(command)
			{
				case CMD_SAVE_SERVER:
				case CMD_SHALLOW_SAVE_SERVER:
				{
					addLogLine(LOGTYPE_EVENT, "saving server");
					Dispatcher::getInstance()->addTask(createTask(boost::bind(
						&Game::saveGameState, &g_game, (command == CMD_SHALLOW_SAVE_SERVER))));

					output->AddByte(AP_MSG_COMMAND_OK);
					break;
				}

				case CMD_CLOSE_SERVER:
				{
					addLogLine(LOGTYPE_EVENT, "closing server");
					Dispatcher::getInstance()->addTask(createTask(boost::bind(
						&Game::setGameState, &g_game, GAME_STATE_CLOSED)));

					output->AddByte(AP_MSG_COMMAND_OK);
					break;
				}

				case CMD_OPEN_SERVER:
				{
					addLogLine(LOGTYPE_EVENT, "opening server");
					g_game.setGameState(GAME_STATE_NORMAL);

					output->AddByte(AP_MSG_COMMAND_OK);
					break;
				}

				case CMD_SHUTDOWN_SERVER:
				{
					addLogLine(LOGTYPE_EVENT, "shutting down server");
					Dispatcher::getInstance()->addTask(createTask(boost::bind(
						&Game::setGameState, &g_game, GAME_STATE_SHUTDOWN)));

					output->AddByte(AP_MSG_COMMAND_OK);
					break;
				}

				case CMD_PAY_HOUSES:
				{
					Dispatcher::getInstance()->addTask(createTask(boost::bind(
						&ProtocolManager::ManagerCommandPayHouses, this)));
					break;
				}

				case CMD_RELOAD_SCRIPTS:
				{
					const int8_t reload = msg.GetByte();
					Dispatcher::getInstance()->addTask(createTask(boost::bind(
						&ProtocolManager::ManagerCommandReload, this, reload)));
					break;
				}

				// why do we run these below on dispatcher thread anyway?
				case CMD_KICK:
				{
					const std::string param = msg.GetString();
					Dispatcher::getInstance()->addTask(createTask(boost::bind(
						&ProtocolManager::ManagerCommandKickPlayer, this, param)));
					break;
				}

				case CMD_SEND_MAIL:
				{
					const std::string xmlData = msg.GetString();
					Dispatcher::getInstance()->addTask(createTask(boost::bind(
						&ProtocolManager::ManagerCommandSendMail, this, xmlData)));
					break;
				}

				case CMD_BROADCAST:
				{
					const std::string param = msg.GetString();
					addLogLine(LOGTYPE_EVENT, "broadcasting: " + param);
					Dispatcher::getInstance()->addTask(createTask(boost::bind(
						&Game::broadcastMessage, &g_game, param, MSG_STATUS_WARNING)));

					output->AddByte(AP_MSG_COMMAND_OK);
					break;
				}

				default:
				{
					output->AddByte(AP_MSG_COMMAND_FAILED);
					output->AddString("not known server command");
					addLogLine(LOGTYPE_EVENT, "not known server command");
				}
			}
			break;
		}

		case AP_MSG_PING:
			output->AddByte(AP_MSG_PING_OK);
			break;

		case AP_MSG_KEEP_ALIVE:
			break;

		default:
		{
			output->AddByte(AP_MSG_ERROR);
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
	g_manager->removeConnection();
	Protocol::deleteProtocolTask();
}

void ProtocolManager::ManagerCommandPayHouses()
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(!output)
		return;

	Houses::getInstance()->payHouses();
	addLogLine(LOGTYPE_EVENT, "pay houses ok");

	TRACK_MESSAGE(output);
	output->AddByte(AP_MSG_COMMAND_OK);
	OutputMessagePool::getInstance()->send(output);
}

void ProtocolManager::ManagerCommandReload(int8_t reload)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(!output)
		return;

	g_game.reloadInfo((ReloadInfo_t)reload);
	addLogLine(LOGTYPE_EVENT, "reload ok");

	TRACK_MESSAGE(output);
	output->AddByte(AP_MSG_COMMAND_OK);
	OutputMessagePool::getInstance()->send(output);
}

void ProtocolManager::ManagerCommandKickPlayer(const std::string& param)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(!output)
		return;

	TRACK_MESSAGE(output);
	Player* player = NULL;
	if(g_game.getPlayerByNameWildcard(param, player) == RET_NOERROR)
	{
		Scheduler::getInstance()->addEvent(createSchedulerTask(SCHEDULER_MINTICKS, boost::bind(&Game::kickPlayer, &g_game, player->getID(), false)));
		addLogLine(LOGTYPE_EVENT, "kicking player " + player->getName());
		output->AddByte(AP_MSG_COMMAND_OK);
	}
	else
	{
		addLogLine(LOGTYPE_EVENT, "failed setting kick for player " + param);
		output->AddByte(AP_MSG_COMMAND_FAILED);
		output->AddString("player is not online");
	}

	OutputMessagePool::getInstance()->send(output);
}

void ProtocolManager::ManagerCommandSendMail(const std::string& xmlData)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(!output)
		return;

	std::string name;
	uint32_t depotId;

	TRACK_MESSAGE(output);
	if(Item* mailItem = Manager::createMail(xmlData, name, depotId))
	{
		if(IOLoginData::getInstance()->playerMail(NULL, name, depotId, mailItem))
		{
			addLogLine(LOGTYPE_EVENT, "sent mailbox to " + name);
			output->AddByte(AP_MSG_COMMAND_OK);
		}
		else
		{
			addLogLine(LOGTYPE_EVENT, "failed sending mailbox to " + name);
			output->AddByte(AP_MSG_COMMAND_FAILED);
			output->AddString("could not send the box");
		}
	}
	else
	{
		addLogLine(LOGTYPE_EVENT, "failed parsing mailbox");
		output->AddByte(AP_MSG_COMMAND_FAILED);
		output->AddString("could not parse the box");
	}

	OutputMessagePool::getInstance()->send(output);
}

bool Manager::loadFromXml()
{
	xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_XML, "manager.xml").c_str());
	if(!doc)
	{
		std::cout << "[Warning - Manager::loadFromXml] Cannot load manager file." << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr p, q, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"tfmanager"))
	{
		std::cout << "[Error - Manager::loadFromXml] Malformed manager file" << std::endl;
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
				std::cout << "[Warning - Manager::loadFromXml]: Login required, but no password specified - using default." << std::endl;
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
									std::cout << "[Error - Manager::loadFromXml]: Could not load RSA key from file " << getFilePath(FILE_TYPE_XML, strValue) << std::endl;
								}
							}
							else
								std::cout << "[Error - Manager::loadFromXml]: Missing file for RSA1024XTEA key." << std::endl;
						}
						else
							std::cout << "[Warning - Manager::loadFromXml]: " << strValue << " is not a valid key type." << std::endl;
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

bool Manager::addConnection()
{
	if(m_currrentConnections >= m_maxConnections)
		return false;

	m_currrentConnections++;
	return true;
}

void Manager::removeConnection()
{
	if(m_currrentConnections > 0)
		m_currrentConnections--;
}

uint16_t Manager::getProtocolPolicy()
{
	uint16_t policy = 0;
	if(requireLogin())
		policy |= REQUIRE_LOGIN;
	if(requireEncryption())
		policy |= REQUIRE_ENCRYPTION;

	return policy;
}

uint32_t Manager::getProtocolOptions()
{
	uint32_t ret = 0;
	if(requireEncryption() && m_key_RSA1024XTEA)
		ret |= ENCRYPTION_RSA1024XTEA;

	return ret;
}

RSA* Manager::getRSAKey(uint8_t type)
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

Item* Manager::createMail(const std::string xmlData, std::string& name, uint32_t& depotId)
{
	xmlDocPtr doc = xmlParseMemory(xmlData.c_str(), xmlData.length());
	if(!doc)
		return NULL;

	xmlNodePtr root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"mail"))
		return NULL;

	int32_t intValue;
	std::string strValue;

	int32_t itemId = ITEM_PARCEL;
	if(readXMLString(root, "to", strValue))
		name = strValue;

	if(readXMLString(root, "town", strValue))
	{
		Town* town = Towns::getInstance()->getTown(strValue);
		if(!town)
			return false;

		depotId = town->getID();
	}
	else if(!IOLoginData::getInstance()->getDefaultTownByName(name, depotId)) //use the players default town
		return false;

	if(readXMLInteger(root, "id", intValue))
		itemId = intValue;

	Item* mailItem = Item::CreateItem(itemId);
	mailItem->setParent(VirtualCylinder::virtualCylinder);
	if(Container* mailContainer = mailItem->getContainer())
	{
		xmlNodePtr node = root->children;
		while(node)
		{
			if(node->type != XML_ELEMENT_NODE)
			{
				node = node->next;
				continue;
			}

			if(!Item::loadItem(node, mailContainer))
			{
				delete mailContainer;
				return NULL;
			}

			node = node->next;
		}
	}

	return mailItem;
}

bool Manager::allowIP(uint32_t ip)
{
	if(!m_onlyLocalHost)
		return !ConnectionManager::getInstance()->isDisabled(ip, 0xFE);

	if(ip == 0x0100007F) //127.0.0.1
		return true;

	if(g_config.getBool(ConfigManager::MANAGER_LOGS_ENABLED))
		LOG_MESSAGE(LOGTYPE_EVENT, "forbidden connection try", "Manager " + convertIPAddress(ip));

	return false;
}

bool Manager::passwordMatch(const std::string& password)
{
	//prevent empty password login
	if(!m_password.length())
		return false;

	return password == m_password;
}

void ProtocolManager::addLogLine(LogType_t type, std::string message)
{
	if(g_config.getBool(ConfigManager::MANAGER_LOGS_ENABLED))
		LOG_MESSAGE(type, message, "Manager " + convertIPAddress(getIP()))
}
#endif
