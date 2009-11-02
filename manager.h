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
#ifndef __MANAGER__
#define __MANAGER__
#include "otsystem.h"

#include "textlogger.h"
#include "player.h"

enum
{
	MP_MSG_LOGIN = 1,
	MP_MSG_ENCRYPTION = 2,
	MP_MSG_KEY_EXCHANGE = 3,
	MP_MSG_COMMAND = 4,
	MP_MSG_PING = 5,
	MP_MSG_KEEP_ALIVE = 6,

	MP_MSG_HELLO = 1,
	MP_MSG_KEY_EXCHANGE_OK = 2,
	MP_MSG_KEY_EXCHANGE_FAILED = 3,
	MP_MSG_LOGIN_OK = 4,
	MP_MSG_LOGIN_FAILED = 5,
	MP_MSG_COMMAND_OK = 6,
	MP_MSG_COMMAND_FAILED = 7,
	MP_MSG_ENCRYPTION_OK = 8,
	MP_MSG_ENCRYPTION_FAILED = 9,
	MP_MSG_PING_OK = 10,
	MP_MSG_MESSAGE = 11,
	MP_MSG_ERROR = 12,
};

enum
{
	CMD_BROADCAST = 1,
	CMD_CLOSE_SERVER = 2,
	CMD_PAY_HOUSES = 3,
	CMD_OPEN_SERVER = 4,
	CMD_SHUTDOWN_SERVER = 5,
	CMD_RELOAD_SCRIPTS = 6,
	//CMD_PLAYER_INFO = 7,
	//CMD_GETONLINE = 8,
	CMD_KICK = 9,
	//CMD_BAN_MANAGER = 10,
	//CMD_SERVER_INFO = 11,
	//CMD_GETHOUSE = 12,
	CMD_SAVE_SERVER = 13,
	CMD_SEND_MAIL = 14,
	CMD_SHALLOW_SAVE_SERVER = 15
};

class NetworkMessage;

class Manager
{
	public:
		Manager()
		{
			m_currrentConnections = 0;
		}

		virtual ~Manager()
		{
		}
		
		static Manager *getInstance()
		{
            static Manager instance;
            return &instance;
        }
		bool addConnection();
		void removeConnection();
		static Item* createMail(const std::string xmlData, std::string& name, uint32_t& depotId);
		bool allowIP(uint32_t ip);
		bool passwordMatch(std::string password);

	protected:
		int32_t m_currrentConnections;
};


class ProtocolManager : public Protocol
{
	public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
		static uint32_t protocolManagerCount;
#endif
		virtual void onRecvFirstMessage(NetworkMessage& msg);

		ProtocolManager(Connection_ptr connection): Protocol(connection)
		{
			m_state = NO_CONNECTED;
			m_loginTries = m_lastCommand = 0;
			m_startTime = time(NULL);
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
			protocolManagerCount++;
#endif
		}
		virtual ~ProtocolManager()
		{
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
			protocolManagerCount--;
#endif
		}

		enum {protocolId = 0xFD};
		enum {isSingleSocket = false};
		enum {hasChecksum = false};
		static const char* protocolName() {return "manager protocol";}

	protected:
		virtual void parsePacket(NetworkMessage& msg);
		virtual void deleteProtocolTask();

		void ManagerCommandPayHouses();
		void ManagerCommandReload(int8_t reload);

		void ManagerCommandKickPlayer(const std::string& name);
		void ManagerCommandSendMail(const std::string& xmlData);

		enum ProtocolState_t
		{
			NO_CONNECTED,
			NO_LOGGED_IN,
			LOGGED_IN,
		};

	private:
		void addLogLine(LogType_t type, std::string message);

		int32_t m_loginTries;
		ProtocolState_t m_state;
		uint32_t m_lastCommand, m_startTime;
};
#endif
