#pragma once

#define ENGINE_API

#include "Common/Common.hpp"
#include "utils/xrLC_Light/xrLC_Light.h"

#include "utils/xrLCUtil/ILevelCompilerLogger.hpp"
#include "utils/xrLCUtil/xrThread.hpp"

#define NUM_THREADS 8

extern ILevelCompilerLogger& Logger;
extern CThread::LogFunc ProxyMsg;
extern CThreadManager::ReportStatusFunc ProxyStatus;
extern CThreadManager::ReportProgressFunc ProxyProgress;

#include "b_globals.h"
