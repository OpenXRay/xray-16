#ifndef __X_RAY_H__
#define __X_RAY_H__

#include <mutex>
#include <array>
#include <filesystem>

#include "xrEngine/Engine.h"

struct SDL_Window;
struct SDL_Surface;

namespace discord
{
class Core;
}

#if defined(XR_PLATFORM_ANDROID)
// Installed before the native engine starts so a crash in SDL, GLES setup, or
// the first renderer call leaves a small async-signal-safe record on disk.
void android_install_crash_handler();
void android_engine_log_early(pcstr message);
void android_set_load_context(pcstr context);
#endif

// definition
class ENGINE_API CApplication final
{
    SDL_Window* m_window{};
    std::thread m_splash_thread;
    std::atomic_bool m_should_exit;

    SDL_Surface* m_surface;
    bool m_headless_smoke{};
    bool m_renderer_smoke{};
    bool m_renderer_vulkan_smoke{};
    std::filesystem::path m_headless_root;
    void* m_renderer_smoke_state{};

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
    CApplication(pcstr commandLine, GameModule* game, const std::array<RendererModule*, 2>& modules);
    ~CApplication();

    int Run();
};

#endif //__XR_BASE_H__
