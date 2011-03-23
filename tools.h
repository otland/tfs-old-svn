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

#ifndef __TOOLS__
#define __TOOLS__
#include "otsystem.h"

#include "enums.h"
#include "const.h"

#include <libxml/parser.h>
#include <boost/tokenizer.hpp>
#include "position.h"

typedef std::vector<std::string> StringVec;
typedef std::vector<int32_t> IntegerVec;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
typedef std::map<int32_t, bool> VocationMap;

enum DistributionType_t
{
	DISTRO_UNIFORM,
	DISTRO_SQUARE,
	DISTRO_NORMAL
};

enum FileType_t
{
	FILE_TYPE_XML,
	FILE_TYPE_LOG,
	FILE_TYPE_OTHER,
	FILE_TYPE_CONFIG,
	FILE_TYPE_MOD
};

std::string transformToMD5(std::string plainText, bool upperCase);
std::string transformToSHA1(std::string plainText, bool upperCase);
std::string transformToSHA256(std::string plainText, bool upperCase);
std::string transformToSHA512(std::string plainText, bool upperCase);

void _encrypt(std::string& str, bool upperCase);
bool encryptTest(std::string plain, std::string& hash);

bool replaceString(std::string& text, const std::string& key, const std::string& value);
void trim_right(std::string& source, const std::string& t);
void trim_left(std::string& source, const std::string& t);
void toLowerCaseString(std::string& source);
void toUpperCaseString(std::string& source);
std::string asLowerCaseString(const std::string& source);
std::string asUpperCaseString(const std::string& source);
bool booleanString(std::string source);

template <typename T>
inline void asString(const T& object, const std::string& s)
{
	std::ostringstream ss;
	ss << object;
	s = ss.str();
}

template <typename T>
inline std::string asString(const T& object)
{
	std::ostringstream ss;
	ss << object;
	return ss.str();
}

template<class T>
inline T fromString(const std::string& s)
{
	std::istringstream ss (s);
	T t;
	ss >> t;
	return t;
}

bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t& value);
bool readXMLInteger64(xmlNodePtr node, const char* tag, int64_t& value);
bool readXMLFloat(xmlNodePtr node, const char* tag, float& value);
bool readXMLString(xmlNodePtr node, const char* tag, std::string& value);
bool readXMLContentString(xmlNodePtr node, std::string& value);
bool parseXMLContentString(xmlNodePtr node, std::string& value);
std::string getLastXMLError();
bool utf8ToLatin1(char* intext, std::string& outtext);

StringVec explodeString(const std::string& string, const std::string& separator, bool trim = true);
IntegerVec vectorAtoi(StringVec stringVector);
bool hasBitSet(uint32_t flag, uint32_t flags);

bool isNumber(char character);
bool isLowercaseLetter(char character);
bool isUppercaseLetter(char character);
bool isPasswordCharacter(char character);

bool isValidAccountName(std::string text);
bool isValidPassword(std::string text);
bool isValidName(std::string text, bool forceUppercaseOnFirstLetter = true);
bool isNumbers(std::string text);

char upchar(char character);
bool checkText(std::string text, std::string str);
std::string trimString(std::string& str);
std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end);

std::string generateRecoveryKey(int32_t fieldCount, int32_t fieldLength, bool mixCase = false);
int32_t random_range(int32_t lowest_number, int32_t highest_number, DistributionType_t type = DISTRO_UNIFORM);

int32_t round(float v);
uint32_t rand24b();
float box_muller(float m, float s);

Skulls_t getSkulls(std::string strValue);
PartyShields_t getShields(std::string strValue);
GuildEmblems_t getEmblems(std::string strValue);

Direction getDirection(std::string string);
Direction getDirectionTo(Position pos1, Position pos2, bool extended = true);
Direction getReverseDirection(Direction dir);
Position getNextPosition(Direction direction, Position pos);

std::string formatDate(time_t _time = 0);
std::string formatDateEx(time_t _time = 0, std::string format = "%d %b %Y, %H:%M:%S");
std::string formatTime(time_t _time = 0, bool miliseconds = false);
std::string convertIPAddress(uint32_t ip);

MagicEffect_t getMagicEffect(const std::string& strValue);
ShootEffect_t getShootType(const std::string& strValue);
Ammo_t getAmmoType(const std::string& strValue);
AmmoAction_t getAmmoAction(const std::string& strValue);
CombatType_t getCombatType(const std::string& strValue);
FluidTypes_t getFluidType(const std::string& strValue);
skills_t getSkillId(const std::string& strValue);

std::string getCombatName(CombatType_t combatType);
std::string getSkillName(uint16_t skillId, bool suffix = true);

std::string getReason(int32_t reasonId);
std::string getAction(ViolationAction_t actionId, bool ipBanishment);

std::string parseVocationString(StringVec vocStringVec);
bool parseVocationNode(xmlNodePtr vocationNode, VocationMap& vocationMap, StringVec& vocStringMap, std::string& errorStr);
bool parseIntegerVec(std::string str, IntegerVec& intVector);

bool fileExists(const char* filename);
uint32_t adlerChecksum(uint8_t *data, size_t length);

std::string getFilePath(FileType_t type, std::string name = "");
#endif
