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
#include "otpch.h"
#include "textlogger.h"

#include "tools.h"
#if defined(WIN32) && not defined(__CONSOLE__)
#include "gui.h"
#endif

Loggar::Loggar()
{
	m_file = fopen("data/logs/admin.log", "a");
}

Loggar::~Loggar()
{
	if(m_file)
		fclose(m_file);
}

void Loggar::logMessage(const char* channel, LogType_t type, int32_t level, std::string message, const char* func)
{
	fprintf(m_file, "%s", formatDate().c_str());
	if(channel)
		fprintf(m_file, " [%s] ", channel);

	std::string typeStr = "unknown";
	switch(type)
	{
		case LOGTYPE_EVENT:
			typeStr = "event";
			break;

		case LOGTYPE_WARNING:
			typeStr = "warning";
			break;

		case LOGTYPE_ERROR:
			typeStr = "error";
			break;

		default:
			break;
	}

	fprintf(m_file, " %s: %s\n", typeStr.c_str(), message.c_str());
	fflush(m_file);
}

#if defined(WIN32) && not defined(__CONSOLE__)
TextLogger::TextLogger()
{
	out = std::cerr.rdbuf();
	err = std::cout.rdbuf();
	displayDate = true;
}

TextLogger::~TextLogger()
{
	std::cerr.rdbuf(err);
	std::cout.rdbuf(out);
}

int32_t TextLogger::overflow(int32_t c)
{
	char buffer[85];
	sprintf(buffer, "data/logs/server/%s.log", formatDateShort().c_str());
	if(FILE* file = fopen(buffer, "a"))
	{
		if(displayDate)
			fprintf(file, "[%s] ", formatDate().c_str());

		fprintf(file, "%c", c);
		fclose(file);
	}

	if(c == '\n')
	{
		GUI::getInstance()->m_logText += "\r\n";
		SendMessage(GetDlgItem(GUI::getInstance()->m_mainWindow, ID_LOG), WM_SETTEXT, 0, (LPARAM)GUI::getInstance()->m_logText.c_str());
		GUI::getInstance()->m_lineCount++;
		SendMessage(GUI::getInstance()->m_logWindow, EM_LINESCROLL, 0, GUI::getInstance()->m_lineCount);
		displayDate = true;
	}
	else
	{
		if(displayDate)
		{
			GUI::getInstance()->m_logText += "[";
			GUI::getInstance()->m_logText += formatDate().c_str();
			GUI::getInstance()->m_logText += "] ";
			displayDate = false;
		}

		GUI::getInstance()->m_logText += (char)c;
	}

	return c;
}
#endif

