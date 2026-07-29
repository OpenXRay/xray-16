#include "stdafx.h"
#include "IBLPrefilterPassSetup.h"
#include "PassCommon.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/Environment.h"
#include "xrEngine/device.h"

extern ENGINE_API int ps_r_sky_ibl;
extern ENGINE_API int ps_r_ibl_prefilter;
extern ENGINE_API int ps_r_ibl_probe_auto;
extern ENGINE_API float ps_r_ibl_probe_radius;
extern ENGINE_API int ps_r_ssgi;
extern ENGINE_API float ps_r_ssgi_intensity;

namespace xray::render::fg::passes
{
using namespace framegraph;

namespace
{
IBLBindResources g_CurrentIBL;
IBLPrefilterPassState* g_ActiveIBLState = nullptr;
}

void SetCurrentIBLBindResources(const IBLBindResources& ibl)
{
    g_CurrentIBL = ibl;
}

const IBLBindResources& GetCurrentIBLBindResources()
{
    return g_CurrentIBL;
}

IBLPrefilterPassState* GetActiveIBLState()
{
    return g_ActiveIBLState;
}

namespace
{
u32 CountMips(u32 size)
{
    u32 mips = 1;
    while (size > 1)
    {
        size >>= 1;
        ++mips;
    }
    return mips;
}

nvrhi::ComputePipelineHandle LoadCS(
    const char* passName, const char* shaderName, nvrhi::IDevice* device, nvrhi::BindingLayoutHandle& outLayout)
{
    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
        return nullptr;
    auto cs = loader->LoadComputeShader(shaderName);
    if (!cs.handle || !cs.reflection)
    {
        Msg("! [IBL] Failed to load %s", shaderName);
        return nullptr;
    }
    auto& cache = GetPassResourceCache();
    outLayout = cache.GetOrCreateBindingLayoutFromReflection(passName, *cs.reflection, device);
    if (!outLayout)
        return nullptr;
    nvrhi::ComputePipelineDesc desc;
    desc.CS = cs.handle;
    desc.bindingLayouts = {outLayout};
    return cache.GetOrCreateComputePipeline(passName, desc, device);
}

struct PrefilterCB
{
    u32 size[2];
    u32 face;
    u32 sampleCount;
    float roughness;
    float mipBias;
    float pad[2];
};

struct SHProjectCB
{
    u32 size;
    u32 outOffset;
    u32 pad[2];
};

struct BRDFLUTCB
{
    u32 dims[2];
    u32 pad[2];
};

struct ProbeCaptureCB
{
    Fmatrix vp;
    Fmatrix invVP;
    Fvector probePos;
    float skyLerp;
    Fvector eyePos;
    float maxDistance;
    u32 size[2];
    u32 face;
    u32 pad;
};
} // namespace

void InitializeIBLPrefilter(fg::RenderDevice* device, IBLPrefilterPassState& state)
{
    nvrhi::IDevice* nv = device ? device->GetNVRHIDevice() : nullptr;
    if (!nv)
        return;

    if (!state.pipelinesReady)
    {
        state.prefilterPipe = LoadCS("IBLPrefilter", "ibl_prefilter_specular", nv, state.prefilterLayout);
        state.shPipe = LoadCS("IBLSH", "ibl_sh_project", nv, state.shLayout);
        state.brdfPipe = LoadCS("IBLBRDF", "ibl_brdf_lut", nv, state.brdfLayout);
        state.probeCapturePipe = LoadCS("IBLProbeCap", "ibl_probe_capture", nv, state.probeCaptureLayout);
        state.pipelinesReady = state.prefilterPipe && state.shPipe && state.brdfPipe;
        if (!state.pipelinesReady)
            Msg("! [IBL] Compute pipelines incomplete — falling back to raw sky IBL");
    }

    if (state.resourcesReady)
        return;

    state.mipCount = CountMips(kIBLSpecSize);

    auto makeCube = [&](const char* name, u32 size, u32 mips) -> nvrhi::TextureHandle {
        nvrhi::TextureDesc td;
        td.width = size;
        td.height = size;
        td.depth = 1;
        td.arraySize = 6;
        td.mipLevels = mips;
        td.format = nvrhi::Format::RGBA16_FLOAT;
        td.dimension = nvrhi::TextureDimension::TextureCube;
        td.isUAV = true;
        td.isShaderResource = true;
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        td.debugName = name;
        return nv->createTexture(td);
    };

    state.spec0 = makeCube("IBL_Spec0", kIBLSpecSize, state.mipCount);
    state.spec1 = makeCube("IBL_Spec1", kIBLSpecSize, state.mipCount);

    {
        nvrhi::TextureDesc td;
        td.width = kIBLBrdfSize;
        td.height = kIBLBrdfSize;
        td.format = nvrhi::Format::RG16_FLOAT;
        td.dimension = nvrhi::TextureDimension::Texture2D;
        td.isUAV = true;
        td.isShaderResource = true;
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        td.debugName = "IBL_BRDFLUT";
        state.brdfLut = nv->createTexture(td);
    }

    {
        nvrhi::TextureDesc td;
        td.width = kIBLProbeSize;
        td.height = kIBLProbeSize;
        td.arraySize = kIBLMaxProbes * 6;
        td.mipLevels = CountMips(kIBLProbeSize);
        td.format = nvrhi::Format::RGBA16_FLOAT;
        td.dimension = nvrhi::TextureDimension::TextureCubeArray;
        td.isUAV = true;
        td.isShaderResource = true;
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        td.debugName = "IBL_ProbeCubeArray";
        state.probeCubeArray = nv->createTexture(td);
    }

    {
        nvrhi::TextureDesc td;
        td.width = kIBLProbeSize;
        td.height = kIBLProbeSize;
        td.arraySize = 6;
        td.mipLevels = 1;
        td.format = nvrhi::Format::RGBA16_FLOAT;
        td.dimension = nvrhi::TextureDimension::TextureCube;
        td.isUAV = true;
        td.isShaderResource = true;
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        td.debugName = "IBL_ProbeCaptureTemp";
        state.probeCaptureTemp = nv->createTexture(td);
    }

    {
        nvrhi::BufferDesc bd;
        bd.byteSize = sizeof(Fvector4) * (kIBLSHCount * 2 + 1);
        bd.structStride = sizeof(Fvector4);
        bd.canHaveUAVs = true;
        bd.initialState = nvrhi::ResourceStates::ShaderResource;
        bd.keepInitialState = true;
        bd.debugName = "IBL_SkySH";
        state.shBuffer = nv->createBuffer(bd);
    }

    {
        nvrhi::BufferDesc bd;
        bd.byteSize = sizeof(Fvector4) * (1 + kIBLMaxProbes * kIBLProbeStride);
        bd.structStride = sizeof(Fvector4);
        bd.canHaveUAVs = false;
        bd.initialState = nvrhi::ResourceStates::ShaderResource;
        bd.keepInitialState = true;
        bd.cpuAccess = nvrhi::CpuAccessMode::Write;
        bd.debugName = "IBL_Probes";
        state.probeBuffer = nv->createBuffer(bd);
    }

    auto makeCB = [&](const char* name, size_t bytes) {
        nvrhi::BufferDesc bd;
        bd.byteSize = bytes;
        bd.isConstantBuffer = true;
        bd.isVolatile = true;
        bd.maxVersions = 4096;
        bd.initialState = nvrhi::ResourceStates::ConstantBuffer;
        bd.keepInitialState = true;
        bd.debugName = name;
        return nv->createBuffer(bd);
    };
    state.prefilterCB = makeCB("IBL_PrefilterCB", sizeof(PrefilterCB));
    state.shCB = makeCB("IBL_SHCB", sizeof(SHProjectCB));
    state.brdfCB = makeCB("IBL_BRDFCB", sizeof(BRDFLUTCB));
    state.probeCaptureCB = makeCB("IBL_ProbeCapCB", sizeof(ProbeCaptureCB));

    state.resourcesReady = state.spec0 && state.spec1 && state.brdfLut && state.shBuffer && state.probeBuffer;
    state.needsPrefilter = true;
    Msg("* [IBL] Resources ready spec=%u mips=%u probes=%u", kIBLSpecSize, state.mipCount, kIBLMaxProbes);
}

static void DispatchPrefilterMip(
    nvrhi::ICommandList* cmd,
    nvrhi::IDevice* nv,
    IBLPrefilterPassState& state,
    nvrhi::ITexture* src,
    nvrhi::ITexture* dst,
    u32 mip)
{
    if (!cmd || !src || !dst || !state.prefilterPipe || mip >= state.mipCount)
        return;

    auto* loader = GEnv.Render->GetShaderLoader();
    auto* refl = loader->GetCachedReflection("ibl_prefilter_specular", ".cs");
    if (!refl)
        return;
    auto& cache = GetPassResourceCache();

    cmd->setTextureState(dst, nvrhi::TextureSubresourceSet(mip, 1, 0, 6), nvrhi::ResourceStates::UnorderedAccess);
    cmd->setTextureState(src, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);

    const u32 size = std::max(1u, kIBLSpecSize >> mip);
    const float roughness = state.mipCount > 1 ? float(mip) / float(state.mipCount - 1) : 0.f;
    const u32 samples = roughness < 1e-3f ? 1u : (mip < 3 ? 16u : 8u);
    for (u32 face = 0; face < 6; ++face)
    {
        PrefilterCB cb{};
        cb.size[0] = size;
        cb.size[1] = size;
        cb.face = face;
        cb.sampleCount = samples;
        cb.roughness = roughness;
        cb.mipBias = 0.f;
        cmd->writeBuffer(state.prefilterCB, &cb, sizeof(cb));

        BindingSetBuilder bsb(*refl, nv, "IBLPrefilter");
        bsb.Texture("t_Source", src)
            .TextureUAV("u_Dest", dst, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(mip, 1, face, 1))
            .ConstantBuffer("PrefilterParams", state.prefilterCB);
        auto set = cache.GetOrCreateBindingSet(bsb.Build(), state.prefilterLayout, nv);
        if (!set)
            continue;

        nvrhi::ComputeState cs;
        cs.pipeline = state.prefilterPipe;
        cs.bindings = {set};
        cmd->setComputeState(cs);
        cmd->dispatch((size + 7) / 8, (size + 7) / 8, 1);
    }

    cmd->setTextureState(dst, nvrhi::TextureSubresourceSet(mip, 1, 0, 6), nvrhi::ResourceStates::ShaderResource);
}

static void DispatchSH(
    nvrhi::ICommandList* cmd,
    nvrhi::IDevice* nv,
    IBLPrefilterPassState& state,
    nvrhi::ITexture* src,
    u32 outOffset)
{
    if (!cmd || !src || !state.shPipe)
        return;
    auto* loader = GEnv.Render->GetShaderLoader();
    auto* refl = loader->GetCachedReflection("ibl_sh_project", ".cs");
    if (!refl)
        return;
    auto& cache = GetPassResourceCache();

    cmd->setBufferState(state.shBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmd->setTextureState(src, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);

    SHProjectCB cb{};
    cb.size = 32;
    cb.outOffset = outOffset;
    cmd->writeBuffer(state.shCB, &cb, sizeof(cb));

    BindingSetBuilder bsb(*refl, nv, "IBLSH");
    bsb.Texture("t_Source", src)
        .BufferUAV("u_SH", state.shBuffer)
        .ConstantBuffer("SHProjectParams", state.shCB);
    auto set = cache.GetOrCreateBindingSet(bsb.Build(), state.shLayout, nv);
    if (!set)
        return;

    nvrhi::ComputeState cs;
    cs.pipeline = state.shPipe;
    cs.bindings = {set};
    cmd->setComputeState(cs);
    cmd->dispatch(1, 1, 1);
    cmd->setBufferState(state.shBuffer, nvrhi::ResourceStates::ShaderResource);
}

static void DispatchBRDF(nvrhi::ICommandList* cmd, nvrhi::IDevice* nv, IBLPrefilterPassState& state)
{
    if (!cmd || !state.brdfPipe || state.brdfReady)
        return;
    auto* loader = GEnv.Render->GetShaderLoader();
    auto* refl = loader->GetCachedReflection("ibl_brdf_lut", ".cs");
    if (!refl)
        return;
    auto& cache = GetPassResourceCache();

    cmd->setTextureState(state.brdfLut, nvrhi::AllSubresources, nvrhi::ResourceStates::UnorderedAccess);
    BRDFLUTCB cb{};
    cb.dims[0] = kIBLBrdfSize;
    cb.dims[1] = kIBLBrdfSize;
    cmd->writeBuffer(state.brdfCB, &cb, sizeof(cb));

    BindingSetBuilder bsb(*refl, nv, "IBLBRDF");
    bsb.TextureUAV("u_BRDFLUT", state.brdfLut).ConstantBuffer("BRDFLUTParams", state.brdfCB);
    auto set = cache.GetOrCreateBindingSet(bsb.Build(), state.brdfLayout, nv);
    if (!set)
        return;

    nvrhi::ComputeState cs;
    cs.pipeline = state.brdfPipe;
    cs.bindings = {set};
    cmd->setComputeState(cs);
    cmd->dispatch((kIBLBrdfSize + 7) / 8, (kIBLBrdfSize + 7) / 8, 1);
    cmd->setTextureState(state.brdfLut, nvrhi::AllSubresources, nvrhi::ResourceStates::ShaderResource);
    state.brdfReady = true;
}

static void IBLUpdateBlends(IBLPrefilterPassState& state)
{
    const float dt = std::max(Device.fTimeDelta, 0.f);
    const float prefSpeed = 1.25f;
    const float probeSpeed = 1.75f;
    const bool wantPref = ps_r_ibl_prefilter != 0 && state.pipelinesReady && state.hasBakedOnce
        && !state.needsPrefilter && state.prefilterStep == 0;
    const float prefTarget = wantPref ? 1.f : 0.f;
    state.prefilterBlend += (prefTarget - state.prefilterBlend)
        * std::min(1.f, dt * prefSpeed);
    state.prefilterBlend = std::clamp(state.prefilterBlend, 0.f, 1.f);

    for (u32 i = 0; i < state.probeCount; ++i)
    {
        auto& p = state.probes[i];
        const float target = (p.ready && !p.dirty) ? p.intensity : 0.f;
        p.blend += (target - p.blend) * std::min(1.f, dt * probeSpeed);
        p.blend = std::clamp(p.blend, 0.f, 1.f);
    }
}

void IBLUploadProbeBuffer(IBLPrefilterPassState& state, nvrhi::ICommandList* cmd, nvrhi::IDevice* nv)
{
    if (!cmd || !state.probeBuffer)
        return;
    IBLUpdateBlends(state);
    xr_vector<Fvector4> data;
    data.resize(1 + kIBLMaxProbes * kIBLProbeStride);
    data[0].set(0, 0, 0, float(state.probeCount));
    for (u32 i = 0; i < state.probeCount; ++i)
    {
        const auto& p = state.probes[i];
        const u32 base = 1 + i * kIBLProbeStride;
        data[base + 0].set(p.position.x, p.position.y, p.position.z, p.radius);
        data[base + 1].set(p.boxMin.x, p.boxMin.y, p.boxMin.z, 0.f);
        data[base + 2].set(p.boxMax.x, p.boxMax.y, p.boxMax.z, p.blend);
        for (u32 s = 0; s < kIBLSHCount; ++s)
            data[base + 3 + s].set(0, 0, 0, 0);
    }
    cmd->writeBuffer(state.probeBuffer, data.data(), data.size() * sizeof(Fvector4));
}

void IBLUpdateFrameMeta(IBLPrefilterPassState& state, nvrhi::ICommandList* cmd, float ssgiSkyScale)
{
    if (!cmd || !state.shBuffer)
        return;
    Fvector4 meta;
    meta.set(float(state.mipCount), state.prefilterBlend, ssgiSkyScale, float(state.probeCount));
    const u32 metaOffset = (kIBLSHCount * 2) * sizeof(Fvector4);
    cmd->setBufferState(state.shBuffer, nvrhi::ResourceStates::CopyDest);
    cmd->writeBuffer(state.shBuffer, &meta, sizeof(meta), metaOffset);
    cmd->setBufferState(state.shBuffer, nvrhi::ResourceStates::ShaderResource);
}

static void IBLFillProbeBounds(IBLProbeCPU& p)
{
    p.boxMin.set(p.position.x - p.radius, p.position.y - p.radius * 0.5f, p.position.z - p.radius);
    p.boxMax.set(p.position.x + p.radius, p.position.y + p.radius * 1.5f, p.position.z + p.radius);
}

static void IBLMarkAllProbesDirty(IBLPrefilterPassState& state)
{
    for (u32 i = 0; i < state.probeCount; ++i)
        state.probes[i].dirty = true;
}

static void IBLScheduleNextDirtyCapture(IBLPrefilterPassState& state)
{
    if (state.pendingCapture >= 0)
        return;
    const Fvector& cam = Device.vCameraPosition;
    int best = -1;
    float bestD = flt_max;
    for (u32 i = 0; i < state.probeCount; ++i)
    {
        if (!state.probes[i].dirty)
            continue;
        const float d = cam.distance_to(state.probes[i].position);
        if (d < bestD)
        {
            bestD = d;
            best = int(i);
        }
    }
    if (best >= 0)
    {
        state.pendingCapture = best;
        state.captureStep = 0;
    }
}

int IBLAddProbeAtCamera(IBLPrefilterPassState& state, float radius)
{
    if (state.probeCount >= kIBLMaxProbes)
        return -1;
    const int idx = int(state.probeCount++);
    auto& p = state.probes[idx];
    p.position = Device.vCameraPosition;
    p.radius = radius;
    IBLFillProbeBounds(p);
    p.intensity = 1.f;
    p.blend = 0.f;
    p.dirty = true;
    p.ready = false;
    state.pendingCapture = idx;
    state.captureStep = 0;
    return idx;
}

void IBLClearProbes(IBLPrefilterPassState& state)
{
    state.probeCount = 0;
    state.pendingCapture = -1;
    state.captureStep = 0;
}

void IBLRequestCapture(IBLPrefilterPassState& state, int probeIndex)
{
    if (probeIndex >= 0 && probeIndex < int(state.probeCount))
    {
        state.probes[probeIndex].dirty = true;
        state.pendingCapture = probeIndex;
        state.captureStep = 0;
    }
}

void IBLUpdateAutoProbes(IBLPrefilterPassState& state, nvrhi::ITexture* srcSky0, nvrhi::ITexture* srcSky1)
{
    if (ps_r_ibl_probe_auto == 0 || !state.resourcesReady || !state.pipelinesReady)
        return;
    if (ps_r_ibl_prefilter == 0 || ps_r_sky_ibl == 0)
        return;
    if (!g_pGameLevel || !srcSky0)
        return;

    const Fvector& cam = Device.vCameraPosition;
    if (cam.square_magnitude() < 1.f)
        return;

    const bool skyChanged = (srcSky0 != state.lastAutoSky0) || (srcSky1 != state.lastAutoSky1);
    if (skyChanged)
    {
        state.lastAutoSky0 = srcSky0;
        state.lastAutoSky1 = srcSky1;
        IBLMarkAllProbesDirty(state);
    }

    const float radius = std::max(2.f, ps_r_ibl_probe_radius);
    const float spacing = radius * 0.85f;

    int nearest = -1;
    float nearestDist = flt_max;
    int farthest = -1;
    float farthestDist = -1.f;
    for (u32 i = 0; i < state.probeCount; ++i)
    {
        const float d = cam.distance_to(state.probes[i].position);
        if (d < nearestDist)
        {
            nearestDist = d;
            nearest = int(i);
        }
        if (d > farthestDist)
        {
            farthestDist = d;
            farthest = int(i);
        }
    }

    if (nearest < 0 || nearestDist > spacing)
    {
        int idx = -1;
        if (state.probeCount < kIBLMaxProbes)
            idx = int(state.probeCount++);
        else if (farthest >= 0)
            idx = farthest;

        if (idx >= 0)
        {
            auto& p = state.probes[idx];
            p.position = cam;
            p.radius = radius;
            IBLFillProbeBounds(p);
            p.intensity = 1.f;
            p.blend = 0.f;
            p.dirty = true;
            p.ready = false;
            static u32 s_placeLog = 0;
            if (s_placeLog < 8)
            {
                Msg("* [IBL] auto probe %d at (%.1f,%.1f,%.1f) r=%.1f", idx, cam.x, cam.y, cam.z, radius);
                ++s_placeLog;
            }
        }
    }

    IBLScheduleNextDirtyCapture(state);
}

static bool CaptureProbeStep(
    nvrhi::ICommandList* cmd,
    nvrhi::IDevice* nv,
    IBLPrefilterPassState& state,
    nvrhi::ITexture* sky0,
    nvrhi::ITexture* sky1,
    nvrhi::ITexture* sceneColor,
    nvrhi::ITexture* sceneDepth,
    int probeIndex)
{
    if (!cmd || !state.probeCapturePipe || probeIndex < 0 || probeIndex >= int(state.probeCount))
        return true;
    if (!sky0 || !sceneColor || !sceneDepth || !state.probeCaptureTemp || !state.probeCubeArray)
        return true;

    auto* loader = GEnv.Render->GetShaderLoader();
    auto& cache = GetPassResourceCache();
    auto& p = state.probes[probeIndex];

    if (state.captureStep < 6)
    {
        auto* refl = loader->GetCachedReflection("ibl_probe_capture", ".cs");
        if (!refl)
            return true;

        float skyLerp = 0.f;
        if (g_pGamePersistent)
            skyLerp = g_pGamePersistent->Environment().CurrentEnv.weight;

        const u32 face = state.captureStep;
        cmd->setTextureState(state.probeCaptureTemp,
            nvrhi::TextureSubresourceSet(0, 1, face, 1), nvrhi::ResourceStates::UnorderedAccess);

        ProbeCaptureCB cb{};
        cb.vp = Device.mFullTransform;
        cb.invVP = Device.mInvFullTransform;
        cb.probePos = p.position;
        cb.skyLerp = skyLerp;
        cb.eyePos = Device.vCameraPosition;
        cb.maxDistance = p.radius * 2.f;
        cb.size[0] = kIBLProbeSize;
        cb.size[1] = kIBLProbeSize;
        cb.face = face;
        cmd->writeBuffer(state.probeCaptureCB, &cb, sizeof(cb));

        BindingSetBuilder bsb(*refl, nv, "IBLProbeCap");
        bsb.Texture("t_Sky0", sky0)
            .Texture("t_Sky1", sky1 ? sky1 : sky0)
            .Texture("g_SceneColor", sceneColor)
            .Texture("g_SceneDepth", sceneDepth)
            .TextureUAV("u_ProbeFaces", state.probeCaptureTemp, nvrhi::Format::UNKNOWN,
                nvrhi::TextureSubresourceSet(0, 1, face, 1))
            .ConstantBuffer("ProbeCaptureParams", state.probeCaptureCB);
        auto set = cache.GetOrCreateBindingSet(bsb.Build(), state.probeCaptureLayout, nv);
        if (set)
        {
            nvrhi::ComputeState cs;
            cs.pipeline = state.probeCapturePipe;
            cs.bindings = {set};
            cmd->setComputeState(cs);
            cmd->dispatch((kIBLProbeSize + 7) / 8, (kIBLProbeSize + 7) / 8, 1);
        }
        cmd->setTextureState(state.probeCaptureTemp,
            nvrhi::TextureSubresourceSet(0, 1, face, 1), nvrhi::ResourceStates::ShaderResource);
        ++state.captureStep;
        return false;
    }

    if (state.prefilterPipe)
    {
        auto* prefRefl = loader->GetCachedReflection("ibl_prefilter_specular", ".cs");
        if (prefRefl)
        {
            cmd->setTextureState(state.probeCaptureTemp, nvrhi::AllSubresources,
                nvrhi::ResourceStates::ShaderResource);
            cmd->setTextureState(state.probeCubeArray,
                nvrhi::TextureSubresourceSet(0, 1, probeIndex * 6, 6),
                nvrhi::ResourceStates::UnorderedAccess);

            for (u32 face = 0; face < 6; ++face)
            {
                PrefilterCB cb{};
                cb.size[0] = kIBLProbeSize;
                cb.size[1] = kIBLProbeSize;
                cb.face = face;
                cb.sampleCount = 1;
                cb.roughness = 0.f;
                cb.mipBias = 0.f;
                cmd->writeBuffer(state.prefilterCB, &cb, sizeof(cb));

                const u32 arraySlice = probeIndex * 6 + face;
                BindingSetBuilder bsb(*prefRefl, nv, "IBLProbePref");
                bsb.Texture("t_Source", state.probeCaptureTemp)
                    .TextureUAV("u_Dest", state.probeCubeArray, nvrhi::Format::UNKNOWN,
                        nvrhi::TextureSubresourceSet(0, 1, arraySlice, 1))
                    .ConstantBuffer("PrefilterParams", state.prefilterCB);
                auto set = cache.GetOrCreateBindingSet(bsb.Build(), state.prefilterLayout, nv);
                if (!set)
                    continue;
                nvrhi::ComputeState cs;
                cs.pipeline = state.prefilterPipe;
                cs.bindings = {set};
                cmd->setComputeState(cs);
                cmd->dispatch((kIBLProbeSize + 7) / 8, (kIBLProbeSize + 7) / 8, 1);
            }
            cmd->setTextureState(state.probeCubeArray,
                nvrhi::TextureSubresourceSet(0, 1, probeIndex * 6, 6),
                nvrhi::ResourceStates::ShaderResource);
        }
    }

    p.dirty = false;
    p.ready = true;
    state.captureStep = 0;
    Msg("* [IBL] Captured probe %d at (%.1f,%.1f,%.1f)", probeIndex, p.position.x, p.position.y, p.position.z);
    return true;
}

IBLBindResources setupIBLPrefilterPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    nvrhi::ITexture* srcSky0,
    nvrhi::ITexture* srcSky1,
    IBLPrefilterPassState& state)
{
    g_ActiveIBLState = &state;

    IBLBindResources out{};
    out.prefilterEnabled = false;
    out.mipCount = state.mipCount;

    auto& cache = GetPassResourceCache();
    nvrhi::IDevice* nv = device ? device->GetNVRHIDevice() : nullptr;
    nvrhi::ITexture* dummyCube = nv ? cache.GetDummyCubeMap(nv) : nullptr;
    const bool hasRealSky = srcSky0 && srcSky0 != dummyCube;
    const bool iblActive = ps_r_ibl_prefilter != 0 && ps_r_sky_ibl != 0 && g_pGameLevel && hasRealSky
        && !Device.dwPrecacheFrame
        && !(g_pGamePersistent && g_pGamePersistent->IsLoadingScreenShown());

    if (!nv || !iblActive)
    {
        out.brdfLut = nv ? cache.GetDummyContactHistory(nv) : nullptr;
        out.shBuffer = nv ? cache.GetDummyStructuredBuffer(nv) : nullptr;
        out.probeBuffer = nv ? cache.GetDummyStructuredBuffer(nv) : nullptr;
        out.probeCubeArray = nv ? cache.GetDummyCubeArray(nv) : nullptr;
        out.spec0 = srcSky0 ? srcSky0 : dummyCube;
        out.spec1 = srcSky1 ? srcSky1 : out.spec0;
        SetCurrentIBLBindResources(out);
        return out;
    }

    InitializeIBLPrefilter(device, state);
    if (!state.resourcesReady || !state.pipelinesReady)
    {
        out.brdfLut = cache.GetDummyContactHistory(nv);
        out.shBuffer = cache.GetDummyStructuredBuffer(nv);
        out.probeBuffer = cache.GetDummyStructuredBuffer(nv);
        out.probeCubeArray = cache.GetDummyCubeArray(nv);
        out.spec0 = srcSky0 ? srcSky0 : dummyCube;
        out.spec1 = srcSky1 ? srcSky1 : out.spec0;
        SetCurrentIBLBindResources(out);
        return out;
    }

    out.brdfLut = state.brdfLut;
    out.shBuffer = state.shBuffer;
    out.probeBuffer = state.probeBuffer;
    out.probeCubeArray = state.probeCubeArray;

    if (srcSky0 != state.lastSrc0 || srcSky1 != state.lastSrc1)
    {
        state.lastSrc0 = srcSky0;
        state.lastSrc1 = srcSky1;
        state.needsPrefilter = true;
        state.prefilterStep = 1;
    }
    else if (state.needsPrefilter && state.prefilterStep == 0)
    {
        state.prefilterStep = 1;
    }

    if (state.hasBakedOnce)
    {
        out.spec0 = state.spec0;
        out.spec1 = state.spec1;
        out.prefilterEnabled = true;
    }
    else
    {
        out.spec0 = srcSky0;
        out.spec1 = srcSky1 ? srcSky1 : srcSky0;
        out.prefilterEnabled = false;
    }
    SetCurrentIBLBindResources(out);

    fg.addCallbackPass<int>(
        "IBL Prefilter",
        [&](FrameGraph& builder, PassHandle passHandle, int&) {
            RenderPassBuilder pb(builder, passHandle);
            pb.sideEffects();
        },
        [&state, srcSky0, srcSky1, nv](const int&, const FrameGraph&, fg::RenderContext* ctx) {
            nvrhi::ICommandList* cmd = ctx ? ctx->GetCommandList() : nullptr;
            if (!cmd || !nv)
                return;

            const u32 stepSpec0 = 2;
            const u32 stepSpec1 = 2 + state.mipCount;
            const u32 stepSH = 2 + 2 * state.mipCount;

            if (state.prefilterStep == 1)
            {
                DispatchBRDF(cmd, nv, state);
                state.prefilterStep = stepSpec0;
            }
            else if (state.prefilterStep >= stepSpec0 && state.prefilterStep < stepSpec1)
            {
                DispatchPrefilterMip(cmd, nv, state, srcSky0, state.spec0,
                    state.prefilterStep - stepSpec0);
                ++state.prefilterStep;
            }
            else if (state.prefilterStep >= stepSpec1 && state.prefilterStep < stepSH)
            {
                nvrhi::ITexture* src1 = (srcSky1 && srcSky1 != srcSky0) ? srcSky1 : srcSky0;
                DispatchPrefilterMip(cmd, nv, state, src1, state.spec1,
                    state.prefilterStep - stepSpec1);
                ++state.prefilterStep;
            }
            else if (state.prefilterStep == stepSH)
            {
                Fvector4 zeros[kIBLSHCount * 2 + 1] = {};
                cmd->setBufferState(state.shBuffer, nvrhi::ResourceStates::CopyDest);
                cmd->writeBuffer(state.shBuffer, zeros, sizeof(zeros));
                DispatchSH(cmd, nv, state, srcSky0, 0);
                DispatchSH(cmd, nv, state, srcSky1 ? srcSky1 : srcSky0, kIBLSHCount);
                state.needsPrefilter = false;
                state.prefilterStep = 0;
                state.hasBakedOnce = true;
                Msg("* [IBL] Prefiltered sky cubes + SH");
            }

            IBLUploadProbeBuffer(state, cmd, nv);
            IBLUpdateFrameMeta(state, cmd, 1.f);
        });

    return out;
}

void setupIBLProbeCapturePass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    nvrhi::ITexture* srcSky0,
    nvrhi::ITexture* srcSky1,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle sceneDepth,
    IBLPrefilterPassState& state)
{
    IBLUpdateAutoProbes(state, srcSky0, srcSky1);

    if (state.needsPrefilter || state.prefilterStep != 0)
        return;
    if (state.pendingCapture < 0 || !sceneColor.is_valid() || !sceneDepth.is_valid())
        return;
    if (!srcSky0 || !state.pipelinesReady)
        return;

    if (!state.probeCapturePipe)
    {
        static bool s_logged = false;
        if (!s_logged)
        {
            Msg("! [IBL] probe capture pipeline missing — auto probes disabled");
            s_logged = true;
        }
        state.pendingCapture = -1;
        return;
    }

    fg.addCallbackPass<int>(
        "IBL Probe Capture",
        [&](FrameGraph& builder, PassHandle passHandle, int&) {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.read(sceneColor, ResourceState::ShaderResource);
            passBuilder.read(sceneDepth, ResourceState::ShaderResource);
            passBuilder.sideEffects();
        },
        [&state, srcSky0, srcSky1, sceneColor, sceneDepth, device](const int&, const FrameGraph& graph, fg::RenderContext* ctx) {
            nvrhi::ICommandList* cmd = ctx ? ctx->GetCommandList() : nullptr;
            nvrhi::IDevice* nvDev = device ? device->GetNVRHIDevice() : nullptr;
            if (!cmd || !nvDev || state.pendingCapture < 0)
                return;
            nvrhi::ITexture* colorTex = graph.GetPhysicalTexture(sceneColor);
            nvrhi::ITexture* depthTex = graph.GetPhysicalTexture(sceneDepth);
            if (!colorTex || !depthTex)
                return;
            const int idx = state.pendingCapture;
            const bool done = CaptureProbeStep(cmd, nvDev, state, srcSky0,
                srcSky1 ? srcSky1 : srcSky0, colorTex, depthTex, idx);
            if (done)
            {
                state.pendingCapture = -1;
                IBLUploadProbeBuffer(state, cmd, nvDev);
                IBLScheduleNextDirtyCapture(state);
            }
        });
}

void BindIBLResources(BindingSetBuilder& bsb, const IBLBindResources& ibl, nvrhi::IDevice* nv)
{
    auto& cache = GetPassResourceCache();
    nvrhi::ITexture* brdf = ibl.brdfLut ? ibl.brdfLut : cache.GetDummyContactHistory(nv);
    nvrhi::IBuffer* sh = ibl.shBuffer ? ibl.shBuffer : cache.GetDummyStructuredBuffer(nv);
    nvrhi::IBuffer* probeMeta = ibl.probeBuffer ? ibl.probeBuffer : cache.GetDummyStructuredBuffer(nv);
    nvrhi::ITexture* probes = ibl.probeCubeArray
        ? ibl.probeCubeArray
        : cache.GetDummyCubeArray(nv);

    if (brdf)
        bsb.Texture("s_envBRDF", brdf);
    if (sh)
        bsb.BufferSRV("g_IBLSkySH", sh);
    if (probeMeta)
        bsb.BufferSRV("g_IBLProbes", probeMeta);
    if (probes)
        bsb.Texture("s_probeSpec", probes);
}

} // namespace xray::render::fg::passes
