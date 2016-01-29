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
#include "CopyProtection.h"
#include "Text_Console.h"
#include <process.h>
#include <locale.h>

#include "xrSASH.h"

//---------------------------------------------------------------------
ENGINE_API CInifile* pGameIni = NULL;
BOOL g_bIntroFinished = FALSE;
// computing build id
XRCORE_API LPCSTR build_date;
XRCORE_API u32 build_id;

#ifdef MASTER_GOLD
# define NO_MULTI_INSTANCES
#endif // #ifdef MASTER_GOLD


static LPSTR month_id[12] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int days_in_month[12] =
{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int start_day = 31; // 31
static int start_month = 1; // January
static int start_year = 1999; // 1999

void compute_build_id()
{
    build_date = __DATE__;

    int days;
    int months = 0;
    int years;
    string16 month;
    string256 buffer;
    xr_strcpy(buffer, __DATE__);
    sscanf(buffer, "%s %d %d", month, &days, &years);

    for (int i = 0; i < 12; i++)
    {
        if (_stricmp(month_id[i], month))
            continue;

        months = i;
        break;
    }

    build_id = (years - start_year) * 365 + days - start_day;

    for (int i = 0; i < months; ++i)
        build_id += days_in_month[i];

    for (int i = 0; i < start_month - 1; ++i)
        build_id -= days_in_month[i];
}

// global variables
ENGINE_API CApplication* pApp = NULL;
static HWND logoWindow = NULL;

void doBenchmark(LPCSTR name);
ENGINE_API bool g_bBenchmark = false;
string512 g_sBenchmarkName;


ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
ENGINE_API string_path g_sLaunchWorkingFolder;
// -------------------------------------------
// startup point
void InitEngine()
{
    Engine.Initialize();
    while (!g_bIntroFinished)
        Sleep(100);
    Device.Initialize();
    CheckCopyProtection();
}

static void InitEngineExt()
{
    Engine.External.Initialize();
}

struct path_excluder_predicate
{
    explicit path_excluder_predicate(xr_auth_strings_t const* ignore) :
        m_ignore(ignore)
    {
    }
    bool xr_stdcall is_allow_include(LPCSTR path)
    {
        if (!m_ignore)
            return true;

        return allow_to_include_path(*m_ignore, path);
    }
    xr_auth_strings_t const* m_ignore;
};

void InitSettings()
{
    string_path fname;
    FS.update_path(fname, "$game_config$", "system.ltx");
#ifdef DEBUG
    Msg("Updated path to system.ltx is %s", fname);
#endif // #ifdef DEBUG
    pSettings = xr_new<CInifile>(fname, TRUE);
    CHECK_OR_EXIT(0 != pSettings->section_count(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

    xr_auth_strings_t tmp_ignore_pathes;
    xr_auth_strings_t tmp_check_pathes;
    fill_auth_check_params(tmp_ignore_pathes, tmp_check_pathes);

    path_excluder_predicate tmp_excluder(&tmp_ignore_pathes);
    CInifile::allow_include_func_t tmp_functor;
    tmp_functor.bind(&tmp_excluder, &path_excluder_predicate::is_allow_include);
    pSettingsAuth = xr_new<CInifile>(
                        fname,
                        TRUE,
                        TRUE,
                        FALSE,
                        0,
                        tmp_functor
                    );

    FS.update_path(fname, "$game_config$", "game.ltx");
    pGameIni = xr_new<CInifile>(fname, TRUE);
    CHECK_OR_EXIT(0 != pGameIni->section_count(), make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
}
void InitConsole()
{
#ifdef DEDICATED_SERVER
    {
        Console = xr_new<CTextConsole>();
    }
#else
    {
        Console = xr_new<CConsole>();
    }
#endif
    Console->Initialize();

    xr_strcpy(Console->ConfigFile, "user.ltx");
    if (strstr(Core.Params, "-ltx "))
    {
        string64 c_name;
        sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
        xr_strcpy(Console->ConfigFile, c_name);
    }
}

void InitInput()
{
    BOOL bCaptureInput = !strstr(Core.Params, "-i");

    pInput = xr_new<CInput>(bCaptureInput);
}
void destroyInput()
{
    xr_delete(pInput);
}

void InitSound1()
{
    CSound_manager_interface::_create(0);
}

void InitSound2()
{
    CSound_manager_interface::_create(1);
}

void destroySound()
{
    CSound_manager_interface::_destroy();
}

void destroySettings()
{
    CInifile** s = (CInifile**)(&pSettings);
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
    {
        thread_spawn(slowdownthread, "slowdown", 0, 0);
    }
    if (strstr(Core.Params, "-slowdown2x"))
    {
        thread_spawn(slowdownthread, "slowdown", 0, 0);
        thread_spawn(slowdownthread, "slowdown", 0, 0);
    }
#endif // DEBUG
}

void Startup()
{
    InitSound1();
    execUserScript();
    InitSound2();

    // ...command line for auto start
    {
        LPCSTR pStartup = strstr(Core.Params, "-start ");
        if (pStartup) Console->Execute(pStartup + 1);
    }
    {
        LPCSTR pStartup = strstr(Core.Params, "-load ");
        if (pStartup) Console->Execute(pStartup + 1);
    }

    // Initialize APP
    ShowWindow(Device.m_hWnd, SW_SHOWNORMAL);
    Device.Create();
    LALib.OnCreate();
    pApp = xr_new<CApplication>();
    g_pGamePersistent = dynamic_cast<IGame_Persistent*>(NEW_INSTANCE(CLSID_GAME_PERSISTANT));
    R_ASSERT(g_pGamePersistent);
    g_SpatialSpace = xr_new<ISpatial_DB>("Spatial obj");
    g_SpatialSpacePhysic = xr_new<ISpatial_DB>("Spatial phys");

    // Destroy LOGO
    DestroyWindow(logoWindow);
    logoWindow = NULL;

    // Main cycle
    CheckCopyProtection();
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

static BOOL CALLBACK logDlgProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_DESTROY:
        break;
    case WM_CLOSE:
        DestroyWindow(hw);
        break;
    case WM_COMMAND:
        if (LOWORD(wp) == IDCANCEL)
            DestroyWindow(hw);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

#define dwStickyKeysStructSize sizeof( STICKYKEYS )
#define dwFilterKeysStructSize sizeof( FILTERKEYS )
#define dwToggleKeysStructSize sizeof( TOGGLEKEYS )

struct damn_keys_filter
{
    BOOL bScreenSaverState;

    // Sticky & Filter & Toggle keys

    STICKYKEYS StickyKeysStruct;
    FILTERKEYS FilterKeysStruct;
    TOGGLEKEYS ToggleKeysStruct;

    DWORD dwStickyKeysFlags;
    DWORD dwFilterKeysFlags;
    DWORD dwToggleKeysFlags;

    damn_keys_filter()
    {
        // Screen saver stuff

        bScreenSaverState = FALSE;

        // Saveing current state
        SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, (PVOID)&bScreenSaverState, 0);

        if (bScreenSaverState)
            // Disable screensaver
            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);

        dwStickyKeysFlags = 0;
        dwFilterKeysFlags = 0;
        dwToggleKeysFlags = 0;


        ZeroMemory(&StickyKeysStruct, dwStickyKeysStructSize);
        ZeroMemory(&FilterKeysStruct, dwFilterKeysStructSize);
        ZeroMemory(&ToggleKeysStruct, dwToggleKeysStructSize);

        StickyKeysStruct.cbSize = dwStickyKeysStructSize;
        FilterKeysStruct.cbSize = dwFilterKeysStructSize;
        ToggleKeysStruct.cbSize = dwToggleKeysStructSize;

        // Saving current state
        SystemParametersInfo(SPI_GETSTICKYKEYS, dwStickyKeysStructSize, (PVOID)&StickyKeysStruct, 0);
        SystemParametersInfo(SPI_GETFILTERKEYS, dwFilterKeysStructSize, (PVOID)&FilterKeysStruct, 0);
        SystemParametersInfo(SPI_GETTOGGLEKEYS, dwToggleKeysStructSize, (PVOID)&ToggleKeysStruct, 0);

        if (StickyKeysStruct.dwFlags & SKF_AVAILABLE)
        {
            // Disable StickyKeys feature
            dwStickyKeysFlags = StickyKeysStruct.dwFlags;
            StickyKeysStruct.dwFlags = 0;
            SystemParametersInfo(SPI_SETSTICKYKEYS, dwStickyKeysStructSize, (PVOID)&StickyKeysStruct, 0);
        }

        if (FilterKeysStruct.dwFlags & FKF_AVAILABLE)
        {
            // Disable FilterKeys feature
            dwFilterKeysFlags = FilterKeysStruct.dwFlags;
            FilterKeysStruct.dwFlags = 0;
            SystemParametersInfo(SPI_SETFILTERKEYS, dwFilterKeysStructSize, (PVOID)&FilterKeysStruct, 0);
        }

        if (ToggleKeysStruct.dwFlags & TKF_AVAILABLE)
        {
            // Disable FilterKeys feature
            dwToggleKeysFlags = ToggleKeysStruct.dwFlags;
            ToggleKeysStruct.dwFlags = 0;
            SystemParametersInfo(SPI_SETTOGGLEKEYS, dwToggleKeysStructSize, (PVOID)&ToggleKeysStruct, 0);
        }
    }

    ~damn_keys_filter()
    {
        if (bScreenSaverState)
            // Restoring screen saver
            SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, NULL, 0);

        if (dwStickyKeysFlags)
        {
            // Restore StickyKeys feature
            StickyKeysStruct.dwFlags = dwStickyKeysFlags;
            SystemParametersInfo(SPI_SETSTICKYKEYS, dwStickyKeysStructSize, (PVOID)&StickyKeysStruct, 0);
        }

        if (dwFilterKeysFlags)
        {
            // Restore FilterKeys feature
            FilterKeysStruct.dwFlags = dwFilterKeysFlags;
            SystemParametersInfo(SPI_SETFILTERKEYS, dwFilterKeysStructSize, (PVOID)&FilterKeysStruct, 0);
        }

        if (dwToggleKeysFlags)
        {
            // Restore FilterKeys feature
            ToggleKeysStruct.dwFlags = dwToggleKeysFlags;
            SystemParametersInfo(SPI_SETTOGGLEKEYS, dwToggleKeysStructSize, (PVOID)&ToggleKeysStruct, 0);
        }

    }
};

#undef dwStickyKeysStructSize
#undef dwFilterKeysStructSize
#undef dwToggleKeysStructSize

#include "xr_ioc_cmd.h"

ENGINE_API bool g_dedicated_server = false;

int APIENTRY WinMain_impl(HINSTANCE hInstance,
                          HINSTANCE hPrevInstance,
                          char* lpCmdLine,
                          int nCmdShow)
{
#ifdef DEDICATED_SERVER
    xrDebug::Initialize(true);
#else // DEDICATED_SERVER
    xrDebug::Initialize(false);
#endif // DEDICATED_SERVER

    if (!IsDebuggerPresent())
    {
        ULONG HeapFragValue = 2;
#ifdef DEBUG
        BOOL const result =
#endif // #ifdef DEBUG
            HeapSetInformation(
                GetProcessHeap(),
                HeapCompatibilityInformation,
                &HeapFragValue,
                sizeof(HeapFragValue));
        VERIFY2(result, "can't set process heap low fragmentation");
    }
#ifndef DEDICATED_SERVER
    // Check for another instance
#ifdef NO_MULTI_INSTANCES
#define STALKER_PRESENCE_MUTEX "Local\\STALKER-COP"

    HANDLE hCheckPresenceMutex = INVALID_HANDLE_VALUE;
    hCheckPresenceMutex = OpenMutex(READ_CONTROL, FALSE, STALKER_PRESENCE_MUTEX);
    if (hCheckPresenceMutex == NULL)
    {
        // New mutex
        hCheckPresenceMutex = CreateMutex(NULL, FALSE, STALKER_PRESENCE_MUTEX);
        if (hCheckPresenceMutex == NULL)
            // Shit happens
            return 2;
    }
    else
    {
        // Already running
        CloseHandle(hCheckPresenceMutex);
        return 1;
    }
#endif
#else // DEDICATED_SERVER
    g_dedicated_server = true;
#endif // DEDICATED_SERVER

    SetThreadAffinityMask(GetCurrentThread(), 1);

    // Title window
    logoWindow = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_STARTUP), 0, logDlgProc);

    HWND logoPicture = GetDlgItem(logoWindow, IDC_STATIC_LOGO);
    RECT logoRect;
    GetWindowRect(logoPicture, &logoRect);

    SetWindowPos(
        logoWindow,
#ifndef DEBUG
        HWND_TOPMOST,
#else
        HWND_NOTOPMOST,
#endif // NDEBUG
        0,
        0,
        logoRect.right - logoRect.left,
        logoRect.bottom - logoRect.top,
        SWP_NOMOVE | SWP_SHOWWINDOW
    );
    UpdateWindow(logoWindow);

    // AVI
    g_bIntroFinished = TRUE;

    g_sLaunchOnExit_app[0] = NULL;
    g_sLaunchOnExit_params[0] = NULL;

    LPCSTR fsgame_ltx_name = "-fsltx ";
    string_path fsgame = "";
    if (strstr(lpCmdLine, fsgame_ltx_name))
    {
        int sz = xr_strlen(fsgame_ltx_name);
        sscanf(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
    }
    compute_build_id();
    Core._initialize("xray", NULL, TRUE, fsgame[0] ? fsgame : NULL);

    InitSettings();

    // Adjust player & computer name for Asian
    if (pSettings->line_exist("string_table", "no_native_input"))
    {
        xr_strcpy(Core.UserName, sizeof(Core.UserName), "Player");
        xr_strcpy(Core.CompName, sizeof(Core.CompName), "Computer");
    }

#ifndef DEDICATED_SERVER
    {
        damn_keys_filter filter;
        (void)filter;
#endif // DEDICATED_SERVER

        FPU::m24r();
        InitEngine();
        InitInput();
        InitConsole();
        Engine.External.CreateRendererList();
        Msg("command line %s", lpCmdLine);
        
        LPCSTR benchName = "-batch_benchmark ";
        if (strstr(lpCmdLine, benchName))
        {
            int sz = xr_strlen(benchName);
            string64 b_name;
            sscanf(strstr(Core.Params, benchName) + sz, "%[^ ] ", b_name);
            doBenchmark(b_name);
            return 0;
        }
        LPCSTR sashName = "-openautomate ";
        if (strstr(lpCmdLine, sashName))
        {
            int sz = xr_strlen(sashName);
            string512 sash_arg;
            sscanf(strstr(Core.Params, sashName) + sz, "%[^ ] ", sash_arg);
            g_SASH.Init(sash_arg);
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
            CCC_LoadCFG_custom* pTmp = xr_new<CCC_LoadCFG_custom>("renderer ");
            pTmp->Execute(Console->ConfigFile);
            xr_delete(pTmp);
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
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            //We use CreateProcess to setup working folder
            char const* temp_wf = (xr_strlen(g_sLaunchWorkingFolder) > 0) ? g_sLaunchWorkingFolder : NULL;
            CreateProcess(g_sLaunchOnExit_app, g_sLaunchOnExit_params, NULL, NULL, FALSE, 0, NULL,
                          temp_wf, &si, &pi);

        }
#ifndef DEDICATED_SERVER
#ifdef NO_MULTI_INSTANCES
        // Delete application presence mutex
        CloseHandle(hCheckPresenceMutex);
#endif
    }
    // here damn_keys_filter class instanse will be destroyed
#endif // DEDICATED_SERVER

    return 0;
}

int stack_overflow_exception_filter(int exception_code)
{
    if (exception_code == EXCEPTION_STACK_OVERFLOW)
    {
        // Do not call _resetstkoflw here, because
        // at this point, the stack is not yet unwound.
        // Instead, signal that the handler (the __except block)
        // is to be executed.
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
        return EXCEPTION_CONTINUE_SEARCH;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     char* lpCmdLine,
                     int nCmdShow)
{
    __try
    {
        WinMain_impl(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    }
    __except (stack_overflow_exception_filter(GetExceptionCode()))
    {
        _resetstkoflw();
        FATAL("stack overflow");
    }

    return (0);
}

extern CRenderDevice Device;

static CTimer phase_timer;

void doBenchmark(LPCSTR name)
{
    g_bBenchmark = true;
    string_path in_file;
    FS.update_path(in_file, "$app_data_root$", name);
    CInifile ini(in_file);
    int test_count = ini.line_count("benchmark");
    LPCSTR test_name, t;
    shared_str test_command;
    for (int i = 0; i < test_count; ++i)
    {
        ini.r_line("benchmark", i, &test_name, &t);
        xr_strcpy(g_sBenchmarkName, test_name);

        test_command = ini.r_string_wb("benchmark", test_name);
        u32 cmdSize = test_command.size() + 1;
        Core.Params = (char*)xr_realloc(Core.Params, cmdSize);
        xr_strcpy(Core.Params, cmdSize, test_command.c_str());
        xr_strlwr(Core.Params);

        InitInput();
        if (i)
            InitEngine();
        Engine.External.Initialize();

        xr_strcpy(Console->ConfigFile, "user.ltx");
        if (strstr(Core.Params, "-ltx "))
        {
            string64 c_name;
            sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
            xr_strcpy(Console->ConfigFile, c_name);
        }

        Startup();
    }
}
