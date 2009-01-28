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

#ifndef __OTSERV_BAN_H__
#define __OTSERV_BAN_H__

#include "otsystem.h"
#include <list>
#include "player.h"

enum BanType_t
{
	BANTYPE_IP_BANISHMENT = 1,
	BANTYPE_NAMELOCK = 2,
	BANTYPE_BANISHMENT = 3,
	BANTYPE_NOTATION = 4,
	BANTYPE_DELETION = 5
};

struct Ban
{
	BanType_t type;
	std::string comment, value, param;
	uint32_t id, added, expires, adminid, reason, action;
};

typedef std::vector<Ban> BansVec;

class IOBan
{
	protected:
		IOBan() {}

	public:
		virtual ~IOBan() {}
		static IOBan* getInstance()
		{
			static IOBan instance;
			return &instance;
		}

		bool isIpBanished(uint32_t ip, uint32_t mask = 0xFFFFFFFF);
		bool isNamelocked(uint32_t guid);
		bool isNamelocked(std::string name);
		bool isBanished(uint32_t account);
		bool isDeleted(uint32_t account);

		bool addIpBanishment(uint32_t ip, time_t banTime, std::string comment, uint32_t gamemaster,
			std::string statement = "");
		bool addNamelock(uint32_t playerId, uint32_t reasonId, uint32_t actionId, std::string comment,
			uint32_t gamemaster, std::string statement = "");
		bool addNamelock(std::string name, uint32_t reasonId, uint32_t actionId, std::string comment,
			uint32_t gamemaster, std::string statement = "");
		bool addBanishment(uint32_t account, time_t banTime, uint32_t reasonId, uint32_t actionId,
			std::string comment, uint32_t gamemaster, std::string statement = "");
		bool addDeletion(uint32_t account, uint32_t reasonId, uint32_t actionId, std::string comment,
			uint32_t gamemaster, std::string statement = "");
		void addNotation(uint32_t account, uint32_t reasonId, uint32_t actionId, std::string comment,
			uint32_t gamemaster, std::string statement = "");

		bool removeIpBanishment(uint32_t ip);
		bool removeNamelock(uint32_t guid);
		bool removeNamelock(std::string name);
		bool removeBanishment(uint32_t account);
		bool removeDeletion(uint32_t account);
		void removeNotations(uint32_t account);

		bool getData(uint32_t value, Ban& ban);
		std::vector<Ban> getList(BanType_t type, uint32_t value = 0);

		//id is account or guid, for guid needed player = true
		uint32_t getAction(uint32_t id, bool player = false);
		uint32_t getReason(uint32_t id, bool player = false);
		uint64_t getExpireTime(uint32_t id, bool player = false);
		uint64_t getAddedTime(uint32_t id, bool player = false);
		std::string getComment(uint32_t id, bool player = false);
		uint32_t getAdminGUID(uint32_t id, bool player = false);

		uint32_t getNotationsCount(uint32_t account);
		bool clearTemporials();
};

#endif
