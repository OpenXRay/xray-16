////////////////////////////////////////////////////////////////////////////
//	Module 		: interfaces.h
//	Created 	: 15.06.2005
//  Modified 	: 22.09.2008
//	Author		: Dmitriy Iassenev
//	Description : lua studio backend interfaces
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/Platform.hpp"

#include "xrScriptEngine/LuaStudio/Config.hpp"

#define CS_LUA_STUDIO_BACKEND_CALL __stdcall

#ifndef CS_LUA_STUDIO_BACKEND_API
#define CS_LUA_STUDIO_BACKEND_API XR_IMPORT
#endif

#define CS_LUA_STUDIO_BACKEND_FILE_NAME	CS_LIBRARY_NAME(lua_studio_backend, dll)

#include "xrScriptEngine/LuaStudio/Backend/Backend.hpp"
#include "xrScriptEngine/LuaStudio/Backend/Engine.hpp"
#include "xrScriptEngine/LuaStudio/Backend/World.hpp"
