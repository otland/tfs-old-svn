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

#ifndef __OTSERV_CONNECTION_H__
#define __OTSERV_CONNECTION_H__
#include "otsystem.h"
#include "networkmessage.h"

class Connection;
class Protocol;
class OutputMessage;
typedef boost::shared_ptr<OutputMessage>OutputMessage_ptr;
class ServiceBase;
typedef boost::shared_ptr<ServiceBase> Service_ptr;
class ServicePort;
typedef boost::shared_ptr<ServicePort> ServicePort_ptr;

#ifdef __DEBUG_NET__
#define PRINT_ASIO_ERROR(desc) \
	std::cout << "[Error - " << __FUNCTION__ << "]: " << desc << " - Value: " <<  \
		error.value() << " Message: " << error.message() << std::endl;
#else
#define PRINT_ASIO_ERROR(desc)
#endif

struct LoginBlock
{
	int32_t lastLogin, lastProtocol, loginsAmount;
};

struct ConnectBlock
{
	uint32_t count;
	uint64_t startTime, blockTime;
};

class ConnectionManager
{
	public:
		virtual ~ConnectionManager() {OTSYS_THREAD_LOCKVARRELEASE(m_connectionManagerLock);}

		static ConnectionManager* getInstance()
		{
			static ConnectionManager instance;
			return &instance;
		}

		Connection* createConnection(boost::asio::ip::tcp::socket* socket, ServicePort_ptr servicer);
		void releaseConnection(Connection* connection);

		bool isDisabled(uint32_t clientIp, int32_t protocolId);
		void addAttempt(uint32_t clientIp, int32_t protocolId, bool success);

		bool acceptConnection(uint32_t clientIp);
		void closeAll();

	protected:
		ConnectionManager() {OTSYS_THREAD_LOCKVARINIT(m_connectionManagerLock);}

		typedef std::map<uint32_t, LoginBlock> IpLoginMap;
		IpLoginMap ipLoginMap;

		typedef std::map<uint32_t, ConnectBlock> IpConnectMap;
		IpConnectMap ipConnectMap;

		std::list<Connection*> m_connections;
		OTSYS_THREAD_LOCKVAR m_connectionManagerLock;
};

class Connection : boost::noncopyable
{
	public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
		static uint32_t connectionCount;
#endif
		enum
		{
			CLOSE_STATE_NONE = 0,
			CLOSE_STATE_REQUESTED = 1,
			CLOSE_STATE_CLOSING = 2
		};

	private:
		Connection(boost::asio::ip::tcp::socket* socket, ServicePort_ptr servicer): m_socket(socket), m_port(servicer)
		{
			m_protocol = NULL;
			m_closeState = CLOSE_STATE_NONE;
			m_refCount = m_pendingWrite = m_pendingRead = 0;
			m_receivedFirst = m_socketClosed = m_writeError = m_readError = false;

			OTSYS_THREAD_LOCKVARINIT(m_connectionLock);
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
			connectionCount++;
#endif
		}

		friend class ConnectionManager;

	public:
		virtual ~Connection()
		{
			ConnectionManager::getInstance()->releaseConnection(this);
			OTSYS_THREAD_LOCKVARRELEASE(m_connectionLock);
#ifdef __ENABLE_SERVER_DIAGNOSTIC__

			connectionCount--;
#endif
			delete m_socket;
		}

		boost::asio::ip::tcp::socket& getHandle() {return *m_socket;}
		uint32_t getIP() const;

		void handle(Protocol* protocol);
		void accept();

		void close();	
		bool send(OutputMessage_ptr msg);

		int32_t addRef() {return ++m_refCount;}
		int32_t unRef() {return --m_refCount;}

	private:
		void internalSend(OutputMessage_ptr msg);

		void closeConnection();
		void releaseConnection();
		void deleteConnection();

		void parseHeader(const boost::system::error_code& error);
		void parsePacket(const boost::system::error_code& error);

		bool write();
		void onWrite(OutputMessage_ptr msg, const boost::system::error_code& error);

		void handleReadError(const boost::system::error_code& error);
		void handleWriteError(const boost::system::error_code& error);

		NetworkMessage m_msg;
		Protocol* m_protocol;

		boost::asio::ip::tcp::socket* m_socket;
		ServicePort_ptr m_port;

		bool m_receivedFirst, m_socketClosed, m_writeError, m_readError;

		uint32_t m_closeState, m_refCount;
		int32_t m_pendingWrite, m_pendingRead;

		typedef std::list<OutputMessage_ptr> OutputQueue;
		OutputQueue m_outputQueue;

		OTSYS_THREAD_LOCKVAR m_connectionLock;
};

#endif
