#ifndef __X_RAY_H__
#define __X_RAY_H__

#include <mutex>

#include "xrCore/Threading/Event.hpp"

struct SDL_Window;
struct SDL_Surface;

namespace discord
{
class Core;
}

// definition
class ENGINE_API CApplication final
{
    SDL_Window* m_window{};
    std::thread m_splash_thread;
    Event m_should_exit;

    size_t m_current_surface_idx{ size_t(-1) };
    xr_vector<SDL_Surface*> m_surfaces;

private:
    std::mutex m_discord_lock;
    discord::Core* m_discord_core{};

private:
    void SplashProc();

    void ShowSplash(bool topmost);
    void HideSplash();

    void UpdateDiscordStatus();

public:
    // Other
    CApplication(pcstr commandLine);
    ~CApplication();

    int Run();
};

#endif //__XR_BASE_H__
