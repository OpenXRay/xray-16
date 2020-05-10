#include "stdafx.h"

#ifdef DEBUG
ECORE_API bool bDebug = false;

#endif

// Video
//. u32 psCurrentMode = 1024;
u32 psCurrentVidMode[2] = {1024, 768};
u32 psCurrentBPP = 32;
// release version always has "mt_*" enabled
Flags32 psDeviceFlags = {
    rsFullscreen | rsDetails | mtPhysics | mtSound | mtNetwork | rsDrawStatic | rsDrawDynamic | rsDrawParticles | rsRefresh60hz};

// textures
int psTextureLOD = 1;
