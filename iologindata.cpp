//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the LoginData loading/saving
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
#include "otpch.h"

#include "iologindata.h"
#include <algorithm>
#include <functional>
#include "item.h"
#include "configmanager.h"
#include "tools.h"
#include "town.h"
#include "definitions.h"
#include "game.h"
#include "vocation.h"
#include "house.h"
#ifdef __LOGIN_SERVER__
#include "gameservers.h"
#endif
#include <iostream>
#include <iomanip>

extern ConfigManager g_config;
extern Vocations g_vocations;
extern Game g_game;

#ifndef __GNUC__
#pragma warning( disable : 4005)
#pragma warning( disable : 4996)
#endif

Account IOLoginData::loadAccount(uint32_t accId, bool preLoad/* = false*/)
{
	Account acc;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `name`, `password`, `premdays`, `lastday`, `key`, `warnings` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return acc;

	acc.number = result->getDataInt("id");
	acc.name = result->getDataString("name");
	acc.password = result->getDataString("password");
	acc.premiumDays = result->getDataInt("premdays");
	acc.lastDay = result->getDataInt("lastday");
	acc.recoveryKey = result->getDataString("key");
	acc.warnings = result->getDataInt("warnings");

	query.str("");
	db->freeResult(result);
	if(preLoad)
		return acc;

#ifndef __LOGIN_SERVER__
	query << "SELECT `name` FROM `players` WHERE `account_id` = " << accId << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `deleted` = 0;";
#else
	query << "SELECT `name`, `world_id` FROM `players` WHERE `account_id` = " << accId << " AND `deleted` = 0;";
#endif
	if(!(result = db->storeQuery(query.str())))
		return acc;

	do
	{
		std::string ss = result->getDataString("name");
#ifndef __LOGIN_SERVER__
		acc.charList.push_back(ss.c_str());
#else
		if(GameServer* server = GameServers::getInstance()->getServerById(result->getDataInt("world_id")))
			acc.charList[ss] = server;
		else
			std::cout << "[Warning - IOLoginData::loadAccount] Invalid server for player '" << ss << "'." << std::endl;
#endif
	}
	while(result->next());
	db->freeResult(result);

#ifndef __LOGIN_SERVER__
	acc.charList.sort();
#endif
	return acc;
}

bool IOLoginData::saveAccount(Account acc)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `accounts` SET `premdays` = " << acc.premiumDays << ", `warnings` = " << acc.warnings << ", `lastday` = " << acc.lastDay << " WHERE `id` = " << acc.number;
	return db->executeQuery(query.str());
}

bool IOLoginData::getAccountId(const std::string& name, uint32_t& number)
{
	if(!name.length())
		return false;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `accounts` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name);
	if(!(result = db->storeQuery(query.str())))
		return false;

	number = result->getDataInt("id");
	db->freeResult(result);
	return true;
}

bool IOLoginData::getAccountName(uint32_t number, std::string& name)
{
	if(number < 2)
	{
		name = number ? "1" : "";
		return true;
	}

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `name` FROM `accounts` WHERE `id` = " << number;
	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	db->freeResult(result);
	return true;
}

bool IOLoginData::hasFlag(uint32_t accId, PlayerFlags value)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t group = result->getDataInt("group_id");
	db->freeResult(result);
	return internalHasFlag(group, value);
}

bool IOLoginData::hasCustomFlag(uint32_t accId, PlayerCustomFlags value)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t group = result->getDataInt("group_id");
	db->freeResult(result);
	return internalHasCustomFlag(group, value);
}

bool IOLoginData::accountExists(uint32_t accId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
}

bool IOLoginData::accountNameExists(const std::string& name)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `accounts` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name);
	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
}

bool IOLoginData::getPassword(uint32_t accId, const std::string& name, std::string& password)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `password` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	std::string accountPassword = result->getDataString("password");
	db->freeResult(result);
	query.str("");
	query << "SELECT `name` FROM `players` WHERE `account_id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	do
	{
		if(result->getDataString("name") == name)
		{
			password = accountPassword;
			db->freeResult(result);
			return true;
		}
	}
	while(result->next());

	db->freeResult(result);
	return false;
}

bool IOLoginData::setNewPassword(uint32_t accountId, std::string newPassword)
{
	Database* db = Database::getInstance();
	if(g_config.getNumber(ConfigManager::PASSWORDTYPE) == PASSWORD_TYPE_MD5)
		newPassword = transformToMD5(newPassword);
	else if(g_config.getNumber(ConfigManager::PASSWORDTYPE) == PASSWORD_TYPE_SHA1)
		newPassword = transformToSHA1(newPassword);

	DBQuery query;
	query << "UPDATE `accounts` SET `password` = " << db->escapeString(newPassword) << " WHERE `id` = " << accountId;
	return db->executeQuery(query.str());
}

bool IOLoginData::validRecoveryKey(uint32_t accountId, const std::string recoveryKey)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `accounts` WHERE `key` " << db->getStringComparisonOperator() << " " << db->escapeString(recoveryKey) << " AND `id` = " << accountId;
	if((result = db->storeQuery(query.str())))
	{
		db->freeResult(result);
		return true;
	}
	return false;
}

bool IOLoginData::setRecoveryKey(uint32_t accountId, std::string recoveryKey)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `accounts` SET `key` = " << db->escapeString(recoveryKey) << " WHERE `id` = " << accountId;
	return db->executeQuery(query.str());
}

bool IOLoginData::createAccount(std::string name, std::string password)
{
	Database* db = Database::getInstance();
	if(g_config.getNumber(ConfigManager::PASSWORDTYPE) == PASSWORD_TYPE_MD5)
		password = transformToMD5(password);
	else if(g_config.getNumber(ConfigManager::PASSWORDTYPE) == PASSWORD_TYPE_SHA1)
		password = transformToSHA1(password);

	DBQuery query;
	query << "INSERT INTO `accounts` (`id`, `name`, `password`) VALUES (NULL, " << db->escapeString(name) << ", " << db->escapeString(password) << ");";
	return db->executeQuery(query.str());
}

void IOLoginData::removePremium(Account account)
{
	uint64_t timeNow = time(NULL);
	if(account.premiumDays > 0 && account.premiumDays < 65535)
	{
		uint32_t days = (uint32_t)std::ceil((timeNow - account.lastDay) / 86400);
		if(days > 0)
		{
			if(account.premiumDays < days)
				account.premiumDays = 0;
			else
				account.premiumDays -= days;

			account.lastDay = timeNow;
		}
	}
	else
		account.lastDay = timeNow;

	if(!saveAccount(account))
		std::cout << "> ERROR: Failed to save account: " << account.name << "!" << std::endl;
}

const PlayerGroup* IOLoginData::getPlayerGroup(uint32_t groupId)
{
	PlayerGroupMap::const_iterator it = playerGroupMap.find(groupId);
	if(it != playerGroupMap.end())
		return it->second;
	else
	{
		Database* db = Database::getInstance();
		DBResult* result;

		DBQuery query;
		query << "SELECT `name`, `flags`, `customflags`, `access`, `violationaccess`, `maxdepotitems`, `maxviplist`, `outfit` FROM `groups` WHERE `id` = " << groupId;
		if(!(result = db->storeQuery(query.str())))
			return NULL;

		PlayerGroup* group = new PlayerGroup;
		group->m_name = result->getDataString("name");
		group->m_flags = result->getDataLong("flags");
		group->m_customflags = result->getDataLong("customflags");
		group->m_access = result->getDataInt("access");
		group->m_violationaccess = result->getDataInt("violationaccess");
		group->m_maxdepotitems = result->getDataInt("maxdepotitems");
		group->m_maxviplist = result->getDataInt("maxviplist");
		group->m_outfit = result->getDataInt("outfit");

		playerGroupMap[groupId] = group;
		db->freeResult(result);
		return group;
	}

	return NULL;
}

const PlayerGroup* IOLoginData::getPlayerGroupByAccount(uint32_t accId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `accounts` WHERE `id` = " << accId;
	if((result = db->storeQuery(query.str())))
	{
		const uint32_t groupId = result->getDataInt("group_id");
		db->freeResult(result);
		return getPlayerGroup(groupId);
	}

	return NULL;
}

bool IOLoginData::internalHasFlag(uint32_t groupId, PlayerFlags value)
{
	PlayerGroupMap::const_iterator it = playerGroupMap.find(groupId);
	if(it != playerGroupMap.end())
		return (0 != (it->second->m_flags & ((uint64_t)1 << value)));

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `flags` FROM `groups` WHERE `id` = " << groupId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint64_t flags = result->getDataLong("flags");
	db->freeResult(result);
	return (0 != (flags & ((uint64_t)1 << value)));
}

bool IOLoginData::internalHasCustomFlag(uint32_t groupId, PlayerCustomFlags value)
{
	PlayerGroupMap::const_iterator it = playerGroupMap.find(groupId);
	if(it != playerGroupMap.end())
		return (0 != (it->second->m_customflags & ((uint64_t)1 << value)));

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `customflags` FROM `groups` WHERE `id` = " << groupId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint64_t flags = result->getDataLong("customflags");
	db->freeResult(result);
	return (0 != (flags & ((uint64_t)1 << value)));
}

bool IOLoginData::loadPlayer(Player* player, const std::string& name, bool preLoad /*= false*/)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `account_id`, `group_id`, `sex`, `vocation`, `experience`, `level`, `maglevel`, `health`, `healthmax`, `blessings`, `mana`, `manamax`, `manaspent`, `soul`, `lookbody`, `lookfeet`, `lookhead`, `looklegs`, `looktype`, `lookaddons`, `posx`, `posy`, `posz`, `cap`, `lastlogin`, `lastlogout`, `lastip`, `conditions`, `redskulltime`, `redskull`, `guildnick`, `rank_id`, `town_id`, `balance`, `stamina`, `loss_experience`, `loss_mana`, `loss_skills`, `loss_items`, `marriage`, `promotion` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint32_t accId = result->getDataInt("account_id");
	if(accId < 1)
	{
		db->freeResult(result);
		return false;
	}

	Account acc = loadAccount(accId, true);
	player->accountId = accId;
	player->account = acc.name;

	player->setGUID(result->getDataInt("id"));
	player->setGroupId(result->getDataInt("group_id"));
	player->premiumDays = acc.premiumDays;

	if(preLoad)
	{
		//only loading basic info
		db->freeResult(result);
		return true;
	}

	player->setSex((PlayerSex_t)result->getDataInt("sex"));
	player->level = std::max((uint32_t)1, (uint32_t)result->getDataInt("level"));

	uint64_t currExpCount = Player::getExpForLevel(player->level);
	uint64_t nextExpCount = Player::getExpForLevel(player->level + 1);
	uint64_t experience = (uint64_t)result->getDataLong("experience");
	if(experience < currExpCount || experience > nextExpCount)
		experience = currExpCount;

	player->experience = experience;
	if(currExpCount < nextExpCount)
		player->levelPercent = Player::getPercentLevel(player->experience - currExpCount, nextExpCount - currExpCount);
	else
		player->levelPercent = 0;

	player->soul = result->getDataInt("soul");
	player->capacity = result->getDataInt("cap");
	player->setStamina(result->getDataLong("stamina"));
	player->balance = result->getDataLong("balance");
	player->marriage = result->getDataInt("marriage");
	if(player->isPremium() || !g_config.getBool(ConfigManager::BLESSING_ONLY_PREMIUM))
		player->blessings = result->getDataInt("blessings");

	uint64_t conditionsSize = 0;
	const char* conditions = result->getDataStream("conditions", conditionsSize);
	PropStream propStream;
	propStream.init(conditions, conditionsSize);

	Condition* condition;
	while((condition = Condition::createCondition(propStream)))
	{
		if(condition->unserialize(propStream))
			player->storedConditionList.push_back(condition);
		else
			delete condition;
	}

	player->vocation_id = result->getDataInt("vocation");
	player->setPromotionLevel(result->getDataInt("promotion"));

	player->health = result->getDataInt("health");
	player->healthMax = result->getDataInt("healthmax");
	player->mana = result->getDataInt("mana");
	player->manaMax = result->getDataInt("manamax");

	player->magLevel = result->getDataInt("maglevel");
	uint64_t nextManaCount = player->vocation->getReqMana(player->magLevel + 1);
	uint64_t manaSpent = result->getDataLong("manaspent");
	if(manaSpent > nextManaCount)
		manaSpent = 0;

	player->manaSpent = manaSpent;
	player->magLevelPercent = Player::getPercentLevel(player->manaSpent, nextManaCount);

	player->defaultOutfit.lookHead = result->getDataInt("lookhead");
	player->defaultOutfit.lookBody = result->getDataInt("lookbody");
	player->defaultOutfit.lookLegs = result->getDataInt("looklegs");
	player->defaultOutfit.lookFeet = result->getDataInt("lookfeet");
	player->defaultOutfit.lookAddons = result->getDataInt("lookaddons");
	if(!player->groupOutfit)
		player->defaultOutfit.lookType = result->getDataInt("looktype");
	else
		player->defaultOutfit.lookType = player->groupOutfit;

	player->currentOutfit = player->defaultOutfit;
	if(g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
	{
		int32_t redSkullSeconds = result->getDataInt("redskulltime") - time(NULL);
		if(redSkullSeconds > 0)
		{
			//ensure that we round up the number of ticks
			player->redSkullTicks = (redSkullSeconds + 2) * 1000;
			if(result->getDataInt("redskull") == 1)
				player->skull = SKULL_RED;
		}
	}

	player->setLossPercent(LOSS_EXPERIENCE, result->getDataInt("loss_experience"));
	player->setLossPercent(LOSS_MANASPENT, result->getDataInt("loss_mana"));
	player->setLossPercent(LOSS_SKILLTRIES, result->getDataInt("loss_skills"));
	player->setLossPercent(LOSS_ITEMS, result->getDataInt("loss_items"));

	player->loginPosition.x = result->getDataInt("posx");
	player->loginPosition.y = result->getDataInt("posy");
	player->loginPosition.z = result->getDataInt("posz");

	player->lastLoginSaved = result->getDataLong("lastlogin");
	player->lastLogout = result->getDataLong("lastlogout");

	player->town = result->getDataInt("town_id");
	Town* town = Towns::getInstance().getTown(player->town);
	if(town)
		player->masterPos = town->getTemplePosition();

	Position loginPos = player->loginPosition;
	if(loginPos.x == 0 && loginPos.y == 0 && loginPos.z == 0)
		player->loginPosition = player->masterPos;

	const uint32_t rankId = result->getDataInt("rank_id");
	const std::string nick = result->getDataString("guildnick");
	db->freeResult(result);
	if(rankId > 0)
	{
		query.str("");
		query << "SELECT `guild_ranks`.`name` AS `rank`, `guild_ranks`.`guild_id` AS `guildid`, `guild_ranks`.`level` AS `level`, `guilds`.`name` AS `guildname` FROM `guild_ranks`, `guilds` WHERE `guild_ranks`.`id` = " << rankId << " AND `guild_ranks`.`guild_id` = `guilds`.`id`";
		if((result = db->storeQuery(query.str())))
		{
			player->guildName = result->getDataString("guildname");
			player->guildLevel = result->getDataInt("level");
			player->guildId = result->getDataInt("guildid");
			player->guildRank = result->getDataString("rank");
			player->guildRankId = rankId;
			player->guildNick = nick;
		}

		db->freeResult(result);
	}
	else if(g_config.getBool(ConfigManager::INGAME_GUILD_MANAGEMENT))
	{
		query.str("");
		query << "SELECT `guild_id` FROM `guild_invites` WHERE `player_id` = " << player->getGUID();
		if((result = db->storeQuery(query.str())))
		{
			do
				player->invitedToGuildsList.push_back((uint32_t)result->getDataInt("guild_id"));
			while(result->next());
			db->freeResult(result);
		}
	}

	query.str("");
	query << "SELECT `password` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	player->password = result->getDataString("password");
	db->freeResult(result);

	// we need to find out our skills
	// so we query the skill table
	query.str("");
	query << "SELECT `skillid`, `value`, `count` FROM `player_skills` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str())))
	{
		//now iterate over the skills
		do
		{
			int32_t skillid = result->getDataInt("skillid");
			if(skillid >= SKILL_FIRST && skillid <= SKILL_LAST)
			{
				uint32_t skillLevel = result->getDataInt("value");
				uint64_t skillCount = result->getDataLong("count");

				uint64_t nextSkillCount = player->vocation->getReqSkillTries(skillid, skillLevel + 1);
				if(skillCount > nextSkillCount)
					skillCount = 0;

				player->skills[skillid][SKILL_LEVEL] = skillLevel;
				player->skills[skillid][SKILL_TRIES] = skillCount;
				player->skills[skillid][SKILL_PERCENT] = Player::getPercentLevel(skillCount, nextSkillCount);
			}
		}
		while(result->next());

		db->freeResult(result);
	}

	query.str("");
 	query << "SELECT `player_id`, `name` FROM `player_spells` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str())))
	{
		do
		{
			std::string spellName = result->getDataString("name");
			player->learnedInstantSpellList.push_back(spellName);
		}
		while(result->next());

		db->freeResult(result);
	}

	//load inventory items
	ItemMap itemMap;
	query.str("");
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_items` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC";
	if((result = db->storeQuery(query.str())))
	{
		loadItems(itemMap, result);
		ItemMap::iterator it;
		for(ItemMap::reverse_iterator rit = itemMap.rbegin(); rit != itemMap.rend(); ++rit)
		{
			Item* item = rit->second.first;
			int32_t pid = rit->second.second;
			if(pid >= 1 && pid <= 10)
				player->__internalAddThing(pid, item);
			else
			{
				it = itemMap.find(pid);
				if(it != itemMap.end())
				{
					if(Container* container = it->second.first->getContainer())
						container->__internalAddThing(item);
				}
			}
		}

		db->freeResult(result);
	}

	//load depot items
	itemMap.clear();
	query.str("");
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_depotitems` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC";
	if((result = db->storeQuery(query.str())))
	{
		loadItems(itemMap, result);
		ItemMap::iterator it;
		for(ItemMap::reverse_iterator rit = itemMap.rbegin(); rit != itemMap.rend(); ++rit)
		{
			Item* item = rit->second.first;
			int32_t pid = rit->second.second;
			if(pid >= 0 && pid < 100)
			{
				if(Container* c = item->getContainer())
				{
					if(Depot* depot = c->getDepot())
						player->addDepot(depot, pid);
					else
						std::cout << "Error loading depot " << pid << " for player " << player->getGUID() << std::endl;
				}
				else
					std::cout << "Error loading depot " << pid << " for player " << player->getGUID() << std::endl;
			}
			else
			{
				it = itemMap.find(pid);
				if(it != itemMap.end())
				{
					if(Container* container = it->second.first->getContainer())
						container->__internalAddThing(item);
				}
			}
		}

		db->freeResult(result);
	}

	//load storage map
	query.str("");
	query << "SELECT `key`, `value` FROM `player_storage` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str())))
	{
		do
			player->addStorageValue((uint32_t)result->getDataInt("key"), result->getDataString("value"));
		while(result->next());

		db->freeResult(result);
	}

	//load vip
	query.str("");
	query << "SELECT `vip_id` FROM `player_viplist` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str())))
	{
		do
		{
			uint32_t vid = result->getDataInt("vip_id");
			std::string vname;
			if(storeNameByGuid(vid))
				player->addVIP(vid, vname, false, true);
		}
		while(result->next());

		db->freeResult(result);
	}

	player->updateBaseSpeed();
	player->updateInventoryWeigth();
	player->updateItemsLight(true);
	return true;
}

void IOLoginData::loadItems(ItemMap& itemMap, DBResult* result)
{
	do
	{
		int32_t sid = result->getDataInt("sid");
		int32_t pid = result->getDataInt("pid");
		int32_t type = result->getDataInt("itemtype");
		int32_t count = result->getDataInt("count");

		uint64_t attrSize = 0;
		const char* attr = result->getDataStream("attributes", attrSize);

		PropStream propStream;
		propStream.init(attr, attrSize);

		if(Item* item = Item::CreateItem(type, count))
		{
			if(!item->unserializeAttr(propStream))
				std::cout << "WARNING: Serialize error in IOLoginData::loadItems" << std::endl;
			std::pair<Item*, int32_t> pair(item, pid);
			itemMap[sid] = pair;
		}
	}
	while(result->next());
}

bool IOLoginData::savePlayer(Player* player, bool preSave/* = true*/)
{
	if(preSave && player->health <= 0)
	{
		player->health = player->healthMax;
		player->mana = player->manaMax;
	}

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `save` FROM `players` WHERE `id` = " << player->getGUID();
	if(!(result = db->storeQuery(query.str())))
		return false;

	const bool save = result->getDataInt("save");
	db->freeResult(result);

	DBTransaction trans(db);
	if(!trans.begin())
		return false;

	query.str("");
	query << "UPDATE `players` SET `lastlogin` = " << player->lastLoginSaved << ", `lastip` = " << player->lastIP;
	if(!save || !player->isSaving())
	{
		query << " WHERE `id` = " << player->getGUID();
		if(!db->executeQuery(query.str()))
			return false;

		return trans.commit();
	}

	query << ", ";
	query << "`level` = " << player->level << ", ";
	query << "`group_id` = " << player->groupId << ", ";
	query << "`health` = " << player->health << ", ";
	query << "`healthmax` = " << player->healthMax << ", ";
	query << "`experience` = " << player->experience << ", ";
	query << "`lookbody` = " << (uint32_t)player->defaultOutfit.lookBody << ", ";
	query << "`lookfeet` = " << (uint32_t)player->defaultOutfit.lookFeet << ", ";
	query << "`lookhead` = " << (uint32_t)player->defaultOutfit.lookHead << ", ";
	query << "`looklegs` = " << (uint32_t)player->defaultOutfit.lookLegs << ", ";
	query << "`looktype` = " << (uint32_t)player->defaultOutfit.lookType << ", ";
	query << "`lookaddons` = " << (uint32_t)player->defaultOutfit.lookAddons << ", ";
	query << "`maglevel` = " << player->magLevel << ", ";
	query << "`mana` = " << player->mana << ", ";
	query << "`manamax` = " << player->manaMax << ", ";
	query << "`manaspent` = " << player->manaSpent << ", ";
	query << "`soul` = " << player->soul << ", ";
	query << "`town_id` = " << player->town << ", ";
	query << "`posx` = " << player->getLoginPosition().x << ", ";
	query << "`posy` = " << player->getLoginPosition().y << ", ";
	query << "`posz` = " << player->getLoginPosition().z << ", ";
	query << "`cap` = " << player->getCapacity() << ", ";
	query << "`sex` = " << player->sex << ", ";
	query << "`balance` = " << player->balance << ", ";
	query << "`stamina` = " << player->getStamina() << ", ";
	query << "`promotion` = " << player->promotionLevel << ", ";

	//serialize conditions
	PropWriteStream propWriteStream;
	for(ConditionList::const_iterator it = player->conditions.begin(); it != player->conditions.end(); ++it)
	{
		if((*it)->isPersistent())
		{
			if(!(*it)->serialize(propWriteStream))
				return false;

			propWriteStream.ADD_UCHAR(CONDITIONATTR_END);
		}
	}

	uint32_t conditionsSize = 0;
	const char* conditions = propWriteStream.getStream(conditionsSize);
	query << "`conditions` = " << db->escapeBlob(conditions, conditionsSize) << ", ";
	query << "`loss_experience` = " << (uint32_t)player->getLossPercent(LOSS_EXPERIENCE) << ", ";
	query << "`loss_mana` = " << (uint32_t)player->getLossPercent(LOSS_MANASPENT) << ", ";
	query << "`loss_skills` = " << (uint32_t)player->getLossPercent(LOSS_SKILLTRIES) << ", ";
	query << "`loss_items` = " << (uint32_t)player->getLossPercent(LOSS_ITEMS) << ", ";
	if(g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
	{
		int32_t redSkullTime = 0;
		if(player->redSkullTicks > 0)
			redSkullTime = time(NULL) + player->redSkullTicks/1000;

		query << "`redskulltime` = " << redSkullTime << ", ";
		int32_t redSkull = 0;
		if(player->skull == SKULL_RED)
			redSkull = 1;

		query << "`redskull` = " << redSkull << ", ";
	}

	query << "`lastlogout` = " << player->getLastLogout() << ", ";
	if(player->isPremium() || !g_config.getBool(ConfigManager::BLESSING_ONLY_PREMIUM))
		query << "`blessings` = " << player->blessings << ", ";

	query << "`marriage` = " << player->marriage << ", ";
	if(g_config.getBool(ConfigManager::INGAME_GUILD_MANAGEMENT))
	{
		query << "`guildnick` = " << db->escapeString(player->guildNick) << ", ";
		query << "`rank_id` = " << IOGuild::getInstance()->getRankIdByGuildIdAndLevel(player->getGuildId(), player->getGuildLevel()) << ", ";
	}

	Vocation* tmpVoc = player->vocation;
	for(uint32_t i = 0; i <= player->promotionLevel; i++)
		tmpVoc = g_vocations.getVocation(tmpVoc->getFromVocation());

	query << "`vocation` = " << tmpVoc->getVocId() << " WHERE `id` = " << player->getGUID();
	if(!db->executeQuery(query.str()))
		return false;

	// skills
	for(int32_t i = 0; i <= 6; i++)
	{
		query.str("");
		query << "UPDATE `player_skills` SET `value` = " << player->skills[i][SKILL_LEVEL] << ", `count` = " << player->skills[i][SKILL_TRIES] << " WHERE `player_id` = " << player->getGUID() << " AND `skillid` = " << i;
		if(!db->executeQuery(query.str()))
			return false;
	}

	// learned spells
	query.str("");
	query << "DELETE FROM `player_spells` WHERE `player_id` = " << player->getGUID();
	if(!db->executeQuery(query.str()))
		return false;

	char buffer[280];
	DBInsert query_insert(db);

	query_insert.setQuery("INSERT INTO `player_spells` (`player_id`, `name`) VALUES ");
	for(LearnedInstantSpellList::const_iterator it = player->learnedInstantSpellList.begin(); it != player->learnedInstantSpellList.end(); ++it)
	{
		sprintf(buffer, "%d, %s", player->getGUID(), db->escapeString(*it).c_str());
		if(!query_insert.addRow(buffer))
			return false;
	}

	if(!query_insert.execute())
		return false;

	//item saving
	query.str("");
	query << "DELETE FROM `player_items` WHERE `player_id` = " << player->getGUID();
	if(!db->executeQuery(query.str()))
		return false;

	ItemBlockList itemList;
	for(int32_t slotId = 1; slotId < 11; ++slotId)
	{
		if(Item* item = player->inventory[slotId])
			itemList.push_back(itemBlock(slotId, item));
	}

	query_insert.setQuery("INSERT INTO `player_items` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");
	if(!saveItems(player, itemList, query_insert))
		return false;

	//save depot items
	query.str("");
	query << "DELETE FROM `player_depotitems` WHERE `player_id` = " << player->getGUID();
	if(!db->executeQuery(query.str()))
		return false;

	itemList.clear();
	for(DepotMap::iterator it = player->depots.begin(); it != player->depots.end(); ++it)
		itemList.push_back(itemBlock(it->first, it->second));

	query_insert.setQuery("INSERT INTO `player_depotitems` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");
	if(!saveItems(player, itemList, query_insert))
		return false;

	query.str("");
	query << "DELETE FROM `player_storage` WHERE `player_id` = " << player->getGUID();
	if(!db->executeQuery(query.str()))
		return false;

	player->genReservedStorageRange();
	query_insert.setQuery("INSERT INTO `player_storage` (`player_id`, `key`, `value`) VALUES ");
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(); cit != player->getStorageIteratorEnd(); ++cit)
	{
		sprintf(buffer, "%u, %u, %s", player->getGUID(), cit->first, db->escapeString(cit->second).c_str());
		if(!query_insert.addRow(buffer))
			return false;
	}

	if(!query_insert.execute())
		return false;

	if(g_config.getBool(ConfigManager::INGAME_GUILD_MANAGEMENT))
	{
		//save guild invites
		query.str("");
		query << "DELETE FROM `guild_invites` WHERE player_id = " << player->getGUID();
		if(!db->executeQuery(query.str()))
			return false;

		query_insert.setQuery("INSERT INTO `guild_invites` (`player_id`, `guild_id`) VALUES ");
		for(InvitedToGuildsList::const_iterator it = player->invitedToGuildsList.begin(); it != player->invitedToGuildsList.end(); ++it)
		{
			sprintf(buffer, "%d, %d", player->getGUID(), *it);
			if(!query_insert.addRow(buffer))
				return false;
		}

		if(!query_insert.execute())
			return false;
	}

	//save vip list
	query.str("");
	query << "DELETE FROM `player_viplist` WHERE `player_id` = " << player->getGUID() << ";";
	if(!db->executeQuery(query.str()))
		return false;

	query_insert.setQuery("INSERT INTO `player_viplist` (`player_id`, `vip_id`) VALUES ");
	for(VIPListSet::iterator it = player->VIPList.begin(); it != player->VIPList.end(); it++)
	{
		if(playerExists(*it))
		{
			sprintf(buffer, "%d, %d", player->getGUID(), *it);
			if(!query_insert.addRow(buffer))
				return false;
		}
	}

	if(!query_insert.execute())
		return false;

	//End the transaction
	return trans.commit();
}

bool IOLoginData::saveItems(const Player* player, const ItemBlockList& itemList, DBInsert& query_insert)
{
	Database* db = Database::getInstance();
	std::list<Container*> listContainer;

	typedef std::pair<Container*, int32_t> containerBlock;
	std::list<containerBlock> stack;

	int32_t parentId = 0, runningId = 100, pid;
	Item* item;

	for(ItemBlockList::const_iterator it = itemList.begin(); it != itemList.end(); ++it)
	{
		pid = it->first;
		item = it->second;
		++runningId;

		uint32_t attributesSize;
		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		const char* attributes = propWriteStream.getStream(attributesSize);

		char buffer[attributesSize * 3 + 100]; //MUST be (size * 2), else people can crash server when filling writable with native characters
		sprintf(buffer, "%d, %d, %d, %d, %d, %s", player->getGUID(), pid, runningId, item->getID(), (int32_t)item->getSubType(), db->escapeBlob(attributes, attributesSize).c_str());
		if(!query_insert.addRow(buffer))
			return false;

		if(Container* container = item->getContainer())
			stack.push_back(containerBlock(container, runningId));
	}

	while(stack.size() > 0)
	{
		const containerBlock& cb = stack.front();
		Container* container = cb.first;
		parentId = cb.second;
		stack.pop_front();
		for(uint32_t i = 0; i < container->size(); ++i)
		{
			++runningId;
			item = container->getItem(i);
			Container* container = item->getContainer();
			if(container)
				stack.push_back(containerBlock(container, runningId));

			uint32_t attributesSize;
			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			char buffer[attributesSize * 3 + 100]; //MUST be (size * 2), else people can crash server when filling writable with native characters
			sprintf(buffer, "%d, %d, %d, %d, %d, %s", player->getGUID(), parentId, runningId, item->getID(), (int32_t)item->getSubType(), db->escapeBlob(attributes, attributesSize).c_str());
			if(!query_insert.addRow(buffer))
				return false;
		}
	}

	if(!query_insert.execute())
		return false;

	return true;
}

bool IOLoginData::updateOnlineStatus(uint32_t guid, bool login)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `online` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint16_t onlineValue = result->getDataInt("online");
	db->freeResult(result);
	if(login)
		onlineValue++;
	else if(!g_config.getNumber(ConfigManager::ALLOW_CLONES))
		onlineValue = 0;
	else if(onlineValue > 0)
		onlineValue--;

	query.str("");
	query << "UPDATE `players` SET `online` = " << onlineValue << " WHERE `id` = " << guid;
	return db->executeQuery(query.str());
}

bool IOLoginData::hasFlag(std::string name, PlayerFlags value)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t group = result->getDataInt("group_id");
	db->freeResult(result);
	return internalHasFlag(group, value);
}

bool IOLoginData::hasCustomFlag(std::string name, PlayerCustomFlags value)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t group = result->getDataInt("group_id");
	db->freeResult(result);
	return internalHasCustomFlag(group, value);
}

bool IOLoginData::isPremium(uint32_t guid)
{
	if(g_config.getBool(ConfigManager::FREE_PREMIUM))
		return true;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `account_id`, `group_id` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t account = result->getDataInt("account_id"), group = result->getDataInt("group_id");
	db->freeResult(result);
	if(internalHasFlag(group, PlayerFlag_IsAlwaysPremium))
		return true;

	query.str("");
	query << "SELECT `premdays` FROM `accounts` WHERE `id` = " << account;
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t premium = result->getDataInt("premdays");
	db->freeResult(result);
	return premium;
}

bool IOLoginData::playerExists(uint32_t guid, bool multiworld /*= false*/)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
		return true;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0";
	if(!multiworld)
		query << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);

	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
}

bool IOLoginData::playerExists(std::string name, bool multiworld /*= false*/)
{
	GuidCacheMap::iterator it = guidCacheMap.find(name);
	if(it != guidCacheMap.end())
		return true;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0";
	if(!multiworld)
		query << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);

	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
}

bool IOLoginData::getNameByGuid(uint32_t guid, std::string& name, bool multiworld /*= false*/)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
	{
		name = it->second;
		return true;
	}

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `name` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0";
	if(!multiworld)
		query << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);

	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	db->freeResult(result);

	nameCacheMap[guid] = name;
	return true;
}

bool IOLoginData::storeNameByGuid(uint32_t guid)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
		return true;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `name` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	nameCacheMap[guid] = result->getDataString("name");
	db->freeResult(result);
	return true;
}

bool IOLoginData::getGuidByName(uint32_t &guid, std::string& name, bool multiworld /*= false*/)
{
	GuidCacheMap::iterator it = guidCacheMap.find(name);
	if(it != guidCacheMap.end())
	{
		name = it->first;
		guid = it->second;
		return true;
	}

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `name`, `id` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0";
	if(!multiworld)
		query << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);

	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	guid = result->getDataInt("id");

	guidCacheMap[name] = guid;
	db->freeResult(result);
	return true;
}

bool IOLoginData::getGuidByNameEx(uint32_t& guid, bool &specialVip, std::string& name)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `name`, `group_id` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0 AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!(result = db->storeQuery(query.str())))
		return false;

	guid = result->getDataInt("id");
	specialVip = internalHasFlag(result->getDataInt("group_id"), PlayerFlag_SpecialVIP);
	name = result->getDataString("name");

	db->freeResult(result);
	return true;
}

uint32_t IOLoginData::getAccountIdByName(std::string name)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `account_id` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t accountId = result->getDataInt("account_id");
	db->freeResult(result);
	return accountId;
}

bool IOLoginData::changeName(uint32_t guid, std::string newName, std::string oldName)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `players` SET `name` = " << db->escapeString(newName) << " WHERE `id` = " << guid;
	if(db->executeQuery(query.str()))
	{
		GuidCacheMap::iterator it = guidCacheMap.find(oldName);
		if(it != guidCacheMap.end())
		{
			guidCacheMap.erase(it);
			guidCacheMap[newName] = guid;
		}

		nameCacheMap[guid] = newName;
		return true;
	}

	return false;
}

bool IOLoginData::createCharacter(uint32_t accountId, std::string characterName, int32_t vocationId, PlayerSex_t sex)
{
	if(playerExists(characterName))
		return false;

	Database* db = Database::getInstance();

	Vocation* vocation = g_vocations.getVocation(vocationId);
	Vocation* rookVoc = g_vocations.getVocation(0);
	uint16_t healthMax = 150, manaMax = 0, capMax = 400, lookType = 136;
	if(sex == PLAYERSEX_MALE)
		lookType = 128;

	uint32_t level = g_config.getNumber(ConfigManager::START_LEVEL);
	uint64_t exp = 0;
	if(level > 1)
		exp = Player::getExpForLevel(level);

	uint32_t tmpLevel = level - 1;
	if(tmpLevel > 0)
	{
		if(tmpLevel > 7)
			tmpLevel = 7;

		healthMax += rookVoc->getHealthGain() * tmpLevel;
		manaMax += rookVoc->getManaGain() * tmpLevel;
		capMax += rookVoc->getCapGain() * tmpLevel;

		if(level > 8)
		{
			tmpLevel = level - 8;
			healthMax += vocation->getHealthGain() * tmpLevel;
			manaMax += vocation->getManaGain() * tmpLevel;
			capMax += vocation->getCapGain() * tmpLevel;
		}
	}

	DBQuery query;
	query << "INSERT INTO `players` (`id`, `name`, `world_id`, `group_id`, `account_id`, `level`, `vocation`, `health`, `healthmax`, `experience`, `lookbody`, `lookfeet`, `lookhead`, `looklegs`, `looktype`, `lookaddons`, `maglevel`, `mana`, `manamax`, `manaspent`, `soul`, `town_id`, `posx`, `posy`, `posz`, `conditions`, `cap`, `sex`, `lastlogin`, `lastip`, `redskull`, `redskulltime`, `save`, `rank_id`, `guildnick`, `lastlogout`, `blessings`, `online`) VALUES (NULL, " << db->escapeString(characterName) << ", " << g_config.getNumber(ConfigManager::WORLD_ID) << ", 1, " << accountId << ", " << level << ", " << vocationId << ", " << healthMax << ", " << healthMax << ", " << exp << ", 68, 76, 78, 39, " << lookType << ", 0, " << g_config.getNumber(ConfigManager::START_MAGICLEVEL) << ", " << manaMax << ", " << manaMax << ", 0, 100, " << g_config.getNumber(ConfigManager::SPAWNTOWN_ID) << ", " << g_config.getNumber(ConfigManager::SPAWNPOS_X) << ", " << g_config.getNumber(ConfigManager::SPAWNPOS_Y) << ", " << g_config.getNumber(ConfigManager::SPAWNPOS_Z) << ", 0, " << capMax << ", " << sex << ", 0, 0, 0, 0, 1, 0, '', 0, 0, 0)";
	return db->executeQuery(query.str());
}

DeleteCharacter_t IOLoginData::deleteCharacter(uint32_t accountId, const std::string characterName)
{
	if(g_game.getPlayerByName(characterName))
		return DELETE_ONLINE;

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(characterName) << " AND `account_id` = " << accountId << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return DELETE_INTERNAL;

	uint32_t id = result->getDataInt("id");
	db->freeResult(result);

	House* house = Houses::getInstance().getHouseByPlayerId(id);
	if(house)
		return DELETE_HOUSE;

	if(IOGuild::getInstance()->getGuildLevel(id) == 3)
		return DELETE_LEADER;

	query.str("");
	query << "UPDATE `players` SET `deleted` = 1 WHERE `id` = " << id;
	if(!db->executeQuery(query.str()))
		return DELETE_INTERNAL;

	query.str("");
	query << "DELETE FROM `guild_invites` WHERE `player_id` = " << id;
	db->executeQuery(query.str());

	query.str("");
	query << "DELETE FROM `player_viplist` WHERE `vip_id` = " << id;
	db->executeQuery(query.str());

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		VIPListSet::iterator it_ = it->second->VIPList.find(id);
		if(it_ != it->second->VIPList.end())
			it->second->VIPList.erase(it_);
	}

	return DELETE_SUCCESS;
}

uint32_t IOLoginData::getLevel(uint32_t guid) const
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `level` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	uint32_t level = result->getDataInt("level");
	db->freeResult(result);
	return level;
}

uint32_t IOLoginData::getLastIP(uint32_t guid) const
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `lastip` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	uint32_t lastip = result->getDataInt("lastip");
	db->freeResult(result);
	return lastip;
}

uint32_t IOLoginData::getLastIPByName(std::string name)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `lastip` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	uint32_t lastip = result->getDataInt("lastip");
	db->freeResult(result);
	return lastip;
}

bool IOLoginData::updatePremiumDays()
{
	Database* db = Database::getInstance();
	DBTransaction trans(db);
	if(!trans.begin())
		return false;

	DBResult* result;
	DBQuery query;
	query << "SELECT `id` FROM `accounts` WHERE `lastday` <= " << time(NULL) - 86400;
	if((result = db->storeQuery(query.str())))
	{
		do
			removePremium(loadAccount(result->getDataInt("id"), true));
		while(result->next());
		db->freeResult(result);
		query.str("");
	}

	return trans.commit();
}

bool IOLoginData::resetOnlineStatus()
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `players` SET `online` = 0 WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	return db->executeQuery(query.str());
}

bool IOLoginData::resetGuildInformation(uint32_t guid)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `players` SET `rank_id` = 0, `guildnick` = '' WHERE `id` = " << guid << " AND `deleted` = 0;";
	return db->executeQuery(query.str());
}
