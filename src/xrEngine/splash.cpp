#include "stdafx.h"
#include "xr_3da/resource.h"
#include "splash.h"

#if defined(WINDOWS)
HWND logoWindow = nullptr;
#else
SDL_Window* logoWindow = nullptr;
SDL_Renderer *logoRenderer;
#endif

#if defined(WINDOWS)
static INT_PTR CALLBACK LogoWndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_DESTROY: break;
    case WM_CLOSE: DestroyWindow(hw); break;
    case WM_COMMAND:
        if (LOWORD(wp) == IDCANCEL)
            DestroyWindow(hw);
        break;
    default: return false;
    }
    return true;
}
#endif

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
#else
    SDL_CreateWindowAndRenderer(320, 240, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN, &logoWindow, &logoRenderer);

    SDL_Surface *surface = SDL_LoadBMP("logo.bmp"); // need placed logo.bmp beside of fsgame.ltx
    if (!surface) {
        Msg("Couldn't create surface from image: %s", SDL_GetError());
        return;
    }

    SDL_Rect rect;
    SDL_GetClipRect(surface, &rect);
    SDL_SetWindowSize(logoWindow, rect.w, rect.h);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(logoRenderer, surface);
    SDL_FreeSurface(surface);
    SDL_ShowWindow(logoWindow);

    SDL_SetRenderDrawColor(logoRenderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(logoRenderer);
    SDL_RenderCopy(logoRenderer, texture, NULL, NULL);
    SDL_RenderPresent(logoRenderer);
    SDL_UpdateWindowSurface(logoWindow);

#endif
}

void hide()
{
    if (logoWindow != nullptr)
    {
#if defined(WINDOWS)
        DestroyWindow(logoWindow);
#else
        SDL_DestroyWindow(logoWindow);
#endif
        logoWindow = nullptr;
    }
}
}
