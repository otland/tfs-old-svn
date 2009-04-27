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
#include "otpch.h"
#include <iostream>
#include <iomanip>

#include "tools.h"
#include "md5.h"
#include "sha1.h"

#include "vocation.h"
#include "configmanager.h"
extern ConfigManager g_config;

std::string transformToSHA1(std::string plainText, bool upperCase /*= false*/)
{
	SHA1 sha1;
	unsigned sha1Hash[5];
	std::stringstream hexStream;

	sha1.Input((const uint8_t*)plainText.c_str(), plainText.length());
	sha1.Result(sha1Hash);

	hexStream.flags(std::ios::hex | std::ios::uppercase);
	for(uint32_t i = 0; i < 5; ++i)
		hexStream << std::setw(8) << std::setfill('0') << (uint32_t)sha1Hash[i];

	std::string hexStr = hexStream.str();
	if(!upperCase)
		toLowerCaseString(hexStr);

	return hexStr;
}

std::string transformToMD5(std::string plainText, bool upperCase /*= false*/)
{
	MD5_CTX m_md5;
	std::stringstream hexStream;

	MD5Init(&m_md5, 0);
	MD5Update(&m_md5, (const uint8_t*)plainText.c_str(), plainText.length());
	MD5Final(&m_md5);

	hexStream.flags(std::ios::hex | std::ios::uppercase);
	for(uint32_t i = 0; i < 16; ++i)
		hexStream << std::setw(2) << std::setfill('0') << (uint32_t)m_md5.digest[i];

	std::string hexStr = hexStream.str();
	if(!upperCase)
		toLowerCaseString(hexStr);

	return hexStr;
}

bool passwordTest(const std::string &plain, std::string &hash)
{
	switch(g_config.getNumber(ConfigManager::PASSWORDTYPE))
	{
		case PASSWORD_TYPE_MD5:
		{
			std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
			return transformToMD5(plain, true) == hash;
		}

		case PASSWORD_TYPE_SHA1:
		{
			std::transform(hash.begin(), hash.end(), hash.begin(), upchar);
			return transformToSHA1(plain, true) == hash;
		}

		default:
			return plain == hash;
	}
	return false;
}

void replaceString(std::string& str, const std::string sought, const std::string replacement)
{
	size_t pos = 0, start = 0, soughtLen = sought.length(), replaceLen = replacement.length();
	while((pos = str.find(sought, start)) != std::string::npos)
	{
		str = str.substr(0, pos) + replacement + str.substr(pos + soughtLen);
		start = pos + replaceLen;
	}
}

void trim_right(std::string& source, const std::string& t)
{
	source.erase(source.find_last_not_of(t)+1);
}

void trim_left(std::string& source, const std::string& t)
{
	source.erase(0, source.find_first_not_of(t));
}

void toLowerCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), tolower);
}

void toUpperCaseString(std::string& source)
{
	std::transform(source.begin(), source.end(), source.begin(), upchar);
}

std::string asLowerCaseString(const std::string& source)
{
	std::string s = source;
	toLowerCaseString(s);
	return s;
}

std::string asUpperCaseString(const std::string& source)
{
	std::string s = source;
	toUpperCaseString(s);
	return s;
}

bool booleanString(std::string source)
{
	toLowerCaseString(source);
	return (source == "yes" || source == "true" || atoi(source.c_str()) > 0);
}

bool readXMLInteger(xmlNodePtr node, const char* tag, int& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue)
	{
		value = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

#if (defined __WINDOWS__ || defined WIN32) && !defined __GNUC__
bool readXMLInteger(xmlNodePtr node, const char* tag, int32_t& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue)
	{
		value = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}
#endif

bool readXMLInteger64(xmlNodePtr node, const char* tag, int64_t& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue)
	{
		value = ATOI64(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool readXMLFloat(xmlNodePtr node, const char* tag, float& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue)
	{
		value = atof(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool readXMLString(xmlNodePtr node, const char* tag, std::string& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue)
	{
		if(!utf8ToLatin1(nodeValue, value))
			value = nodeValue;

		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool readXMLContentString(xmlNodePtr node, std::string& value)
{
	char* nodeValue = (char*)xmlNodeGetContent(node);
	if(nodeValue)
	{
		if(!utf8ToLatin1(nodeValue, value))
			value = nodeValue;

		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool parseXMLContentString(xmlNodePtr node, std::string& value)
{
	bool result = false;
	std::string compareValue;
	while(node)
	{
		if(!xmlStrcmp(node->name, (const xmlChar*)"text") || node->type == XML_CDATA_SECTION_NODE)
		{
			if(readXMLContentString(node, compareValue))
			{
				trim_left(compareValue, "\r");
				trim_left(compareValue, "\n");
				trim_left(compareValue, " ");
				if(compareValue.length() > value.length())
				{
					value = compareValue;
					if(!result)
						result = true;
				}
			}
		}

		node = node->next;
	}

	return result;
}

std::string getLastXMLError()
{
	std::stringstream ss;
	xmlErrorPtr lastError = xmlGetLastError();
	if(lastError->line)
		ss << "Line: " << lastError->line << ", ";

	ss << "Info: " << lastError->message << std::endl;
	return ss.str();
}

bool utf8ToLatin1(char* intext, std::string& outtext)
{
	outtext = "";

	if(intext == NULL)
		return false;

	int32_t inlen  = strlen(intext);
	if(inlen == 0)
		return false;

	int32_t outlen = inlen * 2;
	uint8_t* outbuf = new uint8_t[outlen];
	int32_t res = UTF8Toisolat1(outbuf, &outlen, (uint8_t*)intext, &inlen);
	if(res < 0)
	{
		delete[] outbuf;
		return false;
	}

	outbuf[outlen] = '\0';
	outtext = (char*)outbuf;
	delete[] outbuf;
	return true;
}

StringVec explodeString(const std::string& string, const std::string& separator)
{
	StringVec returnVector;
	size_t start = 0, end = 0;

	while((end = string.find(separator, start)) != std::string::npos)
	{
		returnVector.push_back(string.substr(start, end - start));
		start = end + separator.size();
	}

	returnVector.push_back(string.substr(start));
	return returnVector;
}

IntegerVec vectorAtoi(StringVec stringVector)
{
	IntegerVec returnVector;
	for(StringVec::iterator it = stringVector.begin(); it != stringVector.end(); ++it)
		returnVector.push_back(atoi((*it).c_str()));

	return returnVector;
}

bool hasBitSet(uint32_t flag, uint32_t flags)
{
	return ((flags & flag) == flag);
}

int32_t round(float v)
{
	int32_t t = (long)std::floor(v);
	if((v - t) > 0.5)
		return t + 1;

	return t;
}

uint32_t rand24b()
{
	return ((rand() << 12) ^ (rand())) & (0xFFFFFF);
}

float box_muller(float m, float s)
{
	// normal random variate generator
	// mean m, standard deviation s
	float x1, x2, w, y1;
	static float y2;
	static bool useLast = false;

	if(useLast) // use value from previous call
	{
		y1 = y2;
		useLast = false;
	}
	else
	{
		do
		{
			double r1 = (((float)(rand()) / RAND_MAX));
			double r2 = (((float)(rand()) / RAND_MAX));

			x1 = 2.0 * r1 - 1.0;
			x2 = 2.0 * r2 - 1.0;
			w = x1 * x1 + x2 * x2;
		}
		while(w >= 1.0);

		w = sqrt((-2.0 * log(w)) / w);
		y1 = x1 * w;
		y2 = x2 * w;
		useLast = true;
	}
	return (m + y1 * s);
}

int32_t random_range(int32_t lowestNumber, int32_t highestNumber, DistributionType_t type /*= DISTRO_UNIFORM*/)
{
	if(highestNumber == lowestNumber)
		return lowestNumber;

	if(lowestNumber > highestNumber)
	{
		int32_t tmp = highestNumber;
		highestNumber = lowestNumber;
		lowestNumber = tmp;
	}

	switch(type)
	{
		case DISTRO_UNIFORM:
			return (lowestNumber + ((int32_t)rand24b() % (highestNumber - lowestNumber + 1)));
			break;
		case DISTRO_NORMAL:
			return (lowestNumber + int32_t(float(highestNumber - lowestNumber) * (float)std::min((float)1, std::max((float)0, box_muller(0.5, 0.25)))));
			break;
		default:
		{
			const float randMax = 16777216;
			return (lowestNumber + int32_t(float(highestNumber - lowestNumber) * float(1.f - sqrt((1.f * rand24b()) / randMax))));
			break;
		}
	}
}

char upchar(char character)
{
	if((character >= 97 && character <= 122) || (character <= -1 && character >= -32))
		character -= 32;

	return character;
}

bool isNumber(char character)
{
	return (character >= 48 && character <= 57);
}

bool isLowercaseLetter(char character)
{
	return (character >= 97 && character <= 122);
}

bool isUppercaseLetter(char character)
{
	return (character >= 65 && character <= 90);
}

bool isPasswordCharacter(char character)
{
	return ((character >= 33 && character <= 47) || (character >= 58 && character <= 64) || (character >= 91 && character <= 96) || (character >= 123 && character <= 126));
}

bool isValidAccountName(std::string text)
{
	toLowerCaseString(text);

	uint32_t textLength = text.length();
	for(uint32_t size = 0; size < textLength; size++)
	{
		if(!isLowercaseLetter(text[size]) && !isNumber(text[size]))
			return false;
	}
	return true;
}

bool isValidPassword(std::string text)
{
	toLowerCaseString(text);

	uint32_t textLength = text.length();
	for(uint32_t size = 0; size < textLength; size++)
	{
		if(!isLowercaseLetter(text[size]) && !isNumber(text[size]) && !isPasswordCharacter(text[size]))
			return false;
	}
	return true;
}

bool isValidName(std::string text, bool forceUppercaseOnFirstLetter/* = true*/)
{
	uint32_t textLength = text.length(), lenBeforeSpace = 1/*, lenBeforeQuote = 1*/, lenBeforeDash = 1, repeatedCharacter = 0; //Elf
	char lastChar = 32;

	if(forceUppercaseOnFirstLetter)
	{
		if(!isUppercaseLetter(text[0]))
			return false;
	}
	else if(!isLowercaseLetter(text[0]) && !isUppercaseLetter(text[0]))
		return false;

	for(uint32_t size = 1; size < textLength; size++)
	{
		if(text[size] != 32)
		{
			lenBeforeSpace++;

			// [START] Elf
			/*if(text[size] != 39)
				lenBeforeQuote++;
			else
			{
				if(lenBeforeQuote <= 1 || size == textLength - 1 || text[size + 1] == 32)
					return false;

				lenBeforeQuote = 0;
			}*/
			// [END] Elf

			if(text[size] != 45)
				lenBeforeDash++;
			else
			{
				if(lenBeforeDash <= 1 || size == textLength - 1 || text[size + 1] == 32)
					return false;

				lenBeforeDash = 0;
			}

			if(text[size] == lastChar)
			{
				repeatedCharacter++;
				if(repeatedCharacter > 2)
					return false;
			}
			else
				repeatedCharacter = 0;

			lastChar = text[size];
		}
		else
		{
			if(lenBeforeSpace <= 1 || size == textLength - 1 || text[size + 1] == 32)
				return false;

			lenBeforeSpace = /*lenBeforeQuote = */lenBeforeDash = 0; //Elf
		}

		if(!(isLowercaseLetter(text[size]) || text[size] == 32/* || text[size] == 39*/ || text[size] == 45
			|| (isUppercaseLetter(text[size]) && text[size - 1] == 32)))
			return false;
	}
	return true;
}

bool isNumbers(std::string text)
{
	uint32_t textLength = text.length();
	for(uint32_t size = 0; size < textLength; size++)
	{
		if(!isNumber(text[size]))
			return false;
	}
	return true;
}

bool checkText(std::string text, std::string str)
{
	trimString(text);
	return asLowerCaseString(text) == str;
}

std::string generateRecoveryKey(int32_t fieldCount, int32_t fieldLenght)
{
	std::stringstream key;
	int32_t i = 0, j = 0, lastNumber = 99, number = 0;
	char character = 0, lastCharacter = 0;
	bool madeNumber = false, madeCharacter = false;
	do
	{
		do
		{
			madeNumber = madeCharacter = false;
			if((bool)random_range(0, 1))
			{
				number = random_range(2, 9);
				if(number != lastNumber)
				{
					key << number;
					lastNumber = number;
					madeNumber = true;
				}
			}
			else
			{
				character = (char)random_range(65, 90);
				if(character != lastCharacter)
				{
					key << character;
					lastCharacter = character;
					madeCharacter = true;
				}
			}
		}
		while((!madeCharacter && !madeNumber) ? true : (++j && j < fieldLenght));

		lastCharacter = character = number = j = 0;
		lastNumber = 99;
		if(i < fieldCount - 1)
			key << "-";
	}
	while(++i && i < fieldCount);

	return key.str();
}

std::string trimString(std::string& str)
{
	str.erase(str.find_last_not_of(" ") + 1);
	return str.erase(0, str.find_first_not_of(" "));
}

std::string parseParams(tokenizer::iterator &it, tokenizer::iterator end)
{
	if(it == end)
		return "";

	std::string tmp = (*it);
	++it;
	if(tmp[0] == '"')
	{
		tmp.erase(0, 1);
		while(it != end && tmp[tmp.length() - 1] != '"')
		{
			tmp += " " + (*it);
			++it;
		}
		if(tmp.length() > 0 && tmp[tmp.length() - 1] == '"')
			tmp.erase(tmp.length() - 1);
	}
	return tmp;
}

void formatIP(uint32_t ip, char* buffer/* atleast 17 */)
{
	sprintf(buffer, "%d.%d.%d.%d", ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24));
}

void formatDate(time_t time, char* buffer/* atleast 21 */)
{
	const tm* tms = localtime(&time);
	if(tms)
		sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d", tms->tm_mday, tms->tm_mon + 1, tms->tm_year + 1900, tms->tm_hour, tms->tm_min, tms->tm_sec);
	else
		sprintf(buffer, "UNIX Time: %d", (int32_t)time);
}

void formatDate2(time_t time, char* buffer/* atleast 16 */)
{
	const tm* tms = localtime(&time);
	if(tms)
		strftime(buffer, 12, "%d %b %Y", tms);
	else
		sprintf(buffer, "UNIX Time: %d", (int32_t)time);
}

Skulls_t getSkull(std::string strValue)
{
	Skulls_t skull = SKULL_NONE;
	std::string tmpStrValue = asLowerCaseString(strValue);
	if(tmpStrValue == "red" || tmpStrValue == "4")
		skull = SKULL_RED;
	else if(tmpStrValue == "white" || tmpStrValue == "3")
		skull = SKULL_WHITE;
	else if(tmpStrValue == "green" || tmpStrValue == "2")
		skull = SKULL_GREEN;
	else if(tmpStrValue == "yellow" || tmpStrValue == "1")
		skull = SKULL_YELLOW;

	return skull;
}

PartyShields_t getPartyShield(std::string strValue)
{
	PartyShields_t partyShield = SHIELD_NONE;
	std::string tmpStrValue = asLowerCaseString(strValue);
	if(tmpStrValue == "whitenoshareoff" || tmpStrValue == "10")
		partyShield = SHIELD_YELLOW_NOSHAREDEXP;
	else if(tmpStrValue == "blueshareoff" || tmpStrValue == "9")
		partyShield = SHIELD_BLUE_NOSHAREDEXP;
	else if(tmpStrValue == "yellowshareblink" || tmpStrValue == "8")
		partyShield = SHIELD_YELLOW_NOSHAREDEXP_BLINK;
	else if(tmpStrValue == "blueshareblink" || tmpStrValue == "7")
		partyShield = SHIELD_BLUE_NOSHAREDEXP_BLINK;
	else if(tmpStrValue == "yellowshareon" || tmpStrValue == "6")
		partyShield = SHIELD_YELLOW_SHAREDEXP;
	else if(tmpStrValue == "blueshareon" || tmpStrValue == "5")
		partyShield = SHIELD_BLUE_SHAREDEXP;
	else if(tmpStrValue == "yellow" || tmpStrValue == "4")
		partyShield = SHIELD_YELLOW;
	else if(tmpStrValue == "blue" || tmpStrValue == "3")
		partyShield = SHIELD_BLUE;
	else if(tmpStrValue == "whiteyellow" || tmpStrValue == "2")
		partyShield = SHIELD_WHITEYELLOW;
	else if(tmpStrValue == "whiteblue" || tmpStrValue == "1")
		partyShield = SHIELD_WHITEBLUE;

	return partyShield;
}

Direction getDirection(std::string string)
{
	Direction direction = SOUTH;
	if(string == "north" || string == "n" || string == "0")
		direction = NORTH;
	else if(string == "east" || string == "e" || string == "1")
		direction = EAST;
	else if(string == "south" || string == "s" || string == "2")
		direction = SOUTH;
	else if(string == "west" || string == "w" || string == "3")
		direction = WEST;
	else if(string == "southwest" || string == "south west" || string == "south-west" || string == "sw" || string == "4")
		direction = SOUTHWEST;
	else if(string == "southeast" || string == "south east" || string == "south-east" || string == "se" || string == "5")
		direction = SOUTHEAST;
	else if(string == "northwest" || string == "north west" || string == "north-west" || string == "nw" || string == "6")
		direction = NORTHWEST;
	else if(string == "northeast" || string == "north east" || string == "north-east" || string == "ne" || string == "7")
		direction = NORTHEAST;

	return direction;
}

Direction getDirectionTo(Position pos1, Position pos2, bool extended/* = true*/)
{
	Direction direction = NORTH;
	if(pos1.x > pos2.x)
	{
		direction = WEST;
		if(extended)
		{
			if(pos1.y > pos2.y)
				direction = NORTHWEST;
			else if(pos1.y < pos2.y)
				direction = SOUTHWEST;
		}
	}
	else if(pos1.x < pos2.x)
	{
		direction = EAST;
		if(extended)
		{
			if(pos1.y > pos2.y)
				direction = NORTHEAST;
			else if(pos1.y < pos2.y)
				direction = SOUTHEAST;
		}
	}
	else
	{
		if(pos1.y > pos2.y)
			direction = NORTH;
		else if(pos1.y < pos2.y)
			direction = SOUTH;
	}

	return direction;
}

Direction getReverseDirection(Direction dir)
{
	Direction _dir = SOUTH;
	switch(dir)
	{
		case NORTH:
			_dir = SOUTH;
			break;
		case SOUTH:
			_dir = NORTH;
			break;
		case WEST:
			_dir = EAST;
			break;
		case EAST:
			_dir = WEST;
			break;
		case SOUTHWEST:
			_dir = NORTHEAST;
			break;
		case NORTHWEST:
			_dir = SOUTHEAST;
			break;
		case NORTHEAST:
			_dir = SOUTHWEST;
			break;
		case SOUTHEAST:
			_dir = NORTHWEST;
			break;
	}

	return _dir;
}

Position getNextPosition(Direction direction, Position pos)
{
	switch(direction)
	{
		case NORTH:
			pos.y--;
			break;
		case SOUTH:
			pos.y++;
			break;
		case WEST:
			pos.x--;
			break;
		case EAST:
			pos.x++;
			break;
		case SOUTHWEST:
			pos.x--;
			pos.y++;
			break;
		case NORTHWEST:
			pos.x--;
			pos.y--;
			break;
		case SOUTHEAST:
			pos.x++;
			pos.y++;
			break;
		case NORTHEAST:
			pos.x++;
			pos.y--;
			break;
	}

	return pos;
}

std::string formatTime(int32_t hours, int32_t minutes)
{
	std::stringstream time;
	if(hours)
		time << hours << " " << (hours > 1 ? "hours" : "hour") << (minutes ? " and " : "");

	if(minutes)
		time << minutes << " " << (minutes > 1 ? "minutes" : "minute");

	return time.str();
}

struct AmmoTypeNames
{
	const char* name;
	Ammo_t ammoType;
};

struct MagicEffectNames
{
	const char* name;
	MagicEffectClasses effect;
};

struct ShootTypeNames
{
	const char* name;
	ShootType_t shoot;
};

struct CombatTypeNames
{
	const char* name;
	CombatType_t combat;
};

struct AmmoActionNames
{
	const char* name;
	AmmoAction_t ammoAction;
};

struct FluidTypeNames
{
	const char* name;
	FluidTypes_t fluidType;
};

MagicEffectNames magicEffectNames[] =
{
	{"redspark",		NM_ME_DRAW_BLOOD},
	{"bluebubble",		NM_ME_LOSE_ENERGY},
	{"poff",		NM_ME_POFF},
	{"yellowspark",		NM_ME_BLOCKHIT},
	{"explosionarea",	NM_ME_EXPLOSION_AREA},
	{"explosion",		NM_ME_EXPLOSION_DAMAGE},
	{"firearea",		NM_ME_FIRE_AREA},
	{"yellowbubble",	NM_ME_YELLOW_RINGS},
	{"greenbubble",		NM_ME_POISON_RINGS},
	{"blackspark",		NM_ME_HIT_AREA},
	{"teleport",		NM_ME_TELEPORT},
	{"energy",		NM_ME_ENERGY_DAMAGE},
	{"blueshimmer",		NM_ME_MAGIC_ENERGY},
	{"redshimmer",		NM_ME_MAGIC_BLOOD},
	{"greenshimmer",	NM_ME_MAGIC_POISON},
	{"fire",		NM_ME_HITBY_FIRE},
	{"greenspark",		NM_ME_POISON},
	{"mortarea",		NM_ME_MORT_AREA},
	{"greennote",		NM_ME_SOUND_GREEN},
	{"rednote",		NM_ME_SOUND_RED},
	{"poison",		NM_ME_POISON_AREA},
	{"yellownote",		NM_ME_SOUND_YELLOW},
	{"purplenote",		NM_ME_SOUND_PURPLE},
	{"bluenote",		NM_ME_SOUND_BLUE},
	{"whitenote",		NM_ME_SOUND_WHITE},
	{"bubbles",		NM_ME_BUBBLES},
	{"dice",		NM_ME_CRAPS},
	{"giftwraps",		NM_ME_GIFT_WRAPS},
	{"yellowfirework",	NM_ME_FIREWORK_YELLOW},
	{"redfirework",		NM_ME_FIREWORK_RED},
	{"bluefirework",	NM_ME_FIREWORK_BLUE},
	{"stun",		NM_ME_STUN},
	{"sleep",		NM_ME_SLEEP},
	{"watercreature",	NM_ME_WATERCREATURE},
	{"groundshaker",	NM_ME_GROUNDSHAKER},
	{"hearts",		NM_ME_HEARTS},
	{"fireattack",		NM_ME_FIREATTACK},
	{"energyarea",		NM_ME_ENERGY_AREA},
	{"smallclouds",		NM_ME_SMALLCLOUDS},
	{"holydamage",		NM_ME_HOLYDAMAGE},
	{"bigclouds",		NM_ME_BIGCLOUDS},
	{"icearea",		NM_ME_ICEAREA},
	{"icetornado",		NM_ME_ICETORNADO},
	{"iceattack",		NM_ME_ICEATTACK},
	{"stones",		NM_ME_STONES},
	{"smallplants",		NM_ME_SMALLPLANTS},
	{"carniphila",		NM_ME_CARNIPHILA},
	{"purpleenergy",	NM_ME_PURPLEENERGY},
	{"yellowenergy",	NM_ME_YELLOWENERGY},
	{"holyarea",		NM_ME_HOLYAREA},
	{"bigplants",		NM_ME_BIGPLANTS},
	{"cake",		NM_ME_CAKE},
	{"giantice",		NM_ME_GIANTICE},
	{"watersplash",		NM_ME_WATERSPLASH},
	{"plantattack",		NM_ME_PLANTATTACK},
	{"tutorialarrow",	NM_ME_TUTORIALARROW},
	{"tutorialsquare",	NM_ME_TUTORIALSQUARE}
};

ShootTypeNames shootTypeNames[] =
{
	{"spear",		NM_SHOOT_SPEAR},
	{"bolt",		NM_SHOOT_BOLT},
	{"arrow",		NM_SHOOT_ARROW},
	{"fire",		NM_SHOOT_FIRE},
	{"energy",		NM_SHOOT_ENERGY},
	{"poisonarrow",		NM_SHOOT_POISONARROW},
	{"burstarrow",		NM_SHOOT_BURSTARROW},
	{"throwingstar",	NM_SHOOT_THROWINGSTAR},
	{"throwingknife",	NM_SHOOT_THROWINGKNIFE},
	{"smallstone",		NM_SHOOT_SMALLSTONE},
	{"death",		NM_SHOOT_DEATH},
	{"largerock",		NM_SHOOT_LARGEROCK},
	{"snowball",		NM_SHOOT_SNOWBALL},
	{"powerbolt",		NM_SHOOT_POWERBOLT},
	{"poison",		NM_SHOOT_POISONFIELD},
	{"infernalbolt",	NM_SHOOT_INFERNALBOLT},
	{"huntingspear",	NM_SHOOT_HUNTINGSPEAR},
	{"enchantedspear",	NM_SHOOT_ENCHANTEDSPEAR},
	{"redstar",		NM_SHOOT_REDSTAR},
	{"greenstar",		NM_SHOOT_GREENSTAR},
	{"royalspear",		NM_SHOOT_ROYALSPEAR},
	{"sniperarrow",		NM_SHOOT_SNIPERARROW},
	{"onyxarrow",		NM_SHOOT_ONYXARROW},
	{"piercingbolt",	NM_SHOOT_PIERCINGBOLT},
	{"whirlwindsword",	NM_SHOOT_WHIRLWINDSWORD},
	{"whirlwindaxe",	NM_SHOOT_WHIRLWINDAXE},
	{"whirlwindclub",	NM_SHOOT_WHIRLWINDCLUB},
	{"etherealspear",	NM_SHOOT_ETHEREALSPEAR},
	{"ice",			NM_SHOOT_ICE},
	{"earth",		NM_SHOOT_EARTH},
	{"holy",		NM_SHOOT_HOLY},
	{"suddendeath",		NM_SHOOT_SUDDENDEATH},
	{"flasharrow",		NM_SHOOT_FLASHARROW},
	{"flammingarrow",	NM_SHOOT_FLAMMINGARROW},
	{"flamingarrow",	NM_SHOOT_FLAMMINGARROW},
	{"shiverarrow",		NM_SHOOT_SHIVERARROW},
	{"energyball",		NM_SHOOT_ENERGYBALL},
	{"smallice",		NM_SHOOT_SMALLICE},
	{"smallholy",		NM_SHOOT_SMALLHOLY},
	{"smallearth",		NM_SHOOT_SMALLEARTH},
	{"eartharrow",		NM_SHOOT_EARTHARROW},
	{"explosion",		NM_SHOOT_EXPLOSION},
	{"cake",		NM_SHOOT_CAKE}
};

CombatTypeNames combatTypeNames[] =
{
	{"physical",		COMBAT_PHYSICALDAMAGE},
	{"energy",		COMBAT_ENERGYDAMAGE},
	{"earth",		COMBAT_EARTHDAMAGE},
	{"fire",		COMBAT_FIREDAMAGE},
	{"undefined",		COMBAT_UNDEFINEDDAMAGE},
	{"lifedrain",		COMBAT_LIFEDRAIN},
	{"manadrain",		COMBAT_MANADRAIN},
	{"healing",		COMBAT_HEALING},
	{"drown",		COMBAT_DROWNDAMAGE},
	{"ice",			COMBAT_ICEDAMAGE},
	{"holy",		COMBAT_HOLYDAMAGE},
	{"death",		COMBAT_DEATHDAMAGE}
};

AmmoTypeNames ammoTypeNames[] =
{
	{"spear",		AMMO_SPEAR},
	{"bolt",		AMMO_BOLT},
	{"arrow",		AMMO_ARROW},
	{"poisonarrow",		AMMO_ARROW},
	{"burstarrow",		AMMO_ARROW},
	{"throwingstar",	AMMO_THROWINGSTAR},
	{"throwingknife",	AMMO_THROWINGKNIFE},
	{"smallstone",		AMMO_STONE},
	{"largerock",		AMMO_STONE},
	{"snowball",		AMMO_SNOWBALL},
	{"powerbolt",		AMMO_BOLT},
	{"infernalbolt",	AMMO_BOLT},
	{"huntingspear",	AMMO_SPEAR},
	{"enchantedspear",	AMMO_SPEAR},
	{"royalspear",		AMMO_SPEAR},
	{"sniperarrow",		AMMO_ARROW},
	{"onyxarrow",		AMMO_ARROW},
	{"piercingbolt",	AMMO_BOLT},
	{"etherealspear",	AMMO_SPEAR},
	{"flasharrow",		AMMO_ARROW},
	{"flammingarrow",	AMMO_ARROW},
	{"flamingarrow",	AMMO_ARROW},
	{"shiverarrow",		AMMO_ARROW},
	{"eartharrow",		AMMO_ARROW}
};

AmmoActionNames ammoActionNames[] =
{
	{"move",		AMMOACTION_MOVE},
	{"moveback",		AMMOACTION_MOVEBACK},
	{"removecharge",	AMMOACTION_REMOVECHARGE},
	{"removecount",		AMMOACTION_REMOVECOUNT}
};

FluidTypeNames fluidTypeNames[] =
{
	{"none",		FLUID_NONE},
	{"water",		FLUID_WATER},
	{"blood",		FLUID_BLOOD},
	{"beer",		FLUID_BEER},
	{"slime",		FLUID_SLIME},
	{"lemonade",		FLUID_LEMONADE},
	{"milk",		FLUID_MILK},
	{"mana",		FLUID_MANA},
	{"life",		FLUID_LIFE},
	{"oil",			FLUID_OIL},
	{"urine",		FLUID_URINE},
	{"coconutmilk",		FLUID_COCONUTMILK},
	{"wine",		FLUID_WINE},
	{"mud",			FLUID_MUD},
	{"fruitjuice",		FLUID_FRUITJUICE},
	{"lava",		FLUID_LAVA},
	{"rum",			FLUID_RUM},
	{"swamp",		FLUID_SWAMP}
};

MagicEffectClasses getMagicEffect(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(magicEffectNames) / sizeof(MagicEffectNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), magicEffectNames[i].name) == 0)
			return magicEffectNames[i].effect;
	}
	return NM_ME_UNK;
}

ShootType_t getShootType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(shootTypeNames) / sizeof(ShootTypeNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), shootTypeNames[i].name) == 0)
			return shootTypeNames[i].shoot;
	}
	return NM_SHOOT_UNK;
}

CombatType_t getCombatType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(combatTypeNames) / sizeof(CombatTypeNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), combatTypeNames[i].name) == 0)
			return combatTypeNames[i].combat;
	}
	return COMBAT_NONE;
}

std::string getCombatName(CombatType_t combatType)
{
	for(uint32_t i = 0; i < sizeof(combatTypeNames) / sizeof(CombatTypeNames); ++i)
	{
		if(combatTypeNames[i].combat == combatType)
			return combatTypeNames[i].name;
	}
	return "unknown";
}

Ammo_t getAmmoType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(ammoTypeNames) / sizeof(AmmoTypeNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), ammoTypeNames[i].name) == 0)
			return ammoTypeNames[i].ammoType;
	}
	return AMMO_NONE;
}

AmmoAction_t getAmmoAction(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(ammoActionNames) / sizeof(AmmoActionNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), ammoActionNames[i].name) == 0)
			return ammoActionNames[i].ammoAction;
	}
	return AMMOACTION_NONE;
}

FluidTypes_t getFluidType(const std::string& strValue)
{
	for(uint32_t i = 0; i < sizeof(fluidTypeNames) / sizeof(FluidTypeNames); ++i)
	{
		if(strcasecmp(strValue.c_str(), fluidTypeNames[i].name) == 0)
			return fluidTypeNames[i].fluidType;
	}
	return FLUID_NONE;
}

std::string getSkillName(uint16_t skillId, bool suffix/* = true*/)
{
	switch(skillId)
	{
		case SKILL_FIST:
		{
			std::string tmp = "fist";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_CLUB:
		{
			std::string tmp = "club";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_SWORD:
		{
			std::string tmp = "sword";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_AXE:
		{
			std::string tmp = "axe";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_DIST:
		{
			std::string tmp = "distance";
			if(suffix)
				tmp += " fighting";

			return tmp;
		}
		case SKILL_SHIELD:
			return "shielding";
		case SKILL_FISH:
			return "fishing";
		case SKILL__MAGLEVEL:
			return "magic level";
		case SKILL__LEVEL:
			return "level";
	}

	return "unknown";
}

skills_t getSkillId(std::string param)
{
	if(param == "fist")
		return SKILL_FIST;
	else if(param == "club")
		return SKILL_CLUB;
	else if(param == "sword")
		return SKILL_SWORD;
	else if(param == "axe")
		return SKILL_AXE;
	else if(param == "distance" || param == "dist")
		return SKILL_DIST;
	else if(param == "shielding" || param == "shield")
		return SKILL_SHIELD;
	else if(param == "fishing" || param == "fish")
		return SKILL_FISH;
	else
		return SKILL_FIST;
}

std::string getReason(int32_t reasonId)
{
	switch(reasonId)
	{
		case 0:
			return "Offensive Name";
		case 1:
			return "Invalid Name Format";
		case 2:
			return "Unsuitable Name";
		case 3:
			return "Name Inciting Rule Violation";
		case 4:
			return "Offensive Statement";
		case 5:
			return "Spamming";
		case 6:
			return "Illegal Advertising";
		case 7:
			return "Off-Topic Public Statement";
		case 8:
			return "Non-English Public Statement";
		case 9:
			return "Inciting Rule Violation";
		case 10:
			return "Bug Abuse";
		case 11:
			return "Game Weakness Abuse";
		case 12:
			return "Using Unofficial Software to Play";
		case 13:
			return "Hacking";
		case 14:
			return "Multi-Clienting";
		case 15:
			return "Account Trading or Sharing";
		case 16:
			return "Threatening Gamemaster";
		case 17:
			return "Pretending to Have Influence on Rule Enforcement";
		case 18:
			return "False Report to Gamemaster";
		case 19:
			return "Destructive Behaviour";
		case 20:
			return "Excessive Unjustified Player Killing";
		case 21:
			return "Invalid Payment";
		case 22:
			return "Spoiling Auction";
	}

	return "Unknown Reason";
}

std::string getAction(ViolationAction_t actionId, bool ipBanishment)
{
	std::string action;
	switch(actionId)
	{
		case ACTION_NOTATION:
			action = "Notation";
			break;
		case ACTION_NAMEREPORT:
			action = "Name Report";
			break;
		case ACTION_BANREPORT:
			action = "Name Report + Banishment";
			break;
		case ACTION_BANFINAL:
			action = "Banishment + Final Warning";
			break;
		case ACTION_BANREPORTFINAL:
			action = "Name Report + Banishment + Final Warning";
			break;
		case ACTION_STATEMENT:
			action = "Statement Report";
			break;
		case ACTION_DELETION:
			action = "Deletion";
			break;
		case ACTION_BANISHMENT:
		default:
			action = "Banishment";
			break;
	}

	if(ipBanishment)
		action += " + IP Banishment";

	return action;
}

std::string parseVocationString(StringVec vocStringVec)
{
	std::string str = "";
	if(!vocStringVec.empty())
	{
		for(StringVec::iterator it = vocStringVec.begin(); it != vocStringVec.end(); ++it)
		{
			if((*it) != vocStringVec.front())
			{
				if((*it) != vocStringVec.back())
					str += ", ";
				else
					str += " and ";
			}

			str += (*it);
			str += "s";
		}
	}

	return str;
}

bool parseVocationNode(xmlNodePtr vocationNode, VocationMap& vocationMap, StringVec& vocStringVec, std::string& errorStr)
{
	int32_t intValue;
	std::string strValue, tmpStrValue;
	if(!xmlStrcmp(vocationNode->name,(const xmlChar*)"vocation"))
	{
		int32_t vocationId = -1;
		if(readXMLString(vocationNode, "name", strValue))
		{
			vocationId = Vocations::getInstance()->getVocationId(strValue);
			if(vocationId != -1)
			{
				vocationMap[vocationId] = true;
				int32_t promotedVocation = Vocations::getInstance()->getPromotedVocation(vocationId);
				if(promotedVocation != -1)
					vocationMap[promotedVocation] = true;
			}
			else
			{
				errorStr += "Wrong vocation name: " + strValue;
				return false;
			}
		}
		else if(readXMLInteger(vocationNode, "id", intValue))
		{
			Vocation* vocation = Vocations::getInstance()->getVocation(intValue);
			if(vocation && vocation->getName() != "")
			{
				vocationId = vocation->getId();
				strValue = vocation->getName();

				vocationMap[vocationId] = true;
				int32_t promotedVocation = Vocations::getInstance()->getPromotedVocation(vocationId);
				if(promotedVocation != -1)
					vocationMap[promotedVocation] = true;
			}
			else
			{
				std::stringstream ss;
				ss << "Wrong vocation id: " << intValue;

				errorStr += ss.str();
				return false;
			}
		}

		if(vocationId != -1 && (!readXMLString(vocationNode, "showInDescription", tmpStrValue) || booleanString(tmpStrValue)))
			vocStringVec.push_back(asLowerCaseString(strValue));
	}

	return true;
}

bool fileExists(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if(!f)
		return false;

	fclose(f);
	return true;
}

uint32_t adlerChecksum(uint8_t *data, size_t length)
{
	if(length > NETWORKMESSAGE_MAXSIZE || length < 0)
		return 0;

	const uint16_t adler = 65521;
	uint32_t a = 1, b = 0;
	while(length > 0)
	{
		size_t tmp = length > 5552 ? 5552 : length;
		length -= tmp;
		do
		{
			a += *data++;
			b += a;
		}
		while(--tmp);

		a %= adler;
		b %= adler;
	}

	return (b << 16) | a;
}

std::string getFilePath(FileType_t filetype, std::string filename)
{
	std::string path = "";
	#ifdef __FILESYSTEM_HIERARCHY_STANDARD__
	path = "/usr/share/tfs/";
	#endif
	path += g_config.getString(ConfigManager::DATA_DIRECTORY);
	switch(filetype)
	{
		case FILE_TYPE_XML:
			path += "XML/" + filename;
			break;
		case FILE_TYPE_LOG:
			#ifndef __FILESYSTEM_HIERARCHY_STANDARD__
			path += "logs/" + filename;
			#else
			path = "/var/log/tfs/" + filename;
			#endif
			break;
		case FILE_TYPE_OTHER:
			path += filename;
			break;
		case FILE_TYPE_CONFIG:
		{
			#if defined(__FILESYSTEM_HIERARCHY_STANDARD__) && defined(__HOMEDIR_CONF__)
			if(fileExists("~/.tfs/" + filename))
				path = "~/.tfs/" + filename;
			else
				path = "/etc/tfs/" + filename;
			#elif defined(__FILESYSTEM_HIERARCHY_STANDARD__)
				path = "/etc/tfs/" + filename;
			#else
				path = filename;
			#endif

			break;
		}
		default:
			std::cout << "ERROR: Wrong file type!" << std::endl;
			break;
	}
	return path;
}
