#include "stdafx.h"
#include "IGame_Persistent.h"
#include "xrNetServer/NET_AuthCheck.h"
#include "xr_input.h"
#include "XR_IOConsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "xrCDB/ISpatial.h"
#include "Text_Console.h"
#include <process.h>
#include <locale.h>
#include "xrSASH.h"
#include "xr_ioc_cmd.h"

#ifdef MASTER_GOLD
#define NO_MULTI_INSTANCES
#endif

// global variables
// XXX: use g_dedicated_server from xrAPI
ENGINE_API bool g_dedicated_server = false;
ENGINE_API CApplication *pApp = nullptr;
ENGINE_API CInifile* pGameIni = nullptr;
ENGINE_API bool g_bBenchmark = false;
string512 g_sBenchmarkName;
ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
ENGINE_API string_path g_sLaunchWorkingFolder;

namespace
{
HWND logoWindow = nullptr;

void RunBenchmark(const char *name);
}

void InitEngine()
{
    Engine.Initialize();
    Device.Initialize();
}

static void InitEngineExt()
{
    Engine.External.Initialize();
}

namespace
{
struct PathIncludePred
{
private:
    const xr_auth_strings_t *ignored;

public:
    explicit PathIncludePred(const xr_auth_strings_t *ignoredPaths) :
        ignored(ignoredPaths)
    {}

    bool xr_stdcall IsIncluded(const char *path)
    {
        if (!ignored)
            return true;
        return allow_to_include_path(*ignored, path);
    }
};
}

void InitSettings()
{
    string_path fname;
    FS.update_path(fname, "$game_config$", "system.ltx");
#ifdef DEBUG
    Msg("Updated path to system.ltx is %s", fname);
#endif
    pSettings = new CInifile(fname, TRUE);
    CHECK_OR_EXIT(pSettings->section_count(),
        make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
    xr_auth_strings_t ignoredPaths, checkedPaths;
    fill_auth_check_params(ignoredPaths, checkedPaths);
    PathIncludePred includePred(&ignoredPaths);
    CInifile::allow_include_func_t includeFilter;
    includeFilter.bind(&includePred, &PathIncludePred::IsIncluded);
    pSettingsAuth = new CInifile(fname, TRUE, TRUE, FALSE, 0, includeFilter);
    FS.update_path(fname, "$game_config$", "game.ltx");
    pGameIni = new CInifile(fname, TRUE);
    CHECK_OR_EXIT(pGameIni->section_count(),
        make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
}

void InitConsole()
{
#ifdef DEDICATED_SERVER
    Console = new CTextConsole();
#else
    Console = new CConsole();
#endif
    Console->Initialize();
    xr_strcpy(Console->ConfigFile, "user.ltx");
    if (strstr(Core.Params, "-ltx "))
    {
        string64 c_name;
        sscanf(strstr(Core.Params, "-ltx ")+strlen("-ltx "), "%[^ ] ", c_name);
        xr_strcpy(Console->ConfigFile, c_name);
    }
}

void InitInput()
{
    bool captureInput = !strstr(Core.Params, "-i");
    pInput = new CInput(captureInput);
}

void destroyInput()
{
    xr_delete(pInput);
}

void InitSound()
{
    CSound_manager_interface::_create();
}

void destroySound()
{
    CSound_manager_interface::_destroy();
}

void destroySettings()
{
    auto s = const_cast<CInifile **>(&pSettings);
    xr_delete(*s);
    xr_delete(pGameIni);
}

void destroyConsole()
{
    Console->Execute("cfg_save");
    Console->Destroy();
    xr_delete(Console);
}

void destroyEngine()
{
    Device.Destroy();
    Engine.Destroy();
}

void execUserScript()
{
    Console->Execute("default_controls");
    Console->ExecuteScript(Console->ConfigFile);
}

void slowdownthread(void *)
{
    for (;;)
    {
        if (Device.GetStats().fFPS<30)
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

void Startup()
{
    execUserScript();
    InitSound();
    // ...command line for auto start
    const char *startArgs = strstr(Core.Params, "-start ");
    if (startArgs)
        Console->Execute(startArgs+1);
    const char *loadArgs = strstr(Core.Params, "-load ");
    if (loadArgs)
        Console->Execute(loadArgs+1);
    // Initialize APP
    ShowWindow(Device.m_hWnd, SW_SHOWNORMAL);
    Device.Create();
    LALib.OnCreate();
    pApp = new CApplication();
    g_pGamePersistent = dynamic_cast<IGame_Persistent*>(NEW_INSTANCE(CLSID_GAME_PERSISTANT));
    R_ASSERT(g_pGamePersistent);
    g_SpatialSpace = new ISpatial_DB("Spatial obj");
    g_SpatialSpacePhysic = new ISpatial_DB("Spatial phys");
    // Destroy LOGO
    DestroyWindow(logoWindow);
    logoWindow = NULL;
    // Main cycle
    Memory.mem_usage();
    Device.Run();
    // Destroy APP
    xr_delete(g_SpatialSpacePhysic);
    xr_delete(g_SpatialSpace);
    DEL_INSTANCE(g_pGamePersistent);
    xr_delete(pApp);
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
    destroyEngine();
    destroySound();
}

static BOOL CALLBACK LogoWndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_DESTROY:
        break;
    case WM_CLOSE:
        DestroyWindow(hw);
        break;
    case WM_COMMAND:
        if (LOWORD(wp)==IDCANCEL)
            DestroyWindow(hw);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

class StickyKeyFilter
{
private:
    BOOL screensaverState;
    STICKYKEYS stickyKeys;
    FILTERKEYS filterKeys;
    TOGGLEKEYS toggleKeys;
    DWORD stickyKeysFlags;
    DWORD filterKeysFlags;
    DWORD toggleKeysFlags;

public:
    StickyKeyFilter()
    {
        screensaverState = FALSE;
        SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &screensaverState, 0);
        if (screensaverState)
            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);
        stickyKeysFlags = 0;
        filterKeysFlags = 0;
        toggleKeysFlags = 0;
        stickyKeys = {};
        filterKeys = {};
        toggleKeys = {};
        stickyKeys.cbSize = sizeof(stickyKeys);
        filterKeys.cbSize = sizeof(filterKeys);
        toggleKeys.cbSize = sizeof(toggleKeys);
        SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(stickyKeys), &stickyKeys, 0);
        SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(filterKeys), &filterKeys, 0);
        SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(toggleKeys), &toggleKeys, 0);
        if (stickyKeys.dwFlags & SKF_AVAILABLE)
        {
            stickyKeysFlags = stickyKeys.dwFlags;
            stickyKeys.dwFlags = 0;
            SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(stickyKeys), &stickyKeys, 0);
        }
        if (filterKeys.dwFlags & FKF_AVAILABLE)
        {
            filterKeysFlags = filterKeys.dwFlags;
            filterKeys.dwFlags = 0;
            SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(filterKeys), &filterKeys, 0);
        }
        if (toggleKeys.dwFlags & TKF_AVAILABLE)
        {
            toggleKeysFlags = toggleKeys.dwFlags;
            toggleKeys.dwFlags = 0;
            SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(toggleKeys), &toggleKeys, 0);
        }
    }

    ~StickyKeyFilter()
    {
        if (screensaverState)
            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, NULL, 0);
        if (stickyKeysFlags)
        {
            stickyKeys.dwFlags = stickyKeysFlags;
            SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(stickyKeys), &stickyKeys, 0);
        }
        if (filterKeysFlags)
        {
            filterKeys.dwFlags = filterKeysFlags;
            SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(filterKeys), &filterKeys, 0);
        }
        if (toggleKeysFlags)
        {
            toggleKeys.dwFlags = toggleKeysFlags;
            SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(toggleKeys), &toggleKeys, 0);
        }
    }
};

int RunApplication(const char *commandLine)
{
#ifdef DEDICATED_SERVER
    g_dedicated_server = true;
#endif
    xrDebug::Initialize(g_dedicated_server);
    if (!IsDebuggerPresent())
    {
        u32 heapFragmentation = 2;
        BOOL result = HeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation,
            &heapFragmentation, sizeof(heapFragmentation));
        VERIFY2(result, "can't set process heap low fragmentation");
        (void)result;
    }
#if !defined(DEDICATED_SERVER) && defined(NO_MULTI_INSTANCES)
    CreateMutex(NULL, TRUE, "Local\\STALKER-COP");
    if (GetLastError()==ERROR_ALREADY_EXISTS)
        return 2;
#endif
    SetThreadAffinityMask(GetCurrentThread(), 1);
    logoWindow = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_STARTUP), 0, LogoWndProc);
    HWND logoPicture = GetDlgItem(logoWindow, IDC_STATIC_LOGO);
    RECT logoRect;
    GetWindowRect(logoPicture, &logoRect);
#ifndef DEBUG
    HWND prevWindow = HWND_TOPMOST;
#else
    HWND prevWindow = HWND_NOTOPMOST;
#endif
    SetWindowPos(logoWindow, prevWindow, 0, 0, logoRect.right-logoRect.left, logoRect.bottom-logoRect.top,
        SWP_NOMOVE|SWP_SHOWWINDOW);
    UpdateWindow(logoWindow);
    *g_sLaunchOnExit_app = 0;
    *g_sLaunchOnExit_params = 0;
    const char *fsltx = "-fsltx ";
    string_path fsgame = "";
    if (strstr(commandLine, fsltx))
    {
        u32 sz = xr_strlen(fsltx);
        sscanf(strstr(commandLine, fsltx)+sz, "%[^ ] ", fsgame);
    }
    Core._initialize("xray", NULL, TRUE, *fsgame ? fsgame : nullptr);
    InitSettings();
    // Adjust player & computer name for Asian
    if (pSettings->line_exist("string_table", "no_native_input"))
    {
        xr_strcpy(Core.UserName, sizeof(Core.UserName), "Player");
        xr_strcpy(Core.CompName, sizeof(Core.CompName), "Computer");
    }
#ifndef DEDICATED_SERVER
    StickyKeyFilter filter;
    (void)filter;
#endif
    FPU::m24r();
    InitEngine();
    InitInput();
    InitConsole();
    Engine.External.CreateRendererList();
    Msg("command line %s", commandLine);        
    LPCSTR benchName = "-batch_benchmark ";
    if (strstr(commandLine, benchName))
    {
        u32 sz = xr_strlen(benchName);
        string64 benchmarkName;
        sscanf(strstr(Core.Params, benchName)+sz, "%[^ ] ", benchmarkName);
        RunBenchmark(benchmarkName);
        return 0;
    }
    LPCSTR sashName = "-openautomate ";
    if (strstr(commandLine, sashName))
    {
        u32 sz = xr_strlen(sashName);
        string512 sashArg;
        sscanf(strstr(Core.Params, sashName)+sz, "%[^ ] ", sashArg);
        g_SASH.Init(sashArg);
        g_SASH.MainLoop();
        return 0;
    }        
#ifndef DEDICATED_SERVER
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
    else
    {
        CCC_LoadCFG_custom cmd("renderer ");
        cmd.Execute(Console->ConfigFile);
    }
#else
    Console->Execute("renderer renderer_r1");
#endif
    InitEngineExt(); // load xrRender and xrGame
    Startup();
    Core._destroy();
    // check for need to execute something external
    if (/*xr_strlen(g_sLaunchOnExit_params) && */xr_strlen(g_sLaunchOnExit_app))
    {
        //CreateProcess need to return results to next two structures
        STARTUPINFO si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};
        //We use CreateProcess to setup working folder
        const char *tempDir = xr_strlen(g_sLaunchWorkingFolder) ? g_sLaunchWorkingFolder : nullptr;
        CreateProcess(g_sLaunchOnExit_app, g_sLaunchOnExit_params, NULL, NULL, FALSE, 0, NULL,
            tempDir, &si, &pi);
    }
    return 0;
}

namespace
{
void RunBenchmark(const char *name)
{
    g_bBenchmark = true;
    string_path cfgPath;
    FS.update_path(cfgPath, "$app_data_root$", name);
    CInifile ini(cfgPath);
    u32 benchmarkCount = ini.line_count("benchmark");
    for (u32 i = 0; i<benchmarkCount; i++)
    {
        LPCSTR benchmarkName, t;
        ini.r_line("benchmark", i, &benchmarkName, &t);
        xr_strcpy(g_sBenchmarkName, benchmarkName);
        shared_str benchmarkCommand = ini.r_string_wb("benchmark", benchmarkName);
        u32 cmdSize = benchmarkCommand.size()+1;
        Core.Params = (char *)xr_realloc(Core.Params, cmdSize);
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
            sscanf(strstr(Core.Params, "-ltx ")+strlen("-ltx "), "%[^ ] ", cfgName);
            xr_strcpy(Console->ConfigFile, cfgName);
        }
        Startup();
    }
}

int StackoverflowFilter(int exceptionCode)
{
    if (exceptionCode==EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}
}

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prevInst, char *commandLine, int cmdShow)
{
    int result = 0;
    // BugTrap can't handle stack overflow exception, so handle it here
    __try
    {
        result = RunApplication(commandLine);
    }
    __except (StackoverflowFilter(GetExceptionCode()))
    {
        _resetstkoflw();
        FATAL("stack overflow");
    }
    return result;
}
