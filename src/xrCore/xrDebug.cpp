#include "stdafx.h"
#pragma hdrstop

#include "SDL.h"
#include "SDL_syswm.h"

#include "xrDebug.h"
#include "os_clipboard.h"
#include "log.h"
#if defined(XR_PLATFORM_WINDOWS)
#include "Debug/dxerr.h"
#endif
#include "Threading/ScopeLock.hpp"

#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of '' when no variable is declared
#if defined(XR_PLATFORM_WINDOWS)
#include "Debug/MiniDump.h"
#pragma warning(pop)
#include <malloc.h>
#include <direct.h>
#endif

#if defined(XR_PLATFORM_WINDOWS)
#define USE_BUG_TRAP
static BOOL bException = FALSE;
#endif

#ifndef USE_BUG_TRAP
#include <exception>
#endif

#ifdef USE_BUG_TRAP
#include <BugTrap/source/Client/BugTrap.h>
#endif

#if defined(XR_PLATFORM_WINDOWS)
#include <new.h> // for _set_new_mode
#include <signal.h> // for signals
#include <errorrep.h> // ReportFault
#elif defined(XR_PLATFORM_LINUX)
#include <sys/user.h>
#include <sys/ptrace.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#endif
#pragma comment(lib, "FaultRep.lib")

#ifdef DEBUG
#define USE_OWN_ERROR_MESSAGE_WINDOW
#endif

#if defined(XR_PLATFORM_WINDOWS)
#   if defined(XR_ARCHITECTURE_X86)
#       define MACHINE_TYPE IMAGE_FILE_MACHINE_I386
#   elif defined(XR_ARCHITECTURE_X64)
#       define MACHINE_TYPE IMAGE_FILE_MACHINE_AMD64
#   elif defined(XR_ARCHITECTURE_ARM)
#       define MACHINE_TYPE IMAGE_FILE_MACHINE_ARM
#   elif defined(XR_ARCHITECTURE_ARM64)
#       define MACHINE_TYPE IMAGE_FILE_MACHINE_ARM64
#   else
#       error CPU architecture is not supported.
#   endif
#endif // XR_PLATFORM_WINDOWS

constexpr SDL_MessageBoxButtonData buttons[] =
{
    /* .flags, .buttonid, .text */
    { 0, (int)AssertionResult::ignore, "Continue"  },
    { 0, (int)AssertionResult::tryAgain, "Try again" },

    { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT |
      SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,
         (int)AssertionResult::abort, "Cancel" }
};

SDL_MessageBoxData messageboxdata =
{
    SDL_MESSAGEBOX_ERROR,
    nullptr,
    "Fatal error",
    "Vse clomalocb, tashite novyy dvizhok",
    SDL_arraysize(buttons),
    buttons,
    nullptr
};

AssertionResult xrDebug::ShowMessage(pcstr title, pcstr message, bool simpleMode)
{
#ifdef XR_PLATFORM_WINDOWS // because Windows default Message box is fancy
    HWND hwnd = nullptr;

    if (windowHandler)
    {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(windowHandler->GetApplicationWindow(), &info))
        {
            switch (info.subsystem)
            {
            case SDL_SYSWM_WINDOWS:
                hwnd = info.info.win.window;
                break;
            default: break;
            }
        }
    }

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

    if (windowHandler)
        messageboxdata.window =  windowHandler->GetApplicationWindow();
    messageboxdata.title = title;
    messageboxdata.message = message;
    int button = -1;
    SDL_ShowMessageBox(&messageboxdata, &button);
    return (AssertionResult)button;
#endif
}

SDL_AssertState SDLAssertionHandler(const SDL_AssertData* data,
    void* /*userdata*/)
{
    if (data->always_ignore)
        return SDL_ASSERTION_ALWAYS_IGNORE;

    constexpr pcstr desc = "SDL2 assertion triggered";
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
xrDebug::UnhandledExceptionFilter xrDebug::PrevFilter = nullptr;
xrDebug::OutOfMemoryCallbackFunc xrDebug::OutOfMemoryCallback = nullptr;
xrDebug::CrashHandler xrDebug::OnCrash = nullptr;
xrDebug::DialogHandler xrDebug::OnDialog = nullptr;
string_path xrDebug::BugReportFile;
bool xrDebug::ErrorAfterDialog = false;
bool xrDebug::ShowErrorMessage = false;

bool xrDebug::symEngineInitialized = false;
Lock xrDebug::dbgHelpLock;

#if defined(XR_PLATFORM_WINDOWS)
void xrDebug::SetBugReportFile(const char* fileName) { xr_strcpy(BugReportFile, fileName); }
#elif defined(XR_PLATFORM_LINUX)
void xrDebug::SetBugReportFile(const char* fileName) { xr_strcpy(BugReportFile, 0, fileName); }
#endif

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

    SStringVec traceResult;
    STACKFRAME stackFrame = {};
    xr_string frameStr;

    if (!InitializeSymbolEngine())
    {
        Msg("[xrDebug::BuildStackTrace]InitializeSymbolEngine failed with error: %d", GetLastError());
        return traceResult;
    }

    traceResult.reserve(maxFramesCount);

#if defined XR_ARCHITECTURE_X64
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrPC.Offset = threadCtx->Rip;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = threadCtx->Rsp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = threadCtx->Rbp;
#elif defined XR_ARCHITECTURE_X86
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrPC.Offset = threadCtx->Eip;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = threadCtx->Esp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = threadCtx->Ebp;
#elif defined XR_ARCHITECTURE_ARM
#error TODO
#elif defined XR_ARCHITECTURE_ARM64
#error TODO
#else
#error CPU architecture is not supported.
#endif

    while (GetNextStackFrameString(&stackFrame, threadCtx, frameStr) && traceResult.size() <= maxFramesCount)
    {
        traceResult.push_back(frameStr);
    }

    DeinitializeSymbolEngine();

    return traceResult;
}
#endif // defined(XR_PLATFORM_WINDOWS)

SStringVec xrDebug::BuildStackTrace(u16 maxFramesCount)
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
    SStringVec stackTrace = BuildStackTrace();
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
#elif defined(XR_PLATFORM_LINUX)
    void *array[20];
    int nptrs = backtrace(array, 20);     // get void*'s for all entries on the stack
    char **strings = backtrace_symbols(array, nptrs);

    if (strings)
    {
        size_t demangledBufSize = 0;
        char* demangledName = nullptr;
        for (size_t i = 0; i < nptrs; i++)
        {
            char* functionName = strings[i];

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
            Log(functionName);
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
            buffer += xr_sprintf(buffer, bufferSize, "%s\n", functionName);
#endif // USE_OWN_ERROR_MESSAGE_WINDOW
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
#ifdef PROFILE_CRITICAL_SECTIONS
    static Lock lock(MUTEX_PROFILE_ID(xrDebug::Backend));
#else
    static Lock lock;
#endif
    lock.Enter();
    ErrorAfterDialog = true;
    string4096 assertionInfo;
    auto size = sizeof(assertionInfo);
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

    if (OnCrash)
        OnCrash();

    if (OnDialog)
        OnDialog(true);

    FlushLog();

    if (windowHandler)
        windowHandler->DisableFullscreen();

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
            DEBUG_BREAK;
        } // switch (result)
    }

    if (OnDialog)
        OnDialog(false);

    if (resetFullscreen)
        windowHandler->ResetFullscreen();

    lock.Leave();
    return result;
}

AssertionResult xrDebug::Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, const std::string& desc,
                   const char* arg1, const char* arg2)
{
    return Fail(ignoreAlways, loc, expr, desc.c_str(), arg1, arg2);
}

void xrDebug::DoExit(const std::string& message)
{
    if (windowHandler)
        windowHandler->DisableFullscreen();

    if (OnDialog)
        OnDialog(true);

    FlushLog();

    if (ShowErrorMessage)
    {
        const auto result = ShowMessage(Core.ApplicationName, message.c_str(), false);
        if (result != AssertionResult::abort && DebuggerIsPresent())
            DEBUG_BREAK;
    }
    else
        ShowMessage(Core.ApplicationName, message.c_str());

#if defined(XR_PLATFORM_WINDOWS)
    TerminateProcess(GetCurrentProcess(), 1);
#else
    exit(1);
#endif
    // if you're under debugger, you can jump here manually
    if (OnDialog)
        OnDialog(false);

    if (windowHandler)
        windowHandler->ResetFullscreen();
}

LPCSTR xrDebug::ErrorToString(long code)
{
    const char* result = nullptr;
    static string1024 descStorage;
#if defined(XR_PLATFORM_WINDOWS)
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
        size_t processHeap = Memory.mem_usage();
        size_t ecoStrings = g_pStringContainer->stat_economy();
        size_t ecoSmem = g_pSharedMemoryContainer->stat_economy();
        Msg("* [x-ray]: process heap[%zu K]", processHeap / 1024);
        Msg("* [x-ray]: economy: strings[%zu K], smem[%zu K]", ecoStrings / 1024, ecoSmem);
    }
    xrDebug::Fatal(DEBUG_INFO, "Out of memory. Memory request: %zu K", size / 1024);
    return 1;
}

extern LPCSTR log_name();

void WINAPI xrDebug::PreErrorHandler(INT_PTR)
{
#if defined(USE_BUG_TRAP) && defined(XR_PLATFORM_WINDOWS)
    if (!xr_FS || !FS.m_Flags.test(CLocatorAPI::flReady))
        return;
    string_path logDir;
    __try
    {
        FS.update_path(logDir, "$logs$", "");
        if (logDir[0] != _DELIMITER && logDir[1] != ':')
        {
            string256 currentDir;
            _getcwd(currentDir, sizeof(currentDir));
            string256 relDir;
            xr_strcpy(relDir, logDir);
            strconcat(sizeof(logDir), logDir, currentDir, DELIMITER, relDir);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        xr_strcpy(logDir, "logs");
    }
    string_path temp;
    strconcat(sizeof(temp), temp, logDir, log_name());
    BT_AddLogFile(temp);
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
#ifdef MASTER_GOLD
    else if (!strstr(commandLine, "-detailed_minidump"))
        minidumpFlags |= MiniDumpFilterMemory;
#endif
    
    BT_SetDumpType(minidumpFlags);
    //BT_SetSupportEMail("cop-crash-report@stalker-game.com");
    BT_SetSupportEMail("openxray@yahoo.com");
    BT_SetSupportURL("https://github.com/OpenXRay/xray-16/issues");
#endif
}

void xrDebug::OnFilesystemInitialized()
{
#ifdef USE_BUG_TRAP
    string_path dumpPath;
    if (FS.update_path(dumpPath, "$app_data_root$", "reports", false))
    {
        BT_SetReportFilePath(dumpPath);
    }
#endif
}

bool xrDebug::DebuggerIsPresent()
{
#ifdef XR_PLATFORM_WINDOWS
    return IsDebuggerPresent();
#else
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) == -1)
        return true;
    ptrace(PTRACE_DETACH, 0, 0, 0);
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
        windowHandler->DisableFullscreen();

    if (OnDialog)
        OnDialog(true);

    constexpr pcstr fatalError = "Fatal error";

    AssertionResult msgRes = AssertionResult::abort;

    if (!ErrorAfterDialog && ShowErrorMessage)
    {
        constexpr pcstr msg = "Fatal error occurred\n\n"
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
        PrevFilter(exPtrs);

    if (OnDialog)
        OnDialog(false);

    if (windowHandler)
        windowHandler->ResetFullscreen();

    return EXCEPTION_CONTINUE_SEARCH;
#else
    return 0;
#endif
}

#ifndef USE_BUG_TRAP
void _terminate()
{
#if defined(XR_PLATFORM_WINDOWS)
    if (strstr(GetCommandLine(), "-silent_error_mode"))
        exit(-1);
#endif
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

static void invalid_parameter_handler(const wchar_t* expression, const wchar_t* function, const wchar_t* file,
                                      unsigned int line, uintptr_t reserved)
{
#if defined(XR_PLATFORM_WINDOWS)
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
#endif
}

static void pure_call_handler() { handler_base("pure virtual function call"); }
#ifdef XRAY_USE_EXCEPTIONS
static void unexpected_handler() { handler_base("unexpected program termination"); }
#endif

static void abort_handler(int signal) { handler_base("application is aborting"); }
static void floating_point_handler(int signal) { handler_base("floating point error"); }
static void illegal_instruction_handler(int signal) { handler_base("illegal instruction"); }
static void segmentation_fault_handler(int signal) { handler_base("segmentation fault"); }
static void termination_handler(int signal) { handler_base("termination with exit code 3"); }

void xrDebug::OnThreadSpawn()
{
#if defined(XR_PLATFORM_WINDOWS)
#ifdef USE_BUG_TRAP
    BT_SetTerminate();
#else
    // std::set_terminate(_terminate);
#endif
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    signal(SIGABRT, abort_handler);
    signal(SIGABRT_COMPAT, abort_handler);
    signal(SIGFPE, floating_point_handler);
    signal(SIGILL, illegal_instruction_handler);
    signal(SIGINT, 0);
    signal(SIGTERM, termination_handler);
    _set_invalid_parameter_handler(&invalid_parameter_handler);
    _set_new_mode(1);
    _set_new_handler(&out_of_memory_handler);
    _set_purecall_handler(&pure_call_handler);
#if 0 // should be if we use exceptions
    std::set_unexpected(_terminate);
#endif
#else //XR_PLATFORM_WINDOWS
    signal(SIGABRT, abort_handler);
    signal(SIGFPE, floating_point_handler);
    signal(SIGILL, illegal_instruction_handler);
    signal(SIGINT, 0);
    signal(SIGTERM, termination_handler);
    signal(SIGSEGV, segmentation_fault_handler);
#endif
}

void xrDebug::Initialize(pcstr commandLine)
{
    *BugReportFile = 0;
    OnThreadSpawn();
    SetupExceptionHandler();
    SDL_SetAssertionHandler(SDLAssertionHandler, nullptr);
    // exception handler to all "unhandled" exceptions
#if defined(XR_PLATFORM_WINDOWS)
    PrevFilter = ::SetUnhandledExceptionFilter(UnhandledFilter);
#endif
#ifdef DEBUG
    ShowErrorMessage = true;
#else
    ShowErrorMessage = commandLine ? !!strstr(commandLine, "-show_error_window") : false;
#endif
}
