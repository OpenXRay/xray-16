#include "stdafx.h"

#ifdef DEBUG
ECORE_API bool bDebug = false;

#endif

// Video
u32 psCurrentVidMode[2] = { 0u, 0u };
u32 psCurrentWindowMode = rsFullscreen;
u32 psCurrentBPP = 32;
// release version always has "mt_*" enabled
Flags32 psDeviceFlags = {
    rsDetails | mtPhysics | mtSound | mtNetwork | rsDrawStatic | rsDrawDynamic | rsDrawParticles };

// textures
int psTextureLOD = 1;
