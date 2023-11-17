#pragma once
#include "xrCore/xr_types.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"
#include "Threading/Lock.hpp"

#include <string>
#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
#include <cstdio>
#elif defined(XR_PLATFORM_WINDOWS)
#pragma warning(push)
#pragma warning(disable : 4091) /// 'typedef ': ignored on left of '' when no variable is declared
#include <DbgHelp.h>
#pragma warning(pop)
#endif

struct SDL_Window;

enum class AssertionResult : int
{
    undefined = -1,
    ignore,
    tryAgain,
    abort,
    ok
};

class ErrorLocation
{
public:
    const char* File = nullptr;
    int Line = -1;
    const char* Function = nullptr;

    ErrorLocation(const char* file, int line, const char* function)
    {
        File = file;
        Line = line;
        Function = function;
    }

    ErrorLocation(const ErrorLocation& rhs) : ErrorLocation(rhs.File, rhs.Line, rhs.Function) { }

    ErrorLocation& operator=(const ErrorLocation& rhs)
    {
        File = rhs.File;
        Line = rhs.Line;
        Function = rhs.Function;
        return *this;
    }
};

class XR_NOVTABLE IWindowHandler
{
public:
    virtual ~IWindowHandler() = 0;
    virtual void* GetApplicationWindowHandle() const = 0;
    virtual SDL_Window* GetApplicationWindow() = 0;
    virtual void OnErrorDialog(bool beforeDialog) = 0;
    virtual void OnFatalError() = 0;
};

inline IWindowHandler::~IWindowHandler() = default;

class XR_NOVTABLE IUserConfigHandler
{
public:
    virtual ~IUserConfigHandler() = 0;
    virtual pcstr GetUserConfigFileName() = 0;
};

inline IUserConfigHandler::~IUserConfigHandler() = default;

class XRCORE_API xrDebug
{
public:
    using OutOfMemoryCallbackFunc = void(*)();
    using UnhandledExceptionFilter = LONG(WINAPI*)(EXCEPTION_POINTERS* exPtrs);

private:
    static IWindowHandler* windowHandler;
    static IUserConfigHandler* userConfigHandler;
    static UnhandledExceptionFilter PrevFilter;
    static OutOfMemoryCallbackFunc OutOfMemoryCallback;
    static string_path BugReportFile;
    static bool ErrorAfterDialog;
    static bool ShowErrorMessage;

public:
    xrDebug() = delete;
    static void Initialize(pcstr commandLine);
    static void Destroy();
    static void OnThreadSpawn();
    static void OnFilesystemInitialized();

    static bool DebuggerIsPresent();
    static bool ProcessingFailure() { return failLock.IsLocked(); }

    static IWindowHandler* GetWindowHandler() { return windowHandler; }
    static void SetWindowHandler(IWindowHandler* handler) { windowHandler = handler; }
    static IUserConfigHandler* GetUserConfigHandler() { return userConfigHandler; }
    static void SetUserConfigHandler(IUserConfigHandler* handler) { userConfigHandler = handler; }
    static OutOfMemoryCallbackFunc GetOutOfMemoryCallback() { return OutOfMemoryCallback; }
    static void SetOutOfMemoryCallback(OutOfMemoryCallbackFunc cb) { OutOfMemoryCallback = cb; }
    static const char* ErrorToString(long code);
    static void SetBugReportFile(const char* fileName);
    static void GatherInfo(char* assertionInfo, size_t bufferSize, const ErrorLocation& loc, const char* expr,
                           const char* desc, const char* arg1 = nullptr, const char* arg2 = nullptr);
    static void Fatal(const ErrorLocation& loc, const char* format, ...);
    static AssertionResult Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, long hresult,
                     const char* arg1 = nullptr, const char* arg2 = nullptr);
    static AssertionResult Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr,
                     const char* desc = "assertion failed", const char* arg1 = nullptr, const char* arg2 = nullptr);
    static AssertionResult Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, const std::string& desc,
                     const char* arg1 = nullptr, const char* arg2 = nullptr);
    [[noreturn]]
    static void DoExit(const std::string& message);

    static AssertionResult ShowMessage(pcstr title, pcstr message, bool simpleMode = true);

    static void LogStackTrace(const char* header);
    static xr_vector<xr_string> BuildStackTrace(u16 maxFramesCount = 512);

private:
    static bool symEngineInitialized;
    static Lock dbgHelpLock;
    static Lock failLock;

    static void FormatLastError(char* buffer, const size_t& bufferSize);
    static void SetupExceptionHandler();
    static LONG WINAPI UnhandledFilter(EXCEPTION_POINTERS* exPtrs);
    static void WINAPI PreErrorHandler(INT_PTR);
#if defined(XR_PLATFORM_WINDOWS)
    static xr_vector<xr_string> BuildStackTrace(PCONTEXT threadCtx, u16 maxFramesCount);
    static bool GetNextStackFrameString(LPSTACKFRAME stackFrame, PCONTEXT threadCtx, xr_string& frameStr);
    static bool InitializeSymbolEngine();
    static void DeinitializeSymbolEngine(void);
#endif //XR_PLATFORM_WINDOWS
};

// forward declaration
// Definition is in xrCore/_std_extensions.h
inline int __cdecl xr_sprintf(pstr destination, size_t const buffer_size, LPCSTR format_string, ...);

// for debug purposes only
template<typename... Args>
std::string make_string(cpcstr format, Args... args)
{
    string4096 log;
    xr_sprintf(log, std::size(log), format, std::forward<Args>(args)...);
    return log;
}

#include "xrDebug_macros.h"
