#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/HWCaps.h"
#include "dx9HW.h"

#if !defined(_EDITOR)
#include <nvapi.h>
#endif

namespace
{
#if !defined(_EDITOR)
u32 GetNVGpuNum()
{
    NvLogicalGpuHandle logicalGPUs[NVAPI_MAX_LOGICAL_GPUS];
    NvU32 logicalGPUCount;
    NvPhysicalGpuHandle physicalGPUs[NVAPI_MAX_PHYSICAL_GPUS];
    NvU32 physicalGPUCount;

    //	int result = NVAPI_OK;

    NvAPI_Status status;
    status = NvAPI_Initialize();

    if (status != NVAPI_OK)
    {
        Msg("* NVAPI is missing.");
        return 0;
    }

    // enumerate logical gpus
    status = NvAPI_EnumLogicalGPUs(logicalGPUs, &logicalGPUCount);
    if (status != NVAPI_OK)
    {
        Msg("* NvAPI_EnumLogicalGPUs failed!");
        return 0;
    }

    // enumerate physical gpus
    status = NvAPI_EnumPhysicalGPUs(physicalGPUs, &physicalGPUCount);
    if (status != NVAPI_OK)
    {
        Msg("* NvAPI_EnumPhysicalGPUs failed!");
        return 0;
    }

    int iGpuNum = 0;
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
    const auto atimgpud = XRay::LoadModule("ATIMGPUD");
    if (!atimgpud->IsLoaded())
        return 0;

    using ATIQUERYMGPUCOUNT = INT(*)();

    const auto AtiQueryMgpuCount = (ATIQUERYMGPUCOUNT)atimgpud->GetProcAddress("AtiQueryMgpuCount");

    if (!AtiQueryMgpuCount)
        return 0;

    const int iGpuNum = AtiQueryMgpuCount();
    if (iGpuNum > 1)
        Msg("* ATI MGPU: %d-Way CrossFire detected.", iGpuNum);

    return iGpuNum;
}

u32 GetGpuNum()
{
    u32 res = GetNVGpuNum();
    res = _max(res, GetATIGpuNum());
    res = _max(res, 2);
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

static pcstr GetVertexShaderProfile(const D3DCAPS9& caps)
{
    switch (caps.VertexShaderVersion)
    {
#if RENDER != R_R1
    case D3DVS_VERSION(3, 0): return "vs_3_0";

    case D3DVS_VERSION(2, 0):
        if (caps.VS20Caps.NumTemps >= 13                &&
            caps.VS20Caps.DynamicFlowControlDepth == 24 &&
            caps.VS20Caps.Caps & D3DPS20CAPS_PREDICATION)
        {
            return "vs_2_a";
        }
        [[fallthrough]];
#endif
    default:                  return "vs_2_0";

    case D3DVS_VERSION(1, 1): return "vs_1_1";
    }
}

static pcstr GetPixelShaderProfile(const D3DCAPS9& caps)
{
#if RENDER == R_R1
    return "ps_2_0";
#else // R2
    switch (caps.PixelShaderVersion)
    {
    case D3DPS_VERSION(3, 0): return "ps_3_0";

    case D3DPS_VERSION(2, 0):
        if (caps.PS20Caps.NumTemps >= 22                          &&
            caps.PS20Caps.Caps & D3DPS20CAPS_ARBITRARYSWIZZLE     &&
            caps.PS20Caps.Caps & D3DPS20CAPS_GRADIENTINSTRUCTIONS &&
            caps.PS20Caps.Caps & D3DPS20CAPS_PREDICATION          &&
            caps.PS20Caps.Caps & D3DPS20CAPS_NODEPENDENTREADLIMIT &&
            caps.PS20Caps.Caps & D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT)
        {
            return "ps_2_a";
        }
        if (caps.PS20Caps.NumTemps >= 32                          &&
            caps.PS20Caps.Caps&D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT)
        {
            return "ps_2_b";
        }
        [[fallthrough]];

    default:                  return "ps_2_0";

    case D3DPS_VERSION(1, 4): return "ps_1_4";
    case D3DPS_VERSION(1, 3): return "ps_1_3";
    case D3DPS_VERSION(1, 2): return "ps_1_2";
    case D3DPS_VERSION(1, 1): return "ps_1_1";
    }
#endif
}

void CHWCaps::Update()
{
    D3DCAPS9 caps;
    HW.pDevice->GetDeviceCaps(&caps);

    // ***************** GEOMETRY
    geometry_major = u16((u32(caps.VertexShaderVersion) & (0xf << 8ul)) >> 8);
    geometry_minor = u16((u32(caps.VertexShaderVersion) & 0xf));
    geometry_profile = GetVertexShaderProfile(caps);
    geometry.bSoftware = (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0;
    geometry.bPointSprites = FALSE;
    geometry.bNPatches = (caps.DevCaps & D3DDEVCAPS_NPATCHES) != 0;
    u32 cnt = (caps.MaxVertexShaderConst);
    clamp<u32>(cnt, 0, 256);
    geometry.dwRegisters = cnt;
    geometry.dwInstructions = 256;
    geometry.dwClipPlanes = _min(caps.MaxUserClipPlanes, 15);
    geometry.bVTF = (geometry_major >= 3) && HW.support(D3DFMT_R32F, D3DRTYPE_TEXTURE, D3DUSAGE_QUERY_VERTEXTEXTURE);

    // ***************** PIXEL processing
    raster_major = u16(u32(u32(caps.PixelShaderVersion) & u32(0xf << 8ul)) >> 8);
    raster_minor = u16(u32(u32(caps.PixelShaderVersion) & u32(0xf)));
    raster_profile = GetPixelShaderProfile(caps);
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

    // FFP lights
    max_ffp_lights = caps.MaxActiveLights;

    // DEV INFO

    iGPUNum = GetGpuNum();

    hasFixedPipeline    = true;
    useCombinedSamplers = true;
}
