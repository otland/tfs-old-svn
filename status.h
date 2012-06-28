//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Status-Singleton for OTServ
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

#ifndef __OTSERV_STATUS_H
#define __OTSERV_STATUS_H

#include <string>
#include "definitions.h"
#include "networkmessage.h"
#include "protocol.h"

class ProtocolStatus : public Protocol
{
	public:
		// static protocol information
		enum {server_sends_first = false};
		enum {protocol_identifier = 0xFF};
		enum {use_checksum = false};
		static const char* protocol_name() {return "status protocol";}

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
		static uint32_t protocolStatusCount;
#endif
		ProtocolStatus(Connection_ptr connection) : Protocol(connection)
		{
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
			protocolStatusCount++;
#endif
		}
		virtual ~ProtocolStatus()
		{
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
			protocolStatusCount--;
#endif
		}

		virtual int32_t getProtocolId() {return 0xFF;}

		virtual void onRecvFirstMessage(NetworkMessage& msg);

	protected:
		static std::map<uint32_t, int64_t> ipConnectMap;

		#ifdef __DEBUG_NET_DETAIL__
		virtual void deleteProtocolTask();
		#endif
};

class Status
{
	public:
		static Status* getInstance()
		{
			static Status status;
			return &status;
		}

		void addPlayer();
		void removePlayer();

		std::string getStatusString() const;
		void getInfo(uint32_t requestedInfo, OutputMessage_ptr output, NetworkMessage& msg) const;

		uint32_t getPlayersOnline() const {return m_playersOnline;}
		uint32_t getMaxPlayersOnline() const {return m_playersMax;}
		void setMaxPlayersOnline(uint32_t max) {m_playersMax = max;}

		const std::string& getMapName() const {return m_mapName;}
		void setMapName(std::string mapName) {m_mapName = mapName;}
		void setMapAuthor(std::string mapAuthor) {m_mapAuthor = mapAuthor;}

		uint64_t getUptime() const;

	protected:
		Status();

	private:
		uint64_t m_start;
		uint32_t m_playersMax, m_playersOnline;
		std::string m_mapName, m_mapAuthor;
};

#endif
