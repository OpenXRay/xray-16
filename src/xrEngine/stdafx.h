#pragma once

#include "Common/Common.hpp"

#ifdef _EDITOR
#include "editors/ECore/stdafx.h"
#else

#if 1//ndef NDEBUG
#ifndef INGAME_EDITOR
#define INGAME_EDITOR
#endif // #ifndef INGAME_EDITOR
#endif // #ifndef NDEBUG

#include "xrCore/xrCore.h"
#include "xrCore/_std_extensions.h"

#define ECORE_API

// Our headers
#include "Engine.h"
#include "defines.h"
#ifndef NO_XRLOG
#include "xrCore/log.h"
#endif
#include "device.h"
#include "xrCore/FS.h"

#include "xrCDB/xrXRC.h"

#include "xrSound/sound.h"

extern ENGINE_API CInifile* pGameIni;

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
// XXX: move to script engine headers
#ifndef DEBUG
#define LUABIND_NO_ERROR_CHECKING
#endif
#define LUABIND_DONT_COPY_STRINGS

#define READ_IF_EXISTS(ltx, method, section, name, default_value) \
    (((ltx)->line_exist(section, name)) ? ((ltx)->method(section, name)) : (default_value))

#endif // _EDITOR
