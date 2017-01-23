#pragma once
#include "utils/xrLC_Light/xrLC_Light.h"

#define ENGINE_API				// fake, to enable sharing with engine
//comment - ne figa oni ne sharyatsya

#define ECORE_API				// fake, to enable sharing with editors
#define XR_EPROPS_API
#include "xrCore/clsid.h"

#include "utils/xrLCUtil/ILevelCompilerLogger.hpp"
#include "utils/xrLCUtil/xrThread.hpp"
#include "xrCore/cdecl_cast.hpp"

extern ILevelCompilerLogger& Logger;
extern CThread::LogFunc ProxyMsg;
extern CThreadManager::ReportStatusFunc ProxyStatus;
extern CThreadManager::ReportProgressFunc ProxyProgress;

#include "b_globals.h"
