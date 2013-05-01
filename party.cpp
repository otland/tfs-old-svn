//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Party system
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

#include "party.h"
#include "player.h"
#include "game.h"
#include "chat.h"

extern Game g_game;
extern Chat g_chat;

Party::Party(Player* _leader)
{
	sharedExpActive = false;
	sharedExpEnabled = false;

	if(_leader)
	{
		setLeader(_leader);
		_leader->setParty(this);
		_leader->sendPlayerPartyIcons(_leader);
	}
}

Party::~Party()
{
	//
}

void Party::disband()
{
	getLeader()->setParty(NULL);
	getLeader()->sendClosePrivate(CHANNEL_PARTY);
	getLeader()->sendPlayerPartyIcons(getLeader());
	getLeader()->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");

	for(PlayerVector::iterator it = inviteList.begin(); it != inviteList.end(); ++it)
	{
		(*it)->removePartyInvitation(this);
		(*it)->sendPlayerPartyIcons(getLeader());
		(*it)->sendPlayerPartyIcons((*it));
		getLeader()->sendPlayerPartyIcons((*it));
	}

	inviteList.clear();
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		(*it)->setParty(NULL);
		(*it)->sendClosePrivate(CHANNEL_PARTY);
		(*it)->sendPlayerPartyIcons(getLeader());
		(*it)->sendPlayerPartyIcons((*it));
		(*it)->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
		getLeader()->sendPlayerPartyIcons((*it));
	}

	memberList.clear();
	setLeader(NULL);
	delete this;
}

bool Party::leaveParty(Player* player)
{
	if(!player)
		return false;

	if(!isPlayerMember(player) && getLeader() != player)
		return false;

	bool missingLeader = false;
	if(getLeader() == player)
	{
		if(!memberList.empty())
		{
			if(memberList.size() == 1 && inviteList.empty())
				missingLeader = true;
			else
				passPartyLeadership(memberList.front());
		}
		else
			missingLeader = true;
	}

	//since we already passed the leadership, we remove the player from the list
	PlayerVector::iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end())
		memberList.erase(it);

	it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);

	player->setParty(NULL);
	player->sendClosePrivate(CHANNEL_PARTY);
	player->sendPlayerPartyIcons(player);
	player->sendTextMessage(MSG_INFO_DESCR, "You have left the party.");

	updateSharedExperience();
	updatePartyIcons(player);
	clearPlayerPoints(player);

	std::ostringstream ss;
	ss << player->getName() << " has left the party.";
	broadcastPartyMessage(MSG_INFO_DESCR, ss.str());

	if(missingLeader || disbandParty())
		disband();

	return true;
}

bool Party::passPartyLeadership(Player* player)
{
	if(!player || getLeader() == player || !isPlayerMember(player))
		return false;

	//Remove it before to broadcast the message correctly
	PlayerVector::iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end())
		memberList.erase(it);

	std::ostringstream ss;
	ss << player->getName() << " is now the leader of the party.";
	broadcastPartyMessage(MSG_INFO_DESCR, ss.str(), true);

	Player* oldLeader = getLeader();
	setLeader(player);

	memberList.insert(memberList.begin(), oldLeader);

	updateSharedExperience();
	updatePartyIcons(oldLeader);
	updatePartyIcons(getLeader());
	updatePartyIcons(player);

	player->sendTextMessage(MSG_INFO_DESCR, "You are now the leader of the party.");
	return true;
}

bool Party::joinParty(Player* player)
{
	if(!player || player->isRemoved())
		return false;

	std::ostringstream ss;
	ss << player->getName() << " has joined the party.";
	broadcastPartyMessage(MSG_INFO_DESCR, ss.str());
	ss.str("");

	memberList.push_back(player);
	player->setParty(this);

	PlayerVector::iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);

	player->removePartyInvitation(this);
	updateSharedExperience();
	updatePartyIcons(player);

	const std::string& leaderName = leader->getName();
	ss << "You have joined " << leaderName << "'" << (leaderName[leaderName.length() - 1] == 's' ? "" : "s") <<
		" party. Open the party channel to communicate with your companions.";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	return true;
}

bool Party::removeInvite(Player* player)
{
	if(!player || player->isRemoved() || !isPlayerInvited(player))
		return false;

	PlayerVector::iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);

	getLeader()->sendPlayerPartyIcons(player);
	player->removePartyInvitation(this);
	player->sendPlayerPartyIcons(getLeader());

	if(disbandParty())
		disband();

	return true;
}

void Party::revokeInvitation(Player* player)
{
	std::ostringstream ss;
	ss << leader->getName() << " has revoked " << (leader->getSex() == PLAYERSEX_FEMALE ? "her" : "his") << " invitation.";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());

	ss.str("");
	ss << "Invitation for " << player->getName() << " has been revoked.";
	getLeader()->sendTextMessage(MSG_INFO_DESCR, ss.str());

	removeInvite(player);
}

bool Party::invitePlayer(Player* player)
{
	if(!player || player->isRemoved())
		return false;

	if(std::find(inviteList.begin(), inviteList.end(), player) != inviteList.end())
		return false;

	inviteList.push_back(player);

	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());
	player->addPartyInvitation(this);

	std::ostringstream ss;
	ss << player->getName() << " has been invited.";
	if(!memberList.size())
		ss << " Open the party channel to communicate with your members.";

	leader->sendTextMessage(MSG_INFO_DESCR, ss.str());

	ss.str("");
	ss << leader->getName() << " has invited you to " << (leader->getSex() == PLAYERSEX_FEMALE ? "her" : "his") << " party.";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	return true;
}

bool Party::isPlayerMember(const Player* player) const
{
	PlayerVector::const_iterator it = std::find(memberList.begin(), memberList.end(), player);
	return it != memberList.end();
}

bool Party::isPlayerInvited(const Player* player) const
{
	PlayerVector::const_iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	return it != inviteList.end();
}

void Party::updatePartyIcons(Player* player)
{
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	for(PlayerVector::iterator it = inviteList.begin(); it != inviteList.end(); ++it)
	{
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());
}

void Party::updateAllPartyIcons()
{
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		for(PlayerVector::iterator it2 = memberList.begin(); it2 != memberList.end(); ++it2)
			(*it)->sendPlayerPartyIcons((*it2));

		(*it)->sendPlayerPartyIcons(getLeader());
		getLeader()->sendPlayerPartyIcons((*it));
	}

	for(PlayerVector::iterator it = inviteList.begin(); it != inviteList.end(); ++it)
		(*it)->sendPlayerPartyIcons(getLeader());

	getLeader()->sendPlayerPartyIcons(getLeader());
}

void Party::broadcastPartyMessage(MessageClasses msgClass, const std::string& msg, bool sendToInvitations /*= false*/)
{
	PlayerVector::iterator it;
	if(!memberList.empty())
	{
		for(it = memberList.begin(); it != memberList.end(); ++it)
			(*it)->sendTextMessage(msgClass, msg);
	}

	getLeader()->sendTextMessage(msgClass, msg);
	if(sendToInvitations && !inviteList.empty())
	{
		for(it = inviteList.begin(); it != inviteList.end(); ++it)
			(*it)->sendTextMessage(msgClass, msg);
	}
}

void Party::broadcastPartyLoot(const std::string& loot)
{
	getLeader()->sendTextMessage(MSG_INFO_DESCR, loot);
	if(!memberList.empty())
	{
		for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
			(*it)->sendTextMessage(MSG_INFO_DESCR, loot);
	}
}

void Party::updateSharedExperience()
{
	if(sharedExpActive)
	{
		bool result = canEnableSharedExperience();
		if(result != sharedExpEnabled)
		{
			sharedExpEnabled = result;
			updateAllPartyIcons();
		}
	}
}

bool Party::setSharedExperience(Player* player, bool _sharedExpActive)
{
	if(!player || getLeader() != player)
		return false;

	if(sharedExpActive == _sharedExpActive)
		return true;

	sharedExpActive = _sharedExpActive;
	if(sharedExpActive)
	{
		sharedExpEnabled = canEnableSharedExperience();
		if(sharedExpEnabled)
			getLeader()->sendTextMessage(MSG_INFO_DESCR, "Shared Experience is now active.");
		else
			getLeader()->sendTextMessage(MSG_INFO_DESCR, "Shared Experience has been activated, but some members of your party are inactive.");
	}
	else
		getLeader()->sendTextMessage(MSG_INFO_DESCR, "Shared Experience has been deactivated.");

	updateAllPartyIcons();
	return true;
}

void Party::shareExperience(uint64_t experience)
{
	uint32_t shareExperience = (uint64_t)std::ceil((((double)experience / (memberList.size() + 1)) + ((double)experience * 0.05)));
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
		(*it)->onGainSharedExperience(shareExperience);

	getLeader()->onGainSharedExperience(shareExperience);
}

bool Party::canUseSharedExperience(const Player* player) const
{
	if(!memberList.size())
		return false;

	uint32_t highestLevel = getLeader()->getLevel();
	for(PlayerVector::const_iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		if((*it)->getLevel() > highestLevel)
			highestLevel = (*it)->getLevel();
	}

	uint32_t minLevel = (int32_t)std::ceil(((float)(highestLevel) * 2) / 3);
	if(player->getLevel() < minLevel)
		return false;

	const Position& leaderPos = getLeader()->getPosition();
	const Position& memberPos = player->getPosition();
	if(!Position::areInRange<30,30,1>(leaderPos, memberPos))
		return false;

	if(!player->hasFlag(PlayerFlag_NotGainInFight))
	{
		//check if the player has healed/attacked anything recently
		CountMap::const_iterator it = pointMap.find(player->getID());
		if(it == pointMap.end())
			return false;

		uint64_t timeDiff = OTSYS_TIME() - it->second.ticks;
		if(timeDiff > (uint64_t)g_game.getInFightTicks())
			return false;
	}

	return true;
}

bool Party::canEnableSharedExperience()
{
	if(!canUseSharedExperience(getLeader()))
		return false;

	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		if(!canUseSharedExperience(*it))
			return false;
	}

	return true;
}

void Party::addPlayerHealedMember(Player* player, uint32_t points)
{
	if(!player->hasFlag(PlayerFlag_NotGainInFight))
	{
		if(points > 0)
		{
			CountMap::iterator it = pointMap.find(player->getID());
			if(it == pointMap.end())
			{
				CountBlock_t cb;
				cb.ticks = OTSYS_TIME();
				cb.totalHeal = points;
				cb.totalDamage = 0;
				pointMap[player->getID()] = cb;
			}
			else
			{
				it->second.totalHeal += points;
				it->second.ticks = OTSYS_TIME();
			}

			updateSharedExperience();
		}
	}
}

void Party::addPlayerDamageMonster(Player* player, uint32_t points)
{
	if(!player->hasFlag(PlayerFlag_NotGainInFight))
	{
		if(points > 0)
		{
			CountMap::iterator it = pointMap.find(player->getID());
			if(it == pointMap.end())
			{
				CountBlock_t cb;
				cb.ticks = OTSYS_TIME();
				cb.totalDamage = points;
				cb.totalHeal = 0;
				pointMap[player->getID()] = cb;
			}
			else
			{
				it->second.totalDamage += points;
				it->second.ticks = OTSYS_TIME();
			}

			updateSharedExperience();
		}
	}
}

void Party::clearPlayerPoints(Player* player)
{
	CountMap::iterator it = pointMap.find(player->getID());
	if(it != pointMap.end())
	{
		pointMap.erase(it);
		updateSharedExperience();
	}
}

bool Party::canOpenCorpse(uint32_t ownerId)
{
	if(Player* player = g_game.getPlayerByID(ownerId))
		return leader->getID() == ownerId || isPlayerMember(player);

	return false;
}
