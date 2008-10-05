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

#include "ioban.h"
#include "iologindata.h"
#include "tools.h"
#include "database.h"

bool IOBan::isIpBanished(uint32_t ip, uint32_t mask /*= 0xFFFFFFFF*/)
{
	if(ip != 0)
	{
		OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
		Database* db = Database::getInstance();
		DBResult* result;

		DBQuery query;
		query << "SELECT `expires` FROM `bans` WHERE ((" << ip << " & " << mask << " & `param`) = (`value` & `param` & " << mask << ")) AND `type` = " << (BanType_t)BANTYPE_IP_BANISHMENT << " AND `active` = 1";
		if((result = db->storeQuery(query.str())))
		{
			uint64_t expires = result->getDataInt("expires");
			db->freeResult(result);

			if(expires != 0 && time(NULL) >= (time_t)expires)
			{
				removeIpBanishment(ip);
				return false;
			}

			return true;
		}
	}
	return false;
}

bool IOBan::isNamelocked(uint32_t guid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `bans` WHERE `value` = " << guid << " AND `type` = " << (BanType_t)BANTYPE_NAMELOCK << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		db->freeResult(result);
		return true;
	}
	return false;
}

bool IOBan::isNamelocked(std::string name)
{
	uint32_t _guid;
	if(!IOLoginData::getInstance()->getGuidByName(_guid, name))
		return false;

	return isNamelocked(_guid);
}

bool IOBan::isBanished(uint32_t account)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `expires` FROM `bans` WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_BANISHMENT << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		uint64_t expires = result->getDataInt("expires");
		db->freeResult(result);

		if(expires != 0 && time(NULL) >= (time_t)expires)
		{
			removeBanishment(account);
			return false;
		}

		return true;
	}
	return false;
}

bool IOBan::isDeleted(uint32_t account)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `bans` WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_DELETION << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		db->freeResult(result);
		return true;
	}
	return false;
}

bool IOBan::addIpBanishment(uint32_t ip, time_t banTime, std::string comment, uint32_t gamemaster)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	if(isIpBanished(ip))
		return false;

	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `param`, `expires`, `added`, `admin_id`, `comment`) VALUES (" << (BanType_t)BANTYPE_IP_BANISHMENT << ", " << ip << ", 4294967295, " << banTime << ", " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ")";
	db->executeQuery(query.str());
	return true;
}

bool IOBan::addNamelock(uint32_t playerId, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	if(isNamelocked(playerId))
		return false;

	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << (BanType_t)BANTYPE_NAMELOCK << ", " << playerId << ", '-1', " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ");";
	db->executeQuery(query.str());
	return true;
}

bool IOBan::addNamelock(std::string name, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	uint32_t _guid;
	if(!IOLoginData::getInstance()->getGuidByName(_guid, name))
		return false;

	return addNamelock(_guid, reasonId, actionId, comment, gamemaster);
}

bool IOBan::addBanishment(uint32_t account, time_t banTime, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	if(isBanished(account) || isDeleted(account))
		return false;

	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << (BanType_t)BANTYPE_BANISHMENT << ", " << account << ", " << banTime << ", " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ");";
	db->executeQuery(query.str());
	return true;
}

bool IOBan::addDeletion(uint32_t account, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	if(isDeleted(account))
		return false;

	if(!removeBanishment(account))
	{
		std::cout << "ERROR: IOBan::addDeletion - Couldn't remove banishments" << std::endl;
		return false;
	}

	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << (BanType_t)BANTYPE_DELETION << ", " << account << ", '-1', " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ");";
	db->executeQuery(query.str());
	return true;
}

void IOBan::addNotation(uint32_t account, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();

	DBQuery query;
	query << "INSERT INTO `bans` (`type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`) VALUES (" << (BanType_t)BANTYPE_NOTATION << ", " << account << ", '-1', " << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ");";
	db->executeQuery(query.str());
}

bool IOBan::removeIpBanishment(uint32_t ip)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << ip << " AND `type` = " << (BanType_t)BANTYPE_IP_BANISHMENT;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

bool IOBan::removeNamelock(uint32_t guid)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << guid << " AND `type` = " << (BanType_t)BANTYPE_NAMELOCK;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

bool IOBan::removeNamelock(std::string name)
{
	uint32_t _guid;
	if(!IOLoginData::getInstance()->getGuidByName(_guid, name))
		return false;

	return removeNamelock(_guid);
}

bool IOBan::removeBanishment(uint32_t account)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_BANISHMENT;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

bool IOBan::removeDeletion(uint32_t account)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_DELETION;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

void IOBan::removeNotations(uint32_t account)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_NOTATION;
	db->executeQuery(query.str());
}

uint32_t IOBan::getReason(uint32_t id, bool player /* = false */)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	uint32_t value = 0;
	DBQuery query;
	query << "SELECT `reason` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		value = result->getDataInt("reason");
		db->freeResult(result);
	}

	return value;
}

uint32_t IOBan::getAction(uint32_t id, bool player /* = false */)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	uint32_t value = 0;
	DBQuery query;
	query << "SELECT `action` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		value = result->getDataInt("action");
		db->freeResult(result);
	}

	return value;
}

uint64_t IOBan::getExpireTime(uint32_t id, bool player /* = false */)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	uint64_t value = 0;
	DBQuery query;
	query << "SELECT `expires` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		value = result->getDataInt("expires");
		db->freeResult(result);
	}

	return value;
}

uint64_t IOBan::getAddedTime(uint32_t id, bool player /* = false */)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	uint64_t value = 0;
	DBQuery query;
	query << "SELECT `added` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		value = result->getDataInt("added");
		db->freeResult(result);
	}

	return value;
}

std::string IOBan::getComment(uint32_t id, bool player /* = false */)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	std::string value = "";
	DBQuery query;
	query << "SELECT `comment` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		value = result->getDataString("comment");
		db->freeResult(result);
	}

	return value;
}

uint32_t IOBan::getAdminGUID(uint32_t id, bool player /* = false */)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	uint32_t value = 0;
	DBQuery query;
	query << "SELECT `admin_id` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		value = result->getDataInt("admin_id");
		db->freeResult(result);
	}

	return value;
}

uint32_t IOBan::getNotationsCount(uint32_t account)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(banLock, "");
	Database* db = Database::getInstance();
	DBResult* result;

	uint32_t count = 0;
	DBQuery query;
	query << "SELECT COUNT(`id`) AS `count` FROM `bans` WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_NOTATION << " AND `active` = 1";
	if((result = db->storeQuery(query.str())))
	{
		count = result->getDataInt("count");
		db->freeResult(result);
	}

	return count;
}

bool IOBan::getBanishmentData(uint32_t account, Ban& ban)
{
	Database* db = Database::getInstance();
	DBQuery query;
	DBResult* result;

	uint32_t currentTime = time(NULL);
	query <<
		"SELECT "
			"`id`, "
			"`type`, "
			"`param`, "
			"`expires`, "
			"`added`, "
			"`admin_id`, "
			"`comment`, "
			"`reason`, "
			"`action` "
		"FROM "
			"`bans` "
		"WHERE "
			"`value` = " << account << " AND "
			"`active` = 1 AND " <<
			"(`type` = 3 OR `type` = 5) AND " <<
			"(`expires` > " << currentTime << " OR `expires` <= 0)";

	if((result = db->storeQuery(query.str())))
	{
		ban.value = account;
		ban.id = result->getDataInt("id");
		ban.type = (BanType_t)result->getDataInt("type");
		ban.param = result->getDataString("param");
		ban.expires = (uint32_t)result->getDataLong("expires");
		ban.added = (uint32_t)result->getDataLong("added");
		ban.adminid = result->getDataInt("admin_id");
		ban.comment = result->getDataString("comment");
		ban.reason = result->getDataInt("reason");
		ban.action = result->getDataInt("action");

		db->freeResult(result);
		return true;
	}

	return false;
}

std::vector<Ban> IOBan::bansManager(BanType_t type) const
{
	Database* db = Database::getInstance();
	DBQuery query;
	DBResult* result;

	uint32_t currentTime = time(NULL);
	query <<
		"SELECT "
			"`id`, "
			"`value`, "
			"`param`, "
			"`expires`, "
			"`added`, "
			"`admin_id`, "
			"`comment`, "
			"`reason`, "
			"`action` "
		"FROM "
			"`bans` "
		"WHERE "
			"`type` = " << type << " AND "
			"`active` = 1 AND " <<
			"(`expires` > " << currentTime << " OR `expires` <= 0)";

	std::vector<Ban> vec;
	if((result = db->storeQuery(query.str())))
	{
		Ban tmpBan;
		do {
			tmpBan.type = type;
			tmpBan.id = result->getDataInt("id");
			tmpBan.value = result->getDataString("value");
			tmpBan.param = result->getDataString("param");
			tmpBan.expires = (uint32_t)result->getDataLong("expires");
			tmpBan.added = (uint32_t)result->getDataLong("added");
			tmpBan.adminid = result->getDataInt("admin_id");
			tmpBan.comment = result->getDataString("comment");
			tmpBan.reason = result->getDataInt("reason");
			tmpBan.action = result->getDataInt("action");
			vec.push_back(tmpBan);
		}
		while(result->next());

		db->freeResult(result);
	}

	return vec;
}

bool IOBan::clearTemporials()
{
	Database* db = Database::getInstance();
	return db->executeQuery("UPDATE `bans` SET `active` = 0 WHERE `expires` = 0 AND `active` = 1;");
}
