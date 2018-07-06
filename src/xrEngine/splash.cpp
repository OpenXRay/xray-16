#include "stdafx.h"
#include "xr_3da/resource.h"
#include "splash.h"

HWND logoWindow = nullptr;

static INT_PTR CALLBACK LogoWndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
#if defined(WINDOWS)
    case WM_DESTROY: break;
    case WM_CLOSE: DestroyWindow(hw); break;
    case WM_COMMAND:
        if (LOWORD(wp) == IDCANCEL)
            DestroyWindow(hw);
        break;
#endif
    default: return false;
    }
    return true;
}

namespace splash
{
void show(const bool topmost)
{
    if (logoWindow)
        return;
#if defined(WINDOWS)
    logoWindow = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_STARTUP), nullptr, LogoWndProc);
    const HWND logoPicture = GetDlgItem(logoWindow, IDC_STATIC_LOGO);
    RECT logoRect;
    GetWindowRect(logoPicture, &logoRect);
    const HWND prevWindow = topmost ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(logoWindow, prevWindow, 0, 0, logoRect.right - logoRect.left, logoRect.bottom - logoRect.top,
        SWP_NOMOVE | SWP_SHOWWINDOW);
    UpdateWindow(logoWindow);
#endif
}

void hide()
{
    if (logoWindow != nullptr)
    {
#if defined(WINDOWS)
        DestroyWindow(logoWindow);
#endif
        logoWindow = nullptr;
    }
}
}
