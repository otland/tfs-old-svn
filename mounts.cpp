//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Mounts
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

#include "mounts.h"
#include "tools.h"

Mount::Mount(uint8_t _id, uint16_t _clientId, std::string _name, int32_t _speed)
{
	id = _id;
	clientId = _clientId;
	name = _name;
	speed = _speed;
}

bool Mount::isTamed(Player* player) const
{
	if(!player)
		return false;

	if(player->isAccessPlayer())
		return true;

	uint8_t tmpId = id - 1;

	int32_t value = 0;
	if(!player->getStorageValue(PSTRG_MOUNTS_RANGE_START + (tmpId / 31), value))
		return false;

	int32_t tmp = (int32_t)pow(2, tmpId % 31);
	return (tmp & value) == tmp;
}

Mounts::~Mounts()
{
	for(MountsList::iterator it = mounts.begin(); it != mounts.end(); it++)
		delete (*it);

	mounts.clear();
}

bool Mounts::reload()
{
	for(MountsList::iterator it = mounts.begin(); it != mounts.end(); it++)
		delete (*it);

	mounts.clear();
	return loadFromXml();
}

bool Mounts::loadFromXml()
{
	xmlDocPtr doc = xmlParseFile("data/XML/mounts.xml");
	if(!doc)
		return false;

	xmlNodePtr root, p;
	root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"mounts") != 0)
	{
		xmlFreeDoc(doc);
		return false;
	}

	int32_t intValue;
	std::string strValue;
	p = root->children;
	while(p)
	{
		if(xmlStrcmp(p->name, (const xmlChar*)"mount") == 0)
		{
			int8_t id = 0;
			int16_t clientid = 0;
			std::string name = "";
			int32_t speed = 0;
			if(readXMLInteger(p, "id", intValue))
				id = intValue;

			if(readXMLInteger(p, "clientid", intValue))
				clientid = intValue;

			if(readXMLString(p, "name", strValue))
				name = strValue;

			if(readXMLInteger(p, "speed", intValue))
				speed = intValue;

			mounts.push_back(new Mount(id, clientid, name, speed));
		}
		p = p->next;
	}
	xmlFreeDoc(doc);
	return true;
}

Mount* Mounts::getMountByID(uint8_t id)
{
	for(MountsList::iterator it = mounts.begin(); it != mounts.end(); it++)
	{
		if((*it)->getID() == id)
			return (*it);
	}
	return NULL;
}

Mount* Mounts::getMountByClientID(uint16_t clientId)
{
	for(MountsList::iterator it = mounts.begin(); it != mounts.end(); it++)
	{
		if((*it)->getClientID() == clientId)
			return (*it);
	}
	return NULL;
}

void Mounts::sendMountsList(Player* player, NetworkMessage_ptr msg)
{
	MountsList tmp_list;
	for(MountsList::const_iterator it = mounts.begin(); it != mounts.end(); it++)
	{
		if((*it)->isTamed(player))
			tmp_list.push_back(*it);
	}

	msg->AddByte(tmp_list.size());
	for(MountsList::const_iterator it = tmp_list.begin(); it != tmp_list.end(); it++)
	{
		msg->AddU16((*it)->getClientID());
		msg->AddString((*it)->getName());
	}
}
