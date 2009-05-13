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
#include "otpch.h"
#include "ioban.h"

#include "tools.h"
#include "database.h"
#include "iologindata.h"

bool IOBan::isIpBanished(uint32_t ip, uint32_t mask/* = 0xFFFFFFFF*/) const
{
	if(ip == 0)
		return false;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `value`, `param`, `expires` FROM `bans` WHERE `type` = " << BANTYPE_IP_BANISHMENT << " AND `active` = 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	bool b_result = false;
	do
	{
		uint32_t value = result->getDataInt("value"), param = result->getDataInt("param");
		if((ip & mask & param) == (value & param & mask))
		{
			int32_t expires = result->getDataLong("expires");
			if(expires > 0 && expires <= (int32_t)time(NULL))
				removeIpBanishment(value, param);
			else if(!b_result)
				b_result = true;
		}
	}
	while(result->next());
	result->free();
	return b_result;
}

bool IOBan::isNamelocked(uint32_t guid) const
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `bans` WHERE `value` = " << guid << " AND `type` = " << BANTYPE_NAMELOCK << " AND `active` = 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	result->free();
	return true;
}

bool IOBan::isNamelocked(std::string name) const
{
	uint32_t _guid;
	if(!IOLoginData::getInstance()->getGuidByName(_guid, name))
		return false;

	return isNamelocked(_guid);
}

bool IOBan::isBanished(uint32_t account) const
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `expires` FROM `bans` WHERE `value` = " << account << " AND `type` = " << BANTYPE_BANISHMENT << " AND `active` = 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	const int32_t expires = result->getDataInt("expires");
	result->free();
	if(expires <= 0 || expires > (int32_t)time(NULL))
		return true;

	removeBanishment(account);
	return false;
}

bool IOBan::isDeleted(uint32_t account) const
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `bans` WHERE `value` = " << account << " AND `type` = " << BANTYPE_DELETION << " AND `active` = 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	result->free();
	return true;
}

bool IOBan::addIpBanishment(uint32_t ip, time_t banTime, std::string comment,
	uint32_t gamemaster, uint32_t mask/* = 0xFFFFFFFF*/) const
{
	if(isIpBanished(ip))
		return false;

	Database* db = Database::getInstance();
	DBQuery query;

	query << "INSERT INTO `bans` (`id`, `type`, `value`, `param`, `expires`, `added`, `admin_id`, `comment`) ";
	query << "VALUES (NULL, " << BANTYPE_IP_BANISHMENT << ", " << ip << ", " << mask << ", " << banTime << ", ";
	query << time(NULL) << ", " << gamemaster << ", " << db->escapeString(comment.c_str()) << ");";
	return db->executeQuery(query.str());
}

bool IOBan::addNamelock(uint32_t playerId, uint32_t reasonId, ViolationAction_t actionId, std::string comment,
	uint32_t gamemaster, std::string statement/* = ""*/) const
{
	if(isNamelocked(playerId))
		return false;

	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `bans` (`id`, `type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement`) ";
	query << "VALUES (NULL, " << BANTYPE_NAMELOCK << ", " << playerId << ", '-1', " << time(NULL) << ", " << gamemaster << ", ";
	query << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << (int32_t)actionId << ", " << db->escapeString(statement.c_str()) << ");";
	return db->executeQuery(query.str());
}

bool IOBan::addNamelock(std::string name, uint32_t reasonId, ViolationAction_t actionId, std::string comment,
	uint32_t gamemaster, std::string statement/* = ""*/) const
{
	uint32_t _guid;
	return IOLoginData::getInstance()->getGuidByName(_guid, name) &&
		addNamelock(_guid, reasonId, actionId, comment, gamemaster, statement);
}

bool IOBan::addBanishment(uint32_t account, time_t banTime, uint32_t reasonId, ViolationAction_t actionId,
	std::string comment, uint32_t gamemaster, std::string statement/* = ""*/) const
{
	if(isBanished(account) || isDeleted(account))
		return false;

	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `bans` (`id`, `type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement`) ";
	query << "VALUES (NULL, " << BANTYPE_BANISHMENT << ", " << account << ", " << banTime << ", " << time(NULL) << ", " << gamemaster << ", ";
	query << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << (int32_t)actionId << ", " << db->escapeString(statement.c_str()) << ");";
	return db->executeQuery(query.str());
}

bool IOBan::addDeletion(uint32_t account, uint32_t reasonId, ViolationAction_t actionId, std::string comment,
	uint32_t gamemaster, std::string statement/* = ""*/) const
{
	if(isDeleted(account))
		return false;

	if(!removeBanishment(account))
	{
		std::cout << "[Error - IOBan::addDeletion] Couldn't remove banishment(s)" << std::endl;
		return false;
	}

	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `bans` (`id`, `type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement`) ";
	query << "VALUES (NULL, " << BANTYPE_DELETION << ", " << account << ", '-1', " << time(NULL) << ", " << gamemaster << ", ";
	query << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << (int32_t)actionId << ", " << db->escapeString(statement.c_str()) << ");";
	return db->executeQuery(query.str());
}

bool IOBan::addNotation(uint32_t account, uint32_t reasonId, ViolationAction_t actionId, std::string comment,
	uint32_t gamemaster, std::string statement/* = ""*/) const
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `bans` (`id`, `type`, `value`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement`) ";
	query << "VALUES (NULL, " << BANTYPE_NOTATION << ", " << account << ", '-1', " << time(NULL) << ", " << gamemaster << ", ";
	query << db->escapeString(comment.c_str()) << ", " << reasonId << ", " << (int32_t)actionId << ", " << db->escapeString(statement.c_str()) << ");";
	return db->executeQuery(query.str());
}

bool IOBan::removeIpBanishment(uint32_t ip, uint32_t mask/* = 0xFFFFFFFF*/) const
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << ip << " AND `param` = " << mask << " AND `type` = " << BANTYPE_IP_BANISHMENT;
	return db->executeQuery(query.str());
}

bool IOBan::removeNamelock(uint32_t guid) const
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << guid << " AND `type` = " << BANTYPE_NAMELOCK;
	return db->executeQuery(query.str());
}

bool IOBan::removeNamelock(std::string name) const
{
	uint32_t _guid;
	if(!IOLoginData::getInstance()->getGuidByName(_guid, name))
		return false;

	return removeNamelock(_guid);
}

bool IOBan::removeBanishment(uint32_t account) const
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << BANTYPE_BANISHMENT;
	return db->executeQuery(query.str());
}

bool IOBan::removeDeletion(uint32_t account) const
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << BANTYPE_DELETION;
	return db->executeQuery(query.str());
}

bool IOBan::removeNotations(uint32_t account) const
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `value` = " << account << " AND `type` = " << BANTYPE_NOTATION;
	return db->executeQuery(query.str());
}

uint32_t IOBan::getNotationsCount(uint32_t account) const
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT COUNT(`id`) AS `count` FROM `bans` WHERE `value` = " << account << " AND `type` = ";
	query << BANTYPE_NOTATION << " AND `active` = 1";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t count = result->getDataInt("count");
	result->free();
	return count;
}

bool IOBan::getData(uint32_t value, Ban& ban) const
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `type`, `param`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement` ";
	query << "FROM `bans` WHERE `value` = " << value << " AND `active` = 1 AND (`expires` > " << time(NULL) << " OR `expires` <= 0)";
	if(!(result = db->storeQuery(query.str())))
		return false;

	ban.value = value;
	ban.id = result->getDataInt("id");
	ban.type = (BanType_t)result->getDataInt("type");
	ban.param = result->getDataString("param");
	ban.expires = result->getDataLong("expires");
	ban.added = (uint32_t)result->getDataLong("added");
	ban.adminid = result->getDataInt("admin_id");
	ban.comment = result->getDataString("comment");
	ban.reason = result->getDataInt("reason");
	ban.action = (ViolationAction_t)result->getDataInt("action");
	ban.statement = result->getDataString("statement");

	result->free();
	return true;
}

BansVec IOBan::getList(BanType_t type, uint32_t value/* = 0*/)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `value`, `param`, `expires`, `added`, `admin_id`, `comment`, `reason`, `action`, `statement` FROM `bans` WHERE ";
	if(value > 0)
		query << "`value` = " << value << " AND ";

	query << "`type` = " << type << " AND `active` = 1 AND (`expires` > " << time(NULL) << " OR `expires` <= 0)";
	BansVec data;
	if((result = db->storeQuery(query.str())))
	{
		Ban tmp;
		do
		{
			tmp.type = type;
			tmp.id = result->getDataInt("id");
			tmp.value = result->getDataString("value");
			tmp.param = result->getDataString("param");
			tmp.expires = result->getDataLong("expires");
			tmp.added = (uint32_t)result->getDataLong("added");
			tmp.adminid = result->getDataInt("admin_id");
			tmp.comment = result->getDataString("comment");
			tmp.reason = result->getDataInt("reason");
			tmp.action = (ViolationAction_t)result->getDataInt("action");
			tmp.statement = result->getDataString("statement");
			data.push_back(tmp);
		}
		while(result->next());
		result->free();
	}

	return data;
}

bool IOBan::clearTemporials() const
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `bans` SET `active` = 0 WHERE `expires` <= " << time(NULL) << " AND `expires` >= 0 AND `active` = 1;";
	return db->executeQuery(query.str());
}
