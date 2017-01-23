// stdafx.cpp : source file that includes just the standard includes
// xrLC_Light.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "utils/xrLCUtil/LevelCompilerLoggerConsole.hpp"

ILevelCompilerLogger& Logger = LevelCompilerLoggerConsole();

CThread::LogFunc ProxyMsg = cdecl_cast(
    [](const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        Logger.clMsgV(format, args);
        va_end(args);    
    }
);

CThreadManager::ReportStatusFunc ProxyStatus = cdecl_cast(
    [](const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        Logger.StatusV(format, args);
        va_end(args);    
    }
);

CThreadManager::ReportProgressFunc ProxyProgress = cdecl_cast(
    [](float progress)
    { Logger.Progress(progress); }
);
