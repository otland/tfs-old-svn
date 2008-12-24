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

#include <algorithm>

#include "house.h"
#include "iologindata.h"
#include "game.h"
#include "town.h"
#include "configmanager.h"
#include "tools.h"
#include "beds.h"

extern ConfigManager g_config;
extern Game g_game;

House::House(uint32_t _houseid) :
transferContainer(ITEM_LOCKER1)
{
	isLoaded = false;
	houseName = "Forgotten headquarter (Flat 1, Area 42)";
	houseOwner = 0;
	posEntry.x = 0;
	posEntry.y = 0;
	posEntry.z = 0;
	paidUntil = 0;
	houseid = _houseid;
	rentWarnings = 0;
	lastWarning = 0;
	rent = 0;
	price = 0;
	townid = 0;
	transferItem = NULL;
}

House::~House()
{
	//
}

void House::addTile(HouseTile* tile)
{
	tile->setFlag(TILESTATE_PROTECTIONZONE);
	houseTiles.push_back(tile);
}

void House::setHouseOwner(uint32_t guid, bool cleaned/* = true*/)
{
	if(isLoaded && houseOwner == guid)
		return;

	isLoaded = true;
	if(houseOwner)
	{
		if(cleaned)
			clean();

		setAccessList(SUBOWNER_LIST, "", !cleaned);
		setAccessList(GUEST_LIST, "", !cleaned);
		for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it)
			(*it)->setAccessList("");

		houseOwner = 0;
		paidUntil = 0;
		rentWarnings = 0;
	}

	if(guid != 0)
		houseOwner = guid;

	updateDoorDescription();
	setLastWarning(time(NULL)); //So the new owner has one day before he start the payement
}

void House::updateDoorDescription(std::string name/* = ""*/)
{
	char houseDescription[200];
	if(houseOwner != 0)
	{
		if(name == "")
			IOLoginData::getInstance()->getNameByGuid(houseOwner, name);

		sprintf(houseDescription, "It belongs to house '%s'. %s owns this house.", houseName.c_str(), name.c_str());
	}
	else
		sprintf(houseDescription, "It belongs to house '%s'. Nobody owns this house. It costs %d gold coins.", houseName.c_str(), price);

	for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it)
		(*it)->setSpecialDescription(houseDescription);
}

AccessHouseLevel_t House::getHouseAccessLevel(const Player* player)
{
	if(player)
	{
		if(player->hasFlag(PlayerFlag_CanEditHouses))
			return HOUSE_OWNER;

		if(player->getGUID() == houseOwner)
			return HOUSE_OWNER;

		if(subOwnerList.isInList(player))
			return HOUSE_SUBOWNER;

		if(guestList.isInList(player))
			return HOUSE_GUEST;
	}

	return HOUSE_NO_INVITED;
}

void House::removePlayer(Player* player, bool ignoreRights)
{
	if(ignoreRights && player->hasFlag(PlayerFlag_CanEditHouses))
		return;

	Position tmp = player->getPosition();
	if(g_game.internalTeleport(player, g_game.getClosestFreeTile(player, getEntryPosition()), true) == RET_NOERROR && !player->isInGhostMode())
	{
		g_game.addMagicEffect(tmp, NM_ME_POFF);
		g_game.addMagicEffect(player->getPosition(), NM_ME_TELEPORT);
	}
}

void House::removePlayers(bool ignoreInvites)
{
	PlayerVector kickList;
	for(HouseTileList::iterator it = houseTiles.begin(); it != houseTiles.end(); ++it)
	{
		if(!(*it)->creatures.size())
			continue;

		for(CreatureVector::iterator cit = (*it)->creatures.begin(); cit != (*it)->creatures.end(); ++cit)
		{
			Player* player = (*cit)->getPlayer();
			if(player && !player->isRemoved() && (ignoreInvites || !isInvited(player)))
				kickList.push_back(player);
		}
	}

	if(kickList.size())
	{
		for(PlayerVector::iterator it = kickList.begin(); it != kickList.end(); ++it)
			removePlayer((*it), true);
	}
}

void House::clean()
{
	transferToDepot();
	removePlayers(true);
	for(HouseBedItemList::iterator bit = bedsList.begin(); bit != bedsList.end(); ++bit)
	{
		if((*bit)->getSleeper() != 0)
			(*bit)->wakeUp(NULL);
	}
}

bool House::kickPlayer(Player* player, Player* target)
{
	if(target && !target->isRemoved())
	{
		HouseTile* houseTile = dynamic_cast<HouseTile*>(target->getTile());
		if(houseTile && houseTile->getHouse() == this)
		{
			if(player == target)
			{
				removePlayer(target, false);
				return true;
			}

			if(getHouseAccessLevel(player) >= getHouseAccessLevel(target))
			{
				removePlayer(target, true);
				return true;
			}
		}
	}

	return false;
}

void House::setPrice(uint32_t _price, bool update /*= false*/)
{
	price = _price;
	if(update && !getHouseOwner())
		updateDoorDescription();
}

void House::setAccessList(uint32_t listId, const std::string& textlist, bool teleport/* = true*/)
{
	if(listId == GUEST_LIST)
		guestList.parseList(textlist);
	else if(listId == SUBOWNER_LIST)
		subOwnerList.parseList(textlist);
	else
	{
		if(Door* door = getDoorByNumber(listId))
			door->setAccessList(textlist);
		#ifdef __DEBUG_HOUSES__
		else
			std::cout << "Failure: [House::setAccessList] door == NULL, listId = " << listId <<std::endl;
		#endif

		return;
	}

	if(teleport)
		removePlayers(false);
}

bool House::transferToDepot()
{
	if(townid == 0)
		return false;

	Player* player = NULL;
	if(houseOwner != 0)
	{
		std::string ownerName;
		if(IOLoginData::getInstance()->getNameByGuid(houseOwner, ownerName))
			player = g_game.getPlayerByName(ownerName);
		
		if(!player)
		{
			player = new Player(ownerName, NULL);
			if(!IOLoginData::getInstance()->loadPlayer(player, ownerName))
			{
#ifdef __DEBUG__
				std::cout << "Failure: [House::transferToDepot], can not load player: " << ownerName << std::endl;
#endif
				delete player;
			}
		}
	}

	std::list<Item*> moveItemList;
	Container* tmpContainer = NULL;
	Item* item = NULL;

	for(HouseTileList::iterator it = houseTiles.begin(); it != houseTiles.end(); ++it)
	{
		for(uint32_t i = 0; i < (*it)->getThingCount(); ++i)
		{
			item = (*it)->__getThing(i)->getItem();
			if(!item)
				continue;

			if(item->isPickupable())
				moveItemList.push_back(item);
			else if((tmpContainer = item->getContainer()))
			{
				for(ItemList::const_iterator it = tmpContainer->getItems(); it != tmpContainer->getEnd(); ++it)
					moveItemList.push_back(*it);
			}
		}
	}

	Depot* depot = NULL;
	if(player)
		depot = player->getDepot(townid, true);

	for(std::list<Item*>::iterator it = moveItemList.begin(); it != moveItemList.end(); ++it)
	{
		g_game.internalMoveItem(NULL, (*it)->getParent(), depot, INDEX_WHEREEVER,
			(*it), (*it)->getItemCount(), NULL, FLAG_NOLIMIT);
	}

	if(player && player->isVirtual())
	{
		IOLoginData::getInstance()->savePlayer(player);
		delete player;
	}

	return true;
}

bool House::getAccessList(uint32_t listId, std::string& list) const
{
	if(listId == GUEST_LIST)
	{
		guestList.getList(list);
		return true;
	}
	else if(listId == SUBOWNER_LIST)
	{
		subOwnerList.getList(list);
		return true;
	}
	else
	{
		if(Door* door = getDoorByNumber(listId))
			return door->getAccessList(list);
		#ifdef __DEBUG_HOUSES__
		else
			std::cout << "Failure: [House::getAccessList] door == NULL, listId = " << listId <<std::endl;
		#endif
	}

	return false;
}

bool House::isInvited(const Player* player)
{
	return getHouseAccessLevel(player) != HOUSE_NO_INVITED;
}

void House::addDoor(Door* door)
{
	door->useThing2();
	doorList.push_back(door);
	door->setHouse(this);
	updateDoorDescription();
}

void House::removeDoor(Door* door)
{
	HouseDoorList::iterator it = std::find(doorList.begin(), doorList.end(), door);
	if(it != doorList.end())
	{
		(*it)->releaseThing2();
		doorList.erase(it);
	}
}

void House::addBed(BedItem* bed)
{
	bedsList.push_back(bed);
	bed->setHouse(this);
}

Door* House::getDoorByNumber(uint32_t doorId)
{
	for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it)
	{
		if((*it)->getDoorId() == doorId)
			return *it;
	}

	return NULL;
}

Door* House::getDoorByNumber(uint32_t doorId) const
{
	for(HouseDoorList::const_iterator it = doorList.begin(); it != doorList.end(); ++it)
	{
		if((*it)->getDoorId() == doorId)
			return *it;
	}

	return NULL;
}

Door* House::getDoorByPosition(const Position& pos)
{
	for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it)
	{
		if((*it)->getPosition() == pos)
			return *it;
	}

	return NULL;
}

bool House::canEditAccessList(uint32_t listId, const Player* player)
{
	switch(getHouseAccessLevel(player))
	{
		case HOUSE_OWNER:
			return true;
			break;
		case HOUSE_SUBOWNER:
			return listId == GUEST_LIST;
			break;
		default:
			return false;
			break;
	}
}

HouseTransferItem* House::getTransferItem()
{
	if(transferItem != NULL)
		return NULL;

	transferContainer.setParent(NULL);
	transferItem = HouseTransferItem::createHouseTransferItem(this);
	transferContainer.__addThing(NULL, transferItem);
	return transferItem;
}

void House::resetTransferItem()
{
	if(transferItem)
	{
		Item* tmpItem = transferItem;
		transferItem = NULL;
		transferContainer.setParent(NULL);
		transferContainer.__removeThing(tmpItem, tmpItem->getItemCount());
		g_game.FreeThing(tmpItem);
	}
}

HouseTransferItem* HouseTransferItem::createHouseTransferItem(House* house)
{
	HouseTransferItem* transferItem = new HouseTransferItem(house);
	transferItem->useThing2();
	transferItem->setID(ITEM_HOUSE_TRANSFER);
	transferItem->setSubType(1);

	char buffer[150];
	sprintf(buffer, "It is a house transfer document for '%s'.", house->getName().c_str());
	transferItem->setSpecialDescription(buffer);
	return transferItem;
}

bool HouseTransferItem::onTradeEvent(TradeEvents_t event, Player* owner)
{
	switch(event)
	{
		case ON_TRADE_TRANSFER:
		{
			if(House* house = getHouse())
				house->executeTransfer(this, owner);

			g_game.internalRemoveItem(NULL, this, 1);
			break;
		}
		case ON_TRADE_CANCEL:
		{
			if(House* house = getHouse())
				house->resetTransferItem();

			break;
		}
		default:
			break;
	}

	return true;
}

bool House::executeTransfer(HouseTransferItem* item, Player* newOwner)
{
	if(transferItem != item)
		return false;

	setHouseOwner(newOwner->getGUID());
	transferItem = NULL;
	return true;
}

AccessList::AccessList()
{
	//
}

AccessList::~AccessList()
{
	//
}

bool AccessList::parseList(const std::string& _list)
{
	playerList.clear();
	guildList.clear();
	expressionList.clear();
	regExList.clear();
	list = _list;
	if(_list == "")
		return true;

	std::stringstream listStream(_list);
	std::string line;
	while(getline(listStream, line))
	{
		trimString(line);
		trim_left(line, "\t");
		trim_right(line, "\t");
		trimString(line);

		std::transform(line.begin(), line.end(), line.begin(), tolower);
		if(line.substr(0, 1) == "#" || line.length() > 100)
			continue;

		if(line.find("@") != std::string::npos)
		{
			std::string::size_type pos = line.find("@");
			addGuild(line.substr(pos + 1), line.substr(0, pos));
		}
		else if(line.find("!") != std::string::npos || line.find("*") != std::string::npos || line.find("?") != std::string::npos)
			addExpression(line);
		else
			addPlayer(line);
	}

	return true;
}

bool AccessList::addPlayer(std::string& name)
{
	std::string tmp = name;

	uint32_t guid;
	if(IOLoginData::getInstance()->getGuidByName(guid, tmp))
	{
		if(playerList.find(guid) == playerList.end())
		{
			playerList.insert(guid);
			return true;
		}
	}

	return false;
}

bool AccessList::addGuild(const std::string& guildName, const std::string& rankName)
{
	uint32_t guildId;
	if(IOGuild::getInstance()->getGuildIdByName(guildId, guildName))
	{
		std::string tmp = rankName;

		int32_t rankId;
		if(!IOGuild::getInstance()->getRankIdByGuildIdAndName((uint32_t&)rankId, tmp, guildId) &&
			(tmp.find("?") == std::string::npos || tmp.find("!") == std::string::npos ||
			tmp.find("*") == std::string::npos))
		{
			rankId = -1;
		}

		if(rankId != 0)
		{
			for(GuildList::iterator git = guildList.begin(); git != guildList.end(); ++git)
			{
				if(git->first == guildId && git->second == rankId)
					return true;
			}

			guildList.push_back(std::make_pair(guildId, rankId));
			return true;
		}
	}

	return false;
}

bool AccessList::addExpression(const std::string& expression)
{
	for(ExpressionList::iterator it = expressionList.begin(); it != expressionList.end(); ++it)
	{
		if((*it) == expression)
			return false;
	}

	std::string outExp;
	std::string metachars = ".[{}()\\+|^$";
	for(std::string::const_iterator it = expression.begin(); it != expression.end(); ++it)
	{
		if(metachars.find(*it) != std::string::npos)
			outExp += "\\";

		outExp += (*it);
	}

	replaceString(outExp, "*", ".*");
	replaceString(outExp, "?", ".?");
	try
	{
		if(outExp.length() > 0)
		{
			expressionList.push_back(outExp);
			if(outExp.substr(0, 1) == "!")
			{
				if(outExp.length() > 1)
					regExList.push_front(std::make_pair(boost::regex(outExp.substr(1)), false));
			}
			else
				regExList.push_back(std::make_pair(boost::regex(outExp), true));
		}
	}
	catch(...){}
	return true;
}

bool AccessList::isInList(const Player* player)
{
	std::string name = player->getName();
	boost::cmatch what;

	std::transform(name.begin(), name.end(), name.begin(), tolower);
	for(RegExList::iterator it = regExList.begin(); it != regExList.end(); ++it)
	{
		if(boost::regex_match(name.c_str(), what, it->first))
			return it->second;
	}

	PlayerList::iterator playerIt = playerList.find(player->getGUID());
	if(playerIt != playerList.end())
		return true;

	for(GuildList::iterator git = guildList.begin(); git != guildList.end(); ++git)
	{
		if(git->first == player->getGuildId() && ((uint32_t)git->second == player->getGuildRankId() || git->second == -1))
			return true;
	}

	return false;
}

void AccessList::getList(std::string& _list) const
{
	_list = list;
}

Door::Door(uint16_t _type):
Item(_type)
{
	house = NULL;
	accessList = NULL;
}

Door::~Door()
{
	if(accessList)
		delete accessList;
}

bool Door::unserialize(xmlNodePtr nodeItem)
{
	bool ret = Item::unserialize(nodeItem);

	int32_t intValue;
	if(readXMLInteger(nodeItem, "doorId", intValue))
		setDoorId(intValue);

	return ret;
}

xmlNodePtr Door::serialize()
{
	xmlNodePtr xmlptr = xmlNewNode(NULL,(const xmlChar*)"item");
	char buffer[20];
	sprintf(buffer, "%d", getID());
	xmlSetProp(xmlptr, (const xmlChar*)"id", (const xmlChar*)buffer);
	return xmlptr;
}

bool Door::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if(ATTR_HOUSEDOORID == attr)
	{
		uint8_t _doorId = 0;
		if(!propStream.GET_UCHAR(_doorId))
			return false;

		setDoorId(_doorId);
		return true;
	}
	else
		return Item::readAttr(attr, propStream);
}

bool Door::serializeAttr(PropWriteStream& propWriteStream)
{
	return true;
}

void Door::setHouse(House* _house)
{
	if(house != NULL)
	{
		#ifdef __DEBUG_HOUSES__
		std::cout << "Warning: [Door::setHouse] house != NULL" << std::endl;
		#endif
		return;
	}
	house = _house;

	if(!accessList)
		accessList = new AccessList();
}

bool Door::canUse(const Player* player)
{
	if(!house)
		return true;

	if(house->getHouseAccessLevel(player) >= HOUSE_SUBOWNER)
		return true;

	return accessList->isInList(player);
}

void Door::setAccessList(const std::string& textlist)
{
	if(!accessList)
		accessList = new AccessList();

	accessList->parseList(textlist);
}

bool Door::getAccessList(std::string& list) const
{
	if(!house)
	{
		#ifdef __DEBUG_HOUSES__
		std::cout << "Failure: [Door::getAccessList] house == NULL" << std::endl;
		#endif
		return false;
	}

	accessList->getList(list);
	return true;
}

void Door::copyAttributes(Item* item)
{
	Item::copyAttributes(item);

	if(Door* door = item->getDoor())
	{
		std::string list;
		if(door->getAccessList(list))
			setAccessList(list);
	}
}

void Door::onRemoved()
{
	if(house)
		house->removeDoor(this);
}

Houses::Houses()
{
	rentPeriod = RENTPERIOD_NEVER;

	std::string strRentPeriod = asLowerCaseString(g_config.getString(ConfigManager::HOUSE_RENT_PERIOD));
	if(strRentPeriod == "yearly")
		rentPeriod = RENTPERIOD_YEARLY;
	else if(strRentPeriod == "monthly")
		rentPeriod = RENTPERIOD_MONTHLY;
	else if(strRentPeriod == "weekly")
		rentPeriod = RENTPERIOD_WEEKLY;
	else if(strRentPeriod == "daily")
		rentPeriod = RENTPERIOD_DAILY;
}

Houses::~Houses()
{
	//
}

bool Houses::loadHousesXML(std::string filename)
{
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc)
	{
		xmlNodePtr root, houseNode;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"houses") != 0)
		{
			xmlFreeDoc(doc);
			return false;
		}

		int32_t intValue;
		std::string strValue;

		houseNode = root->children;
		while(houseNode)
		{
			if(xmlStrcmp(houseNode->name,(const xmlChar*)"house") == 0)
			{
				int32_t _houseid = 0;
				Position entryPos(0, 0, 0);
				uint32_t _price = 0;

				if(!readXMLInteger(houseNode, "houseid", _houseid))
				{
					xmlFreeDoc(doc);
					return false;
				}

				House* house = Houses::getInstance().getHouse(_houseid);
				if(!house)
				{
					std::cout << "Error: [Houses::loadHousesXML] Unknown house, id = " << _houseid << std::endl;
					xmlFreeDoc(doc);
					return false;
				}

				if(readXMLString(houseNode, "name", strValue))
					house->setName(strValue);

				if(readXMLInteger(houseNode, "entryx", intValue))
					entryPos.x = intValue;

				if(readXMLInteger(houseNode, "entryy", intValue))
					entryPos.y = intValue;

				if(readXMLInteger(houseNode, "entryz", intValue))
					entryPos.z = intValue;

				house->setEntryPos(entryPos);
				if(entryPos.x == 0 || entryPos.y == 0)
					std::cout << "[Warning - Houses::loadHousesXML] House entry not set for: " << house->getName() << " (" << _houseid << ")" << std::endl;

				if(readXMLInteger(houseNode, "rent", intValue))
					house->setRent(intValue);

				if(readXMLInteger(houseNode, "townid", intValue))
					house->setTownId(intValue);

				if(readXMLInteger(houseNode, "size", intValue))
					house->setSize(intValue);

				if(g_config.getBool(ConfigManager::HOUSE_RENTASPRICE) && house->getRent())
					house->setPrice(house->getRent());
				else
				{
					for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); it++)
						_price += g_config.getNumber(ConfigManager::HOUSE_PRICE);

					if(g_config.getBool(ConfigManager::HOUSE_PRICEASRENT))
						house->setRent(_price);

					house->setPrice(_price);
				}

				house->setHouseOwner(0);
			}

			houseNode = houseNode->next;
		}

		xmlFreeDoc(doc);
		return true;
	}

	return false;
}

bool Houses::reloadPrices()
{
	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it)
	{
		if(g_config.getBool(ConfigManager::HOUSE_RENTASPRICE) && it->second->getRent())
			continue;

		uint32_t _price = 0;
		for(HouseTileList::iterator tit = it->second->getHouseTileBegin(); tit != it->second->getHouseTileEnd(); tit++)
			_price += g_config.getNumber(ConfigManager::HOUSE_PRICE);

		it->second->setPrice(_price, true);
	}

	return true;
}

bool Houses::payHouses()
{
	if(rentPeriod == RENTPERIOD_NEVER)
		return true;

	uint32_t currentTime = time(NULL);
	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it)
	{
		House* house = it->second;
		if(house->getHouseOwner() != 0 && house->getPaidUntil() < currentTime && house->getRent() != 0)
		{
			Town* town = Towns::getInstance().getTown(house->getTownId());
			if(!town)
			{
				#ifdef __DEBUG_HOUSES__
				std::cout << "[Warning - Houses::payHouses]: town = NULL, townid = " <<
					house->getTownId() << ", houseid = " << house->getHouseId() << std::endl;
				#endif
				continue;
			}

			std::string name;
			if(!IOLoginData::getInstance()->getNameByGuid(house->getHouseOwner(), name))
			{
				house->setHouseOwner(0);
				continue;
			}

			Player* player = g_game.getPlayerByName(name);
			if(!player)
			{
				player = new Player(name, NULL);
				if(!IOLoginData::getInstance()->loadPlayer(player, name))
				{
					#ifdef __DEBUG__
					std::cout << "[Failure - Houses::payHouses]: Cannot load player " << name << std::endl;
					#endif
					delete player;
					continue;
				}
			}

			bool savePlayerHere = true;
			if(Depot* depot = player->getDepot(town->getTownID(), true))
			{
				if(player->isPremium() || !g_config.getBool(ConfigManager::HOUSE_NEED_PREMIUM))
				{
					//get money from bank then from depot
					bool paid = false;
					if(g_config.getBool(ConfigManager::BANK_SYSTEM) && player->balance >= house->getRent())
					{
						player->balance -= house->getRent();
						paid = true;
					}
					else if(g_game.removeMoney(depot, house->getRent(), FLAG_NOLIMIT))
						paid = true;

					if(paid)
					{
						uint32_t paidUntil = currentTime;
						switch(rentPeriod)
						{
							case RENTPERIOD_DAILY:
								paidUntil += 24 * 60 * 60;
								break;
							case RENTPERIOD_WEEKLY:
								paidUntil += 24 * 60 * 60 * 7;
								break;
							case RENTPERIOD_MONTHLY:
								paidUntil += 24 * 60 * 60 * 30;
								break;
							case RENTPERIOD_YEARLY:
								paidUntil += 24 * 60 * 60 * 365;
								break;
							default:
								break;
						}

						house->setPaidUntil(paidUntil);
					}
					else if(currentTime >= house->getLastWarning() + 24 * 60 * 60)
					{
						if(house->getPayRentWarnings() >= 7)
						{
							house->setHouseOwner(0);
							savePlayerHere = false;
						}
						else
						{
							std::string period = "";
							switch(rentPeriod)
							{
								case RENTPERIOD_DAILY:
									period = "daily";
									break;
								case RENTPERIOD_WEEKLY:
									period = "weekly";
									break;
								case RENTPERIOD_MONTHLY:
									period = "monthly";
									break;
								case RENTPERIOD_YEARLY:
									period = "annual";
									break;
								default:
									break;
							}

							if(Item* letter = Item::CreateItem(ITEM_LETTER_STAMPED))
							{
								char warningText[200];
								sprintf(warningText, "Warning! \nThe %s rent of %d gold for your house \"%s\" is payable. Have it within %d days or you will lose this house.", period.c_str(), house->getRent(), house->getName().c_str(), (7 - house->getPayRentWarnings()));
								letter->setText(warningText);
								g_game.internalAddItem(NULL, depot, letter, INDEX_WHEREEVER, FLAG_NOLIMIT);
							}

							house->setPayRentWarnings(house->getPayRentWarnings() + 1);
							house->setLastWarning(currentTime);
						}
					}
				}
				else
				{
					house->setHouseOwner(0);
					savePlayerHere = false;
				}
			}

			if(player->isVirtual())
			{
				if(savePlayerHere)
					IOLoginData::getInstance()->savePlayer(player);

				delete player;
			}
		}
	}
	return true;
}

House* Houses::getHouse(uint32_t houseid, bool add/*= false*/)
{
	HouseMap::iterator it = houseMap.find(houseid);
	if(it != houseMap.end())
		return it->second;

	if(add)
	{
		House* house = new House(houseid);
		houseMap[houseid] = house;
		return house;
	}

	return NULL;
}

House* Houses::getHouseByPlayer(Player* player)
{
	if(player && !player->isRemoved())
	{
		if(HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile()))
		{
			if(House* house = houseTile->getHouse())
				return house;
		}
	}
	return NULL;
}

House* Houses::getHouseByPlayerId(uint32_t playerId)
{
	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it)
	{
		House* house = it->second;
		if(house->getHouseOwner() == playerId)
			return house;
	}

	return NULL;
}

uint32_t Houses::getHousesCount(uint32_t accId) const
{
	Account account = IOLoginData::getInstance()->loadAccount(accId);
	uint32_t guid, count = 0;
#ifdef __LOGIN_SERVER__
	for(CharactersMap::iterator it = account.charList.begin(); it != account.charList.end(); ++it)
	{
		if(IOLoginData::getInstance()->getGuidByName(guid, (std::string&)it->first) && getInstance().getHouseByPlayerId(guid))
#else
	for(StringVec::iterator it = account.charList.begin(); it != account.charList.end(); ++it)
	{
		if(IOLoginData::getInstance()->getGuidByName(guid, (*it)) && getInstance().getHouseByPlayerId(guid))
#endif
			count++;
	}
	return count;
}
