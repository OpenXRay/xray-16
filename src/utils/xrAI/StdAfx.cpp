#include "stdafx.h"
#include "xrCore/cdecl_cast.hpp"
#include "utils/xrLCUtil/LevelCompilerLoggerWindow.hpp"

ILevelCompilerLogger& Logger = LevelCompilerLoggerWindow::instance();

CThread::LogFunc ProxyMsg = cdecl_cast([](const char* format, ...) {
    va_list args;
    va_start(args, format);
    Logger.clMsgV(format, args);
    va_end(args);
});

CThreadManager::ReportStatusFunc ProxyStatus = cdecl_cast([](const char* format, ...) {
    va_list args;
    va_start(args, format);
    Logger.StatusV(format, args);
    va_end(args);
});

CThreadManager::ReportProgressFunc ProxyProgress = cdecl_cast([](float progress) { Logger.Progress(progress); });
