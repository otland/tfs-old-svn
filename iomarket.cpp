//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// IOMarket
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

#include "iomarket.h"
#include "iologindata.h"
#include "configmanager.h"

extern ConfigManager g_config;

MarketItemList IOMarket::getActiveOffers(MarketAction_t action, uint16_t itemId)
{
	MarketItemList itemList;

	DBQuery query;
	query << "SELECT `id`, `player_id`, `amount`, `price`, `created`, `anonymous` FROM `market_offers` WHERE `sale` = " << action << " AND `itemtype` = " << itemId << ";";

	Database* db = Database::getInstance();
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return itemList;
	
	const int32_t marketOfferDuration = g_config.getNumber(ConfigManager::MARKET_OFFER_DURATION);
	do
	{
		MarketItem item;
		item.amount = result->getDataInt("amount");
		item.price = result->getDataInt("price");
		item.timestamp = result->getDataInt("created") + marketOfferDuration;
		item.counter = result->getDataInt("id") & 0xFFFF;
		if(result->getDataInt("anonymous") == 0)
		{
			IOLoginData::getInstance()->getNameByGuid(result->getDataInt("player_id"), item.playerName);
			if(item.playerName.empty())
				item.playerName = "Anonymous";
		}
		else
			item.playerName = "Anonymous";

		itemList.push_back(item);
	}
	while(result->next());
	db->freeResult(result);
	return itemList;
}

MarketItemList IOMarket::getOwnOffers(MarketAction_t action, uint32_t playerId)
{
	MarketItemList itemList;

	const int32_t marketOfferDuration = g_config.getNumber(ConfigManager::MARKET_OFFER_DURATION);

	DBQuery query;
	query << "SELECT `id`, `amount`, `price`, `created`, `anonymous`, `itemtype` FROM `market_offers` WHERE `player_id` = " << playerId << " AND `sale` = " << action << " AND `created` > " << (time(NULL) - marketOfferDuration) << ";";

	Database* db = Database::getInstance();
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return itemList;
	
	do
	{
		MarketItem item;
		item.amount = result->getDataInt("amount");
		item.price = result->getDataInt("price");
		item.timestamp = result->getDataInt("created") + marketOfferDuration;
		item.counter = result->getDataInt("id") & 0xFFFF;
		item.itemId = result->getDataInt("itemtype");

		itemList.push_back(item);
	}
	while(result->next());
	db->freeResult(result);
	return itemList;
}

MarketItemList IOMarket::getOwnHistory(MarketAction_t action, uint32_t playerId)
{
	MarketItemList itemList;
	/*
	TODO: expired field or history table?

	const int32_t marketOfferDuration = g_config.getNumber(ConfigManager::MARKET_OFFER_DURATION);

	DBQuery query;
	query << "SELECT `id`, `amount`, `price`, `created`, `anonymous`, `itemtype` FROM `market_offers` WHERE `player_id` = " << playerId << " AND `sale` = " << action << " AND `expired` = 1;";

	Database* db = Database::getInstance();
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return itemList;
	
	do
	{
		MarketItem item;
		item.amount = result->getDataInt("amount");
		item.price = result->getDataInt("price");
		item.timestamp = result->getDataInt("created") + marketOfferDuration;
		item.counter = result->getDataInt("id") & 0xFFFF;
		item.itemId = result->getDataInt("itemtype");

		itemList.push_back(item);
	}
	while(result->next());
	db->freeResult(result);
	*/
	return itemList;
}

ExpiredMarketItemList IOMarket::getExpiredOffers(MarketAction_t action)
{
	ExpiredMarketItemList itemList;

	const int32_t marketOfferDuration = g_config.getNumber(ConfigManager::MARKET_OFFER_DURATION);

	DBQuery query;
	query << "SELECT `id`, `amount`, `price`, `itemtype`, `player_id` FROM `market_offers` WHERE `sale` = " << action << " AND `created` <= " << (time(NULL) - marketOfferDuration) << ";";

	Database* db = Database::getInstance();
	DBResult* result;
	if(!(result = db->storeQuery(query.str())))
		return itemList;
	
	do
	{
		ExpiredMarketItem item;
		item.id = result->getDataInt("id");
		item.amount = result->getDataInt("amount");
		item.price = result->getDataInt("price");
		item.itemId = result->getDataInt("itemtype");
		item.playerId = result->getDataInt("player_id");

		itemList.push_back(item);
	}
	while(result->next());
	db->freeResult(result);
	return itemList;
}

bool IOMarket::deleteOfferById(uint32_t id)
{
	DBQuery query;
	query << "DELETE FROM `market_offers` WHERE `id` = " << id << ";";
	return Database::getInstance()->executeQuery(query.str());
}

MarketItemEx IOMarket::getOfferById(uint32_t id)
{
	MarketItemEx marketItem;
	DBQuery query;
	query << "SELECT `id`, `sale`, `itemtype`, `amount`, `created`, `price`, `player_id` FROM `market_offers` WHERE `id` = " << id << ";";
	Database* db = Database::getInstance();
	DBResult* result;
	if((result = db->storeQuery(query.str())))
	{
		marketItem.type = (MarketAction_t)result->getDataInt("sale");
		marketItem.amount = result->getDataInt("amount");
		marketItem.counter = result->getDataInt("id") & 0xFFFF;
		marketItem.timestamp = result->getDataInt("created");
		marketItem.price = result->getDataInt("price");
		marketItem.playerId = result->getDataInt("player_id");
		marketItem.itemId = result->getDataInt("itemtype");
		db->freeResult(result);
	}
	return marketItem;
}

uint32_t IOMarket::getOfferIdByCounter(uint32_t timestamp, uint16_t counter)
{
	const int32_t created = timestamp - g_config.getNumber(ConfigManager::MARKET_OFFER_DURATION);

	DBQuery query;
	query << "SELECT `id` FROM `market_offers` WHERE `created` = " << created << " AND (`id` & 65535) = " << counter << " LIMIT 1;";
	Database* db = Database::getInstance();
	DBResult* result;
	if((result = db->storeQuery(query.str())))
	{
		uint32_t offerId = result->getDataInt("id");
		db->freeResult(result);
		return offerId;
	}
	return 0;
}

void IOMarket::createOffer(uint32_t playerId, MarketAction_t action, uint32_t itemId, uint16_t amount, uint32_t price, bool anonymous)
{
	DBQuery query;
	query << "INSERT INTO `market_offers` (`player_id`, `sale`, `itemtype`, `amount`, `price`, `created`, `anonymous`) VALUES (" << playerId << ", " << action << ", " << itemId << ", " << amount << ", " << price << ", " << time(NULL) << ", " << anonymous << ");";
	Database::getInstance()->executeQuery(query.str());
}

void IOMarket::acceptOffer(uint32_t offerId, uint16_t amount)
{
	DBQuery query;
	query << "UPDATE `market_offers` SET `amount` = `amount` - " << amount << " WHERE `id` = " << offerId << ";";
	Database::getInstance()->executeQuery(query.str());
}
