#include "stdafx.h"
#include "WinCore.h"
#include "Resource.h"

HINSTANCE WinCore::sm_hInstance = NULL;
HWND WinCore::sm_hWnd = NULL;
DWORD WinCore::sm_dwClientWidth = 0;
DWORD WinCore::sm_dwClientHeight = 0;


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WinCore::WinCore(HINSTANCE hInstance, DWORD dwWidth, DWORD dwHeight, BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
    sm_hInstance = hInstance;
    sm_dwClientWidth = dwWidth;
    sm_dwClientHeight = dwHeight;

    MyRegisterClass();

    if (!InitInstance(SW_SHOW)){
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        throw std::runtime_error("Failed to Initialize Application");
    }

    m_pGameFramework = std::make_shared<GameFramework>(bEnableDebugLayer, bEnableGBV);

}

void WinCore::Run()
{
    HACCEL hAccelTable = LoadAccelerators(sm_hInstance, MAKEINTRESOURCE(IDC_MODELEXPORTERD3D12));
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
    wcex.hInstance = sm_hInstance;
    wcex.hIcon = LoadIcon(sm_hInstance, MAKEINTRESOURCE(IDI_MODELEXPORTERD3D12));
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

    LoadStringW(sm_hInstance, IDS_APP_TITLE, szTitle, 100);
    LoadStringW(sm_hInstance, IDC_MODELEXPORTERD3D12, szWindowClass, 100);

    RECT rc = { 0,0,sm_dwClientWidth, sm_dwClientHeight };
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_BORDER;
    AdjustWindowRect(&rc, dwStyle, FALSE);

    sm_hWnd = CreateWindowW(m_wstrGameName.c_str(), m_wstrGameName.c_str(), dwStyle, CW_USEDEFAULT, 0,
        rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, sm_hInstance, NULL);

    if (!sm_hWnd)
    {
        return FALSE;
    }

    ShowWindow(sm_hWnd, cmdShow);
    UpdateWindow(sm_hWnd);

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
