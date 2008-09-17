//////////////////////////////////////////////////////////////////////
// TheForgottenServer - SQLUpdater - Developed by Talaturen
//////////////////////////////////////////////////////////////////////
// Updater
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

/*
 * Include cstdio, used for printf, FILE, scanf.
 */
#include <cstdio>

/*
 * Include cstdlib, used for free and exit.
 */
#include <cstdlib>

/*
 * Include cstring, used for strcasecmp and
 * strlen.
 */
#include <cstring>

/*
 * Include for compiling on windows.
 */
#ifdef WIN32
#include <windows.h>
#endif

/*
 * Include MySQL header.
 */
#include <mysql/mysql.h>

/*
 * Include SqLite3 header.
 */
#include <sqlite3.h>

/*
 * Created for version of TheForgottenServer.
 */
char version[6] = "0.3";

/*
 * MySQL Queries array, put all MySQL queries
 * to execute inside it.
 */
const char* mysqlQueries[] = 
{
	"CREATE TABLE `server_motd` (`id` INT NOT NULL AUTO_INCREMENT, `text` TEXT NOT NULL, PRIMARY KEY (`id`)) ENGINE = InnoDB;",
	"CREATE TABLE `server_record` (`record` INT NOT NULL, `timestamp` BIGINT NOT NULL, PRIMARY KEY (`timestamp`)) ENGINE = InnoDB;",
	"CREATE TABLE `global_storage` (`key` INT UNSIGNED NOT NULL, `value` INT NOT NULL, PRIMARY KEY  (`key`)) ENGINE = InnoDB;",
	"INSERT INTO `server_record` VALUES (0, 0);",
	"INSERT INTO `server_motd` VALUES (1, 'Welcome to The Forgotten Server!');",
	"DROP TABLE `bans`;",
	"CREATE TABLE `bans` (`id` INT UNSIGNED NOT NULL auto_increment, `type` TINYINT(1) NOT NULL COMMENT 'this field defines if its ip, account, player, or any else ban', `value` INT UNSIGNED NOT NULL COMMENT 'ip, player guid, account number', `param` INT UNSIGNED NOT NULL DEFAULT 4294967295 COMMENT 'mask', `active` TINYINT(1) NOT NULL DEFAULT TRUE, `expires` INT UNSIGNED NOT NULL, `added` INT UNSIGNED NOT NULL, `admin_id` INT UNSIGNED NOT NULL DEFAULT 0, `comment` TEXT NOT NULL DEFAULT '', `reason` INT UNSIGNED NOT NULL DEFAULT 0, `action` INT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY  (`id`), KEY `type` (`type`, `value`)) ENGINE = InnoDB;",
	"TRUNCATE TABLE `groups`;",
	"ALTER TABLE `groups` ADD `customflags` BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER `flags`;",
	"ALTER TABLE `groups` ADD `violationaccess` INT NOT NULL AFTER `access`;",
	"INSERT INTO `groups` VALUES (1, 'Player', 0, 0, 0, 0, 0, 0);",
	"INSERT INTO `groups` VALUES (2, 'Tutor', 16809984, 1, 1, 0, 0, 0);",
	"INSERT INTO `groups` VALUES (3, 'Senior Tutor', 68736352256, 3, 2, 1, 0, 0);",
	"INSERT INTO `groups` VALUES (4, 'Game Master', 492842123151, 63, 3, 2, 4000, 200);",
	"INSERT INTO `groups` VALUES (5, 'Community Manager', 542239465466, 1279, 4, 3, 6000, 300);",
	"INSERT INTO `groups` VALUES (6, 'God', 546534563834, 2047, 5, 3, 8000, 400);",
	"ALTER TABLE `players` ADD `balance` BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER `blessings`;",
	"ALTER TABLE `players` ADD `stamina` BIGINT UNSIGNED NOT NULL DEFAULT 201660000 AFTER `balance`;",
	"ALTER TABLE `players` CHANGE `loss_experience` `loss_experience` INT NOT NULL DEFAULT 10;",
	"ALTER TABLE `players` CHANGE `loss_mana` `loss_mana` INT NOT NULL DEFAULT 10;",
	"ALTER TABLE `players` CHANGE `loss_skills` `loss_skills` INT NOT NULL DEFAULT 10;",
	"ALTER TABLE `players` ADD `loss_items` INT NOT NULL DEFAULT 10 AFTER `loss_skills`;",
	"ALTER TABLE `players` ADD `marriage` INT UNSIGNED NOT NULL DEFAULT 0;",
	"UPDATE `players` SET `loss_experience` = 10, `loss_mana` = 10, `loss_skills` = 10, `loss_items` = 10;",
	"ALTER TABLE `accounts` DROP `type`;",
	"ALTER TABLE `player_deaths` DROP `is_player`;",
	"ALTER TABLE `houses` CHANGE `warnings` `warnings` INT NOT NULL DEFAULT 0;",
	"ALTER TABLE `houses` ADD `lastwarning` INT UNSIGNED NOT NULL DEFAULT 0;",
	"DROP TRIGGER IF EXISTS `ondelete_accounts`;",
	"DROP TRIGGER IF EXISTS `ondelete_players`;",
	"DELIMITER | ",
	"CREATE TRIGGER `ondelete_accounts` BEFORE DELETE ON `accounts` FOR EACH ROW BEGIN DELETE FROM `bans` WHERE `type` != 1 AND `type` != 2 AND `value` = OLD.`id`; END|",
	"CREATE TRIGGER `ondelete_players` BEFORE DELETE ON `players` FOR EACH ROW BEGIN DELETE FROM `bans` WHERE `type` = 2 AND `value` = OLD.`id`; UPDATE `houses` SET `owner` = 0 WHERE `owner` = OLD.`id`; END|",
	"DELIMITER ;"
};

/*
 * SqLite Queries array, put all SqLite queries
 * to execute inside it.
 */
const char* sqliteQueries[] = {};

/*
 * Main function which will run at startup.
 */
int main()
{
	/*
	 * (Character) DatabaseType; either MySQL or SqLite.
	 */
	char DatabaseType[6];

	/*
	 * Application information at startup.
	 */
	printf("TheForgottenServer - SQLUpdater v%s\n", version);

	/*
	 * Incase goto setDatabaseType is executed
	 * the application will start over from here.
	 */
	setDatabaseType:
	
	/*
	 * Ask the user for prefered DatabaseType.
	 */
	printf("[MySQL / SqLite] DatabaseType: ");

	/*
	 * Scan the input.
	 */
	scanf("%s", DatabaseType);

	/*
	 * Check if the input was mysql.
	 */
	if(strcasecmp(DatabaseType, "mysql") == 0)
	{
		/*
		 * (Characters) Variables storing MySQL data required to
		 * connect to the SQL Server and then the database.
		 */
		char MySQLHost[100], MySQLUser[100], MySQLPassword[100], MySQLDatabase[100];

		/*
		 * Incase setConnectionData is executed
		 * because the user entered bad data we will
		 * start over from here.
		 */
		setConnectionData:

		/*
		 * Ask the user for host where the database is located.
		 */
		printf("[MySQL] Host: ");

		/*
		 * Scan the input.
		 */
		scanf("%s", MySQLHost);

		/*
		 * Repeat for the user what the host was set to.
		 */
		printf("[MySQL] Host set to: %s.\n", MySQLHost);

		/*
		 * Ask the user for username to connect to the database.
		 */
		printf("[MySQL] User: ");

		/*
		 * Scan the username input.
		 */
		scanf("%s", MySQLUser);

		/*
		 * Repeat for the user what the username was set to.
		 */
		printf("[MySQL] User set to: %s.\n", MySQLUser);

		/*
		 * Ask the user for password to connect to the database.
		 */
		printf("[MySQL] Password: ");

		/*
		 * Scan the password input.
		 */
		scanf("%s", MySQLPassword); //TODO: Display input text as *

		/*
		 * Inform the user that the password has been set.
		 */
		printf("[MySQL] Password has been set.\n");

		/*
		 * Incase the database name was wrong,
		 * the user will be able to type it again.
		 */
		setDatabaseName:

		/*
		 * Ask the user for the database name.
		 */
		printf("[MySQL] Database: ");

		/*
		 * Scan the database name input.
		 */
		scanf("%s", MySQLDatabase);

		/*
		 * Repeat for the user what the database name was set to.
		 */
		printf("[MySQL] Database set to: %s.\n", MySQLDatabase);

		/*
		 * (MySQL Variable) Handle (sock).
		 */
		MYSQL MySQLHandle;

		/*
		 * MySQL initialize function,
		 * Parameters:
		 * 1 - Handle.
		 */
		if(mysql_init(&MySQLHandle) != NULL)
		{
			/*
			 * Success, now we'll create a connection to the
			 * MySQL Server with the data we received from the user.
			 */

			/*
			 * MySQL connect function,
			 * Parameters:
			 * 1 - Handle.
			 * 2 - MySQL Host.
			 * 3 - MySQL Username.
			 * 4 - MySQL Password.
			 * 5 - MySQL Database (we keep it NULL and select it
			 *	when we have connected to the SQL Server.
			 * 6 - Port (3306)
			 * 7 - NULL
			 * 8 - Client Multi Statements.
			 */
			if(mysql_real_connect(&MySQLHandle, MySQLHost, MySQLUser, MySQLPassword, NULL, 0, NULL, 0))
			{
				/*
				 * Success, now we'll try to connect to the database
				 * with the database name we received from the user.
				 */

				/*
				 * MySQL database select function,
				 * Parameters:
				 * 1 - Handle.
				 * 2 - Database Name.
				 */
				if(!mysql_select_db(&MySQLHandle, MySQLDatabase))
				{
					/*
					 * Success, it's time to loop the queries
					 * in the query array and try to execute them.
					 */

					/*
					 * (Integer) i, to be used in the loop.
					 */
					int i;

					/*
					 * (for) loop, we declare i as 0, and keep looping
					 * while the integer i is below the size of queries
					 * array, for each loop the value of i will increase.
					 */
					for(i = 0; i < sizeof(mysqlQueries) / 8; i++)
					{
						/*
						 * MySQL query executing function,
						 * Parameters:
						 * 1 - Handle.
						 * 2 - Query.
						 * 3 - Query length.
						 */
						if(mysql_real_query(&MySQLHandle, mysqlQueries[i], strlen(mysqlQueries[i])) == 0)
						{
							/*
							 * Inform the user that this query was successfully executed.
							 */
							printf("[MySQL] Successfully executed the query: %s.\n", mysqlQueries[i]);
						}
						else // If the SQL Server failed to execute the query...
						{
							/*
							 * Inform the user that this query failed with error message.
							 */
							printf("[MySQL] ERROR: Failed to execute query: %s.\n", mysql_error(&MySQLHandle));
						}
					}

					/*
					 * We have executed the queries we wanted to,
					 * now it's time to close the socket to the
					 * MySQL Server.
					 */

					/*
					 * MySQL close database connection function,
					 * Parameters:
					 * 1 - Handle (sock).
					 */
					mysql_close(&MySQLHandle);
				}
				else // If the database does not exist...
				{
					/*
					 * Inform the user that we failed to select the database.
					 */
					printf("[MySQL] ERROR: Failed to select database: %s.\n", MySQLDatabase);

					/*
					 * Let the user set database name again.
					 */
					goto setDatabaseName;
				}
			}
			else // If we failed to connect to the database...
			{
				/*
				 * Inform the user that we failed to connect to the database.
				 */
				printf("[MySQL] ERROR: Failed to connect: %s.\n", mysql_error(&MySQLHandle));

				/*
				 * Let the user set the connection data again.
				 */
				goto setConnectionData;
			}
		}
		else // If we failed to initialize...
		{
			/*
			 * Inform the user that we failed to initialize.
			 */
			printf("[MySQL] ERROR: Failed to initialize.\n");

			/*
			 * Let the user set the connection data again.
			 */
			goto setConnectionData;
		}

		/*
		 * If the query array is empty we will inform the user
		 * about that there was no queries to execute.
		 */
		if(sizeof(mysqlQueries) / 8 == 0)
			printf("[MySQL] No queries to execute.\n");
	}
	/*
	 * Check if the input was sqlite.
	 */
	else if(strcasecmp(DatabaseType, "sqlite") == 0)
	{
		/*
		 * (Character) Variable to store SqLite Database
		 * filename in.
		 */
		char SqLiteFilename[255];

		/*
		 * Incase the file does not exist or is not readable,
		 * we let the user to set filename again when goto
		 * setFileName is executed.
		 */
		setFilename:

		/*
		 * Ask the user for the filename to the database.
		 */
		printf("[SqLite] Filename: ");

		/*
		 * Scan the database filename input.
		 */
		scanf("%s", SqLiteFilename);

		/*
		 * Get the result by opening the file.
		 */
		FILE* sqliteFile = fopen(SqLiteFilename, "r");

		/*
		 * If the pointer is not NULL, the file was
		 * successfully open.
		 */
		if(sqliteFile != NULL)
		{
			/*
			 * Repeat for the user what the filename was set to.
			 */
			printf("[SqLite] Filename set to: %s.\n", SqLiteFilename);

			/*
			 * Now that we know the file exists, we can close the
			 * file, keeping it open when we don't need it would be
			 * a memory leak.
			 */
			fclose(sqliteFile);

			/*
			 * (SqLite3 Variable) Handle (sock).
			 */
			sqlite3* SqLiteHandle;

			/*
			 * Now we try to open the file as a SqLite3 file..
			 */

			/*
			 * SqLite3 database loading function,
			 * Parameters:
			 * 1 - Database filename.
			 * 2 - Handle.
			 */
			if(sqlite3_open(SqLiteFilename, &SqLiteHandle) == SQLITE_OK)
			{
				/*
				 * Success! We will now loop the queries in the query array.
				 * by looping it.
				 */

				/*
				 * (Integer) i, to be used in the loop.
				 */
				int i;

				/*
				 * (for) loop, we declare i as 0, and keep looping
				 * while the integer i is below the size of queries
				 * array, for each loop the value of i will increase.
				 */
				for(i = 0; i < sizeof(sqliteQueries) / 8; i++)
				{
					/*
					 * Another pointer, we have to remember to free it
					 * incase we fill it with data! This one will contain
					 * the error message returned if the query was not
					 * successfully executed.
					 */
					char* errorMessage;

					/*
					 * SqLite3 query executing function,
					 * Parameters:
					 * 1 - Handle.
					 * 2 - Query.
					 * 3 - Callback.
					 * 4 - First argument to callback.
					 * 5 - Character pointer to receive error message,
					 *	incase query execution failed.
					 */
					if(sqlite3_exec(SqLiteHandle, sqliteQueries[i], 0, 0, &errorMessage) == SQLITE_OK)
					{
						/*
						 * Inform the user that this query was successfully executed.
						 */
						printf("[SqLite] Successfully executed the query: %s.\n", sqliteQueries[i]);
					}
					else
					{
						/*
						 * Inform the user that this query failed with error message.
						 */
						printf("[SqLite] ERROR: Failed to execute query[%s]: %s.\n", sqliteQueries[i], errorMessage);

						/*
						 * Free the errorMessage character pointer,
						 * now that it is not in use anymore.
						 */
						free(errorMessage);
					}
				}

				/*
				 * We have executed the queries we wanted to,
				 * now it's time to close the SqLite database
				 * file.
				 */

				/*
				 * SqLite close database function,
				 * Parameters:
				 * 1 - Handle.
				 */
				sqlite3_close(SqLiteHandle);
			}
			else // If we failed to initialize...
			{
				/*
				 * Inform the user that we failed to initialize.
				 */
				printf("[SqLite] ERROR: Failed to initialize.\n");

				/*
				 * Let the user set the filename to the database again.
				 */
				goto setFilename;
			}
		}
		else // If file does not exist or is not readable...
		{
			/*
			 * Inform the user that we couldn't read the file.
			 */
			printf("[SqLite] %s does not exist, or is not readable.\n", SqLiteFilename);
			
			/*
			 * Let the user set the filename to the database again.
			 */
			goto setFilename;
		}

		/*
		 * If the query array is empty we will inform the user
		 * about that there was no queries to execute.
		 */
		if(sizeof(sqliteQueries) / 8 == 0)
			printf("[SqLite] No queries to execute.\n");
	}
	else
		goto setDatabaseType;

	/*
	 * Check if WIN32 is defined (Windows is used to compile)
	*/
	#ifdef WIN32
	/*
	 * Incase Windows is used to compile we do not want to
	 * let the console instantly close, therefore we use
	 * getchar to let the console stay alive until the
	 * user has pressed on return.
	 */
	getchar();
	#endif

	/*
	 * The end of the application,
	 * it will now shutdown.
	 */
	return 0;
}
