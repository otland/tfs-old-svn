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

#include "enums.h"
#include <string>
#include <map>
#include "item.h"

class Vocation
{
	public:
		virtual ~Vocation();

		uint32_t getVocId() const {return id;}
		const std::string& getVocName() const {return name;}
		const std::string& getVocDescription() const {return description;}

		uint32_t getReqSkillTries(int32_t skill, int32_t level);
		uint64_t getReqMana(uint32_t magLevel);
		int16_t getAbsorbPercent(CombatType_t combat) const {return absorbPercent[combat];}

		uint32_t getHealthGain() const {return gainHP;}
		uint32_t getHealthGainTicks() const {return gainHealthTicks;}
		uint32_t getHealthGainAmount() const {return gainHealthAmount;}
		uint32_t getManaGain() const {return gainMana;}
		uint32_t getManaGainTicks() const {return gainManaTicks;}
		uint32_t getManaGainAmount() const {return gainManaAmount;}

		uint16_t getSoulMax() const {return std::min((uint32_t)soulMax, (uint32_t)255);}
		uint16_t getSoulGainTicks() const {return gainSoulTicks;}
		uint32_t getCapGain() const {return gainCap;}
		uint32_t getAttackSpeed() const {return attackSpeed;}
		uint32_t getBaseSpeed() const {return baseSpeed;}

		uint32_t getFromVocation() const {return fromVocation;}
		bool isAttackable() const {return attackable;}
		bool isPremiumNeeded() const {return needPremium;}

		float meleeDamageMultipler, distDamageMultipler, defenseMultipler, armorMultipler;

	protected:
		friend class Vocations;
		Vocation();

		uint32_t id;
		std::string name;
		std::string description;

		uint32_t gainHP;
		uint32_t gainHealthTicks;
		uint32_t gainHealthAmount;
		uint32_t gainMana;
		uint32_t gainManaTicks;
		uint32_t gainManaAmount;

		uint16_t gainSoulTicks;
		uint16_t soulMax;
		uint32_t gainCap;
		uint32_t attackSpeed;
		uint32_t baseSpeed;

		uint32_t fromVocation;
		bool attackable;
		bool needPremium;
		int16_t absorbPercent[COMBAT_LAST + 1];

		static uint32_t skillBase[SKILL_LAST + 1];
		float skillMultipliers[SKILL_LAST + 1];
		float manaMultiplier;

		typedef std::map<uint32_t, uint32_t> cacheMap;
		cacheMap cacheSkill[SKILL_LAST + 1];
		cacheMap cacheMana;
};

typedef std::map<uint32_t, Vocation*> VocationsMap;

class Vocations
{
	public:
		Vocations();
		virtual ~Vocations();

		bool loadFromXml();
		Vocation* getVocation(uint32_t vocId);
		int32_t getVocationId(const std::string& name);
		int32_t getPromotedVocation(uint32_t vocationId);

		VocationsMap::iterator getFirstVocation() {return vocationsMap.begin();}
		VocationsMap::iterator getLastVocation() {return vocationsMap.end();}

	private:
		VocationsMap vocationsMap;
		Vocation defVoc;
};

#endif
