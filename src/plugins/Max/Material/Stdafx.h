#pragma once

#include <algorithm>

#include "Common/Common.hpp"
#include "Common/FSMacros.hpp"

#pragma warning(push)
#pragma warning(disable : 4995)

using std::min;
using std::max;

#include "Max.h"

#include "xrCore/xrCore.h"

#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utime.h>

#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "stdmat.h"
#include "UTILAPI.H"

// CS SDK
#ifdef _MAX_EXPORT
#include "phyexp.h"
#include "bipexp.h"
#endif

#include <d3d9types.h>

#define ENGINE_API
#define ECORE_API

enum TMsgDlgType
{
    mtWarning,
    mtError,
    mtInformation,
    mtConfirmation,
    mtCustom
};
enum TMsgDlgBtn
{
    mbYes,
    mbNo,
    mbOK,
    mbCancel,
    mbAbort,
    mbRetry,
    mbIgnore,
    mbAll,
    mbNoToAll,
    mbYesToAll,
    mbHelp
};
typedef TMsgDlgBtn TMsgDlgButtons[mbHelp];

#include <string>

#define AnsiString string
using AStringVec = xr_vector<std::string>;
using std::string;

#include "plugins/Shared/ELog.h"

#define THROW R_ASSERT(0)

#ifdef _MAX_EXPORT
#define _EDITOR_FILE_NAME_ "max_export"
#else
#ifdef _MAX_MATERIAL
#define _EDITOR_FILE_NAME_ "max_material"
#endif
#endif

#define GAMEMTL_NONE u32(-1)

#pragma warning(pop)
