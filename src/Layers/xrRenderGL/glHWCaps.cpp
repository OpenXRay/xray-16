#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/HWCaps.h"
#include "glHW.h"

namespace
{
u32 GetGpuNum()
{
    return 2;
}
}

void CHWCaps::Update()
{
    // ***************** GEOMETRY
    geometry_major = 4;
    geometry_minor = 0;
    geometry.bSoftware = FALSE;
    geometry.bPointSprites = FALSE;
    geometry.bNPatches = FALSE;
    u32 cnt = 256;
    clamp<u32>(cnt, 0, 256);
    geometry.dwRegisters = cnt;
    geometry.dwInstructions = 256;
    geometry.dwClipPlanes = _min(6, 15);
    geometry.bVTF =
        (HW.OpenGLVersion >= std::make_pair(3, 0) || GLEW_ARB_texture_float)
        && !strstr(Core.Params, "-novtf");

    // ***************** PIXEL processing
    raster_major = 4;
    raster_minor = 0;
    // XXX: review this
    raster.dwStages = 15; // Previuos value is 16, but it's out of bounds
    raster.bNonPow2 = TRUE;
    raster.bCubemap = TRUE;
    raster.dwMRT_count = 4;
    // raster.b_MRT_mixdepth		= FALSE;
    raster.b_MRT_mixdepth = TRUE;
    raster.dwInstructions = 256;
    //	TODO: DX10: Find a way to detect cache size
    geometry.dwVertexCache = 24;

    // *******1********** Compatibility : vertex shader
    if (0 == raster_major)
        geometry_major = 0; // Disable VS if no PS

    //
    bTableFog = FALSE; // BOOL	(caps.RasterCaps&D3DPRASTERCAPS_FOGTABLE);

    // Detect if stencil available
    bStencil = TRUE;

    // Scissoring
    bScissor = TRUE;

    // Stencil relative caps
    soInc = D3DSTENCILOP_INCRSAT;
    soDec = D3DSTENCILOP_DECRSAT;
    dwMaxStencilValue = (1 << 8) - 1;

    // DEV INFO

    iGPUNum = GetGpuNum();

    useCombinedSamplers = true;
}
