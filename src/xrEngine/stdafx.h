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

#include "xrSound/Sound.h"

extern ENGINE_API CInifile* pGameIni;

#endif // _EDITOR
