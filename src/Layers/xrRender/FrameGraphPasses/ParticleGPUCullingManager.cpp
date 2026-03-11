// ParticleGPUCullingManager.cpp
// GPU-driven particle culling and billboard generation
#include "stdafx.h"
#include "ParticleGPUCullingManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "xrEngine/device.h"
#include "xrCDB/Frustum.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"

namespace xray::render::RENDER_NAMESPACE::passes {

constexpr u32 CULL_THREAD_GROUP_SIZE = 64;
constexpr u32 BILLBOARD_THREAD_GROUP_SIZE = 64;

ParticleGPUCullingManager::~ParticleGPUCullingManager()
{
    Shutdown();
}

bool ParticleGPUCullingManager::Initialize(ng::RenderDevice* device, u32 maxParticles)
{
    if (m_initialized)
        return true;

    if (!device || maxParticles == 0)
        return false;

    m_device = device;
    m_maxParticles = maxParticles;

    Msg("* [ParticleGPUCulling] Initializing for %u max particles...", maxParticles);

    if (!CreateShaders()) {
        Msg("! [ParticleGPUCulling] Failed to create shaders");
        Shutdown();
        return false;
    }

    if (!CreateBuffers()) {
        Msg("! [ParticleGPUCulling] Failed to create buffers");
        Shutdown();
        return false;
    }

    if (!CreateBindingLayouts()) {
        Msg("! [ParticleGPUCulling] Failed to create binding layouts");
        Shutdown();
        return false;
    }

    if (!CreatePipelines()) {
        Msg("! [ParticleGPUCulling] Failed to create pipelines");
        Shutdown();
        return false;
    }

    m_initialized = true;
    Msg("* [ParticleGPUCulling] Initialization complete");
    return true;
}

void ParticleGPUCullingManager::Shutdown()
{
    m_cullPipeline = nullptr;
    m_billboardPipeline = nullptr;
    m_cullLayout = nullptr;
    m_billboardLayout = nullptr;
    m_cullShader = nullptr;
    m_billboardShader = nullptr;
    m_particleDataBuffer = nullptr;
    m_visibleIndicesBuffer = nullptr;
    m_visibleCountBuffer = nullptr;
    m_vertexBuffer = nullptr;
    m_drawArgsBuffer = nullptr;
    m_pointClampSampler = nullptr;
    m_cullBindingSet = nullptr;
    m_billboardBindingSet = nullptr;

    m_device = nullptr;
    m_maxParticles = 0;
    m_initialized = false;
}

bool ParticleGPUCullingManager::CreateShaders()
{
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return false;

    auto cullResult = shaderLoader->LoadComputeShader("particle_cull", "main");
    if (!cullResult.handle) {
        Msg("! [ParticleGPUCulling] Failed to load particle_cull.cs");
        return false;
    }
    m_cullShader = cullResult.handle;

    auto billboardResult = shaderLoader->LoadComputeShader("particle_billboard", "main");
    if (!billboardResult.handle) {
        Msg("! [ParticleGPUCulling] Failed to load particle_billboard.cs");
        return false;
    }
    m_billboardShader = billboardResult.handle;

    Msg("* [ParticleGPUCulling] Shaders loaded");
    return true;
}

bool ParticleGPUCullingManager::CreateBuffers()
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return false;

    // Particle data buffer (input from CPU)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = m_maxParticles * sizeof(GPUParticleData);
        desc.structStride = sizeof(GPUParticleData);
        desc.debugName = "ParticleDataBuffer";
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.canHaveUAVs = false;
        m_particleDataBuffer = nvDevice->createBuffer(desc);
        if (!m_particleDataBuffer)
            return false;
    }

    // Visible indices buffer (output from culling, input to billboard)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = m_maxParticles * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.debugName = "ParticleVisibleIndices";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        m_visibleIndicesBuffer = nvDevice->createBuffer(desc);
        if (!m_visibleIndicesBuffer)
            return false;
    }

    // Visible count buffer (single u32, output from culling)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(u32);
        desc.debugName = "ParticleVisibleCount";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;  // For InterlockedAdd
        m_visibleCountBuffer = nvDevice->createBuffer(desc);
        if (!m_visibleCountBuffer)
            return false;
    }

    // Vertex buffer (output from billboard generation)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = m_maxParticles * 4 * sizeof(ParticleVertex);  // 4 verts per particle
        desc.structStride = sizeof(ParticleVertex);
        desc.debugName = "ParticleVertexBuffer";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        desc.isVertexBuffer = true;
        m_vertexBuffer = nvDevice->createBuffer(desc);
        if (!m_vertexBuffer)
            return false;
    }

    // Draw args buffer (indirect draw arguments)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = 5 * sizeof(u32);  // DrawIndexedIndirectArgs
        desc.debugName = "ParticleDrawArgs";
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        m_drawArgsBuffer = nvDevice->createBuffer(desc);
        if (!m_drawArgsBuffer)
            return false;
    }

    // Point clamp sampler for Hi-Z
    {
        nvrhi::SamplerDesc desc;
        desc.setAllAddressModes(nvrhi::SamplerAddressMode::Clamp);
        desc.setAllFilters(false);  // Point filtering
        m_pointClampSampler = nvDevice->createSampler(desc);
        if (!m_pointClampSampler)
            return false;
    }

    Msg("* [ParticleGPUCulling] Buffers created (max %u particles, VB size %u KB)",
        m_maxParticles,
        (m_maxParticles * 4 * sizeof(ParticleVertex)) / 1024);
    return true;
}

bool ParticleGPUCullingManager::CreateBindingLayouts()
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return false;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return false;

    auto* cullRefl = shaderLoader->GetCachedReflection("particle_cull", ".cs");
    if (cullRefl) {
        m_cullLayout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection("ParticleGPUCull", *cullRefl, nvDevice);
        if (!m_cullLayout)
            return false;
    }

    auto* billboardRefl = shaderLoader->GetCachedReflection("particle_billboard", ".cs");
    if (billboardRefl) {
        m_billboardLayout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection("ParticleBillboard", *billboardRefl, nvDevice);
        if (!m_billboardLayout)
            return false;
    }

    Msg("* [ParticleGPUCulling] Binding layouts created");
    return true;
}

bool ParticleGPUCullingManager::CreatePipelines()
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    if (!nvDevice)
        return false;

    // Cull pipeline
    {
        nvrhi::ComputePipelineDesc desc;
        desc.CS = m_cullShader;
        desc.bindingLayouts = { m_cullLayout };
        m_cullPipeline = nvDevice->createComputePipeline(desc);
        if (!m_cullPipeline)
            return false;

        // CRITICAL FIX: Query binding layout from pipeline
        nvrhi::IComputePipeline* nativePipeline = m_cullPipeline;
        if (nativePipeline) {
            const nvrhi::ComputePipelineDesc& actualDesc = nativePipeline->getDesc();
            if (!actualDesc.bindingLayouts.empty()) {
                m_cullLayout = actualDesc.bindingLayouts[0];
            }
        }
    }

    // Billboard pipeline
    {
        nvrhi::ComputePipelineDesc desc;
        desc.CS = m_billboardShader;
        desc.bindingLayouts = { m_billboardLayout };
        m_billboardPipeline = nvDevice->createComputePipeline(desc);
        if (!m_billboardPipeline)
            return false;

        // CRITICAL FIX: Query binding layout from pipeline
        nvrhi::IComputePipeline* nativePipeline = m_billboardPipeline;
        if (nativePipeline) {
            const nvrhi::ComputePipelineDesc& actualDesc = nativePipeline->getDesc();
            if (!actualDesc.bindingLayouts.empty()) {
                m_billboardLayout = actualDesc.bindingLayouts[0];
            }
        }
    }

    Msg("* [ParticleGPUCulling] Pipelines created");
    return true;
}

void ParticleGPUCullingManager::UploadParticleData(
    nvrhi::ICommandList* cmdList,
    const xr_vector<GPUParticleData>& particles)
{
    if (particles.empty() || !m_particleDataBuffer)
        return;

    u32 uploadSize = std::min((u32)particles.size(), m_maxParticles) * sizeof(GPUParticleData);
    cmdList->writeBuffer(m_particleDataBuffer, particles.data(), uploadSize);
}

void ParticleGPUCullingManager::ClearVisibleCount(nvrhi::ICommandList* cmdList)
{
    if (!m_visibleCountBuffer)
        return;

    u32 zero = 0;
    cmdList->writeBuffer(m_visibleCountBuffer, &zero, sizeof(zero));
}

void ParticleGPUCullingManager::DispatchCulling(
    nvrhi::ICommandList* cmdList,
    nvrhi::ITexture* hiZPyramid,
    u32 particleCount,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels)
{
    if (!m_initialized || particleCount == 0)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    CFrustum frustum;
    frustum.CreateFromMatrix(Device.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
    u32 planeCount = std::min<u32>((u32)frustum.p_count, 6);

    ParticleCullParams params;
    params.viewProj = Device.mFullTransform;
    for (u32 i = 0; i < 6; i++)
    {
        if (i < planeCount)
            params.frustumPlanes[i].set(frustum.planes[i].n.x, frustum.planes[i].n.y, frustum.planes[i].n.z, frustum.planes[i].d);
        else
            params.frustumPlanes[i].set(0, 0, 0, 1000000.0f);
    }
    params.cameraPos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, 0.0f);
    params.cameraTop.set(Device.vCameraTop.x, Device.vCameraTop.y, Device.vCameraTop.z, 0.0f);
    params.cameraRight.set(Device.vCameraRight.x, Device.vCameraRight.y, Device.vCameraRight.z, 0.0f);
    params.particleCount = std::min(particleCount, m_maxParticles);
    params.hiZWidth = hiZWidth;
    params.hiZHeight = hiZHeight;
    params.hiZMipLevels = hiZMipLevels;

    auto& cache = framegraph::GetPassResourceCache();
    auto cullParamsCB = cache.GetOrCreateVolatileCB("ParticleGPUCull", "CullParams", sizeof(ParticleCullParams), m_device);
    cmdList->writeBuffer(cullParamsCB, &params, sizeof(params));

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* cullReflection = shaderLoader->GetCachedReflection("particle_cull", ".cs");
    if (!cullReflection) return;

    framegraph::BindingSetBuilder bsb(*cullReflection, nvDevice, "ParticleGPUCull");
    bsb.ConstantBuffer("ParticleCullParams", cullParamsCB);
    bsb.BufferSRV("g_ParticleData", m_particleDataBuffer);
    bsb.Texture("g_HiZPyramid", hiZPyramid);
    bsb.BufferUAV("g_VisibleIndices", m_visibleIndicesBuffer);
    bsb.BufferUAV("g_VisibleCount", m_visibleCountBuffer);
    m_cullBindingSet = cache.GetOrCreateBindingSet(bsb.Build(), m_cullLayout, nvDevice);

    // Dispatch
    nvrhi::ComputeState state;
    state.pipeline = m_cullPipeline;
    state.bindings = { m_cullBindingSet };
    cmdList->setComputeState(state);

    u32 numGroups = (params.particleCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
    cmdList->dispatch(numGroups, 1, 1);
}

void ParticleGPUCullingManager::DispatchBillboardGeneration(
    nvrhi::ICommandList* cmdList,
    u32 maxVisibleCount)
{
    if (!m_initialized || maxVisibleCount == 0)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    ParticleBillboardParams params;
    params.cameraTop.set(Device.vCameraTop.x, Device.vCameraTop.y, Device.vCameraTop.z, 0.0f);
    params.cameraRight.set(Device.vCameraRight.x, Device.vCameraRight.y, Device.vCameraRight.z, 0.0f);
    params.visibleCount = std::min(maxVisibleCount, m_maxParticles);
    params.padding[0] = params.padding[1] = params.padding[2] = 0;

    auto& cache = framegraph::GetPassResourceCache();
    auto billboardParamsCB = cache.GetOrCreateVolatileCB("ParticleBillboard", "BillboardParams", sizeof(ParticleBillboardParams), m_device);
    cmdList->writeBuffer(billboardParamsCB, &params, sizeof(params));

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    auto* billboardReflection = shaderLoader->GetCachedReflection("particle_billboard", ".cs");
    if (!billboardReflection) return;

    framegraph::BindingSetBuilder bsb(*billboardReflection, nvDevice, "ParticleBillboard");
    bsb.ConstantBuffer("BillboardParams", billboardParamsCB);
    bsb.BufferSRV("g_ParticleData", m_particleDataBuffer);
    bsb.BufferSRV("g_VisibleIndices", m_visibleIndicesBuffer);
    bsb.BufferSRV("g_VisibleCountBuf", m_visibleCountBuffer);
    bsb.BufferUAV("g_Vertices", m_vertexBuffer);
    bsb.BufferUAV("g_DrawArgs", m_drawArgsBuffer);
    m_billboardBindingSet = cache.GetOrCreateBindingSet(bsb.Build(), m_billboardLayout, nvDevice);

    nvrhi::ComputeState state;
    state.pipeline = m_billboardPipeline;
    state.bindings = { m_billboardBindingSet };
    cmdList->setComputeState(state);

    u32 numGroups = (params.visibleCount + BILLBOARD_THREAD_GROUP_SIZE - 1) / BILLBOARD_THREAD_GROUP_SIZE;
    cmdList->dispatch(numGroups, 1, 1);

}

} // namespace xray::render::RENDER_NAMESPACE::passes
