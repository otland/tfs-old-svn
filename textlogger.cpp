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
#if defined(WIN32) && not defined(__CONSOLE__)
#include "otpch.h"
#include "textlogger.h"

#include "tools.h"
#include "gui.h"

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
	char buf[21], buffer[85], date[21];
	formatDate(time(NULL), date);

	formatDate2(time(NULL), buf);
	sprintf(buffer, "%s%s.log", getFilePath(FILE_TYPE_LOG, "server/").c_str(), buf);
	if(FILE* file = fopen(buffer, "a"))
	{
		if(displayDate)
			fprintf(file, "[%s] ", date);

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
			GUI::getInstance()->m_logText += date;
			GUI::getInstance()->m_logText += "] ";
			displayDate = false;
		}

		GUI::getInstance()->m_logText += (char)c;
	}

	return c;
}
#endif

