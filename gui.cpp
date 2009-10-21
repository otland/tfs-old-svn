#include "gui.h"

BEGIN_EVENT_TABLE(MainGUI, wxFrame)
	EVT_MENU(ID_MAIN_REJECT, MainGUI::menuHandler )
	EVT_MENU(ID_MAIN_CLEAR, MainGUI::menuHandler )
	EVT_MENU(ID_MAIN_SHUTDOWN, MainGUI::menuHandler )
	EVT_MENU(ID_SERVER_TYPEPVP, MainGUI::menuHandler )
	EVT_MENU(ID_SERVER_TYPENONPVP, MainGUI::menuHandler )
	EVT_MENU(ID_SERVER_ENFORCEDPVP, MainGUI::menuHandler )
	EVT_MENU(ID_SERVER_BRODCAST, MainGUI::menuHandler )
	EVT_MENU(ID_SERVER_SAVE, MainGUI::menuHandler )
	EVT_MENU(ID_SERVER_CLEAN, MainGUI::menuHandler )
	EVT_MENU(ID_SERVER_REFRESH, MainGUI::menuHandler )
	EVT_MENU(ID_SERVER_CLOSE, MainGUI::menuHandler )
	EVT_MENU(ID_SERVER_PLAYERS, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_ACTIONS, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_CHAT, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_CONFIG, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_CREATUREEVENTS, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_GLOBALEVENTS, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_GROUPS, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_HIGHSCORES, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_HOUSEPIRCES, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_ITEMS, MainGUI::menuHandler )
	EVT_MENU(ID_RELOAD_MODS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_MONSTERS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_MOVEMENTS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_NPCS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_OUTFITS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_QUESTS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_RAIDS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_SPELLS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_STAGES, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_TALKACTIONS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_VOCATIONS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_WEAPONS, MainGUI::menuHandler)
	EVT_MENU(ID_RELOAD_ALL, MainGUI::menuHandler)
END_EVENT_TABLE()

MainGUI::MainGUI(wxWindow *parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);
	
	wxBoxSizer *boxSizer;
	boxSizer = new wxBoxSizer(wxVERTICAL);
	
	logText = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY|wxNO_BORDER);
	logText->SetMaxLength(65635); 
	boxSizer->Add(logText, 1, wxEXPAND, 0);
	this->SetSizer(boxSizer);
	this->Layout();
	statusBar = this->CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);
	menuBar = new wxMenuBar(0);
	mainMenu = new wxMenu();
	wxMenuItem *mainReject;
	mainReject = new wxMenuItem(mainMenu, ID_MAIN_REJECT, wxString(wxT("&Reject connections")) , wxT("Rejects connetions."), wxITEM_NORMAL);
	mainMenu->Append(mainReject);
	
	wxMenuItem *mainClear;
	mainClear = new wxMenuItem(mainMenu, ID_MAIN_CLEAR, wxString(wxT("&Clear log")) , wxT("Cleans log window output."), wxITEM_NORMAL);
	mainMenu->Append(mainClear);
	
	mainMenu->AppendSeparator();
	
	wxMenuItem *mainShutdown;
	mainShutdown = new wxMenuItem(mainMenu, ID_MAIN_SHUTDOWN, wxString(wxT("&Shutdown")) , wxT("Shutdowns a server."), wxITEM_NORMAL);
	mainMenu->Append(mainShutdown);
	
	menuBar->Append(mainMenu, wxT("&Main"));
	
	serverMenu = new wxMenu();
	worldType = new wxMenu();
	wxMenuItem *worldTypePVP;
	worldTypePVP = new wxMenuItem(worldType, ID_SERVER_TYPEPVP, wxString(wxT("&PVP")) , wxT("Sets world type to PVP."), wxITEM_NORMAL);
	worldType->Append(worldTypePVP);
	
	wxMenuItem *worldTypeNonPVP;
	worldTypeNonPVP = new wxMenuItem(worldType, ID_SERVER_TYPENONPVP, wxString(wxT("&Non-PVP")) , wxT("Sets the world type to Non-PVP."), wxITEM_NORMAL);
	worldType->Append(worldTypeNonPVP);
	
	wxMenuItem *worldTypeEnforcedPVP;
	worldTypeEnforcedPVP = new wxMenuItem(worldType, ID_SERVER_ENFORCEDPVP, wxString(wxT("PVP-&Enforced")) , wxT("Sets the world type to PVP-Enforced."), wxITEM_NORMAL);
	worldType->Append(worldTypeEnforcedPVP);
	
	serverMenu->Append(-1, wxT("&World type"), worldType);
	
	wxMenuItem *brodcastMessage;
	brodcastMessage = new wxMenuItem(serverMenu, ID_SERVER_BRODCAST, wxString(wxT("&Brodcast message")) , wxT("Brodcasts message to all players on the server."), wxITEM_NORMAL);
	serverMenu->Append(brodcastMessage);
	
	wxMenuItem *saveServer;
	saveServer = new wxMenuItem(serverMenu, ID_SERVER_SAVE, wxString(wxT("&Save server")) , wxT("Saves whole server."), wxITEM_NORMAL);
	serverMenu->Append(saveServer);
	
	wxMenuItem *cleanMap;
	cleanMap = new wxMenuItem(serverMenu, ID_SERVER_CLEAN, wxString(wxT("Clean &map")) , wxT("Cleans the whole map."), wxITEM_NORMAL);
	serverMenu->Append(cleanMap);
	
	wxMenuItem *refreshMap;
	refreshMap = new wxMenuItem(serverMenu, ID_SERVER_REFRESH, wxString(wxT("&Refresh map")) , wxT("Refreshing map."), wxITEM_NORMAL);
	serverMenu->Append(refreshMap);
	
	wxMenuItem *closeServer;
	closeServer = new wxMenuItem(serverMenu, ID_SERVER_CLOSE, wxString(wxT("&Close server")) , wxT("Closes server."), wxITEM_NORMAL);
	serverMenu->Append(closeServer);
	
	serverMenu->AppendSeparator();
	
	wxMenuItem *playerBox;
	playerBox = new wxMenuItem(serverMenu, ID_SERVER_PLAYERS, wxString(wxT("&Players managment")) , wxT("Player managment window."), wxITEM_NORMAL);
	serverMenu->Append(playerBox);
	
	menuBar->Append(serverMenu, wxT("&Server"));
	
	reloadMenu = new wxMenu();
	wxMenuItem *reloadActions;
	reloadActions = new wxMenuItem(reloadMenu, ID_RELOAD_ACTIONS, wxString(wxT("Acti&ons")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadActions);
	
	wxMenuItem *reloadChat;
	reloadChat = new wxMenuItem(reloadMenu, ID_RELOAD_CHAT, wxString(wxT("&Chat")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadChat);
	
	wxMenuItem *reloadConfig;
	reloadConfig = new wxMenuItem(reloadMenu, ID_RELOAD_CONFIG, wxString(wxT("Co&nfig")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadConfig);
	
	wxMenuItem *reloadCreatureEvents;
	reloadCreatureEvents = new wxMenuItem(reloadMenu, ID_RELOAD_CREATUREEVENTS, wxString(wxT("Creature &Events")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadCreatureEvents);
	
	wxMenuItem *reloadGlobalEvents;
	reloadGlobalEvents = new wxMenuItem(reloadMenu, ID_RELOAD_GLOBALEVENTS, wxString(wxT("G&lobal Events")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadGlobalEvents);
	
	wxMenuItem *reloadGroups;
	reloadGroups = new wxMenuItem(reloadMenu, ID_RELOAD_GROUPS, wxString(wxT("Gro&ups")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadGroups);
	
	wxMenuItem *reloadHighscores;
	reloadHighscores = new wxMenuItem(reloadMenu, ID_RELOAD_HIGHSCORES, wxString(wxT("&Highscores")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadHighscores);
	
	wxMenuItem *reloadHousePrices;
	reloadHousePrices = new wxMenuItem(reloadMenu, ID_RELOAD_HOUSEPIRCES, wxString(wxT("Ho&use Prices")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadHousePrices);
	
	wxMenuItem *reloadItems;
	reloadItems = new wxMenuItem(reloadMenu, ID_RELOAD_ITEMS, wxString(wxT("&Items")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadItems);
	
	wxMenuItem *reloadMods;
	reloadMods = new wxMenuItem(reloadMenu, ID_RELOAD_MODS, wxString(wxT("Mo&ds")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadMods);
	
	wxMenuItem *reloadMonsters;
	reloadMonsters = new wxMenuItem(reloadMenu, ID_RELOAD_MONSTERS, wxString(wxT("&Monsters")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadMonsters);
	
	wxMenuItem *reloadMovements;
	reloadMovements = new wxMenuItem(reloadMenu, ID_RELOAD_MOVEMENTS, wxString(wxT("Mov&ements")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadMovements);
	
	wxMenuItem *reloadNpcs;
	reloadNpcs = new wxMenuItem(reloadMenu, ID_RELOAD_NPCS, wxString(wxT("&Npcs")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadNpcs);
	
	wxMenuItem *reloadOutfits;
	reloadOutfits = new wxMenuItem(reloadMenu, ID_RELOAD_OUTFITS, wxString(wxT("Out&fits")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadOutfits);
	
	wxMenuItem *reloadQuests;
	reloadQuests = new wxMenuItem(reloadMenu, ID_RELOAD_QUESTS, wxString(wxT("&Quests")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadQuests);
	
	wxMenuItem *reloadRaids;
	reloadRaids = new wxMenuItem(reloadMenu, ID_RELOAD_RAIDS, wxString(wxT("&Raids")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadRaids);
	
	wxMenuItem *reloadSpells;
	reloadSpells = new wxMenuItem(reloadMenu, ID_RELOAD_SPELLS, wxString(wxT("S&pells")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadSpells);
	
	wxMenuItem *reloadStages;
	reloadStages = new wxMenuItem(reloadMenu, ID_RELOAD_STAGES, wxString(wxT("&Stages")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadStages);
	
	wxMenuItem *reloadTalkactions;
	reloadTalkactions = new wxMenuItem(reloadMenu, ID_RELOAD_TALKACTIONS, wxString(wxT("&Talkactions")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadTalkactions);
	
	wxMenuItem *reloadVocations;
	reloadVocations = new wxMenuItem(reloadMenu, ID_RELOAD_VOCATIONS, wxString(wxT("&Vocations")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadVocations);
	
	wxMenuItem *reloadWeapons;
	reloadWeapons = new wxMenuItem(reloadMenu, ID_RELOAD_WEAPONS, wxString(wxT("&Weapons")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadWeapons);
	
	reloadMenu->AppendSeparator();
	
	wxMenuItem *reloadReloadAll;
	reloadReloadAll = new wxMenuItem(reloadMenu, ID_RELOAD_ALL, wxString(wxT("Reload &All")) , wxEmptyString, wxITEM_NORMAL);
	reloadMenu->Append(reloadReloadAll);
	
	menuBar->Append(reloadMenu, wxT("&Reload"));
	
	this->SetMenuBar(menuBar);
	
}

MainGUI::~MainGUI()
{
}
