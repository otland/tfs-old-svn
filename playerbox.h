//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Player box
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

#ifndef __FORGOTTENSERVER_PLAYERBOX__
#define __FORGOTTENSERVER_PLAYERBOX__
#ifndef _CONSOLE
#include <tchar.h>
#include <windows.h>
#include <stdlib.h>
#include "player.h"

class PlayerBox
{
	static HWND playerBox;
	static HWND parent;
	static HWND kick;
	static HWND permBan;
	static HWND online;
	static HINSTANCE m_hInst;
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	public:
		bool popUp(LPCTSTR szCaption);
		PlayerBox();
		virtual ~PlayerBox();
		static HWND list;
		void updatePlayersOnline();
		void addPlayer(Player* player);
		void removePlayer(Player* player);
		void setParent(HWND hWndParent){ parent = hWndParent; }
};

#endif
#endif
