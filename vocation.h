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

		uint32_t getHealthGain() const {return gainHealth;}
		void setGainHealth(uint32_t v) {gainHealth = v;}

		uint32_t getHealthGainTicks() const {return gainHealthTicks;}
		void setGainHealthTicks(uint32_t v) {gainHealthTicks = v;}

		uint32_t getHealthGainAmount() const {return gainHealthAmount;}
		void setGainHealthAmount(uint32_t v) {gainHealthAmount = v;}

		uint32_t getManaGain() const {return gainMana;}
		void setGainMana(uint32_t v) {gainMana = v;}

		uint32_t getManaGainTicks() const {return gainManaTicks;}
		void setGainManaTicks(uint32_t v) {gainManaTicks = v;}

		uint32_t getManaGainAmount() const {return gainManaAmount;}
		void setGainManaAmount(uint32_t v) {gainManaAmount = v;}

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
		void increaseAbsorbPercent(CombatType_t c, int16_t v) {absorbPercent[c] += v;}

		float getManaMultiplier() const {return manaMultiplier;}
		void setManaMultiplier(float v) {manaMultiplier = v;}
		float getMeleeMultiplier() const {return meleeMultiplier;}
		void setMeleeMultiplier(float v) {meleeMultiplier = v;}
		float getDistanceMultiplier() const {return distanceMultiplier;}
		void setDistanceMultiplier(float v) {distanceMultiplier = v;}
		float getWandMultiplier() const {return wandMultiplier;}
		void setWandMultiplier(float v) {wandMultiplier = v;}
		float getMagicMultiplier() const {return magicMultiplier;}
		void setMagicMultiplier(float v) {magicMultiplier = v;}
		float getMagicHealingMultiplier() const {return magicHealingMultiplier;}
		void setMagicHealingMultiplier(float v) {magicHealingMultiplier = v;}
		float getDefenseMultiplier() const {return defenseMultiplier;}
		void setDefenseMultiplier(float v) {defenseMultiplier = v;}
		float getArmorMultiplier() const {return armorMultiplier;}
		void setArmorMultiplier(float v) {armorMultiplier = v;}

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
		uint32_t id, fromVocation, gainHealth, gainHealthTicks, gainHealthAmount, gainMana,
			gainManaTicks, gainManaAmount, gainCap, baseSpeed, attackSpeed;
		float manaMultiplier, meleeMultiplier, distanceMultiplier, wandMultiplier, magicMultiplier,
			magicHealingMultiplier, defenseMultiplier, armorMultiplier;
		std::string name, description;

		int16_t absorbPercent[COMBAT_LAST + 1];
		float skillMultipliers[SKILL_LAST + 1];
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
