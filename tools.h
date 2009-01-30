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

#include "otsystem.h"
#include "position.h"
#include "const.h"
#include "enums.h"

#include <string>
#include <algorithm>

#include <libxml/parser.h>
#include <boost/tokenizer.hpp>

typedef std::vector<std::string> StringVec;
typedef std::vector<int32_t> IntegerVec;
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

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
	FILE_TYPE_CONFIG
};

std::string transformToMD5(std::string plainText, bool upperCase = false);
std::string transformToSHA1(std::string plainText, bool upperCase = false);
bool passwordTest(const std::string &plain, std::string &hash);

void replaceString(std::string& str, const std::string sought, const std::string replacement);
void trim_right(std::string& source, const std::string& t);
void trim_left(std::string& source, const std::string& t);
void toLowerCaseString(std::string& source);
void toUpperCaseString(std::string& source);
std::string asLowerCaseString(const std::string& source);
std::string asUpperCaseString(const std::string& source);
bool booleanString(std::string source);

bool utf8ToLatin1(char* intext, std::string& outtext);
bool readXMLInteger(xmlNodePtr node, const char* tag, int& value);
#if (defined __WINDOWS__ || defined WIN32) && !defined __GNUC__
bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t& value);
#endif
bool readXMLInteger64(xmlNodePtr node, const char* tag, uint64_t& value);
bool readXMLFloat(xmlNodePtr node, const char* tag, float& value);
bool readXMLString(xmlNodePtr node, const char* tag, std::string& value);
bool readXMLContentString(xmlNodePtr node, std::string& value);

StringVec explodeString(const std::string& string, const std::string& separator);
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

std::string generateRecoveryKey(int32_t fieldCount, int32_t fieldLength);
int32_t random_range(int32_t lowest_number, int32_t highest_number, DistributionType_t type = DISTRO_UNIFORM);

Direction getDirection(std::string string);
Direction getReverseDirection(Direction dir);
Position getNextPosition(Direction direction, Position pos);

void formatDate(time_t time, char* buffer);
void formatDate2(time_t time, char* buffer);
void formatIP(uint32_t ip, char* buffer);
std::string formatTime(int32_t hours, int32_t minutes);

MagicEffectClasses getMagicEffect(const std::string& strValue);
ShootType_t getShootType(const std::string& strValue);
Ammo_t getAmmoType(const std::string& strValue);
AmmoAction_t getAmmoAction(const std::string& strValue);
CombatType_t getCombatType(const std::string& strValue);
FluidTypes_t getFluidType(const std::string& strValue);
std::string getCombatName(CombatType_t combatType);

std::string getSkillName(uint16_t skillid, bool suffix = true);
skills_t getSkillId(std::string param);

std::string getReason(int32_t reasonId);
std::string getAction(int32_t actionId, bool ipBanishment);

bool fileExists(const char* filename);
uint32_t adlerChecksum(uint8_t *data, size_t length);

std::string getFilePath(FileType_t filetype, std::string filename);
#endif
