#pragma once

#include "xr_3da/resource.h"

#ifdef XR_PLATFORM_WINDOWS
#   include <SDL3/SDL_syswm.h>
#endif

inline SDL_Surface* XRSDL_SurfaceVerticalFlip(SDL_Surface*& source)
{
    ZoneScoped;
    const size_t pitch = source->pitch;
    const size_t size = pitch * source->h;

    // XXX: don't alloc at all, flip surface in-place
    auto ptr = static_cast<u8*>(xr_malloc(size));
    auto original = ptr;

    CopyMemory(original, source->pixels, size);

    auto flipped = static_cast<u8*>(source->pixels) + size;

    for (auto line = 0; line < source->h; ++line)
    {
        CopyMemory(flipped, original, pitch);
        original += pitch;
        flipped -= pitch;
    }

    xr_free(ptr);
    return source;
}

#ifdef XR_PLATFORM_WINDOWS
inline HANDLE ExtractImage(int idx, UINT type)
{
    ZoneScoped;
    return LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(idx),
        type, 0, 0, LR_CREATEDIBSECTION);
}

inline SDL_Surface* CreateSurfaceFromBitmap(HBITMAP bitmapHandle)
{
    ZoneScoped;
    BITMAP bitmap;
    const int bitmapSize = GetObject(bitmapHandle, sizeof(BITMAP), &bitmap);

    if (0 == bitmapSize)
    {
        DeleteObject(bitmapHandle);
        return nullptr;
    }

    constexpr Uint32 alpha = 0xFF000000;
    constexpr Uint32 red = 0x00FF0000;
    constexpr Uint32 green = 0x0000FF00;
    constexpr Uint32 blue = 0x000000FF;

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        bitmap.bmBits, bitmap.bmWidth, bitmap.bmHeight,
        bitmap.bmBitsPixel, bitmap.bmWidthBytes,
        red, green, blue, alpha);

    if (!surface)
        return nullptr;

    if (!surface->pixels)
    {
        SDL_DestroySurface(surface);
        return nullptr;
    }

    XRSDL_SurfaceVerticalFlip(surface);
    return surface;
}

inline SDL_Surface* ExtractBitmap(int idx)
{
    const HBITMAP bitmap = (HBITMAP)ExtractImage(idx, IMAGE_BITMAP);

    return CreateSurfaceFromBitmap(bitmap);
}

inline SDL_Surface* ExtractSplashScreen()
{
    ZoneScoped;
    return ExtractBitmap(IDB_SPLASH);
}

inline void ExtractAndSetWindowIcon(SDL_Window* wnd, int iconIdx)
{
    ZoneScoped;

    const HICON icon = (HICON)ExtractImage(iconIdx, IMAGE_ICON);

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    R_ASSERT2(SDL_GetWindowWMInfo(wnd, &info), SDL_GetError());

    const HWND hwnd = info.info.win.window;
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
}
#else
inline SDL_Surface* ExtractSplashScreen()
{
    ZoneScoped;

    // You need to place logo.bmp beside fsgame.ltx
    return SDL_LoadBMP("logo.bmp");
}

inline void ExtractAndSetWindowIcon(SDL_Window* wnd, int iconIdx)
{
    ZoneScoped;
    SDL_Surface* surface = nullptr;
    switch (iconIdx)
    {
    case IDI_ICON_COP:
        surface = SDL_LoadBMP("icon_cop.bmp");
        break;
    case IDI_ICON_CS:
        surface = SDL_LoadBMP("icon_cs.bmp");
        break;
    case IDI_ICON_SOC:
        surface = SDL_LoadBMP("icon_soc.bmp");
        break;
    }
    if (surface)
        SDL_SetWindowIcon(wnd, surface);
}
#endif // XR_PLATFORM_WINDOWS
