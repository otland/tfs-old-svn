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

#include "ioguild.h"
#include "database.h"

#include "configmanager.h"
#include "game.h"

extern Game g_game;
extern ConfigManager g_config;

bool IOGuild::getGuildIdByName(uint32_t& guildId, const std::string& guildName)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `guilds` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(guildName) << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!(result = db->storeQuery(query.str())))
		return false;

	guildId = result->getDataInt("id");
	result->free();
	return true;
}

bool IOGuild::guildExists(uint32_t guildId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `guilds` WHERE `id` = " << guildId << " AND `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!(result = db->storeQuery(query.str())))
		return false;

	result->free();
	return true;
}

bool IOGuild::getRankIdByGuildIdAndName(uint32_t &rankId, const std::string& rankName, uint32_t& guildId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `guild_ranks` WHERE `guild_id` = " << guildId << " AND `name` " << db->getStringComparisonOperator() << " " << db->escapeString(rankName);
	if(!(result = db->storeQuery(query.str())))
		return false;

	rankId = result->getDataInt("id");
	result->free();
	return true;
}

uint32_t IOGuild::getRankIdByGuildIdAndLevel(uint32_t guildId, uint32_t guildLevel)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `guild_ranks` WHERE `level` = " << guildLevel << " AND `guild_id` = " << guildId;
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t id = result->getDataInt("id");
	result->free();
	return id;
}

std::string IOGuild::getRankName(int16_t guildLevel, uint32_t guildId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `name` FROM `guild_ranks` WHERE `level` = " << guildLevel << " AND `guild_id` = " << guildId;
	if(!(result = db->storeQuery(query.str())))
		return "";

	const std::string name = result->getDataString("name");
	result->free();
	return name;
}

bool IOGuild::rankNameExists(std::string rankName, uint32_t guildId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `guild_ranks` WHERE `guild_id` = " << guildId << " AND `name` " << db->getStringComparisonOperator() << " " << db->escapeString(rankName);
	if(!(result = db->storeQuery(query.str())))
		return false;

	result->free();
	return true;
}

bool IOGuild::changeRankName(std::string oldRankName, std::string newRankName, uint32_t guildId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `guild_ranks` WHERE `name` " << db->getStringComparisonOperator() << " " << db->escapeString(oldRankName) << " AND `guild_id` = " << guildId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t rankId = result->getDataInt("id");
	result->free();

	query.str("");
	query << "UPDATE `guild_ranks` SET `name` = " << db->escapeString(newRankName) << " WHERE `id` = " << rankId << " AND `guild_id` = " << guildId;
	if(!db->executeQuery(query.str()))
		return false;

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		if((*it).second->getGuildId() == guildId && (*it).second->getGuildRankId() == rankId)
			(*it).second->setGuildRank(newRankName);
	}

	return false;
}

bool IOGuild::createGuild(Player* player)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "INSERT INTO `guilds` (`id`, `world_id`, `name`, `ownerid`, `creationdata`, `motd`) VALUES (NULL, " << g_config.getNumber(ConfigManager::WORLD_ID) << ", " << db->escapeString(player->getGuildName()) << ", " << player->getGUID() << ", " << time(NULL) << ", 'Your guild has been successfully created, to view all available commands type: !commands. If you would like to remove this message use !cleanmotd and to set new motd use !setmotd text.');";
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");
	query << "SELECT `id` FROM `guilds` WHERE `ownerid` = " << player->getGUID();
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t guildId = result->getDataInt("id");
	result->free();
	return joinGuild(player, guildId, true);
}

bool IOGuild::joinGuild(Player* player, uint32_t guildId, bool creation/* = false*/)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id`, `name` FROM `guild_ranks` WHERE `guild_id` = " << guildId << " AND `level` = " << (creation ? "3" : "1");
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t rankId = result->getDataInt("id");
	const std::string rankName = result->getDataString("name");
	result->free();

	std::string guildName;
	if(!creation)
	{
		query.str("");
		query << "SELECT `name` as `guildname` FROM `guilds` WHERE `id` = " << guildId;
		if(!(result = db->storeQuery(query.str())))
			return false;

		guildName = result->getDataString("guildname");
		result->free();
	}

	query.str("");
	query << "UPDATE `players` SET `rank_id` = " << rankId << " WHERE `id` = " << player->getGUID() << ";";
	if(!db->executeQuery(query.str()))
		return false;

	player->setGuildId(guildId);
	GuildLevel_t level = GUILDLEVEL_MEMBER;
	if(!creation)
		player->setGuildName(guildName);
	else
		level = GUILDLEVEL_LEADER;

	player->setGuildLevel(level);
	player->setGuildRankId(rankId);
	player->setGuildRank(rankName);

	player->invitedToGuildsList.clear();
	return true;
}

bool IOGuild::disbandGuild(uint32_t guildId)
{
	Database* db = Database::getInstance();

	DBQuery query;
	query << "UPDATE `players` SET `rank_id` = '' AND `guildnick` = '' WHERE `rank_id` = " << getRankIdByGuildIdAndLevel(guildId, 3) << " OR rank_id = " << getRankIdByGuildIdAndLevel(guildId, 2) << " OR rank_id = " << getRankIdByGuildIdAndLevel(guildId, 1);
	if(!db->executeQuery(query.str()))
		return false;

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		if((*it).second->getGuildId() == guildId)
			(*it).second->resetGuildInformation();
	}

	query.str("");
	query << "DELETE FROM `guilds` WHERE `id` = " << guildId;
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");
	query << "DELETE FROM `guild_invites` WHERE `guild_id` = " << guildId;
	if(!db->executeQuery(query.str()))
		return false;

	query.str("");
	query << "DELETE FROM `guild_ranks` WHERE `guild_id` = " << guildId;
	return db->executeQuery(query.str());
}

bool IOGuild::hasGuild(uint32_t guid)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `rank_id` FROM `players` WHERE `id` = " << guid;
	if(!(result = db->storeQuery(query.str())))
		return false;

	const uint32_t rankId = result->getDataInt("rank_id");
	result->free();
	return (rankId != 0);
}

bool IOGuild::isInvitedToGuild(uint32_t guid, uint32_t guildId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `player_id`, `guild_id` FROM `guild_invites` WHERE `player_id` = " << guid << " AND `guild_id`= " << guildId;
	if(!(result = db->storeQuery(query.str())))
		return false;

	result->free();
	return true;
}

bool IOGuild::invitePlayerToGuild(uint32_t guid, uint32_t guildId)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "INSERT INTO `guild_invites` (`player_id`, `guild_id`) VALUES ('" << guid << "', '" << guildId << "');";
	return db->executeQuery(query.str());
}

bool IOGuild::revokeGuildInvite(uint32_t guid, uint32_t guildId)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "DELETE FROM `guild_invites` WHERE `player_id` = " << guid << " AND `guild_id` = " << guildId;
	return db->executeQuery(query.str());
}

uint32_t IOGuild::getGuildId(uint32_t guid)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `guild_ranks`.`guild_id` FROM `players`, `guild_ranks` WHERE `guild_ranks`.`id`=`players`.`rank_id` AND `players`.`id` = " << guid;
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t guildId = result->getDataInt("guild_id");
	result->free();
	return guildId;
}

int8_t IOGuild::getGuildLevel(uint32_t guid)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `guild_ranks`.`level` FROM `players`, `guild_ranks` WHERE `guild_ranks`.`id`=`players`.`rank_id` AND `players`.`id` = " << guid;
	if(!(result = db->storeQuery(query.str())))
		return 0;

	const uint32_t guildLevel = result->getDataInt("level");
	result->free();
	return guildLevel;
}

bool IOGuild::setGuildLevel(uint32_t guid, GuildLevel_t level)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `id` FROM `guild_ranks` WHERE `guild_id` = " << getGuildId(guid) << " AND `level` = " << level;
	if(!(result = db->storeQuery(query.str())))
		return false;

	query.str("");
	query << "UPDATE `players` SET `rank_id` = " << result->getDataInt("id") << " WHERE `id` = " << guid;
	result->free();
	return db->executeQuery(query.str());
}

bool IOGuild::updateOwnerId(uint32_t guildId, uint32_t guid)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `guilds` SET `ownerid` = " << guid << " WHERE `id` = " << guildId;
	return db->executeQuery(query.str());
}

bool IOGuild::setGuildNick(uint32_t guid, std::string guildNick)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `players` SET `guildnick` = " << db->escapeString(guildNick) << " WHERE `id` = " << guid;
	return db->executeQuery(query.str());
}

bool IOGuild::setMotd(uint32_t guildId, std::string newMotd)
{
	Database* db = Database::getInstance();
	DBQuery query;
	query << "UPDATE `guilds` SET `motd` = " << db->escapeString(newMotd) << " WHERE `id` = " << guildId;
	return db->executeQuery(query.str());
}

std::string IOGuild::getMotd(uint32_t guildId)
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `motd` FROM `guilds` WHERE `id` = " << guildId;
	if(!(result = db->storeQuery(query.str())))
		return "";

	const std::string motd = result->getDataString("motd");
	result->free();
	return motd;
}
