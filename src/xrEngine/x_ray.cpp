//-----------------------------------------------------------------------------
// File: x_ray.cpp
//
// Programmers:
// Oles - Oles Shishkovtsov
// AlexMX - Alexander Maksimchuk
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "x_ray.h"

#include "embedded_resources_management.h"

#include "xrCore/Threading/TaskManager.hpp"
#include "xrNetServer/NET_AuthCheck.h"

#include "std_classes.h"
#include "IGame_Persistent.h"
#include "LightAnimLibrary.h"
#include "XR_IOConsole.h"
#include "xrSASH.h"

#if defined(XR_PLATFORM_WINDOWS)
#include "AccessibilityShortcuts.hpp"
#include "Text_Console.h"
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
#define CTextConsole CConsole
#pragma todo("Implement text console or it's alternative")
#endif

#ifdef XR_PLATFORM_WINDOWS
#include <locale>

#include "DiscordGameSDK/discord.h"
#define USE_DISCORD_INTEGRATION

#include "xrCore/Text/StringConversion.hpp"
#endif

//#define PROFILE_TASK_SYSTEM

#ifdef PROFILE_TASK_SYSTEM
#include "xrCore/Threading/ParallelForEach.hpp"
#endif

// global variables
constexpr u32 SPLASH_FRAMERATE = 30;

constexpr size_t MAX_WINDOW_EVENTS = 32;

#ifdef USE_DISCORD_INTEGRATION
constexpr discord::ClientId DISCORD_APP_ID = 421286728695939072;
#endif

ENGINE_API CInifile* pGameIni = nullptr;
ENGINE_API bool CallOfPripyatMode = false;
ENGINE_API bool ClearSkyMode = false;
ENGINE_API bool ShadowOfChernobylMode = false;

ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
ENGINE_API string_path g_sLaunchWorkingFolder;

namespace
{
struct PathIncludePred
{
private:
    const xr_auth_strings_t* ignored;

public:
    explicit PathIncludePred(const xr_auth_strings_t* ignoredPaths) : ignored(ignoredPaths) {}
    bool IsIncluded(pcstr path)
    {
        if (!ignored)
            return true;

        return allow_to_include_path(*ignored, path);
    }
};
}

template <typename T>
void InitConfig(T& config, pcstr name, bool fatal = true,
    bool readOnly = true, bool loadAtStart = true, bool saveAtEnd = true,
    u32 sectCount = 0, const CInifile::allow_include_func_t& allowIncludeFunc = nullptr)
{
    string_path fname;
    FS.update_path(fname, "$game_config$", name);
    config = xr_new<CInifile>(fname, readOnly, loadAtStart, saveAtEnd, sectCount, allowIncludeFunc);

    CHECK_OR_EXIT(config->section_count() || !fatal,
        make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
}

// XXX: make it more fancy
// некрасиво слишком
void set_shoc_mode()
{
    CallOfPripyatMode = false;
    ShadowOfChernobylMode = true;
    ClearSkyMode = false;
}

void set_cs_mode()
{
    CallOfPripyatMode = false;
    ShadowOfChernobylMode = false;
    ClearSkyMode = true;
}

void set_cop_mode()
{
    CallOfPripyatMode = true;
    ShadowOfChernobylMode = false;
    ClearSkyMode = false;
}

void set_free_mode()
{
    CallOfPripyatMode = false;
    ShadowOfChernobylMode = false;
    ClearSkyMode = false;
}

void InitSettings()
{
    xr_auth_strings_t ignoredPaths, checkedPaths;
    fill_auth_check_params(ignoredPaths, checkedPaths); //TODO port xrNetServer to Linux
    PathIncludePred includePred(&ignoredPaths);
    CInifile::allow_include_func_t includeFilter;
    includeFilter.bind(&includePred, &PathIncludePred::IsIncluded);

    InitConfig(pSettings, "system.ltx");
    InitConfig(pSettingsAuth, "system.ltx", true, true, true, false, 0, includeFilter);
    InitConfig(pSettingsOpenXRay, "openxray.ltx", false, true, true, false);
    InitConfig(pGameIni, "game.ltx");

    if (strstr(Core.Params, "-shoc") || strstr(Core.Params, "-soc"))
        set_shoc_mode();
    else if (strstr(Core.Params, "-cs"))
        set_cs_mode();
    else if (strstr(Core.Params, "-cop"))
        set_cop_mode();
    else if (strstr(Core.Params, "-unlock_game_mode"))
        set_free_mode();
    else
    {
        pcstr gameMode = READ_IF_EXISTS(pSettingsOpenXRay, r_string, "compatibility", "game_mode", "cop");
        if (xr_strcmpi("cop", gameMode) == 0)
            set_cop_mode();
        else if (xr_strcmpi("cs", gameMode) == 0)
            set_cs_mode();
        else if (xr_strcmpi("shoc", gameMode) == 0 || xr_strcmpi("soc", gameMode) == 0)
            set_shoc_mode();
        else if (xr_strcmpi("unlock", gameMode) == 0)
            set_free_mode();
    }
}

void InitConsole()
{
    if (GEnv.isDedicatedServer)
        Console = xr_new<CTextConsole>();
    else
        Console = xr_new<CConsole>();

    Console->Initialize();
    xr_strcpy(Console->ConfigFile, "user.ltx");
    if (strstr(Core.Params, "-ltx "))
    {
        string64 c_name;
        sscanf(strstr(Core.Params, "-ltx ") + strlen("-ltx "), "%[^ ] ", c_name);
        xr_strcpy(Console->ConfigFile, c_name);
    }
}

void InitInput()
{
    bool captureInput = !strstr(Core.Params, "-i");
    pInput = xr_new<CInput>(captureInput);
}

void destroyInput() { xr_delete(pInput); }
void InitSoundDeviceList() { Engine.Sound.CreateDevicesList(); }
void InitSound() { Engine.Sound.Create(); }
void destroySound() { Engine.Sound.Destroy(); }
void destroySettings()
{
    auto s = const_cast<CInifile**>(&pSettings);
    xr_delete(*s);

    auto sa = const_cast<CInifile**>(&pSettingsAuth);
    xr_delete(*sa);

    auto so = const_cast<CInifile**>(&pSettingsOpenXRay);
    xr_delete(*so);

    xr_delete(pGameIni);
}

void destroyConsole()
{
    Console->Execute("cfg_save");
    Console->Destroy();
    xr_delete(Console);
}

void execUserScript()
{
    Console->Execute("default_controls");
    Console->ExecuteScript(Console->ConfigFile);
}

CApplication::CApplication(pcstr commandLine)
{
    xrDebug::Initialize(commandLine);
    R_ASSERT3(SDL_Init(SDL_INIT_VIDEO) == 0, "Unable to initialize SDL", SDL_GetError());

#ifdef XR_PLATFORM_WINDOWS
    AccessibilityShortcuts shortcuts;
    if (!GEnv.isDedicatedServer)
        shortcuts.Disable();
#endif

#ifdef USE_DISCORD_INTEGRATION
    discord::Core::Create(DISCORD_APP_ID, discord::CreateFlags::NoRequireDiscord, &m_discord_core);

#   ifndef MASTER_GOLD
    if (m_discord_core)
    {
        const auto level = xrDebug::DebuggerIsPresent() ? discord::LogLevel::Debug : discord::LogLevel::Info;
        m_discord_core->SetLogHook(level, [](discord::LogLevel level, pcstr message)
        {
            switch (level)
            {
            case discord::LogLevel::Error: Log("!", message); break;
            case discord::LogLevel::Warn:  Log("~", message); break;
            case discord::LogLevel::Info:  Log("*", message); break;
            case discord::LogLevel::Debug: Log("#", message); break;
            }
        });
    }
#   endif

    discord::Activity activity{};
    activity.SetType(discord::ActivityType::Playing);
    activity.SetApplicationId(DISCORD_APP_ID);
    activity.SetState("Starting engine...");
    activity.GetAssets().SetLargeImage("logo");
    if (m_discord_core)
    {
        std::lock_guard guard{ m_discord_lock };
        m_discord_core->ActivityManager().UpdateActivity(activity, nullptr);
    }
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

    return;
#endif
    *g_sLaunchOnExit_app = 0;
    *g_sLaunchOnExit_params = 0;

    InitSettings();
    // Adjust player & computer name for Asian
    if (pSettings->line_exist("string_table", "no_native_input"))
    {
        xr_strcpy(Core.UserName, sizeof(Core.UserName), "Player");
        xr_strcpy(Core.CompName, sizeof(Core.CompName), "Computer");
    }

    FPU::m24r();

    Device.FillVideoModes();
    InitInput();
    InitConsole();

    Engine.Initialize();
    Device.Initialize();

    Console->OnDeviceInitialize();
#ifdef USE_DISCORD_INTEGRATION
    const std::locale locale("");
    activity.SetState(StringToUTF8(Core.ApplicationTitle, locale).c_str());
    if (m_discord_core)
    {
        std::lock_guard guard{ m_discord_lock };
        m_discord_core->ActivityManager().UpdateActivity(activity, nullptr);
    }
#endif

    //if (CheckBenchmark())
    //    return 0;

    InitSoundDeviceList();
    execUserScript();
    InitSound();

    // ...command line for auto start
    pcstr startArgs = strstr(Core.Params, "-start ");
    if (startArgs)
        Console->Execute(startArgs + 1);
    pcstr loadArgs = strstr(Core.Params, "-load ");
    if (loadArgs)
        Console->Execute(loadArgs + 1);

    // Initialize APP
    const auto& createLightAnim = TaskScheduler->AddTask("LALib.OnCreate()", [](Task&, void*)
    {
        LALib.OnCreate();
    });

    Device.Create();
    TaskScheduler->Wait(createLightAnim);

    g_pGamePersistent = dynamic_cast<IGame_Persistent*>(NEW_INSTANCE(CLSID_GAME_PERSISTANT));
    R_ASSERT(g_pGamePersistent || Engine.External.CanSkipGameModuleLoading());
    if (!g_pGamePersistent)
        Console->Show();
}

CApplication::~CApplication()
{
#ifndef PROFILE_TASK_SYSTEM
    // Destroy APP
    DEL_INSTANCE(g_pGamePersistent);
    Engine.Event.Dump();

    // Destroying
    destroyInput();
    if (!g_bBenchmark && !g_SASH.IsRunning())
        destroySettings();

    LALib.OnDestroy();

    if (!g_bBenchmark && !g_SASH.IsRunning())
        destroyConsole();
    else
        Console->Destroy();

    Device.CleanupVideoModes();
    destroySound();

    Device.Destroy();
    Engine.Destroy();

#ifdef USE_DISCORD_INTEGRATION
    discord::Core::Destroy(&m_discord_core);
#endif

    // check for need to execute something external
    if (/*xr_strlen(g_sLaunchOnExit_params) && */ xr_strlen(g_sLaunchOnExit_app))
    {
#if defined(XR_PLATFORM_WINDOWS)
        // CreateProcess need to return results to next two structures
        STARTUPINFO si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};
        // We use CreateProcess to setup working folder
        pcstr tempDir = xr_strlen(g_sLaunchWorkingFolder) ? g_sLaunchWorkingFolder : nullptr;
        CreateProcess(g_sLaunchOnExit_app, g_sLaunchOnExit_params, nullptr, nullptr, FALSE, 0, nullptr, tempDir, &si, &pi);
#endif
    }
#endif // PROFILE_TASK_SYSTEM

    Core._destroy();
    SDL_Quit();
}

int CApplication::Run()
{
#ifdef PROFILE_TASK_SYSTEM
    return 0;
#endif

    // Main cycle
    Device.Run();
    HideSplash();

    while (!SDL_QuitRequested()) // SDL_PumpEvents is here
    {
        bool canCallActivate = false;
        bool shouldActivate = false;

        SDL_Event events[MAX_WINDOW_EVENTS];
        const int count = SDL_PeepEvents(events, MAX_WINDOW_EVENTS,
            SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT);

        for (int i = 0; i < count; ++i)
        {
            const SDL_Event event = events[i];

            switch (event.type)
            {
            case SDL_WINDOWEVENT:
            {
                switch (event.window.event)
                {
                case SDL_WINDOWEVENT_SHOWN:
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                case SDL_WINDOWEVENT_RESTORED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                    canCallActivate = true;
                    shouldActivate = true;
                    continue;

                case SDL_WINDOWEVENT_HIDDEN:
                case SDL_WINDOWEVENT_FOCUS_LOST:
                case SDL_WINDOWEVENT_MINIMIZED:
                    canCallActivate = true;
                    shouldActivate = false;
                    continue;

                case SDL_WINDOWEVENT_ENTER:
                    SDL_ShowCursor(SDL_FALSE);
                    continue;

                case SDL_WINDOWEVENT_LEAVE:
                    SDL_ShowCursor(SDL_TRUE);
                    continue;

                case SDL_WINDOWEVENT_CLOSE:
                    Engine.Event.Defer("KERNEL:disconnect");
                    Engine.Event.Defer("KERNEL:quit");
                    continue;
                } // switch (event.window.event)
            }
            } // switch (event.type)

            // Only process event in Device
            // if it wasn't processed in the switch above
            Device.ProcessEvent(event);
        } // for (int i = 0; i < count; ++i)

        // Workaround for screen blinking when there's too much timeouts
        if (canCallActivate)
        {
            Device.OnWindowActivate(shouldActivate);
        }

        Device.ProcessFrame();

        UpdateDiscordStatus();
    } // while (!SDL_QuitRequested())

    Device.Shutdown();

    return 0;
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

    Threading::SpawnThread(+[](void* self_ptr)
    {
        auto& self = *static_cast<CApplication*>(self_ptr);
        self.SplashProc();
    }, "X-Ray Splash Thread", 0, this);

    while (!m_thread_operational)
        SDL_PumpEvents();
    SDL_PumpEvents();
}

void CApplication::SplashProc()
{
    m_thread_operational = true;

    while (true)
    {
        if (m_should_exit.Wait(SPLASH_FRAMERATE))
            break;

        if (m_surfaces.size() > 1)
        {
            if (m_current_surface_idx >= m_surfaces.size())
                m_current_surface_idx = 0;

            const auto current = SDL_GetWindowSurface(m_window);
            const auto next = m_surfaces[m_current_surface_idx++]; // It's important to have postfix increment!
            SDL_BlitSurface(next, nullptr, current, nullptr);
            SDL_UpdateWindowSurface(m_window);
        }
        UpdateDiscordStatus();
    }

    for (SDL_Surface* surface : m_surfaces)
        SDL_FreeSurface(surface);
    m_surfaces.clear();

    SDL_DestroyWindow(m_window);
    m_window = nullptr;

    m_thread_operational = false;
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

void CApplication::UpdateDiscordStatus()
{
#ifdef USE_DISCORD_INTEGRATION
    if (!m_discord_core)
        return;

    std::lock_guard guard{ m_discord_lock };
    m_discord_core->RunCallbacks();
#endif
}
