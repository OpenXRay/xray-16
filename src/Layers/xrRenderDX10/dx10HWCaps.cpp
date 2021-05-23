#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/HWCaps.h"
#include "dx10HW.h"

#if !defined(_EDITOR)
#include <nvapi.h>
#include <ags_lib/inc/amd_ags.h>
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
    constexpr int AGS_VERSION = AGS_MAKE_VERSION(AMD_AGS_VERSION_MAJOR, AMD_AGS_VERSION_MINOR, AMD_AGS_VERSION_PATCH);

    AGSConfiguration config
    {
        /*.allocCallback =*/ [](size_t size) -> void* { return xr_malloc(size); },
        /*.freeCallback  =*/ [](void* ptr)   -> void  { xr_free(ptr); }
    };
    AGSContext* ags{};
    AGSGPUInfo gpuInfo{};

    auto status = agsInitialize(AGS_VERSION, &config, &ags, &gpuInfo);
    if (status != AGS_SUCCESS)
        return 1;

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    AGSDX11DeviceCreationParams creationParams
    {
        /*.pAdapter       =*/ nullptr,
        /*.DriverType     =*/ D3D_DRIVER_TYPE_UNKNOWN,
        /*.Software       =*/ nullptr,
        /*.Flags          =*/ 0,
        /*.pFeatureLevels =*/ featureLevels,
        /*.FeatureLevels  =*/ std::size(featureLevels),
        /*.SDKVersion     =*/ D3D11_SDK_VERSION,
    };

    AGSDX11ExtensionParams extensionParams{ L"OpenXRay", L"OpenXRay Engine", 1602, 1602 };
    extensionParams.crossfireMode = AGS_CROSSFIRE_MODE_EXPLICIT_AFR; // don't mess drivers

    AGSDX11ReturnedParams returnedParams{};
    status = agsDriverExtensionsDX11_CreateDevice(ags, &creationParams, &extensionParams, &returnedParams);
    if (status != AGS_SUCCESS)
    {
        Msg("! AMD AGS: Unable to get CrossFire GPU count (%d)", status);
        agsDeInitialize(ags);
        return 1;
    }

    const u32 crossfireGpuCount = returnedParams.crossfireGPUCount;
    agsDriverExtensionsDX11_DestroyDevice(ags, returnedParams.pDevice, nullptr, returnedParams.pImmediateContext, nullptr);
    
    Msg("* AMD AGS: %d-Way CrossFire detected.", crossfireGpuCount);
    agsDeInitialize(ags);
    return crossfireGpuCount;
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
    geometry.bVTF = TRUE;

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

    // ***************** Info
    Msg("* GPU shading: vs(%x/%d.%d/%d), ps(%x/%d.%d/%d)", 0, geometry_major, geometry_minor,
        CAP_VERSION(geometry_major, geometry_minor), 0, raster_major, raster_minor,
        CAP_VERSION(raster_major, raster_minor));
    // *******1********** Vertex cache
    Msg("* GPU vertex cache: %s, %d", "unrecognized", u32(geometry.dwVertexCache));

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
