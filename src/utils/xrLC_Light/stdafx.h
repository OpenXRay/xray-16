#pragma once

#include "Common/Common.hpp"

//#define COLLECT_EXECUTION_STATS

#pragma warning(disable : 4661)
#include "xrLC_Light.h"

#include "utils/xrLCUtil/ILevelCompilerLogger.hpp"
#include "utils/xrLCUtil/xrThread.hpp"

#define NUM_THREADS 8

extern ILevelCompilerLogger& Logger;
extern CThread::LogFunc ProxyMsg;
extern CThreadManager::ReportStatusFunc ProxyStatus;
extern CThreadManager::ReportProgressFunc ProxyProgress;

#ifdef DEBUG
#define CL_NET_LOG
#endif
