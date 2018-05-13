#pragma once

#include "Common/Common.hpp"

#include "utils/xrLCUtil/ILevelCompilerLogger.hpp"
#include "utils/xrLCUtil/xrThread.hpp"

extern ILevelCompilerLogger& Logger;
extern CThread::LogFunc ProxyMsg;
extern CThreadManager::ReportStatusFunc ProxyStatus;
extern CThreadManager::ReportProgressFunc ProxyProgress;
