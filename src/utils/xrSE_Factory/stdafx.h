////////////////////////////////////////////////////////////////////////////
//	Module 		: stdafx.h
//	Created 	: 18.06.2004
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Precompiled header creator
////////////////////////////////////////////////////////////////////////////

#pragma once

#define ENGINE_API
#define ECORE_API
#define XRSCRIPTENGINE_EXPORTS

#include "Common/Common.hpp"
#include "xrCore/xrCore.h"
#include "xrCore/_fbox.h"
#include "xrCore/_quaternion.h"
#include "xrScriptEngine/DebugMacros.hpp" // XXX: move debug macros to xrCore
#include "xrServerEntities/smart_cast.h"

#define READ_IF_EXISTS(ltx, method, section, name, default_value)\
    (ltx->line_exist(section, name)) ? ltx->method(section, name) : default_value
