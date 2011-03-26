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
#include "mounts.h"

bool Mount::isTamed(Player* player) const
{
	if(!player)
		return false;

	if(premium && !player->isPremium())
		return false;

	if(player->hasCustomFlag(PlayerCustomFlag_CanUseAllMounts))
		return true;

	uint8_t tmpId = id - 1;
	std::string value;

	int32_t key = PSTRG_MOUNTS_RANGE_START + (tmpId / 31);
	if(player->getStorage(boost::lexical_cast<std::string>(key), value))
	{
		int32_t tmp = (int32_t)std::pow(2., tmpId % 31);
		return (tmp & atoi(value.c_str())) == tmp;
	}

	if(storageId.empty())
		return false;

	player->getStorage(storageId, value);
	if(value == storageValue)
		return true;

	int32_t intValue = atoi(value.c_str());
	if(!intValue && value != "0")
		return false;

	int32_t tmp = atoi(storageValue.c_str());
	if(!tmp && storageValue != "0")
		return false;

	return intValue >= tmp;
}

void Mounts::clear()
{
	for(MountList::iterator it = mounts.begin(); it != mounts.end(); it++)
		delete *it;

	mounts.clear();
}

bool Mounts::reload()
{
	clear();
	return loadFromXml();
}

bool Mounts::loadFromXml()
{
	xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_XML, "mounts.xml").c_str());
	if(!doc)
	{
		std::clog << "[Warning - Mounts::loadFromXml] Cannot load mounts file." << std::endl;
		std::clog << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr p, root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"mounts"))
	{
		std::clog << "[Error - Mounts::loadFromXml] Malformed mounts file." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	p = root->children;
	while(p)
	{
		parseMountNode(p);
		p = p->next;
	}

	xmlFreeDoc(doc);
	return true;
}

bool Mounts::parseMountNode(xmlNodePtr p)
{
	if(xmlStrcmp(p->name, (const xmlChar*)"mount"))
		return false;

	int32_t intValue;
	std::string strValue;

	uint8_t mountId = 0;
	if(readXMLInteger(p, "id", intValue))
		mountId = intValue;

	std::string name;
	if(readXMLString(p, "name", strValue))
		name = strValue;

	uint16_t clientId = 0;
	if(readXMLInteger(p, "clientid", intValue) || readXMLInteger(p, "clientId", intValue) || readXMLInteger(p, "cid", intValue))
		clientId = intValue;

	int32_t speed = 0;
	if(readXMLInteger(p, "speed", intValue))
		speed = intValue;

	bool premium = true;
	if(readXMLString(p, "premium", strValue))
		premium = booleanString(strValue);

	std::string storageId, storageValue;
	if(readXMLString(p, "quest", strValue))
	{
		storageId = strValue;
		storageValue = "1";
	}
	else
	{
		if(readXMLString(p, "storageId", strValue))
			storageId = strValue;

		if(readXMLString(p, "storageValue", strValue))
			storageValue = strValue;
	}

	if(!mountId)
	{
		std::clog << "[Error - Mounts::parseMountNode] Entry without mountId" << std::endl;
		return false;
	}

	if(!clientId)
	{
		std::clog << "[Error - Mounts::parseMountNode] Entry without clientId" << std::endl;
		return false;
	}

	Mount* mount = new Mount(name, mountId, clientId,
		speed, premium, storageId, storageValue);
	if(!mount)
		return false;

	mounts.push_back(mount);
	return true;
}

Mount* Mounts::getMountById(uint16_t id) const
{
	if(!id)
		return NULL;

	for(MountList::const_iterator it = mounts.begin(); it != mounts.end(); it++)
	{
		if((*it)->getId() == id)
			return (*it);
	}

	return NULL;
}

Mount* Mounts::getMountByCid(uint16_t id) const
{
	if(!id)
		return NULL;

	for(MountList::const_iterator it = mounts.begin(); it != mounts.end(); it++)
	{
		if((*it)->getClientId() == id)
			return (*it);
	}

	return NULL;
}

bool Mounts::isPremium() const
{
	for(MountList::const_iterator it = mounts.begin(); it != mounts.end(); it++)
	{
		if(!(*it)->isPremium())
			return false;
	}

	return true;
}