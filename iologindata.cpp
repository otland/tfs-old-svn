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
#include <iostream>
#include <iomanip>

#include "iologindata.h"
#include "tools.h"

#ifdef __LOGIN_SERVER__
#include "gameservers.h"
#endif
#include "town.h"
#include "house.h"

#include "item.h"
#include "vocation.h"

#include "configmanager.h"
#include "game.h"

extern ConfigManager g_config;
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
	result->free();
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
	result->free();

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
	result->free();
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
	result->free();
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

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	result->free();
	return group && group->hasFlag(value);
}

bool IOLoginData::hasCustomFlag(uint32_t accId, PlayerCustomFlags value)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	result->free();
	return group && group->hasCustomFlag(value);
}

bool IOLoginData::hasFlag(PlayerFlags value, const std::string& accName)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `accounts` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(accName);
	if(!(result = db->storeQuery(query.str())))
		return false;

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	result->free();
	return group && group->hasFlag(value);
}

bool IOLoginData::hasCustomFlag(PlayerCustomFlags value, const std::string& accName)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `accounts` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(accName);
	if(!(result = db->storeQuery(query.str())))
		return false;

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	result->free();
	return group && group->hasCustomFlag(value);
}

bool IOLoginData::accountExists(uint32_t accId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	result->free();
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

	result->free();
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
	result->free();

	query.str("");
	query << "SELECT `name` FROM `players` WHERE `account_id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	do
	{
		if(result->getDataString("name") == name)
		{
			password = accountPassword;
			result->free();
			return true;
		}
	}
	while(result->next());
	result->free();
	return false;
}

bool IOLoginData::setNewPassword(uint32_t accountId, std::string newPassword)
{
	Database* db = Database::getInstance();
	switch(g_config.getNumber(ConfigManager::PASSWORDTYPE))
	{
		case PASSWORD_TYPE_MD5:
			newPassword = transformToMD5(newPassword);
			break;
		case PASSWORD_TYPE_SHA1:
			newPassword = transformToSHA1(newPassword);
			break;
		default:
			break;
	}

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
	if(!(result = db->storeQuery(query.str())))
		return false;

	result->free();
	return true;
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
	switch(g_config.getNumber(ConfigManager::PASSWORDTYPE))
	{
		case PASSWORD_TYPE_MD5:
			password = transformToMD5(password);
			break;
		case PASSWORD_TYPE_SHA1:
			password = transformToSHA1(password);
			break;
		default:
			break;
	}

	DBQuery query;
	query << "INSERT INTO `accounts` (`id`, `name`, `password`) VALUES (NULL, " << db->escapeString(name) << ", " << db->escapeString(password) << ");";
	return db->executeQuery(query.str());
}

void IOLoginData::removePremium(Account account)
{
	uint64_t timeNow = time(NULL);
	if(account.premiumDays > 0 && account.premiumDays < 65535)
	{
		uint32_t days = (uint32_t)std::ceil((double)(timeNow - account.lastDay) / 86400);
		if(days > 0)
		{
			if(account.premiumDays >= days)
				account.premiumDays -= days;
			else
				account.premiumDays = 0;

			account.lastDay = timeNow;
		}
	}
	else
		account.lastDay = timeNow;

	if(!saveAccount(account))
		std::cout << "> ERROR: Failed to save account: " << account.name << "!" << std::endl;
}

const Group* IOLoginData::getPlayerGroupByAccount(uint32_t accId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return NULL;

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	result->free();
	return group;
}

bool IOLoginData::loadPlayer(Player* player, const std::string& name, bool preLoad /*= false*/)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `account_id`, `group_id`, `sex`, `vocation`, `experience`, `level`, `maglevel`, `health`, `healthmax`, `blessings`, `mana`, `manamax`, `manaspent`, `soul`, `lookbody`, `lookfeet`, `lookhead`, `looklegs`, `looktype`, `lookaddons`, `posx`, `posy`, `posz`, `cap`, `lastlogin`, `lastlogout`, `lastip`, `conditions`, `redskulltime`, `redskull`, `guildnick`, `rank_id`, `town_id`, `balance`, `stamina`, `direction`, `loss_experience`, `loss_mana`, `loss_skills`, `loss_containers`, `loss_items`, `marriage`, `promotion`, `description` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID) << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint32_t accId = result->getDataInt("account_id");
	if(accId < 1)
	{
		result->free();
		return false;
	}

	Account acc = loadAccount(accId, true);
	player->accountId = accId;
	player->account = acc.name;

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	player->setGroup(group);

	player->setGUID(result->getDataInt("id"));
	player->premiumDays = acc.premiumDays;

	nameCacheMap[player->getGUID()] = name;
	guidCacheMap[name] = player->getGUID();
	if(preLoad)
	{
		//only loading basic info
		result->free();
		return true;
	}

	player->nameDescription += result->getDataString("description");
	player->setSex((PlayerSex_t)result->getDataInt("sex"));
	if(g_config.getBool(ConfigManager::STORE_DIRECTION))
		player->setDirection((Direction)result->getDataInt("direction"));

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
	if(!group || !group->getOutfit())
		player->defaultOutfit.lookType = result->getDataInt("looktype");
	else
		player->defaultOutfit.lookType = group->getOutfit();

	player->defaultOutfit.lookHead = result->getDataInt("lookhead");
	player->defaultOutfit.lookBody = result->getDataInt("lookbody");
	player->defaultOutfit.lookLegs = result->getDataInt("looklegs");
	player->defaultOutfit.lookFeet = result->getDataInt("lookfeet");
	player->defaultOutfit.lookAddons = result->getDataInt("lookaddons");

	player->currentOutfit = player->defaultOutfit;
	if(g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
	{
		int32_t redSkullSeconds = result->getDataInt("redskulltime") - time(NULL);
		if(redSkullSeconds > 0)
		{
			//ensure that we round up the number of ticks
			player->redSkullTicks = (redSkullSeconds + 2) * 1000;
			if(result->getDataInt("redskull"))
				player->skull = SKULL_RED;
		}
	}

	player->setLossPercent(LOSS_EXPERIENCE, result->getDataInt("loss_experience"));
	player->setLossPercent(LOSS_MANA, result->getDataInt("loss_mana"));
	player->setLossPercent(LOSS_SKILLS, result->getDataInt("loss_skills"));
	player->setLossPercent(LOSS_CONTAINERS, result->getDataInt("loss_containers"));
	player->setLossPercent(LOSS_ITEMS, result->getDataInt("loss_items"));

	player->loginPosition = Position(result->getDataInt("posx"), result->getDataInt("posy"), result->getDataInt("posz"));
	player->lastLoginSaved = result->getDataLong("lastlogin");
	player->lastLogout = result->getDataLong("lastlogout");

	player->town = result->getDataInt("town_id");
	if(Town* town = Towns::getInstance().getTown(player->town))
		player->masterPos = town->getTemplePosition();

	Position loginPos = player->loginPosition;
	if(loginPos.x == 0 && loginPos.y == 0 && loginPos.z == 0)
		player->loginPosition = player->masterPos;

	const uint32_t rankId = result->getDataInt("rank_id");
	const std::string nick = result->getDataString("guildnick");

	result->free();
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

			result->free();
		}
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
			result->free();
		}
	}

	query.str("");
	query << "SELECT `password` FROM `accounts` WHERE `id` = " << accId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	player->password = result->getDataString("password");
	result->free();

	// we need to find out our skills
	// so we query the skill table
	query.str("");
	query << "SELECT `skillid`, `value`, `count` FROM `player_skills` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str())))
	{
		//now iterate over the skills
		do
		{
			int32_t skillId = result->getDataInt("skillid");
			if(skillId >= SKILL_FIRST && skillId <= SKILL_LAST)
			{
				uint32_t skillLevel = result->getDataInt("value");
				uint64_t skillCount = result->getDataLong("count");
				uint64_t nextSkillCount = player->vocation->getReqSkillTries(skillId, skillLevel + 1);
				if(skillCount > nextSkillCount)
					skillCount = 0;

				player->skills[skillId][SKILL_LEVEL] = skillLevel;
				player->skills[skillId][SKILL_TRIES] = skillCount;
				player->skills[skillId][SKILL_PERCENT] = Player::getPercentLevel(skillCount, nextSkillCount);
			}
		}
		while(result->next());
		result->free();
	}

	query.str("");
 	query << "SELECT `player_id`, `name` FROM `player_spells` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str())))
	{
		do
			player->learnedInstantSpellList.push_back(result->getDataString("name"));
		while(result->next());
		result->free();
	}

	ItemMap itemMap;
	ItemMap::iterator it;

	//load inventory items
	query.str("");
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_items` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC";
	if((result = db->storeQuery(query.str())))
	{
		loadItems(itemMap, result);
		for(ItemMap::reverse_iterator rit = itemMap.rbegin(); rit != itemMap.rend(); ++rit)
		{
			Item* item = rit->second.first;
			int32_t pid = rit->second.second;
			if(pid > 0 && pid < 11)
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

		result->free();
		itemMap.clear();
	}

	//load depot items
	query.str("");
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_depotitems` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC";
	if((result = db->storeQuery(query.str())))
	{
		loadItems(itemMap, result);
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
						std::cout << "[Error - IOLoginData::loadPlayer] Cannot load depot " << pid << " for player " << name << std::endl;
				}
				else
					std::cout << "[Error - IOLoginData::loadPlayer] Cannot load depot " << pid << " for player " << name << std::endl;
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

		result->free();
		itemMap.clear();
	}

	//load storage map
	query.str("");
	query << "SELECT `key`, `value` FROM `player_storage` WHERE `player_id` = " << player->getGUID();
	if((result = db->storeQuery(query.str())))
	{
		do
			player->addStorageValue((uint32_t)result->getDataInt("key"), result->getDataString("value"));
		while(result->next());
		result->free();
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
		result->free();
	}

	player->updateInventoryWeigth();
	player->updateItemsLight(true);
	player->updateBaseSpeed();
	return true;
}

void IOLoginData::loadItems(ItemMap& itemMap, DBResult* result)
{
	do
	{
		uint64_t attrSize = 0;
		const char* attr = result->getDataStream("attributes", attrSize);

		PropStream propStream;
		propStream.init(attr, attrSize);
		if(Item* item = Item::CreateItem(result->getDataInt("itemtype"), result->getDataInt("count")))
		{
			if(!item->unserializeAttr(propStream))
				std::cout << "[Warning - IOLoginData::loadItems] Unserialize error for item with id " << item->getID() << std::endl;

			itemMap[result->getDataInt("sid")] = std::make_pair(item, result->getDataInt("pid"));
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
	result->free();

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
	query << "`level` = " << std::max((uint32_t)1, player->getLevel()) << ", ";
	query << "`group_id` = " << player->groupId << ", ";
	query << "`health` = " << player->health << ", ";
	query << "`healthmax` = " << player->healthMax << ", ";
	query << "`experience` = " << std::max((uint64_t)0, player->getExperience()) << ", ";
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
	if(g_config.getBool(ConfigManager::STORE_DIRECTION))
		query << "`direction` = " << (uint32_t)player->getDirection() << ", ";

	if(!player->isVirtual())
	{
		std::string name = player->getName(), nameDescription = player->getNameDescription();
		if(!player->isAccountManager() && nameDescription.length() > name.length())
			query << "`description` = " << db->escapeString(nameDescription.substr(name.length())) << ", ";
	}

	//serialize conditions
	PropWriteStream propWriteStream;
	for(ConditionList::const_iterator it = player->conditions.begin(); it != player->conditions.end(); ++it)
	{
		if((*it)->isPersistent() || (*it)->getType() == CONDITION_GAMEMASTER)
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
	query << "`loss_mana` = " << (uint32_t)player->getLossPercent(LOSS_MANA) << ", ";
	query << "`loss_skills` = " << (uint32_t)player->getLossPercent(LOSS_SKILLS) << ", ";
	query << "`loss_containers` = " << (uint32_t)player->getLossPercent(LOSS_CONTAINERS) << ", ";
	query << "`loss_items` = " << (uint32_t)player->getLossPercent(LOSS_ITEMS) << ", ";
	if(g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
	{
		int32_t redSkullTime = 0;
		if(player->redSkullTicks > 0)
			redSkullTime = (time(NULL) + (player->redSkullTicks / 1000));

		query << "`redskulltime` = " << redSkullTime << ", ";
		query << "`redskull` = " << (player->skull == SKULL_RED ? 1 : 0) << ", ";
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
		tmpVoc = Vocations::getInstance()->getVocation(tmpVoc->getFromVocation());

	query << "`vocation` = " << tmpVoc->getId() << " WHERE `id` = " << player->getGUID();
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

	itemList.clear();
	//save depot items
	//std::stringstream ss;
	for(DepotMap::iterator it = player->depots.begin(); it != player->depots.end(); ++it)
	{
		/*if(it->second.second)
		{
			it->second.second = false;
			ss << it->first << ",";*/
			itemList.push_back(itemBlock(it->first, it->second.first));
		//}
	}

	/*std::string s = ss.str();
	size_t size = s.length();
	if(size > 0)
	{*/
		query.str("");
		query << "DELETE FROM `player_depotitems` WHERE `player_id` = " << player->getGUID();// << " AND `pid` IN (" << s.substr(0, --size) << ")";
		if(!db->executeQuery(query.str()))
			return false;

		query_insert.setQuery("INSERT INTO `player_depotitems` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");
		if(!saveItems(player, itemList, query_insert))
			return false;

		itemList.clear();
	//}

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
		if(playerExists(*it, false, false))
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
	typedef std::pair<Container*, uint32_t> Stack;
	std::list<Stack> stackList;

	Item* item = NULL;
	int32_t runningId = 101;
	for(ItemBlockList::const_iterator it = itemList.begin(); it != itemList.end(); ++it, ++runningId)
	{
		item = it->second;

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);

		uint32_t attributesSize = 0;
		const char* attributes = propWriteStream.getStream(attributesSize);
		char buffer[attributesSize * 3 + 100]; //MUST be (size * 2), else people can crash server when filling writable with native characters

		sprintf(buffer, "%d, %d, %d, %d, %d, %s", player->getGUID(), it->first, runningId, item->getID(),
			(int32_t)item->getSubType(), db->escapeBlob(attributes, attributesSize).c_str());
		if(!query_insert.addRow(buffer))
			return false;

		if(Container* container = item->getContainer())
			stackList.push_back(Stack(container, runningId));
	}

	while(stackList.size() > 0)
	{
		Stack stack = stackList.front();
		stackList.pop_front();

		Container* container = stack.first;
		for(uint32_t i = 0; i < container->size(); ++i, ++runningId)
		{
			item = container->getItem(i);
			if(Container* subContainer = item->getContainer())
				stackList.push_back(Stack(subContainer, runningId));

			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);

			uint32_t attributesSize = 0;
			const char* attributes = propWriteStream.getStream(attributesSize);
			char buffer[attributesSize * 3 + 100]; //MUST be (size * 2), else people can crash server when filling writable with native characters

			sprintf(buffer, "%d, %d, %d, %d, %d, %s", player->getGUID(), stack.second, runningId, item->getID(),
				(int32_t)item->getSubType(), db->escapeBlob(attributes, attributesSize).c_str());
			if(!query_insert.addRow(buffer))
				return false;
		}
	}

	return query_insert.execute();
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
	result->free();
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

bool IOLoginData::hasFlag(const std::string& name, PlayerFlags value)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	result->free();
	return group && group->hasFlag(value);
}

bool IOLoginData::hasCustomFlag(const std::string& name, PlayerCustomFlags value)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	result->free();
	return group && group->hasCustomFlag(value);
}

bool IOLoginData::hasFlag(PlayerFlags value, uint32_t guid)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	result->free();
	return group && group->hasFlag(value);
}

bool IOLoginData::hasCustomFlag(PlayerCustomFlags value, uint32_t guid)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `group_id` FROM `players` WHERE `id` = " << guid << " AND `deleted` = 0;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	result->free();
	return group && group->hasCustomFlag(value);
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

	Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id"));
	const uint32_t account = result->getDataInt("account_id");

	result->free();
	if(group && group->hasCustomFlag(PlayerFlag_IsAlwaysPremium))
		return true;

	query.str("");
	query << "SELECT `premdays` FROM `accounts` WHERE `id` = " << account;
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t premium = result->getDataInt("premdays");
	result->free();
	return premium;
}

bool IOLoginData::playerExists(uint32_t guid, bool multiworld /*= false*/, bool checkCache /*= true*/)
{
	if(checkCache)
	{
		NameCacheMap::iterator it = nameCacheMap.find(guid);
		if(it != nameCacheMap.end())
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

	const std::string name = result->getDataString("name");
	result->free();

	nameCacheMap[guid] = name;
	return true;
}

bool IOLoginData::playerExists(std::string& name, bool multiworld /*= false*/, bool checkCache /*= true*/)
{
	if(checkCache)
	{
		GuidCacheMap::iterator it = guidCacheMap.find(name);
		if(it != guidCacheMap.end())
		{
			name = it->first;
			return true;
		}
	}

	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `name` FROM `players` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(name) << " AND `deleted` = 0";
	if(!multiworld)
		query << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);

	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	guidCacheMap[name] = result->getDataInt("id");

	result->free();
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
	result->free();

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
	result->free();
	return true;
}

bool IOLoginData::getGuidByName(uint32_t& guid, std::string& name, bool multiworld /*= false*/)
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
	result->free();
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
	name = result->getDataString("name");
	if(Group* group = Groups::getInstance()->getGroup(result->getDataInt("group_id")))
		specialVip = group->hasFlag(PlayerFlag_SpecialVIP);

	result->free();
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
	result->free();
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

	Vocation* vocation = Vocations::getInstance()->getVocation(vocationId);
	Vocation* rookVoc = Vocations::getInstance()->getVocation(0);
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
	result->free();

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
	result->free();
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
	result->free();
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
	result->free();
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
		result->free();
	}

	query.str("");
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
