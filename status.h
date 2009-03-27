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
#include "otsystem.h"
#include "protocol.h"

enum RequestedInfo_t
{
	REQUEST_BASIC_SERVER_INFO 	= 0x01,
	REQUEST_SERVER_OWNER_INFO	= 0x02,
	REQUEST_MISC_SERVER_INFO	= 0x04,
	REQUEST_PLAYERS_INFO		= 0x08,
	REQUEST_SERVER_MAP_INFO		= 0x10,
	REQUEST_EXT_PLAYERS_INFO	= 0x20,
	REQUEST_PLAYER_STATUS_INFO	= 0x40,
	REQUEST_SERVER_SOFTWARE_INFO	= 0x80
};

typedef std::map<uint32_t, int64_t> IpConnectMap;

class ProtocolStatus : public Protocol
{
	public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
		static uint32_t protocolStatusCount;
#endif
		ProtocolStatus(Connection* connection): Protocol(connection)
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

		enum {protocolId = 0xFF};
		enum {isSingleSocket = false};
		enum {hasChecksum = false};

		virtual void onRecvFirstMessage(NetworkMessage& msg);

	protected:
		static IpConnectMap ipConnectMap;

		virtual void deleteProtocolTask();
};

class Status
{
	public:
		virtual ~Status() {}
		static Status* getInstance()
		{
			static Status status;
			return &status;
		}

		void addPlayer() {m_playersOnline++;}
		void removePlayer() {m_playersOnline--;}
		bool hasSlot() const {return m_playersMax > m_playersOnline;}

		std::string getStatusString() const;
		void getInfo(uint32_t requestedInfo, OutputMessage_ptr output, NetworkMessage& msg) const;

		uint32_t getPlayersOnline() const {return m_playersOnline;}
		uint32_t getMaxPlayersOnline() const {return m_playersMax;}
		void setMaxPlayersOnline(uint32_t max) {m_playersMax = max;}

		const std::string& getMapName() const {return m_mapName;}
		void setMapName(std::string mapName) {m_mapName = mapName;}
		void setMapAuthor(std::string mapAuthor) {m_mapAuthor = mapAuthor;}

		uint64_t getUptime() const {return (OTSYS_TIME() - m_start) / 1000;}

	protected:
		Status()
		{
			m_start = OTSYS_TIME();
			m_playersOnline = m_playersMax = 0;
		}

	private:
		uint64_t m_start;
		uint32_t m_playersMax, m_playersOnline;
		std::string m_mapName, m_mapAuthor;
};

#endif
