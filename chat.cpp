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
#include "otpch.h"

#include "chat.h"
#include "configmanager.h"
#include "player.h"
#include "game.h"
#include "iologindata.h"

extern ConfigManager g_config;
extern Game g_game;
extern Chat g_chat;

PrivateChatChannel::PrivateChatChannel(uint16_t channelId, std::string channelName) :
	ChatChannel(channelId, channelName)
{
	m_owner = 0;
}

bool PrivateChatChannel::isInvited(const Player* player)
{
	if(!player)
		return false;

	if(player->getGUID() == getOwner())
		return true;

	InvitedMap::iterator it = m_invites.find(player->getGUID());
	return it != m_invites.end();
}

bool PrivateChatChannel::addInvited(Player* player)
{
	InvitedMap::iterator it = m_invites.find(player->getGUID());
	if(it != m_invites.end())
		return false;

	m_invites[player->getGUID()] = player;
	return true;
}

bool PrivateChatChannel::removeInvited(Player* player)
{
	InvitedMap::iterator it = m_invites.find(player->getGUID());
	if(it == m_invites.end())
		return false;

	m_invites.erase(it);
	return true;
}

void PrivateChatChannel::invitePlayer(Player* player, Player* invitePlayer)
{
	if(player != invitePlayer && addInvited(invitePlayer))
	{
		std::string msg = player->getName();
		msg += " invites you to ";
		msg += (player->getSex() == PLAYERSEX_FEMALE ? "her" : "his");
		msg += " private chat channel.";
		invitePlayer->sendTextMessage(MSG_INFO_DESCR, msg.c_str());

		msg = invitePlayer->getName();
		msg += " has been invited.";
		player->sendTextMessage(MSG_INFO_DESCR, msg.c_str());
	}
}

void PrivateChatChannel::excludePlayer(Player* player, Player* excludePlayer)
{
	if(player != excludePlayer && removeInvited(excludePlayer))
	{
		removeUser(excludePlayer);

		std::string msg = excludePlayer->getName();
		msg += " has been excluded.";
		player->sendTextMessage(MSG_INFO_DESCR, msg.c_str());

		excludePlayer->sendClosePrivate(getId());
	}
}

void PrivateChatChannel::closeChannel()
{
	UsersMap::iterator cit;
	for(cit = m_users.begin(); cit != m_users.end(); ++cit)
	{
		Player* toPlayer = cit->second->getPlayer();
		if(toPlayer)
			toPlayer->sendClosePrivate(getId());
	}
}

ChatChannel::ChatChannel(uint16_t channelId, std::string channelName)
{
	m_id = channelId;
	m_name = channelName;
}

bool ChatChannel::addUser(Player* player)
{
	if(m_users.find(player->getID()) != m_users.end())
		return false;

	if(m_id == 0x00 && IOGuild::getInstance()->getMotd(player->getGuildId()).length())
	{
		uint32_t playerId = player->getID();
		uint32_t guildId = player->getGuildId();
		g_scheduler.addEvent(createSchedulerTask(150, boost::bind(
			&Game::sendGuildMotd, &g_game, playerId, guildId)));
	}

	if(m_id == 0x03 && !g_config.getBoolean(ConfigManager::ENABLE_RULE_VIOLATION_REPORTS))
	{
		uint32_t playerId = player->getID();
		g_scheduler.addEvent(createSchedulerTask(150, boost::bind(
			&Game::sendRVRDisabled, &g_game, playerId)));
	}

	m_users[player->getID()] = player;
	return true;
}

bool ChatChannel::removeUser(Player* player)
{
	UsersMap::iterator it = m_users.find(player->getID());
	if(it == m_users.end())
		return false;

	m_users.erase(it);
	return true;
}

bool ChatChannel::talk(Player* fromPlayer, SpeakClasses type, const std::string& text, uint32_t time /*= 0*/)
{
	if(m_id != 0x03 && m_users.find(fromPlayer->getID()) == m_users.end())
		return false;

	if(!fromPlayer->hasFlag(PlayerFlag_CannotBeMuted) && (m_id == 0x05 || m_id == 0x07))
	{
		Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_TRADETICKS, 120000, 0);
		fromPlayer->addCondition(condition);
	}

	for(UsersMap::iterator it = m_users.begin(); it != m_users.end(); ++it)
		it->second->sendToChannel(fromPlayer, type, text, getId(), time);

	return true;
}

Chat::Chat()
{
	// Create the default channels
	ChatChannel *newChannel;

	newChannel = new ChatChannel(0x01, "Gamemaster");
	if(newChannel)
		m_normalChannels[0x01] = newChannel;

	newChannel = new ChatChannel(0x02, "Tutor");
	if(newChannel)
		m_normalChannels[0x02] = newChannel;

	newChannel = new ChatChannel(0x03, "Rule Violations");
	if(newChannel)
		m_normalChannels[0x03] = newChannel;

	newChannel = new ChatChannel(0x04, "Game-Chat");
	if(newChannel)
		m_normalChannels[0x04] = newChannel;

	newChannel = new ChatChannel(0x05, "Trade");
	if(newChannel)
		m_normalChannels[0x05] = newChannel;

	newChannel = new ChatChannel(0x06, "RL-Chat");
	if(newChannel)
		m_normalChannels[0x06] = newChannel;

	newChannel = new ChatChannel(0x07, "Trade-Rookgaard");
	if(newChannel)
		m_normalChannels[0x07] = newChannel;

	newChannel = new ChatChannel(0x09, "Help");
	if(newChannel)
		m_normalChannels[0x09] = newChannel;

	newChannel = new PrivateChatChannel(0xFFFF, "Private Chat Channel");
	if(newChannel)
		dummyPrivate = newChannel;
}

Chat::~Chat()
{
	for(NormalChannelMap::iterator it = m_normalChannels.begin(); it != m_normalChannels.end(); ++it)
		delete it->second;

	m_normalChannels.clear();
	for(GuildChannelMap::iterator it = m_guildChannels.begin(); it != m_guildChannels.end(); ++it)
		delete it->second;

	m_guildChannels.clear();
	for(PartyChannelMap::iterator it = m_partyChannels.begin(); it != m_partyChannels.end(); ++it)
		delete it->second;

	m_partyChannels.clear();
	for(PrivateChannelMap::iterator it = m_privateChannels.begin(); it != m_privateChannels.end(); ++it)
		delete it->second;

	m_privateChannels.clear();
	delete dummyPrivate;
}

ChatChannel* Chat::createChannel(Player* player, uint16_t channelId)
{
	if(getChannel(player, channelId))
		return NULL;

	switch(channelId)
	{
		case 0x00:
		{
			ChatChannel* newChannel = NULL;
			if((newChannel = new ChatChannel(channelId, player->getGuildName())))
				m_guildChannels[player->getGuildId()] = newChannel;

			return newChannel;
		}

		case 0x08:
		{
			ChatChannel* newChannel = NULL;
			if(player->getParty() && (newChannel = new ChatChannel(channelId, "Party")))
				m_partyChannels[player->getParty()] = newChannel;

			return newChannel;
		}

		case 0xFFFF:
		{
			//only 1 private channel for each premium player
			if(!player->isPremium() || getPrivateChannel(player))
				return NULL;

			//find a free private channel slot
			for(uint16_t i = 100; i < 10000; ++i)
			{
				if(m_privateChannels.find(i) == m_privateChannels.end())
				{
					PrivateChatChannel* newChannel = NULL;
					if((newChannel = new PrivateChatChannel(i, player->getName() + "'s Channel")))
					{
						newChannel->setOwner(player->getGUID());
						m_privateChannels[i] = newChannel;
					}

					return newChannel;
				}
			}
		}

		default:
			break;
	}

	return NULL;
}

bool Chat::deleteChannel(Player* player, uint16_t channelId)
{
	switch(channelId)
	{
		case 0x00:
		{
			GuildChannelMap::iterator it = m_guildChannels.find(player->getGuildId());
			if(it == m_guildChannels.end())
				return false;

			delete it->second;
			m_guildChannels.erase(it);
			return true;
		}

		case 0x08:
		{
			PartyChannelMap::iterator it = m_partyChannels.find(player->getParty());
			if(it == m_partyChannels.end())
				return false;

			delete it->second;
			m_partyChannels.erase(it);
			return true;
		}

		default:
		{
			PrivateChannelMap::iterator it = m_privateChannels.find(channelId);
			if(it == m_privateChannels.end())
				return false;

			it->second->closeChannel();

			delete it->second;
			m_privateChannels.erase(it);
			return true;
		}
	}
	return false;
}

ChatChannel* Chat::addUserToChannel(Player* player, uint16_t channelId)
{
	ChatChannel* channel = getChannel(player, channelId);
	if(channel && channel->addUser(player))
		return channel;

	return NULL;
}

bool Chat::removeUserFromChannel(Player* player, uint16_t channelId)
{
	ChatChannel* channel = getChannel(player, channelId);
	if(channel && channel->removeUser(player))
	{
		if(channel->getOwner() == player->getGUID())
			deleteChannel(player, channelId);

		return true;
	}
	return false;
}

void Chat::removeUserFromAllChannels(Player* player)
{
	for(NormalChannelMap::iterator it = m_normalChannels.begin(); it != m_normalChannels.end(); ++it)
		it->second->removeUser(player);

	for(PartyChannelMap::iterator it = m_partyChannels.begin(); it != m_partyChannels.end(); ++it)
		it->second->removeUser(player);

	for(GuildChannelMap::iterator it = m_guildChannels.begin(); it != m_guildChannels.end(); ++it)
		it->second->removeUser(player);

	for(PrivateChannelMap::iterator it = m_privateChannels.begin(); it != m_privateChannels.end(); ++it)
	{
		it->second->removeUser(player);
		if(it->second->getOwner() == player->getGUID())
			deleteChannel(player, it->second->getId());
	}
}

bool Chat::talkToChannel(Player* player, SpeakClasses type, const std::string& text, uint16_t channelId)
{
	ChatChannel* channel = getChannel(player, channelId);
	if(!channel || !player)
		return false;

	if(player->getAccountType() < ACCOUNT_TYPE_GAMEMASTER)
	{
		if(player->hasCondition(CONDITION_TRADETICKS) && (channelId == 0x05 || channelId == 0x07))
		{
			player->sendCancel("You may only place one offer in two minutes.");
			return false;
		}
		else if(player->getLevel() < 2 && channelId < 0x08)
		{
			player->sendCancel("You may not speak into channels as long as you are on level 1.");
			return false;
		}
	}

	if(channelId == 0x00 && g_config.getBoolean(ConfigManager::INGAME_GUILD_SYSTEM))
	{
		if(text == "!disband" || text == "!guildonline" || text.substr(0, 7) == "!invite" || text == "!leave"
			|| text.substr(0, 5) == "!kick" || text.substr(0, 7) == "!revoke" || text.substr(0, 7) == "!demote"
			|| text.substr(0, 8) == "!promote" || text.substr(0, 15) == "!passleadership" || text.substr(0, 5) == "!nick"
			|| text.substr(0, 12) == "!setrankname" || text.substr(0, 8) == "!setmotd" || text == "!cleanmotd" || text == "!commands")
		{
			if(!player->getGuildId() || !IOGuild::getInstance()->guildExists(player->getGuildId()))
			{
				player->sendCancel("You are not in a guild.");
				return true;
			}

			ChatChannel* guildChannel = getChannel(player, 0x00);
			if(!guildChannel)
				return false;

			if(text == "!disband")
			{
				if(player->getGuildLevel() != GUILDLEVEL_LEADER)
				{
					player->sendCancel("You are not the leader of your guild.");
					return true;
				}

				uint32_t guildId = player->getGuildId();
				guildChannel->talk(player, SPEAK_CHANNEL_R2, "The guild has been disbanded.");
				IOGuild::getInstance()->disbandGuild(guildId);
			}
			else if(text == "!guildonline")
			{
				std::stringstream ss;
				ss << "Players online in your guild:" << std::endl;
				uint32_t i = 0;
				AutoList<Player>::listiterator it = Player::listPlayer.list.begin();
				while(it != Player::listPlayer.list.end())
				{
					if((*it).second->getGuildId() == player->getGuildId())
					{
						ss << (i > 0 ? ", " : "") << (*it).second->getName() << " [" << (*it).second->getLevel() << "]";
						i++;
					}
					it++;

					if(i == 10)
					{
						ss << (it != Player::listPlayer.list.end() ? "," : ".");
						player->sendToChannel(player, SPEAK_CHANNEL_R2, ss.str(), 0x00);
						ss.str();
						i = 0;
					}
				}

				if(i > 0)
				{
					ss << ".";
					player->sendToChannel(player, SPEAK_CHANNEL_R2, ss.str(), 0x00);
				}
			}
			else if(text.substr(0, 7) == "!invite")
			{
				if(player->getGuildLevel() == GUILDLEVEL_MEMBER)
				{
					player->sendCancel("You don't have rights to invite players to your guild.");
					return true;
				}

				if(text.length() <= 8)
				{
					player->sendCancel("Invalid guildcommand parameters.");
					return true;
				}

				std::string param = text.substr(8);
				trimString(param);
				Player* paramPlayer = g_game.getPlayerByName(param);
				if(paramPlayer)
				{
					if(paramPlayer->getGuildId())
					{
						player->sendCancel("A player with that name is already in a guild.");
						return true;
					}

					if(paramPlayer->isInvitedToGuild(player->getGuildId()))
					{
						player->sendCancel("A player with that name has already been invited to your guild.");
						return true;
					}

					std::stringstream ss;
					ss << player->getName() << " has invited you to join the guild, " << player->getGuildName() << ".";
					paramPlayer->sendTextMessage(MSG_INFO_DESCR, ss.str());

					ss.str("");
					ss << player->getName() << " has invited " << paramPlayer->getName() << " to the guild.";
					guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
					paramPlayer->invitedToGuildsList.push_back(player->getGuildId());
				}
				else if(IOLoginData::getInstance()->playerExists(param))
				{
					uint32_t guid;
					IOLoginData::getInstance()->getGuidByName(guid, param);
					if(IOLoginData::getInstance()->hasGuild(guid))
					{
						player->sendCancel("A player with that name is already in a guild.");
						return true;
					}

					if(IOGuild::getInstance()->isInvitedToGuild(guid, player->getGuildId()))
					{
						player->sendCancel("A player with that name has already been invited to your guild.");
						return true;
					}

					IOGuild::getInstance()->invitePlayerToGuild(guid, player->getGuildId());

					std::stringstream ss;
					ss << player->getName() << " has invited " << param << " to the guild.";
					guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
				}
				else
					player->sendCancel("A player with that name does not exist.");
			}
			else if(text == "!leave")
			{
				if(player->getGuildLevel() == GUILDLEVEL_LEADER)
				{
					player->sendCancel("You have to pass the leadership to another member of your guild or disband the guild to get out of it.");
					return true;
				}

				std::stringstream ss;
				ss << player->getName() << " has left the guild.";
				guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());

				player->leaveGuild();
			}
			else if(text.substr(0, 7) == "!revoke")
			{
				if(player->getGuildLevel() == GUILDLEVEL_MEMBER)
				{
					player->sendCancel("You don't have rights to revoke an invite of someone in your guild.");
					return true;
				}

				if(text.length() <= 8)
				{
					player->sendCancel("Invalid guildcommand parameters.");
					return true;
				}

				std::string param = text.substr(8);
				trimString(param);
				Player* paramPlayer = g_game.getPlayerByName(param);
				if(paramPlayer)
				{
					if(paramPlayer->getGuildId())
					{
						player->sendCancel("A player with that name is already in a guild.");
						return true;
					}

					InvitedToGuildsList::iterator it = std::find(paramPlayer->invitedToGuildsList.begin(),paramPlayer->invitedToGuildsList.end(), player->getGuildId());
					if(it == paramPlayer->invitedToGuildsList.end())
					{
						player->sendCancel("A player with that name is not invited to your guild.");
						return true;
					}

					std::stringstream ss;
					ss << player->getName() << " has revoked your invite to " << (player->getSex() == PLAYERSEX_FEMALE ? "her" : "his") << " guild.";
					paramPlayer->sendTextMessage(MSG_INFO_DESCR, ss.str());
					ss.str("");

					ss << player->getName() << " has revoked the guildinvite of " << paramPlayer->getName();
					guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
					paramPlayer->invitedToGuildsList.erase(it);
				}
				else if(IOLoginData::getInstance()->playerExists(param))
				{
					uint32_t guid;
					IOLoginData::getInstance()->getGuidByName(guid, param);
					if(!IOGuild::getInstance()->isInvitedToGuild(guid, player->getGuildId()))
					{
						player->sendCancel("A player with that name is not invited to your guild.");
						return true;
					}

					std::stringstream ss;
					ss << player->getName() << " has revoked the guildinvite of " << param << ".";
					guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());

					IOGuild::getInstance()->revokeGuildInvite(guid, player->getGuildId());
				}
				else
					player->sendCancel("A player with that name does not exist.");
			}
			else if(text.substr(0, 8) == "!promote" || text.substr(0, 7) == "!demote" || text.substr(0, 15) == "!passleadership" || text.substr(0, 5) == "!kick")
			{
				if(player->getGuildLevel() != GUILDLEVEL_LEADER)
				{
					player->sendCancel("You are not the leader of your guild.");
					return true;
				}

				std::string param;
				uint32_t length = 0;
				if(text[2] == 'r')
					length = 11;
				else if(text[2] == 'e')
					length = 10;
				else if(text[2] == 'a')
					length = 18;
				else
					length = 8;

				if(text.length() < length)
				{
					player->sendCancel("Invalid guildcommand parameters.");
					return true;
				}
				else
					length -= 2;

				param = text.substr(length);
				trimString(param);
				Player* paramPlayer = g_game.getPlayerByName(param);
				if(paramPlayer)
				{
					if(!paramPlayer->getGuildId() || !IOGuild::getInstance()->guildExists(paramPlayer->getGuildId()))
					{
						player->sendCancel("A player with that name is not in a guild.");
						return true;
					}

					if(player->getGuildId() != paramPlayer->getGuildId())
					{
						player->sendCancel("You are not in the same guild as a player with that name.");
						return true;
					}

					if(text[2] == 'r')
					{
						if(paramPlayer->getGuildLevel() != GUILDLEVEL_MEMBER)
						{
							player->sendCancel("You can only promote Members to Vice-Leaders.");
							return true;
						}

						if(!paramPlayer->isPremium())
						{
							player->sendCancel("A player with that name does not have a premium account.");
							return true;
						}

						paramPlayer->setGuildLevel(GUILDLEVEL_VICE);

						std::stringstream ss;
						ss << player->getName() << " has promoted " << paramPlayer->getName() << " to " << paramPlayer->getGuildRank() << ".";
						guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
					}
					else if(text[2] == 'e')
					{
						if(paramPlayer->getGuildLevel() != GUILDLEVEL_VICE)
						{
							player->sendCancel("You can only demote Vice-Leaders to Members.");
							return true;
						}

						paramPlayer->setGuildLevel(GUILDLEVEL_MEMBER);

						std::stringstream ss;
						ss << player->getName() << " has demoted " << paramPlayer->getName() << " to " << paramPlayer->getGuildRank() << ".";
						guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
					}
					else if(text[2] == 'a')
					{
						if(paramPlayer->getGuildLevel() != GUILDLEVEL_VICE)
						{
							player->sendCancel("A player with that name is not a Vice-Leader.");
							return true;
						}

						if(paramPlayer->getLevel() < (uint32_t)g_config.getNumber(ConfigManager::LEVEL_TO_CREATE_GUILD))
						{
							std::stringstream ss;
							ss << "The new guild leader has to be at least Level " << g_config.getNumber(ConfigManager::LEVEL_TO_CREATE_GUILD) << ".";
							player->sendCancel(ss.str());
							return true;
						}

						paramPlayer->setGuildLevel(GUILDLEVEL_LEADER);
						player->setGuildLevel(GUILDLEVEL_VICE);
						IOGuild::getInstance()->updateOwnerId(player->getGuildId(), paramPlayer->getGUID());

						std::stringstream ss;
						ss << player->getName() << " has passed the guild leadership to " << paramPlayer->getName() << ".";
						guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
					}
					else
					{
						if(player->getGuildLevel() <= paramPlayer->getGuildLevel())
						{
							player->sendCancel("You may only kick players with a guild rank below your.");
							return true;
						}

						std::stringstream ss;
						ss << paramPlayer->getName() << " has been kicked from the guild by " << player->getName() << ".";
						guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());

						paramPlayer->leaveGuild();
					}
				}
				else if(IOLoginData::getInstance()->playerExists(param))
				{
					uint32_t guid;
					IOLoginData::getInstance()->getGuidByName(guid, param);
					if(!IOLoginData::getInstance()->hasGuild(guid))
					{
						player->sendCancel("A player with that name is not in a guild.");
						return true;
					}

					if(player->getGuildId() != IOGuild::getInstance()->getGuildId(guid))
					{
						player->sendCancel("You are not in the same guild as a player with that name.");
						return true;
					}

					if(text[2] == 'r')
					{
						if(IOGuild::getInstance()->getGuildLevel(guid) != GUILDLEVEL_MEMBER)
						{
							player->sendCancel("You can only promote Members to Vice-Leaders.");
							return true;
						}

						if(!IOLoginData::getInstance()->isPremium(guid))
						{
							player->sendCancel("A player with that name does not have a premium account.");
							return true;
						}

						IOGuild::getInstance()->setGuildLevel(guid, GUILDLEVEL_VICE);
						std::string rankName = IOGuild::getInstance()->getRankName(
							IOGuild::getInstance()->getGuildLevel(guid),
							IOGuild::getInstance()->getGuildId(guid));

						std::stringstream ss;
						ss << player->getName() << " has promoted " << param << " to " << rankName << ".";
						guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
					}
					else if(text[2] == 'e')
					{
						if(!IOGuild::getInstance()->getGuildLevel(guid) != GUILDLEVEL_VICE)
						{
							player->sendCancel("You can only demote Vice-Leaders to Members.");
							return true;
						}

						IOGuild::getInstance()->setGuildLevel(guid, GUILDLEVEL_MEMBER);
						std::string rankName = IOGuild::getInstance()->getRankName(
							IOGuild::getInstance()->getGuildLevel(guid),
							IOGuild::getInstance()->getGuildId(guid));

						std::stringstream ss;
						ss << player->getName() << " has demoted " << param << " to " << rankName << ".";
						guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
					}
					else if(text[2] == 'a')
					{
						if(IOGuild::getInstance()->getGuildLevel(guid) != GUILDLEVEL_VICE)
						{
							player->sendCancel("A player with that name is not a Vice-Leader.");
							return true;
						}

						if(IOLoginData::getInstance()->getLevel(guid) < g_config.getNumber(ConfigManager::LEVEL_TO_CREATE_GUILD))
						{
							std::stringstream ss;
							ss << "The new guild leader has to be at least Level " << g_config.getNumber(ConfigManager::LEVEL_TO_CREATE_GUILD) << ".";
							player->sendCancel(ss.str());
							return true;
						}

						IOGuild::getInstance()->setGuildLevel(guid, GUILDLEVEL_LEADER);
						player->setGuildLevel(GUILDLEVEL_VICE);
						IOGuild::getInstance()->updateOwnerId(player->getGuildId(), guid);

						std::stringstream ss;
						ss << player->getName() << " has passed the guild leadership to " << param << ".";
						guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
					}
					else
					{
						std::stringstream ss;
						ss << param << " has been kicked from the guild by " << player->getName() << ".";
						guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
						IOLoginData::getInstance()->leaveGuild(guid);
					}
				}
				else
					player->sendCancel("A player with that name does not exist.");
			}
			else if(text.substr(0, 5) == "!nick")
			{
				if(text.length() <= 6)
				{
					player->sendCancel("Invalid guildcommand parameters.");
					return true;
				}

				std::string param = text.substr(6);
				boost::char_separator<char> sep(",");
				tokenizer cmdtokens(param, sep);
				tokenizer::iterator cmdit = cmdtokens.begin();
				std::string param1, param2;
				param1 = parseParams(cmdit, cmdtokens.end());
				param2 = parseParams(cmdit, cmdtokens.end());
				trimString(param1);
				trimString(param2);
				if(param2.length() <= 3)
				{
					player->sendCancel("That guildnick is too short, please select a longer one.");
					return true;
				}

				if(param2.length() >= 15)
				{
					player->sendCancel("That guildnick is too long, please select a shorter one.");
					return true;
				}

				if(!isValidName(param2, false))
				{
					player->sendCancel("That guildnick is not valid");
					return true;
				}

				Player* paramPlayer = g_game.getPlayerByName(param1);
				if(paramPlayer)
				{
					if(!paramPlayer->getGuildId() || !IOGuild::getInstance()->guildExists(paramPlayer->getGuildId()))
					{
						player->sendCancel("A player with that name is not in a guild.");
						return true;
					}

					if(player->getGuildId() != paramPlayer->getGuildId())
					{
						player->sendCancel("A player with that name is not in your guild.");
						return true;
					}

					if(player->getGuildLevel() == GUILDLEVEL_MEMBER || player->getGuildLevel() < paramPlayer->getGuildLevel())
					{
						player->sendCancel("You may only change the guild nick of players that have a lower rank than you.");
						return true;
					}

					paramPlayer->setGuildNick(param2);

					std::stringstream ss;
					if(player != paramPlayer)
						ss << player->getName() << " has set the guildnick of " << paramPlayer->getName() << " to: " << param2 << ".";
					else
						ss << player->getName() << " has set " << (player->getSex() == PLAYERSEX_FEMALE ? "her" : "his") << " guildnick to: " << param2 << ".";

					guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
				}
				else if(IOLoginData::getInstance()->playerExists(param1))
				{
					uint32_t guid;
					IOLoginData::getInstance()->getGuidByName(guid, (std::string&)param1);
					if(!IOLoginData::getInstance()->hasGuild(guid) || !IOGuild::getInstance()->guildExists(guid))
					{
						player->sendCancel("A player with that name is not in any guild");
						return true;
					}

					if(player->getGuildId() != IOGuild::getInstance()->getGuildId(guid))
					{
						player->sendCancel("A player with that name is not in your guild.");
						return true;
					}

					if(player->getGuildLevel() == GUILDLEVEL_MEMBER || player->getGuildLevel() < IOGuild::getInstance()->getGuildLevel(guid))
					{
						player->sendCancel("You may only change the guild nick of players that have a lower rank than you.");
						return true;
					}

					IOGuild::getInstance()->setGuildNick(guid, param2);

					std::stringstream ss;
					ss << player->getName() << " has set the guildnick of " << param1 << " to: " << param2 << ".";
					guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
				}
				else
					player->sendCancel("A player with that name does not exist.");
			}
			else if(text.substr(0, 12) == "!setrankname")
			{
				if(player->getGuildLevel() != GUILDLEVEL_LEADER)
				{
					player->sendCancel("You are not the leader of your guild.");
					return true;
				}

				if(text.length() <= 13)
				{
					player->sendCancel("Invalid guildcommand parameters.");
					return true;
				}

				std::string param = text.substr(13);
				boost::char_separator<char> sep(",");
				tokenizer cmdtokens(param, sep);
				tokenizer::iterator cmdit = cmdtokens.begin();

				std::string param1, param2;
				param1 = parseParams(cmdit, cmdtokens.end());
				param2 = parseParams(cmdit, cmdtokens.end());
				trimString(param1);
				trimString(param2);

				if(param2.length() <= 3)
				{
					player->sendCancel("The new rankname is too short.");
					return true;
				}

				if(param2.length() > 20)
				{
					player->sendCancel("The new rankname is too long.");
					return true;
				}

				if(!isValidName(param2, false))
				{
					player->sendCancel("The new guildrank contains invalid characters");
					return true;
				}

				if(!IOGuild::getInstance()->rankNameExists(param1, player->getGuildId()))
				{
					player->sendCancel("There is no such rankname in your guild");
					return true;
				}

				if(IOGuild::getInstance()->rankNameExists(param2, player->getGuildId()))
				{
					player->sendCancel("There is already a rank in your guild with that name");
					return true;
				}

				IOGuild::getInstance()->changeRankName(param1, param2, player->getGuildId());

				std::stringstream ss;
				ss << player->getName() << " has renamed the guildrank: " << param1 << ", to: " << param2 << ".";
				guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
			}
			else if(text.substr(0, 8) == "!setmotd")
			{
				if(player->getGuildLevel() != GUILDLEVEL_LEADER)
				{
					player->sendCancel("Only the leader of your guild can set the guild motd.");
					return true;
				}

				if(text.length() <= 9)
				{
					player->sendCancel("Invalid guildcommand parameters.");
					return true;
				}

				std::string param = text.substr(9);
				trimString(param);
				if(param.length() <= 3)
				{
					player->sendCancel("That motd is too short.");
					return true;
				}

				if(param.length() >= 225)
				{
					player->sendCancel("That motd is too long.");
					return true;
				}

				IOGuild::getInstance()->setMotd(player->getGuildId(), param);

				std::stringstream ss;
				ss << player->getName() << " has set the Message of the Day to: " << param;
				guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
			}
			else if(text == "!cleanmotd")
			{
				if(player->getGuildLevel() != GUILDLEVEL_LEADER)
				{
					player->sendCancel("Only the leader of your guild can clean the guild motd.");
					return true;
				}

				IOGuild::getInstance()->setMotd(player->getGuildId(), "");

				std::stringstream ss;
				ss << player->getName() << " has cleaned the Message of the Day.";
				guildChannel->talk(player, SPEAK_CHANNEL_R2, ss.str());
			}
			else if(text == "!commands")
				player->sendToChannel(player, SPEAK_CHANNEL_R2, "The guild commands are: <!disband>, <!invite playerName>, <!leave>, <!kick playerName>, <!revoke playerName>, <!demote playerName>, <!promote playerName>, <!passleadership playerName>, <!nick playerName, nick>, <!setrankname oldRankName, newRankName>, <!setmotd newMotd>, <!guildonline> and <!cleanmotd>.", 0x00);

			return true;
		}
	}

	if(channelId == 0x00 && player->getGuildLevel() > 1)
		type = SPEAK_CHANNEL_O;

	return channel->talk(player, type, text);
}

std::string Chat::getChannelName(Player* player, uint16_t channelId)
{
	if(ChatChannel* channel = getChannel(player, channelId))
		return channel->getName();

	return "";
}

ChannelList Chat::getChannelList(Player* player)
{
	ChannelList list;
	if(player->getGuildId() && player->getGuildName().length())
	{
		ChatChannel* channel = getChannel(player, 0x00);
		if(channel)
			list.push_back(channel);
		else if((channel = createChannel(player, 0x00)))
			list.push_back(channel);
	}

	if(player->getParty())
	{
		ChatChannel* channel = getChannel(player, 0x08);
		if(channel)
			list.push_back(channel);
		else if((channel = createChannel(player, 0x08)))
			list.push_back(channel);
	}

	for(NormalChannelMap::iterator it = m_normalChannels.begin(); it != m_normalChannels.end(); ++it)
	{
		if(it->first == 0x04 || it->first == 0x06 || it->first == 0x09)
		{
			list.push_back(it->second);
			continue;
		}

		ChatChannel* channel = getChannel(player, it->first);
		if(channel)
			list.push_back(it->second);
	}

	bool hasPrivate = false;
	for(PrivateChannelMap::iterator pit = m_privateChannels.begin(); pit != m_privateChannels.end(); ++pit)
	{
		if(PrivateChatChannel* channel = pit->second)
		{
			if(channel->isInvited(player))
				list.push_back(channel);

			if(channel->getOwner() == player->getGUID())
				hasPrivate = true;
		}
	}

	if(!hasPrivate && player->isPremium())
		list.push_front(dummyPrivate);

	return list;
}

ChatChannel* Chat::getChannel(Player* player, uint16_t channelId)
{
	if(channelId == 0x00)
	{
		GuildChannelMap::iterator git = m_guildChannels.find(player->getGuildId());
		if(git != m_guildChannels.end())
			return git->second;

		return NULL;
	}

	if(channelId == 0x08)
	{
		if(!player->getParty())
			return NULL;

		PartyChannelMap::iterator it = m_partyChannels.find(player->getParty());
		if(it != m_partyChannels.end())
			return it->second;

		return NULL;
	}

	NormalChannelMap::iterator nit = m_normalChannels.find(channelId);
	if(nit != m_normalChannels.end())
	{
		switch(channelId)
		{
			case 0x01:
				if(player->getAccountType() < ACCOUNT_TYPE_GAMEMASTER)
					return NULL;
				break;

			case 0x02:
				if(player->getAccountType() < ACCOUNT_TYPE_TUTOR)
					return NULL;
				break;

			case 0x03:
				if(!player->hasFlag(PlayerFlag_CanAnswerRuleViolations) && player->getAccountType() < ACCOUNT_TYPE_GAMEMASTER)
					return NULL;
				break;

			case 0x05:
				if(player->getAccountType() < ACCOUNT_TYPE_SENIORTUTOR && player->getVocationId() == 0)
					return NULL;
				break;

			case 0x07:
				if(player->getAccountType() < ACCOUNT_TYPE_SENIORTUTOR && player->getVocationId() != 0)
					return NULL;
				break;

			default:
				break;
		}
		return nit->second;
	}

	PrivateChannelMap::iterator pit = m_privateChannels.find(channelId);
	if(pit != m_privateChannels.end() && pit->second->isInvited(player))
		return pit->second;

	return NULL;
}

ChatChannel* Chat::getChannelById(uint16_t channelId)
{
	NormalChannelMap::iterator it = m_normalChannels.find(channelId);
	if(it != m_normalChannels.end())
		return it->second;

	return NULL;
}

PrivateChatChannel* Chat::getPrivateChannel(Player* player)
{
	PrivateChatChannel* channel = NULL;
	for(PrivateChannelMap::iterator it = m_privateChannels.begin(); it != m_privateChannels.end(); ++it)
	{
		if((channel = it->second) && channel->getOwner() == player->getGUID())
			return channel;
	}
	return channel;
}
