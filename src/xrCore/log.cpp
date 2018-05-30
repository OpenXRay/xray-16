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
xr_vector<xr_string>* LogFile = nullptr;
LogCallback LogCB = 0;

// alpet: выставить в true если лог все же записывается плохо при вылете.
// Слишком частая запись лога вредит SSD и снижает производительность.
bool force_flush_log = false;
// RvP
IWriter* LogWriter;
size_t cached_log = 0;

void FlushLog()
{
    if (!no_log)
    {
        logCS.Enter();
        if (LogWriter)
            LogWriter->flush();
        cached_log = 0;
        logCS.Leave();
    }
}

void AddOne(const char* split)
{
    if (!LogFile)
        return;

    logCS.Enter();

#ifdef DEBUG
    OutputDebugString(split);
    OutputDebugString("\n");
#endif

    // exec CallBack
    if (LogExecCB && LogCB)
        LogCB(split);

    LogFile->push_back(split);

    if (LogWriter)
    {
        switch (*split)
        {
        case 0x21:
        case 0x23:
        case 0x25:
            split++; // пропустить первый символ, т.к. это вероятно цветовой тег
            break;
        }

        char buf[64];
        SYSTEMTIME lt;
        GetLocalTime(&lt);

        sprintf_s(buf, 64, "[%02d:%02d:%02d.%03d] ", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
        LogWriter->w_printf("%s%s\r\n", buf, split);
        cached_log += xr_strlen(buf);
        cached_log += xr_strlen(split) + 2;

        if (force_flush_log || cached_log >= 32768)
            FlushLog();

        //-RvP
    }

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

void InitLog()
{
    R_ASSERT(LogFile == nullptr);
    LogFile = new xr_vector<xr_string>();
}

void CreateLog(BOOL nl)
{
    no_log = nl;
    strconcat(sizeof(log_file_name), log_file_name, Core.ApplicationName, "_", Core.UserName, ".log");
    if (FS.path_exist("$logs$"))
        FS.update_path(logFName, "$logs$", log_file_name);
    if (!no_log)
    {
        //Alun: Backup existing log
        xr_string backup_logFName = EFS.ChangeFileExt(logFName, ".bkp");
        FS.file_rename(logFName, backup_logFName.c_str(), true);
        //-Alun

        LogWriter = FS.w_open(logFName);
        if (LogWriter == NULL)
        {
#if defined(WINDOWS)
            MessageBox(NULL, "Can't create log file.", "Error", MB_ICONERROR);
#endif
            abort();
        }

        time_t t = time(NULL);
        tm* ti = localtime(&t);
        char buf[64];
        strftime(buf, 64, "[%x %X]\t", ti);

        for (u32 it = 0; it < LogFile->size(); it++)
        {
            LPCSTR s = (*LogFile)[it].c_str();
            LogWriter->w_printf("%s%s\n", buf, s ? s : "");
        }
        LogWriter->flush();
    }

    LogFile->reserve(128);

    if (strstr(Core.Params, "-force_flushlog"))
        force_flush_log = true;
}

void CloseLog(void)
{
    FlushLog();
    if (LogWriter)
        FS.w_close(LogWriter);

    LogFile->clear();
    xr_delete(LogFile);
}
