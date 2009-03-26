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

#include "protocolold.h"
#include "outputmessage.h"
#include "connection.h"
#include "resources.h"

#include "rsa.h"
#include "game.h"
#ifndef __CONSOLE__
#include "gui.h"
#endif

extern Game g_game;
extern RSA* g_otservRSA;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t ProtocolOld::protocolOldCount = 0;
#endif

#ifdef __DEBUG_NET_DETAIL__
void ProtocolOld::deleteProtocolTask()
{
	std::cout << "Deleting ProtocolOld" << std::endl;
	Protocol::deleteProtocolTask();
}
#endif

void ProtocolOld::disconnectClient(uint8_t error, const char* message)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if(output)
	{
		TRACK_MESSAGE(output);
		output->AddByte(error);
		output->AddString(message);
		OutputMessagePool::getInstance()->send(output);
	}

	getConnection()->close();
}

bool ProtocolOld::parseFirstPacket(NetworkMessage& msg)
{
	if(
#ifndef __CONSOLE__
		!GUI::getInstance()->m_connections ||
#endif
		g_game.getGameState() == GAME_STATE_SHUTDOWN)
	{
		getConnection()->close();
		return false;
	}

	/*uint16_t operatingSystem = */msg.GetU16();
	uint16_t version  = msg.GetU16();
	msg.SkipBytes(12);
	if(version <= 760)
		disconnectClient(0x0A, CLIENT_VERSION_STRING);

	if(!RSA_decrypt(g_otservRSA, msg))
	{
		getConnection()->close();
		return false;
	}

	uint32_t key[4] = {msg.GetU32(), msg.GetU32(), msg.GetU32(), msg.GetU32()};
	enableXTEAEncryption();
	setXTEAKey(key);

	if(version <= 822)
		disableChecksum();

	disconnectClient(0x0A, CLIENT_VERSION_STRING);
	return false;
}

void ProtocolOld::onRecvFirstMessage(NetworkMessage& msg)
{
	parseFirstPacket(msg);
}

