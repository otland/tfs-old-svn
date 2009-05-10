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
#include "tasks.h"

#include "outputmessage.h"
#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif

#include "game.h"
extern Game g_game;

Dispatcher::DispatcherState Dispatcher::m_threadState = Dispatcher::STATE_TERMINATED;

Dispatcher::Dispatcher()
{
	m_taskList.clear();
	Dispatcher::m_threadState = Dispatcher::STATE_RUNNING;
	OTSYS_THREAD_LOCKVARINIT(m_taskLock);
	OTSYS_THREAD_SIGNALVARINIT(m_taskSignal);
	OTSYS_CREATE_THREAD(Dispatcher::dispatcherThread, NULL);
}

OTSYS_THREAD_RETURN Dispatcher::dispatcherThread(void* p)
{
	#if defined __EXCEPTION_TRACER__
	ExceptionHandler dispatcherExceptionHandler;
	dispatcherExceptionHandler.InstallHandler();
	#endif
	srand((uint32_t)OTSYS_TIME());

	OutputMessagePool* outputPool = NULL;
	while(Dispatcher::m_threadState != Dispatcher::STATE_TERMINATED)
	{
		Task* task = NULL;
		// check if there are tasks waiting
		OTSYS_THREAD_LOCK(getDispatcher().m_taskLock, "");
		if(getDispatcher().m_taskList.empty()) //if the list is empty wait for signal
			OTSYS_THREAD_WAITSIGNAL(getDispatcher().m_taskSignal, getDispatcher().m_taskLock);

		if(!getDispatcher().m_taskList.empty() && Dispatcher::m_threadState != Dispatcher::STATE_TERMINATED)
		{
			// take the first task
			task = getDispatcher().m_taskList.front();
			getDispatcher().m_taskList.pop_front();
		}

		OTSYS_THREAD_UNLOCK(getDispatcher().m_taskLock, "");
		// finally execute the task...
		if(!task)
			continue;

		if(!task->hasExpired())
		{
			if((outputPool = OutputMessagePool::getInstance()))
				outputPool->startExecutionFrame();

			(*task)();
			if(outputPool)
				outputPool->sendAll();

			g_game.clearSpectatorCache();
		}

		delete task;
	}

	#if defined __EXCEPTION_TRACER__
	dispatcherExceptionHandler.RemoveHandler();
	#endif
	#if not defined(WIN32)
	return NULL;
	#endif
}

void Dispatcher::addTask(Task* task, bool front/* = false*/)
{
	bool signal = false;
	if(Dispatcher::m_threadState == Dispatcher::STATE_RUNNING)
	{
		OTSYS_THREAD_LOCK(m_taskLock, "");
		signal = m_taskList.empty();

		if(front)
			m_taskList.push_front(task);
		else
			m_taskList.push_back(task);

		OTSYS_THREAD_UNLOCK(m_taskLock, "");
	}
	#ifdef __DEBUG_SCHEDULER__
	else
		std::cout << "[Error - Dispatcher::addTask] Dispatcher thread is terminated." << std::endl;
	#endif

	// send a signal if the list was empty
	if(signal)
		OTSYS_THREAD_SIGNAL_SEND(m_taskSignal);
}

void Dispatcher::flush()
{
	Task* task = NULL;
	OutputMessagePool* outputPool = NULL;
	while(!m_taskList.empty())
	{
		task = getDispatcher().m_taskList.front();
		m_taskList.pop_front();

		(*task)();
		delete task;

		outputPool = OutputMessagePool::getInstance();
		if(outputPool)
			outputPool->sendAll();

		g_game.clearSpectatorCache();
	}
}

void Dispatcher::stop()
{
	OTSYS_THREAD_LOCK(m_taskLock, "");
	m_threadState = Dispatcher::STATE_CLOSING;
	OTSYS_THREAD_UNLOCK(m_taskLock, "");
}

void Dispatcher::shutdown()
{
	OTSYS_THREAD_LOCK(m_taskLock, "");
	m_threadState = Dispatcher::STATE_TERMINATED;
	flush();
	OTSYS_THREAD_UNLOCK(m_taskLock, "");
}
