#pragma once
#include "GameFramework.h"

class WinCore {
public:
	WinCore(HINSTANCE hInstance, DWORD dwWidth, DWORD dwHeight, BOOL bEnableDebugLayer, BOOL bEnableGBV);

	void Run();

private:
	ATOM MyRegisterClass();
	BOOL InitInstance(int cmdShow);
	static LRESULT CALLBACK WndProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam);


public:
	static HINSTANCE g_hInstance;
	static HWND g_hWnd;

	static DWORD g_dwClientWidth;
	static DWORD g_dwClientHeight;

public:
	std::shared_ptr<GameFramework> m_pGameFramework = nullptr;

};

