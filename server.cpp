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
#include "otpch.h"
#if defined __WINDOWS__ || defined WIN32
#include <winerror.h>
#endif

#include "server.h"
#include "game.h"
#include "configmanager.h"

#include "connection.h"
#include "outputmessage.h"

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

	try
	{
		m_acceptor = new boost::asio::ip::tcp::acceptor(m_io_service, boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address(boost::asio::ip::address_v4(INADDR_ANY)), m_serverPort), false);
	}
	catch(boost::system::system_error& error)
	{
		std::cout << "> ERROR: Can bind only one socket to a specific port (" << m_serverPort << ")." << std::endl;
		std::cout << "The exact error was: " << error.what() << std::endl;
		if(g_config.getBool(ConfigManager::ABORT_SOCKET_FAIL))
			exit(1);
	}

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

	Connection* connection = ConnectionManager::getInstance()->createConnection(m_io_service, shared_from_this());
	m_acceptor->async_accept(connection->getHandle(), boost::bind(
		&ServicePort::handle, this, connection, boost::asio::placeholders::error));
}

void ServicePort::handle(Connection* connection, const boost::system::error_code& error)
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

		if(m_services.front()->isSingleSocket())
			connection->handle(m_services.front()->makeProtocol(connection));
		else
			connection->accept();

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

void ServiceManager::stop()
{
	for(AcceptorsMap::iterator it = m_acceptors.begin(); it != m_acceptors.end(); ++it)
		m_io_service.post(boost::bind(&ServicePort::close, it->second.get()));

	OutputMessagePool::getInstance()->stop();
}

std::list<uint16_t> ServiceManager::getPorts() const
{
	std::list<uint16_t> ports;
	for(AcceptorsMap::const_iterator it = m_acceptors.begin(); it != m_acceptors.end(); ++it)
		ports.push_back(it->first);

	ports.unique();
	return ports;
}
