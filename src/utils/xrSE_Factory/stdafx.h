////////////////////////////////////////////////////////////////////////////
//	Module 		: stdafx.h
//	Created 	: 18.06.2004
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Precompiled header creator
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/Platform.hpp"
#include "xrCore/xrCore.h"

#define ENGINE_API
#define ECORE_API
#define DLL_API XR_EXPORT
#define TIXML_USE_STL

#include "clsid_game.h"

namespace std
{
class exception;
}
namespace boost
{
void throw_exception(std::exception const& A);
}

#include "smart_cast.h"

#define READ_IF_EXISTS(ltx, method, section, name, default_value) \
    (ltx->line_exist(section, name)) ? ltx->method(section, name) : default_value

#if XRAY_EXCEPTIONS
IC xr_string string2xr_string(LPCSTR s) { return s ? s : ""; }
#endif
