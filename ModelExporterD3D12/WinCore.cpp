#include "stdafx.h"
#include "WinCore.h"
#include "Resource.h"

HINSTANCE WinCore::g_hInstance = NULL;
HWND WinCore::g_hWnd = NULL;
DWORD WinCore::g_dwClientWidth = 0;
DWORD WinCore::g_dwClientHeight = 0;


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WinCore::WinCore(HINSTANCE hInstance, DWORD dwWidth, DWORD dwHeight, BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
    g_hInstance = hInstance;
    g_dwClientWidth = dwWidth;
    g_dwClientHeight = dwHeight;

    MyRegisterClass();

    if (!InitInstance(SW_SHOW)){
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        throw std::runtime_error("Failed to Initialize Application");
    }

    m_pGameFramework = std::make_shared<GameFramework>(bEnableDebugLayer, bEnableGBV);

}

void WinCore::Run()
{
    HACCEL hAccelTable = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDC_MODELEXPORTERD3D12));
    MSG msg;

    while (TRUE) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {
            // Framework Update
            m_pGameFramework->Update();
            m_pGameFramework->Render();
        }
    }
}


ATOM WinCore::MyRegisterClass()
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = g_hInstance;
    wcex.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MODELEXPORTERD3D12));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = m_wstrGameName.c_str();
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL WinCore::InitInstance(int cmdShow)
{

    WCHAR szTitle[100];                  // 제목 표시줄 텍스트입니다.
    WCHAR szWindowClass[100];            // 기본 창 클래스 이름입니다.

    LoadStringW(g_hInstance, IDS_APP_TITLE, szTitle, 100);
    LoadStringW(g_hInstance, IDC_MODELEXPORTERD3D12, szWindowClass, 100);

    RECT rc = { 0,0,g_dwClientWidth, g_dwClientHeight };
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_BORDER;
    AdjustWindowRect(&rc, dwStyle, FALSE);

    g_hWnd = CreateWindowW(m_wstrGameName.c_str(), m_wstrGameName.c_str(), dwStyle, CW_USEDEFAULT, 0,
        rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, g_hInstance, NULL);

    if (!g_hWnd)
    {
        return FALSE;
    }

    ShowWindow(g_hWnd, cmdShow);
    UpdateWindow(g_hWnd);

    return TRUE;
}

LRESULT WinCore::WndProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(handle, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(handle, message, wParam, lParam);
    }
    return 0;
}
