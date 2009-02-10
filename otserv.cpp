//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// otserv main. The only place where things get instantiated.
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
#include "otsystem.h"

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <boost/asio.hpp>

#include "server.h"
#include "status.h"
#include "networkmessage.h"
#ifdef __LOGIN_SERVER__
#include "gameservers.h"
#endif
#ifdef __REMOTE_CONTROL__
#include "admin.h"
#endif

#include "game.h"
#include "protocolgame.h"
#include "tools.h"
#include "rsa.h"

#include "scriptmanager.h"
#include "configmanager.h"
#include "databasemanager.h"

#include "iologindata.h"
#include "ioban.h"
#include "outfit.h"
#include "vocation.h"
#include "monsters.h"

#ifndef __CONSOLE__
#ifdef WIN32
#include "shellapi.h"
#include "gui.h"
#include "textlogger.h"
#include "inputbox.h"
#include "commctrl.h"
#endif
#else
#include "resources.h"
#endif

#ifdef __OTSERV_ALLOCATOR__
#include "allocator.h"
#endif
#ifdef __EXCEPTION_TRACER__
#include "exception.h"
#endif
#ifdef BOOST_NO_EXCEPTIONS
#include <exception>

void boost::throw_exception(std::exception const & e)
{
	std::cout << "Boost exception: " << e.what() << std::endl;
}
#endif

IPList serverIPs;
#ifdef __REMOTE_CONTROL__
extern Admin* g_admin;
#endif
ConfigManager g_config;
Game g_game;
Monsters g_monsters;
Npcs g_npcs;
Vocations g_vocations;

#if defined(WIN32) && not defined(__CONSOLE__)
NOTIFYICONDATA NID;
TextLogger logger;
#endif

RSA* g_otservRSA = NULL;
Server* g_server = NULL;

OTSYS_THREAD_LOCKVAR_PTR g_loaderLock;
OTSYS_THREAD_SIGNALVAR g_loaderSignal;

void startupErrorMessage(const std::string& error)
{
	if(error.length() > 0)
		std::cout << std::endl << "> ERROR: " << error << std::endl;

	#ifdef WIN32
	#ifndef __CONSOLE__
	system("pause");
	#endif
	#else
	getchar();
	#endif
	exit(1);
}

#if not defined(WIN32) || defined(__CONSOLE__)
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
			"\t--ip=$1\t\t\tIP address of gameworld server.\n"
			"\t\t\t\tShould be equal to the global IP.\n"
			"\t--port=$1\t\tPort for server to listen on.\n"
			"\t--output-log=$1\t\tAll standard output will be logged to\n"
			"\t\t\t\tthis file.\n"
			"\t--error-log=$1\t\tAll standard errors will be logged to\n"
			"\t\t\t\tthis file.\n";
			return false;
		}

		tmp = explodeString((*it), "=");
		if(tmp[0] == "--config")
			g_config.setString(ConfigManager::CONFIG_FILE, tmp[1]);

		if(tmp[0] == "--ip")
			g_config.setString(ConfigManager::IP, tmp[1]);

		if(tmp[0] == "--port")
			g_config.setNumber(ConfigManager::PORT, atoi(tmp[1].c_str()));

		if(tmp[0] == "--output-log")
			g_config.setString(ConfigManager::OUT_LOG, tmp[1]);

		if(tmp[0] == "--error-log")
			g_config.setString(ConfigManager::ERROR_LOG, tmp[1]);
	}

	return true;
}
#endif

#ifndef WIN32
void signalHandler(int32_t sig)
{
	uint32_t tmp = 0;
	switch(sig)
	{
		case SIGHUP:
			Dispatcher::getDispatcher().addTask(createTask(
				boost::bind(&Game::saveGameState, &g_game, true)));
			break;
		case SIGTRAP:
			g_game.cleanMap(tmp);
			break;
		case SIGCHLD:
			g_game.proceduralRefresh();
			break;
		case SIGUSR1:
			Dispatcher::getDispatcher().addTask(createTask(
				boost::bind(&Game::setGameState, &g_game, GAME_STATE_CLOSED)));
			break;
		case SIGUSR2:
			g_game.setGameState(GAME_STATE_NORMAL);
			break;
		case SIGCONT:
			g_game.reloadInfo(RELOAD_ALL);
			break;
		case SIGQUIT:
			Dispatcher::getDispatcher().addTask(createTask(
				boost::bind(&Game::setGameState, &g_game, GAME_STATE_SHUTDOWN)));
			break;
		case SIGTERM:
                        Dispatcher::getDispatcher().addTask(createTask(
				boost::bind(&Game::shutdown, &g_game)));
			break;
		default:
			break;
	}
}
#endif

void otserv(
#if not defined(WIN32) || defined(__CONSOLE__)
int argc, char *argv[]
#endif
);

#if not defined(WIN32) || defined(__CONSOLE__)
int main(int argc, char *argv[])
{
	if(argc > 1 && !argumentsHandler(StringVec(argv, argv + argc)))
		exit(1);
#else
void serverMain(void* param)
{
	std::cout.rdbuf(&logger);
	std::cerr.rdbuf(&logger);
#endif

	g_config.startup();
	#ifdef __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;
	mainExceptionHandler.InstallHandler();
	#endif
	#ifdef __OTSERV_ALLOCATOR_STATS__
	OTSYS_CREATE_THREAD(allocatorStatsThread, NULL);
	#endif

	#ifndef WIN32
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

	OTSYS_THREAD_LOCKVARINIT(g_loaderLock);
	OTSYS_THREAD_SIGNALVARINIT(g_loaderSignal);
	Dispatcher::getDispatcher().addTask(createTask(boost::bind(otserv
	#if not defined(WIN32) || defined(__CONSOLE__)
	, argc, argv
	#endif
	)));
	OTSYS_THREAD_LOCK(g_loaderLock, "otserv()");
	OTSYS_THREAD_WAITSIGNAL(g_loaderSignal, g_loaderLock);

	std::cout << ">> " << g_config.getString(ConfigManager::SERVER_NAME) << " server Online!" << std::endl << std::endl;
	#if defined(WIN32) && not defined(__CONSOLE__)
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Status: Online!");
	GUI::getInstance()->m_connections = true;
	#else
	boost::shared_ptr<std::ofstream> outFile;
	if(g_config.getString(ConfigManager::OUT_LOG) != "")
	{
		outFile.reset(new std::ofstream(getFilePath(FILE_TYPE_LOG, g_config.getString(ConfigManager::OUT_LOG)).c_str(),
			(g_config.getBool(ConfigManager::TRUNCATE_LOGS) ? std::ios::trunc : std::ios::app) | std::ios::out));
		if(!outFile->is_open())
			startupErrorMessage("Could not open output log file for writing!");

		std::cout.rdbuf(outFile->rdbuf());
	}

	boost::shared_ptr<std::ofstream> errorFile;
	if(g_config.getString(ConfigManager::ERROR_LOG) != "")
	{
		errorFile.reset(new std::ofstream(getFilePath(FILE_TYPE_LOG, g_config.getString(ConfigManager::ERROR_LOG)).c_str(), 
			(g_config.getBool(ConfigManager::TRUNCATE_LOGS) ? std::ios::trunc : std::ios::app) | std::ios::out));
		if(!errorFile->is_open())
			startupErrorMessage("Could not open error log file for writing!");

		std::cerr.rdbuf(errorFile->rdbuf());
	}
	#endif

	Server server(INADDR_ANY, g_config.getNumber(ConfigManager::PORT));
	g_server = &server;
	server.run();
#ifdef __EXCEPTION_TRACER__

	mainExceptionHandler.RemoveHandler();
#endif
#ifdef __CONSOLE__
	return 0;
#endif
}

void otserv(
#if not defined(WIN32) || defined(__CONSOLE__)
int argc, char *argv[]
#endif
)
{
	#ifdef WIN32
	#ifdef __CONSOLE__
	SetConsoleTitle(STATUS_SERVER_NAME);
	#else
	GUI::getInstance()->m_connections = false;
	#endif
	#endif
	g_game.setGameState(GAME_STATE_STARTUP);
	srand((uint32_t)OTSYS_TIME());

	std::cout << STATUS_SERVER_NAME << ", version " << STATUS_SERVER_VERSION << " (" << STATUS_SERVER_CODENAME << ")" << std::endl;
	std::cout << "A server developed by Elf, Talaturen, Lithium, Kiper, Kornholijo, Jonern & Nightmare." << std::endl;
	std::cout << "Visit our forum for updates, support and resources: http://otland.net." << std::endl;
	std::cout << std::endl;
	#if defined __DEBUG__MOVESYS__ || defined __DEBUG_HOUSES__ || defined __DEBUG_MAILBOX__ || defined __DEBUG_LUASCRIPTS__ || defined __DEBUG_RAID__ || defined __DEBUG_NET__
	std::cout << ">> Debugging:";
	#ifdef __DEBUG__MOVESYS__
	std::cout << " MOVESYS";
	#endif
	#ifdef __DEBUG_MAILBOX__
	std::cout << " MAILBOX";
	#endif
	#ifdef __DEBUG_HOUSES__
	std::cout << " HOUSES";
	#endif
	#ifdef __DEBUG_LUASCRIPTS__
	std::cout << " LUA-SCRIPTS";
	#endif
	#ifdef __DEBUG_RAID__
	std::cout << " RAIDS";
	#endif
	#ifdef __DEBUG_NET__
	std::cout << " NET-ASIO";
	#endif
	std::cout << std::endl;
	#endif

	#if !defined(WIN32) && !defined(__ROOT_PERMISSION__)
	if(getuid() == 0 || geteuid() == 0)
		std::cout << "> WARNING: " << STATUS_SERVER_NAME << " has been executed as root user! It is recommended to execute as a normal user." << std::endl;
	#endif

	std::cout << ">> Loading config (" << g_config.getString(ConfigManager::CONFIG_FILE) << ")" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading config");
	#endif
	if(!g_config.load())
		startupErrorMessage("Unable to load " + g_config.getString(ConfigManager::CONFIG_FILE) + "!");

	#ifdef WIN32
	std::string defaultPriority = asLowerCaseString(g_config.getString(ConfigManager::DEFAULT_PRIORITY));
	if(defaultPriority == "realtime")
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	else if(defaultPriority == "high")
  	 	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
  	else if(defaultPriority == "higher")
  		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
  	#endif

	std::string passwordType = asLowerCaseString(g_config.getString(ConfigManager::PASSWORD_TYPE));
	if(passwordType == "md5")
	{
		g_config.setNumber(ConfigManager::PASSWORDTYPE, PASSWORD_TYPE_MD5);
		std::cout << "> Using MD5 passwords" << std::endl;
	}
	else if(passwordType == "sha1")
	{
		g_config.setNumber(ConfigManager::PASSWORDTYPE, PASSWORD_TYPE_SHA1);
		std::cout << "> Using SHA1 passwords" << std::endl;
	}
	else
	{
		g_config.setNumber(ConfigManager::PASSWORDTYPE, PASSWORD_TYPE_PLAIN);
		std::cout << "> Using plaintext passwords" << std::endl;
	}

	std::cout << ">> Loading RSA key" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading RSA Key");
	#endif
	const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
	const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
	const char* d("46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");
	g_otservRSA = new RSA();
	g_otservRSA->setKey(p, q, d);

	std::cout << ">> Starting SQL connection" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Starting SQL connection");
	#endif
	Database* db = Database::getInstance();
	if(db == NULL || !db->isConnected())
		startupErrorMessage("Couldn't estabilish connection to SQL database!");
	else
	{
		std::cout << ">> Running Database Manager" << std::endl;
		if(!DatabaseManager::getInstance()->isDatabaseSetup())
			startupErrorMessage("The database you specified in config.lua is empty, please import schema.<dbengine> to the database (if you are using MySQL, please read doc/MYSQL_HELP for more information).");
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
		DatabaseManager::getInstance()->checkPasswordType();
		if(g_config.getBool(ConfigManager::OPTIMIZE_DB_AT_STARTUP) && !DatabaseManager::getInstance()->optimizeTables())
			std::cout << "> No tables were optimized." << std::endl;
	}

	std::cout << ">> Loading items" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading items");
	#endif
	if(Item::items.loadFromOtb(getFilePath(FILE_TYPE_OTHER, "items/items.otb")))
		startupErrorMessage("Unable to load items (OTB)!");

	if(!Item::items.loadFromXml())
	{
		#ifndef __CONSOLE__
		if(MessageBox(GUI::getInstance()->m_mainWindow, "Unable to load items (XML)! Continue?", "Items (XML)", MB_YESNO) == IDNO)
		#else
		std::cout << "Unable to load items (XML)! Continue? (y/N)" << std::endl;
		char buffer = getchar();
		if(buffer == 10 || (buffer != 121 && buffer != 89))
		#endif
			startupErrorMessage("Unable to load items (XML)!");
	}

	std::cout << ">> Loading vocations" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading vocations");
	#endif
	if(!g_vocations.loadFromXml())
		startupErrorMessage("Unable to load vocations!");

	std::cout << ">> Loading script systems" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading script systems");
	#endif
	if(!ScriptingManager::getInstance()->loadScriptSystems())
		startupErrorMessage("");

	std::cout << ">> Loading outfits" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading outfits");
	#endif
	Outfits* outfits = Outfits::getInstance();
	if(!outfits->loadFromXml())
		startupErrorMessage("Unable to load outfits!");

	std::cout << ">> Loading experience stages" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading experience stages");
	#endif
	if(!g_game.loadExperienceStages())
		startupErrorMessage("Unable to load experience stages!");

	std::cout << ">> Loading monsters" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading monsters");
	#endif
	if(!g_monsters.loadFromXml())
	{
		#ifndef __CONSOLE__
		if(MessageBox(GUI::getInstance()->m_mainWindow, "Unable to load monsters! Continue?", "Monsters", MB_YESNO) == IDNO)
		#else
		std::cout << "Unable to load monsters! Continue? (y/N)" << std::endl;
		char buffer = getchar();
		if(buffer == 10 || (buffer != 121 && buffer != 89))
		#endif
			startupErrorMessage("Unable to load monsters!");
	}

	std::cout << ">> Loading map and spawns..." << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading map and spawns...");
	#endif
	if(!g_game.loadMap(g_config.getString(ConfigManager::MAP_NAME)))
		startupErrorMessage("");

	#ifdef __LOGIN_SERVER__
	std::cout << ">> Loading game servers" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading game servers");
	#endif
	if(!GameServers::getInstance()->loadFromXml(true))
		startupErrorMessage("Unable to load game servers!");
	#endif

	#ifdef __REMOTE_CONTROL__
	g_admin = new Admin();
	std::cout << ">> Loading administration protocol" << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading administration protocol");
	#endif
	if(!g_admin->loadXMLConfig())
		startupErrorMessage("Unable to load administration protocol!");
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

	std::cout << ">> Checking software version... ";
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

	if(Status* status = Status::getInstance())
	{
		status->setMaxPlayersOnline(g_config.getNumber(ConfigManager::MAX_PLAYERS));
		status->setMapAuthor(g_config.getString(ConfigManager::MAP_AUTHOR));
		status->setMapName(g_config.getString(ConfigManager::MAP_NAME));
	}

	std::cout << ">> All modules were loaded, server starting up..." << std::endl;
	#ifndef __CONSOLE__
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> All modules were loaded, server starting up...");
	#endif
	g_game.setGameState(GAME_STATE_INIT);

	if(g_config.getBool(ConfigManager::GLOBALSAVE_ENABLED) && g_config.getNumber(ConfigManager::GLOBALSAVE_H) >= 1 && g_config.getNumber(ConfigManager::GLOBALSAVE_H) <= 24)
	{
		int32_t prepareGlobalSaveHour = g_config.getNumber(ConfigManager::GLOBALSAVE_H) - 1, hoursLeft = 0, minutesLeft = 0, minutesToRemove = 0;
		bool ignoreEvent = false;
		time_t timeNow = time(NULL);
		const tm* theTime = localtime(&timeNow);
		if(theTime->tm_hour > prepareGlobalSaveHour)
		{
			hoursLeft = 24 - (theTime->tm_hour - prepareGlobalSaveHour);
			if(theTime->tm_min > 55 && theTime->tm_min <= 59)
				minutesToRemove = theTime->tm_min - 55;
			else
				minutesLeft = 55 - theTime->tm_min;
		}
		else if(theTime->tm_hour == prepareGlobalSaveHour)
		{
			if(theTime->tm_min >= 55 && theTime->tm_min <= 59)
			{
				if(theTime->tm_min >= 57)
					g_game.setGlobalSaveMessage(0, true);

				if(theTime->tm_min == 59)
					g_game.setGlobalSaveMessage(1, true);

				g_game.prepareGlobalSave();
				ignoreEvent = true;
			}
			else
				minutesLeft = 55 - theTime->tm_min;
		}
		else
		{
			hoursLeft = prepareGlobalSaveHour - theTime->tm_hour;
			if(theTime->tm_min > 55 && theTime->tm_min <= 59)
				minutesToRemove = theTime->tm_min - 55;
			else
				minutesLeft = 55 - theTime->tm_min;
		}

		int32_t hoursLeftInMS = 60000 * 60 * hoursLeft;
		uint32_t minutesLeftInMS = 60000 * (minutesLeft - minutesToRemove);
		if(!ignoreEvent && (hoursLeftInMS + minutesLeftInMS) > 0)
			Scheduler::getScheduler().addEvent(createSchedulerTask(hoursLeftInMS + minutesLeftInMS, boost::bind(&Game::prepareGlobalSave, &g_game)));
	}

	serverIPs.push_back(std::make_pair(inet_addr("127.0.0.1"), 0xFFFFFFFF));
	char hostName[128];
	if(gethostname(hostName, 128) == 0)
	{
		hostent *host = gethostbyname(hostName);
		if(host)
		{
			uint8_t** address = (uint8_t**)host->h_addr_list;
			while(address[0] != NULL)
			{
				serverIPs.push_back(std::make_pair(*(uint32_t*)(*address), 0x0000FFFF));
				address++;
			}
		}
	}

	std::string ip = g_config.getString(ConfigManager::IP);
	uint32_t resolvedIp = inet_addr(ip.c_str());
	if(resolvedIp == INADDR_NONE)
	{
		hostent *host = gethostbyname(ip.c_str());
		if(host != 0)
			resolvedIp = *(uint32_t*)host->h_addr;
		else
			startupErrorMessage("Cannot resolve " + ip + "!");
	}

	serverIPs.push_back(std::make_pair(resolvedIp, 0));
	g_game.setGameState(GAME_STATE_NORMAL);
	OTSYS_THREAD_SIGNAL_SEND(g_loaderSignal);
}

#ifndef __CONSOLE__
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CInputBox iBox(hwnd);
	switch(message)
	{
		case WM_CREATE:
		{
			GUI::getInstance()->m_logWindow = CreateWindow("edit", NULL,
				WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE | ES_MULTILINE | DS_CENTER, 0, 0, 700, 400, hwnd, (HMENU)ID_LOG, NULL, NULL);
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
			OTSYS_CREATE_THREAD(serverMain, hwnd);
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
						std::cout << "A server developed by Elf, Talaturen, Lithium, Kiper, Kornholijo, Jonern & Nightmare." << std::endl;
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
						Dispatcher::getDispatcher().addTask(
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
						Dispatcher::getDispatcher().addTask(createTask(
							boost::bind(&Game::saveGameState, &g_game, true)));
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
						Dispatcher::getDispatcher().addTask(createTask(
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
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_ACTIONS))
							std::cout << "Reloaded actions." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_CONFIG:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_CONFIG))
							std::cout << "Reloaded config." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_CREATUREEVENTS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_CREATUREEVENTS))
							std::cout << "Reloaded creature events." << std::endl;
					}
					break;
				#ifdef __LOGIN_SERVER__
				case ID_MENU_RELOAD_GAMESERVERS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_GAMESERVERS))
							std::cout << "Reloaded game servers." << std::endl;
					}
					break;
				#endif
				case ID_MENU_RELOAD_GLOBALEVENTS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_GLOBALEVENTS))
							std::cout << "Reloaded global events." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_HIGHSCORES:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_HIGHSCORES))
							std::cout << "Reloaded highscores." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_HOUSEPRICES:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_HOUSEPRICES))
							std::cout << "Reloaded house prices." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_ITEMS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_ITEMS))
							std::cout << "Reloaded items." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_MONSTERS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_MONSTERS))
							std::cout << "Reloaded monsters." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_MOVEMENTS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_MOVEEVENTS))
							std::cout << "Reloaded movements." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_NPCS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_NPCS))
							std::cout << "Reloaded npcs." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_OUTFITS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_OUTFITS))
							std::cout << "Reloaded outfits." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_QUESTS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_QUESTS))
							std::cout << "Reloaded quests." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_RAIDS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_RAIDS))
							std::cout << "Reloaded raids." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_SPELLS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_SPELLS))
							std::cout << "Reloaded spells." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_STAGES:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_STAGES))
							std::cout << "Reloaded stages." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_TALKACTIONS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_TALKACTIONS))
							std::cout << "Reloaded talk actions." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_VOCATIONS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_VOCATIONS))
							std::cout << "Reloaded vocations." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_WEAPONS:
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_WEAPONS))
							std::cout << "Reloaded weapons." << std::endl;
					}
					break;
				case ID_MENU_RELOAD_ALL:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadInfo(RELOAD_ALL))
							std::cout << "Reloaded all." << std::endl;
					}
					break;
				}
			}
		break;
		case WM_CLOSE:
		case WM_DESTROY:
			if(MessageBox(hwnd, "Are you sure you want to shutdown the server?", "Shutdown", MB_YESNO) == IDYES)
			{
				Shell_NotifyIcon(NIM_DELETE, &NID);
				Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_SHUTDOWN)));
			}
			break;
		break;
		case WM_USER + 1: // tray icon messages
		{
			switch(lParam)
			{
				case WM_RBUTTONUP: // right click
				{
					POINT mp;
					GetCursorPos(&mp);
					TrackPopupMenu(GetSubMenu(GUI::getInstance()->m_trayMenu, 0), 0, mp.x, mp.y, 0, hwnd, 0);
				}
					break;
				case WM_LBUTTONUP: // left click
				{
					if(GUI::getInstance()->m_minimized)
					{
						ShowWindow(hwnd, SW_SHOW);
						ShowWindow(hwnd, SW_RESTORE);
						ModifyMenu(GUI::getInstance()->m_trayMenu, ID_TRAY_HIDE, MF_STRING, ID_TRAY_HIDE, "&Hide window");
						GUI::getInstance()->m_minimized = false;
					}
				}
					break;
			}
		}
		break;
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
	wincl.hIcon  = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON));
	wincl.hIconSm  = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON), IMAGE_ICON, 16, 16, 0);
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.lpszMenuName = MAKEINTRESOURCE(ID_MENU);
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	if(!RegisterClassEx(&wincl))
		return 0;

	GUI::getInstance()->m_mainWindow = CreateWindowEx(0, "forgottenserver_gui", STATUS_SERVER_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 707, 453, HWND_DESKTOP, NULL, hInstance, NULL);
	ShowWindow(GUI::getInstance()->m_mainWindow, 1);
	while(GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return messages.wParam;
}
#endif
