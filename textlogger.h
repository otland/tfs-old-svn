//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Textlogger
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

#ifndef __TEXTLOGGER_H__
#define __TEXTLOGGER_H__
#if defined(WIN32) && not defined(__CONSOLE__)
#include <windows.h>
#include <ostream>

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
