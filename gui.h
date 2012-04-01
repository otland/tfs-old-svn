//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// GUI
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

#ifndef __FORGOTTENSERVER_GUI_H__
#define __FORGOTTENSERVER_GUI_H__

#ifndef _CONSOLE
#include "playerbox.h"
#include "resources.h"

class GUI
{
	public:
		void initTrayMenu();
		void initFont();

		static GUI* getInstance()
		{
			static GUI instance;
			return &instance;
		}

		bool m_connections, m_minimized;
		HWND m_mainWindow, m_statusBar, m_logWindow;
		uint64_t m_lineCount;
		PlayerBox m_pBox;
		HFONT m_font;
		HMENU m_trayMenu;
		std::string m_logText;
};

#endif
#endif
