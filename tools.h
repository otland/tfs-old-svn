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

#ifndef __OTSERV_TOOLS_H__
#define __OTSERV_TOOLS_H__

#include "definitions.h"
#include "position.h"
#include "const.h"
#include "enums.h"

#include <string>
#include <algorithm>

#include <libxml/parser.h>

#include <boost/tokenizer.hpp>
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

#ifndef OPENTIBIA_API
	#define OPENTIBIA_API extern
#endif

enum DistributionType_t
{
	DISTRO_UNIFORM,
	DISTRO_SQUARE,
	DISTRO_NORMAL
};

OPENTIBIA_API std::string transformToMD5(std::string plainText, bool upperCase = false);
OPENTIBIA_API std::string transformToSHA1(std::string plainText, bool upperCase = false);
OPENTIBIA_API bool passwordTest(const std::string &plain, std::string &hash);

OPENTIBIA_API void replaceString(std::string& str, const std::string sought, const std::string replacement);
OPENTIBIA_API void trim_right(std::string& source, const std::string& t);
OPENTIBIA_API void trim_left(std::string& source, const std::string& t);
OPENTIBIA_API void toLowerCaseString(std::string& source);
OPENTIBIA_API void toUpperCaseString(std::string& source);
OPENTIBIA_API std::string asLowerCaseString(const std::string& source);
OPENTIBIA_API std::string asUpperCaseString(const std::string& source);

OPENTIBIA_API bool utf8ToLatin1(char* intext, std::string& outtext);
OPENTIBIA_API bool readXMLInteger(xmlNodePtr node, const char* tag, int& value);
#if (defined __WINDOWS__ || defined WIN32) && !defined __GNUC__
OPENTIBIA_API bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t& value);
#endif
OPENTIBIA_API bool readXMLInteger64(xmlNodePtr node, const char* tag, uint64_t& value);
OPENTIBIA_API bool readXMLFloat(xmlNodePtr node, const char* tag, float& value);
OPENTIBIA_API bool readXMLString(xmlNodePtr node, const char* tag, std::string& value);
OPENTIBIA_API bool readXMLContentString(xmlNodePtr node, std::string& value);

typedef std::vector<std::string> StringVec;
typedef std::vector<int32_t> IntegerVec;

OPENTIBIA_API StringVec explodeString(const std::string& inString, const std::string& separator, int32_t limit = -1);
OPENTIBIA_API IntegerVec  vectorAtoi(std::vector<std::string> stringVector);
OPENTIBIA_API bool hasBitSet(uint32_t flag, uint32_t flags);

OPENTIBIA_API std::string generateRecoveryKey(int32_t fieldCount, int32_t fieldLength);

OPENTIBIA_API bool isNumber(char character);
OPENTIBIA_API bool isUppercaseLetter(char character);
OPENTIBIA_API bool isLowercaseLetter(char character);
OPENTIBIA_API bool isPasswordCharacter(char character);

OPENTIBIA_API bool isValidName(std::string text, bool forceUppercaseOnFirstLetter = true);
OPENTIBIA_API bool isValidPassword(std::string text);
OPENTIBIA_API bool isNumbers(std::string text);

OPENTIBIA_API bool checkText(std::string text, std::string str);

OPENTIBIA_API int random_range(int lowest_number, int highest_number, DistributionType_t type = DISTRO_UNIFORM);

OPENTIBIA_API Direction getDirection(std::string string);
OPENTIBIA_API Direction getReverseDirection(Direction dir);
OPENTIBIA_API Position getNextPosition(Direction direction, Position pos);

#if !defined __GNUC__ || __GNUC__ < 3
	OPENTIBIA_API char upchar(char c);
#endif

OPENTIBIA_API std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end);

OPENTIBIA_API std::string formatDate(time_t time);
OPENTIBIA_API std::string formatDateShort(time_t time);
OPENTIBIA_API std::string convertIPToString(uint32_t ip);

OPENTIBIA_API std::string trimString(std::string& str);

OPENTIBIA_API MagicEffectClasses getMagicEffect(const std::string& strValue);
OPENTIBIA_API ShootType_t getShootType(const std::string& strValue);
OPENTIBIA_API Ammo_t getAmmoType(const std::string& strValue);
OPENTIBIA_API AmmoAction_t getAmmoAction(const std::string& strValue);
OPENTIBIA_API CombatType_t getCombatType(const std::string& strValue);
OPENTIBIA_API std::string getCombatName(CombatType_t combatType);

OPENTIBIA_API std::string getSkillName(uint16_t skillid);
OPENTIBIA_API skills_t getSkillId(std::string param);

OPENTIBIA_API int32_t actionStringToInt(std::string action);
OPENTIBIA_API int32_t reasonStringToInt(std::string reason);
OPENTIBIA_API std::string getReason(int32_t reasonId);
OPENTIBIA_API std::string getAction(int32_t actionId, bool IPBanishment);

OPENTIBIA_API bool fileExists(const char* filename);
OPENTIBIA_API uint32_t adlerChecksum(uint8_t* data, size_t len);

#undef OPENTIBIA_API

#endif
