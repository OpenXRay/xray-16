#include "stdafx.h"
#pragma hdrstop

#include "xrDebug.h"
#include "os_clipboard.h"
#include "log.h"
#include "Threading/ScopeLock.hpp"

#include <SDL3/SDL.h>

#include <csignal>

#if defined(XR_PLATFORM_WINDOWS)
#   include <direct.h>
#   include <new.h> // for _set_new_mode
#   include <errorrep.h> // ReportFault

#   if defined(XR_ARCHITECTURE_X86)
#       define MACHINE_TYPE IMAGE_FILE_MACHINE_I386
#   elif defined(XR_ARCHITECTURE_X64)
#       define MACHINE_TYPE IMAGE_FILE_MACHINE_AMD64
#   elif defined(XR_ARCHITECTURE_ARM)
#       define MACHINE_TYPE IMAGE_FILE_MACHINE_ARM
#   elif defined(XR_ARCHITECTURE_ARM64)
#       define MACHINE_TYPE IMAGE_FILE_MACHINE_ARM64
#   elif defined(XR_ARCHITECTURE_IA64)
#       define MACHINE_TYPE IMAGE_FILE_MACHINE_IA64
#   else
#       error CPU architecture is not supported.
#   endif

#   define USE_BUG_TRAP
#   ifdef USE_BUG_TRAP
#       include <BugTrap/source/Client/BugTrap.h>
#   endif

#   include "Debug/dxerr.h"
#   include "Debug/MiniDump.h"
#endif

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_APPLE) || defined(XR_PLATFORM_BSD)
#   if __has_include(<execinfo.h>)
#       include <execinfo.h>
#       define BACKTRACE_AVAILABLE

#       if __has_include(<cxxabi.h>)
#           include <cxxabi.h>
#           include <dlfcn.h>
#           define CXXABI_AVAILABLE
#       endif
#   endif

#   if __has_include(<sys/ptrace.h>)
#       include <sys/ptrace.h>
#       define PTRACE_AVAILABLE

#       if defined(XR_PLATFORM_APPLE) || defined(XR_PLATFORM_BSD)
#           define PTRACE_TRACEME PT_TRACE_ME
#           define PTRACE_DETACH PT_DETACH
#       endif
#   endif
#endif

#ifdef DEBUG
#   define USE_OWN_ERROR_MESSAGE_WINDOW
#endif

constexpr SDL_MessageBoxButtonData buttons[] =
{
    /* .flags, .buttonid, .text */
    { 0, (int)AssertionResult::ignore, "Continue"  },
    { 0, (int)AssertionResult::tryAgain, "Try again" },

    { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT |
      SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
         (int)AssertionResult::abort, "Cancel" }
};

AssertionResult xrDebug::ShowMessage(pcstr title, pcstr message, bool simpleMode)
{
#ifdef XR_PLATFORM_WINDOWS // because Windows default Message box is fancy
    HWND hwnd = nullptr;
    if (windowHandler)
        hwnd = static_cast<HWND>(windowHandler->GetApplicationWindowHandle());

    if (simpleMode)
    {
        MessageBox(hwnd, message, title, MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        return AssertionResult::ok;
    }

    const int result = MessageBox(hwnd, message, title,
        MB_CANCELTRYCONTINUE | MB_ICONERROR | MB_SYSTEMMODAL);

    switch (result)
    {
    case IDCANCEL: return AssertionResult::abort;
    case IDTRYAGAIN: return AssertionResult::tryAgain;
    case IDCONTINUE: return AssertionResult::ignore;
    default: return AssertionResult::undefined;
    }
#else
    if (simpleMode)
    {
        SDL_Window* parent = windowHandler ? windowHandler->GetApplicationWindow() : nullptr;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, parent);
        return AssertionResult::ok;
    }

    SDL_MessageBoxData data =
    {
        SDL_MESSAGEBOX_ERROR,
        windowHandler ? windowHandler->GetApplicationWindow() : nullptr,
        title, message, SDL_arraysize(buttons), buttons
    };

    int button = -1;
    SDL_ShowMessageBox(&data, &button);
    return (AssertionResult)button;
#endif
}

SDL_AssertState SDLAssertionHandler(const SDL_AssertData* data,
    void* /*userdata*/)
{
    if (data->always_ignore)
        return SDL_ASSERTION_ALWAYS_IGNORE;

    static constexpr pcstr desc = "SDL2 assertion triggered";
    bool alwaysIgnore = false;

    const auto result = xrDebug::Fail(alwaysIgnore,
        { data->filename, data->linenum, data->function },
        data->condition, desc);

    switch (result)
    {
    case AssertionResult::ignore:
        return SDL_ASSERTION_ALWAYS_IGNORE;

    case AssertionResult::tryAgain:
        return SDL_ASSERTION_RETRY;

    case AssertionResult::abort:
        return SDL_ASSERTION_ABORT;

    case AssertionResult::undefined:
    case AssertionResult::ok:
    default:
        return SDL_ASSERTION_IGNORE;
    }
}

IWindowHandler* xrDebug::windowHandler = nullptr;
IUserConfigHandler* xrDebug::userConfigHandler = nullptr;
xrDebug::UnhandledExceptionFilter xrDebug::PrevFilter = nullptr;
xrDebug::OutOfMemoryCallbackFunc xrDebug::OutOfMemoryCallback = nullptr;
string_path xrDebug::BugReportFile;
bool xrDebug::ErrorAfterDialog = false;
bool xrDebug::ShowErrorMessage = false;

bool xrDebug::symEngineInitialized = false;
Lock xrDebug::dbgHelpLock;
#ifdef PROFILE_CRITICAL_SECTIONS
Lock xrDebug::failLock(MUTEX_PROFILE_ID(xrDebug::Backend));
#else
Lock xrDebug::failLock;
#endif

void xrDebug::SetBugReportFile(const char* fileName) { xr_strcpy(BugReportFile, fileName); }

#if defined(XR_PLATFORM_WINDOWS)
bool xrDebug::GetNextStackFrameString(LPSTACKFRAME stackFrame, PCONTEXT threadCtx, xr_string& frameStr)
{
    BOOL result = StackWalk(MACHINE_TYPE, GetCurrentProcess(), GetCurrentThread(), stackFrame, threadCtx, nullptr,
                            SymFunctionTableAccess, SymGetModuleBase, nullptr);

    if (result == FALSE || stackFrame->AddrPC.Offset == 0)
    {
        return false;
    }

    frameStr.clear();
    string512 formatBuff;

    ///
    /// Module name
    ///
    HINSTANCE hModule = (HINSTANCE)SymGetModuleBase(GetCurrentProcess(), stackFrame->AddrPC.Offset);
    if (hModule && GetModuleFileName(hModule, formatBuff, _countof(formatBuff)))
    {
        frameStr.append(formatBuff);
    }

    ///
    /// Address
    ///
    xr_sprintf(formatBuff, _countof(formatBuff), " at %p", stackFrame->AddrPC.Offset);
    frameStr.append(formatBuff);

    ///
    /// Function info
    ///
    u8 arrSymBuffer[512];
    ZeroMemory(arrSymBuffer, sizeof(arrSymBuffer));
    PIMAGEHLP_SYMBOL functionInfo = reinterpret_cast<PIMAGEHLP_SYMBOL>(arrSymBuffer);
    functionInfo->SizeOfStruct = sizeof(*functionInfo);
    functionInfo->MaxNameLength = sizeof(arrSymBuffer) - sizeof(*functionInfo) + 1;
    DWORD_PTR dwFunctionOffset;

    result = SymGetSymFromAddr(GetCurrentProcess(), stackFrame->AddrPC.Offset, &dwFunctionOffset, functionInfo);

    if (result)
    {
        if (dwFunctionOffset)
        {
            xr_sprintf(formatBuff, _countof(formatBuff), " %s() + %Iu byte(s)", functionInfo->Name, dwFunctionOffset);
        }
        else
        {
            xr_sprintf(formatBuff, _countof(formatBuff), " %s()", functionInfo->Name);
        }
        frameStr.append(formatBuff);
    }

    ///
    /// Source info
    ///
    DWORD dwLineOffset;
    IMAGEHLP_LINE sourceInfo = {};
    sourceInfo.SizeOfStruct = sizeof(sourceInfo);

    result = SymGetLineFromAddr(GetCurrentProcess(), stackFrame->AddrPC.Offset, &dwLineOffset, &sourceInfo);

    if (result)
    {
        if (dwLineOffset)
        {
            xr_sprintf(formatBuff, _countof(formatBuff), " in %s line %u + %u byte(s)", sourceInfo.FileName,
                       sourceInfo.LineNumber, dwLineOffset);
        }
        else
        {
            xr_sprintf(formatBuff, _countof(formatBuff), " in %s line %u", sourceInfo.FileName, sourceInfo.LineNumber);
        }
        frameStr.append(formatBuff);
    }

    return true;
}

bool xrDebug::InitializeSymbolEngine()
{
    if (!symEngineInitialized)
    {
        u32 dwOptions = SymGetOptions();
        SymSetOptions(dwOptions | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

        if (SymInitialize(GetCurrentProcess(), nullptr, TRUE))
        {
            symEngineInitialized = true;
        }
    }

    return symEngineInitialized;
}

void xrDebug::DeinitializeSymbolEngine(void)
{
    if (symEngineInitialized)
    {
        SymCleanup(GetCurrentProcess());

        symEngineInitialized = false;
    }
}

xr_vector<xr_string> xrDebug::BuildStackTrace(PCONTEXT threadCtx, u16 maxFramesCount)
{
    ScopeLock Lock(&dbgHelpLock);

    xr_vector<xr_string> traceResult;
    xr_string frameStr;

    if (!InitializeSymbolEngine())
    {
        Msg("[xrDebug::BuildStackTrace]InitializeSymbolEngine failed with error: %d", GetLastError());
        return traceResult;
    }

    traceResult.reserve(maxFramesCount);

    STACKFRAME stackFrame{};
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrBStore.Mode = AddrModeFlat;

    // https://learn.microsoft.com/en-us/windows/win32/api/dbghelp/ns-dbghelp-stackframe
    // https://github.com/reactos/reactos/blob/master/base/applications/drwtsn32/stacktrace.cpp
#if defined XR_ARCHITECTURE_X86
    stackFrame.AddrPC.Offset = threadCtx->Eip;
    stackFrame.AddrStack.Offset = threadCtx->Esp;
    stackFrame.AddrFrame.Offset = threadCtx->Ebp;
#elif defined XR_ARCHITECTURE_X64
    stackFrame.AddrPC.Offset = threadCtx->Rip;
    stackFrame.AddrStack.Offset = threadCtx->Rsp;
    stackFrame.AddrFrame.Offset = threadCtx->Rbp;
#elif defined XR_ARCHITECTURE_ARM
    stackFrame.AddrPC.Offset = threadCtx->Pc;
    stackFrame.AddrStack.Offset = threadCtx->Sp;
    stackFrame.AddrFrame.Offset = threadCtx->R11;
#elif defined XR_ARCHITECTURE_ARM64
    stackFrame.AddrPC.Offset = threadCtx->Pc;
    stackFrame.AddrStack.Offset = threadCtx->Sp;
    stackFrame.AddrFrame.Offset = threadCtx->Fp;
#elif defined XR_ARCHITECTURE_IA64
    stackFrame.AddrPC.Offset = threadCtx->StIIP;
    stackFrame.AddrStack.Offset = threadCtx->IntSp;
    stackFrame.AddrBStore.Offset = threadCtx->RsBSP;
#else
#   error CPU architecture is not supported.
#endif

    while (GetNextStackFrameString(&stackFrame, threadCtx, frameStr) && traceResult.size() <= maxFramesCount)
    {
        traceResult.push_back(frameStr);
    }

    DeinitializeSymbolEngine();

    return traceResult;
}
#endif // defined(XR_PLATFORM_WINDOWS)

xr_vector<xr_string> xrDebug::BuildStackTrace(u16 maxFramesCount)
{
#if defined(XR_PLATFORM_WINDOWS)
    CONTEXT currentThreadCtx = {};

    RtlCaptureContext(&currentThreadCtx); /// GetThreadContext can't be used on the current thread
    currentThreadCtx.ContextFlags = CONTEXT_FULL;

    return BuildStackTrace(&currentThreadCtx, maxFramesCount);
#else
#pragma todo("Implement stack trace for Linux")
    return {"Implement stack trace for Linux"};
#endif
}

void xrDebug::LogStackTrace(const char* header)
{
    xr_vector<xr_string> stackTrace = BuildStackTrace();
    Msg("%s", header);
    for (const auto& frame : stackTrace)
    {
        Msg("%s", frame.c_str());
    }
}


void xrDebug::GatherInfo(char* assertionInfo, size_t bufferSize, const ErrorLocation& loc, const char* expr,
                         const char* desc, const char* arg1, const char* arg2)
{
    char* buffer = assertionInfo;
    if (!expr)
        expr = "<no expression>";
    bool extendedDesc = desc && strchr(desc, '\n');
    pcstr prefix = "[error] ";
    const char* oneAboveBuffer = assertionInfo + bufferSize;
    buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "\nFATAL ERROR\n\n");
    buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%sExpression    : %s\n", prefix, expr);
    buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%sFunction      : %s\n", prefix, loc.Function);
    buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%sFile          : %s\n", prefix, loc.File);
    buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%sLine          : %d\n", prefix, loc.Line);
    if (extendedDesc)
    {
        buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "\n%s\n", desc);
        if (arg1)
        {
            buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%s\n", arg1);
            if (arg2)
                buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%s\n", arg2);
        }
    }
    else
    {
        buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%sDescription   : %s\n", prefix, desc);
        if (arg1)
        {
            if (arg2)
            {
                buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%sArgument 0    : %s\n", prefix, arg1);
                buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%sArgument 1    : %s\n", prefix, arg2);
            }
            else
                buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%sArguments     : %s\n", prefix, arg1);
        }
    }
    buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "\n");

    Log(assertionInfo);
    FlushLog();

    buffer = assertionInfo;
#if defined(XR_PLATFORM_WINDOWS)
    if (DebuggerIsPresent() || !strstr(GetCommandLine(), "-no_call_stack_assert"))
        return;
#endif
    Log("stack trace:\n");
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
    buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "stack trace:\n\n");
#endif // USE_OWN_ERROR_MESSAGE_WINDOW
#if defined(XR_PLATFORM_WINDOWS)
    xr_vector<xr_string> stackTrace = BuildStackTrace();
    for (size_t i = 2; i < stackTrace.size(); i++)
    {
        Log(stackTrace[i].c_str());
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
        buffer += xr_sprintf(buffer, oneAboveBuffer - buffer, "%s\n", stackTrace[i].c_str());
#endif // USE_OWN_ERROR_MESSAGE_WINDOW
    }
#elif defined(BACKTRACE_AVAILABLE)
    void* array[20];
    int nptrs = backtrace(array, 20); // get void*'s for all entries on the stack
    char** strings = backtrace_symbols(array, nptrs);

    if (strings)
    {
        size_t demangledBufSize = 0;
        char* demangledName = nullptr;
        for (int i = 0; i < nptrs; i++)
        {
            char* functionName = strings[i];

#   ifdef CXXABI_AVAILABLE
            Dl_info info;

            if (dladdr(array[i], &info))
            {
                if (info.dli_sname)
                {
                    int status = -1;
                    demangledName = abi::__cxa_demangle(info.dli_sname, demangledName, &demangledBufSize, &status);
                    if (status == 0)
                    {
                        functionName = demangledName;
                    }
                }
            }
#   endif
            Log(functionName);
#   ifdef USE_OWN_ERROR_MESSAGE_WINDOW
            buffer += xr_sprintf(buffer, bufferSize, "%s\n", functionName);
#   endif // USE_OWN_ERROR_MESSAGE_WINDOW
        }
        ::free(demangledName);
    }
#endif
    FlushLog();
    os_clipboard::copy_to_clipboard(assertionInfo);
}

void xrDebug::Fatal(const ErrorLocation& loc, const char* format, ...)
{
    string1024 desc;
    va_list args;
    va_start(args, format);
    vsnprintf(desc, sizeof(desc), format, args);
    va_end(args);
    bool ignoreAlways = true;
    Fail(ignoreAlways, loc, nullptr, "fatal error", desc);
}

AssertionResult xrDebug::Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, long hresult, const char* arg1,
                   const char* arg2)
{
    return Fail(ignoreAlways, loc, expr, xrDebug::ErrorToString(hresult), arg1, arg2);
}

AssertionResult xrDebug::Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, const char* desc, const char* arg1,
                   const char* arg2)
{
    ScopeLock lock(&failLock);

    if (windowHandler)
        windowHandler->OnErrorDialog(true); // Call it only after locking so that multiple threads won't call this function simultaneously.

    ErrorAfterDialog = true;
    string4096 assertionInfo;
    GatherInfo(assertionInfo, sizeof(assertionInfo), loc, expr, desc, arg1, arg2);

    if (ShowErrorMessage)
    {
        xr_strcat(assertionInfo,
            "\r\n"
            "Press CANCEL to abort execution\r\n"
            "Press TRY AGAIN to continue execution\r\n"
            "Press CONTINUE to continue execution and ignore all the errors of this type\r\n"
            "\r\n");
    }

    FlushLog();

    bool resetFullscreen = false;
    AssertionResult result = AssertionResult::abort;
    if (Core.PluginMode)
        /*result =*/ ShowMessage("X-Ray error", assertionInfo); // Do not assign 'result'
    else
    {
        if (ShowErrorMessage)
            result = ShowMessage("Fatal error", assertionInfo, false);

        switch (result)
        {
        case AssertionResult::tryAgain:
            ErrorAfterDialog = false;
            resetFullscreen = windowHandler != nullptr;
            break;

        case AssertionResult::ignore:
            ErrorAfterDialog = false;
            ignoreAlways = true;
            resetFullscreen = windowHandler != nullptr;
            break;

        case AssertionResult::undefined:
            xr_strcat(assertionInfo, SDL_GetError());
            [[fallthrough]];
        case AssertionResult::abort:
            [[fallthrough]];
        default:
#ifdef USE_BUG_TRAP
            BT_SetUserMessage(assertionInfo);
#endif
            // calling DEBUG_BREAK with no debugger will trigger BugTrap
            // we must hide the window
            if (windowHandler && !DebuggerIsPresent())
                windowHandler->OnFatalError();
            DEBUG_BREAK;
        } // switch (result)
    }

    if (resetFullscreen)
        windowHandler->OnErrorDialog(false); // Call it only before unlocking so that multiple threads won't call this function simultaneously.

    return result;
}

AssertionResult xrDebug::Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, const std::string& desc,
                   const char* arg1, const char* arg2)
{
    return Fail(ignoreAlways, loc, expr, desc.c_str(), arg1, arg2);
}

[[noreturn]]
void xrDebug::DoExit(const std::string& message)
{
    ScopeLock lock(&failLock);

    if (windowHandler)
        windowHandler->OnErrorDialog(true);

    FlushLog();

    if (ShowErrorMessage)
    {
        const auto result = ShowMessage(Core.ApplicationName, message.c_str(), false);
        if (result != AssertionResult::abort && DebuggerIsPresent())
            DEBUG_BREAK;
    }
    else
        ShowMessage(Core.ApplicationName, message.c_str());

    if (windowHandler)
        windowHandler->OnFatalError();

#if defined(XR_PLATFORM_WINDOWS)
    TerminateProcess(GetCurrentProcess(), 1);
#else
    exit(1);
#endif
    // if you're under debugger, you can jump here manually
    if (windowHandler)
        windowHandler->OnErrorDialog(false);
}

pcstr xrDebug::ErrorToString(long code)
{
    const char* result = nullptr;
#if defined(XR_PLATFORM_WINDOWS)
    static string1024 descStorage;
    DXGetErrorDescription(code, descStorage, sizeof(descStorage));
    if (!result)
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, code, 0, descStorage, sizeof(descStorage) - 1, 0);
        result = descStorage;
    }
#endif
    return result;
}

int out_of_memory_handler(size_t size)
{
    xrDebug::OutOfMemoryCallbackFunc cb = xrDebug::GetOutOfMemoryCallback();
    if (cb)
        cb();
    else
    {
        Memory.mem_compact();
        const size_t processHeap = Memory.mem_usage();
        const size_t ecoStrings = g_pStringContainer->stat_economy();
        const size_t ecoSmem = g_pSharedMemoryContainer->stat_economy();
        Msg("* [x-ray]: process heap[%zu K]", processHeap / 1024);
        Msg("* [x-ray]: economy: strings[%zu K], smem[%zu K]", ecoStrings / 1024, ecoSmem);
    }
    xrDebug::Fatal(DEBUG_INFO, "Out of memory. Memory request: %zu K", size / 1024);
    return 1;
}

extern pcstr log_name();

void WINAPI xrDebug::PreErrorHandler(INT_PTR)
{
#if defined(USE_BUG_TRAP) && defined(XR_PLATFORM_WINDOWS)
    if (xr_FS && FS.m_Flags.test(CLocatorAPI::flReady))
    {
        string_path cfg_full_name;
        __try
        {
            // Code below copied from CCC_LoadCFG::Execute (xr_ioc_cmd.cpp)
            // XXX: Refactor Console to accept user config filename on initialization or even construction!
            // XXX: Maybe refactor CCC_LoadCFG, move code for loading user.ltx into a generic function
            const auto cfg_name = userConfigHandler ? userConfigHandler->GetUserConfigFileName() : "user.ltx";
            FS.update_path(cfg_full_name, "$app_data_root$", cfg_name);

            if (!FS.exist(cfg_full_name))
                FS.update_path(cfg_full_name, "$fs_root$", cfg_name);

            if (!FS.exist(cfg_full_name))
                xr_strcpy(cfg_full_name, cfg_name);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            xr_strcpy(cfg_full_name, "user.ltx");
        }
        BT_AddLogFile(cfg_full_name);
    }

    if (*BugReportFile)
        BT_AddLogFile(BugReportFile);

    BT_SaveSnapshot(nullptr);
#endif
}

void xrDebug::SetupExceptionHandler()
{
#if defined(USE_BUG_TRAP) && defined(XR_PLATFORM_WINDOWS)
    const auto commandLine = GetCommandLine();

    // disable 'appname has stopped working' popup dialog
    const auto prevMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(prevMode | SEM_NOGPFAULTERRORBOX);
    BT_InstallSehFilter();

    if (!GEnv.isDedicatedServer && !strstr(commandLine, "-silent_error_mode"))
        BT_SetActivityType(BTA_SHOWUI);
    else
        BT_SetActivityType(BTA_SAVEREPORT);
    BT_SetDialogMessage(BTDM_INTRO2,
                        "This is OpenXRay crash reporting client. "
                        "To help the development process, "
                        "please Submit Bug or save report and email it manually (button More...)."
                        "\r\n"
                        "Many thanks in advance and sorry for the inconvenience.");
    BT_SetPreErrHandler(PreErrorHandler, 0);
    BT_SetAppName("OpenXRay");
    BT_SetReportFormat(BTRF_TEXT);
    BT_SetFlags(BTF_DETAILEDMODE | BTF_ATTACHREPORT);

    auto minidumpFlags = MiniDumpWithDataSegs|
        MiniDumpWithIndirectlyReferencedMemory |
        MiniDumpScanMemory |
        MiniDumpWithProcessThreadData |
        MiniDumpWithThreadInfo;

    if (strstr(commandLine, "-full_memory_dump"))
        minidumpFlags |= MiniDumpWithFullMemory | MiniDumpIgnoreInaccessibleMemory;
    else if (strstr(commandLine, "-detailed_minidump"))
        minidumpFlags |= MiniDumpWithIndirectlyReferencedMemory;

    BT_SetDumpType(minidumpFlags);
    //BT_SetSupportEMail("cop-crash-report@stalker-game.com");
    BT_SetSupportEMail("openxray@yahoo.com");
    BT_SetSupportURL("https://github.com/OpenXRay/xray-16/issues");
#endif
}

void xrDebug::OnFilesystemInitialized()
{
#ifdef USE_BUG_TRAP
    string_path path{};
    FS.update_path(path, "$logs$", "", false);
    if (!path[0] || path[0] != _DELIMITER && path[1] != ':') // relative path
    {
        string_path currentDir;
        _getcwd(currentDir, sizeof(currentDir));
        string_path relDir;
        xr_strcpy(relDir, path);
        strconcat(path, currentDir, DELIMITER, relDir);
    }
    xr_strcat(path, log_name());
    BT_AddLogFile(path);

    if (FS.update_path(path, "$app_data_root$", "reports", false))
    {
        BT_SetReportFilePath(path);
    }
#endif
}

bool xrDebug::DebuggerIsPresent()
{
#ifdef XR_PLATFORM_WINDOWS
    return IsDebuggerPresent();
#elif defined(PTRACE_AVAILABLE)
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1)
        return true;
    ptrace(PTRACE_DETACH, 0, 0, 0);
    return false;
#else
    return false;
#endif
}

void xrDebug::FormatLastError(char* buffer, const size_t& bufferSize)
{
#if defined(XR_PLATFORM_WINDOWS)
    const int lastErr = GetLastError();
    if (lastErr == ERROR_SUCCESS)
    {
        *buffer = 0;
        return;
    }
    void* msg = nullptr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, lastErr,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (pstr)&msg, 0, nullptr);
    // XXX nitrocaster: check buffer overflow
    xr_sprintf(buffer, bufferSize, "[error][%8d]: %s", lastErr, (char*)msg);
    LocalFree(msg);
#endif
}

LONG WINAPI xrDebug::UnhandledFilter(EXCEPTION_POINTERS* exPtrs)
{
#if defined(XR_PLATFORM_WINDOWS)
    ScopeLock lock(&failLock);

    string256 errMsg;
    FormatLastError(errMsg, sizeof(errMsg));
    if (!ErrorAfterDialog && !strstr(GetCommandLine(), "-no_call_stack_assert"))
    {
        CONTEXT save = *exPtrs->ContextRecord;
        xr_vector<xr_string> stackTrace = BuildStackTrace(exPtrs->ContextRecord, 1024);
        *exPtrs->ContextRecord = save;
        Msg("stack trace:\n");
#ifdef DEBUG
        if (!DebuggerIsPresent())
            os_clipboard::copy_to_clipboard("stack trace:\r\n\r\n");
#endif
        string4096 buffer;
        for (size_t i = 0; i < stackTrace.size(); i++)
        {
            Log(stackTrace[i].c_str());
            xr_sprintf(buffer, sizeof(buffer), "%s\r\n", stackTrace[i].c_str());
#ifdef DEBUG
            if (!DebuggerIsPresent())
                os_clipboard::update_clipboard(buffer);
#endif
        }
        if (*errMsg)
        {
            Msg("\n%s", errMsg);
            xr_strcat(errMsg, "\r\n");
#ifdef DEBUG
            if (!DebuggerIsPresent())
                os_clipboard::update_clipboard(buffer);
#endif
        }
    }
    FlushLog();

    if (windowHandler)
        windowHandler->OnErrorDialog(true);

    static constexpr pcstr fatalError = "Fatal error";

    AssertionResult msgRes = AssertionResult::abort;

    if (!ErrorAfterDialog && ShowErrorMessage)
    {
        static constexpr pcstr msg = "Fatal error occurred\n\n"
            "Press OK to abort program execution";
        msgRes = ShowMessage(fatalError, msg);
    }

    BT_SetUserMessage(fatalError);
    BT_SaveSnapshotEx(exPtrs, nullptr);

    const auto reportRes = ReportFault(exPtrs, 0);
    if (msgRes != AssertionResult::abort ||
        reportRes == frrvLaunchDebugger)
    {
        constexpr cpcstr debugger = "Please, attach the debugger to the process"
            " if you want to debug this fatal error.";
        ShowMessage("xrDebug", debugger);
        if (DebuggerIsPresent())
            DEBUG_BREAK;
    }

    // Typically, PrevFilter is BugTrap filter
    if (PrevFilter)
    {
        if (windowHandler)
            windowHandler->OnFatalError();
        PrevFilter(exPtrs);
    }

    if (windowHandler)
        windowHandler->OnErrorDialog(false);

    return EXCEPTION_CONTINUE_SEARCH;
#else
    return 0;
#endif
}

#ifndef USE_BUG_TRAP
[[noreturn]]
void xr_terminate()
{
#if defined(XR_PLATFORM_WINDOWS)
    if (strstr(GetCommandLine(), "-silent_error_mode"))
        exit(-1);
#endif
    //ScopeLock lock(&failLock);

    string4096 assertionInfo;
    xrDebug::GatherInfo(assertionInfo,sizeof(assertionInfo), DEBUG_INFO, nullptr, "Unexpected application termination");
    xr_strcat(assertionInfo, "Press OK to abort execution\r\n");
    xrDebug::ShowMessage("Fatal Error", assertionInfo);
    exit(-1);
}
#endif // USE_BUG_TRAP

static void handler_base(const char* reason)
{
    bool ignoreAlways = false;
    xrDebug::Fail(ignoreAlways, DEBUG_INFO, nullptr, reason, nullptr, nullptr);
}

#if defined(XR_PLATFORM_WINDOWS)
static void invalid_parameter_handler(const wchar_t* expression, const wchar_t* function, const wchar_t* file,
                                      unsigned int line, uintptr_t reserved)
{
    bool ignoreAlways = false;
    string4096 mbExpression;
    string4096 mbFunction;
    string4096 mbFile;
    size_t convertedChars = 0;
    if (expression)
        wcstombs_s(&convertedChars, mbExpression, sizeof(mbExpression), expression, (wcslen(expression) + 1) * 2);
    else
        xr_strcpy(mbExpression, "");
    if (function)
        wcstombs_s(&convertedChars, mbFunction, sizeof(mbFunction), function, (wcslen(function) + 1) * 2);
    else
        xr_strcpy(mbFunction, __FUNCTION__);
    if (file)
        wcstombs_s(&convertedChars, mbFile, sizeof(mbFile), file, (wcslen(file) + 1) * 2);
    else
    {
        line = __LINE__;
        xr_strcpy(mbFile, __FILE__);
    }
    xrDebug::Fail(ignoreAlways, {mbFile, int(line), mbFunction}, mbExpression, "invalid parameter");
}
#endif

void xrDebug::OnThreadSpawn()
{
#ifndef __SANITIZE_ADDRESS__
    std::signal(SIGINT,  nullptr);
    std::signal(SIGILL,  +[](int signal) { handler_base("illegal instruction"); });
    std::signal(SIGFPE,  +[](int signal) { handler_base("floating point error"); });
#   ifdef DEBUG
    std::signal(SIGSEGV, +[](int signal) { handler_base("segmentation fault"); });
#   endif
    std::signal(SIGABRT, +[](int signal) { handler_base("application is aborting"); });
    std::signal(SIGTERM, +[](int signal) { handler_base("termination with exit code 3"); });

#   if defined(XR_PLATFORM_WINDOWS)
    std::signal(SIGABRT_COMPAT, +[](int signal) { handler_base("application is aborting"); });
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _set_invalid_parameter_handler(&invalid_parameter_handler);
    _set_new_mode(1);
    _set_new_handler(&out_of_memory_handler);
    _set_purecall_handler(+[] { handler_base("pure virtual function call"); });
#   endif

#   ifdef USE_BUG_TRAP
    BT_SetTerminate();
#   else
    std::set_terminate(xr_terminate);
#   endif
#endif
}

void xrDebug::OnThreadExit()
{
#ifndef __SANITIZE_ADDRESS__
    std::signal(SIGINT,  nullptr);
    std::signal(SIGILL,  nullptr);
    std::signal(SIGFPE,  nullptr);
    std::signal(SIGSEGV, nullptr);
    std::signal(SIGABRT, nullptr);
    std::signal(SIGTERM, nullptr);
    std::set_terminate(nullptr);

#   if defined(XR_PLATFORM_WINDOWS)
    std::signal(SIGABRT_COMPAT, nullptr);
    _set_abort_behavior(0, 0);
    _set_invalid_parameter_handler(nullptr);
    _set_new_mode(1);
    _set_new_handler(nullptr);
    _set_purecall_handler(nullptr);
#   endif
#endif
}

void xrDebug::Initialize(pcstr commandLine)
{
    ZoneScoped;
    *BugReportFile = 0;
    OnThreadSpawn();
    SetupExceptionHandler();
    SDL_SetAssertionHandler(SDLAssertionHandler, nullptr);
    // exception handler to all "unhandled" exceptions
#if defined(XR_PLATFORM_WINDOWS)
    PrevFilter = SetUnhandledExceptionFilter(UnhandledFilter);
#endif
#ifdef DEBUG
    ShowErrorMessage = true;
#else
    ShowErrorMessage = commandLine ? !!strstr(commandLine, "-show_error_window") : false;
#endif
}

void xrDebug::Finalize()
{
    OnThreadExit();
    SDL_SetAssertionHandler(nullptr, nullptr);
#if defined(XR_PLATFORM_WINDOWS)
    SetUnhandledExceptionFilter(nullptr);
#endif
    ShowErrorMessage = false;
}
