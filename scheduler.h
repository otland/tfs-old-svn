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

#ifndef __SCHEDULER__
#define __SCHEDULER__

#include "otsystem.h"
#include "tasks.h"

#define SCHEDULER_MINTICKS 50
class SchedulerTask : public Task
{
	public:
		virtual ~SchedulerTask() {}

		void setEventId(uint32_t eventId) {m_eventId = eventId;}
		uint32_t getEventId() const {return m_eventId;}

		int64_t getCycle() const {return m_expiration;}
		bool operator<(const SchedulerTask& other) const {return getCycle() > other.getCycle();}

	protected:
		uint32_t m_eventId;

		SchedulerTask(uint32_t delay, const boost::function<void (void)>& f):
			Task(delay, f), m_eventId(0) {}
		friend SchedulerTask* createSchedulerTask(uint32_t, const boost::function<void (void)>&);
};

inline SchedulerTask* createSchedulerTask(uint32_t delay, const boost::function<void (void)>& f)
{
	assert(delay != 0);
	if(delay < SCHEDULER_MINTICKS)
		delay = SCHEDULER_MINTICKS;

	return new SchedulerTask(delay, f);
}
class lessSchedTask : public std::binary_function<SchedulerTask*&, SchedulerTask*&, bool>
{
	public:
		bool operator()(SchedulerTask*& t1, SchedulerTask*& t2) {return (*t1) < (*t2);}
};

typedef std::set<uint32_t> EventIdSet;
class Scheduler
{
	public:
		virtual ~Scheduler() {}
		static Scheduler& getScheduler()
		{
			static Scheduler scheduler;
			return scheduler;
		}

		uint32_t addEvent(SchedulerTask* task);
		bool stopEvent(uint32_t eventId);

		void stop();
		void shutdown();

		static OTSYS_THREAD_RETURN schedulerThread(void* p);

	protected:
		Scheduler();
		enum SchedulerState
		{
			STATE_RUNNING,
			STATE_CLOSING,
			STATE_TERMINATED
		};

		uint32_t m_lastEventId;
		EventIdSet m_eventIds;

		OTSYS_THREAD_LOCKVAR_PTR m_eventLock;
		OTSYS_THREAD_SIGNALVAR m_eventSignal;

		std::priority_queue<SchedulerTask*, std::vector<SchedulerTask*>, lessSchedTask > m_eventList;
		static SchedulerState m_threadState;
};
#endif
