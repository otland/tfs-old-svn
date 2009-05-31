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
#include "otpch.h"
#include "connection.h"

#include "tools.h"
#include "textlogger.h"

#include "configmanager.h"
#include "outputmessage.h"
#include "server.h"
#include "protocol.h"

#include "tasks.h"
#include "scheduler.h"

extern ConfigManager g_config;

bool Connection::m_logError = true;
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t Connection::connectionCount = 0;
#endif

Connection* ConnectionManager::createConnection(boost::asio::ip::tcp::socket* socket,
	boost::asio::io_service& io_service, ServicePort_ptr servicer)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Creating new connection" << std::endl;
	#endif
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionManagerLock);

	Connection* connection = new Connection(socket, io_service, servicer);
	m_connections.push_back(connection);
	return connection;
}

void ConnectionManager::releaseConnection(Connection* connection)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Releasing connection" << std::endl;
	#endif
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionManagerLock);

	std::list<Connection*>::iterator it = std::find(m_connections.begin(), m_connections.end(), connection);
	if(it != m_connections.end())
		m_connections.erase(it);
	else
		std::cout << "[Error - ConnectionManager::releaseConnection] Connection not found" << std::endl;
}

bool ConnectionManager::isDisabled(uint32_t clientIp, int32_t protocolId)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionManagerLock);
	int32_t maxLoginTries = g_config.getNumber(ConfigManager::LOGIN_TRIES);
	if(maxLoginTries == 0 || clientIp == 0)
		return false;

	IpLoginMap::const_iterator it = ipLoginMap.find(clientIp);
	return it != ipLoginMap.end() && it->second.lastProtocol != protocolId && it->second.loginsAmount > maxLoginTries
		&& (int32_t)time(NULL) < it->second.lastLogin + g_config.getNumber(ConfigManager::LOGIN_TIMEOUT) / 1000;
}

void ConnectionManager::addAttempt(uint32_t clientIp, int32_t protocolId, bool success)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionManagerLock);
	if(!clientIp)
		return;

	IpLoginMap::iterator it = ipLoginMap.find(clientIp);
	if(it == ipLoginMap.end())
	{
		LoginBlock tmp;
		tmp.lastLogin = tmp.loginsAmount = 0;
		tmp.lastProtocol = 0x00;

		ipLoginMap[clientIp] = tmp;
		it = ipLoginMap.find(clientIp);
	}

	if(it->second.loginsAmount > g_config.getNumber(ConfigManager::LOGIN_TRIES))
		it->second.loginsAmount = 0;

	int32_t currentTime = time(NULL);
	if(!success || (currentTime < it->second.lastLogin + (int32_t)g_config.getNumber(ConfigManager::RETRY_TIMEOUT) / 1000))
		it->second.loginsAmount++;
	else
		it->second.loginsAmount = 0;

	it->second.lastLogin = currentTime;
	it->second.lastProtocol = protocolId;
}

bool ConnectionManager::acceptConnection(uint32_t clientIp)
{
	if(!clientIp)
		return false;

	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionManagerLock);
	uint64_t currentTime = OTSYS_TIME();

	IpConnectMap::iterator it = ipConnectMap.find(clientIp);
	if(it == ipConnectMap.end())
	{
		ConnectBlock tmp;
		tmp.startTime = currentTime;
		tmp.blockTime = 0;
		tmp.count = 1;

		ipConnectMap[clientIp] = tmp;
		return true;
	}

	it->second.count++;
	if(it->second.blockTime > currentTime)
		return false;

	if(currentTime - it->second.startTime > 1000)
	{
		uint32_t tmp = it->second.count;
		it->second.startTime = currentTime;
		it->second.count = it->second.blockTime = 0;
		if(tmp > 10)
		{
			it->second.blockTime = currentTime + 10000;
			return false;
		}
	}

	return true;
}

void ConnectionManager::closeAll()
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Closing all connections" << std::endl;
	#endif
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionManagerLock);

	std::list<Connection*>::iterator it = m_connections.begin();
	while(it != m_connections.end())
	{
		boost::system::error_code error;
		(*it)->m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		(*it)->m_socket->close(error);
		++it;
	}

	m_connections.clear();
}

uint32_t Connection::getIP() const
{
	//Ip is expressed in network byte order
	boost::system::error_code error;
	const boost::asio::ip::tcp::endpoint endpoint = m_socket->remote_endpoint(error);
	if(!error)
		return htonl(endpoint.address().to_v4().to_ulong());

	PRINT_ASIO_ERROR("Getting remote ip");
	return 0;
}

void Connection::handle(Protocol* protocol)
{
	m_protocol = protocol;
	m_protocol->onConnect();
	accept();
}

void Connection::accept()
{
	// Read size of the first packet
	m_pendingRead++;
	try
	{
		m_timer.expires_from_now(boost::posix_time::seconds(10));
		m_timer.async_wait(boost::bind(&Connection::handleTimeout, this, boost::asio::placeholders::error));

		boost::asio::async_read(getHandle(), boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
	}
	catch(boost::system::system_error& e)
	{
		if(m_logError)
		{
			LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK");
			m_logError = false;
		}
	}
}

void Connection::close()
{
	//any thread
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::close" << std::endl;
	#endif
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionLock);
	if(m_closeState != CLOSE_STATE_NONE)
		return;

	m_closeState = CLOSE_STATE_REQUESTED;
	Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Connection::closeConnection, this)));
}

bool Connection::send(OutputMessage_ptr msg)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::send init" << std::endl;
	#endif
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	if(m_closeState == CLOSE_STATE_CLOSING || m_writeError)
	{
		OTSYS_THREAD_UNLOCK(m_connectionLock, "");
		return false;
	}

	TRACK_MESSAGE(msg);
	if(!m_pendingWrite)
	{
		if(msg->getProtocol())
			msg->getProtocol()->onSendMessage(msg);

		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Connection::send " << msg->getMessageLength() << std::endl;
		#endif
		internalSend(msg);
	}
	else
	{
		OutputMessagePool* outputPool = OutputMessagePool::getInstance();
		outputPool->autoSend(msg);
		if(m_pendingWrite > 100 && g_config.getBool(ConfigManager::FORCE_CLOSE_SLOW_CONNECTION))
		{
			std::cout << "> NOTICE: Forcing slow connection to disconnect" << std::endl;
			close();
		}
	}

	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
	return true;
}

void Connection::internalSend(OutputMessage_ptr msg)
{
	TRACK_MESSAGE(msg);
	m_pendingWrite++;
	try
	{
		boost::asio::async_write(getHandle(), boost::asio::buffer(msg->getOutputBuffer(), msg->getMessageLength()),
			boost::bind(&Connection::onWrite, this, msg, boost::asio::placeholders::error));
	}
	catch(boost::system::system_error& e)
	{
		if(m_logError)
		{
			LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK");
			m_logError = false;
		}
	}
}

void Connection::closeConnection()
{
	//dispatcher thread
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::closeConnection" << std::endl;
	#endif

	OTSYS_THREAD_LOCK(m_connectionLock, "");
	if(m_closeState != CLOSE_STATE_REQUESTED)
	{
		std::cout << "[Error - Connection::closeConnection] m_closeState = " << m_closeState << std::endl;
		OTSYS_THREAD_UNLOCK(m_connectionLock, "");
		return;
	}

	m_closeState = CLOSE_STATE_CLOSING;
	if(m_protocol)
	{
		Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Protocol::releaseProtocol, m_protocol)));
		m_protocol->setConnection(NULL);
		m_protocol = NULL;
	}

	if(!write())
		OTSYS_THREAD_UNLOCK(m_connectionLock, "");
}

void Connection::releaseConnection()
{
	if(m_refCount > 0) //Reschedule it and try again.
		Scheduler::getScheduler().addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
			boost::bind(&Connection::releaseConnection, this)));
	else
		deleteConnection();
}

void Connection::deleteConnection()
{
	//dispatcher thread
	assert(!m_refCount);
	try
	{
		m_io_service.dispatch(boost::bind(&Connection::onStop, this));
	}
	catch(boost::system::system_error& e)
	{
		if(m_logError)
		{
			LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK");
			m_logError = false;
		}
	}
}

void Connection::parseHeader(const boost::system::error_code& error)
{
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	m_timer.cancel();

	m_pendingRead--;
	if(m_closeState == CLOSE_STATE_CLOSING)
	{
		if(!write())
			OTSYS_THREAD_UNLOCK(m_connectionLock, "");

		return;
	}

	int32_t size = m_msg.decodeHeader();
	if(!error && size > 0 && size < NETWORKMESSAGE_MAXSIZE - 16)
	{
		// Read packet content
		m_pendingRead++;
		m_msg.setMessageLength(size + NetworkMessage::header_length);
		boost::asio::async_read(getHandle(), boost::asio::buffer(m_msg.getBodyBuffer(), size),
			boost::bind(&Connection::parsePacket, this, boost::asio::placeholders::error));
	}
	else
		handleReadError(error);

	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
}

void Connection::parsePacket(const boost::system::error_code& error)
{
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	m_pendingRead--;
	if(m_closeState == CLOSE_STATE_CLOSING)
	{
		if(!write())
			OTSYS_THREAD_UNLOCK(m_connectionLock, "");

		return;
	}

	if(!error)
	{
		// Checksum
		uint32_t length = m_msg.getMessageLength() - m_msg.getReadPos() - 4, receivedChecksum = m_msg.PeekU32(), checksum = 0;
		if(length)
			checksum = adlerChecksum((uint8_t*)(m_msg.getBuffer() + m_msg.getReadPos() + 4), length);

		bool checksumEnabled = false;
		if(receivedChecksum == checksum)
		{
			m_msg.SkipBytes(4);
			checksumEnabled = true;
		}

		// Protocol selection
		if(!m_receivedFirst)
		{
			// First message received
			m_receivedFirst = true;
			if(!m_protocol)
			{
				m_protocol = m_port->makeProtocol(checksumEnabled, m_msg);
				if(!m_protocol)
				{
					close();
					OTSYS_THREAD_UNLOCK(m_connectionLock, "");
					return;
				}

				m_protocol->setConnection(this);
			}
			else
				m_msg.SkipBytes(1);

			m_protocol->onRecvFirstMessage(m_msg);
		}
		else
			m_protocol->onRecvMessage(m_msg);

		// Wait to the next packet
		m_pendingRead++;
		boost::asio::async_read(getHandle(), boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, this, boost::asio::placeholders::error));
	}
	else
		handleReadError(error);

	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
}

bool Connection::write()
{
	//any thread
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::write" << std::endl;
	#endif
	if(!m_pendingWrite || m_writeError)
	{
		if(!m_socketClosed)
		{
			#ifdef __DEBUG_NET_DETAIL__
			std::cout << "Closing socket" << std::endl;
			#endif

			boost::system::error_code error;
			m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			if(error && error != boost::asio::error::not_connected)
				PRINT_ASIO_ERROR("Shutdown");

			m_socket->close(error);
			m_socketClosed = true;
			if(error)
				PRINT_ASIO_ERROR("Close");
		}

		if(!m_pendingRead)
		{
			#ifdef __DEBUG_NET_DETAIL__
			std::cout << "Deleting Connection" << std::endl;
			#endif

			OTSYS_THREAD_UNLOCK(m_connectionLock, "");
			Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Connection::releaseConnection, this)));
			return true;
		}
	}

	return false;
}

void Connection::onWrite(OutputMessage_ptr msg, const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "onWrite" << std::endl;
	#endif
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	m_pendingWrite--;

	TRACK_MESSAGE(msg);
	msg.reset();
	if(error)
		handleWriteError(error);

	if(m_closeState == CLOSE_STATE_CLOSING)
	{
		if(!write())
			OTSYS_THREAD_UNLOCK(m_connectionLock, "");

		return;
	}

	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
}

void Connection::onStop()
{
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	m_timer.cancel();

	ConnectionManager::getInstance()->releaseConnection(this);
	if(m_socket->is_open())
	{
		m_socket->cancel();
		m_socket->close();
	}

	delete m_socket;
	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
	delete this;
}

void Connection::handleTimeout(const boost::system::error_code& error)
{
	if(error)
		return;

	OTSYS_THREAD_LOCK(m_connectionLock, "");
	if(m_pendingRead > 0) //cancel all asynchronous operations associated with the socket
		getHandle().cancel();

	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
}

void Connection::handleReadError(const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	PRINT_ASIO_ERROR("Reading - detail");
	#endif
	if(error == boost::asio::error::operation_aborted)
	{
		//Operation aborted because connection will be closed
		//Do NOT call closeConnection() from here
	}
	else if(error == boost::asio::error::eof || error == boost::asio::error::connection_reset
		|| error == boost::asio::error::connection_aborted) //No more to read or connection closed remotely
		close();
	else
	{
		PRINT_ASIO_ERROR("Reading");
		close();
	}

	m_readError = true;
}

void Connection::handleWriteError(const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	PRINT_ASIO_ERROR("Writing - detail");
	#endif
	if(error == boost::asio::error::operation_aborted)
		{} //Operation aborted because connection will be closed, do NOT call closeConnection() from here
	else if(error == boost::asio::error::eof || error == boost::asio::error::connection_reset
		|| error == boost::asio::error::connection_aborted) //No more to read or connection closed remotely
		close();
	else
	{
		PRINT_ASIO_ERROR("Writing");
		close();
	}

	m_writeError = true;
}
