#pragma once
#include "xrCore/_types.h"
#include <string>

struct StackTraceInfo
{
    static const size_t Capacity = 100;
    static const size_t LineCapacity = 256;
    char Frames[Capacity * LineCapacity];
    size_t Count;

    char* operator[](size_t i) { return Frames + i * LineCapacity; }
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

    ErrorLocation& operator=(const ErrorLocation& rhs)
    {
        File = rhs.File;
        Line = rhs.Line;
        Function = rhs.Function;
        return *this;
    }
};

class XRCORE_API xrDebug
{
public:
    using OutOfMemoryCallbackFunc = void (*)();
    using CrashHandler = void (*)();
    using DialogHandler = void (*)(bool);
    using UnhandledExceptionFilter = LONG(WINAPI*)(EXCEPTION_POINTERS* exPtrs);

private:
    static UnhandledExceptionFilter PrevFilter;
    static OutOfMemoryCallbackFunc OutOfMemoryCallback;
    static CrashHandler OnCrash;
    static DialogHandler OnDialog;
    static string_path BugReportFile;
    static bool ErrorAfterDialog;
    static StackTraceInfo StackTrace;

public:
    xrDebug() = delete;
    static void Initialize(const bool& dedicated);
    static void Destroy();
    static void OnThreadSpawn();
    static OutOfMemoryCallbackFunc GetOutOfMemoryCallback() { return OutOfMemoryCallback; }
    static void SetOutOfMemoryCallback(OutOfMemoryCallbackFunc cb) { OutOfMemoryCallback = cb; }
    static CrashHandler GetCrashHandler() { return OnCrash; }
    static void SetCrashHandler(CrashHandler handler) { OnCrash = handler; }
    static DialogHandler GetDialogHandler() { return OnDialog; }
    static void SetDialogHandler(DialogHandler handler) { OnDialog = handler; }
    static const char* ErrorToString(long code);
    static void SetBugReportFile(const char* fileName);
    static void LogStackTrace(const char* header);
    static size_t BuildStackTrace(char* buffer, size_t capacity, size_t lineCapacity);
    static void GatherInfo(char* assertionInfo, const ErrorLocation& loc, const char* expr, const char* desc,
        const char* arg1 = nullptr, const char* arg2 = nullptr);
    static void Fatal(const ErrorLocation& loc, const char* format, ...);
    static void Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, long hresult,
        const char* arg1 = nullptr, const char* arg2 = nullptr);
    static void Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr,
        const char* desc = "assertion failed", const char* arg1 = nullptr, const char* arg2 = nullptr);
    static void Fail(bool& ignoreAlways, const ErrorLocation& loc, const char* expr, const std::string& desc,
        const char* arg1 = nullptr, const char* arg2 = nullptr);
    static void DoExit(const std::string& message);

private:
    static void FormatLastError(char* buffer, const size_t& bufferSize);
    static size_t BuildStackTrace(EXCEPTION_POINTERS* exPtrs, char* buffer, size_t capacity, size_t lineCapacity);
    static void SetupExceptionHandler(const bool& dedicated);
    static LONG WINAPI UnhandledFilter(EXCEPTION_POINTERS* exPtrs);
    static void WINAPI PreErrorHandler(INT_PTR);
    static void SaveMiniDump(EXCEPTION_POINTERS* exPtrs);
};

// for debug purposes only
inline std::string make_string(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    string4096 temp;
    vsprintf(temp, format, args);
    return temp;
}

#include "xrDebug_macros.h"
