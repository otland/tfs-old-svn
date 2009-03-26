//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Account class
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

#ifndef __OTSERV_ACCOUNT_H__
#define __OTSERV_ACCOUNT_H__
#include "otsystem.h"
#ifndef __LOGIN_SERVER__

typedef std::list<std::string> Characters;
#else
#include "gameservers.h"

typedef std::map<std::string, GameServer*> Characters;
#endif

class Account
{
	public:
		Account();
		virtual ~Account();

		uint32_t number, premiumDays, lastDay;
		int32_t warnings;
		std::string name, password, recoveryKey;
		Characters charList;
};

#endif
