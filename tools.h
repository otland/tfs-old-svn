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
typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

enum DistributionType_t
{
	DISTRO_UNIFORM,
	DISTRO_SQUARE,
	DISTRO_NORMAL
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

bool utf8ToLatin1(char* intext, std::string& outtext);
bool readXMLInteger(xmlNodePtr node, const char* tag, int& value);
#if (defined __WINDOWS__ || defined WIN32) && !defined __GNUC__
bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t& value);
#endif
bool readXMLInteger64(xmlNodePtr node, const char* tag, uint64_t& value);
bool readXMLFloat(xmlNodePtr node, const char* tag, float& value);
bool readXMLString(xmlNodePtr node, const char* tag, std::string& value);
bool readXMLContentString(xmlNodePtr node, std::string& value);

std::vector<std::string> explodeString(const std::string& inString, const std::string& separator);
std::vector<int32_t> vectorAtoi(std::vector<std::string> stringVector);
bool hasBitSet(uint32_t flag, uint32_t flags);

std::string generateRecoveryKey(int32_t fieldCount, int32_t fieldLength);

bool isNumber(char character);
bool isUppercaseLetter(char character);
bool isLowercaseLetter(char character);
bool isPasswordCharacter(char character);

bool isValidName(std::string text, bool forceUppercaseOnFirstLetter = true);
bool isValidPassword(std::string text);
bool isNumbers(std::string text);

bool checkText(std::string text, std::string str);

int random_range(int lowest_number, int highest_number, DistributionType_t type = DISTRO_UNIFORM);

Direction getDirection(std::string string);
Direction getReverseDirection(Direction dir);
Position getNextPosition(Direction direction, Position pos);

char upchar(char c);

std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end);

void formatDate(time_t time, char* buffer);
void formatDate2(time_t time, char* buffer);
void formatIP(uint32_t ip, char* buffer);
std::string formatTime(int32_t hours, int32_t minutes);

std::string trimString(std::string& str);

MagicEffectClasses getMagicEffect(const std::string& strValue);
ShootType_t getShootType(const std::string& strValue);
Ammo_t getAmmoType(const std::string& strValue);
AmmoAction_t getAmmoAction(const std::string& strValue);

std::string getSkillName(uint16_t skillid);
skills_t getSkillId(std::string param);

std::string getReason(int32_t reasonId);
std::string getAction(int32_t actionId, bool IPBanishment);

bool fileExists(const char* filename);
uint32_t adlerChecksum(uint8_t *data, size_t length);

bool operator<(const ShopInfo& left, const ShopInfo& right);
void sortItems(std::list<ShopInfo>& itemList);
#endif
