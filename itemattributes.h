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

#ifndef __ITEM_ATTRIBUTES__
#define __ITEM_ATTRIBUTES__
#include "otsystem.h"
#include <boost/any.hpp>

class PropWriteStream;
class PropStream;

class ItemAttribute
{
	public:
		ItemAttribute(): type(ItemAttribute::NONE) {}
		ItemAttribute(const ItemAttribute& o): type(ItemAttribute::NONE) {*this = o;}
		virtual ~ItemAttribute() {clear();}

		ItemAttribute(const std::string& s): type(ItemAttribute::STRING) {new(data) std::string(s);}
		ItemAttribute(int32_t i): type(ItemAttribute::INTEGER) {*reinterpret_cast<int32_t*>(data) = i;}
		ItemAttribute(float f): type(ItemAttribute::FLOAT) {*reinterpret_cast<float*>(data) = f;}
		ItemAttribute(bool b): type(ItemAttribute::BOOLEAN) {*reinterpret_cast<bool*>(data) = b;}

		ItemAttribute& operator=(const ItemAttribute& o);

		void serialize(PropWriteStream& stream) const;
		bool unserialize(PropStream& stream);

		void clear();

		void set(const std::string& s);
		void set(int32_t i);
		void set(float f);
		void set(bool b);
		void set(boost::any a);

		const std::string* getString() const;
		const int32_t* getInteger() const;
		const float* getFloat() const;
		const bool* getBoolean() const;
		boost::any get() const;

	private:
		char data[sizeof(std::string)];
		enum Type
		{
			NONE = 0,
			STRING = 1,
			INTEGER = 2,
			FLOAT = 3,
			BOOLEAN = 4
		} type;
};

class ItemAttributes
{
	public:
		ItemAttributes(): attributes(NULL) {}
		ItemAttributes(const ItemAttributes &i);
		virtual ~ItemAttributes() {delete attributes;}

		void serializeMap(PropWriteStream& stream) const;
		bool unserializeMap(PropStream& stream);

		void eraseAttribute(const char* key);
		void setAttribute(const char* key, boost::any value);
		boost::any getAttribute(const char* key) const;

		void setAttribute(const char* key, const std::string& value);
		void setAttribute(const char* key, int32_t value);
		void setAttribute(const char* key, float value);
		void setAttribute(const char* key, bool value);

		const std::string* getStringAttribute(const char* key) const;
		const int32_t* getIntegerAttribute(const char* key) const;
		const float* getFloatAttribute(const char* key) const;
		const bool* getBooleanAttribute(const char* key) const;

		bool hasStringAttribute(const char* key) const {return getStringAttribute(key) != NULL;}
		bool hasIntegerAttribute(const char* key) const {return getIntegerAttribute(key) != NULL;}
		bool hasFloatAttribute(const char* key) const {return getFloatAttribute(key) != NULL;}
		bool hasBooleanAttribute(const char* key) const {return getBooleanAttribute(key) != NULL;}

	protected:
		void createAttributes();

		typedef std::map<std::string, ItemAttribute> AttributeMap;
		AttributeMap* attributes;
};

#endif
