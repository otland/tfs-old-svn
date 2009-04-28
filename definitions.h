//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// various definitions needed by most files
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

#ifndef __OTSERV_DEFINITIONS_H__
#define __OTSERV_DEFINITIONS_H__

#undef MULTI_SQL_DRIVERS
#define SQL_DRIVERS __USE_SQLITE__+__USE_MYSQL__+__USE_ODBC__+__USE_PGSQL__
#if SQL_DRIVERS > 1
#define MULTI_SQL_DRIVERS
#endif

#ifndef WIN32
	#define __CONSOLE__
#endif

#ifdef XML_GCC_FREE
	#define xmlFreeOTSERV(s)	free(s)
#else
	#define xmlFreeOTSERV(s)	xmlFree(s)
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

#if defined _WIN32
#  ifndef WIN32
#    define WIN32
#  endif
#endif

#if defined __WINDOWS__ || defined WIN32

#if defined _MSC_VER && defined NDEBUG
	#define _SECURE_SCL 0
	#define HAS_ITERATOR_DEBUGGING 0
#endif

#ifndef __FUNCTION__
	#define	__FUNCTION__ __func__
#endif

#define OTSYS_THREAD_RETURN void

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

//Windows 2000	0x0500
//Windows XP	0x0501
//Windows 2003	0x0502
//Windows Vista	0x0600
#define _WIN32_WINNT 0x0501

#ifdef __GNUC__
	#if __GNUC__ >= 4
		#ifndef __GXX_EXPERIMENTAL_CXX0X__
			#include <tr1/unordered_map>
			#include <tr1/unordered_set>
		#else
			#include <unordered_map>
			#include <unordered_set>
		#endif

		#define OTSERV_HASH_MAP std::tr1::unordered_map
		#define OTSERV_HASH_SET std::tr1::unordered_set
	#else
		#include <ext/hash_map>
		#include <ext/hash_set>
		#define OTSERV_HASH_MAP __gnu_cxx::hash_map
		#define OTSERV_HASH_SET __gnu_cxx::hash_set
	#endif
	#include <assert.h>
	#define ATOI64 atoll
#else
	#define _WIN32_WINNT 0x0500

	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#include <hash_map>
	#include <hash_set>
	#include <limits>
	#include <assert.h>
	#define OTSERV_HASH_MAP stdext::hash_map
	#define OTSERV_HASH_SET stdext::hash_set

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

	#define ATOI64 _atoi64

	#pragma warning(disable:4786) // msvc too long debug names in stl
	#pragma warning(disable:4250) // 'class1' : inherits 'class2::member' via dominance
	#pragma warning(disable:4244)
	#pragma warning(disable:4267)
	#pragma warning(disable:4018)

#endif
//*nix systems
#else
	#define OTSYS_THREAD_RETURN void*

	#include <stdint.h>
	#include <string.h>
	#if __GNUC__ >= 4
		#ifndef __GXX_EXPERIMENTAL_CXX0X__
			#include <tr1/unordered_map>
			#include <tr1/unordered_set>
		#else
			#include <unordered_map>
			#include <unordered_set>
		#endif

		#define OTSERV_HASH_MAP std::tr1::unordered_map
		#define OTSERV_HASH_SET std::tr1::unordered_set
	#else
		#include <ext/hash_map>
		#include <ext/hash_set>
		#define OTSERV_HASH_MAP __gnu_cxx::hash_map
		#define OTSERV_HASH_SET __gnu_cxx::hash_set
	#endif
	#include <assert.h>
	#include <time.h>

	#define ATOI64 atoll
#endif
#endif
