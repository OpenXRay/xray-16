#include "stdafx.h"
#include "Text_Console.h"

#if defined(WINDOWS)
LRESULT CALLBACK TextConsole_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        // return 0;
    }
    break;
    case WM_ERASEBKGND:
    {
        int x = 0;
        x = x;
        // CTextConsole* pTextConsole = (CTextConsole*)Console;
        // pTextConsole->OnPaint();
        // return 1;
    }
    break;
    case WM_NCPAINT:
    {
        // CTextConsole* pTextConsole = (CTextConsole*)Console;
        // pTextConsole->OnPaint();
        int x = 0;
        x = x;
        // return 0;
    }
    break;
    default: break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK TextConsole_LogWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return (LRESULT)1; // Say we handled it.

    case WM_PAINT:
    {
        CTextConsole* pTextConsole = (CTextConsole*)Console;
        pTextConsole->OnPaint();
        return (LRESULT)0; // Say we handled it.
    }
    break;
    default: break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
#endif
