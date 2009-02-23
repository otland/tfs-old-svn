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
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

#include "networkmessage.h"

class Protocol;
class OutputMessage;
class Connection;

#ifdef __DEBUG_NET__
#define PRINT_ASIO_ERROR(desc) \
	std::cout << "[Error - " << __FUNCTION__ << "]: " << desc << " - Value: " <<  \
		error.value() << " Message: " << error.message() << std::endl;
#else
#define PRINT_ASIO_ERROR(desc)
#endif

typedef boost::shared_ptr<OutputMessage>OutputMessage_ptr;
struct ConnectionBlock
{
	uint32_t lastLogin, loginsAmount;
};

class ConnectionManager
{
	public:
		virtual ~ConnectionManager()
		{
			OTSYS_THREAD_LOCKVARRELEASE(m_connectionManagerLock);
		}

		static ConnectionManager* getInstance()
		{
			static ConnectionManager instance;
			return &instance;
		}

		Connection* createConnection(boost::asio::io_service& io_service);
		void releaseConnection(Connection* connection);

		bool isDisabled(uint32_t clientIp);
		void addAttempt(uint32_t clientIp, bool success);

		void closeAll();

	protected:
		ConnectionManager();
		uint32_t loginTimeout, maxLoginTries, retryTimeout;

		typedef std::map<uint32_t, ConnectionBlock > IpConnectionMap;
		IpConnectionMap ipConnectionMap;

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
		Connection(boost::asio::io_service& io_service) : m_socket(io_service)
		{
			m_refCount = 0;
			m_protocol = NULL;
			m_pendingWrite = m_pendingRead = 0;
			m_closeState = CLOSE_STATE_NONE;
			m_socketClosed = false;
			m_writeError = m_readError = false;
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
		}

		boost::asio::ip::tcp::socket& getHandle() {return m_socket;}
		uint32_t getIP() const;

		void closeConnection();
		void acceptConnection();

		bool send(OutputMessage_ptr msg);
		int32_t addRef() {return ++m_refCount;}
		int32_t unRef() {return --m_refCount;}

	private:
		void parseHeader(const boost::system::error_code& error);
		void parsePacket(const boost::system::error_code& error);

		void onWriteOperation(OutputMessage_ptr msg, const boost::system::error_code& error);

		void handleReadError(const boost::system::error_code& error);
		void handleWriteError(const boost::system::error_code& error);

		bool closingConnection();
		void closeConnectionTask();
		void deleteConnectionTask();
		void releaseConnection();

		void internalSend(OutputMessage_ptr msg);

		NetworkMessage m_msg;
		boost::asio::ip::tcp::socket m_socket;
		bool m_socketClosed;

		bool m_writeError;
		bool m_readError;

		int32_t m_pendingWrite;
		std::list <OutputMessage_ptr> m_outputQueue;
		int32_t m_pendingRead;
		uint32_t m_closeState;
		uint32_t m_refCount;

		OTSYS_THREAD_LOCKVAR m_connectionLock;
		Protocol* m_protocol;
};

#endif
