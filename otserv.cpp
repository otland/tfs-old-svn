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

#include "definitions.h"
#include <boost/asio.hpp>
#include "server.h"

#include <string>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <iomanip>

#include "otsystem.h"
#include "networkmessage.h"
#include "protocolgame.h"

#include <stdlib.h>
#include <time.h>
#include "game.h"

#include "iologindata.h"
#include "iomarket.h"

#if !defined(__WINDOWS__)
#include <signal.h> // for sigemptyset()
#endif

#include "monsters.h"
#include "commands.h"
#include "outfit.h"
#include "vocation.h"
#include "scriptmanager.h"
#include "configmanager.h"

#include "tools.h"
#include "ban.h"
#include "rsa.h"

#ifndef _CONSOLE
#ifdef WIN32
#include "shellapi.h"
#include "gui.h"
#include "textlogger.h"
#include "inputbox.h"
#include "commctrl.h"
#include "spells.h"
#include "movement.h"
#include "talkaction.h"
#include "raids.h"
#include "quests.h"
#endif
#else
#include "resources.h"
#endif

#include "protocolgame.h"
#include "protocolold.h"
#include "protocollogin.h"
#include "status.h"
#include "admin.h"
#include "globalevent.h"
#include "mounts.h"

#ifdef __OTSERV_ALLOCATOR__
#include "allocator.h"
#endif

#include "databasemanager.h"

#ifdef BOOST_NO_EXCEPTIONS
	#include <exception>
	void boost::throw_exception(std::exception const & e)
	{
		std::cout << "Boost exception: " << e.what() << std::endl;
	}
#endif

Dispatcher g_dispatcher;
Scheduler g_scheduler;

IPList serverIPs;

extern AdminProtocolConfig* g_adminConfig;
Ban g_bans;
Game g_game;
Commands commands;
Npcs g_npcs;
ConfigManager g_config;
Monsters g_monsters;
Vocations g_vocations;
RSA g_RSA;

#ifndef _CONSOLE
#ifdef _WIN32
NOTIFYICONDATA NID;
TextLogger logger;
extern Actions* g_actions;
extern CreatureEvents* g_creatureEvents;
extern GlobalEvents* g_globalEvents;
extern MoveEvents* g_moveEvents;
extern Spells* g_spells;
extern TalkActions* g_talkActions;
#endif
#endif

OTSYS_THREAD_LOCKVAR g_loaderLock;
OTSYS_THREAD_SIGNALVAR g_loaderSignal;

#ifdef __EXCEPTION_TRACER__
#include "exception.h"
#endif
#include "networkmessage.h"

void startupErrorMessage(std::string errorStr)
{
	if(errorStr.length() > 0)
		std::cout << "> ERROR: " << errorStr << std::endl;

	#ifdef WIN32
	#ifndef _CONSOLE
	system("pause");
	#endif
	#else
	getchar();
	#endif
	exit(-1);
}

void mainLoader(
#ifdef _CONSOLE
	int argc, char *argv[],
#endif
	ServiceManager* servicer
);

void badAllocationHandler()
{
	// Use functions that only use stack allocation
	puts("Allocation failed, server out of memory.\nDecrease the size of your map or compile in 64 bits mode.");
	char buf[1024];
	assert(fgets(buf, sizeof(buf), stdin) != 0);
	exit(-1);
}

#ifndef _CONSOLE
void serverMain(void* param)
#else
int main(int argc, char *argv[])
#endif
{
	// Setup bad allocation handler
	std::set_new_handler(badAllocationHandler);

	#ifdef _WIN32
	#ifndef _CONSOLE
	std::cout.rdbuf(&logger);
	#endif
	#endif
	#ifdef __OTSERV_ALLOCATOR_STATS__
	OTSYS_CREATE_THREAD(allocatorStatsThread, NULL);
	#endif

	// Provides stack traces when the server crashes, if compiled in.
	#ifdef __EXCEPTION_TRACER__
	ExceptionHandler mainExceptionHandler;
	mainExceptionHandler.InstallHandler();
	#endif

	// ignore sigpipe...
	#ifdef WIN32
	//nothing yet
	#else
	struct sigaction sigh;
	sigh.sa_handler = SIG_IGN;
	sigh.sa_flags = 0;
	sigemptyset(&sigh.sa_mask);
	sigaction(SIGPIPE, &sigh, NULL);
	#endif

	OTSYS_THREAD_LOCKVARINIT(g_loaderLock);
	OTSYS_THREAD_SIGNALVARINIT(g_loaderSignal);

	ServiceManager servicer;

	g_dispatcher.start();
	g_scheduler.start();

	g_dispatcher.addTask(createTask(boost::bind(mainLoader,
#ifdef _CONSOLE
		argc, argv,
#endif
		&servicer)));

	OTSYS_THREAD_LOCK(g_loaderLock, "main()");
	OTSYS_THREAD_WAITSIGNAL(g_loaderSignal, g_loaderLock);
	OTSYS_SLEEP(1000);

	if(servicer.is_running())
	{
		std::cout << ">> " << g_config.getString(ConfigManager::SERVER_NAME) << " Server Online!" << std::endl << std::endl;
		#ifndef _CONSOLE
		SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Status: Online!");
		GUI::getInstance()->m_connections = true;
		#endif
		servicer.run();
		g_scheduler.join();
		g_dispatcher.join();
	}
	else
	{
		std::cout << ">> No services running. The server is NOT online." << std::endl;
		#ifndef _CONSOLE
		SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Status: Offline (no services running)!");
		#endif
	}

#ifdef __EXCEPTION_TRACER__
	mainExceptionHandler.RemoveHandler();
#endif
#ifndef _CONSOLE
	exit(EXIT_SUCCESS);
#else
	return 0;
#endif
}

#ifdef _CONSOLE
void mainLoader(int argc, char *argv[], ServiceManager* services)
#else
void mainLoader(ServiceManager* services)
#endif
{
	//dispatcher thread
	g_game.setGameState(GAME_STATE_STARTUP);

	srand((unsigned int)OTSYS_TIME());
	#ifdef WIN32
	#ifdef _CONSOLE
	SetConsoleTitle(STATUS_SERVER_NAME);
	#endif
	#endif
	std::cout << STATUS_SERVER_NAME << " - Version " << STATUS_SERVER_VERSION << " (" << STATUS_SERVER_CODENAME << ")." << std::endl;
	std::cout << "Compilied on " << __DATE__ << " " << __TIME__ << " for arch ";

	#if defined(__amd64__) || defined(_M_X64)
	std::cout << "x64" << std::endl;
	#elif defined(__i386__) || defined(_M_IX86) || defined(_X86_)
	std::cout << "x86" << std::endl;
	#else
	std::cout << "unk" << std::endl;
	#endif

	std::cout << std::endl;

	std::cout << "A server developed by " << STATUS_SERVER_DEVELOPERS << "." << std::endl;
	std::cout << "Visit our forum for updates, support, and resources: http://otland.net/." << std::endl;

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

	#ifndef _CONSOLE
	GUI::getInstance()->m_connections = false;
	#endif

	#if !defined(_WIN32) && !defined(__ROOT_PERMISSION__)
	if(getuid() == 0 || geteuid() == 0)
		std::cout << "> WARNING: " << STATUS_SERVER_NAME << " has been executed as root user, it is recommended to execute is as a normal user." << std::endl;
	#endif

	std::cout << std::endl;

	// read global config
	std::cout << ">> Loading config" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading config");
	#endif
	#if !defined(WIN32) && !defined(__NO_HOMEDIR_CONF__)
	std::string configpath;
	configpath = getenv("HOME");
	configpath += "/.otserv/config.lua";
	if(!g_config.loadFile(configpath))
	#else
	if(!g_config.loadFile("config.lua"))
	#endif
	{
		startupErrorMessage("Unable to load config.lua!");
		return;
	}

	#ifdef WIN32
	std::string defaultPriority = asLowerCaseString(g_config.getString(ConfigManager::DEFAULT_PRIORITY));
	if(defaultPriority == "realtime")
		SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	else if(defaultPriority == "high")
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else if(defaultPriority == "higher")
		SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
	#endif

	//set RSA key
	const char* p("14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
	const char* q("7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
	const char* d("46730330223584118622160180015036832148732986808519344675210555262940258739805766860224610646919605860206328024326703361630109888417839241959507572247284807035235569619173792292786907845791904955103601652822519121908367187885509270025388641700821735345222087940578381210879116823013776808975766851829020659073");
	g_RSA.setKey(p, q, d);

	std::cout << ">> Loading database driver..." << std::flush;
	Database* db = Database::getInstance();
	if(!db->isConnected())
	{
		switch(db->getDatabaseEngine())
		{
			case DATABASE_ENGINE_MYSQL:
				startupErrorMessage("Failed to connect to database, read doc/MYSQL_HELP for information or try SQLite which doesn't require any connection.");
				return;

			case DATABASE_ENGINE_SQLITE:
				startupErrorMessage("Failed to connect to sqlite database file, make sure it exists and is readable.");
				return;

			default:
				startupErrorMessage("Unkwown sqlType in config.lua, valid sqlTypes are: \"mysql\" and \"sqlite\".");
				return;
		}
	}
	std::cout << " " << db->getClientName() << " " << db->getClientVersion() << std::endl;

	// run database manager
	std::cout << ">> Running database manager" << std::endl;
	DatabaseManager* dbManager = DatabaseManager::getInstance();
	if(!dbManager->isDatabaseSetup())
	{
		startupErrorMessage("The database you have specified in config.lua is empty, please import the schema to the database.");
		return;
	}

	for(uint32_t version = dbManager->updateDatabase(); version != 0; version = dbManager->updateDatabase())
		std::cout << "> Database has been updated to version " << version << "." << std::endl;

	dbManager->checkTriggers();
	dbManager->checkEncryption();

	if(g_config.getBoolean(ConfigManager::OPTIMIZE_DATABASE) && !dbManager->optimizeTables())
		std::cout << "> No tables were optimized." << std::endl;

	//load bans
	std::cout << ">> Loading bans" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading bans");
	#endif

	g_bans.init();

	//load vocations
	std::cout << ">> Loading vocations" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading vocations");
	#endif
	if(!g_vocations.loadFromXml())
		startupErrorMessage("Unable to load vocations!");

	//load commands
	std::cout << ">> Loading commands" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading commands");
	#endif
	if(!commands.loadFromXml())
		startupErrorMessage("Unable to load commands!");

	// load item data
	std::cout << ">> Loading items" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading items");
	#endif
	if(Item::items.loadFromOtb("data/items/items.otb"))
		startupErrorMessage("Unable to load items (OTB)!");

	if(!Item::items.loadFromXml())
	{
		#if defined(_WIN32) && !defined(_CONSOLE)
		if(MessageBoxA(GUI::getInstance()->m_mainWindow, "Unable to load items (XML)! Continue?", "Items (XML)", MB_YESNO) == IDNO)
		#endif
			startupErrorMessage("Unable to load items (XML)!");
	}

	std::cout << ">> Loading script systems" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading script systems");
	#endif
	if(!ScriptingManager::getInstance()->loadScriptSystems())
		startupErrorMessage("");

	std::cout << ">> Loading monsters" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading monsters");
	#endif
	if(!g_monsters.loadFromXml())
	{
		#ifndef _CONSOLE
		if(MessageBoxA(GUI::getInstance()->m_mainWindow, "Unable to load monsters! Continue?", "Monsters", MB_YESNO) == IDNO)
		#endif
			startupErrorMessage("Unable to load monsters!");
	}

	std::cout << ">> Loading outfits" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading outfits");
	#endif
	Outfits* outfits = Outfits::getInstance();
	if(!outfits->loadFromXml())
		startupErrorMessage("Unable to load outfits!");

	g_adminConfig = new AdminProtocolConfig();
	std::cout << ">> Loading admin protocol config" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading admin protocol config");
	#endif
	if(!g_adminConfig->loadXMLConfig())
		startupErrorMessage("Unable to load admin protocol config!");

	std::cout << ">> Loading experience stages" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading experience stages");
	#endif
	if(!g_game.loadExperienceStages())
		startupErrorMessage("Unable to load experience stages!");

	std::string passwordType = asLowerCaseString(g_config.getString(ConfigManager::PASSWORDTYPE));
	if(passwordType == "md5")
	{
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_MD5);
		std::cout << ">> Using MD5 passwords" << std::endl;
	}
	else if(passwordType == "sha1")
	{
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_SHA1);
		std::cout << ">> Using SHA1 passwords" << std::endl;
	}
	else
	{
		g_config.setNumber(ConfigManager::PASSWORD_TYPE, PASSWORD_TYPE_PLAIN);
		std::cout << ">> Using plaintext passwords" << std::endl;
	}

	std::cout << ">> Checking world type... ";
	std::string worldType = asLowerCaseString(g_config.getString(ConfigManager::WORLD_TYPE));
	if(worldType == "pvp")
		g_game.setWorldType(WORLD_TYPE_PVP);
	else if(worldType == "no-pvp")
		g_game.setWorldType(WORLD_TYPE_NO_PVP);
	else if(worldType == "pvp-enforced")
		g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
	else
	{
		std::cout << std::endl << "> ERROR: Unknown world type: " << g_config.getString(ConfigManager::WORLD_TYPE) << std::endl;
		startupErrorMessage("");
	}
	std::cout << asUpperCaseString(worldType) << std::endl;

	Status* status = Status::getInstance();
	status->setMaxPlayersOnline(g_config.getNumber(ConfigManager::MAX_PLAYERS));
	status->setMapAuthor(g_config.getString(ConfigManager::MAP_AUTHOR));
	status->setMapName(g_config.getString(ConfigManager::MAP_NAME));

	std::cout << ">> Loading map" << std::endl;
	#ifndef _CONSOLE
	SendMessage(GUI::getInstance()->m_statusBar, WM_SETTEXT, 0, (LPARAM)">> Loading map");
	#endif
	if(!g_game.loadMap(g_config.getString(ConfigManager::MAP_NAME)))
		startupErrorMessage("");

	std::cout << ">> Initializing gamestate" << std::endl;
	g_game.setGameState(GAME_STATE_INIT);

	// Tibia protocols
	services->add<ProtocolGame>(g_config.getNumber(ConfigManager::GAME_PORT));
	services->add<ProtocolLogin>(g_config.getNumber(ConfigManager::LOGIN_PORT));

	// OT protocols
	services->add<ProtocolAdmin>(g_config.getNumber(ConfigManager::ADMIN_PORT));
	services->add<ProtocolStatus>(g_config.getNumber(ConfigManager::STATUS_PORT));

	// Legacy protocols
	services->add<ProtocolOldLogin>(g_config.getNumber(ConfigManager::LOGIN_PORT));
	services->add<ProtocolOldGame>(g_config.getNumber(ConfigManager::LOGIN_PORT));

	g_game.timedHighscoreUpdate();

	int32_t autoSaveEachMinutes = g_config.getNumber(ConfigManager::AUTO_SAVE_EACH_MINUTES);
	if(autoSaveEachMinutes > 0)
		g_scheduler.addEvent(createSchedulerTask(autoSaveEachMinutes * 1000 * 60, boost::bind(&Game::autoSave, &g_game)));

	if(g_config.getBoolean(ConfigManager::SERVERSAVE_ENABLED))
	{
		int32_t serverSaveHour = g_config.getNumber(ConfigManager::SERVERSAVE_H);
		if(serverSaveHour >= 0 && serverSaveHour <= 24)
		{
			time_t timeNow = time(NULL);
			tm* timeinfo = localtime(&timeNow);

			if(serverSaveHour == 0)
				serverSaveHour = 23;
			else
				serverSaveHour--;

			timeinfo->tm_hour = serverSaveHour;
			timeinfo->tm_min = 55;
			timeinfo->tm_sec = 0;
			time_t difference = (time_t)difftime(mktime(timeinfo), timeNow);
			if(difference < 0)
				difference += 86400;

			g_scheduler.addEvent(createSchedulerTask(difference * 1000, boost::bind(&Game::prepareServerSave, &g_game)));
		}
	}

	IOGuild::getInstance()->removePending();
	g_npcs.reload();

	if(g_config.getBoolean(ConfigManager::MARKET_ENABLED))
	{
		g_game.checkExpiredMarketOffers();
		IOMarket::getInstance()->updateStatistics();
	}

	std::cout << ">> Loaded all modules, server starting up..." << std::endl;

	std::pair<uint32_t, uint32_t> IpNetMask;
	IpNetMask.first = inet_addr("127.0.0.1");
	IpNetMask.second = 0xFFFFFFFF;
	serverIPs.push_back(IpNetMask);

	char szHostName[128];
	if(gethostname(szHostName, 128) == 0)
	{
		hostent *he = gethostbyname(szHostName);
		if(he)
		{
			unsigned char** addr = (unsigned char**)he->h_addr_list;
			while(addr[0] != NULL)
			{
				IpNetMask.first = *(uint32_t*)(*addr);
				IpNetMask.second = 0xFFFFFFFF;
				serverIPs.push_back(IpNetMask);
				addr++;
			}
		}
	}
	std::string ip;
	ip = g_config.getString(ConfigManager::IP);

	uint32_t resolvedIp = inet_addr(ip.c_str());
	if(resolvedIp == INADDR_NONE)
	{
		struct hostent* he = gethostbyname(ip.c_str());
		if(he != 0)
			resolvedIp = *(uint32_t*)he->h_addr;
		else
		{
			std::cout << "ERROR: Cannot resolve " << ip << "!" << std::endl;
			startupErrorMessage("");
		}
	}

	IpNetMask.first = resolvedIp;
	IpNetMask.second = 0;
	serverIPs.push_back(IpNetMask);

	#if !defined(WIN32) && !defined(__ROOT_PERMISSION__)
	if(getuid() == 0 || geteuid() == 0)
		std::cout << "> WARNING: " << STATUS_SERVER_NAME << " has been executed as root user, it is recommended to execute is as a normal user." << std::endl;
	#endif

	IOLoginData::getInstance()->resetOnlineStatus();
	g_game.start(services);
	g_game.setGameState(GAME_STATE_NORMAL);
	OTSYS_THREAD_SIGNAL_SEND(g_loaderSignal);
}

#ifndef _CONSOLE
LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CInputBox iBox(hwnd);
	switch(message)
	{
		case WM_CREATE:
		{
			GUI::getInstance()->m_logWindow = CreateWindow("edit", NULL, WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE | ES_MULTILINE | DS_CENTER, 0, 0, 700, 400, hwnd, (HMENU)ID_LOG, NULL, NULL);
			GUI::getInstance()->m_statusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, (HMENU)ID_STATUS_BAR, GetModuleHandle(NULL), NULL);
			int32_t statusBarWidthLine[] = {150, -1};
			GUI::getInstance()->m_lineCount = 0;
			SendMessage(GUI::getInstance()->m_statusBar, SB_SETPARTS, sizeof(statusBarWidthLine)/sizeof(int32_t), (LPARAM)statusBarWidthLine);
			SendMessage(GUI::getInstance()->m_statusBar, SB_SETTEXT, 0, (LPARAM)"Not loaded");
			GUI::getInstance()->m_minimized = false;
			GUI::getInstance()->m_pBox.setParent(hwnd);
			SendMessage(GUI::getInstance()->m_logWindow, WM_SETFONT, (WPARAM)GUI::getInstance()->m_font, 0);
			NID.hWnd = hwnd;
			NID.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICON));
			NID.uCallbackMessage = WM_USER+1;
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

				case ID_MENU_GAME_BROADCAST:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(iBox.DoModal("Broadcast Message", "What would you like to broadcast?"))
							g_game.broadcastMessage(iBox.Text, MSG_STATUS_WARNING);
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_ACTIONS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_actions->reload())
							std::cout << "Reloaded actions." << std::endl;
						else
							std::cout << "Failed to reload actions." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_CREATUREEVENTS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_creatureEvents->reload())
							std::cout << "Reloaded creature events." << std::endl;
						else
							std::cout << "Failed to reload creature events." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_COMMANDS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(commands.reload())
							std::cout << "Reloaded commands." << std::endl;
						else
							std::cout << "Failed to reload commands." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_CONFIG:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_config.reload())
							std::cout << "Reloaded config." << std::endl;
						else
							std::cout << "Failed to reload config." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_HIGHSCORES:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_game.reloadHighscores())
							std::cout << "Reloaded highscores." << std::endl;
						else
							std::cout << "Failed to reload highscores." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_MONSTERS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_monsters.reload())
							std::cout << "Reloaded monsters." << std::endl;
						else
							std::cout << "Failed to reload monsters." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_MOUNTS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(Mounts::getInstance()->reload())
							std::cout << "Reloaded mounts." << std::endl;
						else
							std::cout << "Failed to reload mounts." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_MOVEMENTS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_moveEvents->reload())
							std::cout << "Reloaded movements." << std::endl;
						else
							std::cout << "Failed to reload movements." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_NPCS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						g_npcs.reload();
						std::cout << "Reloaded npcs." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_QUESTS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(Quests::getInstance()->reload())
							std::cout << "Reloaded quests." << std::endl;
						else
							std::cout << "Failed to reload quests." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_RAIDS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(Raids::getInstance()->reload() && Raids::getInstance()->startup())
							std::cout << "Reloaded raids." << std::endl;
						else
							std::cout << "Failed to reload raids." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_SPELLS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_spells->reload() && g_monsters.reload())
							std::cout << "Reloaded spells." << std::endl;
						else
							std::cout << "Failed to reload spells." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_TALKACTIONS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_talkActions->reload())
							std::cout << "Reloaded talkactions." << std::endl;
						else
							std::cout << "Failed to reload talkactions." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_GLOBALEVENTS:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						if(g_globalEvents->reload())
							std::cout << "Reloaded globalevents." << std::endl;
						else
							std::cout << "Failed to reload globalevents." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_RELOAD_RELOADALL:
				{
					if(g_game.getGameState() == GAME_STATE_STARTUP)
						break;

					if(!g_config.reload())
						std::cout << "Failed to reload config." << std::endl;

					if(!g_monsters.reload())
						std::cout << "Failed to reload monsters." << std::endl;

					if(!commands.reload())
						std::cout << "Failed to reload commands." << std::endl;

					if(!Quests::getInstance()->reload())
						std::cout << "Failed to reload quests." << std::endl;

					if(!Mounts::getInstance()->reload())
						std::cout << "Failed to reload mounts." << std::endl;

					if(!g_game.reloadHighscores())
						std::cout << "Failed to reload highscores." << std::endl;

					if(!g_actions->reload())
						std::cout << "Failed to reload actions." << std::endl;

					if(!g_moveEvents->reload())
						std::cout << "Failed to reload movements." << std::endl;

					if(!g_talkActions->reload())
						std::cout << "Failed to reload talkactions." << std::endl;

					if(!g_spells->reload())
						std::cout << "Failed to reload spells." << std::endl;

					if(!g_creatureEvents->reload())
						std::cout << "Failed to reload creature events." << std::endl;

					if(!Raids::getInstance()->reload() || !Raids::getInstance()->startup())
						std::cout << "Failed to reload raids." << std::endl;

					if(!g_globalEvents->reload())
						std::cout << "Failed to reload global events." << std::endl;

					g_npcs.reload();
					std::cout << "Reloaded all." << std::endl;
					break;
				}

				case ID_MENU_GAME_WORLDTYPE_PVP:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						g_game.setWorldType(WORLD_TYPE_PVP);
						std::cout << "WorldType set to 'PVP'." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_WORLDTYPE_NOPVP:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						g_game.setWorldType(WORLD_TYPE_NO_PVP);
						std::cout << "WorldType set to 'Non PVP'." << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_WORLDTYPE_PVPENFORCED:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						g_game.setWorldType(WORLD_TYPE_PVP_ENFORCED);
						std::cout << "WorldType set to 'PVP Enforced'." << std::endl;
					}
					break;
				}

				case ID_MENU_FILE_CLEARLOG:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						GUI::getInstance()->m_logText = "";
						GUI::getInstance()->m_lineCount = 0;
						std::cout << STATUS_SERVER_NAME << " - Version " << STATUS_SERVER_VERSION << " (" << STATUS_SERVER_CODENAME << ")." << std::endl;
						std::cout << "A server developed by " << STATUS_SERVER_DEVELOPERS << "." << std::endl;
						std::cout << "Visit our forum for updates, support and resources: http://otland.net/." << std::endl << std::endl;
					}
					break;
				}

				case ID_MENU_GAME_ACCEPT:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP && !GUI::getInstance()->m_connections)
					{
						GUI::getInstance()->m_connections = true;
						ModifyMenu(GetMenu(hwnd), ID_MENU_GAME_ACCEPT, MF_STRING, ID_MENU_GAME_REJECT, "&Reject Connections");
					}
					break;
				}

				case ID_MENU_GAME_REJECT:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP && GUI::getInstance()->m_connections)
					{
						GUI::getInstance()->m_connections = false;
						ModifyMenu(GetMenu(hwnd), ID_MENU_GAME_REJECT, MF_STRING, ID_MENU_GAME_ACCEPT, "&Accept Connections");
					}
					break;
				}

				case ID_TRAY_SHUTDOWN:
				case ID_MENU_FILE_SHUTDOWN:
				{
					if(MessageBoxA(hwnd, "Are you sure you want to shutdown the server?", "Shutdown", MB_YESNO) == IDYES)
					{
						g_dispatcher.addTask(
							createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_SHUTDOWN)));
						Shell_NotifyIcon(NIM_DELETE, &NID);
					}
					break;
				}

				case ID_MENU_GAME_PLAYERS_LIST:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP && GUI::getInstance()->m_connections)
					{
						int32_t playersOnline = g_game.getPlayersOnline();
						if(playersOnline == 0)
							MessageBoxA(NULL, "No players online.", "Player List", MB_OK);
						else
							GUI::getInstance()->m_pBox.popUp("Player List");
					}
				}
				break;

				case ID_MENU_GAME_PLAYERS_SAVE:
				{
					if(g_game.getGameState() != GAME_STATE_STARTUP)
					{
						g_dispatcher.addTask(
							createTask(boost::bind(&Game::saveGameState, &g_game)));
						MessageBoxA(NULL, "The players online have been saved.", "Save players", MB_OK);
					}
				}
				break;
			}
		}
		break;

		case WM_CLOSE:
		case WM_DESTROY:
		{
			if(MessageBoxA(hwnd, "Are you sure you want to shutdown the server?", "Shutdown", MB_YESNO) == IDYES)
			{
				g_dispatcher.addTask(
					createTask(boost::bind(&Game::setGameState, &g_game, GAME_STATE_SHUTDOWN)));
				Shell_NotifyIcon(NIM_DELETE, &NID);
			}
		}
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
