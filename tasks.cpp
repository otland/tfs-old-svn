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

#include "tasks.h"
#include "outputmessage.h"
#include "game.h"

extern Game g_game;

#if defined __EXCEPTION_TRACER__
#include "exception.h"
#endif

Dispatcher::Dispatcher()
{
	OTSYS_THREAD_LOCKVARINIT(m_taskLock);
	OTSYS_THREAD_SIGNALVARINIT(m_taskSignal);
	m_threadState = STATE_TERMINATED;
}

void Dispatcher::start()
{
	m_threadState = STATE_RUNNING;
	OTSYS_CREATE_THREAD(Dispatcher::dispatcherThread, (void*)this);
}

OTSYS_THREAD_RETURN Dispatcher::dispatcherThread(void* p)
{
	Dispatcher* dispatcher = (Dispatcher*)p;
	#if defined __EXCEPTION_TRACER__
	ExceptionHandler dispatcherExceptionHandler;
	dispatcherExceptionHandler.InstallHandler();
	#endif
	srand((unsigned int)OTSYS_TIME());

	OutputMessagePool* outputPool;

	while(dispatcher->m_threadState != STATE_TERMINATED)
	{
		Task* task = NULL;

		// check if there are tasks waiting
		OTSYS_THREAD_LOCK(dispatcher->m_taskLock, "")

		if(dispatcher->m_taskList.empty())
		{
			//if the list is empty wait for signal
			OTSYS_THREAD_WAITSIGNAL(dispatcher->m_taskSignal, dispatcher->m_taskLock);
		}

		if(!dispatcher->m_taskList.empty() && (dispatcher->m_threadState != Dispatcher::STATE_TERMINATED))
		{
			// take the first task
			task = dispatcher->m_taskList.front();
			dispatcher->m_taskList.pop_front();
		}

		OTSYS_THREAD_UNLOCK(dispatcher->m_taskLock, "");

		// finally execute the task...
		if(task)
		{
			OutputMessagePool::getInstance()->startExecutionFrame();
			(*task)();
			delete task;

			outputPool = OutputMessagePool::getInstance();
			if(outputPool)
				outputPool->sendAll();

			OutputMessagePool::getInstance()->sendAll();
			g_game.clearSpectatorCache();
		}
	}
	#if defined __EXCEPTION_TRACER__
	dispatcherExceptionHandler.RemoveHandler();
	#endif
	#ifndef WIN32
	return NULL;
	#endif
}

void Dispatcher::addTask(Task* task)
{
	bool do_signal = false;
	OTSYS_THREAD_LOCK(m_taskLock, "");
	if(m_threadState == Dispatcher::STATE_RUNNING)
	{
		do_signal = m_taskList.empty();
		m_taskList.push_back(task);
	}
	else
	{
		#ifdef __DEBUG_SCHEDULER__
		std::cout << "Error: [Dispatcher::addTask] Dispatcher thread is terminated." << std::endl;
		#endif
		delete task;
	}
	OTSYS_THREAD_UNLOCK(m_taskLock, "");

	// send a signal if the list was empty
	if(do_signal)
		OTSYS_THREAD_SIGNAL_SEND(m_taskSignal);
}

void Dispatcher::flush()
{
	Task* task = NULL;
	while(!m_taskList.empty())
	{
		task = m_taskList.front();
		m_taskList.pop_front();
		(*task)();
		delete task;
		OutputMessagePool::getInstance()->sendAll();
		g_game.clearSpectatorCache();
	}
}

void Dispatcher::stop()
{
	OTSYS_THREAD_LOCK(m_taskLock, "");
	m_threadState = STATE_CLOSING;
	OTSYS_THREAD_UNLOCK(m_taskLock, "");
}

void Dispatcher::shutdown()
{
	OTSYS_THREAD_LOCK(m_taskLock, "");
	m_threadState = STATE_TERMINATED;
	flush();
	OTSYS_THREAD_UNLOCK(m_taskLock, "");
}
