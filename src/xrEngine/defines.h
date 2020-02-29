#pragma once

#ifdef DEBUG
ENGINE_API extern bool bDebug;
#else
#define bDebug 0
#endif

extern ENGINE_API bool CallOfPripyatMode;
extern ENGINE_API bool ClearSkyMode;
extern ENGINE_API bool ShadowOfChernobylMode;

// textures
ENGINE_API extern int psTextureLOD;

// psDeviceFlags
enum
{
    rsFullscreen = (1ul << 0ul),
    rsClearBB = (1ul << 1ul),
    rsVSync = (1ul << 2ul),
    rsWireframe = (1ul << 3ul),
    rsOcclusion = (1ul << 4ul),
    rsStatistic = (1ul << 5ul),
    rsDetails = (1ul << 6ul),
    rsRefresh60hz = (1ul << 7ul),
    rsConstantFPS = (1ul << 8ul),
    rsDrawStatic = (1ul << 9ul),
    rsDrawDynamic = (1ul << 10ul),
    rsDisableObjectsAsCrows = (1ul << 11ul),

    rsOcclusionDraw = (1ul << 12ul),
    rsOcclusionStats = (1ul << 13ul),

    mtSound = (1ul << 14ul),
    mtPhysics = (1ul << 15ul),
    mtNetwork = (1ul << 16ul),
    mtParticles = (1ul << 17ul),

    rsCameraPos = (1ul << 18ul),

    rsR1 = (1ul << 19ul),
    rsR2 = (1ul << 20ul),
    rsR3 = (1ul << 21ul),
    rsR4 = (1ul << 22ul), // 22 was reserved for editor
    rsRGL = (1ul << 23ul), // 23 was reserved for editor

    rsShowFPS = (1ul << 24ul), // 24 was reserved for editor
    rsDrawParticles = (1ul << 25ul), // 25 was reserved for editor
    // 26-32 bit - reserved to Editor
};

//. ENGINE_API extern u32 psCurrentMode ;
ENGINE_API extern u32 psCurrentVidMode[];
ENGINE_API extern u32 psCurrentBPP;
ENGINE_API extern Flags32 psDeviceFlags;

#include "Common/FSMacros.hpp"
