#ifndef __GUI__
#define __GUI__

#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/frame.h>
#include <iostream>

class MainGUI : public wxFrame 
{
	DECLARE_EVENT_TABLE()
    private:
		void menuHandler( wxCommandEvent& event ){
            std::cout << "Menu id: " << event.GetId() << std::endl;
            event.Skip(false); 
        }
		
	
	protected:
		enum
		{
			ID_MAIN_REJECT = 1000,
			ID_MAIN_CLEAR,
			ID_MAIN_SHUTDOWN,
			ID_SERVER_TYPEPVP,
			ID_SERVER_TYPENONPVP,
			ID_SERVER_ENFORCEDPVP,
			ID_SERVER_BRODCAST,
			ID_SERVER_SAVE,
			ID_SERVER_CLEAN,
			ID_SERVER_REFRESH,
			ID_SERVER_CLOSE,
			ID_SERVER_PLAYERS,
			ID_RELOAD_ACTIONS,
			ID_RELOAD_CHAT,
			ID_RELOAD_CONFIG,
			ID_RELOAD_CREATUREEVENTS,
			ID_RELOAD_GLOBALEVENTS,
			ID_RELOAD_GROUPS,
			ID_RELOAD_HIGHSCORES,
			ID_RELOAD_HOUSEPIRCES,
			ID_RELOAD_ITEMS,
			ID_RELOAD_MODS,
			ID_RELOAD_MONSTERS,
			ID_RELOAD_MOVEMENTS,
			ID_RELOAD_NPCS,
			ID_RELOAD_OUTFITS,
			ID_RELOAD_QUESTS,
			ID_RELOAD_RAIDS,
			ID_RELOAD_SPELLS,
			ID_RELOAD_STAGES,
			ID_RELOAD_TALKACTIONS,
			ID_RELOAD_VOCATIONS,
			ID_RELOAD_WEAPONS,
			ID_RELOAD_ALL
		};
		
		wxTextCtrl* logText;
		wxMenuBar* menuBar;
		wxMenu* mainMenu;
		wxMenu* serverMenu;
		wxMenu* worldType;
		wxMenu* reloadMenu;
        wxStatusBar* statusBar;	
	
	public:
		MainGUI( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("The Forgotten Server 0.4"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 496,474 ), long style = wxDEFAULT_FRAME_STYLE|wxCLIP_CHILDREN|wxTAB_TRAVERSAL );
		~MainGUI();
		wxTextCtrl* getLogText()
		{
            return logText;
        };
		
	
};

#endif //__gui__
