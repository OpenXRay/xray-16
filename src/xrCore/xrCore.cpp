// xrCore.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#pragma hdrstop

#if defined(WINDOWS)
#include <mmsystem.h>
#include <objbase.h>
#pragma comment(lib, "winmm.lib")
#endif
#include "xrCore.h"
#include "Threading/ThreadPool.hpp"
#include "Math/MathUtil.hpp"
#include "xrCore/_std_extensions.h"
#include "SDL.h"

#if __has_include(".GitInfo.hpp")
#include ".GitInfo.hpp"
#endif

#ifndef GIT_INFO_CURRENT_BRANCH
#define GIT_INFO_CURRENT_BRANCH unknown
#endif

#ifndef GIT_INFO_CURRENT_COMMIT
#define GIT_INFO_CURRENT_COMMIT unknown
#endif

#include "Compression/compression_ppmd_stream.h"
extern compression::ppmd::stream* trained_model;

XRCORE_API xrCore Core;

static u32 init_counter = 0;

#define DO_EXPAND(VAL) VAL##1
#define EXPAND(VAL) DO_EXPAND(VAL)

#ifdef CI
#if EXPAND(CI) == 1
#undef CI
#endif
#endif

#define HELPER(s) #s
#define TO_STRING(s) HELPER(s)

void PrintBuildInfo()
{
    pcstr name = "Custom";
    pcstr buildId = nullptr;
    pcstr builder = nullptr;
    pcstr commit = TO_STRING(GIT_INFO_CURRENT_COMMIT);
    pcstr branch = TO_STRING(GIT_INFO_CURRENT_BRANCH);

#if defined(CI)
#if defined(APPVEYOR)
    name = "AppVeyor";
    buildId = TO_STRING(APPVEYOR_BUILD_VERSION);
    builder = TO_STRING(APPVEYOR_ACCOUNT_NAME);
#elif defined(TRAVIS)
    name = "Travis";
    buildId = TO_STRING(TRAVIS_BUILD_NUMBER);
#else
#pragma TODO("PrintCI for other CIs")
    name = "CI";
    builder = "Unknown CI";
#endif
#endif

    string512 buf;
    strconcat(sizeof(buf), buf, name, " build "); // "%s build "

    if (buildId)
        strconcat(sizeof(buf), buf, buf, buildId, " "); // "id "

    strconcat(sizeof(buf), buf, buf, "from commit[", commit, "]"); // "from commit[hash]"
    strconcat(sizeof(buf), buf, buf, " branch[", branch, "]"); // " branch[name]"

    if (builder)
        strconcat(sizeof(buf), buf, buf, " (built by ", builder, ")"); // " (built by builder)"

    Log(buf); // "%s build %s from commit[%s] branch[%s] (built by %s)"
}

void SDLLogOutput(void* /*userdata*/,
    int category,
    SDL_LogPriority priority,
    const char* message)
{
    pcstr from;
    switch (category)
    {
    case SDL_LOG_CATEGORY_APPLICATION:
        from = "application";
        break;

    case SDL_LOG_CATEGORY_ERROR:
        from = "error";
        break;

    case SDL_LOG_CATEGORY_ASSERT:
        from = "assert";
        break;

    case SDL_LOG_CATEGORY_SYSTEM:
        from = "system";
        break;

    case SDL_LOG_CATEGORY_AUDIO:
        from = "audio";
        break;

    case SDL_LOG_CATEGORY_VIDEO:
        from = "video";
        break;

    case SDL_LOG_CATEGORY_RENDER:
        from = "render";
        break;

    case SDL_LOG_CATEGORY_INPUT:
        from = "input";
        break;

    case SDL_LOG_CATEGORY_TEST:
        from = "test";
        break;

    case SDL_LOG_CATEGORY_CUSTOM:
        from = "custom";
        break;

    default:
        from = "unknown";
        break;
    }

    char mark;
    pcstr type;
    switch (priority)
    {
    case SDL_LOG_PRIORITY_VERBOSE:
        mark = '%';
        type = "verbose";
        break;

    case SDL_LOG_PRIORITY_DEBUG:
        mark = '#';
        type = "debug";
        break;

    case SDL_LOG_PRIORITY_INFO:
        mark = '=';
        type = "info";
        break;

    case SDL_LOG_PRIORITY_WARN:
        mark = '~';
        type = "warn";
        break;

    case SDL_LOG_PRIORITY_ERROR:
        mark = '!';
        type = "error";
        break;

    case SDL_LOG_PRIORITY_CRITICAL:
        mark = '$';
        type = "critical";
        break;

    default:
        mark = ' ';
        type = "unknown";
        break;
    }

    constexpr pcstr format = "%c [sdl][%s][%s]: %s";
    const size_t size = sizeof(mark) + sizeof(from) + sizeof(type) + sizeof(format) + sizeof(message);
    pstr buf = (pstr)_alloca(size);

    xr_sprintf(buf, size, format, mark, from, type, message);
    Log(buf);
}

void xrCore::Initialize(pcstr _ApplicationName, pcstr commandLine, LogCallback cb, bool init_fs, pcstr fs_fname, bool plugin)
{
    xr_strcpy(ApplicationName, _ApplicationName);
    if (0 == init_counter)
    {
        CalculateBuildId();
        PluginMode = plugin;
        // Init COM so we can use CoCreateInstance
        // HRESULT co_res =
        if (commandLine)
            Params = xr_strdup (commandLine);
        else
            Params = xr_strdup("");

#if defined(WINDOWS)
        if (!strstr(Params, "-weather"))
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

#if defined(WINDOWS)
        string_path fn, dr, di;

        // application path
        GetModuleFileName(GetModuleHandle("xrCore"), fn, sizeof(fn));
        _splitpath(fn, dr, di, nullptr, nullptr);
        strconcat(sizeof(ApplicationPath), ApplicationPath, dr, di);
#else
        SDL_strlcpy(ApplicationPath, SDL_GetBasePath(), sizeof(ApplicationPath));
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

#if defined(WINDOWS)
        GetCurrentDirectory(sizeof(WorkingPath), WorkingPath);
#else
        getcwd(WorkingPath, sizeof(WorkingPath));
#endif

#if defined(WINDOWS)
        // User/Comp Name
        DWORD sz_user = sizeof(UserName);
        GetUserName(UserName, &sz_user);

        DWORD sz_comp = sizeof(CompName);
        GetComputerName(CompName, &sz_comp);
#endif

        Memory._initialize();

        SDL_LogSetOutputFunction(SDLLogOutput, nullptr);
        Msg("%s %s build %d, %s", "OpenXRay", GetBuildConfiguration(), buildId, buildDate);
        PrintBuildInfo();
        Msg("\ncommand line %s\n", Params);
        _initialize_cpu();
        R_ASSERT(SDL_HasSSE());
        ttapi.initialize();
        XRay::Math::Initialize();
        // xrDebug::Initialize ();

        rtc_initialize();

        xr_FS = std::make_unique<CLocatorAPI>();

        xr_EFS = std::make_unique<EFS_Utils>();
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
        flags |= CLocatorAPI::flScanAppRoot;

#ifndef _EDITOR
#ifndef ELocatorAPIH
        if (strstr(Params, "-file_activity") != nullptr)
            flags |= CLocatorAPI::flDumpFileActivity;
#endif
#endif
        FS._initialize(flags, nullptr, fs_fname);
        EFS._initialize();
#ifdef DEBUG
#ifndef _EDITOR
#ifndef LINUX // FIXME!!!
        Msg("Process heap 0x%08x", GetProcessHeap());
#endif
#endif
#endif // DEBUG
    }
    SetLogCB(cb);
    init_counter++;
}

void xrCore::_destroy()
{
    --init_counter;
    if (0 == init_counter)
    {
        ttapi.destroy();
        FS._destroy();
        EFS._destroy();
        xr_FS.reset();
        xr_EFS.reset();

        if (trained_model)
        {
            void* buffer = trained_model->buffer();
            xr_free(buffer);
            xr_delete(trained_model);
        }
        xr_free(Params);
        Memory._destroy();
    }
}

constexpr pcstr xrCore::GetBuildConfiguration()
{
#ifdef NDEBUG
#ifdef XR_X64
    return "Rx64";
#else
    return "Rx86";
#endif
#elif defined(MIXED)
#ifdef XR_X64
    return "Mx64";
#else
    return "Mx86";
#endif
#else
#ifdef XR_X64
    return "Dx64";
#else
    return "Dx86";
#endif
#endif
}

void xrCore::CalculateBuildId()
{
    const int startDay = 31;
    const int startMonth = 1;
    const int startYear = 1999;
    const char* monthId[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const int daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    buildDate = __DATE__;
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

#if defined(WINDOWS)
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
