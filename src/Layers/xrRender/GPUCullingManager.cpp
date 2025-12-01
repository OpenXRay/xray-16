// xrRender/GPUCullingManager.cpp
#include "stdafx.h"
#include "GPUCullingManager.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE {

// ═══════════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════════

constexpr u32 MAX_CULLING_OBJECTS = 65536;  // Maximum objects per frame
constexpr u32 CULL_THREAD_GROUP_SIZE = 64;  // Must match [numthreads] in shader

// ═══════════════════════════════════════════════════════
//  CONSTANT BUFFER STRUCTURE (must match HLSL)
// ═══════════════════════════════════════════════════════

struct CullParamsCB {
    Fmatrix viewProj;           // View-projection matrix
    Fvector cameraPos;          // Camera world position
    float maxDistanceSq;        // Maximum render distance (squared)
    Fvector4 frustumPlanes[6];  // View frustum planes
    u32 objectCount;            // Total objects to cull
    u32 hizWidth;               // Hi-Z pyramid width
    u32 hizHeight;              // Hi-Z pyramid height
    u32 hizMipLevels;           // Hi-Z mip levels
};

// ═══════════════════════════════════════════════════════
//  STATIC STATE
// ═══════════════════════════════════════════════════════

static ref_cs s_object_cull_cs;

// ═══════════════════════════════════════════════════════
//  CONSTRUCTOR / DESTRUCTOR
// ═══════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════
//  INITIALIZATION
// ═══════════════════════════════════════════════════════

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

    // Load compute shader
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

    m_initialized = true;
    Msg("* [GPUCulling] Initialized (max objects: %d)", m_maxObjects);
}

void GPUCullingManager::CreateBuffers(ng::RenderDevice* device)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    // Object buffer: Structured buffer of GPUObjectData
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

    // Visible index buffer: Structured buffer of u32
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

    // Visible count buffer: Single u32 atomic counter
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_VisibleCount";
        desc.byteSize = sizeof(u32);
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;  // For InterlockedAdd
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;

        m_visibleCountBuffer = nvDevice->createBuffer(desc);
        if (!m_visibleCountBuffer) {
            Msg("! [GPUCulling] Failed to create visible count buffer");
            m_computeEnabled = false;
        }
    }

    // Draw arguments buffer: Indirect draw args for each batch
    // NOTE: Cannot use structured buffer with indirect args on D3D11
    // Must use raw buffer (byte address buffer) instead
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "GPUCull_DrawArgs";
        desc.byteSize = m_maxObjects * sizeof(IndirectDrawArgs);
        // No structStride - use as raw buffer
        desc.canHaveUAVs = true;
        desc.canHaveRawViews = true;  // Allow RWByteAddressBuffer access
        desc.isDrawIndirectArgs = true;  // CRITICAL: Allows use with DrawIndexedIndirect
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = false;  // Will transition between UAV and IndirectArgument

        m_drawArgsBuffer = nvDevice->createBuffer(desc);
        if (!m_drawArgsBuffer) {
            Msg("! [GPUCulling] Failed to create draw args buffer");
            m_computeEnabled = false;
        }
    }

    // Constant buffer
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

    // Point sampler for Hi-Z (false = point filtering, true = linear)
    {
        nvrhi::SamplerDesc desc;
        desc.minFilter = false;  // Point filtering
        desc.magFilter = false;  // Point filtering
        desc.mipFilter = false;  // Point filtering
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

    // Create binding layout for main culling pass
    // Matches object_cull.cs bindings:
    // b5: CullParams (constant buffer)
    // t0: g_Objects (structured buffer SRV)
    // t1: g_HiZPyramid (texture SRV)
    // s0: g_PointSampler
    // u0: g_VisibleIndices (structured buffer UAV)
    // u1: g_VisibleCount (raw buffer UAV)
    // u2: g_DrawArgs (raw buffer UAV - can't be structured with indirect args on D3D11)
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
            nvrhi::BindingLayoutItem::RawBuffer_UAV(2)  // Draw args (raw buffer for D3D11 compatibility)
        };

        m_cullLayout = nvDevice->createBindingLayout(layoutDesc);
        if (!m_cullLayout) {
            Msg("! [GPUCulling] Failed to create binding layout");
            m_computeEnabled = false;
            return;
        }
    }

    // Create compute pipeline for main culling
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

    m_initialized = false;
    m_computeEnabled = false;
    m_objectCount = 0;
}

// ═══════════════════════════════════════════════════════
//  UPLOAD SCENE OBJECTS
// ═══════════════════════════════════════════════════════

void GPUCullingManager::UploadSceneObjects(ng::RenderContext* ctx, const GeometryCollector* geometry)
{
    if (!m_computeEnabled || !geometry)
        return;

    const auto& batches = geometry->GetBatches();
    m_objectCount = std::min(static_cast<u32>(batches.size()), m_maxObjects);

    if (m_objectCount == 0)
        return;

    // Build object data and draw args arrays
    m_objectData.clear();
    m_objectData.reserve(m_objectCount);
    m_drawArgsData.clear();
    m_drawArgsData.reserve(m_objectCount);

    for (u32 i = 0; i < m_objectCount; i++) {
        const auto& batch = batches[i];

        // ─────────────────────────────────────────────────────
        //  BUILD OBJECT DATA (for culling)
        // ─────────────────────────────────────────────────────
        GPUObjectData obj;

        // Use pre-computed world-space bounding sphere from batch
        // This handles different visual types correctly:
        // - Static geometry: identity transform applied (no-op)
        // - Trees: no transform applied (sphere already world-space)
        // - Dynamic objects: worldMatrix transform applied
        obj.position = batch.worldBoundsCenter;
        obj.radius = batch.worldBoundsRadius;

        obj.batchIndex = i;

        // Set flags based on batch properties
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

        // ─────────────────────────────────────────────────────
        //  BUILD DRAW ARGS (for indirect draw)
        // ─────────────────────────────────────────────────────
        IndirectDrawArgs args;
        args.indexCountPerInstance = batch.indexCount;
        args.instanceCount = 0;  // Will be set to 1 by culling shader if visible
        args.startIndexLocation = batch.startIndex;
        args.baseVertexLocation = batch.baseVertex;
        args.startInstanceLocation = 0;

        m_drawArgsData.push_back(args);
    }

    // Upload to GPU
    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    cmdList->writeBuffer(m_objectBuffer, m_objectData.data(), m_objectCount * sizeof(GPUObjectData));
    cmdList->writeBuffer(m_drawArgsBuffer, m_drawArgsData.data(), m_objectCount * sizeof(IndirectDrawArgs));
}

// ═══════════════════════════════════════════════════════
//  FRUSTUM PLANE EXTRACTION
// ═══════════════════════════════════════════════════════

void GPUCullingManager::ExtractFrustumPlanes(const Fmatrix& M, Fvector4* outPlanes)
{
    // Extract frustum planes from view-projection matrix
    // EXACTLY matches X-Ray's CFrustum::CreateFromMatrix (src/xrCDB/Frustum.cpp)
    //
    // X-Ray convention: planes have normals pointing INWARD (toward frustum interior)
    // - classify(P) > 0 means OUTSIDE (to be culled)
    // - classify(P) <= 0 means INSIDE (visible)
    //
    // Fmatrix layout:
    //   _11 _12 _13 _14  <- row 0 (i vector + tx)
    //   _21 _22 _23 _24  <- row 1 (j vector + ty)
    //   _31 _32 _33 _34  <- row 2 (k vector + tz)
    //   _41 _42 _43 _44  <- row 3 (c translation + tw)

    // Left plane (matches X-Ray Frustum.cpp line 476-480)
    outPlanes[0].x = -(M._14 + M._11);
    outPlanes[0].y = -(M._24 + M._21);
    outPlanes[0].z = -(M._34 + M._31);
    outPlanes[0].w = -(M._44 + M._41);

    // Right plane (matches X-Ray Frustum.cpp line 486-490)
    outPlanes[1].x = -(M._14 - M._11);
    outPlanes[1].y = -(M._24 - M._21);
    outPlanes[1].z = -(M._34 - M._31);
    outPlanes[1].w = -(M._44 - M._41);

    // Top plane (matches X-Ray Frustum.cpp line 496-500)
    outPlanes[2].x = -(M._14 - M._12);
    outPlanes[2].y = -(M._24 - M._22);
    outPlanes[2].z = -(M._34 - M._32);
    outPlanes[2].w = -(M._44 - M._42);

    // Bottom plane (matches X-Ray Frustum.cpp line 506-510)
    outPlanes[3].x = -(M._14 + M._12);
    outPlanes[3].y = -(M._24 + M._22);
    outPlanes[3].z = -(M._34 + M._32);
    outPlanes[3].w = -(M._44 + M._42);

    // Far plane (matches X-Ray Frustum.cpp line 516-520)
    outPlanes[4].x = -(M._14 - M._13);
    outPlanes[4].y = -(M._24 - M._23);
    outPlanes[4].z = -(M._34 - M._33);
    outPlanes[4].w = -(M._44 - M._43);

    // Near plane (matches X-Ray Frustum.cpp line 526-529)
    outPlanes[5].x = -(M._14 + M._13);
    outPlanes[5].y = -(M._24 + M._23);
    outPlanes[5].z = -(M._34 + M._33);
    outPlanes[5].w = -(M._44 + M._43);

    // Normalize planes (required for correct distance calculations with sphere radius)
    for (int i = 0; i < 6; i++) {
        float len = sqrt(outPlanes[i].x * outPlanes[i].x +
                        outPlanes[i].y * outPlanes[i].y +
                        outPlanes[i].z * outPlanes[i].z);
        if (len > 0.0001f) {
            outPlanes[i].x /= len;
            outPlanes[i].y /= len;
            outPlanes[i].z /= len;
            outPlanes[i].w /= len;
        }
    }
}

// ═══════════════════════════════════════════════════════
//  SETUP CULLING PASS
// ═══════════════════════════════════════════════════════

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

    // Early out if not enabled
    if (!m_computeEnabled) {
        output.visibleIndices = VirtualResourceHandle();
        output.visibleCount = VirtualResourceHandle();
        output.drawArgsBuffer = VirtualResourceHandle();
        return output;
    }

    // Pre-compute object count for pass data
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
        VirtualResourceHandle drawArgsBuffer;  // For framegraph tracking

        GPUCullingManager* manager;
        const GeometryCollector* geometry;  // For uploading during execute
        u32 objectCount;
        u32 hizWidth;
        u32 hizHeight;
        u32 hizMipLevels;
    };

    // Import draw args buffer into framegraph for proper state tracking
    // This allows forward pass to properly transition the buffer to IndirectArgument state
    ResourceDesc drawArgsDesc;
    drawArgsDesc.type = ResourceDesc::Type::Buffer;
    drawArgsDesc.debugName = "GPUCull_DrawArgs";
    drawArgsDesc.bufferSize = m_maxObjects * sizeof(IndirectDrawArgs);
    drawArgsDesc.structStride = sizeof(IndirectDrawArgs);
    drawArgsDesc.isUAV = true;
    drawArgsDesc.isTransient = false;  // Persistent - forward pass needs it

    VirtualResourceHandle drawArgsHandle = fg.ImportBuffer("gpu_cull_drawargs", m_drawArgsBuffer, drawArgsDesc);

    auto& passData = fg.addCallbackPass<GPUCullPassData>(
        "GPU Culling",

        // Setup lambda
        [&, hizWidth, hizHeight, hizMipLevels, drawArgsHandle, geometry](FrameGraph& builder, PassHandle passHandle, GPUCullPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.manager = this;
            data.geometry = geometry;  // Capture for upload during execute
            data.objectCount = m_objectCount;
            data.hizWidth = hizWidth;
            data.hizHeight = hizHeight;
            data.hizMipLevels = hizMipLevels;

            // Read Hi-Z pyramid
            data.hizPyramid = passBuilder.read(hizPyramid, ResourceState::ShaderResource);

            // Write draw args buffer (culling shader enables visible draws)
            data.drawArgsBuffer = passBuilder.write(drawArgsHandle, ResourceState::UnorderedAccess);
        },

        // Execute lambda
        [](const GPUCullPassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            GPUCullingManager* mgr = data.manager;
            if (!mgr->m_computeEnabled || data.objectCount == 0)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("GPU Culling");

            nvrhi::IDevice* nvDevice = mgr->m_device->GetNVRHIDevice();

            // ─────────────────────────────────────────────────────
            //  UPLOAD SCENE OBJECTS (must happen during execute, not setup)
            // ─────────────────────────────────────────────────────
            // This ensures we use the correct command list
            mgr->UploadSceneObjects(ctx, data.geometry);

            // Get Hi-Z texture
            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);
            if (!hizTexture) {
                Msg("! [GPUCulling] Hi-Z texture not available");
                cmdList->endMarker();
                return;
            }

            // Clear visible count to 0
            u32 zero = 0;
            cmdList->writeBuffer(mgr->m_visibleCountBuffer, &zero, sizeof(u32));

            // Fill constant buffer
            CullParamsCB cb;
            cb.viewProj = Device.mFullTransform;
            cb.cameraPos = Device.vCameraPosition;
            cb.maxDistanceSq = 500.0f * 500.0f;  // 500m max distance
            cb.objectCount = data.objectCount;
            cb.hizWidth = data.hizWidth;
            cb.hizHeight = data.hizHeight;
            cb.hizMipLevels = data.hizMipLevels;

            mgr->ExtractFrustumPlanes(Device.mFullTransform, cb.frustumPlanes);

            cmdList->writeBuffer(mgr->m_cullParamsCB, &cb, sizeof(cb));

            // Create binding set (includes draw args UAV at u2)
            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(5, mgr->m_cullParamsCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(0, mgr->m_objectBuffer),
                nvrhi::BindingSetItem::Texture_SRV(1, hizTexture),
                nvrhi::BindingSetItem::Sampler(0, mgr->m_pointSampler),
                nvrhi::BindingSetItem::StructuredBuffer_UAV(0, mgr->m_visibleIndexBuffer),
                nvrhi::BindingSetItem::RawBuffer_UAV(1, mgr->m_visibleCountBuffer),
                nvrhi::BindingSetItem::RawBuffer_UAV(2, mgr->m_drawArgsBuffer)  // Raw buffer for D3D11
            };

            nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, mgr->m_cullLayout);
            if (!bindingSet) {
                Msg("! [GPUCulling] Failed to create binding set");
                cmdList->endMarker();
                return;
            }

            // Set compute state and dispatch
            nvrhi::ComputeState state;
            state.pipeline = mgr->m_cullPipeline;
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);

            u32 groupCount = (data.objectCount + CULL_THREAD_GROUP_SIZE - 1) / CULL_THREAD_GROUP_SIZE;
            cmdList->dispatch(groupCount, 1, 1);

            // NOTE: State transition to IndirectArgument is handled by FrameGraph
            // Forward pass declares read(drawArgsBuffer, IndirectArgument) dependency
            // which ensures proper barrier insertion between passes

            cmdList->endMarker();
        }
    );

    output.visibleIndices = VirtualResourceHandle();  // Not using framegraph for these
    output.visibleCount = VirtualResourceHandle();
    output.drawArgsBuffer = passData.drawArgsBuffer;
    return output;
}

} // namespace xray::render::RENDER_NAMESPACE
