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
#include "server.h"

#include "connection.h"
#include "outputmessage.h"
#include "textlogger.h"

#include "game.h"
#include "configmanager.h"

extern Game g_game;
extern ConfigManager g_config;

bool ServicePort::m_logError = true;

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

void ServicePort::onOpen(boost::weak_ptr<ServicePort> weakService, uint16_t port)
{
	if(weakService.expired())
		return;

	if(ServicePort_ptr service = weakService.lock())
	{
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "ServicePort::onOpen" << std::endl;
		#endif
		service->open(port);
	}
}

void ServicePort::open(uint16_t port)
{
	m_serverPort = port;
	m_pendingStart = false;
	try
	{
		if(g_config.getBool(ConfigManager::BIND_IP_ONLY))
			m_acceptor = new boost::asio::ip::tcp::acceptor(m_io_service, boost::asio::ip::tcp::endpoint(
				boost::asio::ip::address(boost::asio::ip::address_v4::from_string(
				g_config.getString(ConfigManager::IP))), m_serverPort));
		else
			m_acceptor = new boost::asio::ip::tcp::acceptor(m_io_service, boost::asio::ip::tcp::endpoint(
				boost::asio::ip::address(boost::asio::ip::address_v4(INADDR_ANY)), m_serverPort));

		accept();
	}
	catch(boost::system::system_error& e)
	{
		if(m_logError)
		{
			LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK")
			m_logError = false;
		}

		m_pendingStart = true;
		Scheduler::getInstance().addEvent(createSchedulerTask(5000, boost::bind(
			&ServicePort::onOpen, boost::weak_ptr<ServicePort>(shared_from_this()), m_serverPort)));
	}
}

void ServicePort::close()
{
	if(!m_acceptor)
		return;

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

void ServicePort::accept()
{
	if(!m_acceptor)
		return;

	try
	{
		boost::asio::ip::tcp::socket* socket = new boost::asio::ip::tcp::socket(m_io_service);
		m_acceptor->async_accept(*socket, boost::bind(
			&ServicePort::handle, this, socket, boost::asio::placeholders::error));
	}
	catch(boost::system::system_error& e)
	{
		if(m_logError)
		{
			LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK")
			m_logError = false;
		}
	}
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
		const boost::asio::ip::tcp::endpoint ip = socket->remote_endpoint(error);

		uint32_t remoteIp = 0;
		if(!error)
			remoteIp = htonl(ip.address().to_v4().to_ulong());

		Connection_ptr connection;
		if(remoteIp && ConnectionManager::getInstance()->acceptConnection(remoteIp) &&
			(connection = ConnectionManager::getInstance()->createConnection(
			socket, m_io_service, shared_from_this())))
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
		close();
		if(!m_pendingStart)
		{
			m_pendingStart = true;
			Scheduler::getInstance().addEvent(createSchedulerTask(5000, boost::bind(
				&ServicePort::onOpen, boost::weak_ptr<ServicePort>(shared_from_this()), m_serverPort)));
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
	for(int32_t i = 1, j = m_services.size(); i < j; ++i)
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
			return (*it)->makeProtocol(Connection_ptr());
	}

	return NULL;
}

void ServiceManager::run()
{
	assert(!running);
	try
	{
		m_io_service.run();
		if(!running)
			running = true;
	}
	catch(boost::system::system_error& e)
	{
		LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK")
	}
}

void ServiceManager::stop()
{
	if(!running)
		return;

	running = false;
	for(AcceptorsMap::iterator it = m_acceptors.begin(); it != m_acceptors.end(); ++it)
	{
		try
		{
			m_io_service.post(boost::bind(&ServicePort::close, it->second));
		}
		catch(boost::system::system_error& e)
		{
			LOG_MESSAGE(LOGTYPE_ERROR, e.what(), "NETWORK")
		}
	}

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
