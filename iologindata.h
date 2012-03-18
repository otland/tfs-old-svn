//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the LoginData loading/saving
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

#ifndef __IOLOGINDATA_H
#define __IOLOGINDATA_H

#include <string>
#include "account.h"
#include "player.h"
#include "database.h"

struct PlayerGroup
{
	std::string m_name;
	uint64_t m_flags;
	uint32_t m_access;
	uint32_t m_maxdepotitems;
	uint32_t m_maxviplist;
};

typedef std::pair<int32_t, Item*> itemBlock;
typedef std::list<itemBlock> ItemBlockList;

class IOLoginData
{
	public:
		IOLoginData() {}
		~IOLoginData() {}

		static IOLoginData* getInstance()
		{
			static IOLoginData instance;
			return &instance;
		}

		Account loadAccount(uint32_t accno);
		Account loadAccount(std::string name);
		bool saveAccount(Account acc);
		bool createAccount(const std::string& accountNumber, std::string newPassword);
		bool getPassword(const std::string& accname, const std::string& name, std::string& password, uint32_t& accNumber);
		bool getPasswordEx(const std::string& accname, std::string& password, uint32_t& accNumber);
		bool accountNameExists(const std::string& name);
		bool setRecoveryKey(uint32_t accountNumber, std::string recoveryKey);
		bool validRecoveryKey(const std::string& accountName, const std::string& recoveryKey);
		bool setNewPassword(uint32_t accountId, std::string newPassword);
		bool setNewPassword(const std::string& accountName, std::string newPassword);
		AccountType_t getAccountType(std::string name);

		bool updateOnlineStatus(uint32_t guid, bool login);
		bool resetOnlineStatus();

		bool loadPlayer(Player* player, const std::string& name, bool preload = false);
		bool savePlayer(Player* player, bool preSave);
		bool getGuidByName(uint32_t& guid, std::string& name);
		bool getGuidByNameEx(uint32_t &guid, bool& specialVip, std::string& name);
		bool getNameByGuid(uint32_t guid, std::string& name);
		bool playerExists(std::string& name);
		bool playerExists(uint32_t guid);
		int32_t getLevel(uint32_t guid);
		bool isPremium(uint32_t guid);
		bool leaveGuild(uint32_t guid);
		bool changeName(uint32_t guid, std::string newName);
		uint32_t getAccountNumberByName(std::string name);
		bool createCharacter(uint32_t accountNumber, std::string characterName, int32_t vocationId, PlayerSex_t sex);
		int16_t deleteCharacter(uint32_t accountNumber, const std::string& characterName);
		bool addStorageValue(uint32_t guid, uint32_t storageKey, uint32_t storageValue);
		const PlayerGroup* getPlayerGroup(uint32_t groupid);
		uint32_t getLastIPByName(std::string name);
		bool hasGuild(uint32_t guid);
		void increaseBankBalance(uint32_t guid, uint64_t bankBalance);

	protected:
		bool storeNameByGuid(Database &mysql, uint32_t guid);

		const PlayerGroup* getPlayerGroupByAccount(uint32_t accno);
		struct StringCompareCase
		{
			bool operator()(const std::string& l, const std::string& r) const
			{
				return strcasecmp(l.c_str(), r.c_str()) < 0;
			}
		};

		typedef std::map<int32_t ,std::pair<Item*, int32_t> > ItemMap;

		void loadItems(ItemMap& itemMap, DBResult* result);
		bool saveItems(const Player* player, const ItemBlockList& itemList, DBInsert& query_insert);

		typedef std::map<uint32_t, std::string> NameCacheMap;
		typedef std::map<std::string, uint32_t, StringCompareCase> GuidCacheMap;
		typedef std::map<uint32_t, PlayerGroup*> PlayerGroupMap;

		PlayerGroupMap playerGroupMap;
		NameCacheMap nameCacheMap;
		GuidCacheMap guidCacheMap;
};

#endif
