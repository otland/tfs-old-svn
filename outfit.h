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

#ifndef __OUTFIT__
#define __OUTFIT__

#include "otsystem.h"
#include "enums.h"

#define OUTFITS_QUEST_VALUE 1
#define OUTFITS_MAX_NUMBER 25

enum AddonRequirement_t
{
	REQUIREMENT_NONE = 0,
	REQUIREMENT_FIRST,
	REQUIREMENT_SECOND,
	REQUIREMENT_BOTH,
	REQUIERMENT_ANY
};

struct Outfit
{
	Outfit()
	{
		memset(skills, 0, sizeof(skills));
		memset(skillsPercent, 0, sizeof(skillsPercent));
		memset(stats, 0 , sizeof(stats));
		memset(statsPercent, 0, sizeof(statsPercent));

		requirement = REQUIREMENT_BOTH;
		memset(absorbPercent, 0, sizeof(absorbPercent));
		premium = manaShield = invisible = regeneration = false;
		looktype = addons = access = quest = speed = healthGain = healthTicks = manaGain = manaTicks = conditionSuppressions = 0;
	}

	bool premium, manaShield, invisible, regeneration;
	AddonRequirement_t requirement;
	int16_t absorbPercent[COMBAT_LAST + 1];

	uint16_t access, addons;
	uint32_t looktype, quest;
	int32_t skills[SKILL_LAST + 1], skillsPercent[SKILL_LAST + 1], stats[STAT_LAST + 1], statsPercent[STAT_LAST + 1],
		speed, healthGain, healthTicks, manaGain, manaTicks, conditionSuppressions;
};

typedef std::list<Outfit*> OutfitListType;

class OutfitList
{
	public:
		OutfitList() {}
		virtual ~OutfitList();

		void addOutfit(const Outfit& outfit);
		bool remOutfit(const Outfit& outfit);

		const OutfitListType& getOutfits() const {return m_list;}
		bool isInList(uint32_t playerId, uint32_t lookType, uint32_t addons) const;

	private:
		OutfitListType m_list;
};

class Outfits
{
	public:
		virtual ~Outfits();
		static Outfits* getInstance()
		{
			static Outfits instance;
			return &instance;
		}

		bool loadFromXml();
		bool parseOutfitNode(xmlNodePtr p);
		const OutfitListType& getOutfits(uint32_t type) {return getOutfitList(type).getOutfits();}
		const OutfitList& getOutfitList(uint32_t type)
		{
			if(type < m_list.size())
				return *m_list[type];

			if(type == PLAYERSEX_FEMALE)
				return m_femaleList;

			return m_maleList;
		}

		bool addAttributes(uint32_t playerId, uint32_t lookType, uint16_t addons);
		bool removeAttributes(uint32_t playerId, uint32_t lookType);

		const std::string& getOutfitName(uint32_t lookType) const;
		int16_t getOutfitAbsorb(uint32_t lookType, uint32_t type, CombatType_t combat);

	private:
		Outfits();
		OutfitList m_femaleList, m_maleList;

		typedef std::vector<OutfitList*> OutfitsListVector;
		OutfitsListVector m_list;

		typedef std::map<uint32_t, std::string> OutfitNamesMap;
		OutfitNamesMap outfitNamesMap;
};
#endif
