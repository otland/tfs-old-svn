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


#ifndef __OTSERV_COMMANDS_H__
#define __OTSERV_COMMANDS_H__

#include <string>
#include <map>
#include "creature.h"

struct Command;
struct s_defcommands;

class Commands
{
	public:
		Commands();

		bool loadFromXml();
		bool reload();

		bool exeCommand(Creature* creature, const std::string& cmd);

		static ReturnValue placeSummon(Creature* creature, const std::string& name);

	protected:
		bool loaded;

		//commands
		bool placeNpc(Creature* creature, const std::string& cmd, const std::string& param);
		bool placeMonster(Creature* creature, const std::string& cmd, const std::string& param);
		bool placeSummon(Creature* creature, const std::string& cmd, const std::string& param);
		bool teleportMasterPos(Creature* creature, const std::string& cmd, const std::string& param);
		bool teleportHere(Creature* creature, const std::string& cmd, const std::string& param);
		bool teleportToTown(Creature* creature, const std::string& cmd, const std::string& param);
		bool teleportTo(Creature* creature, const std::string& cmd, const std::string& param);
		bool sendTo(Creature* creature, const std::string& cmd, const std::string& param);
		bool createItemById(Creature* creature, const std::string& cmd, const std::string& param);
		bool createItemByName(Creature* creature, const std::string& cmd, const std::string& param);
		bool reloadInfo(Creature* creature, const std::string& cmd, const std::string& param);
		bool getInfo(Creature* creature, const std::string& cmd, const std::string& param);
		bool closeServer(Creature* creature, const std::string& cmd, const std::string& param);
		bool openServer(Creature* creature, const std::string& cmd, const std::string& param);
		bool teleportNTiles(Creature* creature, const std::string& cmd, const std::string& param);
		bool kickPlayer(Creature* creature, const std::string& cmd, const std::string& param);
		bool setHouseOwner(Creature* creature, const std::string& cmd, const std::string& param);
		bool sellHouse(Creature* creature, const std::string& cmd, const std::string& param);
		bool getHouse(Creature* creature, const std::string& cmd, const std::string& param);
		bool serverDiag(Creature* creature, const std::string& cmd, const std::string& param);
		bool changeFloor(Creature* creature, const std::string &cmd, const std::string &param);
		bool removeThing(Creature* creature, const std::string& cmd, const std::string& param);
		bool buyHouse(Creature* creature, const std::string& cmd, const std::string& param);
		bool newType(Creature* creature, const std::string& cmd, const std::string& param);
		bool forceRaid(Creature* creature, const std::string& cmd, const std::string& param);
		bool addSkill(Creature* creature, const std::string& cmd, const std::string& param);
		bool playerFrags(Creature* creature, const std::string& cmd, const std::string& param);
		bool unban(Creature* creature, const std::string& cmd, const std::string& param);
		bool joinGuild(Creature* creature, const std::string& cmd, const std::string& param);
		bool createGuild(Creature* creature, const std::string& cmd, const std::string& param);
		bool ghost(Creature* creature, const std::string& cmd, const std::string& param);
		bool squelch(Creature* creature, const std::string& cmd, const std::string& param);
		bool changeThingProporties(Creature* creature, const std::string& cmd, const std::string& param);
		bool showBanishmentInfo(Creature* creature, const std::string& cmd, const std::string& param);

		//table of commands
		static s_defcommands defined_commands[];

		typedef std::map<std::string, Command*> CommandMap;
		CommandMap commandMap;
};

typedef bool (Commands::*CommandFunc)(Creature*, const std::string&, const std::string&);

struct Command
{
	CommandFunc f;
	uint32_t accessLevel;
	bool loadedAccess;
	bool logged;
	bool loadedLogging;
};

struct s_defcommands
{
	const char* name;
	CommandFunc f;
};

#endif
