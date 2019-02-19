#include "stdafx.h"
#pragma hdrstop

#include "HWCaps.h"
#include "HW.h"

#if !defined(_EDITOR) && !defined(USE_OGL)
#include <nvapi.h>
#include <ags_lib/inc/amd_ags.h>
#endif

namespace
{
#if !defined(_EDITOR) && !defined(USE_OGL)
u32 GetNVGpuNum()
{
    NvLogicalGpuHandle logicalGPUs[NVAPI_MAX_LOGICAL_GPUS];
    NvU32 logicalGPUCount;
    NvPhysicalGpuHandle physicalGPUs[NVAPI_MAX_PHYSICAL_GPUS];
    NvU32 physicalGPUCount;

    //	int result = NVAPI_OK;

    int iGpuNum = 0;

    NvAPI_Status status;
    status = NvAPI_Initialize();

    if (status != NVAPI_OK)
    {
        Msg("* NVAPI is missing.");
        return iGpuNum;
    }

    // enumerate logical gpus
    status = NvAPI_EnumLogicalGPUs(logicalGPUs, &logicalGPUCount);
    if (status != NVAPI_OK)
    {
        Msg("* NvAPI_EnumLogicalGPUs failed!");
        return iGpuNum;
        // error
    }

    // enumerate physical gpus
    status = NvAPI_EnumPhysicalGPUs(physicalGPUs, &physicalGPUCount);
    if (status != NVAPI_OK)
    {
        Msg("* NvAPI_EnumPhysicalGPUs failed!");
        return iGpuNum;
        // error
    }

    Msg("* NVidia MGPU: Logical(%d), Physical(%d)", physicalGPUCount, logicalGPUCount);

    //	Assume that we are running on logical GPU with most physical GPUs connected.
    for (u32 i = 0; i < logicalGPUCount; ++i)
    {
        status = NvAPI_GetPhysicalGPUsFromLogicalGPU(logicalGPUs[i], physicalGPUs, &physicalGPUCount);
        if (status == NVAPI_OK)
            iGpuNum = _max(iGpuNum, physicalGPUCount);
    }

    if (iGpuNum > 1)
        Msg("* NVidia MGPU: %d-Way SLI detected.", iGpuNum);

    return iGpuNum;
}

u32 GetATIGpuNum()
{
    AGSContext* ags = nullptr;
    AGSGPUInfo gpuInfo = {};
    AGSReturnCode status = agsInit(&ags, &gpuInfo);
    if (status != AGS_SUCCESS)
    {
        Msg("* AGS: Initialization failed (%d)", status);
        return 0;
    }
    int crossfireGpuCount = 1;
    status = agsGetCrossfireGPUCount(ags, &crossfireGpuCount);
    if (status != AGS_SUCCESS)
    {
        Msg("! AGS: Unable to get CrossFire GPU count (%d)", status);
        agsDeInit(ags);
        return 1;
    }
    Msg("* AGS: CrossFire GPU count: %d", crossfireGpuCount);
    agsDeInit(ags);
    return crossfireGpuCount;
}

u32 GetGpuNum()
{
    u32 res = GetNVGpuNum();
    res = _max(res, GetATIGpuNum());
    res = _min(res, CHWCaps::MAX_GPUS);

    if (res == 0)
    {
        Log("! Cannot find graphic adapter. Assuming that you have one...");
        res = 1;
    }

    Msg("* Starting rendering as %d-GPU.", res);

    return res;
}
#else
u32 GetGpuNum() { return 1; }
#endif
}

#ifdef USE_DX9
void CHWCaps::Update()
{
    D3DCAPS9 caps;
    HW.pDevice->GetDeviceCaps(&caps);

    // ***************** GEOMETRY
    geometry_major = u16((u32(caps.VertexShaderVersion) & (0xf << 8ul)) >> 8);
    geometry_minor = u16((u32(caps.VertexShaderVersion) & 0xf));
    geometry.bSoftware = (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0;
    geometry.bPointSprites = FALSE;
    geometry.bNPatches = (caps.DevCaps & D3DDEVCAPS_NPATCHES) != 0;
    DWORD cnt = (caps.MaxVertexShaderConst);
    clamp<DWORD>(cnt, 0, 256);
    geometry.dwRegisters = cnt;
    geometry.dwInstructions = 256;
    geometry.dwClipPlanes = _min(caps.MaxUserClipPlanes, 15);
    geometry.bVTF = (geometry_major >= 3) && HW.support(D3DFMT_R32F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_VERTEXTEXTURE);

    // ***************** PIXEL processing
    raster_major = u16(u32(u32(caps.PixelShaderVersion) & u32(0xf << 8ul)) >> 8);
    raster_minor = u16(u32(u32(caps.PixelShaderVersion) & u32(0xf)));
    raster.dwStages = caps.MaxSimultaneousTextures;
    raster.bNonPow2 = ((caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) != 0) ||
        ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) == 0);
    raster.bCubemap = (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP) != 0;
    raster.dwMRT_count = (caps.NumSimultaneousRTs);
    raster.b_MRT_mixdepth = (caps.PrimitiveMiscCaps & D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS) != 0;
    raster.dwInstructions = (caps.PS20Caps.NumInstructionSlots);

    // ***************** Info
    Msg("* GPU shading: vs(%x/%d.%d/%d), ps(%x/%d.%d/%d)", caps.VertexShaderVersion, geometry_major, geometry_minor,
        CAP_VERSION(geometry_major, geometry_minor), caps.PixelShaderVersion, raster_major, raster_minor,
        CAP_VERSION(raster_major, raster_minor));

    // *******1********** Vertex cache
    ID3DQuery* q_vc;
    D3DDEVINFO_VCACHE vc;
    HRESULT _hr = HW.pDevice->CreateQuery(D3DQUERYTYPE_VCACHE, &q_vc);
    if (FAILED(_hr))
    {
        vc.OptMethod = 0;
        vc.CacheSize = 16;
        geometry.dwVertexCache = 16;
    }
    else
    {
        q_vc->Issue(D3DISSUE_END);
        q_vc->GetData(&vc, sizeof(vc), D3DGETDATA_FLUSH);
        _RELEASE(q_vc);
        if (1 == vc.OptMethod)
            geometry.dwVertexCache = vc.CacheSize;
        else
            geometry.dwVertexCache = 16;
    }
    Msg("* GPU vertex cache: %s, %d", (1 == vc.OptMethod) ? "recognized" : "unrecognized", u32(geometry.dwVertexCache));

    // *******1********** Compatibility : vertex shader
    if (0 == raster_major)
        geometry_major = 0; // Disable VS if no PS
#ifdef _EDITOR
    geometry_major = 0;
#endif

    //
    bTableFog = FALSE; // BOOL	(caps.RasterCaps&D3DPRASTERCAPS_FOGTABLE);

    // Detect if stencil available
    bStencil = FALSE;
    IDirect3DSurface9* surfZS = nullptr;
    D3DSURFACE_DESC surfDESC;
    CHK_DX(HW.pDevice->GetDepthStencilSurface(&surfZS));
    R_ASSERT(surfZS);
    CHK_DX(surfZS->GetDesc(&surfDESC));
    _RELEASE(surfZS);

    switch (surfDESC.Format)
    {
    case D3DFMT_D15S1: bStencil = TRUE; break;
    case D3DFMT_D24S8: bStencil = TRUE; break;
    case D3DFMT_D24X4S4: bStencil = TRUE; break;
    }

    // Scissoring
    if (caps.RasterCaps & D3DPRASTERCAPS_SCISSORTEST)
        bScissor = TRUE;
    else
        bScissor = FALSE;

    // Stencil relative caps
    u32 dwStencilCaps = caps.StencilCaps;
    if ((!(dwStencilCaps & D3DSTENCILCAPS_INCR) && !(dwStencilCaps & D3DSTENCILCAPS_INCRSAT)) ||
        (!(dwStencilCaps & D3DSTENCILCAPS_DECR) && !(dwStencilCaps & D3DSTENCILCAPS_DECRSAT)))
    {
        soDec = soInc = D3DSTENCILOP_KEEP;
        dwMaxStencilValue = 0;
    }
    else
    {
        // Prefer sat ops that cap at 0/max, but can use other ones as long as enough stencil bits
        soInc = (dwStencilCaps & D3DSTENCILCAPS_INCRSAT) ? D3DSTENCILOP_INCRSAT : D3DSTENCILOP_INCR;
        soDec = (dwStencilCaps & D3DSTENCILCAPS_DECRSAT) ? D3DSTENCILOP_DECRSAT : D3DSTENCILOP_DECR;
        dwMaxStencilValue = (1 << 8) - 1;
    }

    // DEV INFO

    iGPUNum = GetGpuNum();
}
#else // USE_DX9
void CHWCaps::Update()
{
    // ***************** GEOMETRY
    geometry_major = 4;
    geometry_minor = 0;
    geometry.bSoftware = FALSE;
    geometry.bPointSprites = FALSE;
    geometry.bNPatches = FALSE;
    DWORD cnt = 256;
    clamp<DWORD>(cnt, 0, 256);
    geometry.dwRegisters = cnt;
    geometry.dwInstructions = 256;
    geometry.dwClipPlanes = _min(6, 15);
#ifdef USE_OGL
    // XXX: Disabled by default. Need to:
    // FIX: Sky texture filtering (point filter now) when VTF is on
    // TODO: Implement support VTF and: HW.support(D3DFMT_R32F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_VERTEXTEXTURE)
    geometry.bVTF = (strstr(Core.Params, "-vtf")) ? TRUE : FALSE;
#else // USE_OGL
    geometry.bVTF = TRUE;
#endif // USE_OGL

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

#ifndef USE_OGL
    // ***************** Info
    Msg("* GPU shading: vs(%x/%d.%d/%d), ps(%x/%d.%d/%d)", 0, geometry_major, geometry_minor,
        CAP_VERSION(geometry_major, geometry_minor), 0, raster_major, raster_minor,
        CAP_VERSION(raster_major, raster_minor));
    // *******1********** Vertex cache
    Msg("* GPU vertex cache: %s, %d", "unrecognized", u32(geometry.dwVertexCache));
#endif // USE_OGL
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
}
#endif // USE_DX9
