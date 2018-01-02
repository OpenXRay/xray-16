#pragma once

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <d3dx9.h>
#include <commctrl.h>
#pragma warning(pop)

#define ENGINE_API
#define ECORE_API
#define XR_EPROPS_API
#include "xrCore/clsid.h"
#include "xrCDB/xrCDB.h"
#include "Common/_d3d_extensions.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>

#ifdef AI_COMPILER
#include "smart_cast.h"
#endif

#include "utils/xrLCUtil/ILevelCompilerLogger.hpp"
#include "utils/xrLCUtil/xrThread.hpp"
#include "xrCore/cdecl_cast.hpp"
#include "xrScriptEngine/DebugMacros.hpp" // XXX: move debug macros to xrCore

extern ILevelCompilerLogger& Logger;
extern CThread::LogFunc ProxyMsg;
extern CThreadManager::ReportStatusFunc ProxyStatus;
extern CThreadManager::ReportProgressFunc ProxyProgress;

#define READ_IF_EXISTS(ltx, method, section, name, default_value)\
    (ltx->line_exist(section, name)) ? ltx->method(section, name) : default_value
