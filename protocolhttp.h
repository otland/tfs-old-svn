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

#ifndef __PROTOCOL_HTTP__
#define __PROTOCOL_HTTP__
#include "protocol.h"

class NetworkMessage;
class OutputMessage;

class ProtocolHTTP : public Protocol
{
	public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
		static uint32_t protocolHTTPCount;
#endif
		virtual void onRecvFirstMessage(NetworkMessage& msg) {parseFirstPacket(msg);}

		ProtocolHTTP(Connection* connection) : Protocol(connection)
		{
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
			protocolHTTPCount++;
#endif
			disableXTEAEncryption();
			disableChecksum();
		}
		virtual ~ProtocolHTTP()
		{
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
			protocolHTTPCount--;
#endif
		}

		enum {protocolId = 0x00};
		enum {isSingleSocket = true};
		enum {hasChecksum = false};
		static const char* protocolName() {return "http protocol";}

	protected:
		virtual void deleteProtocolTask();

		void disconnectClient();
		bool parseFirstPacket(NetworkMessage& msg);
};
#endif
