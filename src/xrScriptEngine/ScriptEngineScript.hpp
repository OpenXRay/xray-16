#pragma once
#include "xrCore/xrCore.h"
#include "xrScriptEngine/xrScriptEngine.hpp"

using ScriptTimeGlobalFunc = u32(__cdecl*)();

extern XRSCRIPTENGINE_API ScriptTimeGlobalFunc script_time_global_impl;
extern XRSCRIPTENGINE_API ScriptTimeGlobalFunc script_time_global_async_impl;
