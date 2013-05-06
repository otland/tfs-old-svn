//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Input box
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

#ifndef __INPUTBOX_H__
#define __INPUTBOX_H__
#ifndef _CONSOLE
#include <tchar.h>
#include <windows.h>
#include <stdlib.h>

#define INPUTBOX_WIDTH 400
#define INPUTBOX_HEIGHT 125

class CInputBox
{
	static HFONT m_hFont;
	static HWND  m_hWndInputBox;
	static HWND  m_hWndParent;
	static HWND  m_hWndEdit;
	static HWND  m_hWndOK;
	static HWND  m_hWndCancel;
	static HWND  m_hWndPrompt;

	static HINSTANCE m_hInst;

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	public:
		LPTSTR Text;
		BOOL DoModal(LPCTSTR szCaption, LPCTSTR szPrompt);

		CInputBox(HWND hWndParent);
		virtual ~CInputBox();

};

#endif
#endif
