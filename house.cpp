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

#include "house.h"
#include "tools.h"

#include "beds.h"
#include "town.h"
#include "iologindata.h"

#include "configmanager.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;

House::House(uint32_t _houseid)
{
	isLoaded = false;
	houseName = "Forgotten headquarter (Flat 1, Area 42)";
	houseOwner = 0;
	posEntry = Position();
	paidUntil = 0;
	houseid = _houseid;
	rentWarnings = 0;
	lastWarning = 0;
	rent = 0;
	price = 0;
	townid = 0;
}

void House::addTile(HouseTile* tile)
{
	tile->setFlag(TILESTATE_PROTECTIONZONE);
	houseTiles.push_back(tile);
}

void House::addBed(BedItem* bed)
{
	bedsList.push_back(bed);
	bed->setHouse(this);
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

Door* House::getDoorByNumber(uint32_t doorId) const
{
	for(HouseDoorList::const_iterator it = doorList.begin(); it != doorList.end(); ++it)
	{
		if((*it)->getDoorId() == doorId)
			return (*it);
	}

	return NULL;
}

Door* House::getDoorByPosition(const Position& pos)
{
	for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it)
	{
		if((*it)->getPosition() == pos)
			return (*it);
	}

	return NULL;
}

void House::setPrice(uint32_t _price, bool update /*= false*/)
{
	price = _price;
	if(update && !getHouseOwner())
		updateDoorDescription();
}

void House::setHouseOwner(uint32_t guid, bool _clean/* = true*/)
{
	if(isLoaded && houseOwner == guid)
		return;

	isLoaded = true;
	if(houseOwner)
	{
		if(_clean)
			clean();

		setAccessList(SUBOWNER_LIST, "", !_clean);
		setAccessList(GUEST_LIST, "", !_clean);
		for(HouseDoorList::iterator it = doorList.begin(); it != doorList.end(); ++it)
			(*it)->setAccessList("");

		houseOwner = rentWarnings = paidUntil = 0;
	}

	if(guid)
		houseOwner = guid;

	updateDoorDescription();
	setLastWarning(time(NULL)); //so the new owner has one day before he start the payement
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
		if(!(*it)->creatures || (*it)->creatures->empty())
			continue;

		for(CreatureVector::iterator cit = (*it)->creatures->begin(); cit != (*it)->creatures->end(); ++cit)
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

bool House::transferToDepot()
{
	if(townid == 0)
		return false;

	Player* player = NULL;
	if(houseOwner)
	{
		std::string ownerName;
		if(IOLoginData::getInstance()->getNameByGuid(houseOwner, ownerName))
			player = g_game.getPlayerByName(ownerName);

		if(!player)
		{
			player = new Player(ownerName, NULL);
			if(!IOLoginData::getInstance()->loadPlayer(player, ownerName))
				delete player;
		}

		player->useThing2();
	}

	Item* item = NULL;
	Container* tmpContainer = NULL;

	ItemList moveItemList;
	for(HouseTileList::iterator it = houseTiles.begin(); it != houseTiles.end(); ++it)
	{
		for(uint32_t i = 0; i < (*it)->getThingCount(); ++i)
		{
			if(!(item = (*it)->__getThing(i)->getItem()))
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

	if(player)
	{
		Depot* depot = player->getDepot(townid, true);
		for(ItemList::iterator it = moveItemList.begin(); it != moveItemList.end(); ++it)
			g_game.internalMoveItem(NULL, (*it)->getParent(), depot, INDEX_WHEREEVER, (*it), (*it)->getItemCount(), NULL, FLAG_NOLIMIT);

		if(player->isVirtual())
			IOLoginData::getInstance()->savePlayer(player);

		g_game.FreeThing(player);
		return true;
	}

	for(ItemList::iterator it = moveItemList.begin(); it != moveItemList.end(); ++it)
		g_game.internalRemoveItem(NULL, (*it), (*it)->getItemCount(), false, FLAG_NOLIMIT);

	return true;
}

bool House::isInvited(const Player* player)
{
	return getHouseAccessLevel(player) != HOUSE_NO_INVITED;
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

bool House::canEditAccessList(uint32_t listId, const Player* player)
{
	switch(getHouseAccessLevel(player))
	{
		case HOUSE_OWNER:
			return true;
		case HOUSE_SUBOWNER:
			return listId == GUEST_LIST;
		default:
			break;
	}

	return false;
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
			std::cout << "[Failure - House::setAccessList] door == NULL, listId = " << listId <<std::endl;
		#endif

		return;
	}

	if(teleport)
		removePlayers(false);
}

HouseTransferItem* HouseTransferItem::createHouseTransferItem(House* house)
{
	HouseTransferItem* transferItem = new HouseTransferItem(house);
	transferItem->useThing2();
	transferItem->setID(ITEM_HOUSE_TRANSFER);

	char buffer[150];
	sprintf(buffer, "It is a house transfer document for '%s'.", house->getName().c_str());
	transferItem->setSpecialDescription(buffer);

	transferItem->setSubType(1);
	return transferItem;
}

bool HouseTransferItem::onTradeEvent(TradeEvents_t event, Player* owner, Player* seller)
{
	switch(event)
	{
		case ON_TRADE_TRANSFER:
		{
			if(house)
				house->setHouseOwner(owner->getGUID());

			g_game.internalRemoveItem(NULL, this, 1);
			seller->transferContainer.setParent(NULL);
			break;
		}

		case ON_TRADE_CANCEL:
		{
			owner->transferContainer.setParent(NULL);
			owner->transferContainer.__removeThing(this, getItemCount());
			g_game.FreeThing(this);
			break;
		}

		default:
			return false;
	}

	return true;
}

void AccessList::getList(std::string& _list) const
{
	_list = list;
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
	catch(...) {}
	return true;
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

	return Item::readAttr(attr, propStream);
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

bool Door::canUse(const Player* player)
{
	if(!house)
		return true;

	if(house->getHouseAccessLevel(player) >= HOUSE_SUBOWNER)
		return true;

	return accessList->isInList(player);
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

void Door::setAccessList(const std::string& textlist)
{
	if(!accessList)
		accessList = new AccessList();

	accessList->parseList(textlist);
}

Houses::Houses()
{
	rentPeriod = RENTPERIOD_NEVER;
	std::string strValue = asLowerCaseString(g_config.getString(ConfigManager::HOUSE_RENT_PERIOD));
	if(strValue == "yearly")
		rentPeriod = RENTPERIOD_YEARLY;
	else if(strValue == "monthly")
		rentPeriod = RENTPERIOD_MONTHLY;
	else if(strValue == "weekly")
		rentPeriod = RENTPERIOD_WEEKLY;
	else if(strValue == "daily")
		rentPeriod = RENTPERIOD_DAILY;
}

bool Houses::loadFromXml(std::string filename)
{
	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(!doc)
	{
		std::cout << "[Warning - Houses::loadFromXml] Cannot load houses file." << std::endl;
		std::cout << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr houseNode, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"houses"))
	{
		std::cout << "[Error - Houses::loadFromXml] Malformed houses file." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	int32_t intValue;
	std::string strValue;

	houseNode = root->children;
	while(houseNode)
	{
		if(xmlStrcmp(houseNode->name,(const xmlChar*)"house"))
		{
			houseNode = houseNode->next;
			continue;
		}

		int32_t houseId = 0;
		if(!readXMLInteger(houseNode, "houseid", houseId))
		{
			std::cout << "[Error - Houses::loadFromXml] Could not read houseId" << std::endl;
			xmlFreeDoc(doc);
			return false;
		}

		House* house = Houses::getInstance().getHouse(houseId);
		if(!house)
		{
			std::cout << "[Error - Houses::loadFromXml] Unknown house with id: " << houseId << std::endl;
			xmlFreeDoc(doc);
			return false;
		}

		if(readXMLString(houseNode, "name", strValue))
			house->setName(strValue);

		Position entryPos(0, 0, 0);
		if(readXMLInteger(houseNode, "entryx", intValue))
			entryPos.x = intValue;

		if(readXMLInteger(houseNode, "entryy", intValue))
			entryPos.y = intValue;

		if(readXMLInteger(houseNode, "entryz", intValue))
			entryPos.z = intValue;

		house->setEntryPos(entryPos);
		if(entryPos.x == 0 || entryPos.y == 0)
		{
			std::cout << "[Warning - Houses::loadFromXml] House entry not set for: ";
			std::cout << house->getName() << " (" << houseId << ")" << std::endl;
		}

		if(readXMLInteger(houseNode, "townid", intValue))
			house->setTownId(intValue);

		if(readXMLInteger(houseNode, "size", intValue))
			house->setSize(intValue);

		uint32_t rent = 0;
		if(readXMLInteger(houseNode, "rent", intValue))
			rent = intValue;

		uint32_t price = 0;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); it++)
			price += g_config.getNumber(ConfigManager::HOUSE_PRICE);

		if(g_config.getBool(ConfigManager::HOUSE_RENTASPRICE))
		{
			uint32_t tmp = rent;
			if(!tmp)
				tmp = price;

			house->setPrice(tmp);
		}
		else
			house->setPrice(price);

		if(g_config.getBool(ConfigManager::HOUSE_PRICEASRENT))
			house->setRent(price);
		else
			house->setRent(rent);

		house->setHouseOwner(0);
		houseNode = houseNode->next;
	}

	xmlFreeDoc(doc);
	return true;
}

bool Houses::reloadPrices()
{
	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it)
	{
		if(g_config.getBool(ConfigManager::HOUSE_RENTASPRICE))
			continue;

		uint32_t price = 0;
		for(HouseTileList::iterator tit = it->second->getHouseTileBegin(); tit != it->second->getHouseTileEnd(); tit++)
			price += g_config.getNumber(ConfigManager::HOUSE_PRICE);

		it->second->setPrice(price, true);
	}

	return true;
}

void Houses::payHouses()
{
	if(rentPeriod == RENTPERIOD_NEVER)
		return;

	uint64_t start = OTSYS_TIME();
	std::cout << "> Paying houses..." << std::endl;

	time_t currentTime = time(NULL);
	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it)
		payHouse(it->second, currentTime);

	std::cout << "> Houses paid in " << (OTSYS_TIME() - start) / (1000.) << " seconds." << std::endl;
}

bool Houses::payRent(Player* player, House* house, time_t _time/* = 0*/)
{
	if(!_time)
		_time = time(NULL);

	if(rentPeriod == RENTPERIOD_NEVER || !house->getHouseOwner() ||
                house->getPaidUntil() >= _time || !house->getRent())
                return true;

	Town* town = Towns::getInstance().getTown(house->getTownId());
	if(!town)
		return false;

	bool paid = false;
	if(Depot* depot = player->getDepot(town->getTownID(), true))
	{
		if(g_config.getBool(ConfigManager::BANK_SYSTEM) && player->balance >= house->getRent())
		{
			player->balance -= house->getRent();
			paid = true;
		}
		else
			paid = g_game.removeMoney(depot, house->getRent(), FLAG_NOLIMIT);
	}

	if(!paid)
		return false;

	uint32_t paidUntil = _time;
	switch(rentPeriod)
	{
		case RENTPERIOD_DAILY:
			paidUntil += 86400;
			break;
		case RENTPERIOD_WEEKLY:
			paidUntil += 7 * 86400;
			break;
		case RENTPERIOD_MONTHLY:
			paidUntil += 30 * 86400;
			break;
		case RENTPERIOD_YEARLY:
			paidUntil += 365 * 86400;
			break;
		default:
			break;
	}

	house->setPaidUntil(paidUntil);
	return true;
}

bool Houses::payHouse(House* house, time_t _time)
{
	if(rentPeriod == RENTPERIOD_NEVER || !house->getHouseOwner() ||
		house->getPaidUntil() >= _time || !house->getRent())
		return true;

	Town* town = Towns::getInstance().getTown(house->getTownId());
	if(!town)
		return false;

	std::string name;
	if(!IOLoginData::getInstance()->getNameByGuid(house->getHouseOwner(), name))
	{
		house->setHouseOwner(0);
		return false;
	}

	Player* player = g_game.getPlayerByName(name);
	if(!player)
	{
		player = new Player(name, NULL);
		if(!IOLoginData::getInstance()->loadPlayer(player, name))
		{
			#ifdef __DEBUG_HOUSES__
			std::cout << "[Failure - Houses::payHouse] Cannot load player: " << name << std::endl;
			#endif
			delete player;
			return false;
		}
	}

	bool paid = payRent(player, house, _time), savePlayer = false;
	if(!paid && _time >= (house->getLastWarning() + 86400))
	{
		uint32_t warningsLimit = 7;
		switch(rentPeriod)
		{
			case RENTPERIOD_DAILY:
				warningsLimit = 1;
				break;
			case RENTPERIOD_WEEKLY:
				warningsLimit = 3;
				break;
			case RENTPERIOD_MONTHLY:
				warningsLimit = 7;
				break;
			case RENTPERIOD_YEARLY:
				warningsLimit = 14;
				break;
			default:
				break;
		}

		if(house->getPayRentWarnings() < warningsLimit)
		{
			Depot* depot = player->getDepot(town->getTownID(), true);
			Item* letter = Item::CreateItem(ITEM_LETTER_STAMPED);

			uint32_t warnings = house->getPayRentWarnings();
			if(depot && letter)
			{
				std::string period;
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

				std::stringstream s;
				s << "Warning!\nThe " << period << " rent of " << house->getRent() << " gold for your house \"" << house->getName();
				s << "\" has to be paid. Have it within " << (warningsLimit - warnings) << " days or you will lose your house.";

				letter->setText(s.str().c_str());
				if(g_game.internalAddItem(NULL, depot, letter, INDEX_WHEREEVER, FLAG_NOLIMIT) != RET_NOERROR)
					g_game.FreeThing(letter);
				else
					savePlayer = true;
			}

			house->setPayRentWarnings(warnings++);
			house->setLastWarning(_time);
		}
		else
			house->setHouseOwner(0);
	}

	if(player->isVirtual())
	{
		if(savePlayer)
			IOLoginData::getInstance()->savePlayer(player);

		delete player;
		player = NULL;
	}

	return paid;
}

House* Houses::getHouse(uint32_t houseId, bool add/*= false*/)
{
	HouseMap::iterator it = houseMap.find(houseId);
	if(it != houseMap.end())
		return it->second;

	if(add)
	{
		House* house = new House(houseId);
		houseMap[houseId] = house;
		return house;
	}

	return NULL;
}

House* Houses::getHouseByPlayer(Player* player)
{
	if(!player || player->isRemoved())
		return NULL;

	HouseTile* houseTile = dynamic_cast<HouseTile*>(player->getTile());
	if(!houseTile)
		return NULL;

	House* house = houseTile->getHouse();
	if(!house)
		return NULL;

	return house;
}

House* Houses::getHouseByPlayerId(uint32_t playerId)
{
	for(HouseMap::iterator it = houseMap.begin(); it != houseMap.end(); ++it)
	{
		if(it->second->getHouseOwner() == playerId)
			return it->second;
	}

	return NULL;
}

uint32_t Houses::getHousesCount(uint32_t accId)
{
	Account account = IOLoginData::getInstance()->loadAccount(accId);
	uint32_t guid, count = 0;
	for(Characters::iterator it = account.charList.begin(); it != account.charList.end(); ++it)
	{
#ifndef __LOGIN_SERVER__
		if(IOLoginData::getInstance()->getGuidByName(guid, (*it)) && getHouseByPlayerId(guid))
#else
		if(IOLoginData::getInstance()->getGuidByName(guid, (std::string&)it->first) && getHouseByPlayerId(guid))
#endif
			count++;
	}

	return count;
}
