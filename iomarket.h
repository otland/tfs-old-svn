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

#ifndef __OTSERV_IOMARKET_H__
#define __OTSERV_IOMARKET_H__

#include <string>
#include "account.h"
#include "player.h"
#include "database.h"

class IOMarket
{
	public:
		IOMarket() {}
		~IOMarket() {}

		static IOMarket* getInstance()
		{
			static IOMarket instance;
			return &instance;
		}

		void loadMarket();
		MarketItemList getActiveOffers(MarketAction_t action, uint32_t itemId);
		uint32_t getOfferIdByCounter(uint32_t timestamp, uint16_t counter);
		MarketItemEx getOfferById(uint32_t id);
		void createOffer(uint32_t playerId, MarketAction_t action, uint32_t itemId, uint16_t amount, uint32_t price, bool anonymous);
		void cancelOffer(uint32_t offerId);
		void acceptOffer(uint32_t offerId, uint16_t amount);
};

#endif
