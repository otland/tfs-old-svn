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
#include "otpch.h"

#include "protocollogin.h"
#include "resources.h"

#include "outputmessage.h"
#include "connection.h"
#include "rsa.h"

#include "configmanager.h"
#include "tools.h"
#include "iologindata.h"
#include "ioban.h"
#include <iomanip>
#include "game.h"
#ifndef __CONSOLE__
#include "gui.h"
#endif

extern RSA* g_otservRSA;
extern ConfigManager g_config;
extern IPList serverIPs;
extern Game g_game;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolLogin::protocolLoginCount = 0;
#endif

#ifdef __DEBUG_NET_DETAIL__
void ProtocolLogin::deleteProtocolTask()
{
	std::cout << "Deleting ProtocolLogin" << std::endl;
	Protocol::deleteProtocolTask();
}
#endif

void ProtocolLogin::disconnectClient(uint8_t error, const char* message)
{
	OutputMessage* output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	TRACK_MESSAGE(output);
	output->AddByte(error);
	output->AddString(message);
	OutputMessagePool::getInstance()->send(output);
	getConnection()->closeConnection();
}

bool ProtocolLogin::parseFirstPacket(NetworkMessage& msg)
{
	if(
#ifndef __CONSOLE__
		!GUI::getInstance()->m_connections ||
#endif
		g_game.getGameState() == GAME_STATE_SHUTDOWN)
	{
		getConnection()->closeConnection();
		return false;
	}

	uint32_t clientip = getConnection()->getIP();

	/*uint16_t clientos =*/ msg.GetU16();
	uint16_t version  = msg.GetU16();
	msg.SkipBytes(12);

	if(version <= 760)
		disconnectClient(0x0A, CLIENT_VERSION_STRING);

	if(!RSA_decrypt(g_otservRSA, msg))
	{
		getConnection()->closeConnection();
		return false;
	}

	uint32_t key[4];
	key[0] = msg.GetU32();
	key[1] = msg.GetU32();
	key[2] = msg.GetU32();
	key[3] = msg.GetU32();
	enableXTEAEncryption();
	setXTEAKey(key);

	uint32_t accnumber = msg.GetU32();
	std::string password = msg.GetString();

	if(!accnumber)
	{
		if(g_config.getBool(ConfigManager::ACCOUNT_MANAGER))
		{
			accnumber = 1;
			password = "1";
		}
		else
		{
			disconnectClient(0x0A, "You must enter your account number.");
			return false;
		}
	}

	if(version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX)
	{
		disconnectClient(0x0A, CLIENT_VERSION_STRING);
		return false;
	}

	if(g_game.getGameState() == GAME_STATE_STARTUP)
	{
		disconnectClient(0x0A, "Gameworld is starting up. Please wait.");
		return false;
	}

	if(g_game.getGameState() == GAME_STATE_MAINTAIN)
	{
		disconnectClient(0x0A, "Gameworld is under maintenance. Please re-connect in a while.");
		return false;
	}

	if(ConnectionManager::getInstance()->isDisabled(clientip))
	{
		disconnectClient(0x0A, "Too many connections attempts from this IP. Please try again later.");
		return false;
	}

	if(IOBan::getInstance()->isIpBanished(clientip))
	{
		disconnectClient(0x0A, "Your IP is banished!");
		return false;
	}

	uint32_t serverip = serverIPs[0].first;
	for(uint32_t i = 0; i < serverIPs.size(); i++)
	{
		if((serverIPs[i].first & serverIPs[i].second) == (clientip & serverIPs[i].second))
		{
			serverip = serverIPs[i].first;
			break;
		}
	}

	Account account = IOLoginData::getInstance()->loadAccount(accnumber);
	if(!(accnumber != 0 && account.accnumber == accnumber &&
			passwordTest(password, account.password)))
	{
		ConnectionManager::getInstance()->addLoginAttempt(clientip, false);
		disconnectClient(0x0A, "Account number or password is not correct.");
		return false;
	}

	ConnectionManager::getInstance()->addLoginAttempt(clientip, true);
	OutputMessage* output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	TRACK_MESSAGE(output);

	//Remove premium days
	IOLoginData::getInstance()->removePremium(account);

	//Add MOTD
	output->AddByte(0x14);
	char motd[1300];
	sprintf(motd, "%d\n%s", g_game.getMotdNum(), g_config.getString(ConfigManager::MOTD).c_str());
	output->AddString(motd);

	//Add char list
	output->AddByte(0x64);
	if(accnumber != 1 && g_config.getBool(ConfigManager::ACCOUNT_MANAGER))
	{
		output->AddByte((uint8_t)account.charList.size() + 1);
		output->AddString("Account Manager");
		output->AddString(g_config.getString(ConfigManager::SERVER_NAME));
		output->AddU32(serverip);
		output->AddU16(g_config.getNumber(ConfigManager::PORT));
	}
	else
		output->AddByte((uint8_t)account.charList.size());

	for(std::list<std::string>::iterator it = account.charList.begin(); it != account.charList.end(); it++)
	{
		output->AddString((*it));
		if(g_config.getBool(ConfigManager::ON_OR_OFF_CHARLIST))
		{
			if(g_game.getPlayerByName((*it)))
				output->AddString("Online");
			else
				output->AddString("Offline");
		}
		else
			output->AddString(g_config.getString(ConfigManager::SERVER_NAME));
		output->AddU32(serverip);
		output->AddU16(g_config.getNumber(ConfigManager::PORT));
	}

	//Add premium days
	if(g_config.getBool(ConfigManager::FREE_PREMIUM))
		output->AddU16(65535); //client displays free premium
	else
		output->AddU16(account.premiumDays);

	OutputMessagePool::getInstance()->send(output);
	getConnection()->closeConnection();
	return true;
}

void ProtocolLogin::onRecvFirstMessage(NetworkMessage& msg)
{
	parseFirstPacket(msg);
}
