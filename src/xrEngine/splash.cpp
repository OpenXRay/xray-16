#include "stdafx.h"
#include "splash.h"
#include "embedded_resources_management.h"

#include <thread>

constexpr u32 SPLASH_FRAMERATE = 30;

class splash_screen
{
    SDL_Window* m_window;
    Event m_should_exit;
    bool m_thread_operational;

    size_t m_current_surface_idx;
    xr_vector<SDL_Surface*> m_surfaces;

public:
    splash_screen() : m_window(nullptr), m_current_surface_idx(0) {}

    static void splash_proc(void* self_ptr)
    {
        auto& self = *static_cast<splash_screen*>(self_ptr);
        self.m_thread_operational = true;

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

        for (SDL_Surface* surface : self.m_surfaces)
            SDL_FreeSurface(surface);
        self.m_surfaces.clear();

        SDL_DestroyWindow(self.m_window);
        self.m_window = nullptr;

        self.m_thread_operational = false;
    }

    void show(bool topmost)
    {
        if (m_window)
            return;

        m_surfaces = std::move(ExtractSplashScreen());

        if (m_surfaces.empty())
        {
            Log("! Couldn't create surface from image:", SDL_GetError());
            return;
        }

        Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN;

#if SDL_VERSION_ATLEAST(2,0,5)
        if (topmost)
            flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#endif

        SDL_Surface* surface = m_surfaces.front();
        m_window = SDL_CreateWindow("OpenXRay", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, surface->w, surface->h, flags);

        const auto current = SDL_GetWindowSurface(m_window);
        SDL_BlitSurface(surface, nullptr, current, nullptr);
        SDL_ShowWindow(m_window);
        SDL_UpdateWindowSurface(m_window);

        Threading::SpawnThread(splash_proc, "X-Ray Splash Thread", 0, this);

        while (!m_thread_operational)
            SDL_PumpEvents();
        SDL_PumpEvents();
    }

    void hide()
    {
        m_should_exit.Set();
        while (m_thread_operational)
        {
            SDL_PumpEvents();
            std::this_thread::yield();
        }
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
