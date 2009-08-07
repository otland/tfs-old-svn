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

#ifndef __CONSOLE__
	#define ID_KICK 101
	#define ID_BAN 102
	#define ID_ABOUT 104
	#define ID_LOG 105
	#define ID_ICON 106

	#define ID_TRAY_HIDE 118
	#define ID_TRAY_SHUTDOWN 119
	#define ID_STATUS_BAR 120

	#define ID_MENU 200

	#define ID_MENU_MAIN_REJECT 201
	#define ID_MENU_MAIN_ACCEPT 202
	#define ID_MENU_MAIN_CLEARLOG 203
	#define ID_MENU_MAIN_SHUTDOWN 204

	#define ID_MENU_SERVER_WORLDTYPE_NOPVP 205
	#define ID_MENU_SERVER_WORLDTYPE_PVP 206
	#define ID_MENU_SERVER_WORLDTYPE_PVPENFORCED 207

	#define ID_MENU_SERVER_BROADCAST 208
	#define ID_MENU_SERVER_SAVE 209
	#define ID_MENU_SERVER_CLEAN 210
	#define ID_MENU_SERVER_REFRESH 211
	#define ID_MENU_SERVER_CLOSE 212
	#define ID_MENU_SERVER_OPEN 213

	#define ID_MENU_SERVER_PLAYERBOX 214

	#define ID_MENU_RELOAD_ACTIONS 215
	#define ID_MENU_RELOAD_CHAT 216
	#define ID_MENU_RELOAD_CONFIG 217
	#define ID_MENU_RELOAD_CREATUREEVENTS 218
	#ifdef __LOGIN_SERVER__
	#define ID_MENU_RELOAD_GAMESERVERS 219
	#endif
	#define ID_MENU_RELOAD_GLOBALEVENTS 220
	#define ID_MENU_RELOAD_GROUPS 221
	#define ID_MENU_RELOAD_HIGHSCORES 222
	#define ID_MENU_RELOAD_HOUSEPRICES 223
	#define ID_MENU_RELOAD_ITEMS 224
	#define ID_MENU_RELOAD_MONSTERS 225
	#define ID_MENU_RELOAD_MODS 226
	#define ID_MENU_RELOAD_MOVEMENTS 227
	#define ID_MENU_RELOAD_NPCS 228
	#define ID_MENU_RELOAD_OUTFITS 229
	#define ID_MENU_RELOAD_QUESTS 230
	#define ID_MENU_RELOAD_RAIDS 231
	#define ID_MENU_RELOAD_SPELLS 232
	#define ID_MENU_RELOAD_STAGES 233
	#define ID_MENU_RELOAD_TALKACTIONS 234
	#define ID_MENU_RELOAD_VOCATIONS 235
	#define ID_MENU_RELOAD_WEAPONS 236
	#define ID_MENU_RELOAD_ALL 237
#endif

#define CLIENT_VERSION_MIN 850
#define CLIENT_VERSION_MAX 850
#define CLIENT_VERSION_STRING "Only clients with protocol 8.5 allowed!"

#define STATUS_SERVER_NAME "The Forgotten Server"
#define STATUS_SERVER_VERSION "0.3.5"
#define STATUS_SERVER_CODENAME "Crying Damson"
#define STATUS_SERVER_PROTOCOL "8.5"

#define VERSION_CHECK "http://forgottenserver.otland.net/version.xml"
#define VERSION_PATCH 0
#define VERSION_TIMESTAMP 1249584126
#define VERSION_BUILD 2564
#define VERSION_DATABASE 21
