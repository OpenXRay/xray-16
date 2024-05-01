#include "stdafx.h"
#pragma hdrstop

#include <time.h>
#include "resource.h"
#include "log.h"
#include "xrCore/Threading/Lock.hpp"

bool LogExecCB = true;
string_path logFName = "engine.log";
string_path log_file_name = "engine.log";
bool no_log = true;
#ifdef CONFIG_PROFILE_LOCKS
Lock logCS(MUTEX_PROFILE_ID(log));
#else // CONFIG_PROFILE_LOCKS
Lock logCS;
#endif // CONFIG_PROFILE_LOCKS
xr_vector<xr_string> LogFile;
LogCallback LogCB = nullptr;

bool ForceFlushLog = false;
IWriter* LogWriter = nullptr;

void FlushLog()
{
    if (!no_log)
    {
        logCS.Enter();
        if (LogWriter)
            LogWriter->flush();
        logCS.Leave();
    }
}

void AddOne(pcstr split)
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
        LogWriter->w_printf("%s\r\n", split);

        if (ForceFlushLog)
            FlushLog();
    }

    logCS.Leave();
}

void Log(pcstr s)
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

void __cdecl Msg(pcstr format, ...)
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

void Log(pcstr msg, pcstr dop)
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

void Log(pcstr msg, int dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 1 + 11 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %i", msg, dop);
    Log(buf);
}

void Log(pcstr msg, unsigned int dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 1 + 10 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %u", msg, dop);
    Log(buf);
}

void Log(pcstr msg, long dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %li", msg, dop);
    Log(buf);
}

void Log(pcstr msg, unsigned long dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %lu", msg, dop);
    Log(buf);
}

void Log(pcstr msg, long long dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %lli", msg, dop);
    Log(buf);
}

void Log(pcstr msg, unsigned long long dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %llu", msg, dop);
    Log(buf);
}

void Log(pcstr msg, float dop)
{
    // actually, float string representation should be no more, than 40 characters,
    // but we will count with slight overhead
    const u32 buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s %f", msg, dop);
    Log(buf);
}

void Log(pcstr msg, const Fvector& dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 2 + 3 * (64 + 1) + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s (%f,%f,%f)", msg, VPUSH(dop));
    Log(buf);
}

void Log(pcstr msg, const Fmatrix& dop)
{
    const u32 buffer_size = (xr_strlen(msg) + 2 + 4 * (4 * (64 + 1) + 1) + 1) * sizeof(char);
    pstr buf = static_cast<pstr>(xr_alloca(buffer_size));

    xr_sprintf(buf, buffer_size, "%s:\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n", msg, dop.i.x, dop.i.y,
        dop.i.z, dop._14_, dop.j.x, dop.j.y, dop.j.z, dop._24_, dop.k.x, dop.k.y, dop.k.z, dop._34_, dop.c.x, dop.c.y,
        dop.c.z, dop._44_);
    Log(buf);
}

void LogWinErr(pcstr msg, long err_code) { Msg("%s: %s", msg, xrDebug::ErrorToString(err_code)); }
LogCallback SetLogCB(const LogCallback& cb)
{
    const LogCallback result = LogCB;
    LogCB = cb;
    return (result);
}

pcstr log_name() { return (log_file_name); }

void CreateLog(bool nl)
{
    ZoneScoped;
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

        for (u32 it = 0; it < LogFile.size(); it++)
        {
            pcstr s = LogFile[it].c_str();
            LogWriter->w_printf("%s\r\n", s ? s : "");
        }
        LogWriter->flush();
    }

    if (strstr(Core.Params, "-force_flushlog"))
        ForceFlushLog = true;
}

void CloseLog(void)
{
    ZoneScoped;
    FlushLog();
    if (LogWriter)
        FS.w_close(LogWriter);

    LogFile.clear();
}
