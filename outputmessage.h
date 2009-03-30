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

#ifndef __OTSERV_OUTPUT_MESSAGE_H__
#define __OTSERV_OUTPUT_MESSAGE_H__
#include "otsystem.h"

#include "networkmessage.h"
#include "tools.h"

#ifdef __TRACK_NETWORK__
#include <iostream>
#include <sstream>
#endif

class Protocol;
class Connection;

#define OUTPUT_POOL_SIZE 100

class OutputMessage : public NetworkMessage, boost::noncopyable
{
	private:
		OutputMessage();

	public:
		virtual ~OutputMessage() {}

		char* getOutputBuffer() {return (char*)&m_MsgBuf[m_outputBufferStart];}
		Protocol* getProtocol() const {return m_protocol;}
		Connection* getConnection() const {return m_connection;}

		void writeMessageLength() {addHeader((uint16_t)(m_MsgSize));}
		void addCryptoHeader(bool addChecksum)
		{
			if(addChecksum)
				addHeader((uint32_t)(adlerChecksum((uint8_t*)(m_MsgBuf + m_outputBufferStart), m_MsgSize)));

			addHeader((uint16_t)(m_MsgSize));
		}

#ifdef __TRACK_NETWORK__
		virtual void Track(std::string file, int64_t line, std::string func)
		{
			if(lastUses.size() >= 25)
				lastUses.pop_front();

			std::ostringstream os;
			os << /*file << ":" */"line " << line << " " << func;
			lastUses.push_back(os.str());
		}

		void PrintTrace()
		{
			uint32_t n = 1;
			for(std::list<std::string>::const_reverse_iterator it = lastUses.rbegin(); it != lastUses.rend(); ++it, ++n)
				std::cout << "\t" << n << ".\t" << (*it) << std::endl;
		}
#endif

		enum OutputMessageState
		{
			STATE_FREE,
			STATE_ALLOCATED,
			STATE_ALLOCATED_NO_AUTOSEND,
			STATE_WAITING
		};

	protected:
		template <typename T>
		inline void addHeader(T value)
		{
			if((int32_t)m_outputBufferStart - (int32_t)sizeof(T) < 0)
			{
				std::cout << "[Error - OutputMessage::addHeader] m_outputBufferStart(" << m_outputBufferStart << ") < " << sizeof(T) << std::endl;
				return;
			}

			m_outputBufferStart -= sizeof(T);
			*(T*)(m_MsgBuf + m_outputBufferStart) = value;
			m_MsgSize += sizeof(T);
		}

		void freeMessage()
		{
			setConnection(NULL);
			setProtocol(NULL);
			m_frame = 0;
			//allocate enough size for headers:
			// 2 bytes for unencrypted message
			// 4 bytes for checksum
			// 2 bytes for encrypted message
			m_outputBufferStart = 8;
			setState(OutputMessage::STATE_FREE);
		}

		friend class OutputMessagePool;

		void setProtocol(Protocol* protocol) {m_protocol = protocol;}
		void setConnection(Connection* connection) {m_connection = connection;}

		void setState(OutputMessageState state) {m_state = state;}
		OutputMessageState getState() const {return m_state;}

		void setFrame(uint64_t frame) {m_frame = frame;}
		uint64_t getFrame() const {return m_frame;}

		Protocol* m_protocol;
		Connection* m_connection;

		OutputMessageState m_state;
		uint64_t m_frame;
		uint32_t m_outputBufferStart;
#ifdef __TRACK_NETWORK__
		std::list<std::string> lastUses;
#endif
};

typedef boost::shared_ptr<OutputMessage> OutputMessage_ptr;

class OutputMessagePool
{
	private:
		OutputMessagePool();

	public:
		virtual ~OutputMessagePool();

		static OutputMessagePool* getInstance()
		{
			static OutputMessagePool instance;
			return &instance;
		}

		OutputMessage_ptr getOutputMessage(Protocol* protocol, bool autosend = true);

		void send(OutputMessage_ptr msg);
		void sendAll();

		void startExecutionFrame();
		void stop() {m_shutdown = true;}

		size_t getTotalMessageCount() const {return m_allOutputMessages.size();}
		size_t getAvailableMessageCount() const {return m_outputMessages.size();}
		size_t getAutoMessageCount() const {return m_autoSendOutputMessages.size();}

	protected:
		void configureOutputMessage(OutputMessage_ptr msg, Protocol* protocol, bool autosend);
		void internalReleaseMessage(OutputMessage* msg);

		typedef std::list<OutputMessage*> InternalOutputMessageList;
		typedef std::list<OutputMessage_ptr> OutputMessageList;

		InternalOutputMessageList m_outputMessages;
		InternalOutputMessageList m_allOutputMessages;
		OutputMessageList m_autoSendOutputMessages;

		OTSYS_THREAD_LOCKVAR m_outputPoolLock;
		uint64_t m_frameTime;
		bool m_shutdown;
};

#ifdef __TRACK_NETWORK__
	#define TRACK_MESSAGE(omsg) (omsg)->Track(__FILE__, __LINE__, __FUNCTION__)
#else
	#define TRACK_MESSAGE(omsg)
#endif
#endif
