#include "stdafx.h"
#include "xr_3da/resource.h"
#include "splash.h"

SDL_Window* logoWindow = nullptr;

SDL_Surface* XRSDL_SurfaceVerticalFlip(SDL_Surface*& source)
{
    const auto pitch = source->pitch;
    const auto size = pitch * source->h;

    auto original = static_cast<u8*>(alloca(size));
    CopyMemory(original, source->pixels, size);

    auto flipped = static_cast<u8*>(source->pixels) + size;

    for (auto line = 0; line < source->h; ++line)
    {
        CopyMemory(flipped, original, pitch);
        original += pitch;
        flipped -= pitch;
    }

    return source;
}

namespace splash
{
void show(const bool topmost)
{
    if (logoWindow)
        return;

#ifdef WINDOWS
    BITMAP splash;
    HBITMAP bitmapHandle = (HBITMAP)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    const int bitmapSize = GetObject(bitmapHandle, sizeof(BITMAP), &splash);

    if (0 == bitmapSize)
    {
        DeleteObject(bitmapHandle);
        return;
    }

    constexpr Uint32 alpha = 0xFF000000;
    constexpr Uint32 red   = 0x00FF0000;
    constexpr Uint32 green = 0x0000FF00;
    constexpr Uint32 blue  = 0x000000FF;

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        splash.bmBits, splash.bmWidth, splash.bmHeight,
        splash.bmBitsPixel, splash.bmWidthBytes,
        red, green, blue, alpha);
#else
    SDL_Surface* surface = SDL_LoadBMP("logo.bmp"); // need placed logo.bmp beside of fsgame.ltx
#endif // WINDOWS

    if (!surface)
    {
        Log("Couldn't create surface from image:", SDL_GetError());
        return;
    }

    Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN | SDL_WINDOW_SKIP_TASKBAR;
    if (topmost)
        flags |= SDL_WINDOW_ALWAYS_ON_TOP;

    logoWindow = SDL_CreateWindow("OpenXRay", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, surface->w, surface->h, flags);
    const auto logoSurface = SDL_GetWindowSurface(logoWindow);

#ifdef WINDOWS
    XRSDL_SurfaceVerticalFlip(surface);
#endif
    SDL_BlitSurface(surface, nullptr, logoSurface, nullptr);

    SDL_FreeSurface(surface);
    SDL_ShowWindow(logoWindow);
    SDL_UpdateWindowSurface(logoWindow);
}

void hide()
{
    if (logoWindow != nullptr)
    {
        SDL_DestroyWindow(logoWindow);
        logoWindow = nullptr;
    }
}
} // namespace splash
