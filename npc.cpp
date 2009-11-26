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

#include <functional>
#include <iostream>
#include <fstream>

#include "npc.h"
#include "tools.h"

#include "luascript.h"
#include "position.h"

#include "spells.h"
#include "vocation.h"

#include "configmanager.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;
extern Spells* g_spells;

AutoList<Npc> Npc::autoList;
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t Npc::npcCount = 0;
#endif
NpcScriptInterface* Npc::m_interface = NULL;

void Npcs::reload()
{
	for(AutoList<Npc>::iterator it = Npc::autoList.begin(); it != Npc::autoList.end(); ++it)
		it->second->closeAllShopWindows();

	delete Npc::m_interface;
	Npc::m_interface = NULL;
	for(AutoList<Npc>::iterator it = Npc::autoList.begin(); it != Npc::autoList.end(); ++it)
		it->second->reload();
}

Npc* Npc::createNpc(const std::string& name)
{
	Npc* npc = new Npc(name);
	if(!npc)
		return NULL;

	if(npc->load())
		return npc;

	delete npc;
	return NULL;
}

Npc::Npc(const std::string& _name):
	Creature()
{
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	npcCount++;
#endif
	m_filename = getFilePath(FILE_TYPE_OTHER, "npc/" + _name + ".xml");
	if(!fileExists(m_filename.c_str()))
	{
		std::string tmp = getFilePath(FILE_TYPE_MOD, "npc/" + _name + ".xml");
		if(fileExists(tmp.c_str()))
			m_filename = tmp;
	}

	m_npcEventHandler = NULL;
	loaded = false;
	reset();
}

Npc::~Npc()
{
	reset();
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	npcCount--;
#endif
}

bool Npc::load()
{
	if(isLoaded())
		return true;

	reset();
	if(!m_interface)
	{
		m_interface = new NpcScriptInterface();
		m_interface->loadNpcLib(getFilePath(FILE_TYPE_OTHER, "npc/lib/npc.lua"));
	}

	loaded = loadFromXml(m_filename);
	return isLoaded();
}

void Npc::reset()
{
	loaded = false;
	walkTicks = 1500;
	floorChange = false;
	attackable = false;
	hasBusyReply = false;
	hasScriptedFocus = false;
	focusCreature = 0;
	isIdle = true;
	talkRadius = 2;
	idleTime = 0;
	idleInterval = 5 * 60;
	lastVoice = OTSYS_TIME();
	defaultPublic = true;

	delete m_npcEventHandler;
	m_npcEventHandler = NULL;
	for(ResponseList::iterator it = responseList.begin(); it != responseList.end(); ++it)
		delete *it;

	for(StateList::iterator it = stateList.begin(); it != stateList.end(); ++it)
		delete *it;

	responseList.clear();
	stateList.clear();
	queueList.clear();
	m_parameters.clear();
	itemListMap.clear();
	responseScriptMap.clear();
	shopPlayerList.clear();
	voiceList.clear();
}

void Npc::reload()
{
	reset();
	load();
	//Simulate that the creature is placed on the map again.
	if(m_npcEventHandler)
		m_npcEventHandler->onCreatureAppear(this);

	if(walkTicks > 0)
		addEventWalk();
}

bool Npc::loadFromXml(const std::string& filename)
{
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(!doc)
	{
		std::cout << "[Warning - Npc::loadFromXml] Cannot load npc file (" << filename << ")." << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr p, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"npc"))
	{
		std::cout << "[Error - Npc::loadFromXml] Malformed npc file (" << filename << ")." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	int32_t intValue;
	std::string strValue;

	std::string scriptfile = "";
	if(readXMLString(root, "script", strValue))
		scriptfile = strValue;

	if(readXMLString(root, "name", strValue))
		name = strValue;

	nameDescription = name;
	if(readXMLString(root, "namedescription", strValue) || readXMLString(root, "nameDescription", strValue))
		nameDescription = strValue;

	if(readXMLString(root, "hidename", strValue) || readXMLString(root, "hideName", strValue))
		hideName = booleanString(strValue);

	if(readXMLString(root, "hidehealth", strValue) || readXMLString(root, "hideHealth", strValue))
		hideHealth = booleanString(strValue);

	baseSpeed = 110;
	if(readXMLInteger(root, "speed", intValue))
		baseSpeed = intValue;

	if(readXMLString(root, "attackable", strValue))
		attackable = booleanString(strValue);

	if(readXMLString(root, "walkable", strValue))
		walkable = booleanString(strValue);

	if(readXMLInteger(root, "autowalk", intValue))
	{
		std::cout << "[Notice - Npc::Npc] NPC Name: " << name << " - autowalk has been deprecated, use walkinterval." << std::endl;
		walkTicks = 2000;
	}

	if(readXMLInteger(root, "walkinterval", intValue))
		walkTicks = intValue;

	if(readXMLString(root, "floorchange", strValue))
		floorChange = booleanString(strValue);

	if(readXMLString(root, "skull", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "red" || tmpStrValue == "4")
			setSkull(SKULL_RED);
		else if(tmpStrValue == "white" || tmpStrValue == "3")
			setSkull(SKULL_WHITE);
		else if(tmpStrValue == "green" || tmpStrValue == "2")
			setSkull(SKULL_GREEN);
		else if(tmpStrValue == "yellow" || tmpStrValue == "1")
			setSkull(SKULL_YELLOW);
		else
			setSkull(SKULL_NONE);
	}

	if(readXMLString(root, "shield", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "whitenoshareoff" || tmpStrValue == "10")
			setShield(SHIELD_YELLOW_NOSHAREDEXP);
		else if(tmpStrValue == "blueshareoff" || tmpStrValue == "9")
			setShield(SHIELD_BLUE_NOSHAREDEXP);
		else if(tmpStrValue == "yellowshareblink" || tmpStrValue == "8")
			setShield(SHIELD_YELLOW_NOSHAREDEXP_BLINK);
		else if(tmpStrValue == "blueshareblink" || tmpStrValue == "7")
			setShield(SHIELD_BLUE_NOSHAREDEXP_BLINK);
		else if(tmpStrValue == "yellowshareon" || tmpStrValue == "6")
			setShield(SHIELD_YELLOW_SHAREDEXP);
		else if(tmpStrValue == "blueshareon" || tmpStrValue == "5")
			setShield(SHIELD_BLUE_SHAREDEXP);
		else if(tmpStrValue == "yellow" || tmpStrValue == "4")
			setShield(SHIELD_YELLOW);
		else if(tmpStrValue == "blue" || tmpStrValue == "3")
			setShield(SHIELD_BLUE);
		else if(tmpStrValue == "whiteyellow" || tmpStrValue == "2")
			setShield(SHIELD_WHITEYELLOW);
		else if(tmpStrValue == "whiteblue" || tmpStrValue == "1")
			setShield(SHIELD_WHITEBLUE);
		else
			setShield(SHIELD_NONE);
	}

	p = root->children;
	while(p)
	{
		if(xmlStrcmp(p->name, (const xmlChar*)"health") == 0)
		{
			if(readXMLInteger(p, "now", intValue))
				health = intValue;
			else
				health = 100;

			if(readXMLInteger(p, "max", intValue))
				healthMax = intValue;
			else
				healthMax = 100;
		}
		else if(xmlStrcmp(p->name, (const xmlChar*)"look") == 0)
		{
			if(readXMLInteger(p, "type", intValue))
			{
				defaultOutfit.lookType = intValue;
				if(readXMLInteger(p, "head", intValue))
					defaultOutfit.lookHead = intValue;

				if(readXMLInteger(p, "body", intValue))
					defaultOutfit.lookBody = intValue;

				if(readXMLInteger(p, "legs", intValue))
					defaultOutfit.lookLegs = intValue;

				if(readXMLInteger(p, "feet", intValue))
					defaultOutfit.lookFeet = intValue;

				if(readXMLInteger(p, "addons", intValue))
					defaultOutfit.lookAddons = intValue;
			}
			else if(readXMLInteger(p, "typeex", intValue))
				defaultOutfit.lookTypeEx = intValue;

			currentOutfit = defaultOutfit;
		}
		else if(xmlStrcmp(p->name, (const xmlChar*)"voices") == 0)
		{
			for(xmlNodePtr q = p->children; q != NULL; q = q->next)
			{
				if(xmlStrcmp(q->name, (const xmlChar*)"voice") == 0)
				{
					if(!readXMLString(q, "text", strValue))
						continue;

					Voice voice;
					voice.text = strValue;
					if(readXMLInteger(q, "interval2", intValue))
						voice.interval = intValue;
					else
						voice.interval = 60;

					if(readXMLInteger(q, "margin", intValue))
						voice.margin = intValue;
					else
						voice.margin = 0;

					voice.type = SPEAK_SAY;
					if(readXMLInteger(q, "type", intValue))
						voice.type = (SpeakClasses)intValue;
					else if(readXMLString(q, "yell", strValue) && booleanString(strValue))
						voice.type = SPEAK_YELL;

					if(readXMLString(q, "randomspectator", strValue) || readXMLString(q, "randomSpectator", strValue))
						voice.randomSpectator = booleanString(strValue);
					else
						voice.randomSpectator = false;

					voiceList.push_back(voice);
				}
			}
		}
		else if(xmlStrcmp(p->name, (const xmlChar*)"parameters") == 0)
		{
			for(xmlNodePtr q = p->children; q != NULL; q = q->next)
			{
				if(xmlStrcmp(q->name, (const xmlChar*)"parameter") == 0)
				{
					std::string paramKey, paramValue;
					if(!readXMLString(q, "key", paramKey))
						continue;

					if(!readXMLString(q, "value", paramValue))
						continue;

					m_parameters[paramKey] = paramValue;
				}
			}
		}
		else if(xmlStrcmp(p->name, (const xmlChar*)"interaction") == 0)
		{
			if(readXMLInteger(p, "talkradius", intValue))
				talkRadius = intValue;

			if(readXMLInteger(p, "idletime", intValue))
				idleTime = intValue;

			if(readXMLInteger(p, "idleinterval", intValue))
				idleInterval = intValue;

			if(readXMLInteger(p, "defaultpublic", intValue))
				defaultPublic = intValue != 0;

			responseList = loadInteraction(p->children);
		}

		p = p->next;
	}

	xmlFreeDoc(doc);
	if(scriptfile.empty())
		return true;

	if(scriptfile.find("/") == std::string::npos)
		scriptfile = getFilePath(FILE_TYPE_OTHER, "npc/scripts/" + scriptfile);

	m_npcEventHandler = new NpcScript(scriptfile, this);
	return m_npcEventHandler->isLoaded();
}

uint32_t Npc::loadParams(xmlNodePtr node)
{
	std::string strValue;
	uint32_t params = RESPOND_DEFAULT;
	if(readXMLString(node, "param", strValue))
	{
		StringVec paramList = explodeString(strValue, ";");
		for(StringVec::iterator it = paramList.begin(); it != paramList.end(); ++it)
		{
			std::string tmpParam = asLowerCaseString(*it);
			if(tmpParam == "male")
				params |= RESPOND_MALE;
			else if(tmpParam == "female")
				params |= RESPOND_FEMALE;
			else if(tmpParam == "pzblock")
				params |= RESPOND_PZBLOCK;
			else if(tmpParam == "lowmoney")
				params |= RESPOND_LOWMONEY;
			else if(tmpParam == "noamount")
				params |= RESPOND_NOAMOUNT;
			else if(tmpParam == "lowamount")
				params |= RESPOND_LOWAMOUNT;
			else if(tmpParam == "premium")
				params |= RESPOND_PREMIUM;
			else if(tmpParam == "druid")
				params |= RESPOND_DRUID;
			else if(tmpParam == "knight")
				params |= RESPOND_KNIGHT;
			else if(tmpParam == "paladin")
				params |= RESPOND_PALADIN;
			else if(tmpParam == "sorcerer")
				params |= RESPOND_SORCERER;
			else if(tmpParam == "lowlevel")
				params |= RESPOND_LOWLEVEL;
			else
				std::cout << "[Warning - Npc::loadParams] NPC Name: " << name << " - Unknown param " << (*it) << std::endl;
		}
	}

	return params;
}

ResponseList Npc::loadInteraction(xmlNodePtr node)
{
	std::string strValue;
	int32_t intValue;

	ResponseList _responseList;
	while(node)
	{
		if(xmlStrcmp(node->name, (const xmlChar*)"include") == 0)
		{
			if(readXMLString(node, "file", strValue))
			{
				if(xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_OTHER, "npc/lib/" + strValue).c_str()))
				{
					xmlNodePtr root = xmlDocGetRootElement(doc);
					if(!xmlStrcmp(root->name,(const xmlChar*)"interaction"))
					{
						ResponseList includedResponses = loadInteraction(root->children);
						_responseList.insert(_responseList.end(), includedResponses.begin(), includedResponses.end());
					}
					else
						std::cout << "[Error - Npc::loadInteraction] Malformed interaction file (" << strValue << ")." << std::endl;

					xmlFreeDoc(doc);
				}
				else
				{
					std::cout << "[Warning - Npc::loadInteraction] Cannot load interaction file (" << strValue << ")." << std::endl;
					std::cout << getLastXMLError() << std::endl;
				}
			}
		}
		else if(!xmlStrcmp(node->name, (const xmlChar*)"itemlist"))
		{
			if(readXMLString(node, "listid", strValue))
			{
				ItemListMap::iterator it = itemListMap.find(strValue);
				if(it == itemListMap.end())
				{
					std::string listId = strValue;
					std::list<ListItem>& list = itemListMap[strValue];

					xmlNodePtr tmpNode = node->children;
					while(tmpNode)
					{
						if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"item"))
						{
							ListItem li;
							if(!readXMLInteger(tmpNode, "id", intValue))
							{
								std::cout << "[Warning - Npc::loadInteraction] NPC Name: " << name << " - Missing list item itemId" << std::endl;
								tmpNode = tmpNode->next;
								continue;
							}

							li.itemId = intValue;
							if(readXMLInteger(tmpNode, "sellprice", intValue))
								li.sellPrice = intValue;

							if(readXMLInteger(tmpNode, "buyprice", intValue))
								li.buyPrice = intValue;

							if(readXMLString(tmpNode, "keywords", strValue))
								li.keywords = strValue;
							else
							{
								std::cout << "[Warning - Npc::loadInteraction] NPC Name: " << name << " - Missing list item keywords" << std::endl;
								tmpNode = tmpNode->next;
								continue;
							}

							//optional
							if(readXMLInteger(tmpNode, "subtype", intValue))
								li.subType = intValue;

							if(readXMLString(tmpNode, "name", strValue))
								li.name = strValue;

							if(readXMLString(tmpNode, "pname", strValue))
								li.pluralName = strValue;

							list.push_back(li);
						}

						tmpNode = tmpNode->next;
					}
				}
				else
					std::cout << "[Warning - Npc::loadInteraction] NPC Name: " << name << " - Duplicate listId found: " << strValue << std::endl;
			}
		}
		else if(!xmlStrcmp(node->name, (const xmlChar*)"interact"))
		{
			NpcResponse::ResponseProperties prop;
			prop.publicize = defaultPublic;
			if(readXMLString(node, "keywords", strValue))
				prop.inputList.push_back(asLowerCaseString(strValue));
			else if(readXMLString(node, "event", strValue))
			{
				strValue = asLowerCaseString(strValue);
				if(strValue == "onbusy")
					hasBusyReply = true;

				prop.interactType = INTERACT_EVENT;
				prop.inputList.push_back(strValue);
			}

			if(readXMLInteger(node, "topic", intValue))
				prop.topic = intValue;

			if(readXMLInteger(node, "focus", intValue))
				prop.focusStatus = intValue;

			if(readXMLInteger(node, "storageId", intValue))
				prop.storageId = intValue;

			if(readXMLString(node, "storageValue", strValue))
				prop.storageValue = strValue;

			uint32_t interactParams = loadParams(node);
			if(readXMLString(node, "storageComp", strValue))
			{
				std::string tmpStrValue = asLowerCaseString(strValue);
				if(tmpStrValue == "equal")
					prop.storageComp = STORAGE_EQUAL;
				if(tmpStrValue == "notequal")
					prop.storageComp = STORAGE_NOTEQUAL;
				if(tmpStrValue == "greaterorequal")
					prop.storageComp = STORAGE_GREATEROREQUAL;
				if(tmpStrValue == "greater")
					prop.storageComp = STORAGE_GREATER;
				if(tmpStrValue == "less")
					prop.storageComp = STORAGE_LESS;
				if(tmpStrValue == "lessorequal")
					prop.storageComp = STORAGE_LESSOREQUAL;
			}

			xmlNodePtr tmpNode = node->children;
			while(tmpNode)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"keywords"))
				{
					//alternative input keywords
					xmlNodePtr altKeyNode = tmpNode->children;
					while(altKeyNode)
					{
						if(!xmlStrcmp(altKeyNode->name, (const xmlChar*)"text"))
						{
							if(readXMLContentString(altKeyNode, strValue))
								prop.inputList.push_back(asLowerCaseString(strValue));
						}
						altKeyNode = altKeyNode->next;
					}
				}
				else if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"list"))
				{
					xmlNodePtr listNode = tmpNode->children;
					while(listNode)
					{
						if(!xmlStrcmp(listNode->name, (const xmlChar*)"text"))
						{
							if(readXMLContentString(listNode, strValue))
							{
								ItemListMap::iterator it = itemListMap.find(strValue);
								if(it != itemListMap.end())
									prop.itemList.insert(prop.itemList.end(), it->second.begin(), it->second.end());
								else
									std::cout << "[Warning - Npc::loadInteraction] NPC Name: " << name << " - Could not find a list id called: " << strValue << std::endl;
							}
						}

						listNode = listNode->next;
					}
				}

				tmpNode = tmpNode->next;
			}

			tmpNode = node->children;
			while(tmpNode)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"response"))
				{
					prop.output = prop.knowSpell = "";
					prop.params = interactParams | loadParams(tmpNode);

					ScriptVars scriptVars;
					if(readXMLString(tmpNode, "knowspell", strValue))
						prop.knowSpell = strValue;

					if(readXMLString(tmpNode, "text", strValue))
					{
						prop.responseType = RESPONSE_DEFAULT;
						prop.output = strValue;
					}
					else if(readXMLString(tmpNode, "function", strValue))
					{
						prop.responseType = RESPONSE_SCRIPT;
						prop.output = strValue;
					}

					if(readXMLInteger(tmpNode, "public", intValue))
						prop.publicize = (intValue == 1);

					if(readXMLInteger(tmpNode, "b1", intValue))
						scriptVars.b1 = (intValue == 1);

					if(readXMLInteger(tmpNode, "b2", intValue))
						scriptVars.b2 = (intValue == 1);

					if(readXMLInteger(tmpNode, "b3", intValue))
						scriptVars.b3 = (intValue == 1);

					ResponseList subResponseList;
					xmlNodePtr subNode = tmpNode->children;
					while(subNode)
					{
						if(!xmlStrcmp(subNode->name, (const xmlChar*)"action"))
						{
							ResponseAction action;
							if(readXMLString(subNode, "name", strValue))
							{
								std::string tmpStrValue = asLowerCaseString(strValue);
								if(tmpStrValue == "topic")
								{
									if(readXMLInteger(subNode, "value", intValue))
									{
										action.actionType = ACTION_SETTOPIC;
										action.intValue = intValue;
									}
								}
								else if(tmpStrValue == "price")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SETPRICE;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "amount")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SETAMOUNT;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "item")
								{
									if(readXMLInteger(subNode, "value", intValue))
									{
										action.actionType = ACTION_SETITEM;
										action.intValue = intValue;
									}
								}
								else if(tmpStrValue == "subtype")
								{
									if(readXMLInteger(subNode, "value", intValue))
									{
										action.actionType = ACTION_SETSUBTYPE;
										action.intValue = intValue;
									}
								}
								else if(tmpStrValue == "spell")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SETSPELL;
										action.strValue = strValue;
										if(strValue != "|SPELL|" && !g_spells->getInstantSpellByName(strValue))
											std::cout << "[Warning - Npc::loadInteraction] NPC Name: " << name << " - Could not find an instant spell called: " << strValue << std::endl;
									}
								}
								else if(tmpStrValue == "listname")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SETLISTNAME;
										action.strValue = strValue;
									}
								}
								else if(tmpStrValue == "listpname")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SETLISTPNAME;
										action.strValue = strValue;
									}
								}
								else if(tmpStrValue == "teachspell")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_TEACHSPELL;
										action.strValue = strValue;
										if(strValue != "|SPELL|" && !g_spells->getInstantSpellByName(strValue))
											std::cout << "[Warning - Npc::loadInteraction] NPC Name: " << name << " - Could not find an instant spell called: " << strValue << std::endl;
									}
								}
								else if(tmpStrValue == "unteachspell")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_UNTEACHSPELL;
										action.strValue = strValue;
										if(strValue != "|SPELL|" && !g_spells->getInstantSpellByName(strValue))
											std::cout << "[Warning - Npc::loadInteraction] NPC Name: " << name << " - Could not find an instant spell called: " << strValue << std::endl;
									}
								}
								else if(tmpStrValue == "sell")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SELLITEM;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "buy")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_BUYITEM;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "takemoney")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_TAKEMONEY;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "givemoney")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_GIVEMONEY;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "level")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SETLEVEL;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "giveitem")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_GIVEITEM;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "takeitem")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_TAKEITEM;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "effect")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SETEFFECT;
										action.intValue = getMagicEffect(strValue);
									}
								}
								else if(tmpStrValue == "idle")
								{
									if(readXMLInteger(subNode, "value", intValue))
									{
										action.actionType = ACTION_SETIDLE;
										action.intValue = intValue;
									}
								}
								else if(tmpStrValue == "script")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SCRIPT;
										action.strValue = strValue;
									}
									else if(parseXMLContentString(subNode->children, action.strValue))
										action.actionType = ACTION_SCRIPT;
								}
								else if(tmpStrValue == "scriptparam")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SCRIPTPARAM;
										action.strValue = strValue;
									}
								}
								else if(tmpStrValue == "storage")
								{
									if(readXMLString(subNode, "time", strValue))
									{
										action.actionType = ACTION_SETSTORAGE;
										std::stringstream s;

										s << time(NULL) + atoi(strValue.c_str());
										action.strValue = s.str();
									}
									else if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SETSTORAGE;
										action.strValue = strValue;
									}
								}
								else if(tmpStrValue == "addqueue")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_ADDQUEUE;
										action.strValue = strValue;
										action.intValue = atoi(strValue.c_str());
									}
								}
								else if(tmpStrValue == "teleport")
								{
									if(readXMLString(subNode, "value", strValue))
									{
										action.actionType = ACTION_SETTELEPORT;
										action.strValue = strValue;
										action.pos = Position();

										IntegerVec posList = vectorAtoi(explodeString(strValue, ";"));
										if(posList.size() > 2)
											action.pos = Position(posList[0], posList[1], posList[2]);
									}
								}
								else
									std::cout << "[Warning - Npc::loadInteraction] Unknown action " << strValue << std::endl;
							}

							if(readXMLInteger(subNode, "key", intValue))
								action.key = intValue;

							if(action.actionType != ACTION_NONE)
								prop.actionList.push_back(action);
						}
						else if(!xmlStrcmp(subNode->name, (const xmlChar*)"interact"))
						{
							if(subResponseList.empty())
							{
								ResponseList nodeResponseList = loadInteraction(subNode);
								subResponseList.insert(subResponseList.end(),
									nodeResponseList.begin(), nodeResponseList.end());
							}
						}

						subNode = subNode->next;
					}

					//Check if this interaction has a |list| keyword
					bool hasListKeyword = false;
					for(std::list<std::string>::iterator it = prop.inputList.begin();
						it != prop.inputList.end(); ++it)
					{
						if(it->find("|list|") != std::string::npos)
						{
							hasListKeyword = true;
							break;
						}
					}

					//Iterate through all input keywords and replace all |LIST| with the item list
					if(hasListKeyword && !prop.itemList.empty())
					{
						for(std::list<ListItem>::iterator it = prop.itemList.begin(); it != prop.itemList.end(); ++it)
						{
							NpcResponse::ResponseProperties listItemProp = prop;
							for(std::list<std::string>::iterator iit = listItemProp.inputList.begin();
								iit != listItemProp.inputList.end(); ++iit)
							{
								std::string& input = (*iit);
								if(input.find("|list|") == std::string::npos)
									continue;

								//Replace |LIST| with the keyword in the list
								replaceString(input, "|list|", it->keywords);
								ResponseAction action;

								action.actionType = ACTION_SETITEM;
								action.intValue = it->itemId;
								listItemProp.actionList.push_front(action);

								action.actionType = ACTION_SETSELLPRICE;
								action.intValue = it->sellPrice;
								listItemProp.actionList.push_front(action);

								action.actionType = ACTION_SETBUYPRICE;
								action.intValue = it->buyPrice;
								listItemProp.actionList.push_front(action);

								action.actionType = ACTION_SETSUBTYPE;
								action.intValue = it->subType;
								listItemProp.actionList.push_front(action);

								action.actionType = ACTION_SETSUBTYPE;
								action.intValue = it->subType;
								listItemProp.actionList.push_front(action);

								action.actionType = ACTION_SETLISTNAME;
								if(it->name.empty())
								{
									const ItemType& itemType = Item::items[it->itemId];
									if(itemType.id != 0)
										action.strValue = itemType.article + " " + itemType.name;
								}
								else
									action.strValue = it->name;

								listItemProp.actionList.push_front(action);
								action.actionType = ACTION_SETLISTPNAME;
								if(it->pluralName.empty())
								{
									const ItemType& itemType = Item::items[it->itemId];
									if(itemType.id != 0)
										action.strValue = itemType.pluralName;
								}
								else
									action.strValue = it->pluralName;

								listItemProp.actionList.push_front(action);
								ResponseList list;
								for(ResponseList::iterator rit = subResponseList.begin();
									rit != subResponseList.end(); ++rit)
								{
										list.push_back(new NpcResponse(*(*rit)));
								}

								//Create a new response for this list item
								NpcResponse* response = new NpcResponse(listItemProp, list, scriptVars);
								_responseList.push_back(response);
							}
						}
					}
					else
					{
						NpcResponse* response = new NpcResponse(prop, subResponseList, scriptVars);
						_responseList.push_back(response);
					}
				}

				tmpNode = tmpNode->next;
			}
		}

		node = node->next;
	}

	return _responseList;
}

NpcState* Npc::getState(const Player* player, bool makeNew /*= true*/)
{
	for(StateList::iterator it = stateList.begin(); it != stateList.end(); ++it)
	{
		if((*it)->respondToCreature == player->getID())
			return *it;
	}

	if(!makeNew)
		return NULL;

	NpcState* state = new NpcState;
	state->prevInteraction = state->price = 0;
	state->sellPrice = state->buyPrice = -1;
	state->amount = 1;
	state->itemId = 0;
	state->subType = -1;
	state->ignoreCap = state->inBackpacks = false;
	state->spellName = state->listName = "";
	state->listPluralName = "";
	state->level = state->topic = -1;
	state->isIdle = true;
	state->isQueued = false;
	state->respondToText = "";
	state->respondToCreature = 0;
	state->lastResponse = NULL;
	state->prevRespondToText = "";

	stateList.push_back(state);
	return state;
}

bool Npc::canSee(const Position& pos) const
{
	if(pos.z != getPosition().z)
		return false;

	return Creature::canSee(getPosition(), pos, Map::maxClientViewportX, Map::maxClientViewportY);
}

void Npc::onCreatureAppear(const Creature* creature)
{
	Creature::onCreatureAppear(creature);
	if(creature == this && walkTicks > 0)
		addEventWalk();

	if(creature == this)
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureAppear(creature);

		return;
	}

	//only players for script events
	if(Player* player = const_cast<Player*>(creature->getPlayer()))
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureAppear(creature);

		NpcState* npcState = getState(player);
		if(npcState && canSee(player->getPosition()))
		{
			npcState->respondToCreature = player->getID();
			onPlayerEnter(player, npcState);
		}
	}
}

void Npc::onCreatureDisappear(const Creature* creature, bool isLogout)
{
	Creature::onCreatureDisappear(creature, isLogout);
	if(creature == this) //Close all open shop window's
		closeAllShopWindows();
	else if(Player* player = const_cast<Player*>(creature->getPlayer()))
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureDisappear(creature);

		NpcState* npcState = getState(player);
		if(npcState)
		{
			npcState->respondToCreature = player->getID();
			onPlayerLeave(player, npcState);
		}
	}
}

void Npc::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
		const Tile* oldTile, const Position& oldPos, bool teleport)
{
	Creature::onCreatureMove(creature, newTile, newPos, oldTile, oldPos, teleport);
	if(creature == this)
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureMove(creature, oldPos, newPos);
	}
	else if(Player* player = const_cast<Player*>(creature->getPlayer()))
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureMove(creature, oldPos, newPos);

		NpcState* npcState = getState(player);
		if(npcState)
		{
			bool canSeeNewPos = canSee(newPos), canSeeOldPos = canSee(oldPos);
			if(canSeeNewPos && !canSeeOldPos)
			{
				npcState->respondToCreature = player->getID();
				onPlayerEnter(player, npcState);
			}
			else if(!canSeeNewPos && canSeeOldPos)
			{
				npcState->respondToCreature = player->getID();
				onPlayerLeave(player, npcState);
			}
			else if(canSeeNewPos && canSeeOldPos)
			{
				npcState->respondToCreature = player->getID();
				const NpcResponse* response = getResponse(player, npcState, EVENT_PLAYER_MOVE);
				executeResponse(player, npcState, response);
			}
		}
	}
}

void Npc::onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text, Position* pos/* = NULL*/)
{
	if(creature->getID() == this->getID())
		return;

	//only players for script events
	if(const Player* player = creature->getPlayer())
	{
		if(m_npcEventHandler)
			m_npcEventHandler->onCreatureSay(player, type, text, pos);

		if(type == SPEAK_SAY || type == SPEAK_PRIVATE_PN)
		{
			Position destPos = creature->getPosition();
			if(pos)
				destPos = (*pos);

			const Position& myPos = getPosition();
			if(canSee(myPos))
			{
				if((destPos.x >= myPos.x - talkRadius) && (destPos.x <= myPos.x + talkRadius) &&
					(destPos.y >= myPos.y - talkRadius) && (destPos.y <= myPos.y + talkRadius))
				{
					if(NpcState* npcState = getState(player))
					{
						npcState->respondToText = text;
						npcState->respondToCreature = player->getID();
					}
				}
			}
		}
	}
}

void Npc::onPlayerCloseChannel(const Player* player)
{
	if(m_npcEventHandler)
		m_npcEventHandler->onPlayerCloseChannel(player);
}

void Npc::onPlayerEnter(Player* player, NpcState* state)
{
	const NpcResponse* response = getResponse(player, state, EVENT_PLAYER_ENTER);
	executeResponse(player, state, response);
}

void Npc::onPlayerLeave(Player* player, NpcState* state)
{
	player->closeShopWindow();
	const NpcResponse* response = getResponse(player, state, EVENT_PLAYER_LEAVE);
	executeResponse(player, state, response);
}

void Npc::onThink(uint32_t interval)
{
	Creature::onThink(interval);
	if(m_npcEventHandler)
		m_npcEventHandler->onThink();

	std::vector<Player*> list;
	Player* tmpPlayer = NULL;

	const SpectatorVec& tmpList = g_game.getSpectators(getPosition());
	if(tmpList.size()) //loop only if there's at least one spectator
	{
		for(SpectatorVec::const_iterator it = tmpList.begin(); it != tmpList.end(); ++it)
		{
			if((tmpPlayer = (*it)->getPlayer()) && !tmpPlayer->isRemoved())
				list.push_back(tmpPlayer);
		}
	}

	if(list.size()) //loop only if there's at least one player
	{
		int64_t now = OTSYS_TIME();
		for(VoiceList::iterator it = voiceList.begin(); it != voiceList.end(); ++it)
		{
			if(now < (lastVoice + it->margin))
				continue;

			if((uint32_t)(MAX_RAND_RANGE / it->interval) < (uint32_t)random_range(0, MAX_RAND_RANGE))
				continue;

			tmpPlayer = NULL;
			if(it->randomSpectator)
			{
				size_t random = random_range(0, (int32_t)list.size());
				if(random < list.size()) //1 slot chance to make it public
					tmpPlayer = list[random];
			}

			doSay(it->text, it->type, tmpPlayer);
			lastVoice = now;
			break;
		}
	}

	bool idleResponse = false;
	if((uint32_t)(MAX_RAND_RANGE / idleInterval) >= (uint32_t)random_range(0, MAX_RAND_RANGE))
		idleResponse = true;

	isIdle = true;
	for(StateList::iterator it = stateList.begin(); it != stateList.end();)
	{
		NpcState* npcState = *it;
		Player* player = g_game.getPlayerByID(npcState->respondToCreature);

		const NpcResponse* response = NULL;
		bool closeConversation = false, idleTimeout = false;
		if(!npcState->isQueued)
		{
			if(!npcState->prevInteraction)
				npcState->prevInteraction = OTSYS_TIME();

			if(!queueList.empty() && npcState->isIdle && npcState->respondToText.empty())
				closeConversation = true;
			else if(idleTime > 0 && (OTSYS_TIME() - npcState->prevInteraction) > (uint64_t)(idleTime * 1000))
				idleTimeout = closeConversation = true;
		}

		if(idleResponse && player)
		{
			response = getResponse(player, EVENT_IDLE);
			executeResponse(player, npcState, response);
			idleResponse = false;
		}

		if(!player || closeConversation)
		{
			if(queueList.empty())
			{
				if(idleTimeout && player)
					onPlayerLeave(player, npcState);
			}
			else
			{
				Player* nextPlayer = NULL;
				while(!queueList.empty())
				{
					if((nextPlayer = g_game.getPlayerByID(*queueList.begin())))
					{
						if(NpcState* nextPlayerState = getState(nextPlayer, false))
						{
							nextPlayerState->respondToText = nextPlayerState->prevRespondToText;
							nextPlayerState->isQueued = false;
							break;
						}
					}

					queueList.erase(queueList.begin());
				}
			}

			delete *it;
			stateList.erase(it++);
			continue;
		}

		if(!npcState->respondToText.empty())
		{
			if(hasBusyReply && !isIdle)
			{
				//Check if we have a busy reply
				if((response = getResponse(player, npcState, EVENT_BUSY)))
					executeResponse(player, npcState, response);
			}
			else
			{
				if(npcState->lastResponse)
				{
					//Check previous response chain first
					const ResponseList& list = npcState->lastResponse->getResponseList();
					response = getResponse(list, player, npcState, npcState->respondToText);
				}

				if(!response)
					response = getResponse(player, npcState, npcState->respondToText);

				if(response)
				{
					setCreatureFocus(player);
					executeResponse(player, npcState, response);
				}
			}

			npcState->prevRespondToText = npcState->respondToText;
			npcState->respondToText = "";
		}

		response = getResponse(player, npcState, EVENT_THINK);
		executeResponse(player, npcState, response);
		if(!npcState->isIdle)
		{
			isIdle = false;
			if(hasBusyReply)
				setCreatureFocus(player);
		}

		++it;
	}

	if(isIdle && !hasScriptedFocus)
		setCreatureFocus(NULL);
}

void Npc::executeResponse(Player* player, NpcState* npcState, const NpcResponse* response)
{
	if(response)
	{
		npcState->lastResponse = response;
		npcState->isIdle = response->getFocusState() == 0;

		bool resetTopic = true;
		if(response->getAmount() != -1)
			npcState->amount = response->getAmount();

		for(ActionList::const_iterator it = response->getFirstAction(); it != response->getEndAction(); ++it)
		{
			switch(it->actionType)
			{
				case ACTION_SETTOPIC:
					npcState->topic = it->intValue;
					resetTopic = false;
					break;
				case ACTION_SETSELLPRICE:
					npcState->sellPrice = it->intValue;
					break;
				case ACTION_SETBUYPRICE:
					npcState->buyPrice = it->intValue;
					break;
				case ACTION_SETITEM:
					npcState->itemId = it->intValue;
					break;
				case ACTION_SETSUBTYPE:
					npcState->subType = it->intValue;
					break;
				case ACTION_SETEFFECT:
					g_game.addMagicEffect(player->getPosition(), it->intValue);
					break;
				case ACTION_SETPRICE:
				{
					if(it->strValue == "|SELLPRICE|")
						npcState->price = npcState->sellPrice;
					else if(it->strValue == "|BUYPRICE|")
						npcState->price = npcState->buyPrice;
					else
						npcState->price = it->intValue;

					break;
				}

				case ACTION_SETTELEPORT:
				{
					Position teleportTo = it->pos;
					if(it->strValue == "|TEMPLE|")
						teleportTo = player->getMasterPosition();

					g_game.internalTeleport(player, teleportTo, true);
					break;
				}

				case ACTION_SETIDLE:
				{
					npcState->isIdle = (it->intValue == 1);
					break;
				}

				case ACTION_SETLEVEL:
				{
					if(it->strValue == "|SPELLLEVEL|")
					{
						npcState->level = -1;
						if(InstantSpell* spell = g_spells->getInstantSpellByName(npcState->spellName))
							npcState->level = spell->getLevel();
					}
					else
						npcState->level = it->intValue;

					break;
				}

				case ACTION_SETSPELL:
				{
					npcState->spellName = "";
					if(g_spells->getInstantSpellByName(it->strValue))
						npcState->spellName = it->strValue;

					break;
				}

				case ACTION_SETLISTNAME:
				{
					npcState->listName = it->strValue;
					break;
				}

				case ACTION_SETLISTPNAME:
				{
					npcState->listPluralName = it->strValue;
					break;
				}

				case ACTION_SETAMOUNT:
				{
					int32_t amount = it->intValue;
					if(it->strValue == "|AMOUNT|")
						amount = npcState->amount;

					npcState->amount = amount;
					break;
				}

				case ACTION_TEACHSPELL:
				{
					std::string spellName = it->strValue;
					if(it->strValue == "|SPELL|")
						spellName = npcState->spellName;

					player->learnInstantSpell(spellName);
					break;
				}

				case ACTION_UNTEACHSPELL:
				{
					std::string spellName = it->strValue;
					if(it->strValue == "|SPELL|")
						spellName = npcState->spellName;

					player->unlearnInstantSpell(spellName);
					break;
				}

				case ACTION_SETSTORAGE:
				{
					if(it->key > 0)
						player->setStorage(it->key, it->strValue);

					break;
				}

				case ACTION_ADDQUEUE:
				{
					if(std::find(queueList.begin(), queueList.end(), player->getID()) == queueList.end())
					{
						queueList.push_back(player->getID());
						npcState->isQueued = true;
					}

					break;
				}

				case ACTION_SELLITEM:
				{
					const ItemType& iit = Item::items[npcState->itemId];
					if(iit.id != 0)
					{
						uint32_t moneyCount = it->intValue;
						if(it->strValue == "|PRICE|")
							moneyCount = npcState->price * npcState->amount;

						int32_t subType = -1;
						if(iit.hasSubType())
							subType = npcState->subType;

						int32_t itemCount = player->__getItemTypeCount(iit.id, subType);
						if(itemCount >= npcState->amount)
						{
							g_game.removeItemOfType(player, iit.id, npcState->amount, subType);
							g_game.addMoney(player, moneyCount, FLAG_NOLIMIT);
						}
					}
					break;
				}

				case ACTION_BUYITEM:
				{
					const ItemType& iit = Item::items[npcState->itemId];
					if(iit.id != 0)
					{
						uint32_t moneyCount = it->intValue;
						if(it->strValue == "|PRICE|")
							moneyCount = npcState->price * npcState->amount;

						int32_t subType = -1;
						if(iit.hasSubType())
							subType = npcState->subType;

						if(g_game.getMoney(player) >= moneyCount)
						{
							int32_t amount = npcState->amount;
							if(iit.stackable)
							{
								while(amount > 0)
								{
									int32_t stack = std::min(100, amount);
									Item* item = Item::CreateItem(iit.id, stack);
									if(g_game.internalPlayerAddItem(this, player, item) != RET_NOERROR)
									{
										delete item;
										amount = npcState->amount - amount;
										break;
									}

									amount -= stack;
								}
							}
							else
							{
								for(int32_t i = 0; i < amount; ++i)
								{
									Item* item = Item::CreateItem(iit.id, subType);
									if(g_game.internalPlayerAddItem(this, player, item) != RET_NOERROR)
									{
										delete item;
										amount = i + 1;
										break;
									}
								}
							}

							if(it->strValue == "|PRICE|")
								moneyCount = npcState->price * amount;

							g_game.removeMoney(player, moneyCount);
						}
					}
					break;
				}

				case ACTION_TAKEITEM:
				{
					int32_t itemId = it->intValue;
					if(it->strValue == "|ITEM|")
						itemId = npcState->itemId;

					const ItemType& iit = Item::items[npcState->itemId];
					if(iit.id != 0)
					{
						int32_t subType = -1;
						if(iit.hasSubType())
							subType = npcState->subType;

						int32_t itemCount = player->__getItemTypeCount(itemId, subType);
						if(itemCount >= npcState->amount)
							g_game.removeItemOfType(player, itemId, npcState->amount, subType);
					}
					break;
				}

				case ACTION_GIVEITEM:
				{
					int32_t itemId = it->intValue;
					if(it->strValue == "|ITEM|")
						itemId = npcState->itemId;

					const ItemType& iit = Item::items[itemId];
					if(iit.id != 0)
					{
						int32_t subType = -1;
						if(iit.hasSubType())
							subType = npcState->subType;

						for(int32_t i = 0; i < npcState->amount; ++i)
						{
							Item* item = Item::CreateItem(iit.id, subType);
							if(g_game.internalPlayerAddItem(this, player, item) != RET_NOERROR)
								delete item;
						}
					}
					break;
				}

				case ACTION_TAKEMONEY:
				{
					uint32_t moneyCount = 0;
					if(it->strValue == "|PRICE|")
						moneyCount = npcState->price * npcState->amount;
					else
						moneyCount = it->intValue;

					g_game.removeMoney(player, moneyCount);
					break;
				}

				case ACTION_GIVEMONEY:
				{
					uint32_t moneyCount = 0;
					if(it->strValue == "|PRICE|")
						moneyCount = npcState->price * npcState->amount;
					else
						moneyCount = it->intValue;

					g_game.addMoney(player, moneyCount);
					break;
				}

				case ACTION_SCRIPT:
				{
					NpcScriptInterface interface;
					if(interface.reserveEnv())
					{
						ScriptEnviroment* env = m_interface->getEnv();
						std::stringstream scriptstream;

						//attach various variables that could be interesting
						scriptstream << "local cid = " << env->addThing(player) << std::endl;
						scriptstream << "local text = \"" << npcState->respondToText << "\"" << std::endl;
						scriptstream << "local name = \"" << player->getName() << "\"" << std::endl;
						scriptstream << "local idletime = " << idleTime << std::endl;
						scriptstream << "local idleinterval = " << idleInterval << std::endl;

						scriptstream << "local itemlist = {" << std::endl;
						uint32_t n = 0;
						for(std::list<ListItem>::const_iterator iit = response->prop.itemList.begin(); iit != response->prop.itemList.end(); ++iit)
						{
							scriptstream << "{id = " << iit->itemId
								<< ", subType = " << iit->subType
								<< ", buy = " << iit->buyPrice
								<< ", sell = " << iit->sellPrice
								<< ", name = '" << iit->name << "'}";

							++n;
							if(n != response->prop.itemList.size())
								scriptstream << "," << std::endl;
						}

						scriptstream << "}" << std::endl;
						scriptstream << "_state = {" << std::endl;
						scriptstream << "topic = " << npcState->topic << ',' << std::endl;
						scriptstream << "itemid = " << npcState->itemId << ',' << std::endl;
						scriptstream << "subtype = " << npcState->subType << ',' << std::endl;
						scriptstream << "ignorecap = " << npcState->ignoreCap << ',' << std::endl;
						scriptstream << "inbackpacks = " << npcState->inBackpacks << ',' << std::endl;
						scriptstream << "amount = " << npcState->amount << ',' << std::endl;
						scriptstream << "price = " << npcState->price << ',' << std::endl;
						scriptstream << "level = " << npcState->level << ',' << std::endl;
						scriptstream << "spellname = \"" << npcState->spellName << "\"" << ',' << std::endl;
						scriptstream << "listname = \"" << npcState->listName << "\"" << ',' << std::endl;
						scriptstream << "listpname = \"" << npcState->listPluralName << "\"" << ',' << std::endl;

						scriptstream << "n1 = " << npcState->scriptVars.n1 << ',' << std::endl;
						scriptstream << "n2 = " << npcState->scriptVars.n2 << ',' << std::endl;
						scriptstream << "n3 = " << npcState->scriptVars.n3 << ',' << std::endl;

						scriptstream << "b1 = " << (npcState->scriptVars.b1 ? "true" : "false") << ',' << std::endl;
						scriptstream << "b2 = " << (npcState->scriptVars.b2 ? "true" : "false") << ',' << std::endl;
						scriptstream << "b3 = " << (npcState->scriptVars.b3 ? "true" : "false") << ',' << std::endl;

						scriptstream << "s1 = \"" << npcState->scriptVars.s1 << "\"" << ',' << std::endl;
						scriptstream << "s2 = \"" << npcState->scriptVars.s2 << "\"" << ',' << std::endl;
						scriptstream << "s3 = \"" << npcState->scriptVars.s3 << "\"" << std::endl;
						scriptstream << "}" << std::endl;

						scriptstream << it->strValue;
						if(interface.loadBuffer(scriptstream.str(), this))
						{
							lua_State* L = interface.getState();
							lua_getglobal(L, "_state");
							NpcScriptInterface::popState(L, npcState);
						}

						interface.releaseEnv();
					}

					break;
				}

				default:
					break;
			}
		}

		if(response->getResponseType() == RESPONSE_DEFAULT)
		{
			std::string responseString = formatResponse(player, npcState, response);
			if(!responseString.empty())
			{
				if(!response->publicize())
					doSay(responseString, SPEAK_PRIVATE_NP, player);
				else
					doSay(responseString, SPEAK_SAY, NULL);
			}
		}
		else
		{
			int32_t functionId = -1;
			ResponseScriptMap::iterator it = responseScriptMap.find(response->getText());
			if(it != responseScriptMap.end())
				functionId = it->second;
			else
			{
				functionId = m_interface->getEvent(response->getText());
				responseScriptMap[response->getText()] = functionId;
			}

			if(functionId != -1)
			{
				if(m_interface->reserveEnv())
				{
					ScriptEnviroment* env = m_interface->getEnv();
					lua_State* L = m_interface->getState();

					env->setScriptId(functionId, m_interface);
					Npc* prevNpc = env->getNpc();
					env->setRealPos(getPosition());
					env->setNpc(this);

					m_interface->pushFunction(functionId);
					int32_t paramCount = 0;
					for(ActionList::const_iterator it = response->getFirstAction(); it != response->getEndAction(); ++it)
					{
						if(it->actionType == ACTION_SCRIPTPARAM)
						{
							if(it->strValue == "|PLAYER|")
								lua_pushnumber(L, env->addThing(player));
							else if(it->strValue == "|TEXT|")
								lua_pushstring(L, npcState->respondToText.c_str());
							else
							{
								std::cout << "[Warning - Npc::executeResponse] Unknown script param: " << it->strValue << std::endl;
								break;
							}

							++paramCount;
						}
					}

					NpcScriptInterface::pushState(L, npcState);
					lua_setglobal(L, "_state");
					m_interface->callFunction(paramCount);
					lua_getglobal(L, "_state");

					NpcScriptInterface::popState(L, npcState);
					if(prevNpc)
					{
						env->setRealPos(prevNpc->getPosition());
						env->setNpc(prevNpc);
					}

					m_interface->releaseEnv();
				}
				else
					std::cout << "[Error] Call stack overflow." << std::endl;
			}
		}

		if(resetTopic && response->getTopic() == npcState->topic)
			npcState->topic = -1;

		npcState->prevInteraction = OTSYS_TIME();
	}
}

void Npc::doSay(const std::string& text, SpeakClasses type, Player* player)
{
	if(!player)
	{
		std::string tmp = text;
		replaceString(tmp, "{", "");
		replaceString(tmp, "}", "");

		g_game.internalCreatureSay(this, type, tmp, false);
	}
	else
	{
		player->sendCreatureSay(this, type, text);
		player->onCreatureSay(this, type, text);
	}
}

void Npc::doTurn(Direction dir)
{
	g_game.internalCreatureTurn(this, dir);
}

void Npc::doMove(Direction dir)
{
	g_game.internalMoveCreature(this, dir);
}

void Npc::doMoveTo(Position target)
{
	std::list<Direction> listDir;
	if(!g_game.getPathToEx(this, target, listDir, 1, 1, true, true))
		return;

	startAutoWalk(listDir);
}

uint32_t Npc::getListItemPrice(uint16_t itemId, ShopEvent_t type)
{
	for(ItemListMap::iterator it = itemListMap.begin(); it != itemListMap.end(); ++it)
	{
		std::list<ListItem>& itemList = it->second;
		for(std::list<ListItem>::iterator iit = itemList.begin(); iit != itemList.end(); ++iit)
		{
			if((*iit).itemId == itemId)
			{
				if(type == SHOPEVENT_BUY)
					return (*iit).buyPrice;
				else if(type == SHOPEVENT_SELL)
					return (*iit).sellPrice;
			}
		}
	}

	return 0;
}

void Npc::onPlayerTrade(Player* player, ShopEvent_t type, int32_t callback, uint16_t itemId, uint8_t count,
	uint8_t amount, bool ignoreCap/* = false*/, bool inBackpacks/* = false*/)
{
	if(type == SHOPEVENT_BUY)
	{
		if(NpcState* npcState = getState(player, true))
		{
			npcState->amount = amount;
			npcState->subType = count;
			npcState->itemId = itemId;
			npcState->buyPrice = getListItemPrice(itemId, SHOPEVENT_BUY);
			npcState->ignoreCap = ignoreCap;
			npcState->inBackpacks = inBackpacks;

			const NpcResponse* response = getResponse(player, npcState, EVENT_PLAYER_SHOPBUY);
			executeResponse(player, npcState, response);
		}
	}
	else if(type == SHOPEVENT_SELL)
	{
		if(NpcState* npcState = getState(player, true))
		{
			npcState->amount = amount;
			npcState->subType = count;
			npcState->itemId = itemId;
			npcState->sellPrice = getListItemPrice(itemId, SHOPEVENT_SELL);

			const NpcResponse* response = getResponse(player, npcState, EVENT_PLAYER_SHOPSELL);
			executeResponse(player, npcState, response);
		}
	}

	if(m_npcEventHandler)
		m_npcEventHandler->onPlayerTrade(player, callback, itemId, count, amount, ignoreCap, inBackpacks);

	player->sendGoods();
}

void Npc::onPlayerEndTrade(Player* player, int32_t buyCallback,
	int32_t sellCallback)
{
	lua_State* L = getInterface()->getState();
	if(buyCallback != -1)
		luaL_unref(L, LUA_REGISTRYINDEX, buyCallback);
	if(sellCallback != -1)
		luaL_unref(L, LUA_REGISTRYINDEX, sellCallback);

	removeShopPlayer(player);
	NpcState* npcState = getState(player, true);
	if(npcState)
	{
		const NpcResponse* response = getResponse(player, npcState, EVENT_PLAYER_SHOPCLOSE);
		executeResponse(player, npcState, response);
	}

	if(m_npcEventHandler)
		m_npcEventHandler->onPlayerEndTrade(player);
}

bool Npc::getNextStep(Direction& dir, uint32_t& flags)
{
	if(Creature::getNextStep(dir, flags))
		return true;

	if(walkTicks <= 0 || !isIdle || focusCreature || getTimeSinceLastMove() < walkTicks)
		return false;

	return getRandomStep(dir);
}

bool Npc::canWalkTo(const Position& fromPos, Direction dir)
{
	if(getNoMove())
		return false;

	Position toPos = fromPos;
	toPos = getNextPosition(dir, toPos);
	if(!Spawns::getInstance()->isInZone(masterPosition, masterRadius, toPos))
		return false;

	Tile* tile = g_game.getTile(toPos);
	if(!tile)
		return true;

	if(getTile()->isSwimmingPool() != tile->isSwimmingPool()) // prevent npc entering/exiting to swimming pool
		return false;

	if(floorChange && (tile->floorChange() || tile->positionChange()))
		return true;

	return tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR;
}

bool Npc::getRandomStep(Direction& dir)
{
	std::vector<Direction> dirList;
	const Position& creaturePos = getPosition();
	if(canWalkTo(creaturePos, NORTH))
		dirList.push_back(NORTH);

	if(canWalkTo(creaturePos, SOUTH))
		dirList.push_back(SOUTH);

	if(canWalkTo(creaturePos, EAST))
		dirList.push_back(EAST);

	if(canWalkTo(creaturePos, WEST))
		dirList.push_back(WEST);

	if(dirList.empty())
		return false;

	std::random_shuffle(dirList.begin(), dirList.end());
	dir = dirList[random_range(0, dirList.size() - 1)];
	return true;
}

void Npc::setCreatureFocus(Creature* creature)
{
	if(!creature)
	{
		focusCreature = 0;
		return;
	}

	const Position& creaturePos = creature->getPosition();
	const Position& myPos = getPosition();
	int32_t dx = myPos.x - creaturePos.x, dy = myPos.y - creaturePos.y;

	float tan = 10;
	if(dx != 0)
		tan = dy / dx;

	Direction dir = SOUTH;
	if(std::abs(tan) < 1)
	{
		if(dx > 0)
			dir = WEST;
		else
			dir = EAST;
	}
	else if(dy > 0)
		dir = NORTH;

	focusCreature = creature->getID();
	g_game.internalCreatureTurn(this, dir);
}

const NpcResponse* Npc::getResponse(const ResponseList& list, const Player* player,
	NpcState* npcState, const std::string& text, bool exactMatch /*= false*/)
{
	std::string textString = asLowerCaseString(text);
	StringVec wordList = explodeString(textString, " ");
	int32_t bestMatchCount = 0, totalMatchCount = 0;

	NpcResponse* response = NULL;
	for(ResponseList::const_iterator it = list.begin(); it != list.end(); ++it)
	{
		int32_t matchCount = 0;
		if((*it)->getParams() != RESPOND_DEFAULT)
		{
			uint32_t params = (*it)->getParams();
			if(hasBitSet(RESPOND_MALE, params))
			{
				if(!player->getSex(false))
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_FEMALE, params))
			{
				if(player->getSex(false))
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_PZBLOCK, params))
			{
				if(!player->isPzLocked())
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_PREMIUM, params))
			{
				if(!player->isPremium())
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_DRUID, params))
			{
				Vocation* tmpVoc = player->vocation;
				for(uint32_t i = 0; i <= player->promotionLevel; i++)
					tmpVoc = Vocations::getInstance()->getVocation(tmpVoc->getFromVocation());

				if(tmpVoc->getId() != 2)
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_KNIGHT, params))
			{
				Vocation* tmpVoc = player->vocation;
				for(uint32_t i = 0; i <= player->promotionLevel; i++)
					tmpVoc = Vocations::getInstance()->getVocation(tmpVoc->getFromVocation());

				if(tmpVoc->getId() != 4)
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_PALADIN, params))
			{
				Vocation* tmpVoc = player->vocation;
				for(uint32_t i = 0; i <= player->promotionLevel; i++)
					tmpVoc = Vocations::getInstance()->getVocation(tmpVoc->getFromVocation());

				if(tmpVoc->getId() != 3)
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_SORCERER, params))
			{
				Vocation* tmpVoc = player->vocation;
				for(uint32_t i = 0; i <= player->promotionLevel; i++)
					tmpVoc = Vocations::getInstance()->getVocation(tmpVoc->getFromVocation());

				if(tmpVoc->getId() != 1)
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_LOWLEVEL, params))
			{
				if((int32_t)player->getLevel() >= npcState->level)
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_LOWMONEY, params))
			{
				if((signed)g_game.getMoney(player) >= (npcState->price * npcState->amount))
					continue;

				++matchCount;
			}

			if(hasBitSet(RESPOND_LOWAMOUNT, params) || hasBitSet(RESPOND_NOAMOUNT, params))
			{
				if((signed)player->__getItemTypeCount(npcState->itemId) >= npcState->amount)
					continue;

				if(hasBitSet(RESPOND_LOWAMOUNT, params))
				{
					if(npcState->amount == 1)
						continue;

					++matchCount;
				}

				if(hasBitSet(RESPOND_NOAMOUNT, params))
				{
					if(npcState->amount > 1)
						continue;

					++matchCount;
				}
			}
		}

		if((*it)->getKnowSpell() != "")
		{
			std::string spellName = (*it)->getKnowSpell();
			if(spellName == "|SPELL|")
				spellName = npcState->spellName;

			if(!player->hasLearnedInstantSpell(spellName))
				continue;

			++matchCount;
		}

		if((*it)->scriptVars.b1)
		{
			if(!npcState->scriptVars.b1)
				continue;

			++matchCount;
		}

		if((*it)->scriptVars.b2)
		{
			if(!npcState->scriptVars.b2)
				continue;

			++matchCount;
		}

		if((*it)->scriptVars.b3)
		{
			if(!npcState->scriptVars.b3)
				continue;

			++matchCount;
		}

		if((*it)->getStorageId() != -1)
		{
			std::string value, storageValue = (*it)->getStorage();
			player->getStorage((*it)->getStorageId(), value);
			if(asLowerCaseString(storageValue) == "_time")
			{
				std::stringstream s;
				s << time(NULL);
				storageValue = s.str();
			}

			bool tmp = false;
			switch((*it)->getStorageComp())
			{
				case STORAGE_LESS:
				{
					int32_t v1 = atoi(value.c_str()), v2 = atoi(storageValue.c_str());
					tmp = v1 < v2;
					break;
				}
				case STORAGE_LESSOREQUAL:
				{
					int32_t v1 = atoi(value.c_str()), v2 = atoi(storageValue.c_str());
					tmp = v1 <= v2;
					break;
				}
				case STORAGE_EQUAL:
				{
					tmp = value == storageValue;
					break;
				}
				case STORAGE_NOTEQUAL:
				{
					tmp = value != storageValue;
					break;
				}
				case STORAGE_GREATEROREQUAL:
				{
					int32_t v1 = atoi(value.c_str()), v2 = atoi(storageValue.c_str());
					tmp = v1 >= v2;
					break;
				}
				case STORAGE_GREATER:
				{
					int32_t v1 = atoi(value.c_str()), v2 = atoi(storageValue.c_str());
					tmp = v1 > v2;
					break;
				}
				default:
					break;
			}

			if(!tmp)
				continue;

			++matchCount;
		}

		if((*it)->getInteractType() == INTERACT_TEXT || (*it)->getFocusState() != -1)
		{
			if(npcState->isIdle && (*it)->getFocusState() != 1) //We are idle, and this response does not activate the npc.
				continue;

			if(!npcState->isIdle && (*it)->getFocusState() == 1) //We are not idle, and this response would activate us again.
				continue;
		}

		if(npcState->topic == -1 && (*it)->getTopic() != -1) //Not the right topic
			continue;

		if(npcState->topic != -1 && npcState->topic == (*it)->getTopic()) //Topic is right
			matchCount += 1000;

		if((*it)->getInteractType() == INTERACT_EVENT)
		{
			if((*it)->getInputText() == asLowerCaseString(text))
				++matchCount;
			else
				matchCount = 0;
		}
		else if((*it)->getInteractType() == INTERACT_TEXT)
		{
			int32_t matchAllCount = 0, totalKeywordCount = 0, matchWordCount = getMatchCount(
				*it, wordList, exactMatch, matchAllCount, totalKeywordCount);
			if(matchWordCount> 0)
			{
				//Remove points for |*| matches
				matchWordCount -= matchAllCount;
				//Remove points for not full match
				matchWordCount -= (totalKeywordCount - matchAllCount - matchWordCount);
				//Total "points" for this response, word matches are worth more
				matchCount += matchWordCount * 100000;
			}
			else
				matchCount = 0;
		}

		if(matchCount > bestMatchCount)
		{
			totalMatchCount = 0;
			response = (*it);
			bestMatchCount = matchCount;
		}
		else if(bestMatchCount > 0 && matchCount == bestMatchCount)
			++totalMatchCount;
	}

	if(totalMatchCount > 1)
		return NULL;

	return response;
}

uint32_t Npc::getMatchCount(NpcResponse* response, StringVec wordList,
	bool exactMatch, int32_t& matchAllCount, int32_t& totalKeywordCount)
{
	int32_t bestMatchCount = matchAllCount = totalKeywordCount = 0;
	const std::list<std::string>& inputList = response->getInputList();
	for(std::list<std::string>::const_iterator it = inputList.begin(); it != inputList.end(); ++it)
	{
		std::string keywords = (*it), tmpKit;
		StringVec::iterator lastWordMatch = wordList.begin();

		int32_t matchCount = 0;
		StringVec keywordList = explodeString(keywords, ";");
		for(StringVec::iterator kit = keywordList.begin(); kit != keywordList.end(); ++kit)
		{
			tmpKit = asLowerCaseString(*kit);
			if(!exactMatch && (*kit) == "|*|") //Match anything.
				matchAllCount++;
			else if(tmpKit == "|amount|")
			{
				//TODO: Should iterate through each word until a number or a new keyword is found.
				int32_t amount = atoi((*lastWordMatch).c_str());
				if(amount <= 0)
				{
					response->setAmount(1);
					continue;
				}

				response->setAmount(amount);
			}
			else
			{
				StringVec::iterator wit = wordList.end();
				for(wit = lastWordMatch; wit != wordList.end(); ++wit)
				{
					size_t pos = (*wit).find_first_of("!\"#�%&/()=?`{[]}\\^*><,.-_~");
					if(pos == std::string::npos)
						pos = 0;

					if((*wit).find((*kit), pos) == pos)
						break;
				}

				if(wit == wordList.end())
					continue;

				lastWordMatch = wit + 1;
			}

			++matchCount;
			if(matchCount > bestMatchCount)
			{
				bestMatchCount = matchCount;
				totalKeywordCount = keywordList.size();
			}

			if(lastWordMatch == wordList.end())
				break;
		}
	}

	return bestMatchCount;
}

const NpcResponse* Npc::getResponse(const Player* player, NpcState* npcState, const std::string& text)
{
	return getResponse(responseList, player, npcState, text);
}

const NpcResponse* Npc::getResponse(const Player* player, NpcEvent_t eventType)
{
	std::string eventName = getEventResponseName(eventType);
	if(eventName.empty())
		return NULL;

	std::vector<NpcResponse*> result;
	for(ResponseList::const_iterator it = responseList.begin(); it != responseList.end(); ++it)
	{
		if((*it)->getInteractType() == INTERACT_EVENT)
		{
			if((*it)->getInputText() == asLowerCaseString(eventName))
				result.push_back(*it);
		}
	}

	if(result.empty())
		return NULL;

	return result[random_range(0, result.size() - 1)];
}

const NpcResponse* Npc::getResponse(const Player* player, NpcState* npcState, NpcEvent_t eventType)
{
	std::string eventName = getEventResponseName(eventType);
	if(eventName.empty())
		return NULL;

	return getResponse(responseList, player, npcState, eventName, true);
}

std::string Npc::getEventResponseName(NpcEvent_t eventType)
{
	switch(eventType)
	{
		case EVENT_BUSY:
			return "onBusy";
		case EVENT_THINK:
			return "onThink";
		case EVENT_IDLE:
			return "onIdle";
		case EVENT_PLAYER_ENTER:
			return "onPlayerEnter";
		case EVENT_PLAYER_MOVE:
			return "onPlayerMove";
		case EVENT_PLAYER_LEAVE:
			return "onPlayerLeave";
		case EVENT_PLAYER_SHOPSELL:
			return "onPlayerShopSell";
		case EVENT_PLAYER_SHOPBUY:
			return "onPlayerShopBuy";
		case EVENT_PLAYER_SHOPCLOSE:
			return "onPlayerShopClose";
		default:
			break;
	}

	return "";
}

std::string Npc::formatResponse(Creature* creature, const NpcState* npcState, const NpcResponse* response) const
{
	std::string responseString = response->getText();

	std::stringstream ss;
	ss << npcState->price * npcState->amount;
	replaceString(responseString, "|PRICE|", ss.str());

	ss.str("");
	ss << npcState->amount;
	replaceString(responseString, "|AMOUNT|", ss.str());

	ss.str("");
	ss << npcState->level;
	replaceString(responseString, "|LEVEL|", ss.str());

	ss.str("");
	ss << npcState->scriptVars.n1;
	replaceString(responseString, "|N1|", ss.str());

	ss.str("");
	ss << npcState->scriptVars.n2;
	replaceString(responseString, "|N2|", ss.str());

	ss.str("");
	ss << npcState->scriptVars.n3;
	replaceString(responseString, "|N3|", ss.str());

	replaceString(responseString, "|S1|", npcState->scriptVars.s1);
	replaceString(responseString, "|S2|", npcState->scriptVars.s2);
	replaceString(responseString, "|S3|", npcState->scriptVars.s3);

	ss.str("");
	if(npcState->itemId != -1)
	{
		const ItemType& it = Item::items[npcState->itemId];
		if(npcState->amount <= 1)
		{
			ss << it.article + " " + it.name;
			replaceString(responseString, "|ITEMNAME|", ss.str());
		}
		else
		{
			ss << it.pluralName;
			replaceString(responseString, "|ITEMNAME|", ss.str());
		}
	}

	replaceString(responseString, "|NAME|", creature->getName());
	replaceString(responseString, "|NPCNAME|", getName());
	return responseString;
}

void Npc::addShopPlayer(Player* player)
{
	ShopPlayerList::iterator it = std::find(shopPlayerList.begin(), shopPlayerList.end(), player);
	if(it == shopPlayerList.end())
		shopPlayerList.push_back(player);
}

void Npc::removeShopPlayer(const Player* player)
{
	ShopPlayerList::iterator it = std::find(shopPlayerList.begin(), shopPlayerList.end(), player);
	if(it != shopPlayerList.end())
		shopPlayerList.erase(it);
}

void Npc::closeAllShopWindows()
{
	ShopPlayerList closeList = shopPlayerList;
	for(ShopPlayerList::iterator it = closeList.begin(); it != closeList.end(); ++it)
		(*it)->closeShopWindow();
}

NpcScriptInterface* Npc::getInterface()
{
	return m_interface;
}

NpcScriptInterface::NpcScriptInterface() :
	LuaScriptInterface("Npc interface")
{
	m_libLoaded = false;
	initState();
}


NpcScriptInterface::~NpcScriptInterface()
{
	//
}

bool NpcScriptInterface::initState()
{
	return LuaScriptInterface::initState();
}

bool NpcScriptInterface::closeState()
{
	m_libLoaded = false;
	return LuaScriptInterface::closeState();
}

bool NpcScriptInterface::loadNpcLib(std::string file)
{
	if(m_libLoaded)
		return true;

	if(!loadFile(file))
	{
		std::cout << "Warning: [NpcScriptInterface::loadNpcLib] Cannot load " << file << std::endl;
		return false;
	}

	m_libLoaded = true;
	return true;
}

void NpcScriptInterface::registerFunctions()
{
	LuaScriptInterface::registerFunctions();
	lua_register(m_luaState, "selfFocus", NpcScriptInterface::luaActionFocus);
	lua_register(m_luaState, "selfSay", NpcScriptInterface::luaActionSay);

	lua_register(m_luaState, "selfTurn", NpcScriptInterface::luaActionTurn);
	lua_register(m_luaState, "selfMove", NpcScriptInterface::luaActionMove);
	lua_register(m_luaState, "selfMoveTo", NpcScriptInterface::luaActionMoveTo);
	lua_register(m_luaState, "selfFollow", NpcScriptInterface::luaActionFollow);

	lua_register(m_luaState, "getNpcId", NpcScriptInterface::luaGetNpcId);
	lua_register(m_luaState, "getNpcDistanceTo", NpcScriptInterface::luaGetNpcDistanceTo);
	lua_register(m_luaState, "getNpcParameter", NpcScriptInterface::luaGetNpcParameter);

	lua_register(m_luaState, "getNpcState", NpcScriptInterface::luaGetNpcState);
	lua_register(m_luaState, "setNpcState", NpcScriptInterface::luaSetNpcState);

	lua_register(m_luaState, "openShopWindow", NpcScriptInterface::luaOpenShopWindow);
	lua_register(m_luaState, "closeShopWindow", NpcScriptInterface::luaCloseShopWindow);
}

int32_t NpcScriptInterface::luaActionFocus(lua_State* L)
{
	//selfFocus(cid)
	ScriptEnviroment* env = getEnv();

	Npc* npc = env->getNpc();
	if(!npc)
		return 0;

	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(creature)
		npc->hasScriptedFocus = true;
	else
		npc->hasScriptedFocus = false;

	npc->setCreatureFocus(creature);
	return 0;
}

int32_t NpcScriptInterface::luaActionSay(lua_State* L)
{
	//selfSay(words[, target[, type]])
	int32_t params = lua_gettop(L), target = 0;
	SpeakClasses type = SPEAK_CLASS_NONE;
	if(params > 2)
		type = (SpeakClasses)popNumber(L);

	if(params > 1)
		target = popNumber(L);

	ScriptEnviroment* env = getEnv();
	Npc* npc = env->getNpc();
	if(!npc)
		return 0;

	Player* player = env->getPlayerByUID(target);
	if(type == SPEAK_CLASS_NONE)
	{
		if(player)
			type = SPEAK_PRIVATE_NP;
		else
			type = SPEAK_SAY;
	}

	npc->doSay(popString(L), (SpeakClasses)type, player);
	return 0;
}

int32_t NpcScriptInterface::luaActionTurn(lua_State* L)
{
	//selfTurn(direction)
	ScriptEnviroment* env = getEnv();
	if(Npc* npc = env->getNpc())
		npc->doTurn((Direction)popNumber(L));

	return 0;
}

int32_t NpcScriptInterface::luaActionMove(lua_State* L)
{
	//selfMove(direction)
	ScriptEnviroment* env = getEnv();
	if(Npc* npc = env->getNpc())
		npc->doMove((Direction)popNumber(L));

	return 0;
}

int32_t NpcScriptInterface::luaActionMoveTo(lua_State* L)
{
	//selfMoveTo(x, y, z)
	Position pos;
	pos.z = (uint16_t)popNumber(L);
	pos.y = (uint16_t)popNumber(L);
	pos.x = (uint16_t)popNumber(L);

	ScriptEnviroment* env = getEnv();
	if(Npc* npc = env->getNpc())
		npc->doMoveTo(pos);

	return 0;
}

int32_t NpcScriptInterface::luaActionFollow(lua_State* L)
{
	//selfFollow(cid)
	uint32_t cid = popNumber(L);
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID(cid);
	if(cid && !player)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	Npc* npc = env->getNpc();
	if(!npc)
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, npc->setFollowCreature(player, true));
	return 1;
}

int32_t NpcScriptInterface::luaGetNpcId(lua_State* L)
{
	//getNpcId()
	ScriptEnviroment* env = getEnv();
	if(Npc* npc = env->getNpc())
		lua_pushnumber(L, env->addThing(npc));
	else
		lua_pushnil(L);

	return 1;
}

int32_t NpcScriptInterface::luaGetNpcDistanceTo(lua_State* L)
{
	//getNpcDistanceTo(uid)
	ScriptEnviroment* env = getEnv();
	Npc* npc = env->getNpc();

	Thing* thing = env->getThingByUID(popNumber(L));
	if(thing && npc)
	{
		Position thingPos = thing->getPosition();
		Position npcPos = npc->getPosition();
		if(npcPos.z == thingPos.z)
			lua_pushnumber(L, std::max(std::abs(npcPos.x - thingPos.x), std::abs(npcPos.y - thingPos.y)));
		else
			lua_pushnumber(L, -1);
	}
	else
	{
		errorEx(getError(LUA_ERROR_THING_NOT_FOUND));
		lua_pushnil(L);
	}

	return 1;
}

int32_t NpcScriptInterface::luaGetNpcParameter(lua_State* L)
{
	//getNpcParameter(paramKey)
	ScriptEnviroment* env = getEnv();
	if(Npc* npc = env->getNpc())
	{
		Npc::ParametersMap::iterator it = npc->m_parameters.find(popString(L));
		if(it != npc->m_parameters.end())
			lua_pushstring(L, it->second.c_str());
		else
			lua_pushnil(L);
	}
	else
		lua_pushnil(L);

	return 1;
}

int32_t NpcScriptInterface::luaGetNpcState(lua_State* L)
{
	//getNpcState(cid)
	ScriptEnviroment* env = getEnv();
	Npc* npc = env->getNpc();

	const Player* player = env->getPlayerByUID(popNumber(L));
	if(player && npc)
		NpcScriptInterface::pushState(L, npc->getState(player));
	else
		lua_pushnil(L);

	return 1;
}

int32_t NpcScriptInterface::luaSetNpcState(lua_State* L)
{
	//setNpcState(state, cid)
	ScriptEnviroment* env = getEnv();
	Npc* npc = env->getNpc();

	const Player* player = env->getPlayerByUID(popNumber(L));
	if(player && npc)
	{
		NpcState* tmp = npc->getState(player);
		NpcScriptInterface::popState(L, tmp);
		lua_pushboolean(L, true);
	}
	else
		lua_pushboolean(L, false);

	return 1;
}

void NpcScriptInterface::pushState(lua_State* L, NpcState* state)
{
	lua_newtable(L);
	setField(L, "price", state->price);
	setField(L, "amount", state->amount);
	setField(L, "itemid", state->itemId);
	setField(L, "subtype", state->subType);
	setFieldBool(L, "ignorecap", state->ignoreCap);
	setFieldBool(L, "inbackpacks", state->inBackpacks);
	setField(L, "topic", state->topic);
	setField(L, "level", state->level);
	setField(L, "spellname", state->spellName);
	setField(L, "listname", state->listName);
	setField(L, "listpname", state->listPluralName);
	setFieldBool(L, "isidle", state->isIdle);

	setField(L, "n1", state->scriptVars.n1);
	setField(L, "n2", state->scriptVars.n2);
	setField(L, "n3", state->scriptVars.n3);

	setFieldBool(L, "b1", state->scriptVars.b1);
	setFieldBool(L, "b2", state->scriptVars.b2);
	setFieldBool(L, "b3", state->scriptVars.b3);

	setField(L, "s1", state->scriptVars.s1);
	setField(L, "s2", state->scriptVars.s2);
	setField(L, "s3", state->scriptVars.s3);
}

void NpcScriptInterface::popState(lua_State* L, NpcState* &state)
{
	state->price = getField(L, "price");
	state->amount = getField(L, "amount");
	state->itemId = getField(L, "itemid");
	state->subType = getField(L, "subtype");
	state->ignoreCap = getFieldBool(L, "ignorecap");
	state->inBackpacks = getFieldBool(L, "inbackpacks");
	state->topic = getField(L, "topic");
	state->level = getField(L, "level");
	state->spellName = getFieldString(L, "spellname");
	state->listName = getFieldString(L, "listname");
	state->listPluralName = getFieldString(L, "listpname");
	state->isIdle = getFieldBool(L, "isidle");

	state->scriptVars.n1 = getField(L, "n1");
	state->scriptVars.n2 = getField(L, "n2");
	state->scriptVars.n3 = getField(L, "n3");

	state->scriptVars.b1 = getFieldBool(L, "b1");
	state->scriptVars.b2 = getFieldBool(L, "b2");
	state->scriptVars.n3 = getFieldBool(L, "b3");

	state->scriptVars.s1 = getFieldString(L, "s1");
	state->scriptVars.s2 = getFieldString(L, "s2");
	state->scriptVars.s3 = getFieldString(L, "s3");
}

int32_t NpcScriptInterface::luaOpenShopWindow(lua_State* L)
{
	//openShopWindow(cid, items, onBuy callback, onSell callback)
	ScriptEnviroment* env = getEnv();
	Npc* npc = env->getNpc();
	if(!npc)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	int32_t sellCallback = -1;
	if(lua_isfunction(L, -1) == 0)
		lua_pop(L, 1); // skip it - use default value
	else
		sellCallback = popCallback(L);

	int32_t buyCallback = -1;
	if(lua_isfunction(L, -1) == 0)
		lua_pop(L, 1);
	else
		buyCallback = popCallback(L);

	if(!lua_istable(L, -1))
	{
		error(__FUNCTION__, "item list is not a table.");
		lua_pushboolean(L, false);
		return 1;
	}

	ShopInfoList itemList;
	lua_pushnil(L);
	while(lua_next(L, -2))
	{
		ShopInfo item;
		item.itemId = getField(L, "id");
		item.subType = getField(L, "subType");
		item.buyPrice = getField(L, "buy");
		item.sellPrice = getField(L, "sell");
		item.itemName = getFieldString(L, "name");

		itemList.push_back(item);
		lua_pop(L, 1);
	}

	lua_pop(L, 1);
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	player->closeShopWindow();
	npc->addShopPlayer(player);

	player->setShopOwner(npc, buyCallback, sellCallback, itemList);
	player->openShopWindow();

	lua_pushboolean(L, true);
	return 1;
}

int32_t NpcScriptInterface::luaCloseShopWindow(lua_State* L)
{
	//closeShopWindow(cid)
	ScriptEnviroment* env = getEnv();

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		errorEx(getError(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	Npc* npc = env->getNpc();
	if(!npc)
	{
		errorEx(getError(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, false);
		return 1;
	}

	int32_t onBuy, onSell;
	Npc* merchant = player->getShopOwner(onBuy, onSell);
	if(merchant == npc)
		player->closeShopWindow(npc, onBuy, onSell);

	return 1;
}

NpcEventsHandler::NpcEventsHandler(Npc* npc)
{
	m_npc = npc;
	m_loaded = false;
}

bool NpcEventsHandler::isLoaded()
{
	return m_loaded;
}

NpcScript::NpcScript(std::string file, Npc* npc):
	NpcEventsHandler(npc)
{
	m_interface = npc->getInterface();
	if(!m_interface->loadFile(file, npc))
	{
		std::cout << "[Warning - NpcScript::NpcScript] Cannot load script: " << file << std::endl;
		std::cout << m_interface->getLastError() << std::endl;

		m_loaded = false;
		return;
	}

	m_onCreatureSay = m_interface->getEvent("onCreatureSay");
	m_onCreatureDisappear = m_interface->getEvent("onCreatureDisappear");
	m_onCreatureAppear = m_interface->getEvent("onCreatureAppear");
	m_onCreatureMove = m_interface->getEvent("onCreatureMove");
	m_onPlayerCloseChannel = m_interface->getEvent("onPlayerCloseChannel");
	m_onPlayerEndTrade = m_interface->getEvent("onPlayerEndTrade");
	m_onThink = m_interface->getEvent("onThink");
	m_loaded = true;
}

void NpcScript::onCreatureAppear(const Creature* creature)
{
	if(m_onCreatureAppear == -1)
		return;

	//onCreatureAppear(cid)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		lua_State* L = m_interface->getState();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_onCreatureAppear, m_interface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		m_interface->pushFunction(m_onCreatureAppear);
		lua_pushnumber(L, env->addThing(const_cast<Creature*>(creature)));

		m_interface->callFunction(1);
		m_interface->releaseEnv();
	}
	else
		std::cout << "[Error - NpcScript::onCreatureAppear] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onCreatureDisappear(const Creature* creature)
{
	if(m_onCreatureDisappear == -1)
		return;

	//onCreatureDisappear(cid)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		lua_State* L = m_interface->getState();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_onCreatureDisappear, m_interface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		m_interface->pushFunction(m_onCreatureDisappear);
		lua_pushnumber(L, env->addThing(const_cast<Creature*>(creature)));

		m_interface->callFunction(1);
		m_interface->releaseEnv();
	}
	else
		std::cout << "[Error - NpcScript::onCreatureDisappear] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onCreatureMove(const Creature* creature, const Position& oldPos, const Position& newPos)
{
	if(m_onCreatureMove == -1)
		return;

	//onCreatureMove(cid, oldPos, newPos)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		lua_State* L = m_interface->getState();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_onCreatureMove, m_interface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		m_interface->pushFunction(m_onCreatureMove);
		lua_pushnumber(L, env->addThing(const_cast<Creature*>(creature)));

		LuaScriptInterface::pushPosition(L, oldPos, 0);
		LuaScriptInterface::pushPosition(L, newPos, 0);

		m_interface->callFunction(3);
		m_interface->releaseEnv();
	}
	else
		std::cout << "[Error - NpcScript::onCreatureMove] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text, Position* pos/* = NULL*/)
{
	if(m_onCreatureSay == -1)
		return;

	//onCreatureSay(cid, type, msg)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		lua_State* L = m_interface->getState();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_onCreatureSay, m_interface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		m_interface->pushFunction(m_onCreatureSay);
		lua_pushnumber(L, env->addThing(const_cast<Creature*>(creature)));

		lua_pushnumber(L, type);
		lua_pushstring(L, text.c_str());

		m_interface->callFunction(3);
		m_interface->releaseEnv();
	}
	else
		std::cout << "[Error - NpcScript::onCreatureSay] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onPlayerTrade(const Player* player, int32_t callback, uint16_t itemid,
	uint8_t count, uint8_t amount, bool ignoreCap, bool inBackpacks)
{
	if(callback == -1)
		return;

	//on"Buy/Sell"(cid, itemid, count, amount, ignoreCap, inBackpacks)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		lua_State* L = m_interface->getState();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(-1, m_interface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		uint32_t cid = env->addThing(const_cast<Player*>(player));
		LuaScriptInterface::pushCallback(L, callback);

		lua_pushnumber(L, cid);
		lua_pushnumber(L, itemid);
		lua_pushnumber(L, count);
		lua_pushnumber(L, amount);

		lua_pushboolean(L, ignoreCap);
		lua_pushboolean(L, inBackpacks);

		m_interface->callFunction(6);
		m_interface->releaseEnv();
	}
	else
		std::cout << "[Error - NpcScript::onPlayerTrade] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onPlayerCloseChannel(const Player* player)
{
	if(m_onPlayerCloseChannel == -1)
		return;

	//onPlayerCloseChannel(cid)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		lua_State* L = m_interface->getState();

		env->setScriptId(m_onPlayerCloseChannel, m_interface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		m_interface->pushFunction(m_onPlayerCloseChannel);
		lua_pushnumber(L, env->addThing(const_cast<Player*>(player)));

		m_interface->callFunction(1);
		m_interface->releaseEnv();
	}
	else
		std::cout << "[Error - NpcScript::onPlayerCloseChannel] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onPlayerEndTrade(const Player* player)
{
	if(m_onPlayerCloseChannel == -1)
		return;

	//onPlayerEndTrade(cid)
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();
		lua_State* L = m_interface->getState();

		env->setScriptId(m_onPlayerEndTrade, m_interface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		m_interface->pushFunction(m_onPlayerEndTrade);
		lua_pushnumber(L, env->addThing(const_cast<Player*>(player)));

		m_interface->callFunction(1);
		m_interface->releaseEnv();
	}
	else
		std::cout << "[Error - NpcScript::onPlayerEndTrade] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

void NpcScript::onThink()
{
	if(m_onThink == -1)
		return;

	//onThink()
	if(m_interface->reserveEnv())
	{
		ScriptEnviroment* env = m_interface->getEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "npc " << m_npc->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_onThink, m_interface);
		env->setRealPos(m_npc->getPosition());
		env->setNpc(m_npc);

		m_interface->pushFunction(m_onThink);

		m_interface->callFunction(0);
		m_interface->releaseEnv();
	}
	else
		std::cout << "[Error - NpcScript::onThink] NPC Name: " << m_npc->getName() << " - Call stack overflow" << std::endl;
}

