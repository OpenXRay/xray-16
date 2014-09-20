//----------------------------------------------------
// file: stdafx.h
//----------------------------------------------------
#ifndef __INCDEF_STDAFX_H_
#define __INCDEF_STDAFX_H_

#pragma once

#define _WIN32_WINNT 0x0500        

#pragma warning (disable:4995)
#include "Max.h"

#include "../../../xrCore/xrCore.h"

#undef _MIN
#undef _MAX
#define _MIN(a,b)		(a)<(b)?(a):(b)
#define _MAX(a,b)		(a)>(b)?(a):(b)
template <class T>
T min(T a, T b) { return _MIN(a,b); }
template <class T>
T max(T a, T b) { return _MAX(a,b); }
using std::string;
#undef _MIN
#undef _MAX

#define FLT_MAX flt_max

#ifdef FLT_MIN
#undef FLT_MIN
#endif

#define FLT_MIN flt_max

#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <sys\utime.h>

#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "stdmat.h"
#include "UTILAPI.H"

// CS SDK
#ifdef _MAX_EXPORT
#	include "phyexp.h"
#	include "bipexp.h"
#endif

#include <d3d9types.h>

#define ENGINE_API
#define ECORE_API

enum TMsgDlgType { mtWarning, mtError, mtInformation, mtConfirmation, mtCustom };
enum TMsgDlgBtn { mbYes, mbNo, mbOK, mbCancel, mbAbort, mbRetry, mbIgnore, mbAll, mbNoToAll, mbYesToAll, mbHelp };
typedef TMsgDlgBtn TMsgDlgButtons[mbHelp];

#include <string>

#define AnsiString string
DEFINE_VECTOR(AnsiString,AStringVec,AStringIt);

//#include "clsid.h"
//#include "Engine.h"
//#include "Properties.h"
#include "..\..\Shared\ELog.h"

#define THROW R_ASSERT(0)

#ifdef _MAX_EXPORT
	#define _EDITOR_FILE_NAME_ "max_export"
#else
	#ifdef _MAX_MATERIAL
		#define _EDITOR_FILE_NAME_ "max_material"
	#endif
#endif

#define GAMEMTL_NONE		u32(-1)
#define _game_data_ "$game_data$"

#pragma warning (default:4995)

#endif /*_INCDEF_STDAFX_H_*/





