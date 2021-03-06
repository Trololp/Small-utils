// dllmain.cpp: определяет точку входа для приложения DLL.
#include "stdafx.h"
#include <stdio.h>

extern uintptr_t g_engine_base;
extern int main_loop_func();
extern int Init_all_func();
extern int Destroy_all_func();

void OnDllAttach()
{
	AllocConsole();
	freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	SetConsoleTitleA("CONSOLE");
}

void OnDllDetach()
{
	Destroy_all_func();

	fclose((FILE*)stdin);
	fclose((FILE*)stdout);

	HWND hw_ConsoleHwnd = GetConsoleWindow();
	FreeConsole();
	PostMessageW(hw_ConsoleHwnd, WM_CLOSE, 0, 0);
}

DWORD WINAPI MainThread(LPVOID param)
{
	OnDllAttach();
	uintptr_t modBase = (uintptr_t)GetModuleHandle(NULL);
	if (modBase != g_engine_base)
		g_engine_base = modBase;

	printf("g_engine_base: %08x \n", g_engine_base);
	Init_all_func();
	main_loop_func();

	FreeLibraryAndExitThread((HMODULE)param, 0);
	return false;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		CreateThread(0, 0, MainThread, hModule, 0, 0);
		break;
    case DLL_PROCESS_DETACH:
		Sleep(1000);
		OnDllDetach();
        break;
    }
    return TRUE;
}

