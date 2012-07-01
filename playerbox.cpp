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

#include "otpch.h"

#ifndef _CONSOLE
#include "definitions.h"
#include "game.h"
#include <iostream>
#include "gui.h"
#include "ban.h"

extern Game g_game;

HWND PlayerBox::parent = NULL;
HWND PlayerBox::playerBox = NULL;
HWND PlayerBox::permBan = NULL;
HWND PlayerBox::kick = NULL;
HWND PlayerBox::list = NULL;
HWND PlayerBox::online = NULL;

HINSTANCE PlayerBox::m_hInst = NULL;

PlayerBox::PlayerBox()
{
	HINSTANCE hInst = GetModuleHandle(NULL);
	WNDCLASSEX wcex;
	if(!GetClassInfoEx(hInst, "PlayerBox", &wcex))
	{
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = (WNDPROC)WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInst;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = "PlayerBox";
		wcex.hIconSm = NULL;
		if(RegisterClassEx(&wcex) == 0)
			MessageBoxA(NULL, "Can't create PlayerBox!", "Error", MB_OK);
	}
}

PlayerBox::~PlayerBox()
{
	//
}

void PlayerBox::updatePlayersOnline()
{
	int32_t playersOnline = SendMessage(list, CB_GETCOUNT, 0, 0);

	std::ostringstream ss;
	ss << playersOnline << " player" << (playersOnline != 1 ? "s" : "") << " online";
	SendMessage(online, WM_SETTEXT, 0, (LPARAM)ss.str().c_str());
}

void PlayerBox::addPlayer(Player* player)
{
	SendMessage(list, CB_ADDSTRING, 0, (LPARAM)player->getName().c_str());
	updatePlayersOnline();
}

void PlayerBox::removePlayer(Player* player)
{
	DWORD index = SendMessage(list, CB_FINDSTRING, 0,(LPARAM)player->getName().c_str());
	if((signed)index != CB_ERR)
		SendMessage(list, CB_DELETESTRING, index, 0);
	updatePlayersOnline();
}

LRESULT CALLBACK PlayerBox::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CREATE:
		{
			int32_t playersOnline = g_game.getPlayersOnline();

			std::ostringstream ss;
			ss << playersOnline << " player" << (playersOnline != 1 ? "s" : "") << " online";
			m_hInst = GetModuleHandle(NULL);
			
			permBan = CreateWindowEx(0, "button", "Permanently Ban", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 5, 35, 115, 25, hWnd, NULL, m_hInst, NULL);
			kick = CreateWindowEx(0, "button", "Kick", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 125, 35, 90, 25, hWnd, NULL, m_hInst, NULL);
			list = CreateWindowEx(0, "combobox", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL/* | CBS_DROPDOWNLIST*/ | CBS_SORT, 5, 5, 210, 25, hWnd, NULL, m_hInst, NULL);
			online = CreateWindowEx(WS_EX_STATICEDGE, "static", ss.str().c_str(), WS_VISIBLE | WS_CHILD | WS_TABSTOP, 5, 65, 210, 20, hWnd, NULL, m_hInst, NULL);
			
			SendMessage(permBan, WM_SETFONT, (WPARAM)GUI::getInstance()->m_font, 0);
			SendMessage(kick, WM_SETFONT, (WPARAM)GUI::getInstance()->m_font, 0);
			SendMessage(list, WM_SETFONT, (WPARAM)GUI::getInstance()->m_font, 0);
			SendMessage(online, WM_SETFONT, (WPARAM)GUI::getInstance()->m_font, 0);
			
			AutoList<Player>::listiterator it;
			for(it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
				SendMessage(list, CB_ADDSTRING, 0, (LPARAM)(*it).second->getName().c_str());
		}
		break;
		case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					char name[30];
					GetWindowText(list, name, sizeof(name));
					Player* player = g_game.getPlayerByName(name);
					if(player)
					{
						std::ostringstream ss;
						ss << "Are you sure you want to " << ((HWND)lParam == kick ? "kick" : "permanently ban") << " " << player->getName() << "?";
						if(MessageBoxA(hWnd, ss.str().c_str(), "Player List", MB_YESNO) == IDYES)
						{
							player = g_game.getPlayerByName(name);
							if(player)
							{
								if((HWND)lParam == permBan)
									IOBan::getInstance()->addAccountBan(player->getAccount(), 0xFFFFFFFF, 33, 2, "Permanent banishment", 0);

								player->kickPlayer(true);
							}
						}
					}
					else
						MessageBoxA(hWnd, "A player with this name is not online", "Player List", MB_OK);
				}
				break;
			}
			break;
		}
		case WM_DESTROY:
		{
			EnableWindow(parent, true);
			SetForegroundWindow(parent);
			DestroyWindow(hWnd);
			PostQuitMessage(0);
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool PlayerBox::popUp(LPCTSTR szCaption)
{
	RECT r;
	GetWindowRect(GetDesktopWindow(), &r);

	playerBox = CreateWindowEx(WS_EX_TOOLWINDOW, "PlayerBox", szCaption, WS_POPUPWINDOW|WS_CAPTION|WS_TABSTOP, (r.right-200)/2, (r.bottom-115)/2, 225, 115, parent, NULL, m_hInst, NULL);
	if(playerBox == NULL)
		return false;

	SetForegroundWindow(playerBox);
	EnableWindow(parent, FALSE);
	ShowWindow(playerBox, SW_SHOW);
	UpdateWindow(playerBox);

	BOOL ret = 0;
	MSG msg;
	SendMessage(list, WM_KEYDOWN, VK_DOWN, 0);
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(msg.message == WM_KEYDOWN)
		{
			if(msg.wParam == VK_ESCAPE)
			{
				SendMessage(playerBox, WM_DESTROY, 0, 0);
				ret = 0;
			}
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return ret == TRUE;
}
#endif
