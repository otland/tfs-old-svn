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

#ifndef __OTSERV_SERVER_H__
#define __OTSERV_SERVER_H__
#include "otsystem.h"
#include <boost/enable_shared_from_this.hpp>

class Connection;
class Protocol;
class NetworkMessage;

class ServiceBase;
typedef boost::shared_ptr<ServiceBase> Service_ptr;
class ServicePort;
typedef boost::shared_ptr<ServicePort> ServicePort_ptr;

class ServiceBase : boost::noncopyable
{
	public:
		virtual Protocol* makeProtocol(Connection* connection) const = 0;

		virtual uint8_t getProtocolId() const = 0;
		virtual bool isSingleSocket() const = 0;
		virtual bool hasChecksum() const = 0;
		virtual const char* getProtocolName() const = 0;
};

template <typename ProtocolType>
class Service : public ServiceBase
{
	public:
		Protocol* makeProtocol(Connection* connection) const {return new ProtocolType(connection);}

		uint8_t getProtocolId() const {return ProtocolType::protocolId;}
		bool isSingleSocket() const {return ProtocolType::isSingleSocket;}
		bool hasChecksum() const {return ProtocolType::hasChecksum;}
		const char* getProtocolName() const {return ProtocolType::protocolName();}
};

class ServicePort : boost::noncopyable, public boost::enable_shared_from_this<ServicePort>
{
	public:
		ServicePort(boost::asio::io_service& io_service): m_io_service(io_service),
			m_acceptor(NULL), m_serverPort(0), m_pendingStart(false) {}
		virtual ~ServicePort() {close();}

		bool add(Service_ptr);

		void open(uint16_t port);
		void close();

		void handle(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error);

		bool isSingleSocket() const {return m_services.size() && m_services.front()->isSingleSocket();}
		std::string getProtocolNames() const;

		Protocol* makeProtocol(bool checksum, NetworkMessage& msg) const;

	protected:
		void accept();

		typedef std::vector<Service_ptr> ServiceVec;
		ServiceVec m_services;

		boost::asio::io_service& m_io_service;
		boost::asio::ip::tcp::acceptor* m_acceptor;

		uint16_t m_serverPort;
		bool m_pendingStart;
};

typedef boost::shared_ptr<ServicePort> ServicePort_ptr;

class ServiceManager : boost::noncopyable
{
	ServiceManager(const ServiceManager&);

	public:
		ServiceManager(): m_io_service(), deathTimer(m_io_service), running(false) {}
		virtual ~ServiceManager() {stop();}

		template <typename ProtocolType>
		bool add(uint16_t port);

		void run();
		void stop();

		bool isRunning() const {return !m_acceptors.empty();}
		std::list<uint16_t> getPorts() const;

	protected:
		void die() {m_io_service.stop();}

		boost::asio::io_service m_io_service;
		boost::asio::deadline_timer deathTimer;
		bool running;

		typedef std::map<uint16_t, ServicePort_ptr> AcceptorsMap;
		AcceptorsMap m_acceptors;
};

template <typename ProtocolType>
bool ServiceManager::add(uint16_t port)
{
	if(!port)
	{
		std::cout << "> ERROR: No port provided for service " << ProtocolType::protocolName() << ". Service disabled." << std::endl;
		return false;
	}

	ServicePort_ptr servicePort;
	AcceptorsMap::iterator it = m_acceptors.find(port);
	if(it == m_acceptors.end())
	{
		servicePort.reset(new ServicePort(m_io_service));
		servicePort->open(port);
		m_acceptors[port] = servicePort;
	}
	else
	{
		servicePort = it->second;
		if(servicePort->isSingleSocket() || ProtocolType::isSingleSocket)
		{
			std::cout << "> ERROR: " << ProtocolType::protocolName() << " and " << servicePort->getProtocolNames();
			std::cout << " cannot use the same port (" << port << ")." << std::endl;
			return false;
		}
	}

	return servicePort->add(Service_ptr(new Service<ProtocolType>()));
}
#endif
