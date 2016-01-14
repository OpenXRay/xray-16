#pragma once
#include "xrCore/_types.h"

struct StackTraceInfo
{
    static const int Capacity = 100;
    static const int LineCapacity = 256;
    char Frames[Capacity*LineCapacity];
    size_t Count;

    char *operator[](size_t i) { return Frames+i*LineCapacity; }
};

class XRCORE_API xrDebug
{
public:
    using OutOfMemoryCallbackFunc = void (*)();
    using CrashHandler = void (*)();
    using DialogHandler = void (*)(bool);
    using UnhandledExceptionFilter = LONG (WINAPI *)(EXCEPTION_POINTERS *exPtrs);

private:
    static UnhandledExceptionFilter PrevFilter;
    static OutOfMemoryCallbackFunc OutOfMemoryCallback;
    static CrashHandler OnCrash;
    static DialogHandler OnDialog;
    static string_path BugReportFile;
    static bool	ErrorAfterDialog;
    static StackTraceInfo StackTrace;
    
public:
    xrDebug() = delete;
	static void Initialize(const bool &dedicated);
    static void Destroy();
    static void OnThreadSpawn();
    static OutOfMemoryCallbackFunc GetOutOfMemoryCallback() { return OutOfMemoryCallback; }
    static void SetOutOfMemoryCallback(OutOfMemoryCallbackFunc cb) { OutOfMemoryCallback = cb; }
    static CrashHandler GetCrashHandler() { return OnCrash; }
    static void SetCrashHandler(CrashHandler handler) { OnCrash = handler; }
    static DialogHandler GetDialogHandler() { return OnDialog; }
    static void SetDialogHandler(DialogHandler handler) { OnDialog = handler; }
    static const char *ErrorToString(long code);
    static void SetBugReportFile(const char *fileName);
    static void LogStackTrace(const char *header);
    static size_t BuildStackTrace(char *buffer, size_t capacity, size_t lineCapacity);
    static void GatherInfo(const char *expression, const char *description, const char *arg0, const char *arg1,
        const char *file, int line, const char *function, char *assertionInfo);
    static void Fail(const char *e1, const char *file, int line, const char *function, bool &ignoreAlways);
    static void Fail(const char *e1, const std::string &e2, const char *file, int line, const char *function,
        bool &ignoreAlways);
    static void Fail(const char *e1, const char *e2, const char *file, int line, const char *function,
        bool &ignoreAlways);
    static void Fail(const char *e1, const char *e2, const char *e3, const char *file, int line, const char *function,
        bool &ignoreAlways);
    static void Fail(const char *e1, const char *e2, const char *e3, const char *e4, const char *file, int line,
        const char *function, bool &ignoreAlways);
    static void Error(long code, const char *e1, const char *file, int line, const char *function,
        bool &ignoreAlways);
    static void Error(long code, const char *e1, const char *e2, const char *file, int line, const char *function,
        bool &ignoreAlways);
    static void Fatal(const char *file, int line, const char *function, const char *format, ...);
    static void Backend(const char *reason, const char *expression, const char *arg0, const char *arg1,
        const char *file, int line, const char *function, bool &ignoreAlways);
    static void DoExit(const std::string& message);

private:
    static void FormatLastError(char *buffer, const size_t &bufferSize);
    static size_t BuildStackTrace(EXCEPTION_POINTERS *exPtrs, char *buffer, size_t capacity, size_t lineCapacity);
    static void SetupExceptionHandler(const bool &dedicated);
    static LONG WINAPI UnhandledFilter(EXCEPTION_POINTERS *exPtrs);
    static void WINAPI PreErrorHandler(INT_PTR);
    static void SaveMiniDump(EXCEPTION_POINTERS *exPtrs);
};

// for debug purposes only
inline std::string make_string(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    string4096 temp;
    vsprintf(temp, format, args);
    return std::string(temp);
}

#include "xrDebug_macros.h"
