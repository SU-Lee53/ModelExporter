#include "stdafx.h"
#include "resource.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WinCore* pApp = nullptr;

#ifdef _DEBUG
	pApp = new WinCore(hInstance, 1600, 900, TRUE, TRUE);
#else
	pApp = new WinCore(hInstance, 1600, 900, FALSE, FALSE);
#endif

	pApp->Run();

	delete pApp;
}