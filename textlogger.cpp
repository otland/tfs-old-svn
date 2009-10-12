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

void Logger::open()
{
	m_files[LOGFILE_ADMIN] = fopen(getFilePath(FILE_TYPE_LOG, "admin.log").c_str(), "a");
	m_files[LOGFILE_CLIENT_ASSERTION] = fopen(getFilePath(FILE_TYPE_LOG, "client_assertions.log").c_str(), "a");
}

void Logger::close()
{
	for(uint8_t i = 0; i <= LOGFILE_LAST; i++)
	{
		if(m_files[i])
			fclose(m_files[i]);
	}
}

void Logger::iFile(LogFile_t file, std::string output, bool newLine)
{
	if(!m_files[file])
		return;

	internal(m_files[file], output, newLine);
	fflush(m_files[file]);
}

void Logger::eFile(std::string file, std::string output, bool newLine)
{
	FILE* f = fopen(getFilePath(FILE_TYPE_LOG, file).c_str(), "a");
	if(!f)
		return;

	internal(f, "[" + formatDate() + "] " + output, newLine);
	fclose(f);
}

void Logger::internal(FILE* file, std::string output, bool newLine)
{
	if(!file)
		return;

	if(newLine)
		output += "\n";

	fprintf(file, "%s", output.c_str());
}

void Logger::log(const char* func, LogType_t type, std::string message, std::string channel/* = ""*/, bool newLine/* = true*/)
{
	std::stringstream ss;
	ss << "[" << formatDate() << "]" << " (";
	switch(type)
	{
		case LOGTYPE_EVENT:
			ss << "Event";
			break;

		case LOGTYPE_NOTICE:
			ss << "Notice";
			break;

		case LOGTYPE_WARNING:
			ss << "Warning";
			break;

		case LOGTYPE_ERROR:
			ss << "Error";
			break;

		default:
			break;
	}

	ss << " - " << func << ") ";

	if(!channel.empty())
		ss << channel << ": ";

	ss << message;
	iFile(LOGFILE_ADMIN, ss.str(), newLine);
}
