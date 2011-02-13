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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "definitions.h"

#include "ban.h"
#include "iologindata.h"
#include "configmanager.h"
#include "tools.h"
#include "database.h"

extern ConfigManager g_config;

Ban::Ban()
{
	OTSYS_THREAD_LOCKVARINIT(banLock);
}

void Ban::init()
{
	maxLoginTries = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TRIES);
	retryTimeout = (uint32_t)g_config.getNumber(ConfigManager::RETRY_TIMEOUT) / 1000;
	loginTimeout = (uint32_t)g_config.getNumber(ConfigManager::LOGIN_TIMEOUT) / 1000;
}

bool Ban::acceptConnection(uint32_t clientip)
{
	if(clientip == 0)
		return false;

	OTSYS_THREAD_LOCK(banLock, "");

	uint64_t currentTime = OTSYS_TIME();
	IpConnectMap::iterator it = ipConnectMap.find(clientip);
	if(it == ipConnectMap.end())
	{
		ConnectBlock cb;
		cb.startTime = currentTime;
		cb.blockTime = 0;
		cb.count = 1;

		ipConnectMap[clientip] = cb;
		OTSYS_THREAD_UNLOCK(banLock, "");
		return true;
	}

	it->second.count++;
	if(it->second.blockTime > currentTime)
	{
		OTSYS_THREAD_UNLOCK(banLock, "");
		return false;
	}

	if(currentTime - it->second.startTime > 1000)
	{
		uint32_t connectionPerSec = it->second.count;
		it->second.startTime = currentTime;
		it->second.count = 0;
		it->second.blockTime = 0;
		if(connectionPerSec > 10)
		{
			it->second.blockTime = currentTime + 10000;
			OTSYS_THREAD_UNLOCK(banLock, "");
			return false;
		}
	}

	OTSYS_THREAD_UNLOCK(banLock, "");
	return true;
}

void Ban::addLoginAttempt(uint32_t clientip, bool isSuccess)
{
	if(clientip == 0)
		return;

	OTSYS_THREAD_LOCK(banLock, "");

	time_t currentTime = time(NULL);
	IpLoginMap::iterator it = ipLoginMap.find(clientip);
	if(it == ipLoginMap.end())
	{
		LoginBlock lb;
		lb.lastLoginTime = 0;
		lb.numberOfLogins = 0;

		ipLoginMap[clientip] = lb;
		it = ipLoginMap.find(clientip);
	}

	if(it->second.numberOfLogins >= maxLoginTries)
		it->second.numberOfLogins = 0;

	if(!isSuccess || ((uint32_t)currentTime < (uint32_t)it->second.lastLoginTime + retryTimeout))
		++it->second.numberOfLogins;
	else
		it->second.numberOfLogins = 0;

	it->second.lastLoginTime = currentTime;
	OTSYS_THREAD_UNLOCK(banLock, "");
}

bool Ban::isIpDisabled(uint32_t clientip)
{
	if(maxLoginTries == 0 || clientip == 0)
		return false;

	OTSYS_THREAD_LOCK(banLock, "");

	time_t currentTime = time(NULL);
	IpLoginMap::iterator it = ipLoginMap.find(clientip);
	if(it != ipLoginMap.end())
	{
		if((it->second.numberOfLogins >= maxLoginTries) && ((uint32_t)currentTime < it->second.lastLoginTime + loginTimeout))
		{
			OTSYS_THREAD_UNLOCK(banLock, "");
			return true;
		}
	}
	OTSYS_THREAD_UNLOCK(banLock, "");
	return false;
}

bool IOBan::isIpBanished(uint32_t clientip)
{
	if(clientip == 0)
		return false;

	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	DBQuery query;
	DBResult result;
	query << "SELECT `ip`, `mask`, `time` FROM `bans` WHERE `type` = " << BAN_IPADDRESS << ";";
	if(db->storeQuery(query, result))
	{
		for(uint32_t i = 0; i < result.getNumRows(); ++i)
		{
			uint32_t ip = result.getDataInt("ip", i);
			uint32_t mask = result.getDataInt("mask", i);
			if((ip & mask) == (clientip & mask))
			{
				uint32_t currentTime = time(NULL);
				uint32_t time = result.getDataInt("time", i);
				if(time == 0 || currentTime < time)
					return true;
			}
		}
	}
	return false;
}

bool IOBan::isPlayerNamelocked(const std::string& name)
{
	uint32_t playerId;
	std::string playerName = name;
	if(!IOLoginData::getInstance()->getGuidByName(playerId, playerName))
		return true;

	Database* db = Database::getInstance();
	if(!db->connect())
		return true;

	DBQuery query;
	DBResult result;
	query << "SELECT `type` FROM `bans` WHERE `type` = " << NAMELOCK_PLAYER << " AND `player` = " << playerId << " LIMIT 1;";
	if(db->storeQuery(query, result))
		return result.getNumRows() == 1;

	return false;
}

bool IOBan::isAccountBanned(uint32_t account)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	DBQuery query;
	DBResult result;
	query << "SELECT `type` FROM `bans` WHERE `type` = " << BAN_ACCOUNT << " AND `account` = " << account << " AND `time` > " << time(NULL) << " LIMIT 1;";
	if(db->storeQuery(query, result))
		return result.getNumRows() == 1;

	return false;
}

bool IOBan::getBanInformation(uint32_t account, uint32_t& bannedBy, uint32_t& banTime, int32_t& reason, int32_t& action, std::string& comment, bool& deletion)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	DBQuery query;
	DBResult result;
	query << "SELECT `banned_by`, `time`, `reason_id`, `action_id`, `comment` FROM `bans` WHERE `type` = " << BAN_ACCOUNT << " AND `account` = " << account << " AND `time` > " << time(NULL) << " LIMIT 1;";
	if(db->storeQuery(query, result))
	{
		bannedBy = result.getDataInt("banned_by");
		banTime = result.getDataInt("time");
		reason = result.getDataInt("reason_id");
		action = result.getDataInt("action_id");
		comment = result.getDataString("comment");
		deletion = false;
		return true;
	}

	query.str("");
	query << "SELECT `banned_by`, `time`, `reason_id`, `action_id`, `comment` FROM `bans` WHERE `type` = " << DELETE_ACCOUNT << " AND `account` = " << account << " LIMIT 1;";
	if(db->storeQuery(query, result))
	{
		bannedBy = result.getDataInt("banned_by");
		banTime = result.getDataInt("time");
		reason = result.getDataInt("reason_id");
		action = result.getDataInt("action_id");
		comment = result.getDataString("comment");
		deletion = true;
		return true;
	}
	return false;
}

int32_t IOBan::getNotationsCount(uint32_t account)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return 0;

	DBQuery query;
	DBResult result;
	query << "SELECT `type` FROM `bans` WHERE `type` = " << NOTATION_ACCOUNT << " AND `account` = " << account << ";";
	if(db->storeQuery(query, result))
		return result.getNumRows();

	return 0;
}

void IOBan::addIpBan(uint32_t ip, uint32_t mask, uint64_t time)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return;

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `ip`, `mask`, `time`) VALUES (" << BAN_IPADDRESS << ", " << ip << ", " << mask << ", " << time << ");";
	db->executeQuery(query);
}

void IOBan::addPlayerNamelock(uint32_t playerId, uint32_t time, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t bannedBy)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return;

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `player`, `time`, `reason_id`, `action_id`, `comment`, `banned_by`)"
		" VALUES (" << NAMELOCK_PLAYER << ", " << playerId << ", " << time << ", " << reasonId << ", " << actionId << ", '" << db->escapeString(comment) << "', " << bannedBy << ");";
	db->executeQuery(query);
}

void IOBan::addAccountNotation(uint32_t account, uint64_t time, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t bannedBy)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return;

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `account`, `time`, `reason_id`, `action_id`, `comment`, `banned_by`)"
		" VALUES (" << NOTATION_ACCOUNT << ", " << account << ", " << time << ", " << reasonId << ", " << actionId << ", '" << db->escapeString(comment) << "', " << bannedBy << ");";
	db->executeQuery(query);
}

void IOBan::addAccountDeletion(uint32_t account, uint64_t time, int32_t reasonId, int32_t actionId, std::string comment, uint32_t bannedBy)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return;

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `account`, `time`, `reason_id`, `action_id`, `comment`, `banned_by`)"
		" VALUES (" << DELETE_ACCOUNT << ", " << account << ", " << time << ", " << reasonId << ", " << actionId << ", '" << db->escapeString(comment) << "', " << bannedBy << ");";
	db->executeQuery(query);
}

void IOBan::addAccountBan(uint32_t account, uint64_t time, int32_t reasonId, int32_t actionId, std::string comment, uint32_t bannedBy)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return;

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `account`, `time`, `reason_id`, `action_id`, `comment`, `banned_by`)"
		" VALUES (" << BAN_ACCOUNT << ", " << account << ", " << time << ", " << reasonId << ", " << actionId << ", '" << db->escapeString(comment) << "', " << bannedBy << ");";
	db->executeQuery(query);
}

bool IOBan::removePlayerNamelock(uint32_t guid)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	DBQuery query;
	query << "DELETE FROM `bans` WHERE `type` = " << NAMELOCK_PLAYER << " AND `player` = " << guid << " LIMIT 1;";
	if(!db->executeQuery(query))
		return false;

	return true;
}

bool IOBan::removeAccountNotations(uint32_t account)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	DBQuery query;
	query << "DELETE FROM `bans` WHERE `type` = " << NOTATION_ACCOUNT << " AND `account` = " << account << ";";
	if(!db->executeQuery(query))
		return false;

	return true;
}

bool IOBan::removeIPBan(uint32_t ip)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	DBQuery query;
	query << "DELETE FROM `bans` WHERE `type` = " << BAN_IPADDRESS << " AND `ip` = " << ip << " LIMIT 1;";
	if(!db->executeQuery(query))
		return false;

	return true;
}

bool IOBan::removeAccountBan(uint32_t account)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	DBQuery query;
	query << "UPDATE `bans` SET `time` = " << time(NULL) << " WHERE `type` = " << BAN_ACCOUNT << " AND `account` = " << account << " AND `time` > " << time(NULL) << db->getUpdateQueryLimit() << ";";
	if(!db->executeQuery(query))
		return false;

	return true;
}

bool IOBan::removeAccountDeletion(uint32_t account)
{
	Database* db = Database::getInstance();
	if(!db->connect())
		return false;

	DBQuery query;
	query << "DELETE FROM `bans` WHERE `type` = " << DELETE_ACCOUNT << " AND `account` = " << account << " LIMIT 1;";
	if(!db->executeQuery(query))
		return false;

	return true;
}
