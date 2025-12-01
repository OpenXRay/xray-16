#include "stdafx.h"
#include "GPUCullingManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/xrRender_console.h"

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE {

constexpr u32 MAX_CULLING_OBJECTS = 65536;
constexpr u32 CULL_THREAD_GROUP_SIZE = 64;

struct CullParamsCB {
    Fmatrix viewProj;
    Fvector cameraPos;
    float maxDistanceSq;
    Fvector4 frustumPlanes[6];
    u32 objectCount;
    u32 hizWidth;
    u32 hizHeight;
    u32 hizMipLevels;
};

struct CullDebugParamsCB {
    Fmatrix viewProj;
    Fvector cameraPos;
    float maxDistanceSq;
    Fvector4 frustumPlanes[6];
    u32 objectCount;
    u32 hizWidth;
    u32 hizHeight;
    u32 hizMipLevels;
    float occluderThreshold;
    Fvector padding;
};

struct CullDebugVSParamsCB {
    Fmatrix view;
    Fmatrix viewProj;
    u32 objectCount;
    float wireframeAlpha;
    float padding[2];
};

static ref_cs s_object_cull_cs;
static ref_cs s_object_cull_debug_cs;
static ref_vs s_cull_debug_vs;
static ref_ps s_cull_debug_ps;

GPUCullingManager::GPUCullingManager()
    : m_objectCount(0)
    , m_maxObjects(MAX_CULLING_OBJECTS)
    , m_initialized(false)
    , m_computeEnabled(false)
{
    m_objectData.reserve(MAX_CULLING_OBJECTS);
}

GPUCullingManager::~GPUCullingManager()
{
    Shutdown();
}

void GPUCullingManager::Initialize(ng::RenderDevice* device)
{
    if (m_initialized)
        return;

    m_device = device;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        Msg("! [GPUCulling] NVRHI device not available");
        return;
    }

    s_object_cull_cs.create("object_cull");
    if (s_object_cull_cs && s_object_cull_cs->nvrhiShader) {
        Msg("* [GPUCulling] Loaded object_cull compute shader: OK");
        m_computeEnabled = true;
    } else {
        Msg("! [GPUCulling] object_cull.cs not found - GPU culling disabled");
        m_computeEnabled = false;
        m_initialized = true;
        return;
    }

    CreateBuffers(device);
    CreateComputePipeline(device);
    CreateDebugResources(device);

    m_initialized = true;
    Msg("* [GPUCulling] Initialized (max objects: %d)", m_maxObjects);
}

void GPUCullingManager::CreateBuffers(ng::RenderDevice* device)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_Objects";
        desc.byteSize = m_maxObjects * sizeof(GPUObjectData);
        desc.structStride = sizeof(GPUObjectData);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        m_objectBuffer = nvDevice->createBuffer(desc);
        if (!m_objectBuffer) {
            Msg("! [GPUCulling] Failed to create object buffer");
            m_computeEnabled = false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_VisibleIndices";
        desc.byteSize = m_maxObjects * sizeof(u32);
        desc.structStride = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_visibleIndexBuffer = nvDevice->createBuffer(desc);
        if (!m_visibleIndexBuffer) {
            Msg("! [GPUCulling] Failed to create visible index buffer");
            m_computeEnabled = false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_VisibleCount";
        desc.byteSize = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_visibleCountBuffer = nvDevice->createBuffer(desc);
        if (!m_visibleCountBuffer) {
            Msg("! [GPUCulling] Failed to create visible count buffer");
            m_computeEnabled = false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DrawArgs";
        desc.byteSize = m_maxObjects * sizeof(IndirectDrawArgs);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = false;

        m_drawArgsBuffer = nvDevice->createBuffer(desc);
        if (!m_drawArgsBuffer) {
            Msg("! [GPUCulling] Failed to create draw args buffer");
            m_computeEnabled = false;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_Params";
        desc.byteSize = sizeof(CullParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;

        m_cullParamsCB = nvDevice->createBuffer(desc);
        if (!m_cullParamsCB) {
            Msg("! [GPUCulling] Failed to create constant buffer");
            m_computeEnabled = false;
        }
    }

    {
        nvrhi::SamplerDesc desc;
        desc.minFilter = false;
        desc.magFilter = false;
        desc.mipFilter = false;
        desc.addressU = nvrhi::SamplerAddressMode::Clamp;
        desc.addressV = nvrhi::SamplerAddressMode::Clamp;
        desc.addressW = nvrhi::SamplerAddressMode::Clamp;

        m_pointSampler = nvDevice->createSampler(desc);
    }
}

void GPUCullingManager::CreateComputePipeline(ng::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::ConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::Texture_SRV(1),
            nvrhi::BindingLayoutItem::Sampler(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(1),
            nvrhi::BindingLayoutItem::RawBuffer_UAV(2)
        };

        m_cullLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_cullLayout) {
            Msg("! [GPUCulling] Failed to create binding layout");
            m_computeEnabled = false;
            return;
        }
    }

    {
        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_object_cull_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_cullLayout };

        m_cullPipeline = nvDevice->createComputePipeline(pipeDesc);
        if (!m_cullPipeline) {
            Msg("! [GPUCulling] Failed to create compute pipeline");
            m_computeEnabled = false;
            return;
        }
    }

    Msg("* [GPUCulling] Compute pipeline created successfully");
}

void GPUCullingManager::Shutdown()
{
    m_objectBuffer = nullptr;
    m_visibleIndexBuffer = nullptr;
    m_visibleCountBuffer = nullptr;
    m_cullParamsCB = nullptr;
    m_cullPipeline = nullptr;
    m_cullLayout = nullptr;
    m_pointSampler = nullptr;

    m_debugBuffer = nullptr;
    m_debugComputeParamsCB = nullptr;
    m_debugGraphicsParamsCB = nullptr;
    m_debugComputePipeline = nullptr;
    m_debugComputeLayout = nullptr;
    m_debugGraphicsPipeline = nullptr;
    m_debugGraphicsLayout = nullptr;
    m_debugInputLayout = nullptr;

    m_initialized = false;
    m_computeEnabled = false;
    m_objectCount = 0;
}

void GPUCullingManager::UploadSceneObjects(ng::RenderContext* ctx, const GeometryCollector* geometry)
{
    if (!m_computeEnabled || !geometry)
        return;

    const auto& batches = geometry->GetBatches();
    m_objectCount = std::min(static_cast<u32>(batches.size()), m_maxObjects);

    if (m_objectCount == 0)
        return;

    m_objectData.clear();
    m_objectData.reserve(m_objectCount);
    m_drawArgsData.clear();
    m_drawArgsData.reserve(m_objectCount);

    for (u32 i = 0; i < m_objectCount; i++) {
        const auto& batch = batches[i];

        GPUObjectData obj;

        obj.position = batch.worldBoundsCenter;
        obj.radius = batch.worldBoundsRadius;

        obj.batchIndex = i;

        obj.flags = 0;
        if (batch.IsOpaque())
            obj.flags |= GPU_OBJECT_OPAQUE;
        if (batch.IsAlphaTested())
            obj.flags |= GPU_OBJECT_ALPHA_TEST;
        if (batch.IsStrictB2F())
            obj.flags |= GPU_OBJECT_TRANSPARENT;

        obj.pad0 = 0.0f;
        obj.pad1 = 0.0f;

        m_objectData.push_back(obj);

        IndirectDrawArgs args;
        args.indexCountPerInstance = batch.indexCount;
        args.instanceCount = 0;
        args.startIndexLocation = batch.startIndex;
        args.baseVertexLocation = batch.baseVertex;
        args.startInstanceLocation = 0;

        m_drawArgsData.push_back(args);
    }

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    cmdList->writeBuffer(m_objectBuffer, m_objectData.data(), m_objectCount * sizeof(GPUObjectData));
    cmdList->writeBuffer(m_drawArgsBuffer, m_drawArgsData.data(), m_objectCount * sizeof(IndirectDrawArgs));
}

void GPUCullingManager::ExtractFrustumPlanes(Fmatrix& M, Fvector4* outPlanes)
{
    CFrustum frustum;
    frustum.CreateFromMatrix(M, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

    for (u32 i = 0; i < frustum.p_count && i < 6; i++)
    {
        outPlanes[i].set(
            frustum.planes[i].n.x,
            frustum.planes[i].n.y,
            frustum.planes[i].n.z,
            frustum.planes[i].d
        );
    }
}

GPUCullOutput GPUCullingManager::SetupCullingPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hizPyramid,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels,
    const GeometryCollector* geometry)
{
    using namespace framegraph;

    GPUCullOutput output;
    output.maxObjects = m_maxObjects;

    if (!m_computeEnabled) {
        output.visibleIndices = VirtualResourceHandle();
        output.visibleCount = VirtualResourceHandle();
        output.drawArgsBuffer = VirtualResourceHandle();
        return output;
    }

    const auto& batches = geometry->GetBatches();
    m_objectCount = std::min(static_cast<u32>(batches.size()), m_maxObjects);

    if (m_objectCount == 0) {
        output.visibleIndices = VirtualResourceHandle();
        output.visibleCount = VirtualResourceHandle();
        output.drawArgsBuffer = VirtualResourceHandle();
        return output;
    }

    struct GPUCullPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle drawArgsBuffer;

        GPUCullingManager* manager;
        const GeometryCollector* geometry;
        u32 objectCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    ResourceDesc drawArgsDesc;
    drawArgsDesc.type = ResourceDesc::Type::Buffer;
    drawArgsDesc.debugName = "GPUCull_DrawArgs";
    drawArgsDesc.bufferSize = m_maxObjects * sizeof(IndirectDrawArgs);
    drawArgsDesc.structStride = sizeof(IndirectDrawArgs);
    drawArgsDesc.isUAV = true;
    drawArgsDesc.isTransient = false;

    VirtualResourceHandle drawArgsHandle = fg.ImportBuffer("gpu_cull_drawargs", m_drawArgsBuffer, drawArgsDesc);

    auto& passData = fg.addCallbackPass<GPUCullPassData>(
        "GPU Culling",

        [&, hizWidth, hizHeight, hizMipLevels, drawArgsHandle, geometry](FrameGraph& builder, PassHandle passHandle, GPUCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.manager = this;
            data.geometry = geometry;
            data.objectCount = m_objectCount;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);

            data.drawArgsBuffer = passBuilder.write(drawArgsHandle, ResourceState::UnorderedAccess);
        },

        [](const GPUCullPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_computeEnabled || data.objectCount == 0)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("GPU Culling");

            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

            mgr->UploadSceneObjects(ctx, data.geometry);

            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            if (!hizTexture) {
                Msg("! [GPUCulling] Hi-Z texture not available");
                cmdList->endMarker();
                return;
            }

            u32 zero = 0;
            cmdList->writeBuffer(mgr->m_visibleCountBuffer, &zero, sizeof(u32));

            CullParamsCB cb;
            cb.viewProj.transpose(Device.mFullTransform);
            cb.cameraPos = Device.vCameraPosition;
            cb.maxDistanceSq = 500.0f * 500.0f;
            cb.objectCount = data.objectCount;
            cb.hizWidth = data.hizWidth;
            cb.hizHeight = data.hizHeight;
            cb.hizMipLevels = data.hizMipLevels;

            mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);

            cmdList->writeBuffer(mgr->m_cullParamsCB, &cb, sizeof(cb));

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_cullParamsCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_objectBuffer),
                nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_visibleIndexBuffer),
                nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->m_visibleCountBuffer),
                nvrhi::BindingSetItem::RawBuffer_UAV(2, mgr->m_drawArgsBuffer)
            };

            nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_cullLayout);
            if (!bindingSet) {
                Msg("! [GPUCulling] Failed to create binding set");
                cmdList->endMarker();
                return;
            }

            nvrhi::ComputeState state;
            state.pipeline = mgr->m_cullPipeline;
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);

            u32 groupCount = (data.objectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
            cmdList->dispatch(groupCount, 1, 1);

            cmdList->endMarker();
        }
    );

    output.visibleIndices = VirtualResourceHandle();
    output.visibleCount = VirtualResourceHandle();
    output.drawArgsBuffer = passData.drawArgsBuffer;
    return output;
}

bool GPUCullingManager::IsDebugEnabled() const
{
    return ps_r4_debug_gpu_culling != 0 && m_computeEnabled && m_debugComputePipeline && m_debugGraphicsPipeline;
}

void GPUCullingManager::CreateDebugResources(ng::RenderDevice* device)
{
    if (!m_computeEnabled)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    s_object_cull_debug_cs.create("object_cull_debug");
    s_cull_debug_vs = RImplementation.Resources->_CreateVS("cull_debug");
    s_cull_debug_ps = RImplementation.Resources->_CreatePS("cull_debug");

    if (!s_object_cull_debug_cs || !s_object_cull_debug_cs->nvrhiShader) {
        Msg("! [GPUCulling] object_cull_debug.cs not found - debug visualization disabled");
        return;
    }
    if (!s_cull_debug_vs || !s_cull_debug_vs->nvrhiShader) {
        Msg("! [GPUCulling] cull_debug.vs not found - debug visualization disabled");
        return;
    }
    if (!s_cull_debug_ps || !s_cull_debug_ps->nvrhiShader) {
        Msg("! [GPUCulling] cull_debug.ps not found - debug visualization disabled");
        return;
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DebugData";
        desc.byteSize = m_maxObjects * sizeof(CullDebugData);
        desc.structStride = sizeof(CullDebugData);
        desc.canHaveUAVs = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = false;

        m_debugBuffer = nvDevice->createBuffer(desc);
        if (!m_debugBuffer) {
            Msg("! [GPUCulling] Failed to create debug buffer");
            return;
        }
    }

    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DebugComputeParams";
        desc.byteSize = sizeof(CullDebugParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;

        m_debugComputeParamsCB = nvDevice->createBuffer(desc);
        if (!m_debugComputeParamsCB) {
            Msg("! [GPUCulling] Failed to create debug compute constant buffer");
            return;
        }
    }
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DebugGraphicsParams";
        desc.byteSize = sizeof(CullDebugVSParamsCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = 16;

        m_debugGraphicsParamsCB = nvDevice->createBuffer(desc);
        if (!m_debugGraphicsParamsCB) {
            Msg("! [GPUCulling] Failed to create debug graphics constant buffer");
            return;
        }
    }

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Compute;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::ConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0),
            nvrhi::BindingLayoutItem::Texture_SRV(1),
            nvrhi::BindingLayoutItem::Sampler(0),
            nvrhi::BindingLayoutItem::StructuredBuffer_UAV(0)
        };

        m_debugComputeLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_debugComputeLayout) {
            Msg("! [GPUCulling] Failed to create debug compute binding layout");
            return;
        }

        nvrhi::ComputePipelineDesc pipeDesc;
        pipeDesc.CS = s_object_cull_debug_cs->nvrhiShader;
        pipeDesc.bindingLayouts = { m_debugComputeLayout };

        m_debugComputePipeline = nvDevice->createComputePipeline(pipeDesc);
        if (!m_debugComputePipeline) {
            Msg("! [GPUCulling] Failed to create debug compute pipeline");
            return;
        }
    }

    {
        nvrhi::BindingLayoutDesc layoutDesc;
        layoutDesc.visibility = nvrhi::ShaderType::Vertex | nvrhi::ShaderType::Pixel;
        layoutDesc.bindings = {
            nvrhi::BindingLayoutItem::ConstantBuffer(5),
            nvrhi::BindingLayoutItem::StructuredBuffer_SRV(0)
        };

        m_debugGraphicsLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_debugGraphicsLayout) {
            Msg("! [GPUCulling] Failed to create debug graphics binding layout");
            return;
        }

        nvrhi::GraphicsPipelineDesc pipeDesc;
        pipeDesc.VS = s_cull_debug_vs->nvrhiShader;
        pipeDesc.PS = s_cull_debug_ps->nvrhiShader;
        pipeDesc.bindingLayouts = { m_debugGraphicsLayout };
        pipeDesc.primType = nvrhi::PrimitiveType::TriangleStrip;
        pipeDesc.inputLayout = nullptr;

        pipeDesc.renderState.blendState.targets[0].setBlendEnable(true);
        pipeDesc.renderState.blendState.targets[0].setSrcBlend(nvrhi::BlendFactor::SrcAlpha);
        pipeDesc.renderState.blendState.targets[0].setDestBlend(nvrhi::BlendFactor::InvSrcAlpha);
        pipeDesc.renderState.blendState.targets[0].setBlendOp(nvrhi::BlendOp::Add);
        pipeDesc.renderState.blendState.targets[0].setSrcBlendAlpha(nvrhi::BlendFactor::One);
        pipeDesc.renderState.blendState.targets[0].setDestBlendAlpha(nvrhi::BlendFactor::Zero);
        pipeDesc.renderState.blendState.targets[0].setBlendOpAlpha(nvrhi::BlendOp::Add);

        pipeDesc.renderState.depthStencilState.setDepthTestEnable(false);
        pipeDesc.renderState.depthStencilState.setDepthWriteEnable(false);

        pipeDesc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

        nvrhi::FramebufferInfoEx framebufferInfo;
        framebufferInfo.addColorFormat(nvrhi::Format::RGBA16_FLOAT);
        framebufferInfo.setDepthFormat(nvrhi::Format::D32);

        m_debugGraphicsPipeline = nvDevice->createGraphicsPipeline(pipeDesc, framebufferInfo);
        if (!m_debugGraphicsPipeline) {
            Msg("! [GPUCulling] Failed to create debug graphics pipeline");
            return;
        }
    }

    Msg("* [GPUCulling] Debug visualization resources created");
}

void GPUCullingManager::SetupDebugVisualizationPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle hizPyramid,
    framegraph::VirtualResourceHandle colorTarget,
    framegraph::VirtualResourceHandle depthTarget,
    u32 hizWidth,
    u32 hizHeight,
    u32 hizMipLevels)
{
    using namespace framegraph;

    if (!IsDebugEnabled() || m_objectCount == 0)
        return;

    struct DebugPassData {
        VirtualResourceHandle hizPyramid;
        VirtualResourceHandle colorTarget;
        VirtualResourceHandle depthTarget;

        GPUCullingManager* manager;
        u32 objectCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    fg.addCallbackPass<DebugPassData>(
        "GPU Culling Debug",

        [&, hizWidth, hizHeight, hizMipLevels](FrameGraph& builder, PassHandle passHandle, DebugPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.manager = this;
            data.objectCount = m_objectCount;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);

            data.colorTarget = passBuilder.write(colorTarget, ResourceState::RenderTarget);

            data.depthTarget = passBuilder.read(depthTarget, ResourceState::DepthStencilRead);
        },

        [](const DebugPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

            cmdList->beginMarker("GPU Culling Debug");

            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            nvrhi::ITexture* colorTexture = fg.GetPhysicalTexture(data.colorTarget);
            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depthTarget);

            if (!hizTexture || !colorTexture || !depthTexture) {
                Msg("! [GPUCulling] Debug pass missing textures");
                cmdList->endMarker();
                return;
            }

            {
                cmdList->beginMarker("Debug Compute");

                CullDebugParamsCB cb;
                cb.viewProj.transpose(Device.mFullTransform);
                cb.cameraPos = Device.vCameraPosition;
                cb.maxDistanceSq = 500.0f * 500.0f;
                cb.objectCount = data.objectCount;
                cb.hizWidth = data.hizWidth;
                cb.hizHeight = data.hizHeight;
                cb.hizMipLevels = data.hizMipLevels;
                cb.occluderThreshold = 50.0f;

                mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);

                cmdList->writeBuffer(mgr->m_debugComputeParamsCB, &cb, sizeof(cb));

                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_debugComputeParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_objectBuffer),
                    nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                    nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                    nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_debugBuffer)
                };

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_debugComputeLayout);

                nvrhi::ComputeState state;
                state.pipeline = mgr->m_debugComputePipeline;
                state.bindings = { bindingSet };
                cmdList->setComputeState(state);

                u32 groupCount = (data.objectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
                cmdList->dispatch(groupCount, 1, 1);

                cmdList->endMarker();
            }

            cmdList->setBufferState(mgr->m_debugBuffer, nvrhi::ResourceStates::ShaderResource);

            {
                cmdList->beginMarker("Debug Render");

                CullDebugVSParamsCB vsCB;
                vsCB.view.transpose(Device.mView);
                vsCB.viewProj.transpose(Device.mFullTransform);
                vsCB.objectCount = data.objectCount;
                vsCB.wireframeAlpha = 0.7f;

                cmdList->writeBuffer(mgr->m_debugGraphicsParamsCB, &vsCB, sizeof(vsCB));

                nvrhi::BindingSetDesc bindDesc;
                bindDesc.bindings = {
                    nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_debugGraphicsParamsCB),
                    nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_debugBuffer)
                };

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_debugGraphicsLayout);

                nvrhi::FramebufferDesc fbDesc;
                fbDesc.addColorAttachment(colorTexture);
                fbDesc.setDepthAttachment(depthTexture);
                nvrhi::FramebufferHandle framebuffer = nvDevice->createFramebuffer(fbDesc);

                nvrhi::GraphicsState gfxState;
                gfxState.pipeline = mgr->m_debugGraphicsPipeline;
                gfxState.bindings = { bindingSet };
                gfxState.framebuffer = framebuffer;

                nvrhi::Viewport viewport;
                viewport.minX = 0;
                viewport.minY = 0;
                viewport.maxX = static_cast<float>(colorTexture->getDesc().width);
                viewport.maxY = static_cast<float>(colorTexture->getDesc().height);
                viewport.minZ = 0.0f;
                viewport.maxZ = 1.0f;
                gfxState.viewport.addViewportAndScissorRect(viewport);

                cmdList->setGraphicsState(gfxState);

                nvrhi::DrawArguments drawArgs;
                drawArgs.vertexCount = 4;
                drawArgs.instanceCount = data.objectCount;
                drawArgs.startVertexLocation = 0;
                drawArgs.startInstanceLocation = 0;
                cmdList->draw(drawArgs);

                cmdList->endMarker();
            }

            cmdList->setBufferState(mgr->m_debugBuffer, nvrhi::ResourceStates::UnorderedAccess);

            cmdList->endMarker();
        }
    );
}

}
