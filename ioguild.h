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

#ifndef __IOGUILD__
#define __IOGUILD__
#include "otsystem.h"

#include "player.h"
class IOGuild
{
	public:
		virtual ~IOGuild() {}
		static IOGuild* getInstance()
		{
			static IOGuild instance;
			return &instance;
		}

		bool guildExists(uint32_t guildId);
		bool createGuild(Player* player);
		bool disbandGuild(uint32_t guild_id);

		std::string getMotd(uint32_t guildId);
		bool setMotd(uint32_t guildId, std::string newMotd);

		int8_t getGuildLevel(uint32_t guid);
		bool setGuildLevel(uint32_t guid, GuildLevel_t level);

		bool invitePlayerToGuild(uint32_t guid, uint32_t guildId);
		bool revokeGuildInvite(uint32_t guid, uint32_t guildId);
		bool joinGuild(Player* player, uint32_t guildId, bool creation = false);

		bool rankNameExists(std::string rankName, uint32_t guildId);
		std::string getRankName(int16_t guildLevel, uint32_t guildId);
		bool changeRankName(std::string oldRankName, std::string newRankName, uint32_t guildId);

		bool hasGuild(uint32_t guid);
		bool isInvitedToGuild(uint32_t guid, uint32_t guildId);

		bool getGuildIdByName(uint32_t& guildId, const std::string& guildName);
		bool getGuildNameById(std::string& guildName, uint32_t guildId);

		uint32_t getRankIdByGuildIdAndLevel(uint32_t guildId, uint32_t guildLevel);
		bool getRankIdByGuildIdAndName(uint32_t &rankId, const std::string& rankName, uint32_t& guildId);

		uint32_t getGuildId(uint32_t guid);
		bool setGuildNick(uint32_t guid, std::string guildNick);

		bool swapGuildIdToOwner(uint32_t& value);
		bool updateOwnerId(uint32_t guildId, uint32_t guid);

	private:
		IOGuild() {}
};
#endif
