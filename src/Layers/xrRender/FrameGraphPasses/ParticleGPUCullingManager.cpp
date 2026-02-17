// ParticleGPUCullingManager.cpp
// GPU-driven particle culling and billboard generation
#include "stdafx.h"
#include "ParticleGPUCullingManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

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
    m_cullParamsCB = nullptr;
    m_billboardParamsCB = nullptr;
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

    // Cull params constant buffer
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(ParticleCullParams);
        desc.debugName = "ParticleCullParamsCB";
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;
        m_cullParamsCB = nvDevice->createBuffer(desc);
        if (!m_cullParamsCB)
            return false;
    }

    // Billboard params constant buffer
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = sizeof(ParticleBillboardParams);
        desc.debugName = "ParticleBillboardParamsCB";
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;
        m_billboardParamsCB = nvDevice->createBuffer(desc);
        if (!m_billboardParamsCB)
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

    // Cull binding layout
    {
        nvrhi::BindingLayoutDesc desc;
        desc.visibility = nvrhi::ShaderType::Compute;
        desc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),       // ParticleCullParams (b5 - avoid common.h collision)
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),         // g_ParticleData (t0)
            nvrhi::BindingLayoutItem::Texture_SRV(1),                  // g_HiZPyramid (t1)
            nvrhi::BindingLayoutItem::Sampler(0),                      // g_PointClampSampler (s0)
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),         // g_VisibleIndices (u0)
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),                // g_VisibleCount (u1)
        };
        m_cullLayout = nvDevice->createBindingLayout(desc);
        if (!m_cullLayout)
            return false;
    }

    // Billboard binding layout
    {
        nvrhi::BindingLayoutDesc desc;
        desc.visibility = nvrhi::ShaderType::Compute;
        desc.bindings = {
            nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),       // BillboardParams (b0)
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),         // g_ParticleData (t0)
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1),         // g_VisibleIndices (t1)
            nvrhi::BindingLayoutItem::RawBuffer_SRV(2),                // g_VisibleCountBuf (t2)
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),         // g_Vertices (u0)
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),                // g_DrawArgs (u1)
        };
        m_billboardLayout = nvDevice->createBindingLayout(desc);
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
    const Fmatrix& viewProj,
    const Fvector4* frustumPlanes,
    const Fvector& cameraPos,
    const Fvector& cameraTop,
    const Fvector& cameraRight,
    u32 particleCount,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels)
{
    if (!m_initialized || particleCount == 0)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    // Fill cull params
    ParticleCullParams params;
    // Must transpose matrix - Fmatrix storage is transposed relative to HLSL row-major
    params.viewProj.transpose(viewProj);
    for (int i = 0; i < 6; i++)
        params.frustumPlanes[i] = frustumPlanes[i];
    params.cameraPos.set(cameraPos.x, cameraPos.y, cameraPos.z, 0.0f);
    params.cameraTop.set(cameraTop.x, cameraTop.y, cameraTop.z, 0.0f);
    params.cameraRight.set(cameraRight.x, cameraRight.y, cameraRight.z, 0.0f);
    params.particleCount = std::min(particleCount, m_maxParticles);
    params.hiZWidth = hiZWidth;
    params.hiZHeight = hiZHeight;
    params.hiZMipLevels = hiZMipLevels;

    cmdList->writeBuffer(m_cullParamsCB, &params, sizeof(params));

    // Create binding set
    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(5, m_cullParamsCB),  // b5 to match layout (avoid common.h collision)
        nvrhi::BindingSetItem::StructuredBuffer_SRV(0, m_particleDataBuffer),
        nvrhi::BindingSetItem::Texture_SRV(1, hiZPyramid),
        nvrhi::BindingSetItem::Sampler(0, m_pointClampSampler),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(0, m_visibleIndicesBuffer),
        nvrhi::BindingSetItem::RawBuffer_UAV(1, m_visibleCountBuffer),
    };
    m_cullBindingSet = nvDevice->createBindingSet(bindDesc, m_cullLayout);

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
    const Fvector& cameraTop,
    const Fvector& cameraRight,
    u32 maxVisibleCount)
{
    if (!m_initialized || maxVisibleCount == 0)
        return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    ParticleBillboardParams params;
    params.cameraTop.set(cameraTop.x, cameraTop.y, cameraTop.z, 0.0f);
    params.cameraRight.set(cameraRight.x, cameraRight.y, cameraRight.z, 0.0f);
    params.visibleCount = std::min(maxVisibleCount, m_maxParticles);
    params.padding[0] = params.padding[1] = params.padding[2] = 0;

    cmdList->writeBuffer(m_billboardParamsCB, &params, sizeof(params));

    nvrhi::BindingSetDesc bindDesc;
    bindDesc.bindings = {
        nvrhi::BindingSetItem::ConstantBuffer(0, m_billboardParamsCB),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(0, m_particleDataBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_SRV(1, m_visibleIndicesBuffer),
        nvrhi::BindingSetItem::RawBuffer_SRV(2, m_visibleCountBuffer),
        nvrhi::BindingSetItem::StructuredBuffer_UAV(0, m_vertexBuffer),
        nvrhi::BindingSetItem::RawBuffer_UAV(1, m_drawArgsBuffer),
    };
    m_billboardBindingSet = nvDevice->createBindingSet(bindDesc, m_billboardLayout);

    nvrhi::ComputeState state;
    state.pipeline = m_billboardPipeline;
    state.bindings = { m_billboardBindingSet };
    cmdList->setComputeState(state);

    u32 numGroups = (params.visibleCount + BILLBOARD_THREAD_GROUP_SIZE - 1) / BILLBOARD_THREAD_GROUP_SIZE;
    cmdList->dispatch(numGroups, 1, 1);

}

} // namespace xray::render::RENDER_NAMESPACE::passes
