// CustomEntity_loader_AVP2010.cpp: определяет экспортированные функции для приложения DLL.
//

#include <stdio.h>
#include <vector>
#include <Windows.h>

#include "stdafx.h"
#include "console_cmds.hpp"
#pragma warning(disable : 4996)


//Defines

#define CONSOLE_OUTPUT 


//Structs




//typedefs
// Package related

//typedef char(__cdecl *_LOAD_ARCH)(const char *File_name);

// Entity related
//typedef char(__thiscall *_LOAD_ENTI)(void*, void*);

//Globals

const char* g_mission_names[19] = {
	"Lab\\A01_Lab",
	"Refinery\\A02_Refinery",
	"Ruins\\A03_Ruins",
	"Colony\\A04_Colony",
	"Jungle\\A05_Jungle",
	"Pyramid\\A06_Pyramid", // This actualy didnt exists
	"Colony\\M01_Colony",
	"Refinery\\M02_Refinery",
	"Jungle\\M03_Jungle",
	"Ruins\\M04_Ruins",
	"Lab\\M05_Lab",
	"Pyramid\\M06_Pyramid",
	"P00_Tutorial\\P00_PredTutorial",
	"Jungle\\P01_Jungle",
	"Colony\\P02_Colony",
	"Refinery\\P03_Refinery",
	"Ruins\\P04_Ruins",
	"Lab\\P05_Lab",
	"Pyramid\\P06_Pyramid"
}; // From Game

uintptr_t g_engine_base = 0x40000;
//_LOAD_ENTI g_load_entity_func;
//_LOAD_ARCH g_load_archive_func;

int* g_mission_code;

void dbgprint(const char* debug_note, const char* fmt, ...)
{

#ifdef CONSOLE_OUTPUT
	va_list args;
	va_start(args, fmt);

	printf("(%s) | ", debug_note);
	vprintf(fmt, args);

	va_end(args);
	return;
#endif // !TEXT_LOG

	wchar_t* cwd = _wgetcwd(0, 0);
	wchar_t file_path[260];
	wsprintf(file_path, L"%ws\\debug.txt", cwd);
	FILE* file = _wfopen(file_path, L"a+");
	if (!file)
	{
		MessageBox(nullptr, L"Cannot open debug log !", L"Error", MB_OK);
		return;
	}
	SYSTEMTIME st;
	GetLocalTime(&st);
	fprintf(file, "%02d.%02d.%d %02d:%02d (%s) | ", st.wDay, st.wMonth,
		st.wYear, st.wHour, st.wMinute, debug_note);

	va_start(args, fmt);

	vfprintf(file, fmt, args);

	va_end(args);
	fclose(file);
}




int Init_all_func()
{
	//g_load_entity_func = (_LOAD_ENTI)(g_engine_base + 0x104650);
	//g_load_archive_func = (_LOAD_ARCH)(g_engine_base + 0x10B2E0);

	//Call a console init 
	Init_console();

	g_mission_code = (int*)(g_engine_base + 0x671E2C);
	if(!(*g_mission_code))
	{ 
		printf("Current mission code: None, (Not loaded or not SP) \n");
		return 0;
	}
	printf("Current mission code: %d, (%s) \n", *g_mission_code, g_mission_names[*g_mission_code]);

	return 1;
}

int Destroy_all_func()
{
	printf("Destroy all func !\n");
	return 0;
}

int main_loop_func()
{
	while (true)
	{
		printf("enter command:");

		char str[10];
		scanf_s("%9s", str, (unsigned)_countof(str));

		if (!strcmp(str, "exit"))
		{
			break;
		}
		
		printf("Unknown command ! \n");
	}
	return 0;
}