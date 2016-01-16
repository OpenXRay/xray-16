#ifndef STDAFX_3DA
#define STDAFX_3DA
#pragma once

#ifdef _EDITOR
#include "editors/ECore/stdafx.h"
#else

#ifndef NDEBUG
# ifndef INGAME_EDITOR
# define INGAME_EDITOR
# endif // #ifndef INGAME_EDITOR
#endif // #ifndef NDEBUG

#ifdef INGAME_EDITOR
# define _WIN32_WINNT 0x0550
#endif // #ifdef INGAME_EDITOR

#include "xrCore/xrCore.h"
#include "Include/xrAPI/xrAPI.h"

#define ECORE_API

// Our headers
#include "Engine.h"
#include "defines.h"
#ifndef NO_XRLOG
#include "xrCore/log.h"
#endif
#include "device.h"
#include "xrCore/fs.h"

#include "xrCDB/xrXRC.h"

#include "xrSound/sound.h"

extern ENGINE_API CInifile* pGameIni;

#pragma comment( lib, "xrCore.lib" )
#pragma comment( lib, "xrCDB.lib" )
#pragma comment( lib, "xrSound.lib" )
#pragma comment(lib, "xrScriptEngine.lib")
#pragma comment( lib, "xrAPI.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "dinput8.lib" )
#pragma comment( lib, "dxguid.lib" )
// XXX: move to script engine headers
#ifndef DEBUG
# define LUABIND_NO_ERROR_CHECKING
#endif
#define LUABIND_DONT_COPY_STRINGS

#define READ_IF_EXISTS(ltx,method,section,name,default_value)\
 (((ltx)->line_exist(section, name)) ? ((ltx)->method(section, name)) : (default_value))

#endif // !M_BORLAND
#endif // !defined STDAFX_3DA
