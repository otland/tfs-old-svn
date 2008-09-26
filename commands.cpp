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

#include <string>
#include <fstream>
#include <utility>

#include "commands.h"
#include "player.h"
#include "npc.h"
#include "monsters.h"
#include "game.h"
#include "actions.h"
#include "house.h"
#include "iologindata.h"
#include "tools.h"
#include "ioban.h"
#include "configmanager.h"
#include "town.h"
#include "spells.h"
#include "talkaction.h"
#include "movement.h"
#include "spells.h"
#include "weapons.h"
#include "raids.h"
#include "globalevent.h"
#include "chat.h"
#include "teleport.h"
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
#include "outputmessage.h"
#include "connection.h"
#include "admin.h"
#include "status.h"
#include "protocollogin.h"
#endif
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern ConfigManager g_config;
extern Actions* g_actions;
extern Monsters g_monsters;
extern Npcs g_npcs;
extern TalkActions* g_talkActions;
extern MoveEvents* g_moveEvents;
extern Spells* g_spells;
extern Weapons* g_weapons;
extern Game g_game;
extern Chat g_chat;
extern CreatureEvents* g_creatureEvents;
extern GlobalEvents* g_globalEvents;

extern bool readXMLInteger(xmlNodePtr p, const char* tag, int32_t &value);

#define ipText(a) (uint32_t)a[0] << "." << (uint32_t)a[1] << "." << (uint32_t)a[2] << "." << (uint32_t)a[3]

s_defcommands Commands::defined_commands[] =
{
	{"/s", &Commands::placeNpc},
	{"/m", &Commands::placeMonster},
	{"/n", &Commands::createItemByName},
	{"/reload", &Commands::reloadInfo},
	{"/closeserver", &Commands::closeServer},
	{"/openserver", &Commands::openServer},
	{"/raid", &Commands::forceRaid},
	{"/addskill", &Commands::addSkill},
	{"/unban", &Commands::unban},
	{"/ghost", &Commands::ghost},
	{"/squelch", &Commands::squelch},
	{"/attr", &Commands::changeThingProporties},
	{"/baninfo", &Commands::showBanishmentInfo},
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	{"/serverdiag",&Commands::serverDiag},
#endif
	{"!frags", &Commands::playerFrags},

	//TODO: Make them talkactions
	{"/summon", &Commands::placeSummon},
	{"/t", &Commands::teleportMasterPos},
	{"/c", &Commands::teleportHere},
	{"/i", &Commands::createItemById},
	{"/goto", &Commands::teleportTo},
	{"/info", &Commands::getInfo},
	{"/a", &Commands::teleportNTiles},
	{"/kick", &Commands::kickPlayer},
	{"/owner", &Commands::setHouseOwner},
	{"/gethouse", &Commands::getHouse},
	{"/town", &Commands::teleportToTown},
	{"/up", &Commands::changeFloor},
	{"/down", &Commands::changeFloor},
	{"/r", &Commands::removeThing},
	{"/newtype", &Commands::newType},
	{"/send", &Commands::sendTo},
	{"!buyhouse", &Commands::buyHouse},
 	{"!sellhouse", &Commands::sellHouse},
 	{"!createguild", &Commands::createGuild},
 	{"!joinguild", &Commands::joinGuild}
};

Commands::Commands()
{
	loaded = false;

	//setup command map
	for(uint32_t i = 0; i < sizeof(defined_commands) / sizeof(defined_commands[0]); i++)
	{
		Command* cmd = new Command;
		cmd->loadedAccess = false;
		cmd->accessLevel = 0;
		cmd->loadedLogging = false;
		cmd->logged = true;
		cmd->f = defined_commands[i].f;
		std::string key = defined_commands[i].name;
		commandMap[key] = cmd;
	}
}

bool Commands::loadFromXml()
{
	std::string filename = "data/XML/commands.xml";
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc)
	{
		loaded = true;
		xmlNodePtr root, p;
		std::string strCmd;

		root = xmlDocGetRootElement(doc);
		if(xmlStrcmp(root->name,(const xmlChar*)"commands") != 0)
		{
			xmlFreeDoc(doc);
			return false;
		}

		p = root->children;
		while(p)
		{
			if(xmlStrcmp(p->name, (const xmlChar*)"command") == 0)
			{
				if(readXMLString(p, "cmd", strCmd))
				{
					int32_t intValue;
					std::string strValue;

					CommandMap::iterator it = commandMap.find(strCmd);
					if(it != commandMap.end())
					{
						if(readXMLInteger(p, "access", intValue))
						{
							if(!it->second->loadedAccess)
							{
								it->second->accessLevel = intValue;
								it->second->loadedAccess = true;
							}
							else
								std::cout << "Duplicated access tag for " << strCmd << std::endl;
						}
						else
							std::cout << "Missing access tag for " << strCmd << std::endl;

						if(readXMLString(p, "log", strValue))
						{
							if(!it->second->loadedLogging)
							{
								it->second->logged = booleanString(strValue);
								it->second->loadedLogging = true;
							}
							else
								std::cout << "Duplicated log tag for " << strCmd << std::endl;
						}
						else
							std::cout << "Missing log tag for " << strCmd << std::endl;
					}
					else
						std::cout << "Unknown command " << strCmd << std::endl;
				}
				else
					std::cout << "Missing cmd." << std::endl;
			}
			p = p->next;
		}
		xmlFreeDoc(doc);
	}

	for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it)
	{
		if(!it->second->loadedAccess)
			std::cout << "Warning: Missing access for command " << it->first << std::endl;

		if(!it->second->loadedLogging)
			std::cout << "Warning: Missing log command " << it->first << std::endl;

		g_game.addCommandTag(it->first.substr(0, 1));
	}
	return loaded;
}

bool Commands::reload()
{
	loaded = false;
	for(CommandMap::iterator it = commandMap.begin(); it != commandMap.end(); ++it)
	{
		it->second->accessLevel = 0;
		it->second->loadedAccess = false;
		it->second->logged = true;
		it->second->loadedLogging = false;
	}
	g_game.resetCommandTag();
	return loadFromXml();
}

bool Commands::exeCommand(Creature* creature, const std::string& cmd)
{
	std::string str_command;
	std::string str_param;

	std::string::size_type loc = cmd.find(' ', 0 );
	if(loc != std::string::npos && loc >= 0)
	{
		str_command = std::string(cmd, 0, loc);
		str_param = std::string(cmd, (loc + 1), cmd.size() - loc - 1);
	}
	else
	{
		str_command = cmd;
		str_param = std::string("");
	}

	//find command
	CommandMap::iterator it = commandMap.find(str_command);
	if(it == commandMap.end())
		return false;

	Player* player = creature->getPlayer();
	if(player && (it->second->accessLevel > player->getAccessLevel() || player->name == "Account Manager"))
	{
		if(player->getAccessLevel() > 0)
			player->sendTextMessage(MSG_STATUS_SMALL, "You can not execute this command.");

		return false;
	}

	//execute command
	CommandFunc cfunc = it->second->f;
	(this->*cfunc)(creature, str_command, str_param);
	if(player && it->second->logged)
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, cmd.c_str());

		char buf[21], buffer[100];
		formatDate(time(NULL), buf);
		sprintf(buffer, "data/logs/%s commands.log", player->name.c_str());

		FILE* file = fopen(buffer, "a");
		if(file)
		{
			fprintf(file, "[%s] %s\n", buf, cmd.c_str());
			fclose(file);
		}
	}
	return true;
}

bool Commands::placeNpc(Creature* creature, const std::string& cmd, const std::string& param)
{
	Npc* npc = Npc::createNpc(param);
	if(!npc)
		return false;

	// Place the npc
	if(g_game.placeCreature(npc, creature->getPosition()))
	{
		g_game.addMagicEffect(creature->getPosition(), NM_ME_MAGIC_BLOOD);
		return true;
	}
	else
	{
		delete npc;
		Player* player = creature->getPlayer();
		if(player)
		{
			player->sendCancelMessage(RET_NOTENOUGHROOM);
			g_game.addMagicEffect(creature->getPosition(), NM_ME_POFF);
		}
		return true;
	}
	return false;
}

bool Commands::placeMonster(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();

	Monster* monster = Monster::createMonster(param);
	if(!monster)
	{
		if(player)
		{
			player->sendCancelMessage(RET_NOTPOSSIBLE);
			g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
		}
		return false;
	}

	// Place the monster
	if(g_game.placeCreature(monster, creature->getPosition()))
	{
		g_game.addMagicEffect(creature->getPosition(), NM_ME_MAGIC_BLOOD);
		return true;
	}
	else
	{
		delete monster;
		if(player)
		{
			player->sendCancelMessage(RET_NOTENOUGHROOM);
			g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
		}
	}
	return false;
}

ReturnValue Commands::placeSummon(Creature* creature, const std::string& name)
{
	Monster* monster = Monster::createMonster(name);
	if(!monster)
		return RET_NOTPOSSIBLE;

	// Place the monster
	creature->addSummon(monster);
	if(!g_game.placeCreature(monster, creature->getPosition()))
	{
		creature->removeSummon(monster);
		return RET_NOTENOUGHROOM;
	}
	return RET_NOERROR;
}

bool Commands::placeSummon(Creature* creature, const std::string& cmd, const std::string& param)
{
	ReturnValue ret = placeSummon(creature, param);
	if(ret != RET_NOERROR)
	{
		if(Player* player = creature->getPlayer())
		{
			player->sendCancelMessage(ret);
			g_game.addMagicEffect(player->getPosition(), NM_ME_POFF);
		}
	}
	return (ret == RET_NOERROR);
}

bool Commands::teleportMasterPos(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		Player* targetPlayer = player;
		if(!param.empty())
		{
			if(Player* paramPlayer = g_game.getPlayerByName(param))
				targetPlayer = paramPlayer;
			else
			{
				player->sendCancel("Player not found.");
				return false;
			}
		}

		Position destPos = targetPlayer->masterPos;
		Position oldPosition = targetPlayer->getPosition();
		Position newPosition = g_game.getClosestFreeTile(targetPlayer, destPos);
		if(targetPlayer->getPosition() != destPos)
		{
			if(newPosition.x == 0)
			{
				if(param.empty())
					player->sendCancel("You can not teleport there.");
				else
				{
					char buffer[100];
					sprintf(buffer, "You can not teleport %s there.", targetPlayer->getName().c_str());
					player->sendCancel(buffer);
				}
			}
			else if(g_game.internalTeleport(targetPlayer, newPosition, true) == RET_NOERROR)
			{
				g_game.addMagicEffect(oldPosition, NM_ME_POFF, targetPlayer->isInGhostMode());
				g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, targetPlayer->isInGhostMode());
				return true;
			}
		}
	}
	return false;
}

bool Commands::teleportHere(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		Creature* targetCreature = g_game.getCreatureByName(param);
		if(targetCreature)
		{
			Position oldPosition = targetCreature->getPosition();
			Position destPos = targetCreature->getPosition();
			Position newPosition = g_game.getClosestFreeTile(targetCreature, player->getPosition());
			if(newPosition.x == 0)
			{
				char buffer[100];
				sprintf(buffer, "You can not teleport %s to you.", targetCreature->getName().c_str());
				player->sendCancel(buffer);
			}
			else if(g_game.internalTeleport(targetCreature, newPosition, true) == RET_NOERROR)
			{
				g_game.addMagicEffect(oldPosition, NM_ME_POFF, targetCreature->isInGhostMode());
				g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, targetCreature->isInGhostMode());
				return true;
			}
		}
		else
			player->sendCancel("A creature with that name could not be found.");
	}
	return false;
}

bool Commands::createItemById(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	std::string tmp = param;

	std::string::size_type pos = tmp.find(' ', 0);
	if(pos == std::string::npos)
		pos = tmp.size();

	int32_t type = atoi(tmp.substr(0, pos).c_str());
	int32_t count = 1;
	if(pos < tmp.size())
	{
		tmp.erase(0, pos + 1);
		count = std::max(1, std::min(atoi(tmp.c_str()), 100));
	}

	Item* newItem = Item::CreateItem(type, count);
	if(!newItem)
		return false;

	ReturnValue ret = g_game.internalAddItem(player, newItem);
	if(ret != RET_NOERROR)
	{
		ret = g_game.internalAddItem(player->getTile(), newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			return false;
		}
	}

	g_game.startDecay(newItem);
	g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISON);
	return true;
}

bool Commands::createItemByName(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	std::string::size_type pos1 = param.find("\"");
	pos1 = (std::string::npos == pos1 ? 0 : pos1 + 1);

	std::string::size_type pos2 = param.rfind("\"");
	if(pos2 == pos1 || pos2 == std::string::npos)
	{
		pos2 = param.rfind(' ');
		if(pos2 == std::string::npos)
			pos2 = param.size();
	}

	std::string itemName = param.substr(pos1, pos2 - pos1);

	int32_t count = 1;
	if(pos2 < param.size())
	{
		std::string itemCount = param.substr(pos2 + 1, param.size() - (pos2 + 1));
		count = std::min(atoi(itemCount.c_str()), 100);
	}

	int32_t itemId = Item::items.getItemIdByName(itemName);
	if(itemId == -1)
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, "Item could not be summoned.");
		return false;
	}

	Item* newItem = Item::CreateItem(itemId, count);
	if(!newItem)
		return false;

	ReturnValue ret = g_game.internalAddItem(player, newItem);
	if(ret != RET_NOERROR)
	{
		ret = g_game.internalAddItem(player->getTile(), newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			return false;
		}
	}

	g_game.startDecay(newItem);
	g_game.addMagicEffect(player->getPosition(), NM_ME_MAGIC_POISON);
	return true;
}

bool Commands::reloadInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		std::string tmpParam = asLowerCaseString(param);
		if(tmpParam == "action" || tmpParam == "actions")
		{
			g_actions->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded actions.");
		}
		else if(tmpParam == "config" || tmpParam == "configuration")
		{
			g_config.reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded config.");
		}
		else if(tmpParam == "command" || tmpParam == "commands")
		{
			reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded commands.");
		}
		else if(tmpParam == "creaturescript" || tmpParam == "creaturescripts")
		{
			g_creatureEvents->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded creature scripts.");
		}
		else if(tmpParam == "globalevent" || tmpParam == "globalevents")
		{
			g_globalEvents->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded global events.");
		}
		else if(tmpParam == "highscore" || tmpParam == "highscores")
		{
			g_game.reloadHighscores();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded highscores.");
		}
		else if(tmpParam == "houseprices" || tmpParam == "houseprice")
		{
			Houses::getInstance().reloadPrices();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded house prices.");
		}
		else if(tmpParam == "monster" || tmpParam == "monsters")
		{
			g_monsters.reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded monsters.");
		}
		else if(tmpParam == "move" || tmpParam == "movement" || tmpParam == "movements"
			|| tmpParam == "moveevents" || tmpParam == "moveevent")
		{
			g_moveEvents->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded movements.");
		}
		else if(tmpParam == "npc" || tmpParam == "npcs")
		{
			g_npcs.reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded npcs.");
		}
		else if(tmpParam == "raid" || tmpParam == "raids")
		{
			Raids::getInstance()->reload();
			Raids::getInstance()->startup();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded raids.");
		}
		else if(tmpParam == "spell" || tmpParam == "spells")
		{
			g_spells->reload();
			g_monsters.reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded spells.");
		}
		else if(tmpParam == "talk" || tmpParam == "talkaction" || tmpParam == "talkactions")
		{
			g_talkActions->reload();
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reloaded talk actions.");
		}
		else
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Reload type not found.");
	}

	return true;
}

bool Commands::teleportToTown(Creature* creature, const std::string& cmd, const std::string& param)
{
	std::string tmp = param;
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	Town* town = Towns::getInstance().getTown(tmp);
	if(town)
	{
		Position oldPosition = player->getPosition();
		Position newPosition = g_game.getClosestFreeTile(player, town->getTemplePosition());
		if(player->getPosition() != town->getTemplePosition())
		{
			if(newPosition.x == 0)
				player->sendCancel("You can not teleport there.");
			else if(g_game.internalTeleport(player, newPosition, true) == RET_NOERROR)
			{
				g_game.addMagicEffect(oldPosition, NM_ME_POFF, player->isInGhostMode());
				g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, player->isInGhostMode());
				return true;
			}
		}
	}
	else
		player->sendCancel("Could not find the town.");

	return false;
}

bool Commands::teleportTo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	Creature* targetCreature = g_game.getCreatureByName(param);
	if(targetCreature)
	{
		Position oldPosition = player->getPosition();
		Position newPosition = g_game.getClosestFreeTile(player, targetCreature->getPosition());
		if(newPosition.x > 0)
		{
			if(g_game.internalTeleport(player, newPosition, true) == RET_NOERROR)
			{
				bool ghostMode = false;
				if(player->isInGhostMode() || targetCreature->isInGhostMode())
					ghostMode = true;

				g_game.addMagicEffect(oldPosition, NM_ME_POFF, ghostMode);
				g_game.addMagicEffect(player->getPosition(), NM_ME_TELEPORT, ghostMode);
				return true;
			}
		}
		else
		{
			char buffer[75];
			sprintf(buffer, "You can not teleport to %s.", targetCreature->getName().c_str());
			player->sendCancel(buffer);
		}
	}
	else
	{
		boost::char_separator<char> sep(" ");
		tokenizer cmdtokens(param, sep);
		tokenizer::iterator cmdit = cmdtokens.begin();
		std::string paramx, paramy, paramz;
		paramx = parseParams(cmdit, cmdtokens.end());
		paramy = parseParams(cmdit, cmdtokens.end());
		paramz = parseParams(cmdit, cmdtokens.end());
		trimString(paramx);
		trimString(paramy);
		trimString(paramz);
		Position destPos(atoi(paramx.c_str()), atoi(paramy.c_str()), atoi(paramz.c_str()));

		Position oldPosition = player->getPosition();
		Position newPosition = g_game.getClosestFreeTile(player, destPos);
		if(newPosition.x > 0)
		{
			if(g_game.internalTeleport(player, newPosition, true) == RET_NOERROR);
			{
				g_game.addMagicEffect(oldPosition, NM_ME_POFF, player->isInGhostMode());
				g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, player->isInGhostMode());
				return true;
			}
		}
		else
			player->sendCancel("You can not teleport there."); //since if we type bad name it will return 'Position not found' what is incorrect
	}
	return false;
}

bool Commands::sendTo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	boost::char_separator<char> sep(" ");
	tokenizer cmdtokens(param, sep);
	tokenizer::iterator cmdit = cmdtokens.begin();
	std::string paramPlayer, paramx, paramy, paramz;
	paramPlayer = parseParams(cmdit, cmdtokens.end());
	paramx = parseParams(cmdit, cmdtokens.end());
	paramy = parseParams(cmdit, cmdtokens.end());
	paramz = parseParams(cmdit, cmdtokens.end());
	trimString(paramPlayer);
	trimString(paramx);
	trimString(paramy);
	trimString(paramz);
	Position destPos(atoi(paramx.c_str()), atoi(paramy.c_str()), atoi(paramz.c_str()));

	if(Player* targetPlayer = g_game.getPlayerByName(paramPlayer))
	{
		Position oldPosition = targetPlayer->getPosition();
		Position newPosition = g_game.getClosestFreeTile(targetPlayer, destPos);
		if(newPosition.x > 0)
		{
			if(g_game.internalTeleport(targetPlayer, newPosition, true) == RET_NOERROR);
			{
				g_game.addMagicEffect(oldPosition, NM_ME_POFF, targetPlayer->isInGhostMode());
				g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, targetPlayer->isInGhostMode());
				return true;
			}
		}
		else
		{
			char buffer[100];
			sprintf(buffer, "You can not teleport %s there.", targetPlayer->getName().c_str());
			player->sendCancel(buffer);
		}
	}
	else
		player->sendCancel("Player not found.");

	return false;
}

bool Commands::getInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return true;

	Player* paramPlayer = g_game.getPlayerByName(param);
	if(paramPlayer)
	{
		if(player != paramPlayer && paramPlayer->getAccessLevel() >= player->getAccessLevel())
		{
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You can not get info about this player.");
			return true;
		}
		uint8_t ip[4];
		*(uint32_t*)&ip = paramPlayer->lastIP;
		std::stringstream info;
		info << "name:      " << paramPlayer->name << std::endl <<
			"access:    " << paramPlayer->accessLevel << std::endl <<
			"level:     " << paramPlayer->level << std::endl <<
			"maglvl:    " << paramPlayer->magLevel << std::endl <<
			"speed:     " << paramPlayer->getSpeed() <<std::endl <<
			"position:  " << paramPlayer->getPosition() << std::endl <<
			"notations: " << IOBan::getInstance()->getNotationsCount(paramPlayer->getAccount()) << std::endl <<
			"ip:        " << ipText(ip);
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, info.str().c_str());
	}
	else
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Player not found.");

	return true;
}

bool Commands::closeServer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Dispatcher::getDispatcher().addTask(
		createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_CLOSED)));

	if(creature->getPlayer())
		creature->getPlayer()->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Server is now closed.");

	return true;
}

bool Commands::openServer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Dispatcher::getDispatcher().addTask(
		createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_NORMAL)));

	if(creature->getPlayer())
		creature->getPlayer()->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Server is now open.");

	return true;
}

bool Commands::teleportNTiles(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		int32_t ntiles = atoi(param.c_str());
		if(ntiles != 0)
		{
			Position destPos = player->getPosition();
			switch(player->direction)
			{
				case NORTH:
					destPos.y -= ntiles;
					break;
				case SOUTH:
					destPos.y += ntiles;
					break;
				case EAST:
					destPos.x += ntiles;
					break;
				case WEST:
					destPos.x -= ntiles;
					break;
				default:
					break;
			}

			Position oldPosition = player->getPosition();
			Position newPosition = g_game.getClosestFreeTile(player, destPos);
			if(newPosition.x == 0)
				player->sendCancel("You can not teleport there.");
			else if(g_game.internalTeleport(player, newPosition, true) == RET_NOERROR)
			{
				if(ntiles != 1)
				{
					g_game.addMagicEffect(oldPosition, NM_ME_POFF, player->isInGhostMode());
					g_game.addMagicEffect(newPosition, NM_ME_TELEPORT, player->isInGhostMode());
				}
			}
		}
	}
	return true;
}

bool Commands::kickPlayer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* playerKick = g_game.getPlayerByName(param);
	if(playerKick)
	{
		Player* player = creature->getPlayer();
		if(player && playerKick->getAccessLevel() >= player->getAccessLevel())
		{
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You cannot kick this player.");
			return false;
		}

		playerKick->kickPlayer(true);
		return true;
	}
	return false;
}

bool Commands::setHouseOwner(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		if(player->getTile()->hasFlag(TILESTATE_HOUSE))
		{
			HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
			if(houseTile)
			{
				uint32_t guid;
				std::string name = param;
				if(name == "none")
					houseTile->getHouse()->setHouseOwner(0);
				else if(IOLoginData::getInstance()->getGuidByName(guid, name))
					houseTile->getHouse()->setHouseOwner(guid);
				else
					player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Player not found.");
				return true;
			}
		}
	}
	return false;
}

bool Commands::sellHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		if(!g_config.getBool(ConfigManager::HOUSE_BUY_AND_SELL))
		{
			player->sendCancel("House selling has been disabled by gamemaster.");
			return false;
		}

		House* house = Houses::getInstance().getHouseByPlayerId(player->guid);
		if(!house)
		{
			player->sendCancel("You do not own any house.");
			return false;
		}

		Player* tradePartner = g_game.getPlayerByName(param);
		if(!(tradePartner && tradePartner != player))
		{
			player->sendCancel("Trade player not found.");
			return false;
		}

		uint32_t levelToBuyHouse = g_config.getNumber(ConfigManager::LEVEL_TO_BUY_HOUSE);
		if(tradePartner->getLevel() < levelToBuyHouse)
		{
			char buffer[100];
			sprintf(buffer, "Trade player has to be at least Level %d to buy house.", levelToBuyHouse);
			player->sendCancel(buffer);
			return false;
		}

		if(Houses::getInstance().getHouseByPlayerId(tradePartner->getGUID()))
		{
			player->sendCancel("Trade player already owns a house.");
			return false;
		}

		uint16_t housesPerAccount = g_config.getNumber(ConfigManager::HOUSES_PER_ACCOUNT);
		if(housesPerAccount > 0 && Houses::getInstance().getHousesCount(tradePartner->getAccount()) >= housesPerAccount)
		{
			char buffer[100];
			sprintf(buffer, "Trade player has reached limit of %d house%s per account.", housesPerAccount, (housesPerAccount != 1 ? "s" : ""));
			player->sendCancel(buffer);
			return false;
		}

		if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), player->getPosition()))
		{
			player->sendCancel("Trade player is too far away.");
			return false;
		}

		if(!tradePartner->isPremium())
		{
			player->sendCancel("Trade player does not have a premium account.");
			return false;
		}

		Item* transferItem = house->getTransferItem();
		if(!transferItem)
		{
			player->sendCancel("You can not trade this house.");
			return false;
		}

		transferItem->getParent()->setParent(player);
		if(g_game.internalStartTrade(player, tradePartner, transferItem))
			return true;
		else
			house->resetTransferItem();
	}
	return false;
}

bool Commands::getHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	std::string name = param;
	uint32_t guid;
	if(IOLoginData::getInstance()->getGuidByName(guid, name))
	{
		std::stringstream str;
		str << name;

		House* house = Houses::getInstance().getHouseByPlayerId(guid);
		if(house)
			str << " owns house: " << house->getName() << ".";
		else
			str << " does not own any house.";

		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, str.str().c_str());
	}
	return false;
}

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
bool Commands::serverDiag(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	std::stringstream text;
	text << "Server diagonostic:\n";
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, text.str().c_str());

	text.str("");
	text << "World:" << "\n";
	text << "--------------------\n";
	text << "Player: " << g_game.getPlayersOnline() << " (" << Player::playerCount << ")\n";
	text << "Npc: " << g_game.getNpcsOnline() << " (" << Npc::npcCount << ")\n";
	text << "Monster: " << g_game.getMonstersOnline() << " (" << Monster::monsterCount << ")\n";
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, text.str().c_str());

	text.str("");
	text << "Protocols:" << "\n";
	text << "--------------------\n";
	text << "ProtocolGame: " << ProtocolGame::protocolGameCount << "\n";
	text << "ProtocolLogin: " << ProtocolLogin::protocolLoginCount << "\n";
	text << "ProtocolAdmin: " << ProtocolAdmin::protocolAdminCount << "\n";
	text << "ProtocolStatus: " << ProtocolStatus::protocolStatusCount << "\n\n";
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, text.str().c_str());

	text.str("");
	text << "Connections:\n";
	text << "--------------------\n";
	text << "Active connections: " << Connection::connectionCount << "\n";
	text << "Total message pool: " << OutputMessagePool::getInstance()->getTotalMessageCount() << "\n";
	text << "Auto message pool: " << OutputMessagePool::getInstance()->getAutoMessageCount() << "\n";
	text << "Free message pool: " << OutputMessagePool::getInstance()->getAvailableMessageCount() << "\n";
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, text.str().c_str());

	text.str("");
	text << "Libraries:\n";
	text << "--------------------\n";
	text << "asio: " << BOOST_ASIO_VERSION << "\n";
	text << "XML: " << XML_DEFAULT_VERSION << "\n";
	text << "Lua: " << LUA_VERSION << "\n";
	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, text.str().c_str());

	return true;
}
#endif

bool Commands::buyHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		if(!g_config.getBool(ConfigManager::HOUSE_BUY_AND_SELL))
		{
			player->sendCancel("House buying has been disabled by gamemaster.");
			return false;
		}

		if(Houses::getInstance().getHouseByPlayerId(player->getGUID()))
		{
			player->sendCancel("You are already owner of another house.");
			return false;
		}

		uint16_t housesPerAccount = g_config.getNumber(ConfigManager::HOUSES_PER_ACCOUNT);
		if(housesPerAccount > 0 && Houses::getInstance().getHousesCount(player->getAccount()) >= housesPerAccount)
		{
			char buffer[80];
			sprintf(buffer, "You may own only %d house%s per account.", housesPerAccount, (housesPerAccount != 1 ? "s" : ""));
			player->sendCancel(buffer);
			return false;
		}

		Position pos = player->getPosition();
		pos = getNextPosition(player->direction, pos);
		if(Tile* tile = g_game.getTile(pos.x, pos.y, pos.z))
		{
			if(HouseTile* houseTile = dynamic_cast<HouseTile*>(tile))
			{
				if(House* house = houseTile->getHouse())
				{
					if(house->getDoorByPosition(pos))
					{
						if(!house->getHouseOwner())
						{
							if(!g_config.getBool(ConfigManager::HOUSE_NEED_PREMIUM) || player->isPremium())
							{
								uint32_t levelToBuyHouse = g_config.getNumber(ConfigManager::LEVEL_TO_BUY_HOUSE);
								if(player->getLevel() >= levelToBuyHouse)
								{
									if(house->getPrice())
									{
										if(g_game.getMoney(player) >= house->getPrice() && g_game.removeMoney(player, house->getPrice()))
										{
											house->setHouseOwner(player->guid);
											player->sendTextMessage(MSG_INFO_DESCR, "You have successfully bought this house, remember to leave money at bank or depot of this city for rent.");
											return true;
										}
										else
											player->sendCancel("You do not have enough money.");
									}
									else
										player->sendCancel("You can not buy this house.");
								}
								else
								{
									char buffer[90];
									sprintf(buffer, "You have to be at least Level %d to buy house.", levelToBuyHouse);
									player->sendCancel(buffer);
								}
							}
							else
								player->sendCancelMessage(RET_YOUNEEDPREMIUMACCOUNT);
						}
						else
							player->sendCancel("This house alreadly has an owner.");
					}
					else
						player->sendCancel("You have to be looking at the door of the house you would like to buy.");
				}
				else
					player->sendCancel("You have to be looking at the door of the house you would like to buy.");
			}
			else
				player->sendCancel("You have to be looking at the door of the house you would like to buy.");
		}
		else
			player->sendCancel("You have to be looking at the door of the house you would like to buy.");
	}
	return false;
}

bool Commands::changeFloor(Creature* creature, const std::string &cmd, const std::string &param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return true;

	Position newPos = player->getPosition();
	if(cmd[1] == 'u')
		newPos.z--;
	else
		newPos.z++;

	Position newPosition = g_game.getClosestFreeTile(player, newPos);
	if(newPosition.x != 0)
	{
		Position oldPosition = player->getPosition();
		if(g_game.internalTeleport(player, newPosition, true) == RET_NOERROR)
		{
			g_game.addMagicEffect(oldPosition, NM_ME_POFF, player->isInGhostMode());
			g_game.addMagicEffect(player->getPosition(), NM_ME_TELEPORT, player->isInGhostMode());
			return true;
		}
	}

	player->sendCancel("You can not teleport there.");
	return false;
}

bool Commands::removeThing(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		Position pos = player->getPosition();
		pos = getNextPosition(player->direction, pos);
		Tile* removeTile = g_game.getMap()->getTile(pos);
		if(removeTile != NULL)
		{
			Thing* thing = removeTile->getTopThing();
			if(thing)
			{
				if(Creature* creature = thing->getCreature())
					g_game.removeCreature(creature, true);
				else
				{
					Item* item = thing->getItem();
					if(item && !item->isGroundTile())
					{
						g_game.internalRemoveItem(item, 1);
						g_game.addMagicEffect(pos, NM_ME_MAGIC_BLOOD);
					}
					else if(item && item->isGroundTile())
					{
						player->sendTextMessage(MSG_STATUS_SMALL, "You may not remove a ground tile.");
						g_game.addMagicEffect(pos, NM_ME_POFF);
						return false;
					}
				}
			}
			else
			{
				player->sendTextMessage(MSG_STATUS_SMALL, "No object found.");
				g_game.addMagicEffect(pos, NM_ME_POFF);
				return false;
			}
		}
		else
		{
			player->sendTextMessage(MSG_STATUS_SMALL, "No tile found.");
			g_game.addMagicEffect(pos, NM_ME_POFF);
			return false;
		}
	}
	return true;
}

bool Commands::newType(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	int32_t lookType = atoi(param.c_str());
	if(player)
	{
		if(lookType < 0 || lookType == 1 || lookType == 135 || (lookType > 160 && lookType < 192) || lookType > 301)
			player->sendTextMessage(MSG_STATUS_SMALL, "This looktype does not exist.");
		else
		{
			g_game.internalCreatureChangeOutfit(creature, (const Outfit_t&)lookType);
			return true;
		}
	}
	return false;
}

bool Commands::forceRaid(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	Raid* raid = Raids::getInstance()->getRaidByName(param);
	if(!raid || !raid->isLoaded())
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Such raid does not exists.");
		return false;
	}

	if(Raids::getInstance()->getRunning())
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Another raid is already being executed.");
		return false;
	}

	Raids::getInstance()->setRunning(raid);
	RaidEvent* event = raid->getNextRaidEvent();
	if(!event)
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "The raid does not contain any data.");
		return false;
	}

	raid->setState(RAIDSTATE_EXECUTING);

	uint32_t ticks = event->getDelay();
	if(ticks > 0)
		Scheduler::getScheduler().addEvent(createSchedulerTask(ticks,
			boost::bind(&Raid::executeRaidEvent, raid, event)));
	else
		Dispatcher::getDispatcher().addTask(createTask(
			boost::bind(&Raid::executeRaidEvent, raid, event)));

	player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Raid started.");
	return true;
}

bool Commands::addSkill(Creature* creature, const std::string& cmd, const std::string& param)
{
	boost::char_separator<char> sep(",");
	tokenizer cmdtokens(param, sep);
	tokenizer::iterator cmdit = cmdtokens.begin();
	std::string param1, param2;
	param1 = parseParams(cmdit, cmdtokens.end());
	param2 = parseParams(cmdit, cmdtokens.end());
	trimString(param1);
	trimString(param2);
	Player* player = creature->getPlayer();
	if(player)
	{
		Player* paramPlayer = g_game.getPlayerByName(param1);
		if(paramPlayer)
		{
			if(param2[0] == 'l' || param2[0] == 'e')
				paramPlayer->addExperience(Player::getExpForLevel(paramPlayer->getLevel() + 1) - paramPlayer->experience);
			else if(param2[0] == 'm')
				paramPlayer->addManaSpent(player->vocation->getReqMana(paramPlayer->getMagicLevel() + 1) - paramPlayer->manaSpent);
			else
				paramPlayer->addSkillAdvance(getSkillId(param2), paramPlayer->vocation->getReqSkillTries(getSkillId(param2), paramPlayer->getSkill(getSkillId(param2), SKILL_LEVEL) + 1));

			return true;
		}
		else
			player->sendTextMessage(MSG_STATUS_SMALL, "Could not find target.");
	}
	return false;
}

bool Commands::joinGuild(Creature* creature, const std::string& cmd, const std::string& param)
{
	if(!g_config.getBool(ConfigManager::INGAME_GUILD_MANAGEMENT))
		return false;

	Player* player = creature->getPlayer();
	if(player)
	{
		if(player->guildId == 0)
		{
			trimString((std::string&)param);
			uint32_t guildId;
			if(IOGuild::getInstance()->getGuildIdByName(guildId, param))
			{
				if(player->isInvitedToGuild(guildId))
				{
					player->sendTextMessage(MSG_INFO_DESCR, "You have joined the guild.");
					IOGuild::getInstance()->joinGuild(player, guildId);

					char buffer[80];
					sprintf(buffer, "%s has joined the guild.", player->name.c_str());

					ChatChannel* guildChannel = g_chat.getChannel(player, 0x00);
					if(guildChannel)
						guildChannel->talk(player, SPEAK_CHANNEL_R2, buffer);

					return true;
				}
				else
					player->sendCancel("You are not invited to that guild.");
			}
			else
				player->sendCancel("There's no guild with that name.");
		}
		else
			player->sendCancel("You are already in a guild.");
	}
	return false;
}

bool Commands::createGuild(Creature* creature, const std::string& cmd, const std::string& param)
{
	if(!g_config.getBool(ConfigManager::INGAME_GUILD_MANAGEMENT))
		return false;

	Player* player = creature->getPlayer();
	if(player)
	{
		if(player->guildId == 0)
		{
			trimString((std::string&)param);
			const uint32_t minLength = g_config.getNumber(ConfigManager::MIN_GUILDNAME);
			const uint32_t maxLength = g_config.getNumber(ConfigManager::MAX_GUILDNAME);
			if(param.length() >= minLength)
			{
				if(param.length() <= maxLength)
				{
					uint32_t guildId;
					if(!IOGuild::getInstance()->getGuildIdByName(guildId, param))
					{
						const uint32_t levelToFormGuild = g_config.getNumber(ConfigManager::LEVEL_TO_FORM_GUILD);
						if(player->getLevel() >= levelToFormGuild)
						{
							if(player->isPremium())
							{
								char buffer[50 + maxLength];
								sprintf(buffer, "You have formed the guild: %s!", param.c_str());
								player->sendTextMessage(MSG_INFO_DESCR, buffer);
								player->setGuildName(param);

								IOGuild::getInstance()->createGuild(player);
								return true;
							}
							else
								player->sendCancelMessage(RET_YOUNEEDPREMIUMACCOUNT);
						}
						else
						{
							char buffer[70 + levelToFormGuild];
							sprintf(buffer, "You have to be at least Level %d to form a guild.", levelToFormGuild);
							player->sendCancel(buffer);
						}
					}
					else
						player->sendCancel("There is already a guild with that name.");
				}
				else
					player->sendCancel("That guild name is too long, please select a shorter name.");
			}
			else
				player->sendCancel("That guild name is too short, please select a longer name.");
		}
		else
			player->sendCancel("You are already in a guild.");
	}
	return false;
}

bool Commands::unban(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		uint32_t accountNumber = atoi(param.c_str());
		bool removedIpBan = false;
		if(accountNumber == 0 && IOLoginData::getInstance()->playerExists(param))
		{
			accountNumber = IOLoginData::getInstance()->getAccountNumberByName(param);
			uint32_t lastip = IOLoginData::getInstance()->getLastIPByName(param);
			if(lastip != 0 && IOBan::getInstance()->isIpBanished(lastip))
				removedIpBan = IOBan::getInstance()->removeIpBanishment(lastip);
		}

		if(IOBan::getInstance()->isBanished(accountNumber))
		{
			IOBan::getInstance()->removeBanishment(accountNumber);

			char buffer[80];
			sprintf(buffer, "%s has been unbanned.", param.c_str());
			player->sendTextMessage(MSG_INFO_DESCR, buffer);
		}
		else if(IOBan::getInstance()->isDeleted(accountNumber))
		{
			IOBan::getInstance()->removeDeletion(accountNumber);

			char buffer[80];
			sprintf(buffer, "%s has been undeleted.", param.c_str());
			player->sendTextMessage(MSG_INFO_DESCR, buffer);
		}
		else if(!removedIpBan)
		{
			player->sendCancel("That player or account is not banished or deleted.");
			return false;
		}

		if(removedIpBan)
		{
			char buffer[80];
			sprintf(buffer, "IPBan on %s has been lifted.", param.c_str());
			player->sendTextMessage(MSG_INFO_DESCR, buffer);
		}
	}
	return true;
}

bool Commands::playerFrags(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		int32_t fragTime = g_config.getNumber(ConfigManager::FRAG_TIME);
		if(player->redSkullTicks && fragTime > 0)
		{
			int32_t frags = (player->redSkullTicks / fragTime) + 1;
			int32_t remainingTime = player->redSkullTicks - (fragTime * (frags - 1));
			int32_t hours = ((remainingTime / 1000) / 60) / 60;
			int32_t minutes = ((remainingTime / 1000) / 60) - (hours * 60);

			char buffer[175];
			sprintf(buffer, "You have %d unjustified frag%s. The amount of unjustified frags will decrease after: %s.", frags, (frags > 1 ? "s" : ""), formatTime(hours, minutes).c_str());
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, buffer);
		}
		else
			player->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "You do not have any unjustified frag.");
	}
	return false;
}

bool Commands::ghost(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(!player)
		return false;

	player->switchGhostMode();
	Player* tmpPlayer;

	SpectatorVec list;
	g_game.getSpectators(list, player->getPosition(), true);
	SpectatorVec::const_iterator it;

	Cylinder* cylinder = player->getTopParent();
	int32_t index = cylinder->__getIndexOfThing(creature);

	for(it = list.begin(); it != list.end(); ++it)
	{
		if((tmpPlayer = (*it)->getPlayer()))
		{
			tmpPlayer->sendCreatureChangeVisible(player, !player->isInGhostMode());
			if(tmpPlayer != player && !tmpPlayer->canSeeGhost(player))
			{
				if(player->isInGhostMode())
					tmpPlayer->sendCreatureDisappear(player, index, true);
				else
					tmpPlayer->sendCreatureAppear(player, true);

				tmpPlayer->sendUpdateTile(player->getTile(), player->getPosition());
			}
		}
	}

	for(it = list.begin(); it != list.end(); ++it)
		(*it)->onUpdateTile(player->getTile(), player->getPosition());

	if(player->isInGhostMode())
	{
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
		{
			if(!it->second->canSeeGhost(player))
				it->second->notifyLogOut(player);
		}

		IOLoginData::getInstance()->updateOnlineStatus(player->getGUID(), false);
		player->sendTextMessage(MSG_INFO_DESCR, "You are now invisible.");
	}
	else
	{
		for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
		{
			if(!it->second->canSeeGhost(player))
				it->second->notifyLogIn(player);
		}

		IOLoginData::getInstance()->updateOnlineStatus(player->getGUID(), true);
		player->sendTextMessage(MSG_INFO_DESCR, "You are visible again.");
	}
	return true;
}

bool Commands::squelch(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		player->switchPrivMsgIgnore();
		char buffer[90];
		sprintf(buffer, "You have %s private messages ignoring.", (player->isIgnoringPrivMsg() ? "enabled" : "disabled"));
		player->sendTextMessage(MSG_INFO_DESCR, buffer);
		return true;
	}

	return false;
}

bool Commands::changeThingProporties(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		Position playerPos = player->getPosition();
		Position pos = getNextPosition(player->direction, playerPos);
		Tile* tileInFront = g_game.getMap()->getTile(pos);
		if(tileInFront != NULL)
		{
			Thing* thing = tileInFront->getTopThing();
			if(thing)
			{
				boost::char_separator<char> sep(" ");
				tokenizer cmdtokens(param, sep);
				tokenizer::iterator cmdit = cmdtokens.begin();
				std::string param;
				while(cmdit != cmdtokens.end())
				{
					if(Item *item = thing->getItem())
					{
						param = parseParams(cmdit, cmdtokens.end());
						if(strcasecmp(param.c_str(), "description") == 0)
								item->setSpecialDescription(parseParams(cmdit, cmdtokens.end()));
						else if(strcasecmp(param.c_str(), "count") == 0 || strcasecmp(param.c_str(), "fluidtype") == 0 || strcasecmp(param.c_str(), "charges") == 0)
								item->setSubType(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "action") == 0 || strcasecmp(param.c_str(), "actionid") == 0 || strcasecmp(param.c_str(), "aid") == 0)
								item->setActionId(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "unique") == 0 || strcasecmp(param.c_str(), "uniqueid") == 0 || strcasecmp(param.c_str(), "uid") == 0)
								item->setUniqueId(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "duration") == 0)
								item->setDuration(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "writer") == 0)
								item->setWriter(parseParams(cmdit, cmdtokens.end()));
						else if(strcasecmp(param.c_str(), "text") == 0)
								item->setText(parseParams(cmdit, cmdtokens.end()));
						else if(strcasecmp(param.c_str(), "name") == 0)
								item->setName(parseParams(cmdit, cmdtokens.end()));
						else if(strcasecmp(param.c_str(), "pluralname") == 0)
								item->setPluralName(parseParams(cmdit, cmdtokens.end()));
						else if(strcasecmp(param.c_str(), "article") == 0)
								item->setArticle(parseParams(cmdit, cmdtokens.end()));
						else if(strcasecmp(param.c_str(), "attack") == 0)
								item->setAttack(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "extraattack") == 0)
								item->setExtraAttack(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "defense") == 0)
								item->setDefense(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "extradefense") == 0)
								item->setExtraDefense(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "armor") == 0)
								item->setArmor(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "attackspeed") == 0)
								item->setAttackSpeed(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "hitchance") == 0)
								item->setHitChance(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "depot") == 0 || strcasecmp(param.c_str(), "depotid") == 0)
						{
							if(item->getContainer())
								if(item->getContainer()->getDepot())
									item->getContainer()->getDepot()->setDepotId(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						}
						else if(strcasecmp(param.c_str(), "destination") == 0 || strcasecmp(param.c_str(), "position") == 0 || strcasecmp(param.c_str(), "pos") == 0) //FIXME
						{
							if(item->getTeleport())
								item->getTeleport()->setDestPos(Position(atoi(parseParams(cmdit, cmdtokens.end()).c_str()), atoi(parseParams(cmdit, cmdtokens.end()).c_str()), atoi(parseParams(cmdit, cmdtokens.end()).c_str())));
						}
						else
						{
							player->sendTextMessage(MSG_STATUS_SMALL, "No valid action.");
							g_game.addMagicEffect(playerPos, NM_ME_POFF);
							return false;
						}
					}
					else if(Creature* creature = thing->getCreature())
					{
						param = parseParams(cmdit, cmdtokens.end());
						if(strcasecmp(param.c_str(), "health") == 0)
							creature->changeHealth(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "maxhealth") == 0)
							creature->changeMaxHealth(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "mana") == 0)
							creature->changeMana(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "maxmana") == 0)
							creature->changeMaxMana(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "basespeed") == 0)
							creature->setBaseSpeed(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "droploot") == 0)
							creature->setDropLoot(booleanString(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "lossskill") == 0)
							creature->setLossSkill(booleanString(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(Player* player = creature->getPlayer())
						{
							if(strcasecmp(param.c_str(), "fyi") == 0)
								player->sendFYIBox(parseParams(cmdit, cmdtokens.end()).c_str());
							else if(strcasecmp(param.c_str(), "guildrank") == 0)
								player->setGuildRankId(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "guildnick") == 0)
								player->setGuildNick(parseParams(cmdit, cmdtokens.end()).c_str());
							else if(strcasecmp(param.c_str(), "group") == 0)
								player->setGroupId(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "extrarate") == 0)
								player->setExtraExpRate(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "vocation") == 0)
								player->setVocation(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "sex") == 0)
								player->setSex((PlayerSex_t)atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "stamina") == 0)
								player->setStaminaMinutes(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "town") == 0) //FIXME
								player->setTown(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "skull") == 0)
								player->setSkull((Skulls_t)atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "balance") == 0)
								player->balance = atoi(parseParams(cmdit, cmdtokens.end()).c_str());
							else if(strcasecmp(param.c_str(), "marriage") == 0)
								player->marriage = atoi(parseParams(cmdit, cmdtokens.end()).c_str());
							else if(strcasecmp(param.c_str(), "resetidle") == 0)
								player->resetIdleTime();
							else if(strcasecmp(param.c_str(), "ghost") == 0)
								player->switchGhostMode();
							else if(strcasecmp(param.c_str(), "squelch") == 0)
								player->switchPrivMsgIgnore();
							else
							{
								player->sendTextMessage(MSG_STATUS_SMALL, "No valid action.");
								g_game.addMagicEffect(playerPos, NM_ME_POFF);
								return false;
							}
						}
						/*else if(Npc* npc = creature->getNpc())
							//
						else if(Monster* monster = creature->getMonster())
							//*/
						else
						{
							player->sendTextMessage(MSG_STATUS_SMALL, "No valid action.");
							g_game.addMagicEffect(playerPos, NM_ME_POFF);
							return false;
						}
					}
				}
			}
			else
			{
				player->sendTextMessage(MSG_STATUS_SMALL, "No object found.");
				g_game.addMagicEffect(playerPos, NM_ME_POFF);
				return false;
			}

			const Position& cylinderMapPos = tileInFront->getPosition();
			const SpectatorVec& list = g_game.getSpectators(cylinderMapPos);
			SpectatorVec::const_iterator it;

			Player* tmpPlayer = NULL;
			for(it = list.begin(); it != list.end(); ++it)
			{
				if((tmpPlayer = (*it)->getPlayer()))
					tmpPlayer->sendUpdateTile(tileInFront, cylinderMapPos);
			}

			for(it = list.begin(); it != list.end(); ++it)
				(*it)->onUpdateTile(tileInFront, cylinderMapPos);

			g_game.addMagicEffect(pos, NM_ME_MAGIC_POISON);
		}
		else
		{
			player->sendTextMessage(MSG_STATUS_SMALL, "No tile found.");
			g_game.addMagicEffect(playerPos, NM_ME_POFF);
		}
	}

	return true;
}

bool Commands::showBanishmentInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		uint32_t accountNumber = atoi(param.c_str());
		if(accountNumber == 0 && IOLoginData::getInstance()->playerExists(param))
			accountNumber = IOLoginData::getInstance()->getAccountNumberByName(param);

		Ban ban;
		if(IOBan::getInstance()->getBanishmentData(accountNumber, ban))
		{
			bool deletion = (ban.type == BANTYPE_DELETION);

			std::string name_;
			if(ban.adminid == 0)
				name_ = (deletion ? "Automatic deletion" : "Automatic banishment");
			else
				IOLoginData::getInstance()->getNameByGuid(ban.adminid, name_);

			char date[16], date2[16];
			formatDate2(ban.added, date);
			formatDate2(ban.expires, date2);

			char buffer[500 + ban.comment.length()];
			sprintf(buffer, "Your account has been %s at:\n%s by: %s,\nfor the following reason:\n%s.\nThe action taken was:\n%s.\nThe comment given was:\n%s.\nYour %s%s.",
				(deletion ? "deleted" : "banished"), date, name_.c_str(), getReason(ban.reason).c_str(), getAction(ban.action, false).c_str(),
				ban.comment.c_str(), (deletion ? "account won't be undeleted" : "banishment will be lifted at:\n"),	(deletion ? "." : date));

			player->sendFYIBox(buffer);
		}
		else
		{
			player->sendCancel("That player or account is not banished or deleted.");
			return false;
		}
	}

	return true;
}
