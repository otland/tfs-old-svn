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
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "scriptmanager.h"

#include "actions.h"
#include "talkaction.h"
#include "spells.h"
#include "movement.h"
#include "weapons.h"
#include "creatureevent.h"
#include "globalevent.h"

#include "luascript.h"
#ifndef __CONSOLE__
#include "gui.h"
#endif

Actions* g_actions = NULL;
CreatureEvents* g_creatureEvents = NULL;
Spells* g_spells = NULL;
TalkActions* g_talkActions = NULL;
MoveEvents* g_moveEvents = NULL;
Weapons* g_weapons = NULL;
GlobalEvents* g_globalEvents = NULL;

bool ScriptingManager::loadScriptSystems()
{
	g_weapons = new Weapons();
	if(!g_weapons->loadFromXml())
	{
		std::cout << "> ERROR: Unable to load Weapons!" << std::endl;
		return false;
	}

	g_weapons->loadDefaults();
	g_spells = new Spells();
	if(!g_spells->loadFromXml())
	{
		std::cout << "> ERROR: Unable to load Spells!" << std::endl;
		return false;
	}

	g_actions = new Actions();
	if(!g_actions->loadFromXml())
	{
		std::cout << "> ERROR: Unable to load Actions!" << std::endl;
		return false;
	}

	g_talkActions = new TalkActions();
	if(!g_talkActions->loadFromXml())
	{
		std::cout << "> ERROR: Unable to load TalkActions!" << std::endl;
		return false;
	}

	g_moveEvents = new MoveEvents();
	if(!g_moveEvents->loadFromXml())
	{
		std::cout << "> ERROR: Unable to load MoveEvents!" << std::endl;
		return false;
	}

	g_creatureEvents = new CreatureEvents();
	if(!g_creatureEvents->loadFromXml())
	{
		std::cout << "> ERROR: Unable to load CreatureEvents!" << std::endl;
		return false;
	}

	g_globalEvents = new GlobalEvents();
	if(!g_globalEvents->loadFromXml())
	{
		std::cout << "> ERROR: Unable to load GlobalEvents!" << std::endl;
		return false;
	}

	return true;
}
