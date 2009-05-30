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

Connection_ptr ConnectionManager::createConnection(boost::asio::ip::tcp::socket* socket,
	boost::asio::io_service& io_service, ServicePort_ptr servicer)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Creating new connection" << std::endl;
	#endif
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionManagerLock);
	Connection_ptr connection = boost::shared_ptr<Connection>(new Connection(socket, io_service, servicer));

	m_connections.push_back(connection);
	return connection;
}

void ConnectionManager::releaseConnection(Connection_ptr connection)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Releasing connection" << std::endl;
	#endif
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionManagerLock);

	std::list<Connection_ptr>::iterator it = std::find(m_connections.begin(), m_connections.end(), connection);
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

	std::list<Connection_ptr>::iterator it = m_connections.begin();
	while(it != m_connections.end())
	{
		boost::system::error_code error;
		(*it)->m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		(*it)->m_socket->close(error);
		++it;
	}

	m_connections.clear();
}

void Connection::close()
{
	//any thread
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::close" << std::endl;
	#endif
	OTSYS_THREAD_LOCK_CLASS lockClass(m_connectionLock);
	if(m_connectionState == CONNECTION_STATE_CLOSED || m_connectionState == CONNECTION_STATE_REQUEST_CLOSE)
		return;

	m_connectionState = CONNECTION_STATE_REQUEST_CLOSE;
	Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Connection::closeConnection, this)));
}

void Connection::closeConnection()
{
	//dispatcher thread
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::closeConnection" << std::endl;
	#endif
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	if(m_connectionState != CONNECTION_STATE_REQUEST_CLOSE)
	{
		std::cout << "[Error - Connection::closeConnection] m_connectionState = " << m_connectionState << std::endl;
		OTSYS_THREAD_UNLOCK(m_connectionLock, "");
		return;
	}

	if(m_protocol)
	{
		m_protocol->setConnection(Connection_ptr());
		m_protocol->releaseProtocol();
		m_protocol = NULL;
	}

	m_connectionState = CONNECTION_STATE_CLOSING;
	if(!m_pendingWrite || m_writeError)
	{
		internalClose();
		releaseConnection();
		m_connectionState = CONNECTION_STATE_CLOSED;
	}

	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
}

void Connection::internalClose()
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::internalClose" << std::endl;
	#endif
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	if(m_socket->is_open())
	{
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Closing socket" << std::endl;
		#endif
		m_pendingRead = m_pendingWrite = 0;
		m_socket->cancel();

		boost::system::error_code error;
		m_socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		if(error && error != boost::asio::error::not_connected)
			PRINT_ASIO_ERROR("Shutdown");

		m_socket->close(error);
		if(error)
			PRINT_ASIO_ERROR("Close");
	}

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

void Connection::onStop()
{
	OTSYS_THREAD_LOCK(m_connectionLock, "");

	m_readTimer.cancel();
	m_writeTimer.cancel();
	if(m_socket->is_open())
	{
		m_socket->cancel();
		m_socket->close();
	}

	delete m_socket;
	m_socket = NULL;

	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
	ConnectionManager::getInstance()->releaseConnection(shared_from_this());
}

void Connection::deleteConnection()
{
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

void Connection::handle(Protocol* protocol)
{
	m_protocol = protocol;
	m_protocol->onConnect();
	accept();
}

void Connection::accept()
{
	try
	{
		++m_pendingRead;
		m_readTimer.expires_from_now(boost::posix_time::seconds(30));
		m_readTimer.async_wait(boost::bind(&Connection::handleReadTimeout, boost::weak_ptr<Connection>(
			shared_from_this()), boost::asio::placeholders::error));

		boost::asio::async_read(getHandle(), boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, shared_from_this(), boost::asio::placeholders::error));
	}
	catch(boost::system::system_error& e)
	{
		if(m_logError)
		{
			LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK");
			m_logError = false;
			close();
		}
	}
}

void Connection::parseHeader(const boost::system::error_code& error)
{
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	m_readTimer.cancel();

	int32_t size = m_msg.decodeHeader();
	if(error || size <= 0 || size >= NETWORKMESSAGE_MAXSIZE - 16)
		handleReadError(error);

	if(m_connectionState != CONNECTION_STATE_OPEN || m_readError)
	{
		close();
		OTSYS_THREAD_UNLOCK(m_connectionLock, "");
		return;
	}

	--m_pendingRead;
	try
	{
		++m_pendingRead;
		m_readTimer.expires_from_now(boost::posix_time::seconds(30));
		m_readTimer.async_wait(boost::bind(&Connection::handleReadTimeout, boost::weak_ptr<Connection>(
			shared_from_this()), boost::asio::placeholders::error));

		m_msg.setMessageLength(size + NetworkMessage::header_length);
		boost::asio::async_read(getHandle(), boost::asio::buffer(m_msg.getBodyBuffer(), size),
			boost::bind(&Connection::parsePacket, shared_from_this(), boost::asio::placeholders::error));
	}
	catch(boost::system::system_error& e)
	{
		if(m_logError)
		{
			LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK");
			m_logError = false;
			close();
		}
	}

	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
}

void Connection::parsePacket(const boost::system::error_code& error)
{
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	m_readTimer.cancel();
	if(error)
		handleReadError(error);

	if(m_connectionState != CONNECTION_STATE_OPEN || m_readError)
	{
		close();
		OTSYS_THREAD_UNLOCK(m_connectionLock, "");
		return;
	}

	--m_pendingRead;
	//Check packet checksum
	uint32_t recvChecksum = m_msg.PeekU32(), checksum = 0;

	int32_t len = m_msg.getMessageLength() - m_msg.getReadPos() - 4;
	if(len > 0)
		checksum = adlerChecksum((uint8_t*)(m_msg.getBuffer() + m_msg.getReadPos() + 4), len);

	if(recvChecksum == checksum) //remove the checksum
		m_msg.SkipBytes(4);

	if(!m_receivedFirst)
	{
		m_receivedFirst = true; //first message received
		if(!m_protocol) //game protocol has already been created at this point
		{
			m_protocol = m_service_port->makeProtocol(recvChecksum == checksum, m_msg);
			if(!m_protocol)
			{
				close();
				OTSYS_THREAD_UNLOCK(m_connectionLock, "");
				return;
			}

			m_protocol->setConnection(shared_from_this());
		}
		else //skip protocol ID
			m_msg.SkipBytes(1);

		m_protocol->onRecvFirstMessage(m_msg);
	}
	else //send the packet to the current protocol
		m_protocol->onRecvMessage(m_msg);

	try
	{
		++m_pendingRead;
		m_readTimer.expires_from_now(boost::posix_time::seconds(30));
		m_readTimer.async_wait(boost::bind(&Connection::handleReadTimeout, boost::weak_ptr<Connection>(shared_from_this()),
			boost::asio::placeholders::error));

		//wait to the next packet
		boost::asio::async_read(getHandle(), boost::asio::buffer(m_msg.getBuffer(), NetworkMessage::header_length),
			boost::bind(&Connection::parseHeader, shared_from_this(), boost::asio::placeholders::error));
	}
	catch(boost::system::system_error& e)
	{
		if(m_logError)
		{
			LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK");
			m_logError = false;
			close();
		}
	}

	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
}

bool Connection::send(OutputMessage_ptr msg)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "Connection::send init" << std::endl;
	#endif
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	if(m_connectionState != CONNECTION_STATE_OPEN || m_writeError)
	{
		OTSYS_THREAD_UNLOCK(m_connectionLock, "");
		return false;
	}

	if(!m_pendingWrite)
	{
		if(msg->getProtocol())
			msg->getProtocol()->onSendMessage(msg);

		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Connection::send " << msg->getMessageLength() << std::endl;
		#endif
		TRACK_MESSAGE(msg);
		internalSend(msg);
	}
	else{
		#ifdef __DEBUG_NET__
		std::cout << "Connection::send Adding to queue " << msg->getMessageLength() << std::endl;
		#endif
		TRACK_MESSAGE(msg);
		if(OutputMessagePool* outputPool = OutputMessagePool::getInstance())
			outputPool->autoSend(msg);
	}
	
	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
	return true;
}

void Connection::internalSend(OutputMessage_ptr msg)
{
	TRACK_MESSAGE(msg);
	try
	{
		++m_pendingWrite;
		m_writeTimer.expires_from_now(boost::posix_time::seconds(30));
		m_writeTimer.async_wait(boost::bind(&Connection::handleWriteTimeout, boost::weak_ptr<Connection>(
			shared_from_this()), boost::asio::placeholders::error));

		boost::asio::async_write(getHandle(), boost::asio::buffer(msg->getOutputBuffer(), msg->getMessageLength()),
			boost::bind(&Connection::onWrite, shared_from_this(), msg, boost::asio::placeholders::error));
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

void Connection::onWrite(OutputMessage_ptr msg, const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "onWrite" << std::endl;
	#endif
	OTSYS_THREAD_LOCK(m_connectionLock, "");
	m_writeTimer.cancel();

	TRACK_MESSAGE(msg);
	msg.reset();
	if(error)
		handleWriteError(error);

	if(m_connectionState != CONNECTION_STATE_OPEN || m_writeError)
	{
		internalClose();
		close();

		OTSYS_THREAD_UNLOCK(m_connectionLock, "");
		return;
	}

	--m_pendingWrite;
	OTSYS_THREAD_UNLOCK(m_connectionLock, "");
}

void Connection::handleReadError(const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	PRINT_ASIO_ERROR("Reading - detail");
	#endif
	if(error == boost::asio::error::operation_aborted) //operation aborted because connection will be closed, do NOT call close() from here
		{}
	else if(error == boost::asio::error::eof) //no more to read
		close();
	else if(error == boost::asio::error::connection_reset || error == boost::asio::error::connection_aborted) //connection closed remotely
		close();
	else
	{
		PRINT_ASIO_ERROR("Reading");
		close();
	}

	m_readError = true;
}

void Connection::handleReadTimeout(boost::weak_ptr<Connection> weakConnection, const boost::system::error_code& error)
{
	if(error || weakConnection.expired())
		return;

	if(boost::shared_ptr<Connection> connection = weakConnection.lock())
	{
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Connection::handleReadTimeout" << std::endl;
		#endif
		connection->internalClose();
		connection->close();
	}
}

void Connection::handleWriteError(const boost::system::error_code& error)
{
	#ifdef __DEBUG_NET_DETAIL__
	PRINT_ASIO_ERROR("Writing - detail");
	#endif
	if(error == boost::asio::error::operation_aborted) //operation aborted because connection will be closed, do NOT call close() from here
		{}
	else if(error == boost::asio::error::eof) //no more to write
		close();
	else if(error == boost::asio::error::connection_reset || error == boost::asio::error::connection_aborted) //connection closed remotely
		close();
	else
	{
		PRINT_ASIO_ERROR("Writing");
		close();
	}

	m_writeError = true;
}

void Connection::handleWriteTimeout(boost::weak_ptr<Connection> weakConnection, const boost::system::error_code& error)
{
	if(error || weakConnection.expired())
		return;

	if(boost::shared_ptr<Connection> connection = weakConnection.lock())
	{
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Connection::handleWriteTimeout" << std::endl;
		#endif
		connection->internalClose();
		connection->close();
	}
}
