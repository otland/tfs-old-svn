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
#ifdef __EXCEPTION_TRACER__
#include "otpch.h"
#include "exception.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#ifdef WINDOWS
#include <excpt.h>
#include <tlhelp32.h>
#endif

#include <boost/config.hpp>
#include "tools.h"

#include "configmanager.h"
extern ConfigManager g_config;

typedef std::map<uint32_t, char*> FunctionMap;
FunctionMap functionMap;

uint32_t offMax, offMin;
bool mapLoaded = false;
boost::recursive_mutex mapLock;

#ifdef WINDOWS
void printPointer(std::ostream* output, uint32_t p);
EXCEPTION_DISPOSITION __cdecl _SEHHandler(struct _EXCEPTION_RECORD *ExceptionRecord, void* EstablisherFrame,
	struct _CONTEXT *ContextRecord, void* DispatcherContext);
#endif

ExceptionHandler::ExceptionHandler()
{
	installed = false;
}

ExceptionHandler::~ExceptionHandler()
{
	if(installed)
		RemoveHandler();
}

bool ExceptionHandler::InstallHandler()
{
	#ifdef WINDOWS
	boost::recursive_mutex::scoped_lock lockObj(mapLock);
	if(!mapLoaded)
		LoadMap();

	if(installed)
		return false;

	/*
		mov eax,fs:[0]
		mov [prevSEH],eax
		mov [chain].prev,eax
		mov [chain].SEHfunction,_SEHHandler
		lea eax,[chain]
		mov fs:[0],eax
	*/

	#ifdef __GNUC__
	SEHChain *prevSEH;
	__asm__ ("movl %%fs:0,%%eax;movl %%eax,%0;":"=r"(prevSEH)::"%eax");
	chain.prev = prevSEH;
	chain.SEHfunction = (void*)&_SEHHandler;
	__asm__("movl %0,%%eax;movl %%eax,%%fs:0;": : "g" (&chain):"%eax");
	#endif
	#endif

	installed = true;
	return true;
}


bool ExceptionHandler::RemoveHandler()
{
	if(!installed)
		return false;

	#ifdef WINDOWS
	#ifdef __GNUC__
	__asm__ ("movl %0,%%eax;movl %%eax,%%fs:0;"::"r"(chain.prev):"%eax" );
	#endif
	#endif

	installed = false;
	return true;
}

char* getFunctionName(unsigned long addr, unsigned long& start)
{
	FunctionMap::iterator functions;
	if(addr < offMin || addr > offMax)
		return NULL;

	for(FunctionMap::iterator functions = functionMap.begin(); functions != functionMap.end(); ++functions)
	{
		if(functions->first <= addr || functions == functionMap.begin())
			continue;

		functions--;
		start = functions->first;
		return functions->second;
	}

	return NULL;
}

#ifdef WINDOWS
EXCEPTION_DISPOSITION __cdecl _SEHHandler(struct _EXCEPTION_RECORD *ExceptionRecord, void* EstablisherFrame,
	 struct _CONTEXT *ContextRecord, void* DispatcherContext)
{
	uint32_t *esp;
	uint32_t *next_ret;
	uint32_t stack_val;
	uint32_t *stacklimit;
	uint32_t *stackstart;
	uint32_t nparameters = 0;
	uint32_t file,foundRetAddress = 0;
	_MEMORY_BASIC_INFORMATION mbi;

	std::ostream *outdriver;
	std::cout << ">> CRASH: Writing report file..." << std::endl;
	std::ofstream output(getFilePath(FILE_TYPE_LOG, "server/exceptions.log").c_str(), std::ios_base::app);
	if(output.fail())
	{
		outdriver = &std::cout;
		file = false;
	}
	else
	{
		file = true;
		outdriver = &output;
	}

	time_t rawtime;
	time(&rawtime);
	*outdriver << "*****************************************************" << std::endl;
	*outdriver << "Error report - " << std::ctime(&rawtime) << std::endl;
	*outdriver << "Compiler Info - " << BOOST_COMPILER << std::endl;
	*outdriver << "Compilation Date - " << __DATE__ << " " << __TIME__ << std::endl << std::endl;

	//system and process info
	//- global memory information
	MEMORYSTATUSEX mstate;
	mstate.dwLength = sizeof(mstate);
	if(GlobalMemoryStatusEx(&mstate))
	{
		*outdriver << "Memory load: " << mstate.dwMemoryLoad << std::endl <<
			"Total phys: " << mstate.ullTotalPhys/1024 << " K available phys: " <<
			mstate.ullAvailPhys/1024 << " K" << std::endl;
	}
	else
		*outdriver << "Memory load: Error" << std::endl;

	//- process info
	FILETIME FTcreation, FTexit, FTkernel, FTuser;
	SYSTEMTIME systemtime;
	GetProcessTimes(GetCurrentProcess(), &FTcreation, &FTexit, &FTkernel, &FTuser);
	// creation time
	FileTimeToSystemTime(&FTcreation, &systemtime);
	*outdriver << "Start time: " << systemtime.wDay << "-" << systemtime.wMonth << "-" << systemtime.wYear << "  " <<
		systemtime.wHour << ":" << systemtime.wMinute << ":" << systemtime.wSecond << std::endl;
	// kernel time
	uint32_t miliseconds;
	miliseconds = FTkernel.dwHighDateTime * 429497 + FTkernel.dwLowDateTime/10000;
	*outdriver << "Kernel time: " << miliseconds/3600000;
	miliseconds = miliseconds - (miliseconds/3600000)*3600000;
	*outdriver << ":" << miliseconds/60000;
	miliseconds = miliseconds - (miliseconds/60000)*60000;
	*outdriver << ":" << miliseconds/1000;
	miliseconds = miliseconds - (miliseconds/1000)*1000;
	*outdriver << "." << miliseconds << std::endl;
	// user time
	miliseconds = FTuser.dwHighDateTime * 429497 + FTuser.dwLowDateTime/10000;
	*outdriver << "User time: " << miliseconds/3600000;
	miliseconds = miliseconds - (miliseconds/3600000)*3600000;
	*outdriver << ":" << miliseconds/60000;
	miliseconds = miliseconds - (miliseconds/60000)*60000;
	*outdriver << ":" << miliseconds/1000;
	miliseconds = miliseconds - (miliseconds/1000)*1000;
	*outdriver << "." << miliseconds << std::endl;

	// n threads
	PROCESSENTRY32 uProcess;
	HANDLE lSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	BOOL r;
	if(lSnapShot != 0)
	{
		uProcess.dwSize = sizeof(uProcess);
		r = Process32First(lSnapShot, &uProcess);
		while(r)
		{
			if(uProcess.th32ProcessID == GetCurrentProcessId())
			{
				*outdriver << "Threads: " << uProcess.cntThreads << std::endl;
				break;
			}

			r = Process32Next(lSnapShot, &uProcess);
		}

		CloseHandle(lSnapShot);
	}

	*outdriver << std::endl;
	//exception header type and eip
	outdriver->flags(std::ios::hex | std::ios::showbase);
	*outdriver << "Exception: " << (uint32_t)ExceptionRecord->ExceptionCode << " at eip = " << (uint32_t)ExceptionRecord->ExceptionAddress;
	FunctionMap::iterator functions;
	unsigned long functionAddr;
	char* functionName = getFunctionName((unsigned long)ExceptionRecord->ExceptionAddress, functionAddr);
	if(functionName)
		*outdriver << "(" << functionName << " - " << functionAddr << ")";

	//registers
	*outdriver << std::endl;
	*outdriver << "eax = ";printPointer(outdriver,ContextRecord->Eax);*outdriver << std::endl;
	*outdriver << "ebx = ";printPointer(outdriver,ContextRecord->Ebx);*outdriver << std::endl;
	*outdriver << "ecx = ";printPointer(outdriver,ContextRecord->Ecx);*outdriver << std::endl;
	*outdriver << "edx = ";printPointer(outdriver,ContextRecord->Edx);*outdriver << std::endl;
	*outdriver << "esi = ";printPointer(outdriver,ContextRecord->Esi);*outdriver << std::endl;
	*outdriver << "edi = ";printPointer(outdriver,ContextRecord->Edi);*outdriver << std::endl;
	*outdriver << "ebp = ";printPointer(outdriver,ContextRecord->Ebp);*outdriver << std::endl;
	*outdriver << "esp = ";printPointer(outdriver,ContextRecord->Esp);*outdriver << std::endl;
	*outdriver << "efl = " << ContextRecord->EFlags << std::endl;

	//stack dump
	esp = (uint32_t *)(ContextRecord->Esp);
	VirtualQuery(esp, &mbi, sizeof(mbi));
	stacklimit = (uint32_t*)((uint32_t)(mbi.BaseAddress) + mbi.RegionSize);

	*outdriver << std::endl;
	*outdriver << "---Stack Trace---" << std::endl;
	*outdriver << "From: " << (uint32_t)esp <<
		" to: " << (uint32_t)stacklimit << std::endl;

	stackstart = esp;
	next_ret = (uint32_t*)(ContextRecord->Ebp);
	uint32_t frame_param_counter;
	frame_param_counter = 0;
	while(esp < stacklimit)
	{
		stack_val = *esp;
		if(foundRetAddress)
			nparameters++;

		if(esp - stackstart < 20 || nparameters < 10 || std::abs(esp - next_ret) < 10 || frame_param_counter < 8)
		{
			*outdriver << (uint32_t)esp << " | ";
			printPointer(outdriver,stack_val);
			if(esp == next_ret)
				*outdriver << " \\\\\\\\\\\\ stack frame //////";
			else if(esp - next_ret == 1)
				*outdriver << " <-- ret" ;
			else if(esp - next_ret == 2)
			{
				next_ret = (uint32_t*)*(esp - 2);
				frame_param_counter = 0;
			}

			frame_param_counter++;
			*outdriver<< std::endl;
		}

		if(stack_val >= offMin && stack_val <= offMax)
		{
			foundRetAddress++;
			//
			unsigned long functionAddr;
			char* functionName = getFunctionName(stack_val, functionAddr);
			output << (unsigned long)esp << "  " << functionName << "(" <<
				functionAddr << ")" << std::endl;
		}

		esp++;
	}

	*outdriver << "*****************************************************" << std::endl;
	if(file)
		((std::ofstream*)outdriver)->close();

	if(g_config.getBool(ConfigManager::TRACER_BOX))
	{
		std::stringstream ss;
		ss << "If you want developers review this crash log, please open a tracker ticket for the software at OtLand.net and attach the " << getFilePath(FILE_TYPE_LOG, "server/exceptions.log") << " file.";
		MessageBoxA(NULL, ss.str().c_str(), "Error", MB_OK | MB_ICONERROR);
	}

	std::cout << "> Crash report generated, killing server." << std::endl;
	exit(1);
	return ExceptionContinueSearch;
}

void printPointer(std::ostream* output, uint32_t p)
{
	*output << p;
	if(!IsBadReadPtr((void*)p, 4))
		*output << " -> " << *(uint32_t*)p;
}
#endif

bool ExceptionHandler::LoadMap()
{
	#ifdef __GNUC__
	if(mapLoaded)
		return false;

	functionMap.clear();
	installed = false;
	//load map file if exists
	char line[1024];
	FILE* input = fopen("forgottenserver.map", "r");
	offMin = 0xFFFFFF;
	offMax = 0;
	int32_t n = 0;
	if(!input)
	{
		MessageBoxA(NULL, "Failed loading symbols, forgottenserver.map file not found.", "Error", MB_OK | MB_ICONERROR);
		std::cout << "Failed loading symbols, forgottenserver.map file not found. " << std::endl;
		exit(1);
		return false;
	}

	//read until found .text		   0x00401000
	while(fgets(line, 1024, input))
	{
		if(!memcmp(line, ".text",5))
			break;
	}

	if(feof(input))
		return false;

	char tofind[] = "0x";
	char lib[] = ".a(";
	while(fgets(line, 1024, input))
	{
		char* pos = strstr(line, lib);
		if(pos)
			break; //not load libs

		pos = strstr(line, tofind);
		if(pos)
		{
			//read hex offset
			char hexnumber[12];
			strncpy(hexnumber, pos, 10);
			hexnumber[10] = 0;
			char* pEnd;
			uint32_t offset = strtol(hexnumber, &pEnd, 0);
			if(offset)
			{
				//read function name
				char* pos2 = pos + 12;
				while(*pos2 != 0)
				{
					if(*pos2 != ' ')
						break;

					pos2++;
				}

				if(*pos2 == 0 || (*pos2 == '0' && *(pos2+1) == 'x'))
					continue;

				char* name = new char[strlen(pos2)+1];
				strcpy(name, pos2);
				name[strlen(pos2) - 1] = 0;
				functionMap[offset] = name;
				if(offset > offMax)
					offMax = offset;
				if(offset < offMin)
					offMin = offset;
				n++;
			}
		}
	}

	//close file
	fclose(input);
	mapLoaded = true;
	#endif
	return true;
}

void ExceptionHandler::dumpStack()
{
	#ifndef __GNUC__
	return;
	#endif

	uint32_t *esp;
	uint32_t *next_ret;
	uint32_t stack_val;
	uint32_t *stacklimit;
	uint32_t *stackstart;
	uint32_t nparameters = 0;
	uint32_t foundRetAddress = 0;
	_MEMORY_BASIC_INFORMATION mbi;

	std::cout << ">> CRASH: Writing report file..." << std::endl;
	std::ofstream output(getFilePath(FILE_TYPE_LOG, "server/exceptions.log").c_str(), std::ios_base::app);
	output.flags(std::ios::hex | std::ios::showbase);
	time_t rawtime;
	time(&rawtime);
	output << "*****************************************************" << std::endl;
	output << "Stack dump - " << std::ctime(&rawtime) << std::endl;
	output << "Compiler Info - " << BOOST_COMPILER << std::endl;
	output << "Compilation Date - " << __DATE__ << " " << __TIME__ << std::endl << std::endl;

	#ifdef __GNUC__
	__asm__ ("movl %%esp, %0;":"=r"(esp)::);
	#else
	//
	#endif

	VirtualQuery(esp, &mbi, sizeof(mbi));
	stacklimit = (uint32_t*)((uint32_t)(mbi.BaseAddress) + mbi.RegionSize);

	output << "---Stack Trace---" << std::endl;
	output << "From: " << (uint32_t)esp <<
		" to: " << (uint32_t)stacklimit << std::endl;

	stackstart = esp;
	#ifdef __GNUC__
	__asm__ ("movl %%ebp, %0;":"=r"(next_ret)::);
	#else
	//
	#endif
	uint32_t frame_param_counter;
	frame_param_counter = 0;
	while(esp < stacklimit)
	{
		stack_val = *esp;
		if(foundRetAddress)
			nparameters++;

		if(esp - stackstart < 20 || nparameters < 10 || std::abs(esp - next_ret) < 10 || frame_param_counter < 8)
		{
			output << (uint32_t)esp << " | ";
			printPointer(&output, stack_val);
			if(esp == next_ret)
				output << " \\\\\\\\\\\\ stack frame //////";
			else if(esp - next_ret == 1)
				output << " <-- ret" ;
			else if(esp - next_ret == 2)
			{
				next_ret = (uint32_t*)*(esp - 2);
				frame_param_counter = 0;
			}

			frame_param_counter++;
			output << std::endl;
		}

		if(stack_val >= offMin && stack_val <= offMax)
		{
			foundRetAddress++;
			//
			unsigned long functionAddr;
			char* functionName = getFunctionName(stack_val, functionAddr);
			output << (unsigned long)esp << "  " << functionName << "(" <<
				functionAddr << ")" << std::endl;
		}

		esp++;
	}

	output << "*****************************************************" << std::endl;
	output.close();
	std::cout << "> Crash report generated, killing server." << std::endl;
}
#endif
