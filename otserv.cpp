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
#include "otsystem.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#ifndef WINDOWS
#include <unistd.h>
#endif
#include <boost/config.hpp>

#include "server.h"
#ifdef __LOGIN_SERVER__
#include "gameservers.h"
#endif
#include "networkmessage.h"

#include "game.h"
#include "chat.h"
#include "tools.h"
#include "rsa.h"
#include "textlogger.h"

#include "protocollogin.h"
#include "protocolgame.h"
#include "protocolold.h"
#include "status.h"
#ifdef __REMOTE_CONTROL__
#include "admin.h"
#endif

#include "configmanager.h"
#include "scriptmanager.h"
#include "databasemanager.h"

#include "iologindata.h"
#include "ioban.h"

#include "outfit.h"
#include "vocation.h"
#include "group.h"

#include "monsters.h"
#ifdef __OTSERV_ALLOCATOR__
#include "allocator.h"
#endif
#ifdef __EXCEPTION_TRACER__
#include "exception.h"
#endif

#if defined(WINDOWS) && !defined(__CONSOLE__)
#include "shellapi.h"
#include "gui.h"
#include "inputbox.h"
#include "commctrl.h"
#else
#include "resources.h"
#endif

#ifdef __NO_BOOST_EXCEPTIONS__
#include <exception>

void boost::throw_exception(std::exception const & e)
{
	std::cout << "Boost exception: " << e.what() << std::endl;
}
#endif

ConfigManager g_config;
Game g_game;
Monsters g_monsters;
Npcs g_npcs;

RSA g_RSA;
Chat g_chat;
#if defined(WINDOWS) && !defined(__CONSOLE__)
GUILogger g_logger;
NOTIFYICONDATA NID;
#endif

IpList serverIps;
boost::mutex g_loaderLock;
boost::condition_variable g_loaderSignal;

boost::unique_lock<boost::mutex> g_loaderUniqueLock(g_loaderLock);
#ifdef __REMOTE_CONTROL__
extern Admin* g_admin;
#endif

#if !defined(WINDOWS) || defined(__CONSOLE__)
bool argumentsHandler(StringVec args)
{
	StringVec tmp;
	for(StringVec::iterator it = args.begin(); it != args.end(); ++it)
	{
		if((*it) == "--help")
		{
			std::cout << "Usage:\n"
			"\n"
			"\t--config=$1\t\tAlternate configuration file path.\n"
			"\t--data-directory=$1\tAlternate data directory path.\n"
			"\t--ip=$1\t\t\tIP address of gameworld server.\n"
			"\t\t\t\tShould be equal to the global IP.\n"
			"\t--login-port=$1\tPort for login server to listen on.\n"
			"\t--game-port=$1\tPort for game server to listen on.\n"
			"\t--admin-port=$1\tPort for admin server to listen on.\n"
			"\t--status-port=$1\tPort for status server to listen on.\n";
#ifndef WINDOWS
			std::cout << "\t--runfile=$1\t\tSpecifies run file. Will contain the pid\n"
			"\t\t\t\tof the server process as long as it is running.\n";
#endif
			std::cout << "\t--output-log=$1\t\tAll standard output will be logged to\n"
			"\t\t\t\tthis file.\n"
			"\t--error-log=$1\t\tAll standard errors will be logged to\n"
			"\t\t\t\tthis file.\n";
			return false;
		}

		if((*it) == "--version")
		{
			std::cout << STATUS_SERVER_NAME << ", version " << STATUS_SERVER_VERSION << " (" << STATUS_SERVER_CODENAME << ")\n"
			"Compiled with " << BOOST_COMPILER << " at " << __DATE__ << ", " << __TIME__ << ".\n"
			"A server developed by Elf, slawkens, Talaturen, Lithium, KaczooH, Kiper, Kornholijo.\n"
			"Visit our forum for updates, support and resources: http://otland.net.\n";
			return false;
		}

		tmp = explodeString((*it), "=");
		if(tmp[0] == "--config")
			g_config.setString(ConfigManager::CONFIG_FILE, tmp[1]);
		else if(tmp[0] == "--data-directory")
			g_config.setString(ConfigManager::DATA_DIRECTORY, tmp[1]);
		else if(tmp[0] == "--ip")
			g_config.setString(ConfigManager::IP, tmp[1]);
		else if(tmp[0] == "--login-port")
			g_config.setNumber(ConfigManager::LOGIN_PORT, atoi(tmp[1].c_str()));
		else if(tmp[0] == "--game-port")
			g_config.setNumber(ConfigManager::GAME_PORT, atoi(tmp[1].c_str()));
		else if(tmp[0] == "--admin-port")
			g_config.setNumber(ConfigManager::ADMIN_PORT, atoi(tmp[1].c_str()));
		else if(tmp[0] == "--status-port")
			g_config.setNumber(ConfigManager::STATUS_PORT, atoi(tmp[1].c_str()));
#ifndef WINDOWS
		else if(tmp[0] == "--runfile")
			g_config.setString(ConfigManager::RUNFILE, tmp[1]);
#endif
		else if(tmp[0] == "--output-log")
			g_config.setString(ConfigManager::OUT_LOG, tmp[1]);
		else if(tmp[0] == "--error-log")
			g_config.setString(ConfigManager::ERROR_LOG, tmp[1]);
	}

	return true;
}
#endif

#ifndef WINDOWS
void signalHandler(int32_t sig)
{
	uint32_t tmp = 0;
	switch(sig)
	{
		case SIGHUP:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::saveGameState, &g_game, false)));
			break;

		case SIGTRAP:
			g_game.cleanMap(tmp);
			break;

		case SIGCHLD:
			g_game.proceduralRefresh();
			break;

		case SIGUSR1:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::setGameState, &g_game, GAME_STATE_CLOSED)));
			break;

		case SIGUSR2:
			g_game.setGameState(GAME_STATE_NORMAL);
			break;

		case SIGCONT:
			g_game.reloadInfo(RELOAD_ALL);
			break;

		case SIGQUIT:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::setGameState, &g_game, GAME_STATE_SHUTDOWN)));
			break;

		case SIGTERM:
			Dispatcher::getInstance().addTask(createTask(
				boost::bind(&Game::shutdown, &g_game)));
			break;

		default:
			break;
	}
}

void runfileHandler(void)
{
	std::ofstream runfile(g_config.getString(ConfigManager::RUNFILE).c_str(), std::ios::trunc | std::ios::out);
	runfile.close();
}
#endif

void allocationHandler()
{
	puts("Allocation failed, server out of memory!\nDecrease size of your map or compile in a 64-bit mode.");
	char buffer[1024];
	fgets(buffer, 1024, stdin);
	exit(-1);
}

void startupErrorMessage(const std::string& error)
{
	if(error.length() > 0)
		std::cout << std::endl << "> ERROR: " << error << std::endl;

	#if defined(WINDOWS) && !defined(__CONSOLE__)
	MessageBox(GUI::getInstance()->m_mainWindow, error.c_str(), "Error", MB_OK);
	system("pause");
	#else
	getchar();
	#endif
	exit(-1);
}

void otserv(
#if !defined(WINDOWS) || defined(__CONSOLE__)
StringVec args,
#endif
ServiceManager* services);

#if !defined(WINDOWS) || defined(__CONSOLE__)
int main(int argc, char *argv[])
{
	StringVec args = StringVec(argv, argv + argc);
	if(argc > 1 && !argumentsHandler(args))
		return 0;

#else
void serverMain(void* param)
{
	std::cout.rdbuf(&g_logger);
	std::cerr.rdbuf(&g_logger);

#endif
	std::set_new_handler(allocationHandler);
	ServiceManager servicer;
	g_config.startup();

	#ifdef __OTSERV_ALLOCATOR_STATS__
	boost::thread(boost::bind(&allocatorStatsThread, (void*)NULL));
	// TODO: destruct this thread...
	#endif
	#ifdef __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;
	mainExceptionHandler.InstallHandler();
	#endif
	#ifndef WINDOWS

	// ignore sigpipe...
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigh.sa_flags = 0;
	sigemptyset(&sigh.sa_mask);
	sigaction(SIGPIPE, &sigh, NULL);

	// register signals
	signal(SIGHUP, signalHandler); //save
	signal(SIGTRAP, signalHandler); //clean
	signal(SIGCHLD, signalHandler); //refresh
	signal(SIGUSR1, signalHandler); //close server
	signal(SIGUSR2, signalHandler); //open server
	signal(SIGCONT, signalHandler); //reload all
	signal(SIGQUIT, signalHandler); //save & shutdown
	signal(SIGTERM, signalHandler); //shutdown
	#endif

	Dispatcher::getInstance().addTask(createTask(boost::bind(otserv,
	#if !defined(WINDOWS) || defined(__CONSOLE__)
	args,
	#endif
	&servicer)));
	g_loaderSignal.wait(g_loaderUniqueLock);

	#if !defined(WINDOWS) || defined(__CONSOLE__)
	std::string outPath = g_config.getString(ConfigManager::OUT_LOG), errPath = g_config.getString(ConfigManager::ERROR_LOG);
	if(outPath.length() < 3)
		outPath = "";
	else if(outPath[0] != '/' && outPath[1] != ':')
	{
		outPath = getFilePath(FILE_TYPE_LOG, outPath);
		std::cout << "> Logging output to file: " << outPath << std::endl;
	}

	if(errPath.length() < 3)
		errPath = "";
	else if(errPath[0] != '/' && errPath[1] != ':')
	{
		errPath = getFilePath(FILE_TYPE_LOG, errPath);
		std::cout << "> Logging errors to file: " << errPath << std::endl;
	}

	if(outPath != "")
	{
		boost::shared_ptr<std::ofstream> outFile;
		outFile.reset(new std::ofstream(outPath.c_str(), (g_config.getBool(ConfigManager::TRUNCATE_LOGS) ?
			std::ios::trunc : std::ios::app) | std::ios::out));
		if(!outFile->is_open())
			startupErrorMessage("Could not open output log file for writing!");

		std::cout.rdbuf(outFile->rdbuf());
	}

	if(errPath != "")
	{
		boost::shared_ptr<std::ofstream> errFile;
		errFile.reset(new std::ofstream(errPath.c_str(), (g_config.getBool(ConfigManager::TRUNCATE_LOGS) ?
			std::ios::trunc : std::ios::app) | std::ios::out));
		if(!errFile->is_open())
			startupErrorMessage("Could not open error log file for writing!");

		std::cerr.rdbuf(errFile->rdbuf());
	}
	#endif

	if(servicer.isRunning())
	{
		std::cout << ">> " << g_config.getString(ConfigManager::SERVER_NAME) << " server Online!" << std::endl << std::endl;
		#if defined(WINDOWS) && !defined(__CONSOLE__)
		SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Status: Online!");
		GUI::getInstance()->m_connections = true;
		#endif
		servicer.run();
	}
	else
	{
		std::cout << ">> " << g_config.getString(ConfigManager::SERVER_NAME) << " server Offline! No services available..." << std::endl << std::endl;
		#if defined(WINDOWS) && !defined(__CONSOLE__)
		SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Status: Offline!");
		GUI::getInstance()->m_connections = true;
		#endif
	}

#ifdef __EXCEPTION_TRACER__
	mainExceptionHandler.RemoveHandler();
#endif
#if !defined(WINDOWS) || defined(__CONSOLE__)
	return 0;
#endif
}

void otserv(
#if !defined(WINDOWS) || defined(__CONSOLE__)
StringVec args,
#endif
ServiceManager* services)
{
	srand((uint32_t)OTSYS_TIME());
	#if defined(WINDOWS)
	#if defined(__CONSOLE__)
	SetConsoleTitle(STATUS_SERVER_NAME);
	#else
	GUI::getInstance()->m_connections = false;
	#endif
	#endif

	g_game.setGameState(GAME_STATE_STARTUP);
	#if !defined(WINDOWS) && !defined(__ROOT_PERMISSION__)
	if(!getuid() || !geteuid())
	{
		std::cout << "> WARNING: " << STATUS_SERVER_NAME << " has been executed as root user! It is recommended to execute as a normal user." << std::endl;
		std::cout << "Continue? (y/N)" << std::endl;

		char buffer = getchar();
		if(buffer == 10 || (buffer != 121 && buffer != 89))
			startupErrorMessage("Aborted.");
	}
	#endif

	std::cout << STATUS_SERVER_NAME << ", version " << STATUS_SERVER_VERSION << " (" << STATUS_SERVER_CODENAME << ")" << std::endl;
	std::cout << "Compiled with " << BOOST_COMPILER << " at " << __DATE__ << ", " << __TIME__ << "." << std::endl;
	std::cout << "A server developed by Elf, slawkens, Talaturen, KaczooH, Lithium, Kiper, Kornholijo." << std::endl;
	std::cout << "Visit our forum for updates, support and resources: http://otland.net." << std::endl << std::endl;

	std::stringstream ss;
	#ifdef __DEBUG__
	ss << " GLOBAL";
	#endif
	#ifdef __DEBUG_MOVESYS__
	ss << " MOVESYS";
	#endif
	#ifdef __DEBUG_CHAT__
	ss << " CHAT";
	#endif
	#ifdef __DEBUG_EXCEPTION_REPORT__
	ss << " EXCEPTION-REPORT";
	#endif
	#ifdef __DEBUG_HOUSES__
	ss << " HOUSES";
	#endif
	#ifdef __DEBUG_LUASCRIPTS__
	ss << " LUA-SCRIPTS";
	#endif
	#ifdef __DEBUG_MAILBOX__
	ss << " MAILBOX";
	#endif
	#ifdef __DEBUG_NET__
	ss << " NET";
	#endif
	#ifdef __DEBUG_NET_DETAIL__
	ss << " NET-DETAIL";
	#endif
	#ifdef __DEBUG_RAID__
	ss << " RAIDS";
	#endif
	#ifdef __DEBUG_SCHEDULER__
	ss << " SCHEDULER";
	#endif
	#ifdef __DEBUG_SPAWN__
	ss << " SPAWNS";
	#endif
	#ifdef __SQL_QUERY_DEBUG__
	ss << " SQL-QUERIES";
	#endif

	std::string debug = ss.str();
	if(!debug.empty())
	{
		std::cout << ">> Debugging:";
		#if defined(WINDOWS) && !defined(__CONSOLE__)
		SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Displaying debugged components");
		#endif
		std::cout << debug << "." << std::endl;
	}

	std::cout << ">> Loading config (" << g_config.getString(ConfigManager::CONFIG_FILE) << ")" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading config");
	#endif
	if(!g_config.load())
		startupErrorMessage("Unable to load " + g_config.getString(ConfigManager::CONFIG_FILE) + "!");

	Logger::getInstance()->open();
	IntegerVec cores = vectorAtoi(explodeString(g_config.getString(ConfigManager::CORES_USED), ","));
	if(cores[0] != -1)
	{
	#ifdef WINDOWS
		int32_t mask = 0;
		for(IntegerVec::iterator it = cores.begin(); it != cores.end(); ++it)
			mask += 1 << (*it);

		SetProcessAffinityMask(GetCurrentProcess(), mask);
	}

	std::stringstream mutexName;
	mutexName << "forgottenserver_" << g_config.getNumber(ConfigManager::WORLD_ID);

	CreateMutex(NULL, FALSE, mutexName.str().c_str());
	if(GetLastError() == ERROR_ALREADY_EXISTS)
		startupErrorMessage("Another instance of The Forgotten Server is already running with the same worldId.\nIf you want to run multiple servers, please change the worldId in configuration file.");

	std::string defaultPriority = asLowerCaseString(g_config.getString(ConfigManager::DEFAULT_PRIORITY));
	if(defaultPriority == "realtime")
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	else if(defaultPriority == "high")
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else if(defaultPriority == "higher")
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

	#else
		cpu_set_t mask;
		CPU_ZERO(&mask);
		for(IntegerVec::iterator it = cores.begin(); it != cores.end(); ++it)
			CPU_SET((*it), &mask);

		sched_setaffinity(getpid(), (int32_t)sizeof(mask), &mask);
	}

	std::string runPath = g_config.getString(ConfigManager::RUNFILE);
	if(runPath != "" && runPath.length() > 2)
	{
		std::ofstream runFile(runPath.c_str(), std::ios::trunc | std::ios::out);
		runFile << getpid();
		runFile.close();
		atexit(runfileHandler);
	}

	if(!nice(g_config.getNumber(ConfigManager::NICE_LEVEL))) {}
	#endif
	std::string encryptionType = asLowerCaseString(g_config.getString(ConfigManager::ENCRYPTION_TYPE));
	if(encryptionType == "md5")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_MD5);
		std::cout << "> Using MD5 encryption" << std::endl;
	}
	else if(encryptionType == "sha1")
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_SHA1);
		std::cout << "> Using SHA1 encryption" << std::endl;
	}
	else
	{
		g_config.setNumber(ConfigManager::ENCRYPTION, ENCRYPTION_PLAIN);
		std::cout << "> Using plaintext encryption" << std::endl;
	}
	
	std::cout << ">> Checking software version... ";
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Checking software version");
	#endif
	if(xmlDocPtr doc = xmlParseFile(VERSION_CHECK))
	{
		xmlNodePtr p, root = xmlDocGetRootElement(doc);
		if(!xmlStrcmp(root->name, (const xmlChar*)"versions"))
		{
			p = root->children->next;
			if(!xmlStrcmp(p->name, (const xmlChar*)"entry"))
			{
				std::string version;
				int32_t patch, build, timestamp;

				bool tmp = false;
				if(readXMLString(p, "version", version) && version != STATUS_SERVER_VERSION)
					tmp = true;

				if(readXMLInteger(p, "patch", patch) && patch > VERSION_PATCH)
					tmp = true;

				if(readXMLInteger(p, "build", build) && build > VERSION_BUILD)
					tmp = true;

				if(readXMLInteger(p, "timestamp", timestamp) && timestamp > VERSION_TIMESTAMP)
					tmp = true;

				if(tmp)
				{
					std::cout << "outdated, please consider updating!" << std::endl;
					std::cout << "> Current version information - version: " << STATUS_SERVER_VERSION << ", patch: " << VERSION_PATCH;
					std::cout << ", build: " << VERSION_BUILD << ", timestamp: " << VERSION_TIMESTAMP << "." << std::endl;
					std::cout << "> Latest version information - version: " << version << ", patch: " << patch;
					std::cout << ", build: " << build << ", timestamp: " << timestamp << "." << std::endl;
					if(g_config.getBool(ConfigManager::CONFIM_OUTDATED_VERSION) && version.find("_SVN") == std::string::npos)
					{
						#if defined(WINDOWS) && !defined(__CONSOLE__)
						if(MessageBox(GUI::getInstance()->m_mainWindow, "Continue?", "Outdated software", MB_YESNO) == IDNO)
						#else
						std::cout << "Continue? (y/N)" << std::endl;
	
						char buffer = getchar();
						if(buffer == 10 || (buffer != 121 && buffer != 89))
						#endif
							startupErrorMessage("Aborted.");
					}
				}
				else
					std::cout << "up to date!" << std::endl;
			}
			else
				std::cout << "failed checking - malformed entry." << std::endl;
		}
		else
			std::cout << "failed checking - malformed file." << std::endl;

		xmlFreeDoc(doc);
	}
	else
		std::cout << "failed - could not parse remote file (are you connected to the internet?)" << std::endl;

	std::cout << ">> Fetching blacklist" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Fetching blacklist");
	#endif
	if(!g_game.fetchBlacklist())
	{
		#if defined(WINDOWS) && !defined(__CONSOLE__)
		if(MessageBox(GUI::getInstance()->m_mainWindow, "Unable to fetch blacklist! Continue?", "Blacklist", MB_YESNO) == IDNO)
		#else
		std::cout << "Unable to fetch blacklist! Continue? (y/N)" << std::endl;
		char buffer = getchar();
		if(buffer == 10 || (buffer != 121 && buffer != 89))
		#endif
			startupErrorMessage("Unable to fetch blacklist!");
	}

	std::cout << ">> Loading RSA key" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading RSA Key");
	#endif

	const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
	const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
	const char* d("46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");
	g_RSA.setKey(p, q, d);

	std::cout << ">> Starting SQL connection" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Starting SQL connection");
	#endif
	Database* db = Database::getInstance();
	if(db && db->isConnected())
	{
		std::cout << ">> Running Database Manager" << std::endl;
		if(!DatabaseManager::getInstance()->isDatabaseSetup())
			startupErrorMessage("The database you specified in config.lua is empty, please import schemas/<dbengine>.sql to the database (if you are using MySQL, please read doc/MYSQL_HELP for more information).");
		else
		{
			uint32_t version = 0;
			do
			{
				version = DatabaseManager::getInstance()->updateDatabase();
				if(version == 0)
					break;

				std::cout << "> Database has been updated to version: " << version << "." << std::endl;
			}
			while(version < VERSION_DATABASE);
		}

		DatabaseManager::getInstance()->checkTriggers();
		DatabaseManager::getInstance()->checkEncryption();
		if(g_config.getBool(ConfigManager::OPTIMIZE_DB_AT_STARTUP) && !DatabaseManager::getInstance()->optimizeTables())
			std::cout << "> No tables were optimized." << std::endl;
	}
	else
		startupErrorMessage("Couldn't estabilish connection to SQL database!");

	std::cout << ">> Loading items" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading items");
	#endif
	if(Item::items.loadFromOtb(getFilePath(FILE_TYPE_OTHER, "items/items.otb")))
		startupErrorMessage("Unable to load items (OTB)!");

	if(!Item::items.loadFromXml())
	{
		#if defined(WINDOWS) && !defined(__CONSOLE__)
		if(MessageBox(GUI::getInstance()->m_mainWindow, "Unable to load items (XML)! Continue?", "Items (XML)", MB_YESNO) == IDNO)
		#else
		std::cout << "Unable to load items (XML)! Continue? (y/N)" << std::endl;
		char buffer = getchar();
		if(buffer == 10 || (buffer != 121 && buffer != 89))
		#endif
			startupErrorMessage("Unable to load items (XML)!");
	}

	std::cout << ">> Loading groups" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading groups");
	#endif
	if(!Groups::getInstance()->loadFromXml())
		startupErrorMessage("Unable to load groups!");

	std::cout << ">> Loading vocations" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading vocations");
	#endif
	if(!Vocations::getInstance()->loadFromXml())
		startupErrorMessage("Unable to load vocations!");

	std::cout << ">> Loading script systems" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading script systems");
	#endif
	if(!ScriptingManager::getInstance()->load())
		startupErrorMessage("");

	std::cout << ">> Loading chat channels" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading chat channels");
	#endif
	if(!g_chat.loadFromXml())
		startupErrorMessage("Unable to load chat channels!");

	std::cout << ">> Loading outfits" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading outfits");
	#endif
	if(!Outfits::getInstance()->loadFromXml())
		startupErrorMessage("Unable to load outfits!");

	std::cout << ">> Loading experience stages" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading experience stages");
	#endif
	if(!g_game.loadExperienceStages())
		startupErrorMessage("Unable to load experience stages!");

	std::cout << ">> Loading monsters" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading monsters");
	#endif
	if(!g_monsters.loadFromXml())
	{
		#if defined(WINDOWS) && !defined(__CONSOLE__)
		if(MessageBox(GUI::getInstance()->m_mainWindow, "Unable to load monsters! Continue?", "Monsters", MB_YESNO) == IDNO)
		#else
		std::cout << "Unable to load monsters! Continue? (y/N)" << std::endl;
		char buffer = getchar();
		if(buffer == 10 || (buffer != 121 && buffer != 89))
		#endif
			startupErrorMessage("Unable to load monsters!");
	}

	std::cout << ">> Loading mods..." << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading mods...");
	#endif
	if(!ScriptingManager::getInstance()->loadMods())
		startupErrorMessage("Unable to load mods!");

	std::cout << ">> Loading map and spawns..." << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading map and spawns...");
	#endif
	if(!g_game.loadMap(g_config.getString(ConfigManager::MAP_NAME)))
		startupErrorMessage("");

	#ifdef __LOGIN_SERVER__
	std::cout << ">> Loading game servers" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading game servers");
	#endif
	if(!GameServers::getInstance()->loadFromXml(true))
		startupErrorMessage("Unable to load game servers!");
	#endif

	#ifdef __REMOTE_CONTROL__
	std::cout << ">> Loading administration protocol" << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading administration protocol");
	#endif

	g_admin = new Admin();
	if(!g_admin->loadFromXml())
		startupErrorMessage("Unable to load administration protocol!");

	services->add<ProtocolAdmin>(g_config.getNumber(ConfigManager::ADMIN_PORT));
	#endif

	std::cout << ">> Checking world type... ";
	std::string worldType = asLowerCaseString(g_config.getString(ConfigManager::WORLD_TYPE));
	if(worldType == "pvp" || worldType == "2" || worldType == "normal")
	{
		g_game.setWorldType(WORLD_TYPE_PVP);
		std::cout << "PvP" << std::endl;
	}
	else if(worldType == "no-pvp" || worldType == "nopvp" || worldType == "non-pvp" || worldType == "nonpvp" || worldType == "1" || worldType == "safe")
	{
		g_game.setWorldType(WORLD_TYPE_NO_PVP);
		std::cout << "NoN-PvP" << std::endl;
	}
	else if(worldType == "pvp-enforced" || worldType == "pvpenforced" || worldType == "pvp-enfo" || worldType == "pvpenfo" || worldType == "pvpe" || worldType == "enforced" || worldType == "enfo" || worldType == "3" || worldType == "war")
	{
		g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
		std::cout << "PvP-Enforced" << std::endl;
	}
	else
	{
		std::cout << std::endl;
		startupErrorMessage("Unknown world type: " + g_config.getString(ConfigManager::WORLD_TYPE));
	}

	std::cout << ">> Initializing game state modules and registering services..." << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Initializing game state and registering services...");
	#endif
	g_game.setGameState(GAME_STATE_INIT);

	std::string ip = g_config.getString(ConfigManager::IP);
	std::cout << "> Global address: " << ip << std::endl;
	serverIps.push_back(std::make_pair(LOCALHOST, 0xFFFFFFFF));

	char hostName[128];
	hostent* host = NULL;
	if(!gethostname(hostName, 128) && (host = gethostbyname(hostName)))
	{
		uint8_t** address = (uint8_t**)host->h_addr_list;
		while(address[0] != NULL)
		{
			serverIps.push_back(std::make_pair(*(uint32_t*)(*address), 0x0000FFFF));
			address++;
		}
	}

	uint32_t resolvedIp = inet_addr(ip.c_str());
	if(resolvedIp == INADDR_NONE)
	{
		if((host = gethostbyname(ip.c_str())))
			resolvedIp = *(uint32_t*)host->h_addr;
		else
			startupErrorMessage("Cannot resolve " + ip + "!");
	}

	serverIps.push_back(std::make_pair(resolvedIp, 0));
	Status::getInstance()->setMapName(g_config.getString(ConfigManager::MAP_NAME));

	services->add<ProtocolStatus>(g_config.getNumber(ConfigManager::STATUS_PORT));
	if(
#ifdef __LOGIN_SERVER__
	true
#else
	!g_config.getBool(ConfigManager::LOGIN_ONLY_LOGINSERVER)
#endif
	)
	{
		services->add<ProtocolLogin>(g_config.getNumber(ConfigManager::LOGIN_PORT));
		services->add<ProtocolOldLogin>(g_config.getNumber(ConfigManager::LOGIN_PORT));
	}

	services->add<ProtocolGame>(g_config.getNumber(ConfigManager::GAME_PORT));
	services->add<ProtocolOldGame>(g_config.getNumber(ConfigManager::LOGIN_PORT));
	std::cout << "> Local ports: ";

	std::list<uint16_t> ports = services->getPorts();
	for(std::list<uint16_t>::iterator it = ports.begin(); it != ports.end(); ++it)
		std::cout << (*it) << "\t";

	std::cout << std::endl << ">> All modules were loaded, server is starting up..." << std::endl;
	#if defined(WINDOWS) && !defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> All modules were loaded, server is starting up...");
	#endif
	g_game.setGameState(GAME_STATE_NORMAL);

	g_game.start(services);
	g_loaderSignal.notify_all();
}

#if defined(WINDOWS) && !defined(__CONSOLE__)
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CInputBox iBox(hwnd);
	switch(message)
	{
		case WM_CREATE:
		{
			GUI::getInstance()->m_logWindow = CreateWindow("edit", NULL,
				WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE | ES_MULTILINE | DS_CENTER, 0, 0, 640, 450, hwnd, (HMENU)ID_LOG, NULL, NULL);
			GUI::getInstance()->m_statusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL,
				WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, (HMENU)ID_STATUS_BAR, GetModuleHandle(NULL), NULL);

			int32_t statusBarWidthLine[] = {150, -1};
			GUI::getInstance()->m_lineCount = 0;

			SendMessage(GUI::getInstance()->m_statusBar, SB_SETPARTS, sizeof(statusBarWidthLine) / sizeof(int32_t), (LPARAM)statusBarWidthLine);
			SendMessage(GUI::getInstance()->m_statusBar, SB_SETTEXT, 0, (LPARAM)"Not loaded");

			GUI::getInstance()->m_minimized = false;
			GUI::getInstance()->m_pBox.setParent(hwnd);
			SendMessage(GUI::getInstance()->m_logWindow, WM_SETFONT, (WPARAM)GUI::getInstance()->m_font, 0);

			NID.hWnd = hwnd;
			NID.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON));
			NID.uCallbackMessage = WM_USER + 1;
			NID.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;

			strcpy(NID.szTip, STATUS_SERVER_NAME);
			Shell_NotifyIcon(NIM_ADD, &NID);

			boost::thread(boost::bind(&serverMain, (void*)hwnd));
			break;
		}

		case WM_SIZE:
		{
			if(wParam == SIZE_MINIMIZED)
			{
				GUI::getInstance()->m_minimized = true;
				ShowWindow(hwnd, SW_HIDE);
				ModifyMenu(GUI::getInstance()->m_trayMenu, ID_TRAY_HIDE, MF_STRING, ID_TRAY_HIDE, "&Show window");
			}
			else
			{
				RECT rcStatus;
				int32_t iStatusHeight;
				int32_t iEditHeight;
				RECT rcClient;
				GUI::getInstance()->m_statusBar = GetDlgItem(hwnd, ID_STATUS_BAR);
				SendMessage(GUI::getInstance()->m_statusBar, WM_SIZE, 0, 0);
				GetWindowRect(GUI::getInstance()->m_statusBar, &rcStatus);
				iStatusHeight = rcStatus.bottom - rcStatus.top;
				GetClientRect(hwnd, &rcClient);
				iEditHeight = rcClient.bottom - iStatusHeight;
				GUI::getInstance()->m_logWindow = GetDlgItem(hwnd, ID_LOG);
				SetWindowPos(GUI::getInstance()->m_logWindow, NULL, 0, rcClient.top, rcClient.right, iEditHeight, SWP_NOZORDER);
			}

			break;
		}

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case ID_TRAY_HIDE:
				{
					if(GUI::getInstance()->m_minimized)
					{
						ShowWindow(hwnd, SW_SHOW);
						ShowWindow(hwnd, SW_RESTORE);
						ModifyMenu(GUI::getInstance()->m_trayMenu, ID_TRAY_HIDE, MF_STRING, ID_TRAY_HIDE, "&Hide window");
						GUI::getInstance()->m_minimized = false;
					}
					else
					{
						ShowWindow(hwnd, SW_HIDE);
						ModifyMenu(GUI::getInstance()->m_trayMenu, ID_TRAY_HIDE, MF_STRING, ID_TRAY_HIDE, "&Show window");
						GUI::getInstance()->m_minimized = true;
					}

					break;
				}

				case ID_MENU_MAIN_ACCEPT:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP && !GUI::getInstance()->m_connections)
					{
						GUI::getInstance()->m_connections = true;
						ModifyMenu(GetMenu(hwnd), ID_MENU_MAIN_ACCEPT, MF_STRING, ID_MENU_MAIN_REJECT, "&Reject connections");
					}

					break;
				}

				case ID_MENU_MAIN_REJECT:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP && GUI::getInstance()->m_connections)
					{
						GUI::getInstance()->m_connections = false;
						ModifyMenu(GetMenu(hwnd), ID_MENU_MAIN_REJECT, MF_STRING, ID_MENU_MAIN_ACCEPT, "&Accept connections");
					}

					break;
				}

				case ID_MENU_MAIN_CLEARLOG:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						GUI::getInstance()->m_logText = "";
						GUI::getInstance()->m_lineCount = 0;
						std::cout << STATUS_SERVER_NAME << ", version " << STATUS_SERVER_VERSION << " (" << STATUS_SERVER_CODENAME << ")" << std::endl;
						std::cout << "Compiled with " << BOOST_COMPILER << " at " << __DATE__ << ", " << __TIME__ << "." << std::endl;
						std::cout << "A server developed by Elf, slawkens, Talaturen, Lithium, KaczooH, Kiper, Kornholijo." << std::endl;
						std::cout << "Visit our forum for updates, support and resources: http://otland.net." << std::endl;
						std::cout << std::endl;
					}

					break;
				}

				case ID_TRAY_SHUTDOWN:
				case ID_MENU_MAIN_SHUTDOWN:
				{
					if(MessageBox(hwnd, "Are you sure you want to shutdown the server?", "Shutdown", MB_YESNO) == IDYES)
					{
						Dispatcher::getInstance().addTask(
							createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_SHUTDOWN)));
						Shell_NotifyIcon(NIM_DELETE, &NID);
					}

					break;
				}

				case ID_MENU_SERVER_WORLDTYPE_PVP:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						g_game.setWorldType(WORLD_TYPE_PVP);
						std::cout << "WorldType set to 'PVP'." << std::endl;
					}

					break;
				}

				case ID_MENU_SERVER_WORLDTYPE_NOPVP:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						g_game.setWorldType(WORLD_TYPE_NO_PVP);
						std::cout << "WorldType set to 'Non-PVP'." << std::endl;
					}

					break;
				}

				case ID_MENU_SERVER_WORLDTYPE_PVPENFORCED:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
						std::cout << "WorldType set to 'PVP-Enforced'." << std::endl;
					}

					break;
				}

				case ID_MENU_SERVER_BROADCAST:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(iBox.DoModal("Broadcast message", "What would you like to broadcast?"))
							g_game.broadcastMessage(iBox.Text, MSG_STATUS_WARNING);
					}

					break;
				}

				case ID_MENU_SERVER_SAVE:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						Dispatcher::getInstance().addTask(createTask(
							boost::bind(&Game::saveGameState, &g_game, false)));
						MessageBox(NULL, "Server has been saved.", "Server save", MB_OK);
					}

					break;
				}

				case ID_MENU_SERVER_CLEAN:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						uint32_t count = 0;
						g_game.cleanMap(count);

						char buffer[100];
						sprintf(buffer, "Map has been cleaned, collected %u items.", count);
						MessageBox(NULL, buffer, "Map clean", MB_OK);
					}

					break;
				}

				case ID_MENU_SERVER_REFRESH:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						g_game.proceduralRefresh();
						MessageBox(NULL, "Map will now refresh in a while.", "Map refresh", MB_OK);
					}

					break;
				}

				case ID_MENU_SERVER_OPEN:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP && GUI::getInstance()->m_connections)
					{
						g_game.setGameState(GAME_STATE_NORMAL);
						ModifyMenu(GetMenu(hwnd), ID_MENU_SERVER_OPEN, MF_STRING, ID_MENU_SERVER_CLOSE, "&Close server");
					}

					break;
				}

				case ID_MENU_SERVER_CLOSE:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP && GUI::getInstance()->m_connections)
					{
						Dispatcher::getInstance().addTask(createTask(
							boost::bind(&Game::setGameState, &g_game, GAME_STATE_CLOSED)));
						ModifyMenu(GetMenu(hwnd), ID_MENU_SERVER_CLOSE, MF_STRING, ID_MENU_SERVER_OPEN, "&Open server");
					}

					break;
				}

				case ID_MENU_SERVER_PLAYERBOX:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP && GUI::getInstance()->m_connections)
					{
						if(g_game.getPlayersOnline() == 0)
							MessageBox(NULL, "No players online.", "Player management", MB_OK);
						else
							GUI::getInstance()->m_pBox.popUp("Player management");
					}

					break;
				}

				case ID_MENU_RELOAD_ACTIONS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_ACTIONS))
							std::cout << "Reloaded actions." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_CHAT:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_CHAT))
							std::cout << "Reloaded chat channels." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_CONFIG:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_CONFIG))
							std::cout << "Reloaded config." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_CREATUREEVENTS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_CREATUREEVENTS))
							std::cout << "Reloaded creature events." << std::endl;
					}

					break;
				}

				#ifdef __LOGIN_SERVER__
				case ID_MENU_RELOAD_GAMESERVERS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_GAMESERVERS))
							std::cout << "Reloaded game servers." << std::endl;
					}

					break;
				}

				#endif
				case ID_MENU_RELOAD_GLOBALEVENTS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_GLOBALEVENTS))
							std::cout << "Reloaded global events." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_GROUPS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_GROUPS))
							std::cout << "Reloaded groups." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_HIGHSCORES:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_HIGHSCORES))
							std::cout << "Reloaded highscores." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_HOUSEPRICES:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_HOUSEPRICES))
							std::cout << "Reloaded house prices." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_ITEMS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_ITEMS))
							std::cout << "Reloaded items." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_MODS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_MODS))
							std::cout << "Reloaded mods." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_MONSTERS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_MONSTERS))
							std::cout << "Reloaded monsters." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_MOVEMENTS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_MOVEEVENTS))
							std::cout << "Reloaded movements." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_NPCS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_NPCS))
							std::cout << "Reloaded npcs." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_OUTFITS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_OUTFITS))
							std::cout << "Reloaded outfits." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_QUESTS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_QUESTS))
							std::cout << "Reloaded quests." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_RAIDS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_RAIDS))
							std::cout << "Reloaded raids." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_SPELLS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_SPELLS))
							std::cout << "Reloaded spells." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_STAGES:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_STAGES))
							std::cout << "Reloaded stages." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_TALKACTIONS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_TALKACTIONS))
							std::cout << "Reloaded talk actions." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_VOCATIONS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_VOCATIONS))
							std::cout << "Reloaded vocations." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_WEAPONS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_WEAPONS))
							std::cout << "Reloaded weapons." << std::endl;
					}

					break;
				}

				case ID_MENU_RELOAD_ALL:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_ALL))
							std::cout << "Reloaded all." << std::endl;
					}

					break;
				}

				default:
					break;
			}

			break;
		}

		case WM_CLOSE:
		case WM_DESTROY:
		{
			if(MessageBox(hwnd, "Are you sure you want to shutdown the server?", "Shutdown", MB_YESNO) == IDYES)
			{
				Shell_NotifyIcon(NIM_DELETE, &NID);
				Dispatcher::getInstance().addTask(createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_SHUTDOWN)));
			}

			break;
		}

		case WM_USER + 1: // tray icon messages
		{
			switch(lParam)
			{
				case WM_RBUTTONUP: // right click
				{
					POINT mp;
					GetCursorPos(&mp);
					TrackPopupMenu(GetSubMenu(GUI::getInstance()->m_trayMenu, 0), 0, mp.x, mp.y, 0, hwnd, 0);
					break;
				}

				case WM_LBUTTONUP: // left click
				{
					if(GUI::getInstance()->m_minimized)
					{
						ShowWindow(hwnd, SW_SHOW);
						ShowWindow(hwnd, SW_RESTORE);
						ModifyMenu(GUI::getInstance()->m_trayMenu, ID_TRAY_HIDE, MF_STRING, ID_TRAY_HIDE, "&Hide window");
						GUI::getInstance()->m_minimized = false;
					}

					break;
				}
			}

			break;
		}

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

int32_t WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int32_t WindowStyle)
{
	MSG messages;
	WNDCLASSEX wincl;
	GUI::getInstance()->initTrayMenu();
	GUI::getInstance()->initFont();
	wincl.hInstance = hInstance;
	wincl.lpszClassName = "forgottenserver_gui";
	wincl.lpfnWndProc = WindowProcedure;
	wincl.style = CS_DBLCLKS;
	wincl.cbSize = sizeof(WNDCLASSEX);
	wincl.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON));
	wincl.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON), IMAGE_ICON, 16, 16, 0);
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.lpszMenuName = MAKEINTRESOURCE(ID_MENU);
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	if(!RegisterClassEx(&wincl))
		return 0;

	GUI::getInstance()->m_mainWindow = CreateWindowEx(0, "forgottenserver_gui", STATUS_SERVER_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 450, HWND_DESKTOP, NULL, hInstance, NULL);
	ShowWindow(GUI::getInstance()->m_mainWindow, 1);
	while(GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return messages.wParam;
}
#endif
