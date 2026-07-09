#include "stdafx.h"

#include "StackTrace.h"

#include "Threading/ScopeLock.hpp"
#include <cstdio>
#include <cstdlib>

#ifdef XR_PLATFORM_WINDOWS
#   include <DbgHelp.h>
#elif defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_APPLE) || defined(XR_PLATFORM_BSD)
#   if __has_include(<execinfo.h>)
#       include <execinfo.h>
#       define BACKTRACE_AVAILABLE

#       if __has_include(<elfutils/libdwfl.h>) && __has_include(<cxxabi.h>)
#           include <elfutils/libdwfl.h>
#           include <cxxabi.h>
#           include <dlfcn.h>
#           define CXXABI_AVAILABLE
#       endif
#   endif
#endif

#ifdef XR_PLATFORM_WINDOWS
namespace
{
#   if defined(XR_ARCHITECTURE_X86)
constexpr DWORD MACHINE_TYPE = IMAGE_FILE_MACHINE_I386;
#   elif defined(XR_ARCHITECTURE_X64)
constexpr DWORD MACHINE_TYPE = IMAGE_FILE_MACHINE_AMD64;
#   elif defined(XR_ARCHITECTURE_ARM)
constexpr DWORD MACHINE_TYPE = IMAGE_FILE_MACHINE_ARM;
#   elif defined(XR_ARCHITECTURE_ARM64)
constexpr DWORD MACHINE_TYPE = IMAGE_FILE_MACHINE_ARM64;
#   elif defined(XR_ARCHITECTURE_IA64)
constexpr DWORD MACHINE_TYPE = IMAGE_FILE_MACHINE_IA64;
#   else
#       error CPU architecture is not supported.
#   endif

Lock s_dbghelp_lock;

HMODULE s_dbghelp{};

decltype(&SymInitialize)          symInitialize{};
decltype(&SymCleanup)             symCleanup{};
decltype(&SymGetOptions)          symGetOptions{};
decltype(&SymSetOptions)          symSetOptions{};
decltype(&StackWalk)              stackWalk{};
decltype(&SymFunctionTableAccess) symFunctionTableAccess{};
decltype(&SymGetModuleBase)       symGetModuleBase{};
decltype(&SymGetSymFromAddr)      symGetSymFromAddr{};
decltype(&SymGetLineFromAddr)     symGetLineFromAddr{};

template <typename T>
ICF void load_function(T& func, cpcstr name)
{
    func = reinterpret_cast<T>(GetProcAddress(s_dbghelp, name)); // NOLINT(clang-diagnostic-cast-function-type-strict)

    if (!func)
        Msg("! [StackTraceBuilder] Failed to load %s function", name);
}

#   define STRINGIZE_HELPER(value) #value
#   define STRINGIZE(value) STRINGIZE_HELPER(value)

ICF void init_dbghelp()
{
    s_dbghelp = GetModuleHandleA("dbghelp.dll");
    if (!s_dbghelp)
    {
        Log("! [StackTraceBuilder] Failed to load dbghelp.dll");
        return;
    }

    load_function(symGetOptions, "SymGetOptions");
    load_function(symSetOptions, "SymSetOptions");
    load_function(symInitialize, "SymInitialize");
    load_function(symCleanup,    "SymCleanup");

    load_function(stackWalk, STRINGIZE(StackWalk));
    load_function(symGetModuleBase, STRINGIZE(SymGetModuleBase));
    load_function(symGetSymFromAddr, STRINGIZE(SymGetSymFromAddr));
    load_function(symGetLineFromAddr, STRINGIZE(SymGetLineFromAddr));
    load_function(symFunctionTableAccess, STRINGIZE(SymFunctionTableAccess));
}

#   undef STRINGIZE_HELPER
#   undef STRINGIZE
} // namespace

struct StackTraceBuilder
{
    StackTraceBuilder();
    ~StackTraceBuilder();

    bool GetNextStackFrameString(LPSTACKFRAME stackFrame, PCONTEXT threadCtx, xr_string& frameStr);

    bool IsInitialized{};
};

StackTraceBuilder::StackTraceBuilder()
{
    if (!s_dbghelp)
        init_dbghelp();

    u32 dwOptions = symGetOptions();
    symSetOptions(dwOptions | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    IsInitialized = symInitialize(GetCurrentProcess(), nullptr, TRUE);

    if (!IsInitialized)
    {
        Msg("[StackTraceBuilder] SymInitialize failed with error: 0x%u", GetLastError());
    }
}

StackTraceBuilder::~StackTraceBuilder()
{
    if (IsInitialized)
        symCleanup(GetCurrentProcess());
}

bool StackTraceBuilder::GetNextStackFrameString(LPSTACKFRAME stackFrame, PCONTEXT threadCtx, xr_string& frameStr)
{
    BOOL result = stackWalk(MACHINE_TYPE, GetCurrentProcess(), GetCurrentThread(), stackFrame, threadCtx, nullptr,
        symFunctionTableAccess, symGetModuleBase, nullptr);

    if (result == FALSE || stackFrame->AddrPC.Offset == 0)
    {
        return false;
    }

    frameStr.clear();
    string512 formatBuff;

    ///
    /// Module name
    ///
    HINSTANCE hModule = (HINSTANCE)symGetModuleBase(GetCurrentProcess(), stackFrame->AddrPC.Offset);
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
    u8 arrSymBuffer[512]{};
    PIMAGEHLP_SYMBOL functionInfo = reinterpret_cast<PIMAGEHLP_SYMBOL>(arrSymBuffer);
    functionInfo->SizeOfStruct  = sizeof(*functionInfo);
    functionInfo->MaxNameLength = sizeof(arrSymBuffer) - sizeof(*functionInfo) + 1;
    DWORD_PTR dwFunctionOffset;

    result = symGetSymFromAddr(GetCurrentProcess(), stackFrame->AddrPC.Offset, &dwFunctionOffset, functionInfo);

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

    result = symGetLineFromAddr(GetCurrentProcess(), stackFrame->AddrPC.Offset, &dwLineOffset, &sourceInfo);

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

xr_vector<xr_string> BuildStackTrace(PCONTEXT threadCtx, u16 maxFramesCount)
{
    ScopeLock Lock(&s_dbghelp_lock);

    StackTraceBuilder builder;
    if (!builder.IsInitialized)
        return {};

    xr_vector<xr_string> traceResult;
    xr_string frameStr;

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

    while (builder.GetNextStackFrameString(&stackFrame, threadCtx, frameStr) && traceResult.size() <= maxFramesCount)
    {
        traceResult.emplace_back(std::move(frameStr));
    }

    return traceResult;
}

xr_vector<xr_string> BuildStackTrace(u16 maxFramesCount)
{
    CONTEXT currentThreadCtx = {};

    RtlCaptureContext(&currentThreadCtx); /// GetThreadContext can't be used on the current thread
    currentThreadCtx.ContextFlags = CONTEXT_FULL;

    return BuildStackTrace(&currentThreadCtx, maxFramesCount);
}
#elif defined(BACKTRACE_AVAILABLE)

#ifdef CXXABI_AVAILABLE
struct DebugInfoSession {
    Dwfl_Callbacks callbacks = {};
    char* debuginfo_path = nullptr;
    Dwfl* dwfl = nullptr;

    DebugInfoSession() {
        callbacks.find_elf = dwfl_linux_proc_find_elf;
        callbacks.find_debuginfo = dwfl_standard_find_debuginfo;
        callbacks.debuginfo_path = &debuginfo_path;

        dwfl = dwfl_begin(&callbacks);

        int r;
        r = dwfl_linux_proc_report(dwfl, getpid());
        r = dwfl_report_end(dwfl, nullptr, nullptr);
        static_cast<void>(r);
    }

    ~DebugInfoSession() {
        dwfl_end(dwfl);
    }

    DebugInfoSession(DebugInfoSession const&) = delete;
    DebugInfoSession& operator=(DebugInfoSession const&) = delete;
};

struct DebugInfo {
    void* ip;
    std::string function;
    char const* file;
    int line;

    DebugInfo(DebugInfoSession const& dis, void* ip)
        : ip(ip)
        , file()
        , line(-1)
    {
        // Get function name.
        uintptr_t ip2 = reinterpret_cast<uintptr_t>(ip);
        Dwfl_Module* module = dwfl_addrmodule(dis.dwfl, ip2);
        char const* name = dwfl_module_addrname(module, ip2);

        char* demangledName = abi::__cxa_demangle(name, NULL, NULL, NULL);

        function = demangledName ? demangledName : name ? name : "<unknown>";
        ::free(demangledName);

        // Get source filename and line number.
        if(Dwfl_Line* dwfl_line = dwfl_module_getsrc(module, ip2)) {
            Dwarf_Addr addr;
            file = dwfl_lineinfo(dwfl_line, &addr, &line, nullptr, nullptr, nullptr);
        }
    }
};
#endif

xr_vector<xr_string> BuildStackTrace(u16 maxFramesCount)
{
    xr_vector<xr_string> result;

    void** array = reinterpret_cast<void**>(xr_alloca(sizeof(void*) * maxFramesCount));
    int nptrs = backtrace(array, maxFramesCount); // get void*'s for all entries on the stack
    char** strings = backtrace_symbols(array, nptrs);

    if (strings)
    {
#   ifdef CXXABI_AVAILABLE
        DebugInfoSession dis;
#   endif
        for (int i = 1; i < nptrs; i++) // skip this function
        {
            char* functionName = strings[i];

#   ifdef CXXABI_AVAILABLE
            DebugInfo dinfo = DebugInfo(dis, array[i]);
            char func [2048];
            std::snprintf(func, 2048, ">> \033[31;1m%s\033[0m at \033[32;1;4m%s\033[0m:%d", dinfo.function.data(), dinfo.file, dinfo.line);
            functionName = func;
#   endif
            result.emplace_back(functionName);
        }
    }

    return result;
}
#else
xr_vector<xr_string> BuildStackTrace(u16 maxFramesCount)
{
#   pragma todo("Implement stack trace for this platform")
    return { "Stack trace is not implemented for this platform." };
}
#endif
