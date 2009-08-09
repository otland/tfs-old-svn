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

#ifndef __OTSYSTEM__
#define __OTSYSTEM__
#include "definitions.h"

#include <string>
#include <algorithm>
#include <bitset>
#include <queue>
#include <set>
#include <vector>
#include <list>
#include <map>

#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

#include <stddef.h>
#include <stdlib.h>
#include <sys/timeb.h>
#ifdef WIN32
#include <process.h>
#include <windows.h>

#ifdef __WIN_LOW_FRAG_HEAP__
#define _WIN32_WINNT 0x0501
#endif
#define OTSYS_SLEEP(t)			Sleep(t);

#define OTSYS_CREATE_THREAD(a, b)	_beginthread(a, 0, b)
#define OTSYS_THREAD_LOCKVAR		CRITICAL_SECTION
#define OTSYS_THREAD_LOCKVAR_PTR	CRITICAL_SECTION

#define OTSYS_THREAD_LOCKVARINIT(a)	InitializeCriticalSection(&a)
#define OTSYS_THREAD_LOCKVARRELEASE(a)	DeleteCriticalSection(&a)
#define OTSYS_THREAD_LOCK(a, b)		EnterCriticalSection(&a)
#define OTSYS_THREAD_UNLOCK(a, b)	LeaveCriticalSection(&a)
#define OTSYS_THREAD_UNLOCK_PTR(a, b)	LeaveCriticalSection(a)

#define OTSYS_THREAD_TIMEOUT		WAIT_TIMEOUT
#define OTSYS_THREAD_SIGNALVARINIT(a)	a = CreateEvent(NULL, FALSE, FALSE, NULL)
#define OTSYS_THREAD_SIGNAL_SEND(a)	SetEvent(a);

typedef HANDLE OTSYS_THREAD_SIGNALVAR;

inline int64_t OTSYS_TIME()
{
	_timeb t;
	_ftime(&t);
	return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}

inline int OTSYS_THREAD_WAITSIGNAL(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock)
{
	//LeaveCriticalSection(&lock);
	OTSYS_THREAD_UNLOCK(lock, "OTSYS_THREAD_WAITSIGNAL");
	WaitForSingleObject(signal, INFINITE);
	//EnterCriticalSection(&lock);
	OTSYS_THREAD_LOCK(lock, "OTSYS_THREAD_WAITSIGNAL");

	return -0x4711;
}

inline int OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock, int64_t cycle)
{
	int64_t tout64 = (cycle - OTSYS_TIME());

	DWORD tout = 0;
	if(tout64 > 0)
		tout = (DWORD)(tout64);

	OTSYS_THREAD_UNLOCK(lock, "OTSYS_THREAD_WAITSIGNAL_TIMED");
	int ret = WaitForSingleObject(signal, tout);
	OTSYS_THREAD_LOCK(lock, "OTSYS_THREAD_WAITSIGNAL_TIMED");

	return ret;
}
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE

#define OTSYS_THREAD_LOCKVARRELEASE(a)	//todo: working macro
#define OTSYS_THREAD_LOCK(a, b)		pthread_mutex_lock(&a)
#define OTSYS_THREAD_UNLOCK(a, b)	pthread_mutex_unlock(&a)
#define OTSYS_THREAD_UNLOCK_PTR(a, b)	pthread_mutex_unlock(a)

#define OTSYS_THREAD_TIMEOUT		ETIMEDOUT
#define OTSYS_THREAD_SIGNALVARINIT(a)	pthread_cond_init(&a, NULL)
#define OTSYS_THREAD_SIGNAL_SEND(a)	pthread_cond_signal(&a)

typedef pthread_mutex_t OTSYS_THREAD_LOCKVAR;
typedef pthread_mutex_t OTSYS_THREAD_LOCKVAR_PTR;
typedef pthread_cond_t OTSYS_THREAD_SIGNALVAR;

inline void OTSYS_CREATE_THREAD(void *(*a)(void*), void *b)
{
	pthread_attr_t attr;
	pthread_t id;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	pthread_create(&id, &attr, a, b);
}

inline void OTSYS_THREAD_LOCKVARINIT(OTSYS_THREAD_LOCKVAR& l)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&l, &attr);
}

inline void OTSYS_SLEEP(int t)
{
	timespec tv;
	tv.tv_sec = t / 1000;
	tv.tv_nsec = (t % 1000) * 1000000;
	nanosleep(&tv, NULL);
}

inline int OTSYS_THREAD_WAITSIGNAL(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock)
{
	return pthread_cond_wait(&signal, &lock);
}

inline int OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock, int64_t cycle)
{
	timespec tv;
	tv.tv_sec = (int64_t)(cycle / 1000);
	tv.tv_nsec = (int64_t)(cycle % 1000) * 1000000;
	return pthread_cond_timedwait(&signal, &lock, &tv);
}

inline int64_t OTSYS_TIME()
{
	timeb t;
	ftime(&t);
	return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}
#endif

class OTSYS_THREAD_LOCK_CLASS
{
	public:
		inline OTSYS_THREAD_LOCK_CLASS(OTSYS_THREAD_LOCKVAR &a)
		{
			mutex = &a;
			OTSYS_THREAD_LOCK(a, "");
		}

		inline OTSYS_THREAD_LOCK_CLASS(OTSYS_THREAD_LOCKVAR &a, const char* s)
		{
			mutex = &a;
			OTSYS_THREAD_LOCK(a, "");
		}

		inline ~OTSYS_THREAD_LOCK_CLASS()
		{
			OTSYS_THREAD_UNLOCK_PTR(mutex, "");
		}

		OTSYS_THREAD_LOCKVAR *mutex;
};

#ifdef __GNUC__
#define __OTSERV_PRETTY_FUNCTION__ __PRETTY_FUNCTION__
#endif
#ifdef _MSC_VER
#define __OTSERV_PRETTY_FUNCTION__ __FUNCDNAME__
#endif

typedef std::vector<std::pair<uint32_t, uint32_t> > IpList;
#endif
