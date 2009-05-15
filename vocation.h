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

#ifndef __OTSERV_VOCATION_H__
#define __OTSERV_VOCATION_H__
#include "otsystem.h"
#include "enums.h"

enum multiplier_t
{
	MULTIPLIER_FIRST = 0,
	MULTIPLIER_MELEE = MULTIPLIER_FIRST,
	MULTIPLIER_DISTANCE = 1,
	MULTIPLIER_DEFENSE = 2,
	MULTIPLIER_ARMOR = 3,
	MULTIPLIER_MAGIC = 4,
	MULTIPLIER_MAGICHEALING = 5,
	MULTIPLIER_WAND = 6,
	MULTIPLIER_MANA = 7,
	MULTIPLIER_LAST = MULTIPLIER_MANA
};

enum gain_t
{
	GAIN_FIRST = 0,
	GAIN_HEALTH = GAIN_FIRST,
	GAIN_MANA = 1,
	GAIN_LAST = GAIN_MANA
};

class Vocation
{
	public:
		virtual ~Vocation();

		Vocation() {reset();}
		Vocation(uint32_t _id): id(_id) {reset();}

		void reset();

		uint32_t getId() const {return id;}
		void setId(int32_t v) {id = v;}

		const std::string& getName() const {return name;}
		void setName(const std::string& v) {name = v;}

		const std::string& getDescription() const {return description;}
		void setDescription(const std::string& v) {description = v;}

		uint32_t getGain(gain_t type) const {return gain[type];}
		void setGain(gain_t type, uint32_t v) {gain[type] = v;}

		uint32_t getGainTicks(gain_t type) const {return gainTicks[type];}
		void setGainTicks(gain_t type, uint32_t v) {gainTicks[type] = v;}

		uint32_t getGainAmount(gain_t type) const {return gainAmount[type];}
		void setGainAmount(gain_t type, uint32_t v) {gainAmount[type] = v;}

		uint16_t getSoulGainTicks() const {return soulGainTicks;}
		void setSoulGainTicks(uint16_t v) {soulGainTicks = v;}

		uint16_t getSoulMax() const {return std::min((uint32_t)soulMax, (uint32_t)255);}
		void setSoulMax(uint16_t v) {soulMax = std::min((uint32_t)v, (uint32_t)255);}

		uint32_t getCapGain() const {return gainCap;}
		void setGainCap(uint32_t v) {gainCap = v;}

		uint32_t getBaseSpeed() const {return baseSpeed;}
		void setBaseSpeed(uint32_t v) {baseSpeed = v;}

		uint32_t getAttackSpeed() const {return attackSpeed;}
		void setAttackSpeed(uint32_t v) {attackSpeed = v;}

		uint32_t getFromVocation() const {return fromVocation;}
		void setFromVocation(int32_t v) {fromVocation = v;}

		int32_t getLessLoss() const {return lessLoss;}
		void setLessLoss(int32_t v) {lessLoss = v;}

		bool isAttackable() const {return attackable;}
		void setAttackable(bool v) {attackable = v;}

		bool isPremiumNeeded() const {return needPremium;}
		void setNeedPremium(bool v) {needPremium = v;}

		int16_t getAbsorbPercent(CombatType_t combat) const {return absorbPercent[combat];}
		void increaseAbsorbPercent(CombatType_t combat, int16_t v) {absorbPercent[combat] += v;}

		float getMultiplier(multiplier_t type) const {return formulaMultipliers[type];}
		void setMultiplier(multiplier_t type, float v) {formulaMultipliers[type] = v;}

		uint32_t getReqSkillTries(int32_t skill, int32_t level);
		uint64_t getReqMana(uint32_t magLevel);
		void setSkillMultiplier(skills_t s, float v) {skillMultipliers[s] = v;}
		static uint32_t skillBase[SKILL_LAST + 1];

	private:
		int32_t lessLoss;
		typedef std::map<uint32_t, uint32_t> cacheMap;
		cacheMap cacheSkill[SKILL_LAST + 1];
		cacheMap cacheMana;

		bool attackable, needPremium;
		uint16_t soulGainTicks, soulMax;
		uint32_t id, fromVocation, gainCap, baseSpeed, attackSpeed;
		std::string name, description;

		uint32_t gain[GAIN_LAST + 1], gainTicks[GAIN_LAST + 1], gainAmount[GAIN_LAST + 1];
		int16_t absorbPercent[COMBAT_LAST + 1];
		float skillMultipliers[SKILL_LAST + 1];
		float formulaMultipliers[MULTIPLIER_LAST + 1];
};

typedef std::map<uint32_t, Vocation*> VocationsMap;

class Vocations
{
	public:
		virtual ~Vocations() {clear();}
		static Vocations* getInstance()
		{
			static Vocations instance;
			return &instance;
		}

		bool reload();
		bool loadFromXml();
		bool parseVocationNode(xmlNodePtr p);

		Vocation* getVocation(uint32_t vocId);
		int32_t getVocationId(const std::string& name);
		int32_t getPromotedVocation(uint32_t vocationId);

		VocationsMap::iterator getFirstVocation() {return vocationsMap.begin();}
		VocationsMap::iterator getLastVocation() {return vocationsMap.end();}

	private:
		static Vocation defVoc;
		VocationsMap vocationsMap;

		Vocations() {}
		void clear();
};

#endif
