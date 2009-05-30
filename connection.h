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

#ifndef __CONNECTION__
#define __CONNECTION__
#include "otsystem.h"

#include "networkmessage.h"
#include <boost/enable_shared_from_this.hpp>

#ifdef __DEBUG_NET__
#define PRINT_ASIO_ERROR(desc) \
	std::cout << "[Error - " << __FUNCTION__ << "] " << desc << "- value: " <<  \
		error.value() << ", message: " << error.message() << std::endl;
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

class Connection;
typedef boost::shared_ptr<Connection> Connection_ptr;
class ServiceBase;
typedef boost::shared_ptr<ServiceBase> Service_ptr;
class ServicePort;
typedef boost::shared_ptr<ServicePort> ServicePort_ptr;

class ConnectionManager
{
	public:
		virtual ~ConnectionManager() {OTSYS_THREAD_LOCKVARRELEASE(m_connectionManagerLock);}

		static ConnectionManager* getInstance()
		{
			static ConnectionManager instance;
			return &instance;
		}

		Connection_ptr createConnection(boost::asio::ip::tcp::socket* socket,
			boost::asio::io_service& io_service, ServicePort_ptr servicer);
		void releaseConnection(Connection_ptr connection);

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

		std::list<Connection_ptr> m_connections;
		OTSYS_THREAD_LOCKVAR m_connectionManagerLock;
};

class OutputMessage;
typedef boost::shared_ptr<OutputMessage>OutputMessage_ptr;

class Protocol;
class Connection : public boost::enable_shared_from_this<Connection>, boost::noncopyable
{
	public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
		static uint32_t connectionCount;
#endif
		enum ConnectionState_t
		{
			CONNECTION_STATE_OPEN = 0,
			CONNECTION_STATE_REQUEST_CLOSE = 1,
			CONNECTION_STATE_CLOSING = 2,
			CONNECTION_STATE_CLOSED = 3,
		};

	private:
		Connection(boost::asio::ip::tcp::socket* socket, boost::asio::io_service& io_service,
			ServicePort_ptr servicePort): m_readTimer(io_service), m_writeTimer(io_service),
			m_socket(socket), m_io_service(io_service), m_service_port(servicePort)
		{
			m_connectionState = CONNECTION_STATE_OPEN;
			m_protocol = NULL;
			m_refCount = m_pendingWrite = m_pendingRead = 0;
			m_receivedFirst = m_writeError = m_readError = false;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
			connectionCount++;
#endif
		}

		friend class ConnectionManager;

	public:
		virtual ~Connection()
		{
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
			connectionCount--;
#endif
		}

		bool send(OutputMessage_ptr msg);
		void onClose();

		void handle(Protocol* protocol);
		void accept();

		boost::asio::ip::tcp::socket& getHandle() {return *m_socket;}
		uint32_t getIP() const;

		int32_t addRef() {return ++m_refCount;}
		int32_t unRef() {return --m_refCount;}

	private:
		void internalSend(OutputMessage_ptr msg);
		void closeSocket();

		void onWrite(OutputMessage_ptr msg, const boost::system::error_code& error);
		void onStop();

		void parseHeader(const boost::system::error_code& error);
		void parsePacket(const boost::system::error_code& error);

		void handleReadError(const boost::system::error_code& error);
		void handleWriteError(const boost::system::error_code& error);

		static void handleReadTimeout(boost::weak_ptr<Connection> weakConnection, const boost::system::error_code& error);
		static void handleWriteTimeout(boost::weak_ptr<Connection> weakConnection, const boost::system::error_code& error);

		void closeConnection();
		void deleteConnection();
		void releaseConnection();

		NetworkMessage m_msg;
		Protocol* m_protocol;

		boost::asio::deadline_timer m_readTimer;
		boost::asio::deadline_timer m_writeTimer;

		boost::asio::ip::tcp::socket* m_socket;
		boost::asio::io_service& m_io_service;
		ServicePort_ptr m_service_port;

		bool m_receivedFirst, m_writeError, m_readError;
		int32_t m_pendingWrite, m_pendingRead;
		uint32_t m_refCount;

		static bool m_logError;
		ConnectionState_t m_connectionState;
		OTSYS_THREAD_LOCKVAR m_connectionLock;
};
#endif
