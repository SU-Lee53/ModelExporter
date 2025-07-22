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
	static HINSTANCE sm_hInstance;
	static HWND sm_hWnd;

	static DWORD sm_dwClientWidth;
	static DWORD sm_dwClientHeight;

public:
	std::wstring m_wstrGameName = L"Model Importer";
	std::shared_ptr<GameFramework> m_pGameFramework = nullptr;

};

