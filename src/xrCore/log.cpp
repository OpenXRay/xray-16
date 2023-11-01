#include "stdafx.h"
#pragma hdrstop

#include <time.h>
#include "resource.h"
#include "log.h"
#include "xrCore/Threading/Lock.hpp"

BOOL LogExecCB = TRUE;
string_path logFName = "engine.log";
string_path log_file_name = "engine.log";
BOOL no_log = TRUE;
#ifdef CONFIG_PROFILE_LOCKS
Lock logCS(MUTEX_PROFILE_ID(log));
#else // CONFIG_PROFILE_LOCKS
Lock logCS;
#endif // CONFIG_PROFILE_LOCKS
xr_vector<xr_string> LogFile;
LogCallback LogCB = nullptr;

bool ForceFlushLog = false;
IWriter* LogWriter = nullptr;
//size_t CachedLog = 0;

void FlushLog()
{
    if (!no_log)
    {
        logCS.Enter();
        if (LogWriter)
            LogWriter->flush();
        //CachedLog = 0;
        logCS.Leave();
    }
}

void AddOne(const char* split)
{
    logCS.Enter();

    OutputDebugString(split);
    OutputDebugString("\n");

    LogFile.push_back(split);

    // exec CallBack
    if (LogExecCB && LogCB)
        LogCB(split);

    if (LogWriter)
    {
#ifdef USE_LOG_TIMING
        char buf[64];
        char curTime[64];

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) -
            std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());

        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&time));
        int len = xr_sprintf(curTime, 64, "[%s.%03lld] ", buf, ms.count());

        LogWriter->w_printf("%s%s\r\n", curTime, split);
        CachedLog += len;
#else
        LogWriter->w_printf("%s\r\n", split);
#endif
        //CachedLog += xr_strlen(split) + 2;

        if (ForceFlushLog /*|| CachedLog >= 32768*/)
            FlushLog();

        //-RvP
    }

    logCS.Leave();
}

void Log(const char* s)
{
    int i, j;

    const u32 length = xr_strlen(s);
    pstr split = static_cast<pstr>(xr_alloca((length + 1) * sizeof(char)));
    for (i = 0, j = 0; s[i] != 0; i++)
    {
        if (s[i] == '\n')
        {
            split[j] = 0; // end of line
            if (split[0] == 0)
            {
                split[0] = ' ';
                split[1] = 0;
            }
            AddOne(split);
            j = 0;
        }
        else
        {
            split[j++] = s[i];
        }
    }
    split[j] = 0;
    AddOne(split);
}

void __cdecl Msg(const char* format, ...)
{
    va_list mark;
    string2048 buf;
    va_start(mark, format);
    const int sz = std::vsnprintf(buf, sizeof(buf) - 1, format, mark);
    buf[sizeof(buf) - 1] = 0;
    va_end(mark);
    if (sz)
        Log(buf);
}

void Log(const char* msg, const char* dop)
{
    if (!dop)
    {
        Log(msg);
        return;
    }

    const u32 buffer_size = (xr_strlen(msg) + 1 + xr_strlen(dop) + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));
    strconcat(buffer_size, buf, msg, " ", dop);
    Log(buf);
}

void Log(const char* msg, u32 dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 1 + 10 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %d", msg, dop);
    Log(buf);
}

void Log(const char* msg, u64 dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %d", msg, dop);
    Log(buf);
}

void Log(const char* msg, int dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 1 + 11 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %i", msg, dop);
    Log(buf);
}

void Log(const char* msg, float dop)
{
    // actually, float string representation should be no more, than 40 characters,
    // but we will count with slight overhead
    const u32 buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %f", msg, dop);
    Log(buf);
}

void Log(const char* msg, const Fvector& dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 2 + 3 * (64 + 1) + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s (%f,%f,%f)", msg, VPUSH(dop));
    Log(buf);
}

void Log(const char* msg, const Fmatrix& dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 2 + 4 * (4 * (64 + 1) + 1) + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s:\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n", msg, dop.i.x, dop.i.y,
        dop.i.z, dop._14_, dop.j.x, dop.j.y, dop.j.z, dop._24_, dop.k.x, dop.k.y, dop.k.z, dop._34_, dop.c.x, dop.c.y,
        dop.c.z, dop._44_);
    Log(buf);
}

void LogWinErr(const char* msg, long err_code) { Msg("%s: %s", msg, xrDebug::ErrorToString(err_code)); }
LogCallback SetLogCB(const LogCallback& cb)
{
    const LogCallback result = LogCB;
    LogCB = cb;
    return (result);
}

LPCSTR log_name() { return (log_file_name); }

void CreateLog(BOOL nl)
{
    LogFile.reserve(1000);

    no_log = nl;
    strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, ".log");
    if (FS.path_exist("$logs$"))
        FS.update_path(logFName, "$logs$", log_file_name);
    if (!no_log)
    {
        // Alun: Backup existing log
        const xr_string backup_logFName = EFS.ChangeFileExt(logFName, ".bkp");
        FS.file_rename(logFName, backup_logFName.c_str(), true);
        //-Alun

        LogWriter = FS.w_open(logFName);
        if (LogWriter == nullptr)
        {
#if defined(XR_PLATFORM_WINDOWS)
            MessageBox(nullptr, "Can't create log file.", "Error", MB_ICONERROR);
#endif
            abort();
        }

#ifdef USE_LOG_TIMING
        time_t t = time(nullptr);
        tm* ti = localtime(&t);
        char buf[64];
        strftime(buf, 64, "[%x %X]\t", ti);
#endif

        for (u32 it = 0; it < LogFile.size(); it++)
        {
            LPCSTR s = LogFile[it].c_str();
#ifdef USE_LOG_TIMING
            LogWriter->w_printf("%s%s\r\n", buf, s ? s : "");
#else
            LogWriter->w_printf("%s\r\n", s ? s : "");
#endif
        }
        LogWriter->flush();
    }

    if (strstr(Core.Params, "-force_flushlog"))
        ForceFlushLog = true;
}

void CloseLog(void)
{
    FlushLog();
    if (LogWriter)
        FS.w_close(LogWriter);

    LogFile.clear();
}
