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

#ifndef __DEFINITIONS__
#define __DEFINITIONS__
#undef MULTI_SQL_DRIVERS
#define SQL_DRIVERS __USE_SQLITE__+__USE_MYSQL__+__USE_ODBC__+__USE_PGSQL__
#if SQL_DRIVERS > 1
#define MULTI_SQL_DRIVERS
#endif

#ifdef __MINGW32__
	#define XML_GCC_FREE
	#ifndef __WINDOWS__
		#define __WINDOWS__
	#endif
#endif

#if defined _WIN32 || defined WIN32 || defined _WIN64 || defined WIN64 || defined __WINDOWS__ || defined WINDOWS
	#if defined _WIN64 || defined WIN64
		#ifndef _WIN64
			#define _WIN64
		#endif
		#ifndef WIN64
			#define WIN64
		#endif
	#else
		#ifndef _WIN32
			#define _WIN32
		#endif
		#ifndef WIN32
			#define WIN32
		#endif
	#endif
	#ifndef __WINDOWS__
		#define __WINDOWS__
	#endif
	#ifndef WINDOWS
		#define WINDOWS
	#endif
#endif

#ifdef __CYGWIN__
	#undef WIN32
	#undef WIN64
	#undef _WIN32
	#undef _WIN64
	#undef WINDOWS
	#undef __WINDOWS__
	#define HAVE_ERRNO_AS_DEFINE
#endif

#ifdef XML_GCC_FREE
	#define xmlFree(s) free(s)
#endif

#ifdef __USE_MINIDUMP__
	#ifndef __EXCEPTION_TRACER__
		#define __EXCEPTION_TRACER__
	#endif
#endif

#ifdef __DEBUG_EXCEPTION_REPORT__
	#define DEBUG_REPORT int *a = NULL; *a = 1;
#else
	#ifdef __EXCEPTION_TRACER__
		#include "exception.h"
		#define DEBUG_REPORT ExceptionHandler::dumpStack();
	#else
		#define DEBUG_REPORT
	#endif
#endif

#define BOOST_ASIO_ENABLE_CANCELIO 1
#if defined WINDOWS
#if defined _MSC_VER && defined NDEBUG
	#define _SECURE_SCL 0
	#define HAS_ITERATOR_DEBUGGING 0
#endif

#ifndef __FUNCTION__
	#define	__FUNCTION__ __func__
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#ifdef _WIN64_WINNT
#undef _WIN64_WINNT
#endif

//Windows 2000	0x0500
//Windows XP	0x0501
//Windows 2003	0x0502
//Windows Vista	0x0600
//Windows Seven 0x0601

#define _WIN32_WINNT 0x0501
#define _WIN64_WINNT 0x0501

#ifndef __GNUC__
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#include <cstring>
	inline int strcasecmp(const char *s1, const char *s2)
	{
		return ::_stricmp(s1, s2);
	}

	inline int strncasecmp(const char *s1, const char *s2, size_t n)
	{
		return ::_strnicmp(s1, s2, n);
	}

	typedef unsigned long long uint64_t;
	typedef signed long long int64_t;
	typedef unsigned long uint32_t;
	typedef signed long int32_t;
	typedef unsigned short uint16_t;
	typedef signed short int16_t;
	typedef unsigned char uint8_t;
	typedef signed char int8_t;

	#define atoll _atoi64

	#pragma warning(disable:4786) // msvc too long debug names in stl
	#pragma warning(disable:4250) // 'class1' : inherits 'class2::member' via dominance
	#pragma warning(disable:4244)
	#pragma warning(disable:4267)
	#pragma warning(disable:4018)

#endif
#endif
#endif

