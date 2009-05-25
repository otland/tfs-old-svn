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

#ifndef __TEXTLOGGER__
#define __TEXTLOGGER__
#include "otsystem.h"

#if defined(WIN32) && not defined(__CONSOLE__)
#include <ostream>
#include <fstream>
#endif

enum LogType_t
{
	LOGTYPE_EVENT,
	LOGTYPE_WARNING,
	LOGTYPE_ERROR,
};

class Loggar
{
	public:
		virtual ~Loggar();
		static Loggar* getInstance()
		{
			static Loggar instance;
			return &instance;
		}

		void logMessage(const char* channel, LogType_t type, int32_t level, std::string message, const char* func);

	private:
		Loggar();
		FILE* m_file;
};

#define LOG_MESSAGE(channel, type, level, message) \
	Loggar::getInstance()->logMessage(channel, type, level, message, __OTSERV_PRETTY_FUNCTION__);

#if defined(WIN32) && not defined(__CONSOLE__)
class TextLogger : public std::streambuf
{
	public:
		TextLogger();
		virtual ~TextLogger();

		std::streambuf* out;
		std::streambuf* err;

	protected:
		int32_t overflow(int32_t c);
		bool displayDate;
};
#endif
#endif
