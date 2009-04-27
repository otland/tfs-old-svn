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
#if defined __WINDOWS__ || defined WIN32
#include <winerror.h>
#endif

#include "server.h"
#include "connection.h"
#include "outputmessage.h"

#include "game.h"
#include "configmanager.h"

extern Game g_game;
extern ConfigManager g_config;

bool ServicePort::add(Service_ptr newService)
{
	for(ServiceVec::const_iterator it = m_services.begin(); it != m_services.end(); ++it)
	{
		if((*it)->isSingleSocket())
			return false;
	}

	m_services.push_back(newService);
	return true;
}

void ServicePort::open(uint16_t port)
{
	m_serverPort = port;
	if(m_pendingStart)
		m_pendingStart = false;

	m_acceptor = new boost::asio::ip::tcp::acceptor(m_io_service, boost::asio::ip::tcp::endpoint(
		boost::asio::ip::address(boost::asio::ip::address_v4(INADDR_ANY)), m_serverPort));
	accept();
}

void ServicePort::close()
{
	if(m_acceptor)
	{
		if(m_acceptor->is_open())
		{
			boost::system::error_code error;
			m_acceptor->close(error);
			if(error)
				PRINT_ASIO_ERROR("Closing listen socket");

		}

		delete m_acceptor;
		m_acceptor = NULL;
	}
}

void ServicePort::accept()
{
	if(!m_acceptor)
	{
#ifdef __DEBUG_NET__
		std::cout << "[Error - ServerPort::accept] NULL m_acceptor." << std::endl;
#endif
		return;
	}

	boost::asio::ip::tcp::socket* socket = new boost::asio::ip::tcp::socket(m_io_service);
	m_acceptor->async_accept(*socket, boost::bind(
		&ServicePort::handle, this, socket, boost::asio::placeholders::error));
}

void ServicePort::handle(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error)
{
	if(!error)
	{
		if(m_services.empty())
		{
#ifdef __DEBUG_NET__
			std::cout << "[Error - ServerPort::handle] No services running!" << std::endl;
#endif
			return;
		}

		boost::system::error_code error;
		const boost::asio::ip::tcp::endpoint endpoint = socket->remote_endpoint(error);

		uint32_t remoteIp = 0;
		if(!error)
			remoteIp = htonl(endpoint.address().to_v4().to_ulong());

		Connection* connection = NULL;
		if(remoteIp != 0 && ConnectionManager::getInstance()->acceptConnection(remoteIp) &&
			(connection = ConnectionManager::getInstance()->createConnection(socket, shared_from_this())))
		{
			if(m_services.front()->isSingleSocket())
				connection->handle(m_services.front()->makeProtocol(connection));
			else
				connection->accept();
		}
		else if(socket->is_open())
		{
			boost::system::error_code error;
			socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			socket->close(error);
			delete socket;
		}

#ifdef __DEBUG_NET_DETAIL__
		std::cout << "handle - OK" << std::endl;
#endif
		accept();
	}
	else if(error != boost::asio::error::operation_aborted)
	{
		PRINT_ASIO_ERROR("Handling");
		if(m_listenErrors > 99)
		{
#ifndef __ENABLE_LISTEN_ERROR__
			m_listenErrors = 0;
			std::cout << "[Warning - Server::handle] More than 100 listen errors." << std::endl;
#else
			close();
			std::cout << "[Error - Server::handle] More than 100 listen errors." << std::endl;
			return;
#endif
		}

		m_listenErrors++;
		if(!m_pendingStart)
		{
			m_pendingStart = true;
			Scheduler::getScheduler().addEvent(createSchedulerTask(5000,
				boost::bind(&ServicePort::open, this, m_serverPort)));
		}
	}
#ifdef __DEBUG_NET__
	else
		std::cout << "[Error - ServerPort::handle] Operation aborted." << std::endl;
#endif
}

std::string ServicePort::getProtocolNames() const
{
	if(m_services.empty())
		return "";

	std::string str = m_services.front()->getProtocolName();
	for(uint32_t i = 1; i < m_services.size(); ++i)
	{
		str += ", ";
		str += m_services[i]->getProtocolName();
	}

	return str;
}

Protocol* ServicePort::makeProtocol(bool checksum, NetworkMessage& msg) const
{
	uint8_t protocolId = msg.GetByte();
	for(ServiceVec::const_iterator it = m_services.begin(); it != m_services.end(); ++it)
	{
		if((*it)->getProtocolId() == protocolId && ((checksum && (*it)->hasChecksum()) || !(*it)->hasChecksum()))
			return (*it)->makeProtocol(NULL);
	}

	return NULL;
}

void ServiceManager::run()
{
	assert(!running);
	running = true;
	m_io_service.run();
}

void ServiceManager::stop()
{
	if(!running)
		return;

	running = false;
	for(AcceptorsMap::iterator it = m_acceptors.begin(); it != m_acceptors.end(); ++it)
		m_io_service.post(boost::bind(&ServicePort::close, it->second));

	m_acceptors.clear();
	OutputMessagePool::getInstance()->stop();

	deathTimer.expires_from_now(boost::posix_time::seconds(3));
	deathTimer.async_wait(boost::bind(&ServiceManager::die, this));
}

std::list<uint16_t> ServiceManager::getPorts() const
{
	std::list<uint16_t> ports;
	for(AcceptorsMap::const_iterator it = m_acceptors.begin(); it != m_acceptors.end(); ++it)
		ports.push_back(it->first);

	ports.unique();
	return ports;
}
