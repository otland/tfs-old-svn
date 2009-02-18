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
#include "otpch.h"

#include <string>
#include <iostream>

#include "networkmessage.h"

#include "container.h"
#include "creature.h"
#include "player.h"

#include "position.h"
#include "rsa.h"

int32_t NetworkMessage::decodeHeader()
{
	int32_t size = (int32_t)(m_MsgBuf[0] | m_MsgBuf[1] << 8);
	m_MsgSize = size;
	return size;
}

/******************************************************************************/
std::string NetworkMessage::GetString()
{
	uint16_t stringLen = GetU16();
	if(stringLen >= (NETWORKMESSAGE_MAXSIZE - m_ReadPos))
		return std::string();

	char* v = (char*)(m_MsgBuf + m_ReadPos);
	m_ReadPos += stringLen;
	return std::string(v, stringLen);
}

std::string NetworkMessage::GetRaw()
{
	uint16_t stringLen = (m_MsgSize - m_ReadPos);
	if(stringLen >= (NETWORKMESSAGE_MAXSIZE - m_ReadPos))
		return std::string();

	char* v = (char*)(m_MsgBuf + m_ReadPos);
	m_ReadPos += stringLen;
	return std::string(v, stringLen);
}

Position NetworkMessage::GetPosition()
{
	Position pos;
	pos.x = GetU16();
	pos.y = GetU16();
	pos.z = GetByte();
	return pos;
}
/******************************************************************************/

void NetworkMessage::AddString(const char* value)
{
	uint32_t stringLen = (uint32_t)strlen(value);
	if(!canAdd(stringLen + 2) || stringLen > 8192)
		return;

	AddU16(stringLen);
	strcpy((char*)(m_MsgBuf + m_ReadPos), value);
	m_ReadPos += stringLen;
	m_MsgSize += stringLen;
}

void NetworkMessage::AddBytes(const char* bytes, uint32_t size)
{
	if(!canAdd(size) || size > 8192)
		return;

	memcpy(m_MsgBuf + m_ReadPos, bytes, size);
	m_ReadPos += size;
	m_MsgSize += size;
}

void NetworkMessage::AddPaddingBytes(uint32_t n)
{
	if(!canAdd(n))
		return;

	memset((void*)&m_MsgBuf[m_ReadPos], 0x33, n);
	m_MsgSize = m_MsgSize + n;
}

void NetworkMessage::AddPosition(const Position& pos)
{
	AddU16(pos.x);
	AddU16(pos.y);
	AddByte(pos.z);
}

void NetworkMessage::AddItem(uint16_t id, uint8_t count)
{
	const ItemType &it = Item::items[id];
	AddU16(it.clientId);
	if(it.stackable || it.isRune())
		AddByte(count);
	else if(it.isSplash() || it.isFluidContainer())
	{
		uint32_t fluidIndex = (count % 8);
		AddByte(fluidMap[fluidIndex]);
	}
}

void NetworkMessage::AddItem(const Item* item)
{
	const ItemType &it = Item::items[item->getID()];
	AddU16(it.clientId);
	if(it.stackable || it.isRune())
		AddByte(item->getSubType());
	else if(it.isSplash() || it.isFluidContainer())
		AddByte(fluidMap[item->getSubType() % 8]);
}

void NetworkMessage::AddItemId(const Item *item)
{
	const ItemType &it = Item::items[item->getID()];
	AddU16(it.clientId);
}

void NetworkMessage::AddItemId(uint16_t itemId)
{
	const ItemType &it = Item::items[itemId];
	AddU16(it.clientId);
}
