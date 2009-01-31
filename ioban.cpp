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
	if(ip == 0)
		return false;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `expires` FROM `bans` WHERE `type` = " << (BanType_t)BANTYPE_IP_BANISHMENT << " AND ((" << ip << " & " << mask << " & `param`) = (`value` & `param` & " << mask << ")) AND `active` = 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint64_t expires = result->getDataLong("expires");
	db->freeResult(result);
	if(expires == 0 || (time_t)expires > time(NULL))
		return true;

	removeIpBanishment(ip);
	return false;
}

bool IOBan::isNamelocked(uint32_t guid)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `bans` WHERE `value` = " << guid << " AND `type` = " << (BanType_t)BANTYPE_NAMELOCK << " AND `active` = 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
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
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `expires` FROM `bans` WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_BANISHMENT << " AND `active` = 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint64_t expires = result->getDataInt("expires");
	db->freeResult(result);
	if(expires == 0 || (time_t)expires > time(NULL))
		return true;

	removeBanishment(account);
	return false;
}

bool IOBan::isDeleted(uint32_t account)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `bans` WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_DELETION << " AND `active` = 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
}

bool IOBan::addIpBanishment(uint32_t ip, time_t banTime, std::string comment, uint32_t gamemaster, std::string statement/* = ""*/)
{
	if(isIpBanished(ip))
		return false;

	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `bans` (`id`, `type`, `value`, `param`, `expires`, `added`, `admin_id`, `comment`, `statement`) ";
	query << "VALUES (NULL, " << (BanType_t)BANTYPE_IP_BANISHMENT << ", " << ip << ", 4294967295, " << banTime << ", " << time(NULL) << ", ";
	query << gamemaster << ", " << db->escapeString(comment.c_str()) << ", " << db->escapeString(statement.c_str()) << ");";
	return db->executeQuery(query.str());
}

bool IOBan::addNamelock(uint32_t playerId, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster, std::string statement/* = ""*/)
{
	if(isNamelocked(playerId))
		return false;

	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `bans` (`id`, `type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement`) ";
	query << "VALUES (NULL, " << (BanType_t)BANTYPE_NAMELOCK << ", " << playerId << ", '-1', " << time(NULL) << ", " << gamemaster << ", ";
	query << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ", " << db->escapeString(statement.c_str()) << ");";
	return db->executeQuery(query.str());
}

bool IOBan::addNamelock(std::string name, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster, std::string statement/* = ""*/)
{
	uint32_t _guid;
	if(!IOLoginData::getInstance()->getGuidByName(_guid, name))
		return false;

	return addNamelock(_guid, reasonId, actionId, comment, gamemaster, statement);
}

bool IOBan::addBanishment(uint32_t account, time_t banTime, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster, std::string statement/* = ""*/)
{
	if(isBanished(account) || isDeleted(account))
		return false;

	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `bans` (`id`, `type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement`) ";
	query << "VALUES (NULL, " << (BanType_t)BANTYPE_BANISHMENT << ", " << account << ", " << banTime << ", " << time(NULL) << ", " << gamemaster << ", ";
	query << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ", " << db->escapeString(statement.c_str()) << ");";
	return db->executeQuery(query.str());
}

bool IOBan::addDeletion(uint32_t account, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster, std::string statement/* = ""*/)
{
	if(isDeleted(account))
		return false;

	if(!removeBanishment(account))
	{
		std::cout << "[Error - IOBan::addDeletion]: Couldn't remove banishment(s)" << std::endl;
		return false;
	}

	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `bans` (`id`, `type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement`) ";
	query << "VALUES (NULL, " << (BanType_t)BANTYPE_DELETION << ", " << account << ", '-1', " << time(NULL) << ", " << gamemaster << ", ";
	query << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ", " << db->escapeString(statement.c_str()) << ");";
	return db->executeQuery(query.str());
}

void IOBan::addNotation(uint32_t account, uint32_t reasonId, uint32_t actionId, std::string comment, uint32_t gamemaster, std::string statement/* = ""*/)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `bans` (`id`, `type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement`) ";
	query << "VALUES (NULL, " << (BanType_t)BANTYPE_NOTATION << ", " << account << ", '-1', " << time(NULL) << ", " << gamemaster << ", ";
	query << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << actionId << ", " << db->escapeString(statement.c_str()) << ");";
	db->executeQuery(query.str());
}

bool IOBan::removeIpBanishment(uint32_t ip)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << ip << " AND `type` = " << (BanType_t)BANTYPE_IP_BANISHMENT;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

bool IOBan::removeNamelock(uint32_t guid)
{
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
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_BANISHMENT;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

bool IOBan::removeDeletion(uint32_t account)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_DELETION;
	if(db->executeQuery(query.str()))
		return true;

	return false;
}

void IOBan::removeNotations(uint32_t account)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_NOTATION;
	db->executeQuery(query.str());
}

uint32_t IOBan::getReason(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `reason` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t value = result->getDataInt("reason");
	db->freeResult(result);
	return value;
}

uint32_t IOBan::getAction(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `action` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t value = result->getDataInt("action");
	db->freeResult(result);
	return value;
}

uint64_t IOBan::getExpireTime(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `expires` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint64_t value = result->getDataInt("expires");
	db->freeResult(result);
	return value;
}

uint64_t IOBan::getAddedTime(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `added` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint64_t value = result->getDataInt("added");
	db->freeResult(result);
	return value;
}

std::string IOBan::getComment(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `comment` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return "";

	const std::string value = result->getDataString("comment");
	db->freeResult(result);
	return value;
}

uint32_t IOBan::getAdminGUID(uint32_t id, bool player /* = false */)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `admin_id` FROM `bans` WHERE `value` = " << id << " AND `type` = " << (player ? (BanType_t)BANTYPE_NAMELOCK : (BanType_t)BANTYPE_BANISHMENT)  << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t value = result->getDataInt("admin_id");
	db->freeResult(result);
	return value;
}

uint32_t IOBan::getNotationsCount(uint32_t account)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT COUNT(`id`) AS `count` FROM `bans` WHERE `value` = " << account << " AND `type` = " << (BanType_t)BANTYPE_NOTATION << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t count = result->getDataInt("count");
	db->freeResult(result);
	return count;
}

bool IOBan::getData(uint32_t value, Ban& ban)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `type`, `param`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action` FROM `bans` WHERE `value` = " << value << " AND `active` = 1 AND (`expires` > " << time(NULL) << " OR `expires` <= 0)";
	if(!(result = db->storeQuery(query.str())))
		return false;

	ban.value = value;
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

BansVec IOBan::getList(BanType_t type, uint32_t value/* = 0*/)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `value`, `param`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action` FROM `bans` WHERE ";
	if(value > 0)
		query << "`value` = " << value << " AND ";

	query << "`type` = " << type << " AND `active` = 1 AND (`expires` > " << time(NULL) << " OR `expires` <= 0)";
	BansVec data;
	if((result = db->storeQuery(query.str())))
	{
		Ban tmp;
		do {
			tmp.type = type;
			tmp.id = result->getDataInt("id");
			tmp.value = result->getDataString("value");
			tmp.param = result->getDataString("param");
			tmp.expires = (uint32_t)result->getDataLong("expires");
			tmp.added = (uint32_t)result->getDataLong("added");
			tmp.adminid = result->getDataInt("admin_id");
			tmp.comment = result->getDataString("comment");
			tmp.reason = result->getDataInt("reason");
			tmp.action = result->getDataInt("action");
			data.push_back(tmp);
		}
		while(result->next());
		db->freeResult(result);
	}

	return data;
}

bool IOBan::clearTemporials()
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `expires` <= " << time(NULL) << " AND `expires` >= 0 AND `active` = 1;";
	return db->executeQuery(query.str());
}
