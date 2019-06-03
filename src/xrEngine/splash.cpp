#include "stdafx.h"
#include "xr_3da/resource.h"
#include "splash.h"

constexpr u32 SPLASH_FRAMERATE = 30;

SDL_Surface* XRSDL_SurfaceVerticalFlip(SDL_Surface*& source)
{
    const size_t pitch = source->pitch;
    const size_t size = pitch * source->h;

    // XXX: get rid of alloca usage, possible stack overflow
    //auto original = new u8(size);

    auto original = static_cast<u8*>(alloca(size));
    CopyMemory(original, source->pixels, size);

    auto flipped = static_cast<u8*>(source->pixels) + size;

    for (auto line = 0; line < source->h; ++line)
    {
        CopyMemory(flipped, original, pitch);
        original += pitch;
        flipped -= pitch;
    }

    //xr_delete(original);
    return source;
}

#ifdef WINDOWS
SDL_Surface* ExtractSurfaceFromApplication(int idx)
{
    BITMAP splash;
    const HBITMAP bitmapHandle = (HBITMAP)LoadImage(GetModuleHandle(nullptr), // NOLINT
        MAKEINTRESOURCE(idx), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    const int bitmapSize = GetObject(bitmapHandle, sizeof(BITMAP), &splash);

    if (0 == bitmapSize)
    {
        DeleteObject(bitmapHandle);
        return nullptr;
    }

    constexpr Uint32 alpha = 0xFF000000;
    constexpr Uint32 red   = 0x00FF0000;
    constexpr Uint32 green = 0x0000FF00;
    constexpr Uint32 blue  = 0x000000FF;

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        splash.bmBits, splash.bmWidth, splash.bmHeight,
        splash.bmBitsPixel, splash.bmWidthBytes,
        red, green, blue, alpha);

    XRSDL_SurfaceVerticalFlip(surface);
    return surface;
}
#endif // WINDOWS

class splash_screen
{
    SDL_Window* m_window;
    Event m_should_exit;

    size_t m_current_surface_idx;
    xr_vector<SDL_Surface*> m_surfaces;

public:
    splash_screen() : m_window(nullptr), m_current_surface_idx(0) {}

    static void splash_proc(void* self_ptr)
    {
        auto& self = *static_cast<splash_screen*>(self_ptr);

        while (true)
        {
            if (self.m_should_exit.Wait(SPLASH_FRAMERATE))
                break;

            if (self.m_surfaces.size() > 1)
            {
                if (self.m_current_surface_idx >= self.m_surfaces.size())
                    self.m_current_surface_idx = 0;

                const auto current = SDL_GetWindowSurface(self.m_window);
                const auto next = self.m_surfaces[self.m_current_surface_idx++]; // It's important to have postfix increment!
                SDL_BlitSurface(next, nullptr, current, nullptr);
                SDL_UpdateWindowSurface(self.m_window);
            }
        }

        SDL_DestroyWindow(self.m_window);
        self.m_window = nullptr;
        for (SDL_Surface* surface : self.m_surfaces)
            SDL_FreeSurface(surface);
        self.m_surfaces.clear();
    }

    void show(bool topmost)
    {
        if (m_window)
            return;

        SDL_Surface* surface = nullptr;

        // XXX: that's the place, where splash frames can be added
        // Animated splash screen!
#ifdef WINDOWS
        surface = ExtractSurfaceFromApplication(IDB_BITMAP1);
#else
        surface = SDL_LoadBMP("logo.bmp"); // You need to place logo.bmp beside fsgame.ltx
#endif
        if (!surface)
        {
            Log("! Couldn't create surface from image:", SDL_GetError());
            return;
        }

        Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN | SDL_WINDOW_SKIP_TASKBAR;
        if (topmost)
            flags |= SDL_WINDOW_ALWAYS_ON_TOP;

        m_window = SDL_CreateWindow("OpenXRay", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, surface->w, surface->h, flags);

        m_surfaces.push_back(surface);
        const auto current = SDL_GetWindowSurface(m_window);
        SDL_BlitSurface(surface, nullptr, current, nullptr);
        SDL_ShowWindow(m_window);
        SDL_UpdateWindowSurface(m_window);

        m_should_exit.Reset();
        thread_spawn(splash_proc, "X-Ray Splash Thread", 0, this);
    }

    void hide()
    {
        m_should_exit.Set();
    }
} g_splash_screen;

namespace splash
{
void show(const bool topmost)
{
    g_splash_screen.show(topmost);
}

void hide()
{
    g_splash_screen.hide();
}
} // namespace splash
