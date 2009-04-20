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
#ifndef __OTSERV_RAIDS_H__
#define __OTSERV_RAIDS_H__
#include "otsystem.h"
#include "const.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "position.h"
#include "baseevents.h"

enum RaidState_t
{
	RAIDSTATE_IDLE = 0,
	RAIDSTATE_EXECUTING
};

struct MonsterSpawn
{
	std::string name;
	uint32_t minAmount;
	uint32_t maxAmount;
};

class Raid;
class RaidEvent;

typedef std::list<Raid*> RaidList;
typedef std::vector<RaidEvent*> RaidEventVector;
typedef std::list<MonsterSpawn*> MonsterSpawnList;

#define MAX_RAND_RANGE 10000000
#define MAXIMUM_TRIES_PER_MONSTER 10
#define CHECK_RAIDS_INTERVAL 60
#define RAID_MINTICKS 1000

class Raids
{
	public:
		virtual ~Raids();
		static Raids* getInstance()
		{
			static Raids instance;
			return &instance;
		}

		bool loadFromXml();
		bool parseRaidNode(xmlNodePtr raidNode);
		bool startup();

		void clear();
		bool reload();

		Raid* getRunning() const {return running;}
		void setRunning(Raid* newRunning) {running = newRunning;}

		uint64_t getLastRaidEnd() const {return lastRaidEnd;}
		void setLastRaidEnd(uint64_t newLastRaidEnd) {lastRaidEnd = newLastRaidEnd;}

		bool isLoaded() const {return loaded;}
		bool isStarted() const {return started;}

		std::string getScriptBaseName() const {return "raids";}
		Raid* getRaidByName(const std::string& name);
		void checkRaids();

	private:
		Raids();
		bool loaded, started;

		RaidList raidList;
		Raid* running;

		uint32_t checkRaidsEvent;
		uint64_t lastRaidEnd;
};

class Raid
{
	public:
		Raid(const std::string& _name, uint32_t _interval, uint64_t _margin, bool _enabled);
		virtual ~Raid();

		bool loadFromXml(const std::string& _filename);

		void startRaid();
		void resetRaid();

		void executeRaidEvent(RaidEvent* raidEvent);
		RaidEvent* getNextRaidEvent();
		void stopEvents();

		std::string getName() const {return name;}

		uint64_t getMargin() const {return margin;}
		uint32_t getInterval() const {return interval;}

		bool isLoaded() const {return loaded;}
		bool isEnabled() const {return enabled;}

		void setState(RaidState_t newState) {state = newState;}

	private:
		bool loaded;
		RaidState_t state;
		RaidEventVector raidEvents;

		uint32_t nextEvent;
		uint32_t nextEventEvent;

		std::string name;
		uint32_t interval;
		uint64_t margin;
		bool enabled;
};

class RaidEvent
{
	public:
		RaidEvent() {}
		virtual ~RaidEvent() {}

		virtual bool configureRaidEvent(xmlNodePtr eventNode);
		virtual bool executeEvent() const {return false;}

		uint32_t getDelay() const {return m_delay;}
		void setDelay(uint32_t newDelay) {m_delay = newDelay;}

		static bool compareEvents(const RaidEvent* lhs, const RaidEvent* rhs)
		{
			return lhs->getDelay() < rhs->getDelay();
		}

	private:
		uint32_t m_delay;
};

class AnnounceEvent : public RaidEvent
{
	public:
		AnnounceEvent() {}
		virtual ~AnnounceEvent() {}

		virtual bool configureRaidEvent(xmlNodePtr eventNode);
		virtual bool executeEvent() const;

	private:
		std::string m_message;
		MessageClasses m_messageType;
};

class SingleSpawnEvent : public RaidEvent
{
	public:
		SingleSpawnEvent() {}
		virtual ~SingleSpawnEvent() {}

		virtual bool configureRaidEvent(xmlNodePtr eventNode);
		virtual bool executeEvent() const;

	private:
		std::string m_monsterName;
		Position m_position;
};

class AreaSpawnEvent : public RaidEvent
{
	public:
		AreaSpawnEvent() {}
		virtual ~AreaSpawnEvent();

		virtual bool configureRaidEvent(xmlNodePtr eventNode);
		virtual bool executeEvent() const;

		void addMonster(MonsterSpawn* monsterSpawn);
		void addMonster(const std::string& monsterName, uint32_t minAmount, uint32_t maxAmount);

	private:
		MonsterSpawnList m_spawnList;
		Position m_fromPos, m_toPos;
};

class ScriptEvent : public RaidEvent, public Event
{
	public:
		ScriptEvent();
		virtual ~ScriptEvent() {}

		virtual bool configureRaidEvent(xmlNodePtr eventNode);
		virtual bool executeEvent() const;

		virtual bool configureEvent(xmlNodePtr p) {return false;}

	protected:
		virtual std::string getScriptEventName() const {return "onRaid";}
		virtual std::string getScriptEventParams() const {return "";}

		static LuaScriptInterface m_scriptInterface;
};

#endif
