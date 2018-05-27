#include "stdafx.h"
#pragma hdrstop

#include <time.h>
#include "resource.h"
#include "log.h"
#include "xrCore/Threading/Lock.hpp"
#ifdef _EDITOR
#include "malloc.h"
#endif

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
LogCallback LogCB = 0;

void FlushLog()
{
    if (!no_log)
    {
        logCS.Enter();
        IWriter* f = FS.w_open(logFName);
        if (f)
        {
            for (const auto &i : LogFile)
            {
                LPCSTR s = i.c_str();
                f->w_string(s ? s : "");
            }
            FS.w_close(f);
        }
        logCS.Leave();
    }
}

void AddOne(const char* split)
{
    logCS.Enter();

#ifdef DEBUG
    OutputDebugString(split);
    OutputDebugString("\n");
#endif

    LogFile.push_back(split);

    // exec CallBack
    if (LogExecCB && LogCB)
        LogCB(split);

    logCS.Leave();
}

void Log(const char* s)
{
    int i, j;

    u32 length = xr_strlen(s);
#ifndef _EDITOR
    PSTR split = (PSTR)_alloca((length + 1) * sizeof(char));
#else
    PSTR split = (PSTR)alloca((length + 1) * sizeof(char));
#endif
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
    int sz = std::vsnprintf(buf, sizeof(buf) - 1, format, mark);
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

    u32 buffer_size = (xr_strlen(msg) + 1 + xr_strlen(dop) + 1) * sizeof(char);
    PSTR buf = (PSTR)_alloca(buffer_size);
    strconcat(buffer_size, buf, msg, " ", dop);
    Log(buf);
}

void Log(const char* msg, u32 dop)
{
    u32 buffer_size = (xr_strlen(msg) + 1 + 10 + 1) * sizeof(char);
    PSTR buf = (PSTR)_alloca(buffer_size);

    xr_sprintf(buf, buffer_size, "%s %d", msg, dop);
    Log(buf);
}

void Log(const char* msg, u64 dop)
{
    u32 buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
    PSTR buf = (PSTR)_alloca(buffer_size);

    xr_sprintf(buf, buffer_size, "%s %d", msg, dop);
    Log(buf);
}

void Log(const char* msg, int dop)
{
    u32 buffer_size = (xr_strlen(msg) + 1 + 11 + 1) * sizeof(char);
    PSTR buf = (PSTR)_alloca(buffer_size);

    xr_sprintf(buf, buffer_size, "%s %i", msg, dop);
    Log(buf);
}

void Log(const char* msg, float dop)
{
    // actually, float string representation should be no more, than 40 characters,
    // but we will count with slight overhead
    u32 buffer_size = (xr_strlen(msg) + 1 + 64 + 1) * sizeof(char);
    PSTR buf = (PSTR)_alloca(buffer_size);

    xr_sprintf(buf, buffer_size, "%s %f", msg, dop);
    Log(buf);
}

void Log(const char* msg, const Fvector& dop)
{
    u32 buffer_size = (xr_strlen(msg) + 2 + 3 * (64 + 1) + 1) * sizeof(char);
    PSTR buf = (PSTR)_alloca(buffer_size);

    xr_sprintf(buf, buffer_size, "%s (%f,%f,%f)", msg, VPUSH(dop));
    Log(buf);
}

void Log(const char* msg, const Fmatrix& dop)
{
    u32 buffer_size = (xr_strlen(msg) + 2 + 4 * (4 * (64 + 1) + 1) + 1) * sizeof(char);
    PSTR buf = (PSTR)_alloca(buffer_size);

    xr_sprintf(buf, buffer_size, "%s:\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n", msg, dop.i.x, dop.i.y,
        dop.i.z, dop._14_, dop.j.x, dop.j.y, dop.j.z, dop._24_, dop.k.x, dop.k.y, dop.k.z, dop._34_, dop.c.x, dop.c.y,
        dop.c.z, dop._44_);
    Log(buf);
}

void LogWinErr(const char* msg, long err_code) { Msg("%s: %s", msg, xrDebug::ErrorToString(err_code)); }
LogCallback SetLogCB(const LogCallback& cb)
{
    LogCallback result = LogCB;
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
        IWriter* f = FS.w_open(logFName);
        if (f == NULL)
        {
#if defined(WINDOWS)
            MessageBox(NULL, "Can't create log file.", "Error", MB_ICONERROR);
#endif
            abort();
        }
        FS.w_close(f);
    }
}

void CloseLog(void)
{
    FlushLog();
    LogFile.clear();
}
