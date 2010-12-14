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

#include "itemattributes.h"
#include "fileloader.h"

ItemAttributes::ItemAttributes(const ItemAttributes& o)
{
	if(o.attributes)
		attributes = new AttributeMap(*o.attributes);
}

void ItemAttributes::createAttributes()
{
	if(!attributes)
		attributes = new AttributeMap;
}

void ItemAttributes::eraseAttribute(const std::string& key)
{
	if(!attributes)
		return;

	AttributeMap::iterator it = attributes->find(key);
	if(it != attributes->end())
		attributes->erase(it);
}

void ItemAttributes::setAttribute(const std::string& key, boost::any value)
{
	createAttributes();
	(*attributes)[key].set(value);
}

boost::any ItemAttributes::getAttribute(const std::string& key) const
{
	if(!attributes)
		return boost::any();

	AttributeMap::iterator it = attributes->find(key);
	if(it != attributes->end())
		return it->second.get();

	return boost::any();
}

void ItemAttributes::setAttribute(const std::string& key, const std::string& value)
{
	createAttributes();
	(*attributes)[key].set(value);
}

void ItemAttributes::setAttribute(const std::string& key, int32_t value)
{
	createAttributes();
	(*attributes)[key].set(value);
}

void ItemAttributes::setAttribute(const std::string& key, float value)
{
	createAttributes();
	(*attributes)[key].set(value);
}

void ItemAttributes::setAttribute(const std::string& key, bool value)
{
	createAttributes();
	(*attributes)[key].set(value);
}

const std::string* ItemAttributes::getStringAttribute(const std::string& key) const
{
	if(!attributes)
		return NULL;

	AttributeMap::iterator it = attributes->find(key);
	if(it != attributes->end())
		return it->second.getString();

	return NULL;
}

const int32_t* ItemAttributes::getIntegerAttribute(const std::string& key) const
{
	if(!attributes)
		return NULL;

	AttributeMap::iterator it = attributes->find(key);
	if(it != attributes->end())
		return it->second.getInteger();

	return NULL;
}

const float* ItemAttributes::getFloatAttribute(const std::string& key) const
{
	if(!attributes)
		return NULL;

	AttributeMap::iterator it = attributes->find(key);
	if(it != attributes->end())
		return it->second.getFloat();

	return NULL;
}

const bool* ItemAttributes::getBooleanAttribute(const std::string& key) const
{
	if(!attributes)
		return NULL;

	AttributeMap::iterator it = attributes->find(key);
	if(it != attributes->end())
		return it->second.getBoolean();

	return NULL;
}

ItemAttribute& ItemAttribute::operator=(const ItemAttribute& o)
{
	if(&o == this)
		return *this;

	clear();
	type = o.type;
	switch(type)
	{
		case STRING:
			new(data) std::string(*reinterpret_cast<const std::string*>(&o.data));
			break;
		case INTEGER:
			*reinterpret_cast<int32_t*>(data) = *reinterpret_cast<const int32_t*>(&o.data);
			break;
		case FLOAT:
			*reinterpret_cast<float*>(data) = *reinterpret_cast<const float*>(&o.data);
			break;
		case BOOLEAN:
			*reinterpret_cast<bool*>(data) = *reinterpret_cast<const bool*>(&o.data);
			break;
		default:
			type = NONE;
			break;
	}

	return *this;

}

void ItemAttribute::clear()
{
	type = NONE;
	if(type == STRING)
		(reinterpret_cast<std::string*>(&data))->~basic_string();
}

void ItemAttribute::set(const std::string& s)
{
	clear();
	type = STRING;
	new(data) std::string(s);
}

void ItemAttribute::set(int32_t i)
{
	clear();
	type = INTEGER;
	*reinterpret_cast<int32_t*>(&data) = i;
}

void ItemAttribute::set(float f)
{
	clear();
	type = FLOAT;
	*reinterpret_cast<float*>(&data) = f;
}

void ItemAttribute::set(bool b)
{
	clear();
	type = BOOLEAN;
	*reinterpret_cast<bool*>(&data) = b;
}

void ItemAttribute::set(boost::any a)
{
	clear();
	if(a.empty())
		return;

	if(a.type() == typeid(std::string))
	{
		type = STRING;
		new(data) std::string(boost::any_cast<std::string>(a));
	}
	else if(a.type() == typeid(int32_t))
	{
		type = INTEGER;
		*reinterpret_cast<int32_t*>(&data) = boost::any_cast<int32_t>(a);
	}
	else if(a.type() == typeid(float))
	{
		type = FLOAT;
		*reinterpret_cast<float*>(&data) = boost::any_cast<float>(a);
	}
	else if(a.type() == typeid(bool))
	{
		type = BOOLEAN;
		*reinterpret_cast<bool*>(&data) = boost::any_cast<bool>(a);
	}
}

const std::string* ItemAttribute::getString() const
{
	if(type != STRING)
		return NULL;

	return reinterpret_cast<const std::string*>(&data);
}

const int32_t* ItemAttribute::getInteger() const
{
	if(type != INTEGER)
		return NULL;

	return reinterpret_cast<const int32_t*>(&data);
}

const float* ItemAttribute::getFloat() const
{
	if(type != FLOAT)
		return NULL;

	return reinterpret_cast<const float*>(&data);
}

const bool* ItemAttribute::getBoolean() const
{
	if(type != BOOLEAN)
		return NULL;

	return reinterpret_cast<const bool*>(&data);
}

boost::any ItemAttribute::get() const
{
	switch(type)
	{
		case STRING:
			return *reinterpret_cast<const std::string*>(&data);
		case INTEGER:
			return *reinterpret_cast<const int*>(&data);
		case FLOAT:
			return *reinterpret_cast<const float*>(&data);
		case BOOLEAN:
			return *reinterpret_cast<const bool*>(&data);
		default:
			break;
	}

	return boost::any();
}

bool ItemAttributes::unserializeMap(PropStream& stream)
{
	uint16_t n;
	if(!stream.getShort(n))
		return true;

	createAttributes();
	while(n--)
	{
		std::string key;
		if(!stream.getString(key))
			return false;

		ItemAttribute attr;
		if(!attr.unserialize(stream))
			return false;

		(*attributes)[key] = attr;
	}

	return true;
}

void ItemAttributes::serializeMap(PropWriteStream& stream) const
{
	stream.addShort((uint16_t)std::min((size_t)0xFFFF, attributes->size()));
	AttributeMap::const_iterator it = attributes->begin();
	for(int32_t i = 0; it != attributes->end() && i <= 0xFFFF; ++it, ++i)
	{
		std::string key = it->first;
		if(key.size() > 0xFFFF)
			stream.addString(key.substr(0, 0xFFFF));
		else
			stream.addString(key);

		it->second.serialize(stream);
	}
}

bool ItemAttribute::unserialize(PropStream& stream)
{
	uint8_t type = 0;
	stream.getByte(type);
	switch(type)
	{
		case STRING:
		{
			std::string v;
			if(!stream.getLongString(v))
				return false;

			set(v);
			break;
		}
		case INTEGER:
		{
			uint32_t v;
			if(!stream.getLong(v))
				return false;

			set(*reinterpret_cast<int32_t*>(&v));
			break;
		}
		case FLOAT:
		{
			float v;
			if(!stream.getFloat(v))
				return false;

			set(*reinterpret_cast<float*>(&v));
			break;
		}
		case BOOLEAN:
		{
			uint8_t v;
			if(!stream.getByte(v))
				return false;

			set(v != 0);
		}
		default:
			break;
	}

	return true;
}

void ItemAttribute::serialize(PropWriteStream& stream) const
{
	stream.addByte((uint8_t)type);
	switch(type)
	{
		case STRING:
			stream.addLongString(*getString());
			break;
		case INTEGER:
			stream.addLong(*(uint32_t*)getInteger());
			break;
		case FLOAT:
			stream.addLong(*(uint32_t*)getFloat());
			break;
		case BOOLEAN:
			stream.addByte(*(uint8_t*)getBoolean());
		default:
			break;
	}
}
