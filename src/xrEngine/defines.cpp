#include "stdafx.h"

#ifdef DEBUG
ECORE_API bool bDebug = false;

#endif

// Video
DeviceMode psDeviceMode =
{
    /* .Monitor        = */ 0,
    /* .WindowStyle    = */ rsFullscreen,
    /* .Width          = */ 0,
    /* .Height         = */ 0,
    /* .RefreshRate    = */ 0,
    /* .BitsPerPixel   = */ 32
};

// release version always has "mt_*" enabled
Flags32 psDeviceFlags =
{
   rsDrawStatic | rsDrawDynamic | rsDrawDetails | rsDrawParticles | mtSound | mtNetwork
};

// textures
int psTextureLOD = 1;
