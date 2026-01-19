#include "Engine.h"
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	Engine GameEngine;
	if (GameEngine.Initialize())
		GameEngine.Run();

	return 0;
}