#pragma once

#define ECORE_API
#define XR_EPROPS_API

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"

#include "xrAICore/AISpaceBase.hpp"

#include <memory>

#include <d3dx9.h>
#include "Common/_d3d_extensions.h"

#include "utils/xrLCUtil/ILevelCompilerLogger.hpp"
#include "utils/xrLCUtil/xrThread.hpp"

#define NUM_THREADS 8

extern ILevelCompilerLogger& Logger;
extern CThread::LogFunc ProxyMsg;
extern CThreadManager::ReportStatusFunc ProxyStatus;
extern CThreadManager::ReportProgressFunc ProxyProgress;

#ifdef AI_COMPILER
#include "xrServerEntities/smart_cast.h"
#endif

// Used in:
// src\xrServerEntities\xrServer_Objects_ALife_Items.cpp
// src\xrServerEntities\xrServer_Objects_ALife_Monsters.cpp
