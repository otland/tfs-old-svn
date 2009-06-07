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
#include "scheduler.h"

#include "outputmessage.h"
#include "protocol.h"

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
	OutputMessageList::iterator it;
	for(it = m_toAddQueue.begin(); it != m_toAddQueue.end();)
	{
		//drop messages that are older than 10 seconds
		if(OTSYS_TIME() - (*it)->getFrame() > 10000)
		{
			if((*it)->getProtocol())
				(*it)->getProtocol()->onSendMessage(*it);

			it = m_toAddQueue.erase(it);
			continue;
		}

		(*it)->setState(OutputMessage::STATE_ALLOCATED);
		m_autoSendOutputMessages.push_back(*it);
		++it;
	}

	m_toAddQueue.clear();
	for(it = m_autoSendOutputMessages.begin(); it != m_autoSendOutputMessages.end(); )
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

void OutputMessagePool::releaseMessage(OutputMessage* msg)
{
	Dispatcher::getDispatcher().addTask(createTask(boost::bind(
		&OutputMessagePool::internalReleaseMessage, this, msg)), true);
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
#ifdef __TRACK_NETWORK__
	msg->clearTrack();
#endif

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

	if(m_outputMessages.empty())
	{
		OutputMessage* msg = new OutputMessage();
		m_outputMessages.push_back(msg);
#ifdef __TRACK_NETWORK__
		m_allOutputMessages.push_back(msg);
#endif
	}

	OutputMessage_ptr omsg;
	omsg.reset(m_outputMessages.back(),
		boost::bind(&OutputMessagePool::releaseMessage, this, _1));

	m_outputMessages.pop_back();
	configureOutputMessage(omsg, protocol, autosend);
	return omsg;
}

void OutputMessagePool::configureOutputMessage(OutputMessage_ptr msg, Protocol* protocol, bool autosend)
{
	TRACK_MESSAGE(msg);
	msg->Reset();
	if(autosend)
	{
		msg->setState(OutputMessage::STATE_ALLOCATED);
		m_autoSendOutputMessages.push_back(msg);
	}
	else
		msg->setState(OutputMessage::STATE_ALLOCATED_NO_AUTOSEND);

	Connection* connection = protocol->getConnection();
	assert(connection);

	msg->setProtocol(protocol);
	protocol->addRef();
	msg->setConnection(connection);
	connection->addRef();

	msg->setFrame(m_frameTime);
}
