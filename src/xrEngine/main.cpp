// Entry point is in xr_3da/entry_point.cpp
#include "stdafx.h"
#include "main.h"

#if defined(WINDOWS)
#include <process.h>
#elif defined(LINUX)
#include <lockfile.h>
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
#if defined(WINDOWS)
#include "Text_Console.h"
#elif defined(LINUX)
#define CTextConsole CConsole
#pragma todo("Implement text console or it's alternative")
#endif
#if !defined(LINUX)
#include "xrSASH.h"
#endif
#include "xr_ioc_cmd.h"

#ifdef MASTER_GOLD
#define NO_MULTI_INSTANCES
#endif

// global variables
ENGINE_API CInifile* pGameIni = nullptr;
ENGINE_API bool g_bBenchmark = false;
string512 g_sBenchmarkName;
ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
ENGINE_API string_path g_sLaunchWorkingFolder;

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
    bool xr_stdcall IsIncluded(pcstr path)
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
    config = new CInifile(fname, readOnly, loadAtStart, saveAtEnd, sectCount, allowIncludeFunc);

    CHECK_OR_EXIT(config->section_count() || !fatal,
        make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
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
}

ENGINE_API void InitConsole()
{
    if (GEnv.isDedicatedServer)
        Console = new CTextConsole();
    else
        Console = new CConsole();

    Console->Initialize();
    xr_strcpy(Console->ConfigFile, "user.ltx");
    if (strstr(Core.Params, "-ltx "))
    {
        string64 c_name;
        sscanf(strstr(Core.Params, "-ltx ") + strlen("-ltx "), "%[^ ] ", c_name);
        xr_strcpy(Console->ConfigFile, c_name);
    }
}

ENGINE_API void InitInput()
{
    bool captureInput = !strstr(Core.Params, "-i") && !GEnv.isEditor;
    pInput = new CInput(captureInput);
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
#if defined(LINUX)
    lockfile_remove("/var/lock/stalker-cop.lock");
#endif
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

    if (strstr(Core.Params, "-gl"))
        Console->Execute("renderer renderer_gl");
    else if (strstr(Core.Params, "-r4"))
        Console->Execute("renderer renderer_r4");
    else if (strstr(Core.Params, "-r3"))
        Console->Execute("renderer renderer_r3");
    else if (strstr(Core.Params, "-r2.5"))
        Console->Execute("renderer renderer_r2.5");
    else if (strstr(Core.Params, "-r2a"))
        Console->Execute("renderer renderer_r2a");
    else if (strstr(Core.Params, "-r2"))
        Console->Execute("renderer renderer_r2");
    else if (strstr(Core.Params, "-r1"))
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
    if (strstr(Core.Params, "-slowdown"))
        thread_spawn(slowdownthread, "slowdown", 0, 0);
    if (strstr(Core.Params, "-slowdown2x"))
    {
        thread_spawn(slowdownthread, "slowdown", 0, 0);
        thread_spawn(slowdownthread, "slowdown", 0, 0);
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
    Device.Create();
    LALib.OnCreate();
    pApp = new CApplication();
    g_pGamePersistent = dynamic_cast<IGame_Persistent*>(NEW_INSTANCE(CLSID_GAME_PERSISTANT));
    R_ASSERT(g_pGamePersistent);
    g_SpatialSpace = new ISpatial_DB("Spatial obj");
    g_SpatialSpacePhysic = new ISpatial_DB("Spatial phys");

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
#if !defined(LINUX)
    if (!g_bBenchmark && !g_SASH.IsRunning())
#endif
        destroySettings();
    LALib.OnDestroy();
#if !defined(LINUX)
    if (!g_bBenchmark && !g_SASH.IsRunning())
#endif
        destroyConsole();
#if !defined(LINUX)
    else
        Console->Destroy();
#endif
    destroyEngine();
    destroySound();
}

ENGINE_API int RunApplication()
{
    R_ASSERT2(Core.Params, "Core must be initialized");

#ifdef NO_MULTI_INSTANCES
    if (!GEnv.isDedicatedServer)
    {
#if defined(WINDOWS)
        CreateMutex(nullptr, TRUE, "Local\\STALKER-COP");
        if (GetLastError() == ERROR_ALREADY_EXISTS)
            return 2;
#elif defined(LINUX)
        int lock_res = lockfile_create("/var/lock/stalker-cop.lock", 0, L_PID);
        if(L_ERROR == lock_res)
            return 2;
#endif
    }
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

    InitInput();
    InitConsole();

    Engine.External.CreateRendererList();
    CheckAndSetupRenderer();

    InitEngine();

    if (CheckBenchmark())
        return 0;

    Engine.External.Initialize();
    Startup();
    // check for need to execute something external
    if (/*xr_strlen(g_sLaunchOnExit_params) && */ xr_strlen(g_sLaunchOnExit_app))
    {
#if defined(WINDOWS)
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
        const u32 sz = xr_strlen(benchName);
        string64 benchmarkName;
        sscanf(strstr(Core.Params, benchName) + sz, "%[^ ] ", benchmarkName);
        RunBenchmark(benchmarkName);
        return true;
    }

    pcstr sashName = "-openautomate ";
    if (strstr(Core.Params, sashName))
    {
        const u32 sz = xr_strlen(sashName);
        string512 sashArg;
        sscanf(strstr(Core.Params, sashName) + sz, "%[^ ] ", sashArg);
#if !defined(LINUX)
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
    for (u32 i = 0; i < benchmarkCount; i++)
    {
        LPCSTR benchmarkName, t;
        ini.r_line("benchmark", i, &benchmarkName, &t);
        xr_strcpy(g_sBenchmarkName, benchmarkName);
        shared_str benchmarkCommand = ini.r_string_wb("benchmark", benchmarkName);
        u32 cmdSize = benchmarkCommand.size() + 1;
        Core.Params = (char*)xr_realloc(Core.Params, cmdSize);
        xr_strcpy(Core.Params, cmdSize, benchmarkCommand.c_str());
        xr_strlwr(Core.Params);
        InitInput();
        if (i)
            InitEngine();
        Engine.External.Initialize();
        xr_strcpy(Console->ConfigFile, "user.ltx");
        if (strstr(Core.Params, "-ltx "))
        {
            string64 cfgName;
            sscanf(strstr(Core.Params, "-ltx ") + strlen("-ltx "), "%[^ ] ", cfgName);
            xr_strcpy(Console->ConfigFile, cfgName);
        }
        Startup();
    }
}
}
