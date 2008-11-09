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
#include "protocolold.h"
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

TalkActions::TalkActions() :
m_scriptInterface("TalkAction Interface")
{
	m_scriptInterface.initState();
}

TalkActions::~TalkActions()
{
	clear();
}

void TalkActions::clear()
{
	TalkActionsMap::iterator it = talksMap.begin();
	while(it != talksMap.end())
	{
		delete it->second;
		talksMap.erase(it);
		it = talksMap.begin();
	}

	m_scriptInterface.reInitState();
}

LuaScriptInterface& TalkActions::getScriptInterface()
{
	return m_scriptInterface;
}

std::string TalkActions::getScriptBaseName()
{
	return "talkactions";
}

Event* TalkActions::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "talkaction")
		return new TalkAction(&m_scriptInterface);
	else
		return NULL;
}

bool TalkActions::registerEvent(Event* event, xmlNodePtr p)
{
	TalkAction* talkAction = dynamic_cast<TalkAction*>(event);
	if(!talkAction)
		return false;

	talksMap[talkAction->getWords()] = talkAction;
	return true;
}

bool TalkActions::onPlayerSay(Player* player, uint16_t channelId, const std::string& words)
{
	std::string cmdstring[TALKFILTER_LAST] = words, paramstring[TALKFILTER_LAST] = "";
	size_t loc = words.find('"', 0);
	if(loc != std::string::npos && loc >= 0)
	{
		cmdstring[TALKFILTER_QUOTATION] = std::string(words, 0, loc);
		paramstring[TALKFILTER_QUOTATION] = std::string(words, (loc + 1), (words.size() - loc - 1));
		trimString(cmdstring[TALKFILTER_QUOTATION]);
	}

	loc = words.find(" ", 0);
	if(loc != std::string::npos && loc >= 0)
	{
		cmdstring[TALKFILTER_WORD] = std::string(words, 0, loc);
		paramstring[TALKFILTER_WORD] = std::string(words, (loc + 1), (words.size() - loc - 1));
	}

	TalkAction* talkAction = NULL;
	for(TalkActionsMap::iterator it = talksMap.begin(); it != talksMap.end(); ++it)
	{
		if(it->first == cmdstring[it->second->getFilter()] || (!it->second->isSensitive() && 
			strcasecmp(it->first.c_str(), cmdstring[it->second->getFilter()].c_str()) == 0))
		{
			talkAction = it->second;
			break;
		}
	}

	if(!talkAction || (talkAction->getChannel() != -1 && talkAction->getChannel() != channelId))
		return false;

	if(talkAction->getAccess() > player->getAccessLevel() || player->isAccountManager())
	{
		if(player->hasCustomFlag(PlayerCustomFlag_GamemasterPrivileges))
		{
			player->sendTextMessage(MSG_STATUS_SMALL, "You cannot execute this talkaction.");
			return true;
		}

		return false;
	}

	if(talkAction->isLogged())
	{
		player->sendTextMessage(MSG_STATUS_CONSOLE_RED, words.c_str());

		char buf[21], buffer[100];
		formatDate(time(NULL), buf);
		sprintf(buffer, "%s.log", getFilePath(FILE_TYPE_LOG, player->getName()).c_str());

		if(FILE* file = fopen(buffer, "a"))
		{
			fprintf(file, "[%s] %s\n", buf, words.c_str());
			fclose(file);
		}
	}

	if(talkAction->isScripted())
		talkAction->executeSay(player, cmdstring[talkAction->getFilter()], paramstring[talkAction->getFilter()]);
	else if(talkAction->callback)
		talkAction->callback(player, cmdstring[talkAction->getFilter()], paramstring[talkAction->getFilter()]);

	return true;
}

TalkActionCallback_t TalkAction::definedCallbacks[] =
{
	{"placenpc", &placeNpc},
	{"placemonster", &placeMonster},
	{"placesummon", &placeSummon},
	{"reloadinfo", &reloadInfo},
	{"closeserver", &closeServer},
	{"openserver", &openServer},
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	{"serverdiag",&serverDiag},
#endif
	{"changethingproporties", &changeThingProporties},
	{"addskill", &addSkill},
	{"showbanishmentinfo", &showBanishmentInfo},
	{"buyhouse", &buyHouse},
 	{"sellhouse", &sellHouse},
 	{"joinguild", &joinGuild},
 	{"createguild", &createGuild},
	{"ghost", &ghost},
	{"squelch", &squelch},
	{"mapteleport", &mapTeleport}
};

TalkAction::TalkAction(LuaScriptInterface* _interface) : Event(_interface)
{
	m_filter = TALKFILTER_QUOTATION;
	m_access = 0;
	m_channel = -1;
	m_logged = false;
	m_sensitive = true;
}

bool TalkAction::configureEvent(xmlNodePtr p)
{
	std::string strValue;
	if(readXMLString(p, "words", strValue))
		m_words = strValue;
	else
	{
		std::cout << "[Error - TalkAction::configureEvent] No words for TalkAction." << std::endl;
		return false;
	}

	if(readXMLString(p, "filter", strValue))
	{
		strValue = asLowerCaseString(strValue);
		if(strValue == "quotation")
			m_filter = TALKFILTER_QUOTATION;
		else if(strValue == "word")
			m_filter = TALKFILTER_WORD;
	}

	int32_t intValue;
	if(readXMLInteger(p, "access", intValue))
		m_access = intValue;

	if(readXMLInteger(p, "channel", intValue))
		m_channel = intValue;

	if(readXMLString(p, "log", strValue))
		m_logged = booleanString(asLowerCaseString(strValue));

	if(readXMLString(p, "case-sensitive", strValue))
		m_sensitive = booleanString(asLowerCaseString(strValue));

	return true;
}

bool TalkAction::loadFunction(const std::string& functionName)
{
	std::string tmpFunctionName = asLowerCaseString(functionName);
	for(uint32_t i = 0; i < sizeof(definedCallbacks) / sizeof(definedCallbacks[0]); i++)
	{
		if(tmpFunctionName == definedCallbacks[i].name)
		{
			callback = definedCallbacks[i].callback;
			m_scripted = false;
			return true;
		}
	}

	std::cout << "[Warning - TalkAction::loadFunction] Function \"" << functionName << "\" does not exist." << std::endl;
	return false;
}

bool TalkAction::placeNpc(Creature* creature, const std::string& cmd, const std::string& param)
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
		if(Player* player = creature->getPlayer())
		{
			player->sendCancelMessage(RET_NOTENOUGHROOM);
			g_game.addMagicEffect(creature->getPosition(), NM_ME_POFF);
		}
		return true;
	}
	return false;
}

bool TalkAction::placeMonster(Creature* creature, const std::string& cmd, const std::string& param)
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

bool TalkAction::placeSummon(Creature* creature, const std::string& cmd, const std::string& param)
{
	ReturnValue ret = g_game.placeSummon(creature, param);
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

bool TalkAction::reloadInfo(Creature* creature, const std::string& cmd, const std::string& param)
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

bool TalkAction::closeServer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Dispatcher::getDispatcher().addTask(
		createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_CLOSED)));

	if(creature->getPlayer())
		creature->getPlayer()->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Server is now closed.");

	return true;
}

bool TalkAction::openServer(Creature* creature, const std::string& cmd, const std::string& param)
{
	Dispatcher::getDispatcher().addTask(
		createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_NORMAL)));

	if(creature->getPlayer())
		creature->getPlayer()->sendTextMessage(MSG_STATUS_CONSOLE_BLUE, "Server is now open.");

	return true;
}

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
bool TalkAction::serverDiag(Creature* creature, const std::string& cmd, const std::string& param)
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
	text << "ProtocolStatus: " << ProtocolStatus::protocolStatusCount << "\n";
	text << "ProtocolOld: " << ProtocolOld::protocolOldCount << "\n\n";
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

bool TalkAction::changeThingProporties(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		Position playerPos = player->getPosition();
		Position pos = getNextPosition(player->getDirection(), playerPos);
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
					else if(Creature* _creature = thing->getCreature())
					{
						param = parseParams(cmdit, cmdtokens.end());
						if(strcasecmp(param.c_str(), "health") == 0)
							_creature->changeHealth(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "maxhealth") == 0)
							_creature->changeMaxHealth(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "mana") == 0)
							_creature->changeMana(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "maxmana") == 0)
							_creature->changeMaxMana(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "basespeed") == 0)
							_creature->setBaseSpeed(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "droploot") == 0)
							_creature->setDropLoot(booleanString(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(strcasecmp(param.c_str(), "lossskill") == 0)
							_creature->setLossSkill(booleanString(parseParams(cmdit, cmdtokens.end()).c_str()));
						else if(Player* _player = _creature->getPlayer())
						{
							if(strcasecmp(param.c_str(), "fyi") == 0)
								_player->sendFYIBox(parseParams(cmdit, cmdtokens.end()).c_str());
							else if(strcasecmp(param.c_str(), "guildrank") == 0)
								_player->setGuildRankId(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "guildnick") == 0)
								_player->setGuildNick(parseParams(cmdit, cmdtokens.end()).c_str());
							else if(strcasecmp(param.c_str(), "group") == 0)
								_player->setGroupId(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "extrarate") == 0)
								_player->setExtraExpRate(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "vocation") == 0)
								_player->setVocation(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "sex") == 0)
								_player->setSex((PlayerSex_t)atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "stamina") == 0)
								_player->setStaminaMinutes(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "town") == 0) //FIXME
								_player->setTown(atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "skull") == 0)
								_player->setSkull((Skulls_t)atoi(parseParams(cmdit, cmdtokens.end()).c_str()));
							else if(strcasecmp(param.c_str(), "balance") == 0)
								_player->balance = atoi(parseParams(cmdit, cmdtokens.end()).c_str());
							else if(strcasecmp(param.c_str(), "marriage") == 0)
								_player->marriage = atoi(parseParams(cmdit, cmdtokens.end()).c_str());
							else if(strcasecmp(param.c_str(), "resetidle") == 0)
								_player->resetIdleTime();
							else if(strcasecmp(param.c_str(), "ghost") == 0)
								_player->switchGhostMode();
							else if(strcasecmp(param.c_str(), "squelch") == 0)
								_player->switchPrivMsgIgnore();
							else
							{
								player->sendTextMessage(MSG_STATUS_SMALL, "No valid action.");
								g_game.addMagicEffect(playerPos, NM_ME_POFF);
								return false;
							}
						}
						/*else if(Npc* npc = _creature->getNpc())
							//
						else if(Monster* monster = _creature->getMonster())
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

bool TalkAction::addSkill(Creature* creature, const std::string& cmd, const std::string& param)
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
				paramPlayer->addExperience(Player::getExpForLevel(paramPlayer->getLevel() + 1) - paramPlayer->getExperience());
			else if(param2[0] == 'm')
				paramPlayer->addManaSpent(player->getVocation()->getReqMana(paramPlayer->getMagicLevel() + 1) - paramPlayer->getSpentMana());
			else
				paramPlayer->addSkillAdvance(getSkillId(param2), paramPlayer->getVocation()->getReqSkillTries(getSkillId(param2), paramPlayer->getSkill(getSkillId(param2), SKILL_LEVEL) + 1));

			return true;
		}
		else
			player->sendTextMessage(MSG_STATUS_SMALL, "Could not find target.");
	}
	return false;
}

bool TalkAction::showBanishmentInfo(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		uint32_t accountNumber = atoi(param.c_str());
		if(accountNumber == 0 && IOLoginData::getInstance()->playerExists(param))
			accountNumber = IOLoginData::getInstance()->getAccountIdByName(param);

		Ban ban;
		if(IOBan::getInstance()->getData(accountNumber, ban))
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
			sprintf(buffer, "Account has been %s at:\n%s by: %s,\nfor the following reason:\n%s.\nThe action taken was:\n%s.\nThe comment given was:\n%s.\n%s%s.",
				(deletion ? "deleted" : "banished"), date, name_.c_str(), getReason(ban.reason).c_str(), getAction(ban.action, false).c_str(),
				ban.comment.c_str(), (deletion ? "Account won't be undeleted" : "Banishment will be lifted at:\n"), (deletion ? "." : date));

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

bool TalkAction::buyHouse(Creature* creature, const std::string& cmd, const std::string& param)
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
		pos = getNextPosition(player->getDirection(), pos);
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
											house->setHouseOwner(player->getGUID());
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

bool TalkAction::sellHouse(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		if(!g_config.getBool(ConfigManager::HOUSE_BUY_AND_SELL))
		{
			player->sendCancel("House selling has been disabled by gamemaster.");
			return false;
		}

		House* house = Houses::getInstance().getHouseByPlayerId(player->getGUID());
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

bool TalkAction::joinGuild(Creature* creature, const std::string& cmd, const std::string& param)
{
	if(!g_config.getBool(ConfigManager::INGAME_GUILD_MANAGEMENT))
		return false;

	Player* player = creature->getPlayer();
	if(player)
	{
		if(player->getGuildId() == 0)
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
					sprintf(buffer, "%s has joined the guild.", player->getName().c_str());

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

bool TalkAction::createGuild(Creature* creature, const std::string& cmd, const std::string& param)
{
	if(!g_config.getBool(ConfigManager::INGAME_GUILD_MANAGEMENT))
		return false;

	Player* player = creature->getPlayer();
	if(player)
	{
		if(player->getGuildId() == 0)
		{
			trimString((std::string&)param);
			if(isValidName(param))
			{
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
				player->sendCancel("That guild name contains illegal characters, please choose another name.");
		}
		else
			player->sendCancel("You are already in a guild.");
	}
	return false;
}

bool TalkAction::ghost(Creature* creature, const std::string& cmd, const std::string& param)
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

bool TalkAction::squelch(Creature* creature, const std::string& cmd, const std::string& param)
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

bool TalkAction::mapTeleport(Creature* creature, const std::string& cmd, const std::string& param)
{
	Player* player = creature->getPlayer();
	if(player)
	{
		player->switchTeleportByMap();
		char buffer[90];
		sprintf(buffer, "You have %s map click teleporting.", (player->isTeleportingByMap() ? "enabled" : "disabled"));
		player->sendTextMessage(MSG_INFO_DESCR, buffer);
		return true;
	}

	return false;
}

std::string TalkAction::getScriptEventName()
{
	return "onSay";
}

int32_t TalkAction::executeSay(Creature* creature, const std::string& words, const std::string& param)
{
	//onSay(cid, words, param)
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		char desc[125];
		sprintf(desc, "%s - %s- %s", creature->getName().c_str(), words.c_str(), param.c_str());
		env->setEventDesc(desc);
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushstring(L, words.c_str());
		lua_pushstring(L, param.c_str());

		int32_t result = m_scriptInterface->callFunction(3);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else
	{
		std::cout << "[Error] Call stack overflow. TalkAction::executeSay" << std::endl;
		return 0;
	}
}
