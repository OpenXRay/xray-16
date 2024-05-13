// xrCore.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#pragma hdrstop

#if defined(XR_PLATFORM_WINDOWS)
#include <mmsystem.h>
#include <objbase.h>
#pragma comment(lib, "winmm.lib")
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#endif
#include "xrCore.h"
#include "xrCore/_std_extensions.h"
#include "Threading/TaskManager.hpp"

#include <SDL3/SDL.h>

#if __has_include(".GitInfo.hpp")
#include ".GitInfo.hpp"
#endif

#include "Compression/compression_ppmd_stream.h"
extern compression::ppmd::stream* trained_model;

XRCORE_API xrCore Core;

static u32 init_counter = 0;

#define DO_EXPAND(...) __VA_ARGS__##1
#define EXPAND(VAL) DO_EXPAND(VAL)

#ifdef CI
#if EXPAND(CI) == 1
#undef CI
#endif
#endif

#ifdef APPVEYOR
#if EXPAND(APPVEYOR) == 1
#undef APPVEYOR
#endif
#endif

#ifdef GITHUB_ACTIONS
#if EXPAND(GITHUB_ACTIONS) == 1
#undef GITHUB_ACTIONS
#endif
#endif

#ifndef GIT_INFO_CURRENT_COMMIT
#define GIT_INFO_CURRENT_COMMIT unknown
#endif

#ifndef GIT_INFO_CURRENT_BRANCH
#define GIT_INFO_CURRENT_BRANCH unknown
#endif

void SDLLogOutput(void* userdata, int category, SDL_LogPriority priority, const char* message);

const pcstr xrCore::buildDate = __DATE__;
const pcstr xrCore::buildCommit = GIT_INFO_CURRENT_COMMIT;
const pcstr xrCore::buildBranch = GIT_INFO_CURRENT_BRANCH;

void SanitizeString(pcstr str)
{
    pstr mut_str = const_cast<pstr>(str);

    while (*mut_str != '\0')
    {
        switch (*mut_str)
        {
        case '\\':
        case '/':
        case ',':
        case '.':
            *mut_str = '_';
            [[fallthrough]];
        default:
            ++mut_str;
        }
    }
}

xrCore::xrCore()
    : ApplicationName{}, ApplicationPath{},
      WorkingPath{},
      UserName{}, CompName{},
      Params(nullptr), dwFrame(0),
      PluginMode(false)
{
    CalculateBuildId();
}

void xrCore::CalculateBuildId()
{
    const int startDay = 31;
    const int startMonth = 1;
    const int startYear = 1999;
    const char* monthId[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    const int daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int days;
    int months = 0;
    int years;
    string16 month;
    string256 buffer;
    xr_strcpy(buffer, buildDate);
    sscanf(buffer, "%s %d %d", month, &days, &years);
    for (int i = 0; i < 12; i++)
    {
        if (xr_stricmp(monthId[i], month))
            continue;
        months = i;
        break;
    }
    buildId = (years - startYear) * 365 + days - startDay;
    for (int i = 0; i < months; i++)
        buildId += daysInMonth[i];
    for (int i = 0; i < startMonth - 1; i++)
        buildId -= daysInMonth[i];
}

void xrCore::PrintBuildInfo()
{
    Msg("%s %s build %d, %s (%s)", ApplicationName, XRAY_BUILD_CONFIGURATION, buildId, buildDate, XRAY_BUILD_CONFIGURATION2);

    pcstr name      = "Custom";
    pcstr buildUniqueId = nullptr;
    pcstr buildId   = nullptr;
    pcstr builder   = nullptr;
    pcstr commit    = GetBuildCommit();
    pcstr branch    = GetBuildBranch();

#if defined(CI)
#   if defined(APPVEYOR)
    name            = "AppVeyor";
    buildUniqueId   = APPVEYOR_BUILD_ID;
    buildId         = APPVEYOR_BUILD_VERSION;
    builder         = APPVEYOR_ACCOUNT_NAME;
#   elif defined(GITHUB_ACTIONS)
    name            = "GitHub Actions";
    buildUniqueId   = GITHUB_RUN_ID;
    buildId         = GITHUB_RUN_NUMBER;
    builder         = GITHUB_REPOSITORY;
#else
#   pragma TODO("PrintBuildInfo for other CIs")
    name            = "CI";
    builder         = "Unknown CI";
#   endif
#endif

    string512 buf;
    strconcat(buf, name, " build "); // "%s build "

    if (buildId)
    {
        strconcat(buf, buf, buildId, " "); // "id "
        if (buildUniqueId)
            strconcat(buf, buf, "(", buildUniqueId, ") "); // "(unique id) "
    }

    strconcat(buf, buf, "from commit[", commit, "]"); // "from commit[hash]"
    strconcat(buf, buf, " branch[", branch, "]"); // " branch[name]"

    if (builder)
        strconcat(buf, buf, " (built by ", builder, ")"); // " (built by builder)"

    Log(buf); // "%s build %s from commit[%s] branch[%s] (built by %s)"
}

void xrCore::Initialize(pcstr _ApplicationName, pcstr commandLine, bool init_fs, pcstr fs_fname, bool plugin)
{
    ZoneScoped;
    xr_strcpy(ApplicationName, _ApplicationName);
    PrintBuildInfo();

    if (0 == init_counter)
    {
#if defined(XR_ARCHITECTURE_X86) || defined(XR_ARCHITECTURE_X64)
        R_ASSERT2(CPU::HasSSE2, "Your CPU must support SSE2.");
#endif

        PluginMode = plugin;
        if (commandLine)
            Params = xr_strdup(commandLine);
        else
            Params = xr_strdup("");

#if defined(XR_PLATFORM_WINDOWS)
        CoInitializeEx(nullptr, COINIT_MULTITHREADED); // needed for OpenAL initialization

        string_path fn, dr, di;

        // application path
        GetModuleFileName(GetModuleHandle("xrCore"), fn, sizeof(fn));
        _splitpath(fn, dr, di, nullptr, nullptr);
        strconcat(sizeof(ApplicationPath), ApplicationPath, dr, di);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
        char* base_path = SDL_GetBasePath();
        if (!base_path)
        {
            if (strstr(Core.Params, "-shoc") || strstr(Core.Params, "-soc"))
                base_path = SDL_GetPrefPath("GSC Game World", "S.T.A.L.K.E.R. - Shadow of Chernobyl");
            else if (strstr(Core.Params, "-cs"))
                base_path = SDL_GetPrefPath("GSC Game World", "S.T.A.L.K.E.R. - Clear Sky");
            else
                base_path = SDL_GetPrefPath("GSC Game World", "S.T.A.L.K.E.R. - Call of Pripyat");
        }
        SDL_strlcpy(ApplicationPath, base_path, sizeof(ApplicationPath));
        SDL_free(base_path);
#else
#   error Select or add implementation for your platform
#endif

#ifdef _EDITOR
        // working path
        if (strstr(Params, "-wf"))
        {
            string_path c_name;
            sscanf(strstr(Core.Params, "-wf ") + 4, "%[^ ] ", c_name);
            SetCurrentDirectory(c_name);
        }
#endif

#if defined(XR_PLATFORM_WINDOWS)
        GetCurrentDirectory(sizeof(WorkingPath), WorkingPath);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
        getcwd(WorkingPath, sizeof(WorkingPath));
#else
#   error Select or add implementation for your platform
#endif

#if defined(XR_PLATFORM_WINDOWS)
        // User/Comp Name
        DWORD sz_user = sizeof(UserName);
        GetUserName(UserName, &sz_user);

        DWORD sz_comp = sizeof(CompName);
        GetComputerName(CompName, &sz_comp);
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
        uid_t uid = geteuid();
        struct passwd *pw = getpwuid(uid);
        if (pw)
        {
            strncpy(UserName, pw->pw_gecos, sizeof(UserName) - 1);
            if (UserName[0] == '\0')
                strncpy(UserName, pw->pw_name, sizeof(UserName) - 1);
        }
        else
            Msg("! Failed to get user name");

        if (gethostname(CompName, sizeof(CompName)) == 0)
            CompName[sizeof(CompName) - 1] = '\0';
        else
            Msg("! Failed to get computer name");
#else
#   error Select or add implementation for your platform
#endif

        SanitizeString(UserName);
        SanitizeString(CompName);

#ifdef DEBUG
        Msg("UserName: %s", UserName);
        Msg("ComputerName: %s", CompName);
#endif

        Memory._initialize();

        SDL_SetLogOutputFunction(SDLLogOutput, nullptr);
        Msg("\ncommand line %s\n", Params);
        _initialize_cpu();
        TaskScheduler = xr_make_unique<TaskManager>();
        TaskScheduler->SpawnThreads();
        // xrDebug::Initialize ();

        rtc_initialize();

        xr_FS = xr_make_unique<CLocatorAPI>();

        xr_EFS = xr_make_unique<EFS_Utils>();
        //. R_ASSERT (co_res==S_OK);
    }
    if (init_fs)
    {
        u32 flags = 0u;
        if (strstr(Params, "-build") != nullptr)
            flags |= CLocatorAPI::flBuildCopy;
        if (strstr(Params, "-ebuild") != nullptr)
            flags |= CLocatorAPI::flBuildCopy | CLocatorAPI::flEBuildCopy;
#ifdef DEBUG
        if (strstr(Params, "-cache"))
            flags |= CLocatorAPI::flCacheFiles;
        else
            flags &= ~CLocatorAPI::flCacheFiles;
#endif // DEBUG
#ifdef _EDITOR // for EDITORS - no cache
        flags &= ~CLocatorAPI::flCacheFiles;
#endif // _EDITOR

// TODO Add proper check for CMake Windows build
#if !defined(XR_PLATFORM_WINDOWS)
        if (xr_stricmp(ApplicationPath, CMAKE_INSTALL_FULL_DATAROOTDIR) != 0)
            flags |= CLocatorAPI::flScanAppRoot;
#endif

#ifndef _EDITOR
#ifndef ELocatorAPIH
        if (strstr(Params, "-file_activity") != nullptr)
            flags |= CLocatorAPI::flDumpFileActivity;
#endif
#endif
        FS._initialize(flags, nullptr, fs_fname);
        EFS._initialize();
    }
    init_counter++;
}

void xrCore::_destroy()
{
    --init_counter;
    if (0 == init_counter)
    {
        ZoneScoped;
        FS._destroy();
        EFS._destroy();
        xr_FS = nullptr;
        xr_EFS = nullptr;

        if (trained_model)
        {
            void* buffer = trained_model->buffer();
            xr_free(buffer);
            xr_delete(trained_model);
        }
        TaskScheduler = nullptr;
        xr_free(Params);
        Memory._destroy();
#ifdef XR_PLATFORM_WINDOWS
        CoUninitialize();
#endif
    }
}

#if defined(XR_PLATFORM_WINDOWS)
#ifdef _EDITOR
BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
#else
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved)
#endif
{
    switch (ul_reason_for_call)
    {
    /*
    По сути это не рекомендуемый Microsoft, но повсеместно используемый способ повышения точности
    соблюдения и измерения временных интревалов функциями Sleep, QueryPerformanceCounter,
    timeGetTime и GetTickCount.
    Функция действует на всю операционную систему в целом (!) и нет необходимости вызывать её при
    старте нового потока. Вызов timeEndPeriod специалисты Microsoft считают обязательным.
    Есть подозрения, что Windows сама устанавливает максимальную точность при старте таких
    приложений как, например, игры. Тогда есть шанс, что вызов timeBeginPeriod здесь бессмысленен.
    Недостатком данного способа является то, что он приводит к общему замедлению работы как
    текущего приложения, так и всей операционной системы.
    Ещё можно посмотреть ссылки:
        https://msdn.microsoft.com/en-us/library/vs/alm/dd757624(v=vs.85).aspx
        https://users.livejournal.com/-winnie/151099.html
        https://github.com/tebjan/TimerTool
    */
    case DLL_PROCESS_ATTACH: timeBeginPeriod(1); break;
    case DLL_PROCESS_DETACH: timeEndPeriod  (1); break;
    }
    return TRUE;
}
#endif

void SDLLogOutput(void* /*userdata*/, int category, SDL_LogPriority priority, const char* message)
{
    pcstr from;
    switch (category)
    {
    case SDL_LOG_CATEGORY_APPLICATION:  from = "application"; break;
    case SDL_LOG_CATEGORY_ERROR:        from = "error"; break;
    case SDL_LOG_CATEGORY_ASSERT:       from = "assert"; break;
    case SDL_LOG_CATEGORY_SYSTEM:       from = "system"; break;
    case SDL_LOG_CATEGORY_AUDIO:        from = "audio"; break;
    case SDL_LOG_CATEGORY_VIDEO:        from = "video"; break;
    case SDL_LOG_CATEGORY_RENDER:       from = "render"; break;
    case SDL_LOG_CATEGORY_INPUT:        from = "input"; break;
    case SDL_LOG_CATEGORY_TEST:         from = "test"; break;
    case SDL_LOG_CATEGORY_CUSTOM:       from = "custom"; break;
    default:                            from = "unknown"; break;
    }

    char mark;
    pcstr type;
    switch (priority)
    {
    case SDL_LOG_PRIORITY_VERBOSE:      mark = '%'; type = "verbose"; break;
    case SDL_LOG_PRIORITY_DEBUG:        mark = '#'; type = "debug"; break;
    case SDL_LOG_PRIORITY_INFO:         mark = '='; type = "info"; break;
    case SDL_LOG_PRIORITY_WARN:         mark = '~'; type = "warn"; break;
    case SDL_LOG_PRIORITY_ERROR:        mark = '!'; type = "error"; break;
    case SDL_LOG_PRIORITY_CRITICAL:     mark = '$'; type = "critical"; break;
    default:                            mark = ' '; type = "unknown"; break;
    }

    static constexpr pcstr format = "%c [sdl][%s][%s]: %s";
    const size_t size = sizeof(mark) + sizeof(from) + sizeof(type) + sizeof(format) + sizeof(message);
    pstr buf = (pstr)xr_alloca(size);

    xr_sprintf(buf, size, format, mark, from, type, message);
    Log(buf);
}
