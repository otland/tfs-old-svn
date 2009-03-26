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

#include "outputmessage.h"
#include "connection.h"
#include "protocol.h"

OutputMessage::OutputMessage()
{
	freeMessage();
}

//*********** OutputMessagePool ****************//

OutputMessagePool::OutputMessagePool()
{
	OTSYS_THREAD_LOCKVARINIT(m_outputPoolLock);
	for(uint32_t i = 0; i < OUTPUT_POOL_SIZE; ++i)
	{
		OutputMessage* msg = new OutputMessage();
		m_outputMessages.push_back(msg);
#ifdef __TRACK_NETWORK__
		m_allOutputMessages.push_back(msg);
#endif
	}

	m_frameTime = OTSYS_TIME();
}

void OutputMessagePool::startExecutionFrame()
{
	m_frameTime = OTSYS_TIME();
	m_shutdown = false;
}

OutputMessagePool::~OutputMessagePool()
{
	for(InternalOutputMessageList::iterator it = m_outputMessages.begin(); it != m_outputMessages.end(); ++it)
		delete (*it);

	m_outputMessages.clear();
	OTSYS_THREAD_LOCKVARRELEASE(m_outputPoolLock);
}

void OutputMessagePool::send(OutputMessage_ptr msg)
{
	OTSYS_THREAD_LOCK(m_outputPoolLock, "");
	OutputMessage::OutputMessageState state = msg->getState();
	OTSYS_THREAD_UNLOCK(m_outputPoolLock, "");

	if(state == OutputMessage::STATE_ALLOCATED_NO_AUTOSEND)
	{
		#ifdef __DEBUG_NET_DETAIL__
		std::cout << "Sending message - SINGLE" << std::endl;
		#endif
		if(msg->getConnection())
		{
			if(!msg->getConnection()->send(msg) && msg->getProtocol())
				msg->getProtocol()->onSendMessage(msg);
		}
		#ifdef __DEBUG_NET__
		else
			std::cout << "Error: [OutputMessagePool::send] NULL connection." << std::endl;
		#endif
	}
	#ifdef __DEBUG_NET__
	else
		std::cout << "Warning: [OutputMessagePool::send] State != STATE_ALLOCATED_NO_AUTOSEND" << std::endl;
	#endif
}

void OutputMessagePool::sendAll()
{
	OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
	for(OutputMessageList::iterator it = m_autoSendOutputMessages.begin(); it != m_autoSendOutputMessages.end(); )
	{
		OutputMessage_ptr omsg = (*it);
		#ifdef __NO_PLAYER_SENDBUFFER__
		//use this define only for debugging
		if(true)
		#else
		//It will send only messages bigger then 1 kb or with a lifetime greater than 10 ms
		if(omsg->getMessageLength() > 1024 || (m_frameTime - omsg->getFrame() > 10))
		#endif
		{
			#ifdef __DEBUG_NET_DETAIL__
			std::cout << "Sending message - ALL" << std::endl;
			#endif
			if(omsg->getConnection())
			{
				if(!omsg->getConnection()->send(omsg) && omsg->getProtocol())
					omsg->getProtocol()->onSendMessage(omsg);
			}
			#ifdef __DEBUG_NET__
			else
				std::cout << "Error: [OutputMessagePool::send] NULL connection." << std::endl;
			#endif

			it = m_autoSendOutputMessages.erase(it);
		}
		else
			++it;
	}
}

void OutputMessagePool::internalReleaseMessage(OutputMessage* msg)
{
	if(msg->getProtocol())
		msg->getProtocol()->unRef();
	else
		std::cout << "[Warning - OutputMessagePool::internalReleaseMessage] protocol not found." << std::endl;

	if(msg->getConnection())
		msg->getConnection()->unRef();
	else
		std::cout << "[Warning - OutputMessagePool::internalReleaseMessage] connection not found." << std::endl;

	msg->freeMessage();
	OTSYS_THREAD_LOCK(m_outputPoolLock, "");	
	m_outputMessages.push_back(msg);
	OTSYS_THREAD_UNLOCK(m_outputPoolLock, "");
}

OutputMessage_ptr OutputMessagePool::getOutputMessage(Protocol* protocol, bool autosend /*= true*/)
{
	#ifdef __DEBUG_NET_DETAIL__
	std::cout << "request output message - auto = " << autosend << std::endl;
	#endif
	if(m_shutdown)
		return OutputMessage_ptr();

	OTSYS_THREAD_LOCK_CLASS lockClass(m_outputPoolLock);
	if(!protocol->getConnection())
		return OutputMessage_ptr();

	OutputMessage_ptr omsg;
	if(m_outputMessages.empty())
	{
#ifdef __TRACK_NETWORK__
		if(m_allOutputMessages.size() >= 5000)
		{
			std::cout << "High usage of outputmessages: " << std::endl;
			m_allOutputMessages.back()->PrintTrace();
		}

#endif
		omsg.reset(new OutputMessage, boost::bind(&OutputMessagePool::internalReleaseMessage, this, _1));
#ifdef __TRACK_NETWORK__
		m_allOutputMessages.push_back(omsg.get());
#endif
	}
	else
	{
		omsg.reset(m_outputMessages.back(), boost::bind(&OutputMessagePool::internalReleaseMessage, this, _1));
#ifdef __TRACK_NETWORK__
		// Print message trace
		if(omsg->getState() != OutputMessage::STATE_FREE)
		{
			std::cout << "Using allocated message, message trace:" << std::endl;
			omsg->PrintTrace();
		}

#else
		assert(omsg->getState() == OutputMessage::STATE_FREE);
#endif
		m_outputMessages.pop_back();
	}

	configureOutputMessage(omsg, protocol, autosend);
	return omsg;
}

void OutputMessagePool::configureOutputMessage(OutputMessage_ptr msg, Protocol* protocol, bool autosend)
{
	msg->Reset();
	if(autosend)
	{
		msg->setState(OutputMessage::STATE_ALLOCATED);
		m_autoSendOutputMessages.push_back(msg);
	}
	else
		msg->setState(OutputMessage::STATE_ALLOCATED_NO_AUTOSEND);

	Connection* connection = protocol->getConnection();
	assert(connection != NULL);

	msg->setProtocol(protocol);
	protocol->addRef();
	msg->setConnection(connection);
	connection->addRef();

	msg->setFrame(m_frameTime);
}
