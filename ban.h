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

struct idBan
{
	uint32_t id;
	uint32_t time;
	int32_t reasonId;
	int32_t actionId;
	std::string comment;
	uint32_t bannedBy;
	idBan(uint32_t _id, uint32_t _time, int32_t _reasonId, int32_t _actionId, std::string _comment, uint32_t _bannedBy)
	{
		id = _id;
		time = _time;
		reasonId = _reasonId;
		actionId = _actionId;
		comment = _comment;
		bannedBy = _bannedBy;
	}
};
typedef idBan AccountBanStruct;
typedef idBan AccountDeletionStruct;

struct idNamelock
{
	uint32_t id;
	idNamelock(uint32_t _id)
	{
		id = _id;
	}
};
typedef idNamelock PlayerNamelockStruct;

struct idNotation
{
	uint32_t id;
	uint32_t time;
	std::string comment;
	uint32_t bannedBy;
	idNotation(uint32_t _id, uint32_t _time, std::string _comment, uint32_t _bannedBy)
	{
		id = _id;
		time = _time;
		comment = _comment;
		bannedBy = _bannedBy;
	}
};
typedef idNotation AccountNotationStruct;

struct IpBanStruct
{
	uint32_t ip;
	uint32_t mask;
	uint32_t time;
	IpBanStruct(uint32_t _ip, uint32_t _mask, uint32_t _time)
	{
		ip = _ip;
		mask = _mask;
		time = _time;
	}
};

struct LoginBlock
{
	uint32_t lastLoginTime;
	uint32_t numberOfLogins;
};

typedef std::list< IpBanStruct > IpBanList;
typedef std::list< PlayerNamelockStruct > PlayerNamelockList;
typedef std::list< AccountNotationStruct > AccountNotationList;
typedef std::list< AccountBanStruct > AccountBanList;
typedef std::list< AccountDeletionStruct > AccountDeletionList;
typedef std::map<uint32_t, LoginBlock > IpLoginMap;

enum BanType_t
{
	BAN_IPADDRESS = 1,
	NAMELOCK_PLAYER = 2,
	BAN_ACCOUNT = 3,
	NOTATION_ACCOUNT = 4,
	DELETE_ACCOUNT = 5
};

class Ban
{
	public:
		Ban();
		virtual ~Ban() {}
		void init();

		bool isIpBanished(uint32_t clientip);
		bool isPlayerNamelocked(const std::string& name);
		bool isIpDisabled(uint32_t clientip);

		void addIpBan(uint32_t ip, uint32_t mask, uint64_t time);
		void addPlayerNamelock(uint32_t playerId);
		void addAccountBan(uint32_t account, uint64_t time, int32_t reasonId, int32_t actionId, std::string comment, uint32_t bannedBy);
		void addAccountDeletion(uint32_t account, uint64_t time, int32_t reasonId, int32_t actionId, std::string comment, uint32_t bannedBy);
		void addAccountNotation(uint32_t account, uint64_t time, std::string comment, uint32_t bannedBy);
		void addLoginAttempt(uint32_t clientip, bool isSuccess);

		bool getBanInformation(uint32_t account, uint32_t& bannedBy, uint32_t& banTime, int32_t& reason, int32_t& action, std::string& comment, bool& deletion);
		int32_t getNotationsCount(uint32_t account);

		bool removeAccountBan(uint32_t account);
		bool removeAccountNotations(uint32_t account);
		bool removeAccountDeletion(uint32_t account);
		bool removeIPBan(uint32_t ip);
		bool removePlayerNamelock(uint32_t guid);

		bool loadBans();
		bool saveBans();

		const IpBanList& getIpBans();
		const PlayerNamelockList& getPlayerNamelocks();
		const AccountBanList& getAccountBans();

	protected:
		IpBanList ipBanList;
		PlayerNamelockList playerNamelockList;
		AccountNotationList accountNotationList;
		AccountBanList accountBanList;
		AccountDeletionList accountDeletionList;
		IpLoginMap ipLoginMap;

		uint32_t loginTimeout;
		uint32_t maxLoginTries;
		uint32_t retryTimeout;

		OTSYS_THREAD_LOCKVAR banLock;

		friend class IOBan;
};

class IOBan
{
	public:
		static IOBan* getInstance()
		{
			static IOBan instance;
			return &instance;
		}

		virtual bool loadBans(Ban& banclass);
		virtual bool saveBans(const Ban& banclass);

	protected:
		IOBan() {}
		virtual ~IOBan() {}
};

#endif
