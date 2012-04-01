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

#include "otpch.h"

#ifndef _CONSOLE
#ifdef WIN32
#include "gui.h"

void GUI::initTrayMenu()
{
	m_trayMenu = CreateMenu();
	HMENU subFile = CreatePopupMenu();
	AppendMenu(subFile, MF_STRING, ID_TRAY_HIDE, "&Hide window");
	AppendMenu(subFile, MF_SEPARATOR, 0, 0);
	AppendMenu(subFile, MF_STRING, ID_TRAY_SHUTDOWN, "&Shutdown");
	AppendMenu(m_trayMenu, MF_STRING | MF_POPUP, (UINT)subFile, "&File");
}

void GUI::initFont()
{
	LOGFONT lFont;
	memset(&lFont, 0, sizeof(lFont));
	lstrcpy(lFont.lfFaceName, _T("Arial"));
	lFont.lfHeight = 16;
	lFont.lfWeight = FW_NORMAL;
	lFont.lfItalic = FALSE;
	lFont.lfCharSet = DEFAULT_CHARSET;
	lFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lFont.lfQuality = DEFAULT_QUALITY;
	lFont.lfPitchAndFamily = DEFAULT_PITCH;
	m_font = CreateFontIndirect(&lFont);
}

#endif
#endif
