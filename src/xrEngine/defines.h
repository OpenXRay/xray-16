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
    rsAlwaysActive          = (1ul << 0ul),
    rsClearBB               = (1ul << 1ul),
    rsVSync                 = (1ul << 2ul),
    rsWireframe             = (1ul << 3ul),

    rsConstantFPS           = (1ul << 4ul),
    rsDisableObjectsAsCrows = (1ul << 5ul),

    rsStatistic             = (1ul << 6ul),
    rsCameraPos             = (1ul << 7ul),
    rsShowFPS               = (1ul << 8ul),
    rsShowFPSGraph          = (1ul << 9ul),
    rsOcclusionDraw         = (1ul << 10ul),

    rsDrawStatic            = (1ul << 11ul),
    rsDrawDynamic           = (1ul << 12ul),
    rsDrawDetails           = (1ul << 13ul),
    rsDrawParticles         = (1ul << 14ul),

    mtSound                 = (1ul << 15ul),
    mtPhysics               = (1ul << 16ul),
    mtNetwork               = (1ul << 17ul),
    mtParticles             = (1ul << 18ul),

    // 20-32 bit - reserved to Editor
};

enum
{
    rsWindowed,
    rsWindowedBorderless,
    rsFullscreenBorderless, // windowed + topmost + resolution matches desktop resolution + without borders = looks like fullscreen
    rsFullscreen,           // true, exclusive fullscreen
};

ENGINE_API extern u32 psCurrentVidMode[3]; // width, height, refresh rate
ENGINE_API extern u32 psCurrentWindowMode;
ENGINE_API extern u32 psCurrentBPP;
ENGINE_API extern Flags32 psDeviceFlags;

#include "Common/FSMacros.hpp"
