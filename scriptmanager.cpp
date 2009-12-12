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
#include "scriptmanager.h"

#include <boost/filesystem.hpp>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "actions.h"
#include "movement.h"
#include "spells.h"
#include "talkaction.h"
#include "creatureevent.h"
#include "globalevent.h"
#include "weapons.h"

#include "monsters.h"
#include "spawn.h"
#include "raids.h"
#include "group.h"
#include "vocation.h"
#include "outfit.h"
#include "quests.h"
#include "items.h"
#include "chat.h"

#include "configmanager.h"
#include "luascript.h"

Actions* g_actions = NULL;
CreatureEvents* g_creatureEvents = NULL;
Spells* g_spells = NULL;
TalkActions* g_talkActions = NULL;
MoveEvents* g_moveEvents = NULL;
Weapons* g_weapons = NULL;
GlobalEvents* g_globalEvents = NULL;

extern Chat g_chat;
extern ConfigManager g_config;
extern Monsters g_monsters;

bool ScriptingManager::load()
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

bool ScriptingManager::loadMods()
{
	boost::filesystem::path modsPath(getFilePath(FILE_TYPE_MOD, ""));
	if(!boost::filesystem::exists(modsPath))
		return true; //silently ignore

	int32_t i = 0, j = 0;
	bool enabled = false;
	for(boost::filesystem::directory_iterator it(modsPath), end; it != end; ++it)
	{
		std::string s = it->leaf();
		if(boost::filesystem::is_directory(it->status()) && (s.size() > 4 ? s.substr(s.size() - 4) : "") != ".xml")
			continue;

		std::cout << "> Loading " << s << "...";
		if(loadFromXml(s, enabled))
		{
			std::cout << " done";
			if(!enabled)
			{
				++j;
				std::cout << ", but disabled";
			}

			std::cout << ".";
		}
		else
			std::cout << " failed!";

		std::cout << std::endl;
		++i;
	}

	std::cout << "> " << i << " mods were loaded";
	if(j)
		std::cout << " (" << j << " disabled)";

	std::cout << "." << std::endl;
	modsLoaded = true;
	return true;
}

void ScriptingManager::clearMods()
{
	modMap.clear();
	libMap.clear();
}

bool ScriptingManager::reloadMods()
{
	clearMods();
	return loadMods();
}

bool ScriptingManager::loadFromXml(const std::string& file, bool& enabled)
{
	enabled = false;
	std::string modPath = getFilePath(FILE_TYPE_MOD, file);

	xmlDocPtr doc = xmlParseFile(modPath.c_str());
	if(!doc)
	{
		std::cout << "[Error - ScriptingManager::loadFromXml] Cannot load mod " << modPath << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	int32_t intValue;
	std::string strValue;

	xmlNodePtr p, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"mod"))
	{
		std::cout << "[Error - ScriptingManager::loadFromXml] Malformed mod " << modPath << std::endl;
		std::cout << getLastXMLError() << std::endl;

		xmlFreeDoc(doc);
		return false;
	}

	if(!readXMLString(root, "name", strValue))
	{
		std::cout << "[Warning - ScriptingManager::loadFromXml] Empty name in mod " << modPath << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	ModBlock mod;
	mod.enabled = false;
	if(readXMLString(root, "enabled", strValue) && booleanString(strValue))
		mod.enabled = true;

	mod.file = file;
	mod.name = strValue;
	if(readXMLString(root, "author", strValue))
		mod.author = strValue;
	if(readXMLString(root, "version", strValue))
		mod.version = strValue;
	if(readXMLString(root, "contact", strValue))
		mod.contact = strValue;

	if(mod.enabled)
	{
		std::string scriptsPath = getFilePath(FILE_TYPE_MOD, "scripts/");
		p = root->children;
		while(p)
		{
			if(!xmlStrcmp(p->name, (const xmlChar*)"quest"))
				Quests::getInstance()->parseQuestNode(p, modsLoaded);
			else if(!xmlStrcmp(p->name, (const xmlChar*)"outfit"))
				Outfits::getInstance()->parseOutfitNode(p);
			else if(!xmlStrcmp(p->name, (const xmlChar*)"vocation"))
				Vocations::getInstance()->parseVocationNode(p); //duplicates checking is dangerous, shouldn't be performed
			else if(!xmlStrcmp(p->name, (const xmlChar*)"group"))
				Groups::getInstance()->parseGroupNode(p); //duplicates checking is dangerous, shouldn't be performed
			else if(!xmlStrcmp(p->name, (const xmlChar*)"raid"))
				Raids::getInstance()->parseRaidNode(p, modsLoaded, FILE_TYPE_MOD);
			else if(!xmlStrcmp(p->name, (const xmlChar*)"spawn"))
				Spawns::getInstance()->parseSpawnNode(p, modsLoaded);
			else if(!xmlStrcmp(p->name, (const xmlChar*)"channel"))
				g_chat.parseChannelNode(p); //TODO: duplicates (channel destructor needs sending self close to users)
			else if(!xmlStrcmp(p->name, (const xmlChar*)"monster"))
			{
				std::string file, name;
				if(readXMLString(p, "file", file) && readXMLString(p, "name", name))
				{
					file = getFilePath(FILE_TYPE_MOD, "monster/" + file);
					g_monsters.loadMonster(file, name, true);
				}
			}
			else if(!xmlStrcmp(p->name, (const xmlChar*)"item"))
			{
				if(readXMLInteger(p, "id", intValue))
					Item::items.parseItemNode(p, intValue); //duplicates checking isn't necessary here
			}
			if(!xmlStrcmp(p->name, (const xmlChar*)"description") || !xmlStrcmp(p->name, (const xmlChar*)"info"))
			{
				if(parseXMLContentString(p->children, strValue))
				{
					replaceString(strValue, "\t", "");
					mod.description = strValue;
				}
			}
			else if(!xmlStrcmp(p->name, (const xmlChar*)"lib") || !xmlStrcmp(p->name, (const xmlChar*)"config"))
			{
				if(!readXMLString(p, "name", strValue))
				{
					std::cout << "[Warning - ScriptingManager::loadFromXml] Lib without name in mod " << strValue << std::endl;
					p = p->next;
					continue;
				}

				toLowerCaseString(strValue);
				std::string strLib;
				if(parseXMLContentString(p->children, strLib))
				{
					LibMap::iterator it = libMap.find(strValue);
					if(it == libMap.end())
					{
						LibBlock lb;
						lb.first = file;
						lb.second = strLib;

						libMap[strValue] = lb;
					}
					else
						std::cout << "[Warning - ScriptingManager::loadFromXml] Duplicated lib in mod "
							<< strValue << ", previously declared in " << it->second.first << std::endl;
				}
			}
			else if(!g_actions->parseEventNode(p, scriptsPath, modsLoaded))
			{
				if(!g_talkActions->parseEventNode(p, scriptsPath, modsLoaded))
				{
					if(!g_moveEvents->parseEventNode(p, scriptsPath, modsLoaded))
					{
						if(!g_creatureEvents->parseEventNode(p, scriptsPath, modsLoaded))
						{
							if(!g_globalEvents->parseEventNode(p, scriptsPath, modsLoaded))
							{
								if(!g_spells->parseEventNode(p, scriptsPath, modsLoaded))
									g_weapons->parseEventNode(p, scriptsPath, modsLoaded);
							}
						}
					}
				}
			}

			p = p->next;
		}
	}

	enabled = mod.enabled;
	modMap[mod.name] = mod;
	xmlFreeDoc(doc);
	return true;
}
