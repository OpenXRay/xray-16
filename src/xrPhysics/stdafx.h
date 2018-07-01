#pragma once

#include "Common/Common.hpp"

#define MTL_EXPORT_API
#define ECORE_API

#include "xrCore/xrCore.h"

#include "xrServerEntities/smart_cast.h"

#include "xrCDB/xrCDB.h"
#include "xrSound/Sound.h"
#include "xrEngine/GameMtlLib.h"
#include "xrCore/_std_extensions.h"

#include "xrPhysics.h"

#ifdef DEBUG
#include <d3d9types.h>
#endif

// XXX: TODO: What on earth have CODE to do in a PCH header like this?!
// IC IGame_Level &GLevel()
//{
//	VERIFY( g_pGameLevel );
//	return *g_pGameLevel;
//}
class CGameMtlLibrary;
IC CGameMtlLibrary& GMLibrary()
{
    VERIFY(GEnv.PGMLib);
    return *GEnv.PGMLib;
}
