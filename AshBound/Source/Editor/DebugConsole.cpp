#include "Editor/DebugConsole.h"
#include <Windows.h>
#include <cstdio>
#include <iostream>

void DebugConsole::Init()
{
#ifdef _DEBUG
	if (AllocConsole())
	{
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		freopen_s(&fp, "CONOUT$", "r", stdin);
		SetConsoleTitle(L"Debug Console");
		std::ios::sync_with_stdio(true);
		std::cout << "== Debug Console Initialized" << std::endl;
	}
#endif
}

void DebugConsole::Free()
{
#ifdef _DEBUG
	FreeConsole();
#endif
}
