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
#include <iostream>
#include <iomanip>

extern ConfigManager g_config;
extern Vocations g_vocations;
extern Game g_game;

#ifndef __GNUC__
#pragma warning( disable : 4005)
#pragma warning( disable : 4996)
#endif

Account IOLoginData::loadAccount(uint32_t accno)
{
	Account acc;

	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `id`, `name`, `password`, `type`, `premdays`, `lastday`, `key`, `warnings` FROM `accounts` WHERE `id` = " << accno << ";";
	if((result = db->storeQuery(query.str())))
	{
		acc.id = result->getDataInt("id");
		acc.name = result->getDataString("name");
		acc.password = result->getDataString("password");
		acc.accountType = (AccountType_t)result->getDataInt("type");
		acc.premiumDays = result->getDataInt("premdays");
		acc.lastDay = result->getDataInt("lastday");
		acc.recoveryKey = result->getDataString("key");
		acc.warnings = result->getDataInt("warnings");
		db->freeResult(result);

		query.str("");
		query << "SELECT `name` FROM `players` WHERE `account_id` = " << acc.id << ";";
		if((result = db->storeQuery(query.str())))
		{
			do
			{
				std::string characterName = result->getDataString("name");
				acc.charList.push_back(characterName.c_str());
			}
			while(result->next());
			db->freeResult(result);

			acc.charList.sort();
		}
	}
	return acc;
}

Account IOLoginData::loadAccount(std::string name)
{
	Account acc;

	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `id`, `name`, `password`, `type`, `premdays`, `lastday`, `key`, `warnings` FROM `accounts` WHERE `name` " << db->getStringComparer() << db->escapeString(name) << " LIMIT 1;";
	if((result = db->storeQuery(query.str())))
	{
		acc.id = result->getDataInt("id");
		acc.name = result->getDataString("name");
		acc.password = result->getDataString("password");
		acc.accountType = (AccountType_t)result->getDataInt("type");
		acc.premiumDays = result->getDataInt("premdays");
		acc.lastDay = result->getDataInt("lastday");
		acc.recoveryKey = result->getDataString("key");
		acc.warnings = result->getDataInt("warnings");
		db->freeResult(result);

		query.str("");
		query << "SELECT `name` FROM `players` WHERE `account_id` = " << acc.id << ";";
		if((result = db->storeQuery(query.str())))
		{
			do
			{
				std::string characterName = result->getDataString("name");
				acc.charList.push_back(characterName.c_str());
			}
			while(result->next());
			db->freeResult(result);

			acc.charList.sort();
		}
	}
	return acc;
}

bool IOLoginData::saveAccount(Account acc)
{
	DBQuery query;
	query << "UPDATE `accounts` SET `premdays` = " << acc.premiumDays << ", `warnings` = " << acc.warnings << ", `lastday` = " << acc.lastDay << " WHERE `id` = " << acc.id << ";";
	return Database::getInstance()->executeQuery(query.str());
}

bool IOLoginData::createAccount(const std::string& accountName, std::string newPassword)
{
	Database* db = Database::getInstance();

	if(g_config.getNumber(ConfigManager::PASSWORD_TYPE) == PASSWORD_TYPE_MD5)
		newPassword = transformToMD5(newPassword);
	else if(g_config.getNumber(ConfigManager::PASSWORD_TYPE) == PASSWORD_TYPE_SHA1)
		newPassword = transformToSHA1(newPassword);

	DBQuery query;
	query << "INSERT INTO `accounts` (`id`, `name`, `password`, `type`, `premdays`, `lastday`, `key`, `warnings`, `group_id`) VALUES (NULL, " << db->escapeString(accountName) << ", " << db->escapeString(newPassword) << ", 1, 0, 0, 0, 0, 1);";
	return db->executeQuery(query.str());
}

bool IOLoginData::getPassword(const std::string& accname, const std::string& name, std::string& password, uint32_t& accNumber)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `id`, `password` FROM `accounts` WHERE `name` " << db->getStringComparer() << db->escapeString(accname) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	std::string accountPassword = result->getDataString("password");
	accNumber = result->getDataInt("id");
	db->freeResult(result);
	query.str("");
	query << "SELECT `name` FROM `players` WHERE `account_id` = " << accNumber << ";";
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

bool IOLoginData::getPasswordEx(const std::string& accname, std::string& password, uint32_t& accNumber)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `id`, `password` FROM `accounts` WHERE `name` " << db->getStringComparer() << db->escapeString(accname) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	accNumber = result->getDataInt("id");
	password = result->getDataString("password");
	db->freeResult(result);
	return true;
}

bool IOLoginData::accountNameExists(const std::string& name)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `id` FROM `accounts` WHERE `name` " << db->getStringComparer() << db->escapeString(name) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
}

bool IOLoginData::setRecoveryKey(uint32_t accountNumber, std::string recoveryKey)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `accounts` SET `key` = " << db->escapeString(recoveryKey) << " WHERE `id` = " << accountNumber << ";";
	return db->executeQuery(query.str());
}

bool IOLoginData::validRecoveryKey(const std::string& accountName, const std::string& recoveryKey)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `id`, `key` FROM `accounts` WHERE `name` " << db->getStringComparer() << db->escapePatternString(accountName) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	std::string realRecoveryKey = result->getDataString("key");
	db->freeResult(result);
	return realRecoveryKey == recoveryKey;
}

bool IOLoginData::setNewPassword(uint32_t accountId, std::string newPassword)
{
	Database* db = Database::getInstance();

	if(g_config.getNumber(ConfigManager::PASSWORD_TYPE) == PASSWORD_TYPE_MD5)
		newPassword = transformToMD5(newPassword);
	else if(g_config.getNumber(ConfigManager::PASSWORD_TYPE) == PASSWORD_TYPE_SHA1)
		newPassword = transformToSHA1(newPassword);

	DBQuery query;
	query << "UPDATE `accounts` SET `password` = " << db->escapeString(newPassword) << " WHERE `id` = " << accountId << ";";
	return db->executeQuery(query.str());
}

bool IOLoginData::setNewPassword(const std::string& accountName, std::string newPassword)
{
	Database* db = Database::getInstance();

	if(g_config.getNumber(ConfigManager::PASSWORD_TYPE) == PASSWORD_TYPE_MD5)
		newPassword = transformToMD5(newPassword);
	else if(g_config.getNumber(ConfigManager::PASSWORD_TYPE) == PASSWORD_TYPE_SHA1)
		newPassword = transformToSHA1(newPassword);

	DBQuery query;
	query << "UPDATE `accounts` SET `password` = " << db->escapeString(newPassword) << " WHERE `name` " << db->getStringComparer() << db->escapePatternString(accountName) << db->getUpdateLimiter();
	return db->executeQuery(query.str());
}

AccountType_t IOLoginData::getAccountType(std::string name)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;
	query << "SELECT `account_id` FROM `players` WHERE `name` " << db->getStringComparer() << db->escapePatternString(name) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return ACCOUNT_TYPE_NORMAL;

	query.str("");
	query << "SELECT `type` FROM `accounts` WHERE `id` = " << result->getDataInt("account_id") << ";";
	db->freeResult(result);
	if(!(result = db->storeQuery(query.str())))
		return ACCOUNT_TYPE_NORMAL;

	AccountType_t accountType = (AccountType_t)result->getDataInt("type");
	db->freeResult(result);
	return accountType;
}

uint32_t IOLoginData::getLastIPByName(std::string name)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;
	query << "SELECT `lastip` FROM `players` WHERE `name` " << db->getStringComparer() << db->escapePatternString(name) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	uint32_t lastIp = result->getDataInt("lastip");
	db->freeResult(result);
	return lastIp;
}

bool IOLoginData::resetOnlineStatus()
{
	return Database::getInstance()->executeQuery("UPDATE `players` SET `online` = 0;");
}

bool IOLoginData::updateOnlineStatus(uint32_t guid, bool login)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	uint16_t onlineValue = login;
	if(g_config.getBoolean(ConfigManager::ALLOW_CLONES))
	{
		query << "SELECT `online` FROM `players` WHERE `id` = " << guid << ";";
		if(!(result = db->storeQuery(query.str())))
			return false;

		onlineValue = result->getDataInt("online");
		db->freeResult(result);
		if(login)
			onlineValue++;
		else if(onlineValue > 0)
			onlineValue--;

		query.str("");
	}

	query << "UPDATE `players` SET `online` = " << onlineValue << " WHERE `id` = " << guid << ";";
	return db->executeQuery(query.str());
}

bool IOLoginData::loadPlayer(Player* player, const std::string& name, bool preload /*= false*/)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `id`, `account_id`, `group_id`, `sex`, `vocation`, `experience`, `level`, `maglevel`, `health`, `healthmax`, `blessings`, `mana`, `manamax`, `manaspent`, `soul`, `lookbody`, `lookfeet`, `lookhead`, `looklegs`, `looktype`, `lookaddons`, `posx`, `posy`, `posz`, `cap`, `lastlogin`, `lastlogout`, `lastip`, `conditions`, `skulltime`, `skull`, `guildnick`, `rank_id`, `town_id`, `balance`, `offlinetraining_time`, `offlinetraining_skill` FROM `players` WHERE `name` " << db->getStringComparer() << db->escapePatternString(name) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	uint32_t accno = result->getDataInt("account_id");
	if(accno < 1)
		return false;

	Account acc = loadAccount(accno);

	player->setGUID(result->getDataInt("id"));
	player->accountNumber = accno;

	player->accountType = acc.accountType;

	if(g_config.getBoolean(ConfigManager::FREE_PREMIUM))
		player->premiumDays = 0xFFFF;
	else
		player->premiumDays = acc.premiumDays;

	player->setGroupId(result->getDataInt("group_id"));

	if(preload)
	{
		//only loading basic info
		return true;
	}

	player->bankBalance = (uint64_t)result->getDataLong("balance");

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
	player->blessings = result->getDataInt("blessings");

	unsigned long conditionsSize = 0;
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

	player->setVocation(result->getDataInt("vocation"));
	player->mana = result->getDataInt("mana");
	player->manaMax = result->getDataInt("manamax");
	player->magLevel = result->getDataInt("maglevel");

	uint64_t nextManaCount = player->vocation->getReqMana(player->magLevel + 1);
	uint64_t manaSpent = result->getDataLong("manaspent");
	if(manaSpent > nextManaCount)
		manaSpent = 0;

	player->manaSpent = manaSpent;
	player->magLevelPercent = Player::getPercentLevel(player->manaSpent,
		nextManaCount);

	player->health = result->getDataInt("health");
	player->healthMax = result->getDataInt("healthmax");

	if(player->accessLevel)
	{
		if(acc.accountType > 4)
			player->defaultOutfit.lookType = 266;
		else
			player->defaultOutfit.lookType = 75;
	}
	else
		player->defaultOutfit.lookType = result->getDataInt("looktype");

	player->defaultOutfit.lookHead = result->getDataInt("lookhead");
	player->defaultOutfit.lookBody = result->getDataInt("lookbody");
	player->defaultOutfit.lookLegs = result->getDataInt("looklegs");
	player->defaultOutfit.lookFeet = result->getDataInt("lookfeet");
	player->defaultOutfit.lookAddons = result->getDataInt("lookaddons");
	player->currentOutfit = player->defaultOutfit;

	if(g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
	{
		int32_t skullSeconds = result->getDataInt("skulltime") - time(NULL);
		if(skullSeconds > 0)
		{
			//ensure that we round up the number of ticks
			player->skullTicks = (skullSeconds + 2) * 1000;
			int32_t skull = result->getDataInt("skull");
			if(skull == SKULL_RED)
				player->skull = SKULL_RED;
			else if (skull == SKULL_BLACK)
				player->skull = SKULL_BLACK;
		}
	}

	player->loginPosition.x = result->getDataInt("posx");
	player->loginPosition.y = result->getDataInt("posy");
	player->loginPosition.z = result->getDataInt("posz");

	player->lastLoginSaved = result->getDataLong("lastlogin");
	player->lastLogout = result->getDataLong("lastlogout");

	player->offlineTrainingTime = result->getDataInt("offlinetraining_time") * 1000;
	player->offlineTrainingSkill = result->getDataInt("offlinetraining_skill");

	player->town = result->getDataInt("town_id");
	Town* town = Towns::getInstance().getTown(player->town);
	if(town)
		player->masterPos = town->getTemplePosition();
	Position loginPos = player->loginPosition;
	if(loginPos.x == 0 && loginPos.y == 0 && loginPos.z == 0)
		player->loginPosition = player->masterPos;

	uint32_t rankid = result->getDataInt("rank_id");
	if(rankid)
	{
		player->guildNick = result->getDataString("guildnick");
		db->freeResult(result);
		query.str("");
		query << "SELECT `guild_ranks`.`name` AS `rank`, `guild_ranks`.`guild_id` AS `guildid`, `guild_ranks`.`level` AS `level`, `guilds`.`name` AS `guildname` FROM `guild_ranks`, `guilds` WHERE `guild_ranks`.`id` = " << rankid << " AND `guild_ranks`.`guild_id` = `guilds`.`id` LIMIT 1;";
		if((result = db->storeQuery(query.str())))
		{
			player->guildName = result->getDataString("guildname");
			player->guildLevel = result->getDataInt("level");
			player->guildId = result->getDataInt("guildid");
			player->guildRank = result->getDataString("rank");
			player->guildWarList = IOGuild::getInstance()->getWarList(player->guildId);
			db->freeResult(result);
		}
	}
	else if(g_config.getBoolean(ConfigManager::INGAME_GUILD_SYSTEM))
	{
		db->freeResult(result);
		query.str("");
		query << "SELECT `guild_id` FROM `guild_invites` WHERE `player_id` = " << player->getGUID();
		if((result = db->storeQuery(query.str())))
		{
			do
			{
				player->invitedToGuildsList.push_back(result->getDataInt("guild_id"));
			}
			while(result->next());
			db->freeResult(result);
		}
	}
	else
		db->freeResult(result);

	//get password
	query.str("");
	query << "SELECT `password` FROM `accounts` WHERE `id` = " << accno << ";";
	if(!(result = db->storeQuery(query.str())))
		return false;

	player->password = result->getDataString("password");
	db->freeResult(result);

	// we need to find out our skills
	// so we query the skill table
	query.str("");
	query << "SELECT `skillid`, `value`, `count` FROM `player_skills` WHERE `player_id` = " << player->getGUID() << ";";
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
 	query << "SELECT `player_id`, `name` FROM `player_spells` WHERE `player_id` = " << player->getGUID() << ";";
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
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_items` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC;";
	if((result = db->storeQuery(query.str())))
	{
		loadItems(itemMap, result);
		db->freeResult(result);

		ItemMap::reverse_iterator it;
		ItemMap::iterator it2;

		for(it = itemMap.rbegin(); it != itemMap.rend(); ++it)
		{
			std::pair<Item*, int32_t>& pair = it->second;
			Item* item = pair.first;
			int32_t pid = pair.second;
			if(pid >= 1 && pid <= 10)
				player->__internalAddThing(pid, item);
			else
			{
				it2 = itemMap.find(pid);
				if(it2 != itemMap.end())
				{
					if(Container* container = it2->second.first->getContainer())
						container->__internalAddThing(item);
				}
			}
		}
	}

	//load depot items
	itemMap.clear();

	query.str("");
	query << "SELECT `pid`, `sid`, `itemtype`, `count`, `attributes` FROM `player_depotitems` WHERE `player_id` = " << player->getGUID() << " ORDER BY `sid` DESC;";
	DepotMap depotsMap;
	if((result = db->storeQuery(query.str())))
	{
		loadItems(itemMap, result);
		db->freeResult(result);

		ItemMap::reverse_iterator it;
		ItemMap::iterator it2;
		for(it = itemMap.rbegin(); it != itemMap.rend(); ++it)
		{
			Item* item = it->second.first;
			int32_t pid = it->second.second;
			if(pid >= 0 && pid < 100)
			{
				if(Container* c = item->getContainer())
				{
					if(Depot* depot = c->getDepot())
						depotsMap[pid] = depot;
					else
						std::cout << "Error loading depot " << pid << " for player " << player->getGUID() << std::endl;
				}
				else
					std::cout << "Error loading depot " << pid << " for player " << player->getGUID() << std::endl;
			}
			else
			{
				it2 = itemMap.find(pid);
				if(it2 != itemMap.end())
				{
					if(Container* container = it2->second.first->getContainer())
						container->__internalAddThing(item);
				}
			}
		}
	}

	// Add depots and move items from locker to inbox
	for(DepotMap::const_iterator it = depotsMap.begin(), end = depotsMap.end(); it != end; ++it)
	{
		Depot* depot = it->second;
		player->addDepot(depot, it->first);
		if(depot->size() > 3 && depot->getInbox())
		{
			player->depotChange = true;
			ItemList::const_reverse_iterator _it = depot->getReversedItems(), end = depot->getReversedEnd();
			while(_it != end)
			{
				Item* item = (*_it);
				if(item->getID() != ITEM_INBOX && item->getID() != ITEM_MARKET && item->getID() != ITEM_DEPOT)
				{
					g_game.internalMoveItem(item->getParent(), depot->getInbox(), INDEX_WHEREEVER, item, item->getItemCount(), NULL, FLAG_NOLIMIT);

					_it = depot->getReversedItems();
					end = depot->getReversedEnd();
				}
				else
					++_it;
			}
		}
	}

	//load storage map
	query.str("");
	query << "SELECT `key`, `value` FROM `player_storage` WHERE `player_id` = " << player->getGUID() << ";";
	if((result = db->storeQuery(query.str())))
	{
		do
		{
			player->addStorageValue(result->getDataInt("key"), result->getDataLong("value"), true);
		}
		while(result->next());
		db->freeResult(result);
	}

	//load vip
	query.str("");
	query << "SELECT `vip_id` FROM `player_viplist` WHERE `player_id` = " << player->getGUID() << ";";
	if((result = db->storeQuery(query.str())))
	{
		do
		{
			uint32_t vip_id = result->getDataInt("vip_id");
			std::string dummy_str;
			if(storeNameByGuid(*db, vip_id))
				player->addVIP(vip_id, dummy_str, false, true);
		}
		while(result->next());
		db->freeResult(result);
	}

	player->updateBaseSpeed();
	player->updateInventoryWeight();
	player->updateItemsLight(true);
	return true;
}

bool IOLoginData::saveItems(const Player* player, const ItemBlockList& itemList, DBInsert& query_insert)
{
	std::list<Container*> listContainer;
	std::ostringstream stream;

	typedef std::pair<Container*, int32_t> containerBlock;
	std::list<containerBlock> stack;

	int32_t parentId = 0;
	int32_t runningId = 100;

	Database* db = Database::getInstance();
	Item* item;
	int32_t pid;

	for(ItemBlockList::const_iterator it = itemList.begin(); it != itemList.end(); ++it)
	{
		pid = it->first;
		item = it->second;
		++runningId;

		uint32_t attributesSize = 0;

		PropWriteStream propWriteStream;
		item->serializeAttr(propWriteStream);
		const char* attributes = propWriteStream.getStream(attributesSize);

		stream << player->getGUID() << "," << pid << "," << runningId << "," << item->getID() << "," << (int32_t)item->getSubType() << "," << db->escapeBlob(attributes, attributesSize);
		if(!query_insert.addRow(stream))
			return false;

		if(Container* container = item->getContainer())
			stack.push_back(containerBlock(container, runningId));
	}

	while(!stack.empty())
	{
		const containerBlock& cb = stack.front();
		Container* container = cb.first;
		parentId = cb.second;
		stack.pop_front();
		for(ItemList::const_iterator it = container->getItems(), end = container->getEnd(); it != end; ++it)
		{
			++runningId;
			item = *it;
			Container* container = item->getContainer();
			if(container)
				stack.push_back(containerBlock(container, runningId));

			uint32_t attributesSize = 0;
			PropWriteStream propWriteStream;
			item->serializeAttr(propWriteStream);
			const char* attributes = propWriteStream.getStream(attributesSize);

			stream << player->getGUID() << "," << parentId << "," << runningId << "," << item->getID() << "," << (int32_t)item->getSubType() << "," << db->escapeBlob(attributes, attributesSize);
			if(!query_insert.addRow(stream))
				return false;
		}
	}
	return query_insert.execute();
}

bool IOLoginData::savePlayer(Player* player, bool preSave)
{
	if(preSave)
		player->preSave();

	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `save` FROM `players` WHERE `id` = " << player->getGUID() << ";";
	if(!(result = db->storeQuery(query.str())))
		return false;

	if(result->getDataInt("save") == 0)
	{
		db->freeResult(result);
		query.str("");
		query << "UPDATE `players` SET `lastlogin` = " << player->lastLoginSaved << ", `lastip` = " << player->lastIP << " WHERE `id` = " << player->getGUID() << ";";
		return db->executeQuery(query.str());
	}
	db->freeResult(result);

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

	//First, an UPDATE query to write the player itself
	query.str("");
	query << "UPDATE `players` SET ";
	query << "`level` = " << player->level << ", ";
	query << "`group_id` = " << player->groupId << ", ";
	query << "`vocation` = " << (int32_t)player->getVocationId() << ", ";
	query << "`health` = " << player->health << ", ";
	query << "`healthmax` = " << player->healthMax << ", ";
	query << "`experience` = " << player->experience << ", ";
	query << "`lookbody` = " << (int32_t)player->defaultOutfit.lookBody << ", ";
	query << "`lookfeet` = " << (int32_t)player->defaultOutfit.lookFeet << ", ";
	query << "`lookhead` = " << (int32_t)player->defaultOutfit.lookHead << ", ";
	query << "`looklegs` = " << (int32_t)player->defaultOutfit.lookLegs << ", ";
	query << "`looktype` = " << (int32_t)player->defaultOutfit.lookType << ", ";
	query << "`lookaddons` = " << (int32_t)player->defaultOutfit.lookAddons << ", ";
	query << "`maglevel` = " << player->magLevel << ", ";
	query << "`mana` = " << player->mana << ", ";
	query << "`manamax` = " << player->manaMax << ", ";
	query << "`manaspent` = " << player->manaSpent << ", ";
	query << "`soul` = " << player->soul << ", ";
	query << "`town_id` = " << player->town << ", ";

	const Position& loginPosition = player->getLoginPosition();
	query << "`posx` = " << loginPosition.x << ", ";
	query << "`posy` = " << loginPosition.y << ", ";
	query << "`posz` = " << loginPosition.z << ", ";

	query << "`cap` = " << player->getCapacity() << ", ";
	query << "`sex` = " << player->sex << ", ";

	if(player->lastLoginSaved != 0)
		query << "`lastlogin` = " << player->lastLoginSaved << ", ";

	if(player->lastIP != 0)
		query << "`lastip` = " << player->lastIP << ", ";

	query << "`conditions` = " << db->escapeBlob(conditions, conditionsSize) << ", ";

	if(g_game.getWorldType() != WORLD_TYPE_PVP_ENFORCED)
	{
		int32_t skullTime = 0;
		if(player->skullTicks > 0)
			skullTime = time(NULL) + player->skullTicks / 1000;

		query << "`skulltime` = " << skullTime << ", ";
		int32_t skull = 0;
		if(player->skull == SKULL_RED)
			skull = SKULL_RED;
		else if(player->skull == SKULL_BLACK)
			skull = SKULL_BLACK;

		query << "`skull` = " << skull << ", ";
	}
	query << "`lastlogout` = " << player->getLastLogout() << ", ";
	query << "`balance` = " << player->bankBalance << ", ";
	query << "`offlinetraining_time` = " << player->getOfflineTrainingTime() / 1000 << ", ";
	query << "`offlinetraining_skill` = " << player->getOfflineTrainingSkill() << ", ";
	query << "`blessings` = " << player->blessings;
	if(g_config.getBoolean(ConfigManager::INGAME_GUILD_SYSTEM))
	{
		query << ", `guildnick` = " << db->escapeString(player->guildNick) << ", ";
		query << "`rank_id` = " << IOGuild::getInstance()->getRankIdByGuildIdAndLevel(player->getGuildId(), player->getGuildLevel()) << " ";
	}
	query << " WHERE `id` = " << player->getGUID() << ";";

	DBTransaction transaction(db);
	if(!transaction.begin())
		return false;

	if(!db->executeQuery(query.str()))
		return false;

	// skills
	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; i++)
	{
		query.str("");
		query << "UPDATE `player_skills` SET `value` = " << player->skills[i][SKILL_LEVEL] << ", `count` = " << player->skills[i][SKILL_TRIES] << " WHERE `player_id` = " << player->getGUID() << " AND `skillid` = " << i << db->getUpdateLimiter();
		if(!db->executeQuery(query.str()))
			return false;
	}

	// learned spells
	query.str("");
	query << "DELETE FROM `player_spells` WHERE `player_id` = " << player->getGUID() << ";";
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");

	DBInsert stmt(db);
	stmt.setQuery("INSERT INTO `player_spells` (`player_id`, `name` ) VALUES ");
	for(LearnedInstantSpellList::const_iterator it = player->learnedInstantSpellList.begin();
			it != player->learnedInstantSpellList.end(); ++it)
	{
		query << player->getGUID() << "," << db->escapeString(*it);
		if(!stmt.addRow(query))
			return false;
	}

	if(!stmt.execute())
		return false;

	//item saving
	query << "DELETE FROM `player_items` WHERE `player_id` = " << player->getGUID() << ";";
	if(!db->executeQuery(query.str()))
		return false;

	stmt.setQuery("INSERT INTO `player_items` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");

	ItemBlockList itemList;
	Item* item;
	for(int32_t slotId = 1; slotId <= 10; ++slotId)
	{
		if((item = player->inventory[slotId]))
			itemList.push_back(itemBlock(slotId, item));
	}

	if(!saveItems(player, itemList, stmt))
		return false;

	if(player->depotChange)
	{
		//save depot items
		query.str("");
		query << "DELETE FROM `player_depotitems` WHERE `player_id` = " << player->getGUID() << ";";
		if(!db->executeQuery(query.str()))
			return false;

		stmt.setQuery("INSERT INTO `player_depotitems` (`player_id`, `pid`, `sid`, `itemtype`, `count`, `attributes`) VALUES ");
		itemList.clear();
		for(DepotMap::iterator it = player->depots.begin(); it !=player->depots.end() ;++it)
			itemList.push_back(itemBlock(it->first, it->second));

		if(!saveItems(player, itemList, stmt))
			return false;
	}

	query.str("");
	query << "DELETE FROM `player_storage` WHERE `player_id` = " << player->getGUID() << ";";

	if(!db->executeQuery(query.str()))
		return false;

	query.str("");

	stmt.setQuery("INSERT INTO `player_storage` (`player_id`, `key`, `value`) VALUES ");
	player->genReservedStorageRange();
	for(StorageMap::const_iterator cit = player->getStorageIteratorBegin(), end = player->getStorageIteratorEnd(); cit != end; ++cit)
	{
		query << player->getGUID() << "," << cit->first << "," << cit->second;
		if(!stmt.addRow(query))
			return false;
	}

	if(!stmt.execute())
		return false;

	if(g_config.getBoolean(ConfigManager::INGAME_GUILD_SYSTEM))
	{
		//save guild invites
		query << "DELETE FROM `guild_invites` WHERE `player_id` = " << player->getGUID() << ";";
		if(!db->executeQuery(query.str()))
			return false;

		query.str("");

		stmt.setQuery("INSERT INTO `guild_invites` (`player_id`, `guild_id`) VALUES ");
		for(InvitedToGuildsList::const_iterator it = player->invitedToGuildsList.begin(); it != player->invitedToGuildsList.end(); ++it)
		{
			if(IOGuild::getInstance()->guildExists(*it))
			{
				query << player->getGUID() << "," << *it;
				if(!stmt.addRow(query))
					return false;
			}
		}

		if(!stmt.execute())
			return false;
	}

	//save vip list
	query << "DELETE FROM `player_viplist` WHERE `player_id` = " << player->getGUID() << ";";
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");

	stmt.setQuery("INSERT INTO `player_viplist` (`player_id`, `vip_id`) VALUES ");
	for(VIPListSet::iterator it = player->VIPList.begin(), end = player->VIPList.end(); it != end; ++it)
	{
		if(playerExists(*it))
		{
			query << player->getGUID() << "," << *it;
			if(!stmt.addRow(query))
				return false;
		}
	}

	if(!stmt.execute())
		return false;

	//End the transaction
	return transaction.commit();
}

bool IOLoginData::storeNameByGuid(Database &db, uint32_t guid)
{
	DBQuery query;
	DBResult* result;

	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
		return true;

	query << "SELECT `name` FROM `players` WHERE `id` = " << guid << ";";
	if(!(result = db.storeQuery(query.str())))
		return false;

	nameCacheMap[guid] = result->getDataString("name");
	db.freeResult(result);
	return true;
}

bool IOLoginData::getNameByGuid(uint32_t guid, std::string& name)
{
	NameCacheMap::iterator it = nameCacheMap.find(guid);
	if(it != nameCacheMap.end())
	{
		name = it->second;
		return true;
	}

	DBQuery query;
	query << "SELECT `name` FROM `players` WHERE `id` = " << guid << ";";

	Database* db = Database::getInstance();
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	db->freeResult(result);

	nameCacheMap[guid] = name;
	return true;
}

bool IOLoginData::getGuidByName(uint32_t &guid, std::string& name)
{
	GuidCacheMap::iterator it = guidCacheMap.find(name);
	if(it != guidCacheMap.end())
	{
		name = it->first;
		guid = it->second;
		return true;
	}

	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `id`, `name` FROM `players` WHERE `name` " << db->getStringComparer() << db->escapePatternString(name) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	guid = result->getDataInt("id");
	db->freeResult(result);

	guidCacheMap[name] = guid;
	return true;
}

bool IOLoginData::getGuidByNameEx(uint32_t &guid, bool &specialVip, std::string& name)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `name`, `id`, `group_id`, `account_id` FROM `players` WHERE `name` " << db->getStringComparer() << db->escapePatternString(name) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return false;

	name = result->getDataString("name");
	guid = result->getDataInt("id");
	const PlayerGroup* accountGroup = getPlayerGroupByAccount(result->getDataInt("account_id"));
	const PlayerGroup* playerGroup = getPlayerGroup(result->getDataInt("group_id"));
	db->freeResult(result);

	uint64_t flags = 0;
	if(playerGroup)
		flags |= playerGroup->m_flags;

	if(accountGroup)
		flags |= accountGroup->m_flags;

	specialVip = (0 != (flags & ((uint64_t)1 << PlayerFlag_SpecialVIP)));
	return true;
}

bool IOLoginData::playerExists(std::string& name)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `name` FROM `players` WHERE `name` " << db->getStringComparer() << db->escapePatternString(name) << " LIMIT 1;";
	if((result = db->storeQuery(query.str())))
	{
		name = result->getDataString("name");
		db->freeResult(result);
		return true;
	}
	return false;
}

bool IOLoginData::playerExists(uint32_t guid)
{
	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `id` FROM `players` WHERE `id` = " << guid << ";";
	if(!(result = db->storeQuery(query.str())))
		return false;

	db->freeResult(result);
	return true;
}

const PlayerGroup* IOLoginData::getPlayerGroup(uint32_t groupid)
{
	PlayerGroupMap::const_iterator it = playerGroupMap.find(groupid);
	if(it != playerGroupMap.end())
		return it->second;

	Database* db = Database::getInstance();

	DBQuery query;
	DBResult* result;

	query << "SELECT `name`, `flags`, `access`, `maxdepotitems`, `maxviplist` FROM `groups` WHERE `id` = " << groupid << ";";
	if(!(result = db->storeQuery(query.str())))
		return NULL;

	PlayerGroup* group = new PlayerGroup;
	group->m_name = result->getDataString("name");
	group->m_flags = result->getDataLong("flags");
	group->m_access = result->getDataInt("access");
	group->m_maxdepotitems = result->getDataInt("maxdepotitems");
	group->m_maxviplist = result->getDataInt("maxviplist");
	db->freeResult(result);

	playerGroupMap[groupid] = group;
	return group;
}

const PlayerGroup* IOLoginData::getPlayerGroupByAccount(uint32_t accno)
{
	Database* db = Database::getInstance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `group_id` FROM `accounts` WHERE `id` = " << accno << ";";
	if(!(result = db->storeQuery(query.str())))
		return NULL;

	uint32_t groupId = result->getDataInt("group_id");
	db->freeResult(result);
	return getPlayerGroup(groupId);
}

void IOLoginData::loadItems(ItemMap& itemMap, DBResult* result)
{
	do
	{
		int32_t sid = result->getDataInt("sid");
		int32_t pid = result->getDataInt("pid");
		int32_t type = result->getDataInt("itemtype");
		int32_t count = result->getDataInt("count");

		unsigned long attrSize = 0;
		const char* attr = result->getDataStream("attributes", attrSize);

		PropStream propStream;
		propStream.init(attr, attrSize);

		Item* item = Item::CreateItem(type, count);
		if(item)
		{
			if(!item->unserializeAttr(propStream))
				std::cout << "WARNING: Serialize error in IOLoginData::loadItems" << std::endl;

			std::pair<Item*, int32_t> pair(item, pid);
			itemMap[sid] = pair;
		}
	}
	while(result->next());
}

int32_t IOLoginData::getLevel(uint32_t guid)
{
	Database* db = Database::getInstance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `level` FROM `players` WHERE `id` = " << guid << ";";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	int32_t level = result->getDataInt("level");
	db->freeResult(result);
	return level;
}

bool IOLoginData::isPremium(uint32_t guid)
{
	if(g_config.getBoolean(ConfigManager::FREE_PREMIUM))
		return true;

	Database* db = Database::getInstance();
	DBQuery query;
	DBResult* result;

	query << "SELECT `account_id` FROM `players` WHERE `id` = " << guid << ";";
	if(!(result = db->storeQuery(query.str())))
		return false;

	query.str("");
	query << "SELECT `premdays` FROM `accounts` WHERE `id` = " << result->getDataInt("account_id") << ";";
	db->freeResult(result);
	if(!(result = db->storeQuery(query.str())))
		return false;

	bool isPremium = result->getDataInt("premdays") > 0;
	db->freeResult(result);
	return isPremium;
}

bool IOLoginData::leaveGuild(uint32_t guid)
{
	DBQuery query;
	query << "UPDATE `players` SET `rank_id` = 0, `guildnick` = '' WHERE `id` = " << guid << ";";
	return Database::getInstance()->executeQuery(query.str());
}

bool IOLoginData::changeName(uint32_t guid, std::string newName)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `players` SET `name` = " << db->escapeString(newName) << " WHERE `id` = " << guid << ";";
	if(!db->executeQuery(query.str()))
		return false;

	nameCacheMap[guid] = newName;
	return true;
}

uint32_t IOLoginData::getAccountNumberByName(std::string name)
{
	Database* db = Database::getInstance();
	DBQuery query;
	DBResult* result;
	query << "SELECT `account_id` FROM `players` WHERE `name` " << db->getStringComparer() << db->escapePatternString(name) << " LIMIT 1;";
	if(!(result = db->storeQuery(query.str())))
		return 0;

	uint32_t accountId = result->getDataInt("account_id");
	db->freeResult(result);
	return accountId;
}

bool IOLoginData::createCharacter(uint32_t accountNumber, std::string characterName, int32_t vocationId, PlayerSex_t sex)
{
	if(playerExists(characterName))
		return false;

	Vocation* vocation = g_vocations.getVocation(vocationId);
	Vocation* rookVoc = g_vocations.getVocation(VOCATION_NONE);
	uint16_t healthMax = 150, manaMax = 0, capMax = 400, lookType = 136;
	if(sex == PLAYERSEX_MALE)
		lookType = 128;

	uint64_t exp = 0;
	const uint32_t level = g_config.getNumber(ConfigManager::START_LEVEL);
	if(level > 1)
		exp = Player::getExpForLevel(level);

	uint32_t tmpLevel = std::min((uint32_t)7, level - 1);
	if(tmpLevel > 0)
	{
		healthMax += rookVoc->getHPGain() * tmpLevel;
		manaMax += rookVoc->getManaGain() * tmpLevel;
		capMax += rookVoc->getCapGain() * tmpLevel;
		if(level > 8)
		{
			tmpLevel = level - 8;

			healthMax += vocation->getHPGain() * tmpLevel;
			manaMax += vocation->getManaGain() * tmpLevel;
			capMax += vocation->getCapGain() * tmpLevel;
		}
	}

	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `players` (`id`, `name`, `group_id`, `account_id`, `level`, `vocation`, `health`, `healthmax`, `experience`, `lookbody`, `lookfeet`, `lookhead`, `looklegs`, `looktype`, `lookaddons`, `maglevel`, `mana`, `manamax`, `manaspent`, `soul`, `town_id`, `posx`, `posy`, `posz`, `conditions`, `cap`, `sex`, `lastlogin`, `lastip`, `skull`, `skulltime`, `save`, `rank_id`, `guildnick`, `lastlogout`, `blessings`, `online`) VALUES (NULL, " << db->escapeString(characterName) << ", 1, " << accountNumber << ", " << level << ", " << vocationId << ", " << healthMax << ", " << healthMax << ", " << exp << ", 68, 76, 78, 39, " << lookType << ", 0, " << g_config.getNumber(ConfigManager::START_MAGICLEVEL) << ", " << manaMax << ", " << manaMax << ", 0, 100, " << g_config.getNumber(ConfigManager::SPAWNTOWN_ID) << ", " << g_config.getNumber(ConfigManager::SPAWNPOS_X) << ", " << g_config.getNumber(ConfigManager::SPAWNPOS_Y) << ", " << g_config.getNumber(ConfigManager::SPAWNPOS_Z) << ", 0, " << capMax << ", " << sex << ", 0, 0, 0, 0, 1, 0, '', 0, 0, 0);";
	return db->executeQuery(query.str());
}

int16_t IOLoginData::deleteCharacter(uint32_t accountNumber, const std::string& characterName)
{
	Player* _player = g_game.getPlayerByName(characterName);
	if(_player)
		return 4;

	Database* db = Database::getInstance();
	DBQuery query;
	query << "SELECT `id` FROM `players` WHERE `name` " << db->getStringComparer() << db->escapePatternString(characterName) << " AND `account_id` = " << accountNumber << " LIMIT 1;";
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return 0;

	uint32_t id = result->getDataInt("id");
	db->freeResult(result);
	if(IOGuild::getInstance()->getGuildLevel(id) == 3)
		return 3;

	House* house = Houses::getInstance().getHouseByPlayerId(id);
	if(house)
		return 2;

	query.str("");
	query << "DELETE FROM `players` WHERE `id` = " << id << ";";
	if(!db->executeQuery(query.str()))
		return 0;

	query.str("");
	query << "DELETE FROM `player_items` WHERE `player_id` = " << id << ";";
	db->executeQuery(query.str());
	query.str("");
	query << "DELETE FROM `player_depotitems` WHERE `player_id` = " << id << ";";
	db->executeQuery(query.str());
	query.str("");
	query << "DELETE FROM `player_storage` WHERE `player_id` = " << id << ";";
	db->executeQuery(query.str());
	query.str("");
	query << "DELETE FROM `player_skills` WHERE `player_id` = " << id << ";";
	db->executeQuery(query.str());
	query.str("");
	query << "DELETE FROM `player_viplist` WHERE `player_id` = " << id << ";";
	db->executeQuery(query.str());
	query.str("");
	query << "DELETE FROM `player_viplist` WHERE `vip_id` = " << id << ";";
	db->executeQuery(query.str());
	query.str("");
	query << "DELETE FROM `guild_invites` WHERE `player_id` = " << id << ";";
	db->executeQuery(query.str());
	return 1;
}

bool IOLoginData::addStorageValue(uint32_t guid, uint32_t storageKey, uint32_t storageValue)
{
	DBQuery query;
	query << "SELECT `key` FROM `player_storage` WHERE `player_id` = " << guid << " AND `key` = " << storageKey << " LIMIT 1;";

	Database* db = Database::getInstance();
	DBResult* result;
	if((result = db->storeQuery(query.str())))
	{
		db->freeResult(result);
		query.str("");
		query << "UPDATE `player_storage` SET `value` = " << storageValue << " WHERE `player_id` = " << guid << " AND `key` = " << storageKey << db->getUpdateLimiter();
	}
	else
	{
		query.str("");
		query << "INSERT INTO `player_storage` (`player_id`, `key`, `value`) VALUES (" << guid << ", " << storageKey << ", " << storageValue << ");";
	}
	return db->executeQuery(query.str());
}

bool IOLoginData::hasGuild(uint32_t guid)
{
	DBQuery query;
	query << "SELECT `rank_id` FROM `players` WHERE `id` = " << guid << ";";

	Database* db = Database::getInstance();
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return false;

	bool hasGuild = result->getDataInt("rank_id") != 0;
	db->freeResult(result);
	return hasGuild;
}

void IOLoginData::increaseBankBalance(uint32_t guid, uint64_t bankBalance)
{
	DBQuery query;
	query << "UPDATE `players` SET `balance` = `balance` + " << bankBalance << " WHERE `id` = " << guid << ";";
	Database::getInstance()->executeQuery(query.str());
}
