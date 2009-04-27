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

#ifndef __OTSERV_OTSYSTEM_H__
#define __OTSERV_OTSYSTEM_H__
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
#ifdef __USE_BOOST_THREAD__
#include <boost/thread.hpp>
#endif
#include <boost/shared_ptr.hpp>
typedef std::vector<std::pair<uint32_t, uint32_t> > IPList;

#include <stddef.h>
#include <stdlib.h>
#include <sys/timeb.h>
#ifdef WIN32
#ifdef __WIN_LOW_FRAG_HEAP__
#define _WIN32_WINNT 0x0501
#endif
#include <process.h>
#include <windows.h>

inline int64_t OTSYS_TIME()
{
	_timeb t;
	_ftime(&t);
	return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}

#ifndef __USE_BOOST_THREAD__
#define OTSYS_CREATE_THREAD(a, b)	_beginthread(a, 0, b)
typedef CRITICAL_SECTION		OTSYS_THREAD_LOCKVAR;
typedef CRITICAL_SECTION		OTSYS_THREAD_LOCKVAR_PTR;

#define OTSYS_THREAD_LOCKVARINIT(a)	InitializeCriticalSection(&a);
#define OTSYS_THREAD_LOCKVARRELEASE(a)	DeleteCriticalSection(&a);
#define OTSYS_THREAD_LOCK(a, b)		EnterCriticalSection(&a);
#define OTSYS_THREAD_UNLOCK(a, b)	LeaveCriticalSection(&a);
#define OTSYS_THREAD_UNLOCK_PTR(a, b)	LeaveCriticalSection(a);

#define OTSYS_SLEEP(a)			Sleep(t);
typedef int64_t				OTSYS_THREAD_CYCLE;
#define OTSYS_THREAD_DELAY(a)		OTSYS_TIME() + a
#define OTSYS_THREAD_TIMEOUT		WAIT_TIMEOUT

typedef HANDLE				OTSYS_THREAD_SIGNALVAR;
#define OTSYS_THREAD_SIGNALVARINIT(a)	a = CreateEvent(NULL, FALSE, FALSE, NULL)
#define OTSYS_THREAD_SIGNAL_SEND(a)	SetEvent(a);
#define OTSYS_THREAD_SIGNAL_SEND_ALL(a) SetEvent(a);
typedef int				OTSYS_THREAD_SIGNAL_RETURN;

inline OTSYS_THREAD_SIGNAL_RETURN OTSYS_THREAD_WAITSIGNAL(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock)
{
	OTSYS_THREAD_UNLOCK(lock, "OTSYS_THREAD_WAITSIGNAL");
	WaitForSingleObject(signal, INFINITE);
	OTSYS_THREAD_LOCK(lock, "OTSYS_THREAD_WAITSIGNAL");

	return -0x4711;
}

inline OTSYS_THREAD_SIGNAL_RETURN OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock, OTSYS_THREAD_CYCLE cycle)
{
	int64_t tout64 = (cycle - OTSYS_TIME());
	DWORD tout = 0;
	if(tout64 > 0)
		tout = (DWORD)(tout64);

	OTSYS_THREAD_UNLOCK(lock, "OTSYS_THREAD_WAITSIGNAL_TIMED");
	OTSYS_THREAD_SIGNAL_RETURN ret = WaitForSingleObject(signal, tout);
	OTSYS_THREAD_LOCK(lock, "OTSYS_THREAD_WAITSIGNAL_TIMED");

	return ret;
}

#endif
#else
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

inline int64_t OTSYS_TIME()
{
	timeb t;
	ftime(&t);
	return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}

#ifndef __USE_BOOST_THREAD__
#define PTHREAD_MUTEX_RECURSIVE_NP	PTHREAD_MUTEX_RECURSIVE

inline void OTSYS_CREATE_THREAD(void *(*a)(void*), void *b)
{
	pthread_attr_t attr;
	pthread_t id;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&id, &attr, a, b);
}
typedef pthread_mutex_t			OTSYS_THREAD_LOCKVAR;
typedef pthread_mutex_t			OTSYS_THREAD_LOCKVAR_PTR;

inline void OTSYS_THREAD_LOCKVARINIT(OTSYS_THREAD_LOCKVAR& l)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&l, &attr);
}
#define OTSYS_THREAD_LOCKVARRELEASE(a)	//todo?
#define OTSYS_THREAD_LOCK(a, b)		pthread_mutex_lock(&a);
#define OTSYS_THREAD_UNLOCK(a, b)	pthread_mutex_unlock(&a);
#define OTSYS_THREAD_UNLOCK_PTR(a, b)	pthread_mutex_unlock(a);

inline void OTSYS_SLEEP(int32_t t)
{
	timespec tv;
	tv.tv_sec  = t / 1000;
	tv.tv_nsec = (t % 1000) * 1000000;
	nanosleep(&tv, NULL);
}
typedef int64_t				OTSYS_THREAD_CYCLE;
#define OTSYS_THREAD_DELAY(a)		OTSYS_TIME() + a
#define OTSYS_THREAD_TIMEOUT		ETIMEDOUT;

typedef pthread_cond_t			OTSYS_THREAD_SIGNALVAR;
#define OTSYS_THREAD_SIGNALVARINIT(a)	pthread_cond_init(&a, NULL);
#define OTSYS_THREAD_SIGNAL_SEND(a)	pthread_cond_signal(&a);
#define OTSYS_THREAD_SIGNAL_SEND_ALL(a) pthread_cond_signal(&a);
typedef int				OTSYS_THREAD_SIGNAL_RETURN;

inline OTSYS_THREAD_SIGNAL_RETURN OTSYS_THREAD_WAITSIGNAL(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock)
{
	return pthread_cond_wait(&signal, &lock);
}

inline OTSYS_THREAD_SIGNAL_RETURN OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock, OTSYS_THREAD_CYCLE cycle)
{
	timespec tv;
	tv.tv_sec = (int64_t)(cycle / 1000);
	tv.tv_nsec = (int64_t)(cycle % 1000) * 1000000;
	return pthread_cond_timedwait(&signal, &lock, &tv);
}

#endif
#endif
#ifdef __USE_BOOST_THREAD__
#define OTSYS_SLEEP(time)				boost::this_thread::sleep(boost::posix_time::milliseconds(time))

#define OTSYS_CREATE_THREAD(a, b)			boost::thread(boost::bind(&a, (void*)b))
typedef boost::recursive_mutex				OTSYS_THREAD_LOCKVAR;
typedef boost::mutex					OTSYS_THREAD_LOCKVAR_PTR;
typedef boost::unique_lock<OTSYS_THREAD_LOCKVAR_PTR>	OTSYS_THREAD_UNIQUE;
typedef boost::defer_lock				OTSYS_THREAD_UNIQUE_VAL

#define OTSYS_THREAD_LOCKVARINIT(a)			//todo?
#define OTSYS_THREAD_LOCKVARRELEASE(a)			//todo?
#define OTSYS_THREAD_LOCK(a, b)				a.lock();
#define OTSYS_THREAD_UNLOCK(a, b)			a.unlock();
#define OTSYS_THREAD_UNLOCK_PTR(a, b)			a->unlock();

typedef boost::system_time				OTSYS_THREAD_CYCLE;
#define OTSYS_THREAD_DELAY(a)				boost::get_system_time() + boost::posix_time::milliseconds(a)
#define OTSYS_THREAD_TIMEOUT				false

typedef boost::condition_variable			OTSYS_THREAD_SIGNALVAR;
#define OTSYS_THREAD_SIGNALVARINIT(a)			//todo?
#define OTSYS_THREAD_SIGNAL_SEND(a)			a.notify_one()
#define OTSYS_THREAD_SIGNAL_SEND_ALL(a)			a.notify_all()
typedef bool						OTSYS_THREAD_SIGNAL_RETURN;

inline OTSYS_THREAD_SIGNAL_RETURN OTSYS_THREAD_WAITSIGNAL(OTSYS_THREAD_SIGNALVAR& a, OTSYS_THREAD_UNIQUE& b)
{
	a.wait(b);
	return false;
}

inline OTSYS_THREAD_SIGNAL_RETURN OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR&a, OTSYS_THREAD_UNIQUE&b, OTSYS_THREAD_CYCLE c)
{
	return a.timed_wait(b, c);
}

typedef boost::recursive_mutex::scoped_lock OTSYS_THREAD_LOCK_CLASS;
#else
class OTSYS_THREAD_LOCK_CLASS
{
	public:
		inline OTSYS_THREAD_LOCK_CLASS(OTSYS_THREAD_LOCKVAR &a)
		{
			mutex = &a;
			OTSYS_THREAD_LOCK(a, "")
		}

		inline OTSYS_THREAD_LOCK_CLASS(OTSYS_THREAD_LOCKVAR &a, const char* s)
		{
			mutex = &a;
			OTSYS_THREAD_LOCK(a, "")
		}

		inline ~OTSYS_THREAD_LOCK_CLASS()
		{
			OTSYS_THREAD_UNLOCK_PTR(mutex, "")
		}

		OTSYS_THREAD_LOCKVAR *mutex;
};
#endif

#ifdef __GNUC__
#define __OTSERV_PRETTY_FUNCTION__ __PRETTY_FUNCTION__
#endif
#ifdef _MSC_VER
#define __OTSERV_PRETTY_FUNCTION__ __FUNCDNAME__
#endif

#endif // #ifndef __OTSYSTEM_H__
