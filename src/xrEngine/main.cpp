// Entry point is in xr_3da/entry_point.cpp
#include "stdafx.h"
#include "main.h"

#if defined(XR_PLATFORM_WINDOWS)
#include <process.h>
#endif
#include <locale.h>

#include "IGame_Persistent.h"
#include "xrNetServer/NET_AuthCheck.h"
#include "xr_input.h"
#include "XR_IOConsole.h"
#include "x_ray.h"
#include "std_classes.h"

#include "LightAnimLibrary.h"
#include "xrCDB/ISpatial.h"
#if defined(XR_PLATFORM_WINDOWS)
#include "Text_Console.h"
#elif defined(XR_PLATFORM_LINUX)
#define CTextConsole CConsole
#pragma todo("Implement text console or it's alternative")
#endif
#if !defined(XR_PLATFORM_LINUX)
#include "xrSASH.h"
#endif
#include "xr_ioc_cmd.h"

#include "xrCore/Threading/TaskManager.hpp"
#include "xrCore/command_line_key.h"

// global variables
ENGINE_API CInifile* pGameIni = nullptr;
ENGINE_API bool g_bBenchmark = false;
string512 g_sBenchmarkName;
ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
ENGINE_API string_path g_sLaunchWorkingFolder;

ENGINE_API bool CallOfPripyatMode = false;
ENGINE_API bool ClearSkyMode = false;
ENGINE_API bool ShadowOfChernobylMode = false;

static command_line_key<bool> unlock_game_mode("-unlock_game_mode", "unlock_game_mode", false);
static command_line_key<pcstr> ltx_flag("-ltx", "ltx", "");
static command_line_key<bool> captureInput("-i", "Capture input", false);
static command_line_key<bool> gl_flag("-rgl", "opengl renderer", false);
static command_line_key<bool> r4_flag("-r4", "r4 renderer", false);
static command_line_key<bool> r3_flag("-r3", "r3 renderer", false);
static command_line_key<bool> r25_flag("-r2.5", "r2.5 renderer", false);
static command_line_key<bool> r2a_flag("-r2a", "r2a renderer", false);
static command_line_key<bool> r2_flag("-r2", "r2 renderer", false);
static command_line_key<bool> r1_flag("-r1", "r1 renderer", false);
#ifdef DEBUG
static command_line_key<bool> slowdown_flag("-slowdown", "slowdown", false);
static command_line_key<bool> slowdown2x_flag("-slowdown2x", "slowdown2x", false);
#endif


extern XRCORE_API command_line_key<pcstr> fsltx_path;
extern XRCORE_API command_line_key<bool> shoc_flag, soc_flag, cs_flag, cop_flag;

namespace
{
bool CheckBenchmark();
void RunBenchmark(pcstr name);
}

ENGINE_API void InitEngine()
{
    Engine.Initialize();
    Device.Initialize();

    Console->OnDeviceInitialize();
}

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

ENGINE_API void InitSettings()
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

    if (shoc_flag.OptionValue() || soc_flag.OptionValue())
        set_shoc_mode();
    else if (cs_flag.OptionValue())
        set_cs_mode();
    else if (cop_flag.OptionValue())
        set_cop_mode();
    else if (unlock_game_mode.OptionValue())
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

ENGINE_API void InitConsole()
{
    if (GEnv.isDedicatedServer)
        Console = xr_new<CTextConsole>();
    else
        Console = xr_new<CConsole>();

    Console->Initialize();
    xr_strcpy(Console->ConfigFile, "user.ltx");
    if (ltx_flag.IsProvided())
        xr_strcpy(Console->ConfigFile, ltx_flag.OptionValue());
}

ENGINE_API void InitInput()
{
    pInput = xr_new<CInput>(!captureInput.OptionValue() && !GEnv.isEditor);
}

ENGINE_API void destroyInput() { xr_delete(pInput); }
ENGINE_API void InitSound() { ISoundManager::_create(); }
ENGINE_API void destroySound() { ISoundManager::_destroy(); }
ENGINE_API void destroySettings()
{
    auto s = const_cast<CInifile**>(&pSettings);
    xr_delete(*s);

    auto sa = const_cast<CInifile**>(&pSettingsAuth);
    xr_delete(*sa);

    auto so = const_cast<CInifile**>(&pSettingsOpenXRay);
    xr_delete(*so);

    xr_delete(pGameIni);
}

ENGINE_API void destroyConsole()
{
    Console->Execute("cfg_save");
    Console->Destroy();
    xr_delete(Console);
}

ENGINE_API void destroyEngine()
{
    Device.Destroy();
    Engine.Destroy();
}

void execUserScript()
{
    Console->Execute("default_controls");
    Console->ExecuteScript(Console->ConfigFile);
}

void CheckAndSetupRenderer()
{
    if (GEnv.isDedicatedServer)
    {
        Console->Execute("renderer renderer_r1");
        return;
    }

    if (gl_flag.OptionValue())
        Console->Execute("renderer renderer_rgl");
    else if (r4_flag.OptionValue())
        Console->Execute("renderer renderer_r4");
    else if (r3_flag.OptionValue())
        Console->Execute("renderer renderer_r3");
    else if (r25_flag.OptionValue())
        Console->Execute("renderer renderer_r2.5");
    else if (r2a_flag.OptionValue())
        Console->Execute("renderer renderer_r2a");
    else if (r2_flag.OptionValue())
        Console->Execute("renderer renderer_r2");
    else if (r1_flag.OptionValue())
        Console->Execute("renderer renderer_r1");
    else
    {
        CCC_LoadCFG_custom cmd("renderer ");
        cmd.Execute(Console->ConfigFile);
        renderer_allow_override = true;
    }
}

void slowdownthread(void*)
{
    for (;;)
    {
        if (Device.GetStats().fFPS < 30)
            Sleep(1);
        if (Device.mt_bMustExit || !pSettings || !Console || !pInput || !pApp)
            return;
    }
}
void CheckPrivilegySlowdown()
{
#ifdef DEBUG
    if (slowdown_flag.OptionValue())
        Threading::SpawnThread(slowdownthread, "slowdown", 0, 0);
    if (slowdown2x_flag.OptionValue())
    {
        Threading::SpawnThread(slowdownthread, "slowdown", 0, 0);
        Threading::SpawnThread(slowdownthread, "slowdown", 0, 0);
    }
#endif
}

ENGINE_API void Startup()
{
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

    const auto& createApplication = TaskScheduler->AddTask("CreateApplication()", [](Task&, void*)
    {
        pApp = xr_new<CApplication>();
#ifdef XR_PLATFORM_WINDOWS // XXX: Remove this macro check
        if (GEnv.isDedicatedServer)
            pApp->SetLoadingScreen(xr_new<TextLoadingScreen>());
#endif
    });
    const auto& createSpatialSpace = TaskScheduler->AddTask("CreateSpatialSpace()", [](Task&, void*)
    {
        g_SpatialSpace = xr_new<ISpatial_DB>("Spatial obj");
        g_SpatialSpacePhysic = xr_new<ISpatial_DB>("Spatial phys");
    });

    Device.Create();
    TaskScheduler->Wait(createLightAnim);
    TaskScheduler->Wait(createApplication);
    TaskScheduler->Wait(createSpatialSpace);

    g_pGamePersistent = dynamic_cast<IGame_Persistent*>(NEW_INSTANCE(CLSID_GAME_PERSISTANT));
    R_ASSERT(g_pGamePersistent || Engine.External.CanSkipGameModuleLoading());

    // Main cycle
    Device.Run();
    // Destroy APP
    xr_delete(g_SpatialSpacePhysic);
    xr_delete(g_SpatialSpace);
    DEL_INSTANCE(g_pGamePersistent);
    xr_delete(pApp);
    Engine.Event.Dump();
    // Destroying
    destroyInput();
#if !defined(XR_PLATFORM_LINUX)
    if (!g_bBenchmark && !g_SASH.IsRunning())
#endif
        destroySettings();
    LALib.OnDestroy();
#if !defined(XR_PLATFORM_LINUX)
    if (!g_bBenchmark && !g_SASH.IsRunning())
#endif
        destroyConsole();
#if !defined(XR_PLATFORM_LINUX)
    else
        Console->Destroy();
#endif
    Device.CleanupVideoModes();
    destroyEngine();
    destroySound();
}

ENGINE_API int RunApplication()
{
    R_ASSERT2(Core.Params, "Core must be initialized");

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

    Engine.External.CreateRendererList();
    CheckAndSetupRenderer();

    Engine.External.Initialize();
    InitEngine();

    if (CheckBenchmark())
        return 0;

    Startup();
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
    return 0;
}

namespace
{
bool CheckBenchmark()
{
    pcstr benchName = "-batch_benchmark ";
    if (strstr(Core.Params, benchName))
    {
        const size_t sz = xr_strlen(benchName);
        string64 benchmarkName;
        sscanf(strstr(Core.Params, benchName) + sz, "%[^ ] ", benchmarkName);
        RunBenchmark(benchmarkName);
        return true;
    }

    pcstr sashName = "-openautomate ";
    if (strstr(Core.Params, sashName))
    {
        const size_t sz = xr_strlen(sashName);
        string512 sashArg;
        sscanf(strstr(Core.Params, sashName) + sz, "%[^ ] ", sashArg);
#if !defined(XR_PLATFORM_LINUX)
        g_SASH.Init(sashArg);
        g_SASH.MainLoop();
#endif
        return true;
    }

    return false;
}
void RunBenchmark(pcstr name)
{
    g_bBenchmark = true;
    string_path cfgPath;
    FS.update_path(cfgPath, "$app_data_root$", name);
    CInifile ini(cfgPath);
    const u32 benchmarkCount = ini.line_count("benchmark");
    const size_t hyphenLtxLen = xr_strlen("-ltx ");
    for (u32 i = 0; i < benchmarkCount; i++)
    {
        pcstr benchmarkName, t;
        ini.r_line("benchmark", i, &benchmarkName, &t);
        xr_strcpy(g_sBenchmarkName, benchmarkName);
        shared_str benchmarkCommand = ini.r_string_wb("benchmark", benchmarkName);
        u32 cmdSize = benchmarkCommand.size() + 1;
        Core.Params = (char*)xr_realloc(Core.Params, cmdSize);
        xr_strcpy(Core.Params, cmdSize, benchmarkCommand.c_str());
        xr_strlwr(Core.Params);
        InitInput();
        Engine.External.Initialize();
        if (i)
            InitEngine();
        xr_strcpy(Console->ConfigFile, "user.ltx");
        if (strstr(Core.Params, "-ltx "))
        {
            string64 cfgName;
            sscanf(strstr(Core.Params, "-ltx ") + hyphenLtxLen, "%[^ ] ", cfgName);
            xr_strcpy(Console->ConfigFile, cfgName);
        }
        Startup();
    }
}
}
