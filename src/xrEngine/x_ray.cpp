//-----------------------------------------------------------------------------
// File: x_ray.cpp
//
// Programmers:
// Oles - Oles Shishkovtsov
// AlexMX - Alexander Maksimchuk
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "x_ray.h"

#include "main.h"
#include "AccessibilityShortcuts.hpp"
#include "embedded_resources_management.h"

//#define PROFILE_TASK_SYSTEM

#ifdef PROFILE_TASK_SYSTEM
#include "xrCore/Threading/ParallelForEach.hpp"
#endif

constexpr u32 SPLASH_FRAMERATE = 30;

CApplication::CApplication(pcstr commandLine)
{
    xrDebug::Initialize(commandLine);
    R_ASSERT3(SDL_Init(SDL_INIT_VIDEO) == 0, "Unable to initialize SDL", SDL_GetError());

#ifdef XR_PLATFORM_WINDOWS
    AccessibilityShortcuts shortcuts;
    if (!GEnv.isDedicatedServer)
        shortcuts.Disable();
#endif

    if (!strstr(commandLine, "-nosplash"))
    {
        const bool topmost = !strstr(commandLine, "-splashnotop");
#ifndef PROFILE_TASK_SYSTEM
        ShowSplash(topmost);
#endif
    }

    pcstr fsltx = "-fsltx ";
    string_path fsgame = "";
    if (strstr(commandLine, fsltx))
    {
        const size_t sz = xr_strlen(fsltx);
        sscanf(strstr(commandLine, fsltx) + sz, "%[^ ] ", fsgame);
    }

    Core.Initialize("OpenXRay", commandLine, nullptr, true, *fsgame ? fsgame : nullptr);

#ifdef PROFILE_TASK_SYSTEM
    const auto task = [](const TaskRange<int>&){};

    constexpr int task_count = 1048576;
    constexpr int iterations = 250;
    u64 results[iterations];

    CTimer timer;
    for (int i = 0; i < iterations; ++i)
    {
        timer.Start();
        xr_parallel_for(TaskRange(0, task_count, 1), task);
        results[i] = timer.GetElapsed_ns();
    }

    u64 min = std::numeric_limits<u64>::max();
    u64 average{};
    for (int i = 0; i < iterations; ++i)
    {
        min = std::min(min, results[i]);
        average += results[i] / 1000;
        Log("Time:", results[i]);
    }
    Msg("Time min: %f microseconds", float(min) / 1000.f);
    Msg("Time average: %f microseconds", float(average) / float(iterations));
#endif
}

CApplication::~CApplication()
{
    Core._destroy();
    SDL_Quit();
}

int CApplication::Run()
{
#ifdef PROFILE_TASK_SYSTEM
    return 0;
#endif
    HideSplash();
    return RunApplication();
}

void CApplication::ShowSplash(bool topmost)
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

    Threading::SpawnThread(SplashProc, "X-Ray Splash Thread", 0, this);

    while (!m_thread_operational)
        SDL_PumpEvents();
    SDL_PumpEvents();
}

void CApplication::SplashProc(void* self_ptr)
{
    auto& self = *static_cast<CApplication*>(self_ptr);
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

void CApplication::HideSplash()
{
    if (!m_window)
        return;

    m_should_exit.Set();
    while (m_thread_operational)
    {
        SDL_PumpEvents();
        SwitchToThread();
    }
}
