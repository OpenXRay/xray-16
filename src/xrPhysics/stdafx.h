
#pragma once

#define MTL_EXPORT_API
#define ENGINE_API
#define DLL_API
#define ECORE_API

#include "xrCore/xrCore.h"

#include "xrServerEntities/smart_cast.h"

#include "xrCDB/xrCDB.h"
#include "xrSound/sound.h"

#pragma comment(lib, "xrCore.lib")

#include "xrPhysics.h"

#include "Include/xrAPI/xrapi.h"
#ifdef DEBUG
#include "d3d9types.h"
#endif
// IC IGame_Level &GLevel()
//{
//	VERIFY( g_pGameLevel );
//	return *g_pGameLevel;
//}
class CGameMtlLibrary;
IC CGameMtlLibrary& GMLibrary()
{
    VERIFY(GlobalEnv.PGMLib);
    return *GlobalEnv.PGMLib;
}
