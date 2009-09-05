//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Quests
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

#ifndef _QUESTS_H_
#define _QUESTS_H_

#include <list>
#include <string>
#include "player.h"
#include "networkmessage.h"

class MissionState;
class Mission;
class Quest;

typedef std::map<int32_t, MissionState*> StateList;
typedef std::list<Mission*> MissionsList;
typedef std::list<Quest*> QuestsList;

class MissionState
{
	public:
		MissionState(std::string _description, int32_t _missionID);
		int32_t getMissionID() { return missionID; }
		std::string getMissionDescription() { return description; }

	private:
		std::string description;
		int32_t missionID;
};

class Mission
{
	public:
		Mission(std::string _missionName, int32_t _storageID, int32_t _startValue, int32_t _endValue);
		~Mission();
		bool isCompleted(Player* player) const;
		bool isStarted(Player* player) const;
		std::string getName(Player* player);
		std::string getDescription(Player* player);
		StateList state;

	private:
		std::string missionName;
		int32_t storageID, startValue, endValue;
};

class Quest
{
	public:
		Quest(std::string _name, uint16_t _id, int32_t _startStorageID, int32_t _startStorageValue);
		~Quest();

		bool isCompleted(Player* player);
		bool isStarted(Player* player) const;
		void getMissionList(Player* player, NetworkMessage_ptr msg);
		uint16_t getID() {return id;}
		std::string getName() {return name;}
		uint16_t getMissionsCount(Player* player);
		MissionsList missions;

	private:
		std::string name;
		uint16_t id;
		int32_t startStorageID, startStorageValue;
};

class Quests
{
	public:
		Quests();
		~Quests();

		static Quests* getInstance()
		{
			static Quests instance;
			return &instance;
		}

		bool loadFromXml();
		Quest* getQuestByID(uint16_t id);
		void getQuestsList(Player* player, NetworkMessage_ptr msg);
		uint16_t getQuestsCount(Player* player);
		QuestsList quests;
		bool reload();
};

#endif
