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

#ifndef __DATABASE_MYSQL_H__
#define __DATABASE_MYSQL_H__
#ifdef __USE_MYSQL__
#include "otsystem.h"

#ifdef WIN32
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif
#include "database.h"

class DatabaseMySQL : public _Database
{
	public:
		DatabaseMySQL();
		virtual ~DatabaseMySQL();

		/** Connect to a mysql DatabaseMySQL
		*\returns
		* 	TRUE if the connection is ok
		* 	FALSE if the connection fails
		*/
		virtual bool connect();

		/** Disconnects from the connected DatabaseMySQL
		*\returns
		* 	TRUE if the DatabaseMySQL was disconnected
		* 	FALSE if the DatabaseMySQL was not disconnected or no DatabaseMySQL selected
		*/
		virtual bool disconnect();

		/** Execute a query which don't get any information of the DatabaseMySQL (for ex.: INSERT, UPDATE, etc)
		*\returns
		* 	TRUE if the query is ok
		* 	FALSE if the query fails
		*\ref q The query object
		*/
		virtual bool executeQuery(DBQuery &q);

		/** Store a query which get information of the DatabaseMySQL (for ex.: SELECT)
		*\returns
		* 	TRUE if the query is ok
		* 	FALSE if the query fails
		*\ref q The query object
		*\ref res The DBResult object where to insert the results of the query
		*/
		virtual bool storeQuery(DBQuery &q, DBResult &res);

		virtual bool rollback();
		virtual bool commit();

		virtual std::string escapeBlob(const char* s, uint32_t length);

	private:
		/** initialize the DatabaseMySQL
		*\returns
		* 	TRUE if the DatabaseMySQL was successfully initialized
		* 	FALSE if the DatabaseMySQL was not successfully initialized
		*/
		bool init();

		bool m_initialized;
		MYSQL m_handle;
};

#endif
#endif
