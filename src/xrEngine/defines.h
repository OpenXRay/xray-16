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
    //rsFullscreen = (1ul << 0ul), // XXX: remove, don't just left commented
    rsClearBB = (1ul << 1ul),
    rsVSync = (1ul << 2ul),
    rsWireframe = (1ul << 3ul),
    rsOcclusion = (1ul << 4ul),
    rsStatistic = (1ul << 5ul),
    rsDetails = (1ul << 6ul),
    rsAlwaysActive = (1ul << 7ul),
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

    rsShowFPS = (1ul << 19ul),
    rsShowFPSGraph = (1ul << 20ul),
    rsDrawParticles = (1ul << 21ul), // XXX: place after rsDrawDynamic
    // 22-32 bit - reserved to Editor
};

enum
{
    rsWindowed,
    rsWindowedBorderless,
    rsFullscreenBorderless, // windowed + topmost + resolution matches desktop resolution + without borders = looks like fullscreen
    rsFullscreen,           // true, exclusive fullscreen
};

//. ENGINE_API extern u32 psCurrentMode ;
ENGINE_API extern u32 psCurrentVidMode[];
ENGINE_API extern u32 psCurrentWindowMode;
ENGINE_API extern u32 psCurrentBPP;
ENGINE_API extern Flags32 psDeviceFlags;

#include "Common/FSMacros.hpp"
