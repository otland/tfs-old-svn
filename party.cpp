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
#include "party.h"

#include "player.h"
#include "chat.h"
#include "game.h"
#include "configmanager.h"

extern Game g_game;
extern Chat g_chat;
extern ConfigManager g_config;

Party::Party(Player* _leader)
{
	sharedExpActive = sharedExpEnabled = false;
	if(_leader)
	{
		setLeader(_leader);
		_leader->setParty(this);
		_leader->sendPlayerPartyIcons(_leader);
	}
}

void Party::disband()
{
	getLeader()->sendClosePrivate(CHANNEL_PARTY);
	getLeader()->setParty(NULL);
	getLeader()->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
	getLeader()->sendPlayerPartyIcons(getLeader());
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
		(*it)->sendClosePrivate(CHANNEL_PARTY);
		(*it)->setParty(NULL);
		(*it)->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
		(*it)->sendPlayerPartyIcons((*it));
		(*it)->sendPlayerPartyIcons(getLeader());
		getLeader()->sendPlayerPartyIcons((*it));
	}

	memberList.clear();
	setLeader(NULL);
	delete this;
}

bool Party::leave(Player* player)
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
				passLeadership(memberList.front());
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

	player->sendTextMessage(MSG_INFO_DESCR, "You have left the party.");
	player->sendPlayerPartyIcons(player);

	updateSharedExperience();
	updateIcons(player);
	clearPlayerPoints(player);

	char buffer[105];
	sprintf(buffer, "%s has left the party.", player->getName().c_str());

	broadcastMessage(MSG_INFO_DESCR, buffer);
	if(missingLeader || canDisband())
		disband();

	return true;
}

bool Party::passLeadership(Player* player)
{
	if(!player || getLeader() == player || !isPlayerMember(player))
		return false;

	//Remove it before to broadcast the message correctly
	PlayerVector::iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end())
		memberList.erase(it);

	Player* oldLeader = getLeader();
	setLeader(player);
	memberList.insert(memberList.begin(), oldLeader);

	char buffer[125];
	sprintf(buffer, "%s is now the leader of the party.", player->getName().c_str());
	broadcastMessage(MSG_INFO_DESCR, buffer, true);

	player->sendTextMessage(MSG_INFO_DESCR, "You are now the leader of the party.");
	updateSharedExperience();

	updateIcons(oldLeader);
	updateIcons(player);
	return true;
}

bool Party::join(Player* player)
{
	if(!player || player->isRemoved())
		return false;

	memberList.push_back(player);
	player->setParty(this);

	player->removePartyInvitation(this);
	PlayerVector::iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end())
		inviteList.erase(it);

	char buffer[200];
	sprintf(buffer, "%s has joined the party.", player->getName().c_str());
	broadcastMessage(MSG_INFO_DESCR, buffer);

	sprintf(buffer, "You have joined %s'%s party. Open the party channel to communicate with your companions.", leader->getName().c_str(), (leader->getName()[leader->getName().length() - 1] == 's' ? "" : "s"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);

	updateSharedExperience();
	updateIcons(player);
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
	player->sendPlayerPartyIcons(getLeader());

	player->removePartyInvitation(this);
	if(canDisband())
		disband();

	return true;
}

void Party::revokeInvitation(Player* player)
{
	char buffer[150];
	sprintf(buffer, "%s has revoked %s invitation.", leader->getName().c_str(), (leader->getSex(false) ? "his" : "her"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);

	sprintf(buffer, "Invitation for %s has been revoked.", player->getName().c_str());
	getLeader()->sendTextMessage(MSG_INFO_DESCR, buffer);
	removeInvite(player);
}

bool Party::invitePlayer(Player* player)
{
	if(!player || player->isRemoved())
		return false;

	if(std::find(inviteList.begin(), inviteList.end(), player) != inviteList.end())
		return false;

	inviteList.push_back(player);
	player->addPartyInvitation(this);

	char buffer[150];
	sprintf(buffer, "%s has been invited.%s", player->getName().c_str(), (!memberList.size() ? " Open the party channel to communicate with your members." : ""));
	leader->sendTextMessage(MSG_INFO_DESCR, buffer);

	sprintf(buffer, "%s has invited you to %s party.", leader->getName().c_str(), (leader->getSex(false) ? "his" : "her"));
	player->sendTextMessage(MSG_INFO_DESCR, buffer);

	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());
	return true;
}

void Party::updateIcons(Player* player)
{
	PlayerVector::iterator it;
	for(it = memberList.begin(); it != memberList.end(); ++it)
	{
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	for(it = inviteList.begin(); it != inviteList.end(); ++it)
	{
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());
}

void Party::updateAllIcons()
{
	PlayerVector::iterator it;
	for(it = memberList.begin(); it != memberList.end(); ++it)
	{
		for(PlayerVector::iterator iit = memberList.begin(); iit != memberList.end(); ++iit)
			(*it)->sendPlayerPartyIcons((*iit));

		(*it)->sendPlayerPartyIcons(getLeader());
		getLeader()->sendPlayerPartyIcons((*it));
	}

	getLeader()->sendPlayerPartyIcons(getLeader());
	for(it = inviteList.begin(); it != inviteList.end(); ++it)
		(*it)->sendPlayerPartyIcons(getLeader());
}

void Party::broadcastMessage(MessageClasses messageClass, const std::string& text, bool sendToInvitations/* = false*/)
{
	PlayerVector::iterator it;
	if(!memberList.empty())
	{
		for(it = memberList.begin(); it != memberList.end(); ++it)
			(*it)->sendTextMessage(messageClass, text);
	}

	getLeader()->sendTextMessage(messageClass, text);
	if(sendToInvitations && !inviteList.empty())
	{
		for(it = inviteList.begin(); it != inviteList.end(); ++it)
			(*it)->sendTextMessage(messageClass, text);
	}
}

void Party::updateSharedExperience()
{
	if(!sharedExpActive)
		return;

	bool result = canEnableSharedExperience();
	if(result != sharedExpEnabled)
	{
		sharedExpEnabled = result;
		updateAllIcons();
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

	updateAllIcons();
	return true;
}

void Party::shareExperience(double experience, bool fromMonster, bool multiplied)
{
	double shareExperience = (experience / (double)(memberList.size() + 1));
	if(experience > (double)g_config.getNumber(ConfigManager::EXTRA_PARTY_LIMIT))
		shareExperience += (experience * (double)(g_config.getNumber(ConfigManager::EXTRA_PARTY_PERCENT) / 100));

	double tmpExperience = shareExperience;
	getLeader()->onGainSharedExperience(tmpExperience, fromMonster, multiplied);
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		tmpExperience = shareExperience;
		(*it)->onGainSharedExperience(tmpExperience, fromMonster, multiplied);
	}
}

bool Party::canUseSharedExperience(const Player* player, uint32_t highestLevel/* = 0*/) const
{
	if(!highestLevel)
	{
		highestLevel = getLeader()->getLevel();
		for(PlayerVector::const_iterator it = memberList.begin(); it != memberList.end(); ++it)
		{
			if((*it)->getLevel() > highestLevel)
				highestLevel = (*it)->getLevel();
		}
	}

	if(player->getLevel() < (uint32_t)std::ceil((double)highestLevel * g_config.getDouble(
		ConfigManager::PARTY_DIFFERENCE)) || !Position::areInRange(Position(
		g_config.getNumber(ConfigManager::PARTY_RADIUS_X), g_config.getNumber(
		ConfigManager::PARTY_RADIUS_Y), g_config.getNumber(ConfigManager::PARTY_RADIUS_Z)),
		getLeader()->getPosition(), player->getPosition()))
		return false;

	CountMap::const_iterator it = pointMap.find(player->getID());
	return it != pointMap.end() && (OTSYS_TIME() - it->second.ticks) <= g_config.getNumber(
		ConfigManager::EXPERIENCE_SHARE_ACTIVITY);
}

bool Party::canEnableSharedExperience()
{
	if(!memberList.size())
		return false;

	uint32_t highestLevel = getLeader()->getLevel();
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		if((*it)->getLevel() > highestLevel)
			highestLevel = (*it)->getLevel();
	}

	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it)
	{
		if(!canUseSharedExperience((*it), highestLevel))
			return false;
	}

	return canUseSharedExperience(getLeader(), highestLevel);
}

void Party::addPlayerHealedMember(Player* player, uint32_t points)
{
	if(points <= 0)
		return;

	CountMap::iterator it = pointMap.find(player->getID());
	if(it == pointMap.end())
	{
		CountBlock_t cb;
		cb.totalDamage = 0;
		cb.totalHeal = points;
		cb.ticks = OTSYS_TIME();
		pointMap[player->getID()] = cb;
	}
	else
	{
		it->second.totalHeal += points;
		it->second.ticks = OTSYS_TIME();
	}

	updateSharedExperience();
}

void Party::addPlayerDamageMonster(Player* player, uint32_t points)
{
	if(points <= 0)
		return;

	CountMap::iterator it = pointMap.find(player->getID());
	if(it == pointMap.end())
	{
		CountBlock_t cb;
		cb.totalHeal = 0;
		cb.totalDamage = points;
		cb.ticks = OTSYS_TIME();
		pointMap[player->getID()] = cb;
	}
	else
	{
		it->second.totalDamage += points;
		it->second.ticks = OTSYS_TIME();
	}

	updateSharedExperience();
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

bool Party::isPlayerMember(const Player* player) const
{
	return std::find(memberList.begin(), memberList.end(), player) != memberList.end();
}

bool Party::isPlayerInvited(const Player* player) const
{
	return std::find(inviteList.begin(), inviteList.end(), player) != inviteList.end();
}

bool Party::canOpenCorpse(uint32_t ownerId)
{
	if(Player* player = g_game.getPlayerByID(ownerId))
		return leader->getID() == ownerId || isPlayerMember(player);

	return false;
}
