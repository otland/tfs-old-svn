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

#ifndef __OTSERV_OUTFIT_H__
#define __OTSERV_OUTFIT_H__
#include "otsystem.h"
#include "enums.h"

#define OUTFITS_QUEST_VALUE 1
#define OUTFITS_MAX_NUMBER 25

struct Outfit
{
	uint32_t looktype;
	uint32_t addons;
	uint32_t quest;
	bool premium;
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
		bool isInList(int32_t playerId, uint32_t looktype, uint32_t addons) const;

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

		const OutfitListType& getOutfits(uint32_t type)
		{
			return getOutfitList(type).getOutfits();
		}

		const OutfitList& getOutfitList(uint32_t type)
		{
			if(type < m_list.size())
				return *m_list[type];


			if(type == PLAYERSEX_FEMALE)
				return m_femaleList;

			return m_maleList;
		}

		const std::string& getOutfitName(uint32_t looktype) const
		{
			OutfitNamesMap::const_iterator it = outfitNamesMap.find(looktype);
			if(it != outfitNamesMap.end())
				return it->second;

			static const std::string ret = "Outfit";
			return ret;
		}

	private:
		Outfits();

		typedef std::vector<OutfitList*> OutfitsListVector;
		OutfitsListVector m_list;

		typedef std::map<uint32_t, std::string> OutfitNamesMap;
		OutfitNamesMap outfitNamesMap;

		OutfitList m_femaleList;
		OutfitList m_maleList;
};

#endif
