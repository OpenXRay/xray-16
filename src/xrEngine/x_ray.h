#ifndef __X_RAY_H__
#define __X_RAY_H__

#include <mutex>

#include "xrEngine/Engine.h"

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
    std::atomic_bool m_should_exit;

    SDL_Surface* m_surface;

private:
    std::mutex m_discord_lock;
    discord::Core* m_discord_core{};

private:
    GameModule* m_game_module{};

private:
    void SplashProc();

    void ShowSplash(bool topmost);
    void HideSplash();

    void InitializeDiscord();
    void UpdateDiscordStatus();

public:
    // Other
    CApplication(pcstr commandLine, GameModule* game);
    ~CApplication();

    int Run();
};

#endif //__XR_BASE_H__
