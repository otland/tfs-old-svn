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
#include <limits>

#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#include <time.h>
#include <assert.h>
#ifdef WINDOWS
#include <windows.h>
#include <sys/timeb.h>

#define OTSERV_ACCESS(file, mode) _access(file, mode);
inline int64_t OTSYS_TIME()
{
	_timeb t;
	_ftime(&t);
	return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}
#else
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define OTSERV_ACCESS(file, mode) access(file, mode);
inline int64_t OTSYS_TIME()
{
	timeb t;
	ftime(&t);
	return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}
#endif

#ifdef __GNUC__
	#define __OTSERV_FUNCTION__ __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
	#define __OTSERV_FUNCTION__ __FUNCDNAME__
#endif

#define foreach BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH

typedef std::vector<std::pair<uint32_t, uint32_t> > IpList;
#endif
