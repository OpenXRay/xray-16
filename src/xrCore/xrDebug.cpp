#include "stdafx.h"
#pragma hdrstop

#include "xrDebug.h"
#include "os_clipboard.h"
#include "log.h"
#if defined(WINDOWS)
#include "Debug/dxerr.h"
#endif
#include "Threading/ScopeLock.hpp"

#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of '' when no variable is declared
#if defined(WINDOWS)
#include "Debug/MiniDump.h"
#pragma warning(pop)
#include <malloc.h>
#include <direct.h>
#endif

extern bool shared_str_initialized;

#if defined(WINDOWS)
#ifdef __BORLANDC__
#include "d3d9.h"
#include "d3dx9.h"
#include "D3DX_Wrapper.h"
#pragma comment(lib, "EToolsB.lib")
#define USE_BUG_TRAP
#else
#define USE_BUG_TRAP
static BOOL bException = FALSE;
#endif
#endif

#ifndef USE_BUG_TRAP
#include <exception>
#endif

#ifdef USE_BUG_TRAP
#include <BugTrap/source/Client/BugTrap.h>
#endif

#if defined(WINDOWS)
#include <new.h> // for _set_new_mode
#include <signal.h> // for signals
#include <errorrep.h> // ReportFault
#elif defined(LINUX)
#include <sys/user.h>
#include <sys/ptrace.h>
#endif
#pragma comment(lib, "FaultRep.lib")

#ifdef DEBUG
#define USE_OWN_ERROR_MESSAGE_WINDOW
#else
#define USE_OWN_MINI_DUMP
#endif

#if defined XR_X64
#	define MACHINE_TYPE IMAGE_FILE_MACHINE_AMD64
#elif defined XR_X86
#	define MACHINE_TYPE IMAGE_FILE_MACHINE_I386
#else
#	error CPU architecture is not supported.
#endif

namespace
{
ICN void* GetInstructionPtr()
{
#if defined(LINUX)
    pid_t traced_process;
    struct user_regs_struct regs;
    ptrace(PTRACE_ATTACH, traced_process, NULL, NULL);
    ptrace(PTRACE_GETREGS, traced_process, NULL, &regs);

    return regs.rip;
#else
#ifdef _MSC_VER
    return _ReturnAddress();
#else
#ifdef _WIN64
    _asm mov rax, [rsp]
    _asm retn
#else
    _asm mov eax, [esp]
    _asm retn
#endif
#endif
#endif
}
}

xrDebug::UnhandledExceptionFilter xrDebug::PrevFilter = nullptr;
xrDebug::OutOfMemoryCallbackFunc xrDebug::OutOfMemoryCallback = nullptr;
xrDebug::CrashHandler xrDebug::OnCrash = nullptr;
xrDebug::DialogHandler xrDebug::OnDialog = nullptr;
string_path xrDebug::BugReportFile;
bool xrDebug::ErrorAfterDialog = false;

bool xrDebug::symEngineInitialized = false;
Lock xrDebug::dbgHelpLock;

#if defined(WINDOWS)
void xrDebug::SetBugReportFile(const char* fileName) { strcpy_s(BugReportFile, fileName); }
#elif defined(LINUX)
void xrDebug::SetBugReportFile(const char* fileName) { strcpy_s(BugReportFile, 0, fileName); }
#endif

#if defined(WINDOWS)
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
    BYTE arrSymBuffer[512];
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
        DWORD dwOptions = SymGetOptions();
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

#if defined XR_X64
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrPC.Offset = threadCtx->Rip;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = threadCtx->Rsp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = threadCtx->Rbp;
#elif defined XR_X86
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrPC.Offset = threadCtx->Eip;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = threadCtx->Esp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = threadCtx->Ebp;
#else
#	error CPU architecture is not supported.
#endif

    while (GetNextStackFrameString(&stackFrame, threadCtx, frameStr) && traceResult.size() <= maxFramesCount)
    {
        traceResult.push_back(frameStr);
    }

    DeinitializeSymbolEngine();

    return traceResult;
}

SStringVec xrDebug::BuildStackTrace(u16 maxFramesCount)
{
    CONTEXT currentThreadCtx = {};

    RtlCaptureContext(&currentThreadCtx); /// GetThreadContext can't be used on the current thread 
    currentThreadCtx.ContextFlags = CONTEXT_FULL;

    return BuildStackTrace(&currentThreadCtx, maxFramesCount);
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
#endif // defined(WINDOWS)


void xrDebug::GatherInfo(char* assertionInfo, const ErrorLocation& loc, const char* expr, const char* desc,
                         const char* arg1, const char* arg2)
{
    char* buffer = assertionInfo;
    if (!expr)
        expr = "<no expression>";
    bool extendedDesc = desc && strchr(desc, '\n');
    pcstr prefix = "[error] ";
    buffer += sprintf(buffer, "\nFATAL ERROR\n\n");
    buffer += sprintf(buffer, "%sExpression    : %s\n", prefix, expr);
    buffer += sprintf(buffer, "%sFunction      : %s\n", prefix, loc.Function);
    buffer += sprintf(buffer, "%sFile          : %s\n", prefix, loc.File);
    buffer += sprintf(buffer, "%sLine          : %d\n", prefix, loc.Line);
    if (extendedDesc)
    {
        buffer += sprintf(buffer, "\n%s\n", desc);
        if (arg1)
        {
            buffer += sprintf(buffer, "%s\n", arg1);
            if (arg2)
                buffer += sprintf(buffer, "%s\n", arg2);
        }
    }
    else
    {
        buffer += sprintf(buffer, "%sDescription   : %s\n", prefix, desc);
        if (arg1)
        {
            if (arg2)
            {
                buffer += sprintf(buffer, "%sArgument 0    : %s\n", prefix, arg1);
                buffer += sprintf(buffer, "%sArgument 1    : %s\n", prefix, arg2);
            }
            else
                buffer += sprintf(buffer, "%sArguments     : %s\n", prefix, arg1);
        }
    }
    buffer += sprintf(buffer, "\n");
    if (shared_str_initialized)
    {
        Log(assertionInfo);
        FlushLog();
    }
    buffer = assertionInfo;
#if defined(WINDOWS)
    if (IsDebuggerPresent() || !strstr(GetCommandLine(), "-no_call_stack_assert"))
        return;
#endif
    if (shared_str_initialized)
        Log("stack trace:\n");
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
    buffer += sprintf(buffer, "stack trace:\n\n");
#endif // USE_OWN_ERROR_MESSAGE_WINDOW
    xr_vector<xr_string> stackTrace = BuildStackTrace();
    for (size_t i = 2; i < stackTrace.size(); i++)
    {
        if (shared_str_initialized)
            Log(stackTrace[i].c_str());
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
        buffer += sprintf(buffer, "%s\n", stackTrace[i].c_str());
#endif // USE_OWN_ERROR_MESSAGE_WINDOW
    }
    if (shared_str_initialized)
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

void xrDebug::Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, long hresult, const char* arg1,
                   const char* arg2)
{
    Fail(ignoreAlways, loc, expr, xrDebug::ErrorToString(hresult), arg1, arg2);
}

void xrDebug::Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, const char* desc, const char* arg1,
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
    GatherInfo(assertionInfo, loc, expr, desc, arg1, arg2);
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
    strcat(assertionInfo,
           "\r\n"
           "Press CANCEL to abort execution\r\n"
           "Press TRY AGAIN to continue execution\r\n"
           "Press CONTINUE to continue execution and ignore all the errors of this type\r\n"
           "\r\n");
#endif
    if (OnCrash)
        OnCrash();
    if (OnDialog)
        OnDialog(true);
    FlushLog();
#if defined(WINDOWS)
    if (Core.PluginMode)
        MessageBox(NULL, assertionInfo, "X-Ray error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    else
    {
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
        int result = MessageBox(NULL, assertionInfo, "Fatal error",
                                MB_CANCELTRYCONTINUE | MB_ICONERROR | MB_SYSTEMMODAL);
        switch (result)
        {
        case IDCANCEL:
#ifdef USE_BUG_TRAP
            BT_SetUserMessage(assertionInfo);
#endif
            DEBUG_BREAK;
            break;
        case IDTRYAGAIN: ErrorAfterDialog = false;
            break;
        case IDCONTINUE:
            ErrorAfterDialog = false;
            ignoreAlways = true;
            break;
        default: DEBUG_BREAK;
        }
#else // !USE_OWN_ERROR_MESSAGE_WINDOW
#ifdef USE_BUG_TRAP
        BT_SetUserMessage(assertionInfo);
#endif
        DEBUG_BREAK;
#endif
    }
#endif
    if (OnDialog)
        OnDialog(false);

    lock.Leave();
}

void xrDebug::Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, const std::string& desc,
                   const char* arg1, const char* arg2)
{
    Fail(ignoreAlways, loc, expr, desc.c_str(), arg1, arg2);
}

void xrDebug::DoExit(const std::string& message)
{
    FlushLog();
#if defined(WINDOWS)
    MessageBox(NULL, message.c_str(), "Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    TerminateProcess(GetCurrentProcess(), 1);
#endif
}

LPCSTR xrDebug::ErrorToString(long code)
{
    const char* result = nullptr;
    static string1024 descStorage;
#if defined(WINDOWS)
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

#ifdef USE_BUG_TRAP
void WINAPI xrDebug::PreErrorHandler(INT_PTR)
{
    if (!xr_FS || !FS.m_Flags.test(CLocatorAPI::flReady))
        return;
    string_path logDir;
    __try
    {
        FS.update_path(logDir, "$logs$", "");
        if (logDir[0] != '\\' && logDir[1] != ':')
        {
            string256 currentDir;
            _getcwd(currentDir, sizeof(currentDir));
            string256 relDir;
            strcpy_s(relDir, logDir);
            strconcat(sizeof(logDir), logDir, currentDir, "\\", relDir);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        strcpy_s(logDir, "logs");
    }
    string_path temp;
    strconcat(sizeof(temp), temp, logDir, log_name());
#if defined(WINDOWS)
    BT_AddLogFile(temp);
    if (*BugReportFile)
        BT_AddLogFile(BugReportFile);

    string_path dumpPath;
    if (FS.path_exist("$app_data_root$"))
        FS.update_path(dumpPath, "$app_data_root$", "");
    xr_strcat(dumpPath, "reports");

    BT_SetReportFilePath(dumpPath);
    BT_SaveSnapshot(nullptr);
#endif
}

void xrDebug::SetupExceptionHandler(const bool& dedicated)
{
#if defined(WINDOWS)
    // disable 'appname has stopped working' popup dialog
    UINT prevMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(prevMode | SEM_NOGPFAULTERRORBOX);
    BT_InstallSehFilter();
    if (!dedicated && !strstr(GetCommandLine(), "-silent_error_mode"))
        BT_SetActivityType(BTA_SHOWUI);
    else
        BT_SetActivityType(BTA_SAVEREPORT);
    BT_SetDialogMessage(BTDM_INTRO2,
                        "This is X-Ray Engine v1.6 crash reporting client. "
                        "To help the development process, "
                        "please Submit Bug or save report and email it manually (button More...)."
                        "\r\n"
                        "Many thanks in advance and sorry for the inconvenience.");
    BT_SetPreErrHandler(PreErrorHandler, 0);
    BT_SetAppName("X-Ray Engine");
    BT_SetReportFormat(BTRF_TEXT);
    BT_SetFlags(BTF_DETAILEDMODE | BTF_ATTACHREPORT);
#ifdef MASTER_GOLD
#ifdef _EDITOR // MASTER_GOLD && EDITOR
    auto minidumpFlags = !dedicated ? MiniDumpNoDump : MiniDumpWithDataSegs;
#else // MASTER_GOLD && !EDITOR
    auto minidumpFlags = !dedicated ? MiniDumpNoDump : MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory;
#endif
#else
#ifdef EDITOR // !MASTER_GOLD && EDITOR
    auto minidumpFlags = MiniDumpWithDataSegs;
#else // !MASTER_GOLD && !EDITOR
    auto minidumpFlags = MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory;
#endif
#endif
    BT_SetDumpType(minidumpFlags);
    //BT_SetSupportEMail("cop-crash-report@stalker-game.com");
    BT_SetSupportEMail("openxray@yahoo.com");
#endif
}
#endif // USE_BUG_TRAP

#ifdef USE_OWN_MINI_DUMP
void xrDebug::SaveMiniDump(EXCEPTION_POINTERS *exPtrs)
{
#if defined(WINDOWS)
    string64 dateStr;
    timestamp(dateStr);
    string_path dumpPath;
    sprintf(dumpPath, "%s_%s_%s.mdmp", Core.ApplicationName, Core.UserName, dateStr);
    __try
    {
        if (FS.path_exist("$logs$"))
            FS.update_path(dumpPath, "$logs$", dumpPath);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        string_path temp;
        strcpy_s(temp, dumpPath);
        sprintf(dumpPath, "logs/%s", temp);
    }
    WriteMiniDump(MINIDUMP_TYPE(MiniDumpFilterMemory | MiniDumpScanMemory), dumpPath, GetCurrentThreadId(), exPtrs);
#endif
}
#endif

void xrDebug::FormatLastError(char* buffer, const size_t& bufferSize)
{
#if defined(WINDOWS)
    int lastErr = GetLastError();
    if (lastErr == ERROR_SUCCESS)
    {
        *buffer = 0;
        return;
    }
    void* msg = nullptr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, lastErr,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, nullptr);
    // XXX nitrocaster: check buffer overflow
    sprintf(buffer, "[error][%8d]: %s", lastErr, (char*)msg);
    LocalFree(msg);
#endif
}

LONG WINAPI xrDebug::UnhandledFilter(EXCEPTION_POINTERS* exPtrs)
{
#if defined(WINDOWS)
    string256 errMsg;
    FormatLastError(errMsg, sizeof(errMsg));
    if (!ErrorAfterDialog && !strstr(GetCommandLine(), "-no_call_stack_assert"))
    {
        CONTEXT save = *exPtrs->ContextRecord;
        xr_vector<xr_string> stackTrace = BuildStackTrace(exPtrs->ContextRecord, 1024);
        *exPtrs->ContextRecord = save;
        if (shared_str_initialized)
            Msg("stack trace:\n");
        if (!IsDebuggerPresent())
            os_clipboard::copy_to_clipboard("stack trace:\r\n\r\n");
        string4096 buffer;
        for (size_t i = 0; i < stackTrace.size(); i++)
        {
            if (shared_str_initialized)
                Log(stackTrace[i].c_str());
            sprintf(buffer, "%s\r\n", stackTrace[i].c_str());
#ifdef DEBUG
            if (!IsDebuggerPresent())
                os_clipboard::update_clipboard(buffer);
#endif
        }
        if (*errMsg)
        {
            if (shared_str_initialized)
                Msg("\n%s", errMsg);
            strcat(errMsg, "\r\n");
#ifdef DEBUG
            if (!IsDebuggerPresent())
                os_clipboard::update_clipboard(buffer);
#endif
        }
    }
    if (shared_str_initialized)
        FlushLog();
#ifndef USE_OWN_ERROR_MESSAGE_WINDOW
#ifdef USE_OWN_MINI_DUMP
    SaveMiniDump(exPtrs);
#endif
#else
    if (!ErrorAfterDialog)
    {
        if (OnDialog)
            OnDialog(true);
        MessageBox(NULL,
                   "Fatal error occurred\n\n"
                   "Press OK to abort program execution",
                   "Fatal error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    }
#endif
    ReportFault(exPtrs, 0);
    if (PrevFilter)
        PrevFilter(exPtrs);
#ifdef USE_OWN_ERROR_MESSAGE_WINDOW
    if (OnDialog)
        OnDialog(false);
#endif
    return EXCEPTION_CONTINUE_SEARCH;
#else
    return 0;
#endif
}

#ifndef USE_BUG_TRAP
void _terminate()
{
#if defined(WINDOWS)
    if (strstr(GetCommandLine(), "-silent_error_mode"))
        exit(-1);
    string4096 assertionInfo;
    xrDebug::GatherInfo(assertionInfo, DEBUG_INFO, nullptr, "Unexpected application termination");
    strcat(assertionInfo, "Press OK to abort execution\r\n");
    MessageBox(GetTopWindow(NULL), assertionInfo, "Fatal Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
#endif
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
#if defined(WINDOWS)
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
static void termination_handler(int signal) { handler_base("termination with exit code 3"); }

void xrDebug::OnThreadSpawn()
{
#if defined(WINDOWS)
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
#endif
}

void xrDebug::Initialize(const bool& dedicated)
{
    *BugReportFile = 0;
    OnThreadSpawn();
    SetupExceptionHandler(dedicated);
    // exception handler to all "unhandled" exceptions
#if defined(WINDOWS)
    PrevFilter = ::SetUnhandledExceptionFilter(UnhandledFilter);
#endif
}
