#include "stdafx.h"
#include "RTAccelStructManager.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"
#include "Layers/xrRender/Bindless/TerrainMaterialBuffer.h"
#include "Layers/xrRender/Bindless/VariantBuffer.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/SkinnedGeometryPools.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/ResourceManager/FGResourceManager.h"
#include "xrEngine/IRenderBackend.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include <nvrhi/utils.h>
#include <tuple>

extern ENGINE_API float ps_r3_grass_blade_width;
extern ENGINE_API float ps_r3_grass_blade_height;
extern ENGINE_API float ps_r3_grass_wind_displacement;

namespace xray::render::fg
{

nvrhi::ComputePipelineHandle RTAccelStructManager::s_skinPipeline;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_skinLayout;
BufferHandle RTAccelStructManager::s_skinCB;
nvrhi::ComputePipelineHandle RTAccelStructManager::s_grassPipeline;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_grassLayout;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_grassSourceLayout;
BufferHandle RTAccelStructManager::s_grassCB;
nvrhi::ComputePipelineHandle RTAccelStructManager::s_billboardPipeline;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_billboardLayout;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_billboardSourceLayout;
BufferHandle RTAccelStructManager::s_billboardCB;

RTTextureBindings::RTTextureBindings() = default;

RTTextureBindings::~RTTextureBindings()
{
    if (m_backend)
        m_backend->ReleaseBindlessTextures(m_indices.data(), u32(m_indices.size()));
}

void RTTextureBindings::Capture(xr_vector<u32>& indices)
{
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    auto* backend = GEnv.Backend;
    R_ASSERT(backend && backend->GetBindlessDescriptorTable());
    R_ASSERT(!m_backend || (m_backend == backend && m_table == backend->GetBindlessDescriptorTable()));
    if (m_backend && m_indices == indices)
        return;
    R_ASSERT2(backend->RetainBindlessTextures(indices.data(), u32(indices.size())),
        "[RT] a scene material references an unavailable bindless texture");
    if (m_backend)
        m_backend->ReleaseBindlessTextures(m_indices.data(), u32(m_indices.size()));
    m_indices.swap(indices);
    m_backend = backend;
    m_table = backend->GetBindlessDescriptorTable();
    m_textures.clear();
    for (u32 index : m_indices)
    {
        auto* texture = backend->GetBindlessTexture(index);
        R_ASSERT(texture);
        m_textures.insert(texture);
    }
}

nvrhi::IDescriptorTable* RTTextureBindings::GetTable() const
{
    return m_table.Get();
}

const xr_set<nvrhi::ITexture*>& RTTextureBindings::GetTextures() const
{
    return m_textures;
}

void RTAccelStructManager::AppendMaterialTextures(u32 materialID, bool terrain)
{
    auto append = [&](u32 index)
    {
        if (index != bindless::INVALID_TEXTURE_INDEX)
            m_textureScratch.push_back(index);
    };
    if (terrain)
    {
        const auto& materials = bindless::TerrainMaterialBuffer::Instance();
        R_ASSERT(materialID < materials.GetMaterialCount());
        const auto& material = *materials.GetMaterial(materialID);
        for (u32 index : { material.baseAlbedoIndex, material.blendMaskIndex,
            material.detailR_Index, material.detailG_Index, material.detailB_Index, material.detailA_Index,
            material.normalR_Index, material.normalG_Index, material.normalB_Index, material.normalA_Index,
            material.pbrR_Index, material.pbrG_Index, material.pbrB_Index, material.pbrA_Index })
        {
            append(index);
        }
    }
    else
    {
        const auto& materials = bindless::MaterialBuffer::Instance();
        R_ASSERT(materialID < materials.GetMaterialCount());
        const auto& material = *materials.GetMaterial(materialID);
        for (u32 index : { material.diffuseIndex, material.normalIndex, material.detailIndex, material.pbrIndex })
            append(index);
    }
}

static framegraph::VirtualResourceHandle ImportRTBuffer(framegraph::FrameGraph& graph,
    const char* name, nvrhi::IBuffer* buffer)
{
    if (!buffer)
        return {};
    framegraph::ResourceDesc desc;
    desc.type = framegraph::ResourceDesc::Type::Buffer;
    desc.debugName = name;
    desc.bufferSize = buffer->getDesc().byteSize;
    desc.structStride = buffer->getDesc().structStride;
    desc.isUAV = buffer->getDesc().canHaveUAVs;
    desc.isIndirectArgs = buffer->getDesc().isDrawIndirectArgs;
    desc.isTransient = false;
    return graph.ImportBuffer(name, buffer, desc);
}

static void EnsureRTBuffer(nvrhi::IDevice* device, nvrhi::BufferHandle& buffer,
    const nvrhi::BufferDesc& desc)
{
    R_ASSERT(desc.byteSize != 0);
    if (!buffer || buffer->getDesc().byteSize < desc.byteSize ||
        buffer->getDesc().structStride != desc.structStride)
    {
        buffer = device->createBuffer(desc);
        R_ASSERT2(buffer, "[RT] buffer allocation failed");
    }
}

static nvrhi::BufferDesc RTVertexBufferDesc(const char* name, u64 size)
{
    nvrhi::BufferDesc desc;
    desc.debugName = name;
    desc.byteSize = size;
    desc.canHaveRawViews = true;
    desc.canHaveUAVs = true;
    desc.isAccelStructBuildInput = true;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    return desc;
}

static nvrhi::rt::GeometryDesc RTTriangles(nvrhi::IBuffer* vertices,
    nvrhi::IBuffer* indices, u32 stride, u32 firstVertex, u32 vertexCount,
    u32 firstIndex, u32 indexCount, bool opaque)
{
    R_ASSERT(vertices && indices && vertexCount && indexCount && indexCount % 3 == 0);
    R_ASSERT(u64(firstVertex + u64(vertexCount)) * stride <= vertices->getDesc().byteSize);
    R_ASSERT(u64(firstIndex + u64(indexCount)) * sizeof(u32) <= indices->getDesc().byteSize);
    nvrhi::rt::GeometryTriangles triangles;
    triangles.setVertexBuffer(vertices).setVertexFormat(nvrhi::Format::RGB32_FLOAT)
        .setVertexStride(stride).setVertexOffset(u64(firstVertex) * stride)
        .setVertexCount(vertexCount).setIndexBuffer(indices)
        .setIndexFormat(nvrhi::Format::R32_UINT).setIndexOffset(u64(firstIndex) * sizeof(u32))
        .setIndexCount(indexCount);
    nvrhi::rt::GeometryDesc desc;
    desc.setTriangles(triangles).setFlags(opaque ? nvrhi::rt::GeometryFlags::Opaque :
        nvrhi::rt::GeometryFlags::None);
    return desc;
}

static nvrhi::rt::InstanceDesc RTInstance(nvrhi::rt::IAccelStruct* blas,
    u32 firstBatch, const Fmatrix& world, bool opaque)
{
    R_ASSERT(firstBatch < (1u << 24));
    nvrhi::rt::AffineTransform transform;
    transform[0] = world._11; transform[1] = world._21;
    transform[2] = world._31; transform[3] = world._41;
    transform[4] = world._12; transform[5] = world._22;
    transform[6] = world._32; transform[7] = world._42;
    transform[8] = world._13; transform[9] = world._23;
    transform[10] = world._33; transform[11] = world._43;
    nvrhi::rt::InstanceDesc result;
    result.setTransform(transform).setInstanceID(firstBatch).setInstanceMask(0x01)
        .setFlags(opaque ? nvrhi::rt::InstanceFlags::ForceOpaque :
            nvrhi::rt::InstanceFlags::TriangleCullDisable).setBLAS(blas);
    return result;
}

bool RTAccelStructManager::s_skinAttempted = false;
bool RTAccelStructManager::s_grassAttempted = false;
bool RTAccelStructManager::s_billboardAttempted = false;

void RTAccelStructManager::Initialize(RenderDevice* device)
{
    m_device = device;
    auto* native = device->GetNVRHIDevice();
    m_rtSupported = native->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct) &&
        native->queryFeatureSupport(nvrhi::Feature::RayQuery);
}

void RTAccelStructManager::Shutdown()
{
    if (GEnv.Backend)
    {
        GEnv.Backend->WaitForIdle();
        for (const auto& record : m_leases)
            GEnv.Backend->ReleaseSubmissionLease(record.lease);
    }
    m_leases.clear();
    m_scene.reset();
    m_staticGeometry.reset();
    m_generations.clear();
    m_textureScratch.clear();
    m_textureScratch.shrink_to_fit();
    InvalidateShaderPipelines();
    m_device = nullptr;
    m_rtSupported = false;
}

void RTAccelStructManager::InvalidateShaderPipelines()
{
    s_skinPipeline = nullptr;
    s_skinLayout = nullptr;
    s_skinCB = {};
    s_grassPipeline = nullptr;
    s_grassLayout = nullptr;
    s_grassCB = {};
    s_grassSourceLayout = nullptr;
    s_billboardPipeline = nullptr;
    s_billboardLayout = nullptr;
    s_billboardSourceLayout = nullptr;
    s_billboardCB = {};
    s_skinAttempted = false;
    s_grassAttempted = false;
    s_billboardAttempted = false;
}

bool RTAccelStructManager::IsReady() const
{
    if (!m_scene ||
        !m_scene->tlas ||
        !m_scene->geometry ||
        !m_scene->geometry->vertices ||
        !m_scene->geometry->indices ||
        !m_scene->batchInfo ||
        !m_scene->materials ||
        !m_scene->terrainMaterials ||
        !m_scene->variants ||
        !m_scene->textures.GetTable())
        return false;
    if (m_scene->counts.skinned && (!m_scene->skinnedVertices || !m_scene->skinnedIndices || !m_scene->skinBuild.handle))
        return false;
    if (m_scene->counts.grass && (!m_scene->grassVertices || !m_scene->grassIndices || !m_scene->grassBuild.handle))
        return false;
    return true;
}

bool RTAccelStructManager::IsSupported() const
{
    return m_rtSupported;
}

const RTBatchCounts& RTAccelStructManager::GetBatchCounts() const
{
    R_ASSERT(m_scene);
    return m_scene->counts;
}

u32 RTAccelStructManager::GetDetailAtlasIndex() const
{
    return m_scene ? m_scene->detailAtlasIndex : 0;
}

void RTAccelStructManager::RetireScenes()
{
    for (auto it = m_leases.begin(); it != m_leases.end();)
    {
        const auto state = GEnv.Backend->PollSubmissionLease(it->lease);
        if (state == IRenderBackend::SubmissionLeaseState::Open ||
            state == IRenderBackend::SubmissionLeaseState::Pending)
        {
            ++it;
            continue;
        }
        R_ASSERT2(state != IRenderBackend::SubmissionLeaseState::Unknown,
            "[RT] scene generation lost its submission lease");
        GEnv.Backend->ReleaseSubmissionLease(it->lease);
        R_ASSERT(it->scene->leases != 0);
        --it->scene->leases;
        if (state == IRenderBackend::SubmissionLeaseState::Failed)
        {
            it->scene->recorded = false;
            if (it->scene->geometry == m_staticGeometry)
            {
                m_scene.reset();
                m_staticGeometry.reset();
            }
            else if (it->scene == m_scene)
                m_scene.reset();
        }
        if (it->scene->leases == 0 && it->scene->recorded)
        {
            it->scene->skinIndexData.clear();
            it->scene->skinJobs.clear();
            it->scene->skinSources.clear();
            it->scene->grassJobs.clear();
            it->scene->bones = nullptr;
            it->scene->sourceMaterials = nullptr;
            it->scene->sourceTerrainMaterials = nullptr;
            it->scene->sourceVariants = nullptr;
            it->scene->grassFrame.reset();
            it->scene->grassPipeline = nullptr;
            it->scene->grassLayout = nullptr;
            it->scene->grassWind = nullptr;
        }
        it = m_leases.erase(it);
    }
}

void RTAccelStructManager::HashSceneData(u64& signature, const void* data, size_t size)
{
    const auto* bytes = static_cast<const u8*>(data);
    for (size_t i = 0; i < size; ++i)
    {
        signature ^= bytes[i];
        signature *= 1099511628211ull;
    }
}

u64 RTAccelStructManager::ComputeStaticSignature(const GPUCullingManager* gpu) const
{
    u64 signature = 14695981039346656037ull;
    const auto* vertices = gpu->GetRTVertexBuffer();
    const auto* indices = gpu->GetRTIndexBuffer();
    HashSceneData(signature, &vertices, sizeof(vertices));
    HashSceneData(signature, &indices, sizeof(indices));
    auto append = [&](const auto& values)
    {
        const size_t count = values.size();
        HashSceneData(signature, &count, sizeof(count));
        HashSceneData(signature, values.data(), count * sizeof(*values.data()));
    };
    append(gpu->GetStaticDrawArgsData());
    const auto& instances = gpu->GetStaticInstanceData();
    const size_t instanceCount = instances.size();
    HashSceneData(signature, &instanceCount, sizeof(instanceCount));
    for (const auto& instance : instances)
        HashSceneData(signature, &instance.world, sizeof(instance.world));
    append(gpu->GetStaticBatchVertexCounts());
    append(gpu->GetStaticMaterialIDData());
    append(gpu->GetTerrainDrawArgsData());
    append(gpu->GetTerrainMaterialIDData());
    append(gpu->GetTransparentDrawArgsData());
    append(gpu->GetTransparentMaterialIDData());
    return signature;
}

u64 RTAccelStructManager::ComputeSceneSignature(const GPUCullingManager* gpu, const FGDetailManager* detail,
    const xr_vector<GeometryBatch>& world, const xr_vector<GeometryBatch>& hud) const
{
    u64 signature = m_staticSignature;
    auto append = [&](const auto& value)
    {
        HashSceneData(signature, &value, sizeof(value));
    };
    append(bindless::MaterialBuffer::Instance().GetRevision());
    append(bindless::TerrainMaterialBuffer::Instance().GetRevision());
    append(bindless::VariantBuffer::Instance().GetRevision());
    auto appendBatches = [&](const xr_vector<GeometryBatch>& batches)
    {
        append(batches.size());
        for (const auto& batch : batches)
        {
            append(batch.visualLifetimeID);
            append(batch.renderableLifetimeID);
            append(batch.geometrySubset);
            append(batch.worldMatrix);
            append(batch.bindlessMaterialID);
            append(batch.indexCount);
            append(batch.startIndex);
            append(batch.vertexCount);
            append(batch.skinnedPoolFormat);
            append(batch.skinnedPoolBaseVertex);
            append(batch.skinnedPoolFirstIndex);
            append(batch.skinningRenderMode);
            CKinematics* skeleton = nullptr;
            if (batch.visual->getType() == MT_SKELETON_GEOMDEF_ST)
                skeleton = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
            else if (batch.visual->getType() == MT_SKELETON_GEOMDEF_PM)
                skeleton = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
            R_ASSERT(skeleton);
            u32 boneCount = 0;
            const Fmatrix* bones = gpu->GetPreparedSkeletonMatrices(skeleton, boneCount);
            append(boneCount);
            HashSceneData(signature, bones, size_t(boneCount) * sizeof(Fmatrix));
        }
    };
    appendBatches(world);
    appendBatches(hud);
    const auto frame = detail ? detail->GetCompletedVisibilityFrame() : nullptr;
    if (frame && frame->source)
    {
        append(frame->source->id);
        append(frame->statsReady ? frame->contentSignature : frame->id);
        append(frame->stats.visibleBillboardCount);
        append(frame->stats.visibleLOD0Count);
        append(frame->stats.visibleLOD1Count);
        append(frame->stats.visibleLOD2Count);
        append(detail->buildDetailsBindlessIndex);
        if (frame->source->params.grassMode &&
            (frame->stats.visibleLOD0Count || frame->stats.visibleLOD1Count || frame->stats.visibleLOD2Count))
        {
            append(ps_r3_grass_blade_height);
            append(ps_r3_grass_blade_width);
            append(ps_r3_grass_wind_displacement);
            append(Device.fTimeGlobal);
            if (ps_r3_grass_wind_displacement != 0.0f)
            {
                append(detail->windSpeed);
                if (g_pGamePersistent)
                    append(g_pGamePersistent->Environment().CurrentEnv.wind_direction);
            }
        }
    }
    return signature;
}

u64 RTAccelStructManager::GetSceneRevision() const
{
    return m_sceneRevision;
}

u64 RTAccelStructManager::GetTextureRevision(nvrhi::ITexture* sky0, nvrhi::ITexture* sky1) const
{
    if (!m_scene || !m_device || !m_device->GetFGResourceManager())
        return 0;
    return m_device->GetFGResourceManager()->GetTextureManager()->GetContentRevision(m_scene->textures.GetTextures(), sky0, sky1);
}

void RTAccelStructManager::PrepareStatic(GPUCullingManager* gpu)
{
    const bool emptySource = !gpu->GetRTVertexBuffer() && !gpu->GetRTIndexBuffer();
    R_ASSERT(emptySource || (gpu->GetRTVertexBuffer() && gpu->GetRTIndexBuffer()));
    const u64 signature = ComputeStaticSignature(gpu);
    if (m_staticGeometry && m_staticGeometry->emptySource == emptySource && m_staticSignature == signature)
        return;
    auto geometry = std::make_shared<RTStaticGeometry>();
    geometry->vertices = gpu->GetRTVertexBuffer();
    geometry->indices = gpu->GetRTIndexBuffer();
    geometry->emptySource = emptySource;
    if (emptySource)
    {
        nvrhi::BufferDesc desc;
        desc.debugName = "RT_EmptySourceVertices";
        desc.byteSize = GPUCullingManager::RT_VERTEX_STRIDE;
        desc.structStride = GPUCullingManager::RT_VERTEX_STRIDE;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        EnsureRTBuffer(m_device->GetNVRHIDevice(), geometry->vertices, desc);
        desc.debugName = "RT_EmptySourceIndices";
        desc.byteSize = sizeof(u32);
        desc.structStride = 0;
        desc.canHaveRawViews = true;
        desc.isIndexBuffer = true;
        EnsureRTBuffer(m_device->GetNVRHIDevice(), geometry->indices, desc);
    }
    const auto& staticArgs = gpu->GetStaticDrawArgsData();
    const auto& staticInstances = gpu->GetStaticInstanceData();
    const auto& staticCounts = gpu->GetStaticBatchVertexCounts();
    const auto& staticMaterials = gpu->GetStaticMaterialIDData();
    R_ASSERT(staticArgs.size() == staticInstances.size());
    R_ASSERT(staticArgs.size() == staticCounts.size());
    R_ASSERT(staticArgs.size() == staticMaterials.size());
    RTGeometryBuild identity;
    identity.desc.debugName = "RT_IdentityBLAS";
    identity.desc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;
    auto append = [&](const xr_vector<IndirectDrawArgs>& args, const xr_vector<u32>& materials,
        bool staticSource, u32& count)
    {
        R_ASSERT(args.size() == materials.size());
        for (u32 i = 0; i < args.size(); ++i)
        {
            if (!args[i].indexCountPerInstance || (staticSource &&
                memcmp(&staticInstances[i].world, &Fidentity, sizeof(Fmatrix)) != 0))
                continue;
            R_ASSERT(args[i].baseVertexLocation >= 0);
            const u32 base = u32(args[i].baseVertexLocation);
            if (base >= gpu->GetRTVertexCount() || u64(args[i].startIndexLocation)
                + args[i].indexCountPerInstance > gpu->GetRTIndexCount())
                continue;
            const u32 vertices = staticSource ? staticCounts[i] : gpu->GetRTVertexCount() - base;
            identity.desc.addBottomLevelGeometry(RTTriangles(geometry->vertices, geometry->indices,
                GPUCullingManager::RT_VERTEX_STRIDE, base, vertices, args[i].startIndexLocation,
                args[i].indexCountPerInstance, true));
            geometry->batches.push_back({ materials[i], args[i].startIndexLocation,
                args[i].baseVertexLocation, args[i].indexCountPerInstance });
            ++count;
        }
    };
    append(staticArgs, staticMaterials, true, geometry->counts.identityStatic);
    append(gpu->GetTerrainDrawArgsData(), gpu->GetTerrainMaterialIDData(), false, geometry->counts.terrain);
    append(gpu->GetTransparentDrawArgsData(), gpu->GetTransparentMaterialIDData(), false, geometry->counts.transparent);
    auto* device = m_device->GetNVRHIDevice();
    if (!geometry->batches.empty())
    {
        identity.handle = device->createAccelStruct(identity.desc);
        R_ASSERT2(identity.handle, "[RT] identity BLAS allocation failed");
        geometry->instances.push_back(RTInstance(identity.handle, 0, Fidentity, true));
        geometry->builds.push_back(std::move(identity));
    }
    xr_map<std::tuple<u32, s32, u32>, u32> unique;
    for (u32 i = 0; i < staticArgs.size(); ++i)
    {
        const auto& args = staticArgs[i];
        if (!args.indexCountPerInstance || memcmp(&staticInstances[i].world, &Fidentity, sizeof(Fmatrix)) == 0)
            continue;
        if (args.baseVertexLocation < 0 || u64(args.baseVertexLocation) + staticCounts[i] > gpu->GetRTVertexCount()
            || u64(args.startIndexLocation) + args.indexCountPerInstance > gpu->GetRTIndexCount())
            continue;
        const auto key = std::make_tuple(args.startIndexLocation, args.baseVertexLocation, args.indexCountPerInstance);
        auto found = unique.find(key);
        u32 buildIndex;
        if (found == unique.end())
        {
            R_ASSERT(args.baseVertexLocation >= 0);
            RTGeometryBuild build;
            build.desc.debugName = "RT_InstancedBLAS";
            build.desc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;
            build.desc.addBottomLevelGeometry(RTTriangles(geometry->vertices, geometry->indices,
                GPUCullingManager::RT_VERTEX_STRIDE, u32(args.baseVertexLocation), staticCounts[i],
                args.startIndexLocation, args.indexCountPerInstance, true));
            build.handle = device->createAccelStruct(build.desc);
            R_ASSERT2(build.handle, "[RT] instanced BLAS allocation failed");
            buildIndex = u32(geometry->builds.size());
            geometry->builds.push_back(std::move(build));
            unique.emplace(key, buildIndex);
        }
        else
            buildIndex = found->second;
        geometry->instances.push_back(RTInstance(geometry->builds[buildIndex].handle,
            u32(geometry->batches.size()), staticInstances[i].world, true));
        geometry->batches.push_back({ staticMaterials[i], args.startIndexLocation,
            args.baseVertexLocation, args.indexCountPerInstance });
        ++geometry->counts.instancedTotal;
    }
    m_textureScratch.clear();
    const u32 terrainEnd = geometry->counts.identityStatic + geometry->counts.terrain;
    for (u32 i = 0; i < geometry->batches.size(); ++i)
        AppendMaterialTextures(geometry->batches[i].materialID,
            i >= geometry->counts.identityStatic && i < terrainEnd);
    geometry->textures.Capture(m_textureScratch);
    m_staticGeometry = std::move(geometry);
    m_staticSignature = signature;
}

u32 RTAccelStructManager::GetSkinningFormatID(u32 poolFormat)
{
    switch (poolFormat)
    {
    case VF_SKINNED_NONHQ: return 0;
    case VF_SKINNED_HQ1W: return 1;
    case VF_SKINNED_HQ2W: return 2;
    case VF_SKINNED_HQ3W: return 3;
    case VF_SKINNED_HQ4W: return 4;
    default: FATAL("[RT] unrecognized authored skinning vertex format");
    }
    return 0;
}

bool RTAccelStructManager::InitSkinningPipeline()
{
    if (s_skinAttempted)
        return s_skinPipeline != nullptr;
    s_skinAttempted = true;
    auto* device = m_device->GetNVRHIDevice();
    auto shader = GEnv.Render->GetShaderLoader()->LoadComputeShader("rt_skin_vertices");
    if (!shader.handle || !shader.reflection)
        return false;
    auto& cache = framegraph::GetPassResourceCache();
    RenderDevice::BufferDesc desc;
    desc.debugName = "RTSkinningCB";
    desc.byteSize = sizeof(RTSkinningCB);
    desc.isConstantBuffer = true;
    desc.isVolatile = true;
    desc.maxVersions = RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    s_skinCB = m_device->CreateBuffer(desc);
    s_skinLayout = cache.GetOrCreateBindingLayoutFromReflection("RTSkinning", *shader.reflection, device);
    if (!s_skinLayout || !s_skinCB.IsValid() || !m_device->GetNativeBuffer(s_skinCB))
        return false;
    nvrhi::ComputePipelineDesc pipeline;
    pipeline.CS = shader.handle;
    pipeline.bindingLayouts = { s_skinLayout };
    s_skinPipeline = device->createComputePipeline(pipeline);
    return s_skinPipeline != nullptr;
}

void RTAccelStructManager::PrepareSkin(RTSceneGeneration& scene, GPUCullingManager* gpu,
    const xr_vector<GeometryBatch>& world, const xr_vector<GeometryBatch>& hud)
{
    if (world.empty() && hud.empty())
        return;
    const auto& pools = gpu->GetSkinnedPools();
    u64 vertexCount = 0;
    auto slotOf = [&](nvrhi::IBuffer* source)
    {
        for (u32 slot = 0; slot < scene.skinSources.size(); ++slot)
        {
            if (scene.skinSources[slot] == source)
                return slot;
        }
        scene.skinSources.push_back(source);
        return u32(scene.skinSources.size() - 1);
    };
    auto append = [&](const GeometryBatch& batch)
    {
        R_ASSERT(batch.visual && batch.indexCount && batch.isSkinned);
        CKinematics* skeleton = nullptr;
        if (batch.visual->getType() == MT_SKELETON_GEOMDEF_ST)
            skeleton = static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
        else if (batch.visual->getType() == MT_SKELETON_GEOMDEF_PM)
            skeleton = static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
        R_ASSERT(skeleton);
        auto* mesh = static_cast<IRender_Mesh*>(static_cast<Fvisual*>(batch.visual));
        const u32 format = batch.skinnedPoolFormat;
        const bool pooled = format >= SkinnedGeometryPools::FIRST_FORMAT
            && format < SkinnedGeometryPools::FORMAT_COUNT;
        const u32 vertices = batch.vertexCount;
        R_ASSERT(vertices && vertexCount + vertices <= UINT32_MAX
            && scene.skinIndexData.size() + u64(batch.indexCount) <= UINT32_MAX);
        nvrhi::IBuffer* source = nullptr;
        const u16* indices = nullptr;
        IndexStagingBuffer* mapped = nullptr;
        u32 stride = 0;
        u32 baseVertex = 0;
        u32 formatID = 0;
        if (pooled)
        {
            source = pools.GetVertexBuffer(format);
            if (!source)
                FATAL_F("[RT] skinned vertex pool %u has no storage; pooled skinning never ran", format);
            stride = SkinnedFormatStride(format);
            baseVertex = u32(batch.skinnedPoolBaseVertex);
            formatID = GetSkinningFormatID(format);
            R_ASSERT(batch.skinnedPoolBaseVertex >= 0
                && u64(baseVertex) + vertices <= pools.GetVertexCount(format));
            indices = pools.GetIndexRange(format, batch.skinnedPoolFirstIndex, batch.indexCount);
            if (!indices)
                FATAL_F("[RT] pooled skinned index range %u+%u escapes format %u",
                    batch.skinnedPoolFirstIndex, batch.indexCount, format);
        }
        else
        {
            R_ASSERT(mesh->p_rm_Vertices && mesh->p_rm_Indices);
            source = mesh->p_rm_Vertices->GetBufferHandle().Get();
            if (!source)
                FATAL("[RT] a skinned visual has neither pooled geometry nor a resident vertex buffer");
            stride = mesh->vStride;
            baseVertex = mesh->vBase;
            formatID = GetSkinningFormatID(SkinnedFormatFromRenderMode(batch.skinningRenderMode, stride));
            if ((u64(batch.startIndex) + batch.indexCount) * sizeof(u16)
                > mesh->p_rm_Indices->GetSystemMemoryUsage())
                FATAL_F("[RT] skinned index range %u+%u escapes its retained staging allocation",
                    batch.startIndex, batch.indexCount);
            mapped = mesh->p_rm_Indices;
            const auto* host = static_cast<const u16*>(mapped->Map(0, 0, true));
            R_ASSERT(host);
            indices = host + batch.startIndex;
        }
        R_ASSERT(stride && (u64(baseVertex) + vertices) * stride <= source->getDesc().byteSize);
        RTSkinJob job;
        job.constants = {};
        job.constants.worldMatrix = batch.worldMatrix;
        Fmatrix inverse;
        inverse.invert(batch.worldMatrix);
        job.constants.normalMatrix.transpose(inverse);
        job.constants.vertexCount = vertices;
        job.constants.vertexStride = stride;
        job.constants.formatID = formatID;
        job.constants.boneOffset = gpu->GetPreparedSkeletonOffset(skeleton);
        job.constants.outputOffset = u32(vertexCount);
        job.constants.inputBaseVertex = baseVertex;
        job.sourceSlot = slotOf(source);
        job.indexOffset = u32(scene.skinIndexData.size());
        job.indexCount = batch.indexCount;
        job.materialID = batch.bindlessMaterialID;
        for (u32 i = 0; i < batch.indexCount; ++i)
        {
            const u32 index = indices[i];
            R_ASSERT(index < vertices);
            scene.skinIndexData.push_back(index);
        }
        if (mapped)
            mapped->Unmap();
        vertexCount += vertices;
        scene.skinJobs.push_back(std::move(job));
    };
    for (const auto& batch : world)
        append(batch);
    for (const auto& batch : hud)
        append(batch);
    auto* device = m_device->GetNVRHIDevice();
    EnsureRTBuffer(device, scene.skinnedVertices, RTVertexBufferDesc("RT_SkinVertices", vertexCount * SKIN_VERTEX_STRIDE));
    EnsureRTBuffer(device, scene.skinnedIndices, RTVertexBufferDesc("RT_SkinIndices", scene.skinIndexData.size() * sizeof(u32)));
    scene.bones = gpu->GetGlobalBoneBuffer();
    R_ASSERT(scene.bones);
    scene.skinBuild.desc.debugName = "RT_SkinBLAS";
    scene.skinBuild.desc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastBuild;
    const u32 firstBatch = u32(scene.batches.size());
    for (const auto& job : scene.skinJobs)
    {
        scene.skinBuild.desc.addBottomLevelGeometry(RTTriangles(scene.skinnedVertices,
            scene.skinnedIndices, SKIN_VERTEX_STRIDE, job.constants.outputOffset, job.constants.vertexCount,
            job.indexOffset, job.indexCount, true));
        scene.batches.push_back({ job.materialID, job.indexOffset, s32(job.constants.outputOffset), job.indexCount });
    }
    scene.skinBuild.handle = device->createAccelStruct(scene.skinBuild.desc);
    R_ASSERT2(scene.skinBuild.handle, "[RT] skin BLAS allocation failed");
    scene.instances.push_back(RTInstance(scene.skinBuild.handle, firstBatch, Fidentity, true));
    scene.counts.skinned = u32(scene.skinJobs.size());
}

bool RTAccelStructManager::InitGrassPipeline(const FGDetailManager::InstanceGeneration& source)
{
    if (s_grassAttempted && s_grassSourceLayout == source.bindingLayout)
        return s_grassPipeline != nullptr;
    s_grassAttempted = true;
    s_grassSourceLayout = source.bindingLayout;
    s_grassPipeline = nullptr;
    auto* device = m_device->GetNVRHIDevice();
    auto shader = GEnv.Render->GetShaderLoader()->LoadComputeShader("rt_grass_vertices");
    if (!shader.handle || !shader.reflection)
        return false;
    auto& cache = framegraph::GetPassResourceCache();
    if (!s_grassCB.IsValid())
    {
        RenderDevice::BufferDesc desc;
        desc.debugName = "GrassRTCB";
        desc.byteSize = sizeof(GrassRTCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
        s_grassCB = m_device->CreateBuffer(desc);
    }
    s_grassLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGrass", *shader.reflection, device);
    if (!s_grassLayout || !s_grassCB.IsValid() || !m_device->GetNativeBuffer(s_grassCB))
        return false;
    nvrhi::ComputePipelineDesc pipeline;
    pipeline.CS = shader.handle;
    pipeline.bindingLayouts = { s_grassLayout, m_device->GetBackend()->GetBindlessLayout(), source.bindingLayout };
    s_grassPipeline = device->createComputePipeline(pipeline);
    return s_grassPipeline != nullptr;
}

bool RTAccelStructManager::InitBillboardPipeline(const FGDetailManager::InstanceGeneration& source)
{
    if (s_billboardAttempted && s_billboardSourceLayout == source.bindingLayout)
        return s_billboardPipeline != nullptr;
    s_billboardAttempted = true;
    s_billboardSourceLayout = source.bindingLayout;
    s_billboardPipeline = nullptr;
    auto* device = m_device->GetNVRHIDevice();
    auto shader = GEnv.Render->GetShaderLoader()->LoadComputeShader("rt_grass_billboard");
    if (!shader.handle || !shader.reflection)
        return false;
    auto& cache = framegraph::GetPassResourceCache();
    if (!s_billboardCB.IsValid())
    {
        RenderDevice::BufferDesc desc;
        desc.debugName = "BillboardRTCB";
        desc.byteSize = sizeof(BillboardRTCB);
        desc.isConstantBuffer = true;
        desc.isVolatile = true;
        desc.maxVersions = RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
        s_billboardCB = m_device->CreateBuffer(desc);
    }
    s_billboardLayout = cache.GetOrCreateBindingLayoutFromReflection("RTBillboard", *shader.reflection, device);
    if (!s_billboardLayout || !s_billboardCB.IsValid() || !m_device->GetNativeBuffer(s_billboardCB))
        return false;
    nvrhi::ComputePipelineDesc pipeline;
    pipeline.CS = shader.handle;
    pipeline.bindingLayouts = { s_billboardLayout, m_device->GetBackend()->GetBindlessLayout(), source.bindingLayout };
    s_billboardPipeline = device->createComputePipeline(pipeline);
    return s_billboardPipeline != nullptr;
}

bool RTAccelStructManager::EnsureBuildResources(FGDetailManager* detail, bool needsSkin)
{
    if (!GEnv.Render || !GEnv.Render->GetShaderLoader())
        return false;
    auto* backend = m_device->GetBackend();
    if (!backend || !backend->GetBindlessLayout() || !backend->GetBindlessDescriptorTable())
        return false;
    if (needsSkin && !InitSkinningPipeline())
        return false;
    const auto frame = detail ? detail->GetCompletedVisibilityFrame() : nullptr;
    if (!frame || !frame->source || frame->source->chunks.empty())
        return true;
    const auto& source = *frame->source;
    const auto& stats = frame->stats;
    if (!source.params.grassMode)
    {
        if (!source.maxPulledIndexCount || !stats.visibleBillboardCount)
            return true;
        return source.bindingLayout && source.descriptorTable && frame->visible[FGDetailManager::VIS_KIND_MESH] && InitBillboardPipeline(source);
    }
    if (!stats.visibleLOD0Count && !stats.visibleLOD1Count && !stats.visibleLOD2Count)
        return true;
    if (!source.bindingLayout || !source.descriptorTable || !detail->perlin4dTexture)
        return false;
    const u32 counts[] = { stats.visibleLOD0Count, stats.visibleLOD1Count, stats.visibleLOD2Count };
    for (u32 i = 0; i < 3; ++i)
    {
        if (counts[i] && !frame->visible[i])
            return false;
    }
    return InitGrassPipeline(source);
}

void RTAccelStructManager::PrepareGrass(RTSceneGeneration& scene, FGDetailManager* detail)
{
    scene.grassFrame.reset();
    scene.grassPipeline = nullptr;
    scene.grassLayout = nullptr;
    if (!detail)
        return;
    auto frame = detail->GetCompletedVisibilityFrame();
    if (!frame || !frame->source || frame->source->chunks.empty())
        return;
    const auto& source = *frame->source;
    const auto& stats = frame->stats;
    scene.billboard = !source.params.grassMode;
    u64 vertices = 0;
    u64 indices = 0;
    if (scene.billboard)
    {
        const u32 maximum = source.maxPulledIndexCount / 3 * 3;
        if (!maximum || !stats.visibleBillboardCount)
            return;
        scene.grassPipeline = s_billboardPipeline;
        scene.grassLayout = s_billboardLayout;
        scene.billboardConstants.maxVertsPerBillboard = maximum;
        scene.billboardConstants.billboardCount = stats.visibleBillboardCount;
        vertices = u64(maximum) * stats.visibleBillboardCount;
        indices = vertices;
        scene.grassJobs.push_back({ frame->visible[FGDetailManager::VIS_KIND_MESH], {} });
        scene.detailAtlasIndex = detail->buildDetailsBindlessIndex;
    }
    else
    {
        const u32 counts[] = { stats.visibleLOD0Count, stats.visibleLOD1Count, stats.visibleLOD2Count };
        if (!counts[0] && !counts[1] && !counts[2])
            return;
        scene.grassPipeline = s_grassPipeline;
        scene.grassLayout = s_grassLayout;
        scene.grassWind = detail->perlin4dTexture;
        GrassRTCB constants = {};
        const float angle = g_pGamePersistent ? g_pGamePersistent->Environment().CurrentEnv.wind_direction : 0.0f;
        constants.wind_direction.set(angle, detail->windSpeed, 0.0f, 0.0f);
        constants.wave.set(1.0f / 5.0f, 1.0f / 7.0f, 1.0f / 3.0f, Device.fTimeGlobal);
        constants.grass_wind_displacement = ps_r3_grass_wind_displacement;
        constants.grass_blade_height = ps_r3_grass_blade_height;
        constants.grass_blade_width = ps_r3_grass_blade_width;
        for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; ++lod)
        {
            if (!counts[lod])
                continue;
            RTGrassJob job;
            job.visible = frame->visible[lod];
            job.constants = constants;
            job.constants.segments = FGDetailManager::LOD_SEGMENTS[lod];
            job.constants.vertsPerBlade = job.constants.segments * 2 + 1;
            job.constants.indicesPerBlade = (job.constants.segments - 1) * 6 + 3;
            job.constants.bladeCount = counts[lod];
            R_ASSERT(vertices <= UINT32_MAX && indices <= UINT32_MAX);
            job.constants.outputVertexOffset = u32(vertices);
            job.constants.outputIndexOffset = u32(indices);
            vertices += u64(counts[lod]) * job.constants.vertsPerBlade;
            indices += u64(counts[lod]) * job.constants.indicesPerBlade;
            scene.grassJobs.push_back(std::move(job));
        }
    }
    const auto memory = m_device->GetBackend()->GetMemoryBudget();
    const u64 rawRange = u64(UINT32_MAX) + 1;
    const u64 range = memory.bufferRangeBytes ? std::min(rawRange, memory.bufferRangeBytes) : rawRange;
    R_ASSERT2(vertices && indices && vertices <= range / 24 && indices <= range / sizeof(u32),
        "[RT] visible grass expansion exceeds the native raw-buffer address range");
    scene.grassFrame = std::move(frame);
    scene.grassVertexCount = u32(vertices);
    scene.grassIndexCount = u32(indices);
    auto* device = m_device->GetNVRHIDevice();
    EnsureRTBuffer(device, scene.grassVertices, RTVertexBufferDesc("RT_GrassVertices", vertices * 24));
    EnsureRTBuffer(device, scene.grassIndices, RTVertexBufferDesc("RT_GrassIndices", indices * sizeof(u32)));
    scene.grassBuild.desc.debugName = "RT_GrassBLAS";
    scene.grassBuild.desc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastBuild;
    scene.grassBuild.desc.addBottomLevelGeometry(RTTriangles(scene.grassVertices, scene.grassIndices,
        24, 0, scene.grassVertexCount, 0, scene.grassIndexCount, !scene.billboard));
    scene.grassBuild.handle = device->createAccelStruct(scene.grassBuild.desc);
    R_ASSERT2(scene.grassBuild.handle, "[RT] grass BLAS allocation failed");
    scene.instances.push_back(RTInstance(scene.grassBuild.handle, u32(scene.batches.size()),
        Fidentity, !scene.billboard));
    scene.batches.push_back({ 0, 0, 0, scene.grassIndexCount });
    scene.counts.grass = 1;
}

void RTAccelStructManager::PrepareScene(GPUCullingManager* gpu, FGDetailManager* detail,
    const xr_vector<GeometryBatch>& world, const xr_vector<GeometryBatch>& hud)
{
    std::shared_ptr<RTSceneGeneration> next;
    for (const auto& candidate : m_generations)
    {
        if (candidate->leases == 0)
        {
            next = candidate;
            break;
        }
    }
    if (!next)
    {
        next = std::make_shared<RTSceneGeneration>();
        m_generations.push_back(next);
    }
    auto& scene = *next;
    scene.geometry = m_staticGeometry;
    scene.skinBuild = {};
    scene.grassBuild = {};
    scene.skinJobs.clear();
    scene.skinSources.clear();
    scene.grassJobs.clear();
    scene.skinIndexData.clear();
    scene.batches = m_staticGeometry->batches;
    scene.instances = m_staticGeometry->instances;
    scene.counts = m_staticGeometry->counts;
    scene.detailAtlasIndex = 0;
    scene.grassVertexCount = 0;
    scene.grassIndexCount = 0;
    scene.recorded = false;
    PrepareSkin(scene, gpu, world, hud);
    PrepareGrass(scene, detail);
    if (scene.instances.empty())
    {
        m_scene.reset();
        return;
    }
    auto* device = m_device->GetNVRHIDevice();
    nvrhi::rt::AccelStructDesc tlas;
    tlas.debugName = "RT_SceneTLAS";
    tlas.isTopLevel = true;
    tlas.topLevelMaxInstances = u32(scene.instances.size());
    tlas.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;
    scene.tlas = device->createAccelStruct(tlas);
    R_ASSERT2(scene.tlas, "[RT] TLAS allocation failed");
    nvrhi::BufferDesc batches;
    batches.debugName = "RT_BatchInfo";
    batches.byteSize = scene.batches.size() * sizeof(RTBatchInfo);
    batches.structStride = sizeof(RTBatchInfo);
    batches.initialState = nvrhi::ResourceStates::ShaderResource;
    batches.keepInitialState = true;
    EnsureRTBuffer(device, scene.batchInfo, batches);
    scene.sourceMaterials = bindless::MaterialBuffer::Instance().GetBuffer();
    scene.sourceTerrainMaterials = bindless::TerrainMaterialBuffer::Instance().GetBuffer();
    scene.sourceVariants = bindless::VariantBuffer::Instance().GetBuffer();
    R_ASSERT(scene.sourceMaterials && scene.sourceTerrainMaterials && scene.sourceVariants);
    auto materialDesc = scene.sourceMaterials->getDesc();
    materialDesc.debugName = "RT_MaterialSnapshot";
    EnsureRTBuffer(device, scene.materials, materialDesc);
    materialDesc = scene.sourceTerrainMaterials->getDesc();
    materialDesc.debugName = "RT_TerrainMaterialSnapshot";
    EnsureRTBuffer(device, scene.terrainMaterials, materialDesc);
    auto variantDesc = scene.sourceVariants->getDesc();
    variantDesc.debugName = "RT_VariantSnapshot";
    EnsureRTBuffer(device, scene.variants, variantDesc);
    m_textureScratch.clear();
    const u32 skinEnd = u32(scene.geometry->batches.size()) + scene.counts.skinned;
    const u32 terrainEnd = scene.counts.identityStatic + scene.counts.terrain;
    for (u32 i = 0; i < skinEnd; ++i)
        AppendMaterialTextures(scene.batches[i].materialID, i >= scene.counts.identityStatic && i < terrainEnd);
    if (scene.counts.grass && scene.billboard && scene.detailAtlasIndex != bindless::INVALID_TEXTURE_INDEX)
        m_textureScratch.push_back(scene.detailAtlasIndex);
    scene.textures.Capture(m_textureScratch);
    m_scene = std::move(next);
}

RTFrameResources RTAccelStructManager::ImportScene(framegraph::FrameGraph& graph,
    const RTSceneGeneration& scene) const
{
    RTFrameResources resources;
    resources.tlas = graph.ImportAccelerationStructure("RT_SceneTLAS", scene.tlas);
    resources.batchInfo = ImportRTBuffer(graph, "RT_BatchInfo", scene.batchInfo);
    resources.vertices = ImportRTBuffer(graph, "rt_source_vertices", scene.geometry->vertices);
    resources.indices = ImportRTBuffer(graph, "rt_source_indices", scene.geometry->indices);
    resources.materials = ImportRTBuffer(graph, "RT_MaterialSnapshot", scene.materials);
    resources.terrainMaterials = ImportRTBuffer(graph, "RT_TerrainMaterialSnapshot", scene.terrainMaterials);
    resources.variants = ImportRTBuffer(graph, "RT_VariantSnapshot", scene.variants);
    resources.textures = scene.textures.GetTable();
    R_ASSERT(resources.textures == scene.geometry->textures.GetTable());
    if (scene.counts.skinned)
    {
        resources.skinnedVertices = ImportRTBuffer(graph, "RT_SkinVertices", scene.skinnedVertices);
        resources.skinnedIndices = ImportRTBuffer(graph, "RT_SkinIndices", scene.skinnedIndices);
    }
    if (scene.counts.grass)
    {
        resources.grassVertices = ImportRTBuffer(graph, "RT_GrassVertices", scene.grassVertices);
        resources.grassIndices = ImportRTBuffer(graph, "RT_GrassIndices", scene.grassIndices);
    }
    return resources;
}

bool RTAccelStructManager::SetupBuildPass(framegraph::FrameGraph& graph, GPUCullingManager* gpu, FGDetailManager* detail, const xr_vector<GeometryBatch>& world,
    const xr_vector<GeometryBatch>& hud)
{
    if (!m_rtSupported || !gpu || !GEnv.Backend || !GEnv.Backend->SupportsSubmissionLeases())
        return false;
    RetireScenes();
    PrepareStatic(gpu);
    if (!m_staticGeometry)
        return false;
    const u64 signature = ComputeSceneSignature(gpu, detail, world, hud);
    if (!m_scene || signature != m_sceneSignature || m_scene->geometry != m_staticGeometry || !m_scene->recorded)
    {
        if (!EnsureBuildResources(detail, !world.empty() || !hud.empty()))
            return false;
        PrepareScene(gpu, detail, world, hud);
        if (IsReady())
        {
            m_sceneSignature = signature;
            ++m_sceneRevision;
        }
    }
    if (!IsReady())
        return false;
    const u64 lease = GEnv.Backend->OpenSubmissionLease();
    if (!lease)
        return false;
    ++m_scene->leases;
    m_leases.push_back({ m_scene, lease });
    if (m_scene->recorded)
        return true;
    using namespace framegraph;
    const auto scene = m_scene;
    const auto resources = ImportScene(graph, *scene);
    graph.addCallbackPass<RTBuildPassData>("RT Source and Pose",
        [&](FrameGraph& builder, PassHandle pass, RTBuildPassData& data)
        {
            RenderPassBuilder pb(builder, pass);
            data.manager = this;
            data.scene = scene;
            data.resources = resources;
            pb.write(resources.batchInfo, ResourceState::CopyDest);
            pb.write(resources.materials, ResourceState::CopyDest);
            pb.write(resources.terrainMaterials, ResourceState::CopyDest);
            pb.write(resources.variants, ResourceState::CopyDest);
            data.sourceMaterials = pb.read(ImportRTBuffer(builder, "bindless_materials",
                scene->sourceMaterials), ResourceState::CopySource);
            data.sourceTerrainMaterials = pb.read(ImportRTBuffer(builder, "terrain_materials",
                scene->sourceTerrainMaterials), ResourceState::CopySource);
            data.sourceVariants = pb.read(ImportRTBuffer(builder, "bindless_variants",
                scene->sourceVariants), ResourceState::CopySource);
            if (scene->counts.skinned)
            {
                pb.write(resources.skinnedVertices, ResourceState::UnorderedAccess);
                pb.write(resources.skinnedIndices, ResourceState::CopyDest);
                data.bones = pb.read(ImportRTBuffer(builder, "skinned_bone_matrices", scene->bones),
                    ResourceState::ShaderResource);
                data.skinSources.clear();
                for (const auto& source : scene->skinSources)
                {
                    const auto& name = source->getDesc().debugName;
                    data.skinSources.push_back(pb.read(ImportRTBuffer(builder,
                        name.empty() ? "RT_SkinSource" : name.c_str(), source),
                        ResourceState::ShaderResource));
                }
            }
            if (scene->counts.grass)
            {
                pb.write(resources.grassVertices, ResourceState::UnorderedAccess);
                pb.write(resources.grassIndices, ResourceState::UnorderedAccess);
                const auto& frame = *scene->grassFrame;
                for (u32 chunk : frame.visibleChunks)
                {
                    const auto& buffer = frame.source->chunks[chunk].buffer;
                    data.grassSources.push_back(pb.read(ImportRTBuffer(builder, buffer->getDesc().debugName.c_str(), buffer),
                        ResourceState::ShaderResource));
                }
                for (const auto& job : scene->grassJobs)
                    data.grassVisible.push_back(pb.read(ImportRTBuffer(builder, "RT_GrassVisible", job.visible),
                        ResourceState::ShaderResource));
                if (scene->billboard)
                {
                    data.grassModels = pb.read(ImportRTBuffer(builder, "RT_GrassModels", frame.source->models), ResourceState::ShaderResource);
                    data.grassPulledVertices = pb.read(ImportRTBuffer(builder, "RT_GrassPulledVertices", frame.source->pulledVertices), ResourceState::ShaderResource);
                }
                else
                {
                    R_ASSERT(scene->grassWind);
                    const auto& source = scene->grassWind->getDesc();
                    ResourceDesc desc;
                    desc.type = ResourceDesc::Type::Texture3D;
                    desc.width = source.width;
                    desc.height = source.height;
                    desc.depth = source.depth;
                    desc.mipLevels = source.mipLevels;
                    desc.format = source.format;
                    desc.isUAV = source.isUAV;
                    desc.isTransient = false;
                    data.wind = pb.read(builder.ImportTexture("RT_GrassWind", scene->grassWind, desc), ResourceState::ShaderResource);
                }
            }
        },
        [](const RTBuildPassData& data, const FrameGraph& fg, RenderContext* ctx)
        {
            data.manager->RecordInputs(data, fg, ctx->GetCommandList());
        });
    graph.addCallbackPass<RTBuildPassData>("RT BLAS Build",
        [&](FrameGraph& builder, PassHandle pass, RTBuildPassData& data)
        {
            RenderPassBuilder pb(builder, pass);
            data.manager = this;
            data.scene = scene;
            data.resources = resources;
            auto input = [&](VirtualResourceHandle handle)
            {
                if (handle.is_valid())
                    data.buffers.push_back(pb.read(handle, ResourceState::AccelStructBuildInput));
            };
            auto output = [&](const RTGeometryBuild& build)
            {
                if (build.handle)
                    data.structures.push_back(pb.write(builder.ImportAccelerationStructure(
                        build.desc.debugName.c_str(), build.handle), ResourceState::AccelStructWrite));
            };
            if (!scene->geometry->recorded)
            {
                input(resources.vertices);
                input(resources.indices);
                for (const auto& build : scene->geometry->builds)
                    output(build);
            }
            if (scene->counts.skinned)
            {
                input(resources.skinnedVertices);
                input(resources.skinnedIndices);
                output(scene->skinBuild);
            }
            if (scene->counts.grass)
            {
                input(resources.grassVertices);
                input(resources.grassIndices);
                output(scene->grassBuild);
            }
        },
        [](const RTBuildPassData& data, const FrameGraph& fg, RenderContext* ctx)
        {
            data.manager->RecordBLAS(data, fg, ctx->GetCommandList());
        });
    graph.addCallbackPass<RTBuildPassData>("RT TLAS Build",
        [&](FrameGraph& builder, PassHandle pass, RTBuildPassData& data)
        {
            RenderPassBuilder pb(builder, pass);
            data.manager = this;
            data.scene = scene;
            data.resources = resources;
            pb.write(resources.tlas, ResourceState::AccelStructWrite);
            auto input = [&](const RTGeometryBuild& build)
            {
                if (build.handle)
                    data.structures.push_back(pb.read(builder.ImportAccelerationStructure(
                        build.desc.debugName.c_str(), build.handle), ResourceState::AccelStructBuildBlas));
            };
            for (const auto& build : scene->geometry->builds)
                input(build);
            input(scene->skinBuild);
            input(scene->grassBuild);
        },
        [](const RTBuildPassData& data, const FrameGraph& fg, RenderContext* ctx)
        {
            data.manager->RecordTLAS(data, fg, ctx->GetCommandList());
        });
    return true;
}

void RTAccelStructManager::RecordInputs(const RTBuildPassData& data,
    const framegraph::FrameGraph& graph, nvrhi::ICommandList* commandList)
{
    const auto& scene = *data.scene;
    auto buffer = [&](framegraph::VirtualResourceHandle handle)
    {
        return graph.GetPhysicalBuffer(handle);
    };
    commandList->writeBuffer(buffer(data.resources.batchInfo), scene.batches.data(),
        scene.batches.size() * sizeof(RTBatchInfo));
    auto* source = buffer(data.sourceMaterials);
    commandList->copyBuffer(buffer(data.resources.materials), 0, source, 0, source->getDesc().byteSize);
    source = buffer(data.sourceTerrainMaterials);
    commandList->copyBuffer(buffer(data.resources.terrainMaterials), 0, source, 0, source->getDesc().byteSize);
    source = buffer(data.sourceVariants);
    commandList->copyBuffer(buffer(data.resources.variants), 0, source, 0, source->getDesc().byteSize);
    auto* device = m_device->GetNVRHIDevice();
    if (scene.counts.skinned)
    {
        auto* output = buffer(data.resources.skinnedVertices);
        commandList->writeBuffer(buffer(data.resources.skinnedIndices), scene.skinIndexData.data(),
            scene.skinIndexData.size() * sizeof(u32));
        const auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("rt_skin_vertices", ".cs");
        R_ASSERT(reflection);
        auto* bones = buffer(data.bones);
        for (const auto& job : scene.skinJobs)
        {
            framegraph::BindingSetBuilder bindings(*reflection, device, "RT.SkinVertices");
            bindings.BufferSRV("g_SrcVB", buffer(data.skinSources[job.sourceSlot]))
                .BufferSRV("g_BoneMatrices", bones)
                .BufferUAV("g_Output", output).ConstantBuffer("RTSkinningCB", m_device->GetNativeBuffer(s_skinCB));
            auto bindingSet = device->createBindingSet(bindings.Build(), s_skinLayout);
            R_ASSERT(bindingSet);
            commandList->writeBuffer(m_device->GetNativeBuffer(s_skinCB), &job.constants, sizeof(job.constants));
            nvrhi::ComputeState state;
            state.pipeline = s_skinPipeline;
            state.bindings = { bindingSet };
            commandList->setComputeState(state);
            commandList->dispatch((job.constants.vertexCount + 255) / 256, 1, 1);
        }
    }
    if (!scene.counts.grass)
        return;
    auto* vertices = buffer(data.resources.grassVertices);
    auto* indices = buffer(data.resources.grassIndices);
    const auto& grassSource = *scene.grassFrame->source;
    for (auto handle : data.grassSources)
        commandList->setBufferState(buffer(handle), nvrhi::ResourceStates::NonPixelShaderResource);
    if (scene.billboard)
    {
        const auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("rt_grass_billboard", ".cs");
        R_ASSERT(reflection);
        framegraph::BindingSetBuilder bindings(*reflection, device, "RT.Billboard");
        bindings.BufferSRV("g_VisibleIndices", buffer(data.grassVisible.front()))
            .BufferSRV("g_DetailModels", buffer(data.grassModels))
            .BufferSRV("g_PulledVerts", buffer(data.grassPulledVertices))
            .BufferUAV("g_Output", vertices).BufferUAV("g_OutputIB", indices)
            .ConstantBuffer("BillboardRTCB", m_device->GetNativeBuffer(s_billboardCB));
        auto set = device->createBindingSet(bindings.Build(), scene.grassLayout);
        R_ASSERT(set);
        commandList->writeBuffer(m_device->GetNativeBuffer(s_billboardCB), &scene.billboardConstants,
            sizeof(scene.billboardConstants));
        nvrhi::ComputeState state;
        state.pipeline = scene.grassPipeline;
        state.bindings = { set, m_device->GetBackend()->GetBindlessDescriptorTable(), grassSource.descriptorTable };
        commandList->setComputeState(state);
        const u32 groups = (scene.billboardConstants.billboardCount + 255u) / 256u;
        commandList->dispatch(std::min(groups, 1024u), (groups + 1023u) / 1024u, 1);
    }
    else
    {
        const auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("rt_grass_vertices", ".cs");
        R_ASSERT(reflection);
        for (u32 i = 0; i < scene.grassJobs.size(); ++i)
        {
            const auto& job = scene.grassJobs[i];
            framegraph::BindingSetBuilder bindings(*reflection, device, "RT.Grass");
            bindings.BufferSRV("g_VisibleIndices", buffer(data.grassVisible[i]))
                .Texture("g_WindTexture", graph.GetPhysicalTexture(data.wind))
                .BufferUAV("g_Output", vertices).BufferUAV("g_OutputIB", indices)
                .ConstantBuffer("GrassRTCB", m_device->GetNativeBuffer(s_grassCB));
            auto set = device->createBindingSet(bindings.Build(), scene.grassLayout);
            R_ASSERT(set);
            commandList->writeBuffer(m_device->GetNativeBuffer(s_grassCB), &job.constants, sizeof(job.constants));
            nvrhi::ComputeState state;
            state.pipeline = scene.grassPipeline;
            state.bindings = { set, m_device->GetBackend()->GetBindlessDescriptorTable(), grassSource.descriptorTable };
            commandList->setComputeState(state);
            const u32 vertices = job.constants.bladeCount * job.constants.vertsPerBlade;
            const u32 groups = (vertices + 255u) / 256u;
            commandList->dispatch(std::min(groups, 1024u), (groups + 1023u) / 1024u, 1);
        }
    }
}

void RTAccelStructManager::RecordBLAS(const RTBuildPassData& data,
    const framegraph::FrameGraph& graph, nvrhi::ICommandList* commandList)
{
    for (const auto handle : data.buffers)
        R_ASSERT(graph.GetPhysicalBuffer(handle));
    u32 index = 0;
    auto build = [&](const RTGeometryBuild& item)
    {
        if (!item.handle)
            return;
        auto* destination = graph.GetPhysicalAccelerationStructure(data.structures[index++]);
        nvrhi::utils::BuildBottomLevelAccelStruct(commandList, destination, item.desc);
    };
    if (!data.scene->geometry->recorded)
    {
        for (const auto& item : data.scene->geometry->builds)
            build(item);
        data.scene->geometry->recorded = true;
    }
    if (data.scene->counts.skinned)
        build(data.scene->skinBuild);
    if (data.scene->counts.grass)
        build(data.scene->grassBuild);
    R_ASSERT(index == data.structures.size());
}

void RTAccelStructManager::RecordTLAS(const RTBuildPassData& data,
    const framegraph::FrameGraph& graph, nvrhi::ICommandList* commandList)
{
    for (const auto handle : data.structures)
        R_ASSERT(graph.GetPhysicalAccelerationStructure(handle));
    auto* tlas = graph.GetPhysicalAccelerationStructure(data.resources.tlas);
    commandList->buildTopLevelAccelStruct(tlas, data.scene->instances.data(), u32(data.scene->instances.size()));
    data.scene->recorded = true;
}

RTFrameResources RTAccelStructManager::UseScene(framegraph::FrameGraph& graph,
    framegraph::RenderPassBuilder& builder) const
{
    R_ASSERT(IsReady());
    auto resources = ImportScene(graph, *m_scene);
    builder.read(resources.tlas, framegraph::ResourceState::AccelStructRead);
    auto input = [&](framegraph::VirtualResourceHandle handle)
    {
        if (handle.is_valid())
            builder.read(handle, framegraph::ResourceState::ShaderResource);
    };
    input(resources.batchInfo);
    input(resources.vertices);
    input(resources.indices);
    input(resources.materials);
    input(resources.terrainMaterials);
    input(resources.variants);
    input(resources.skinnedVertices);
    input(resources.skinnedIndices);
    input(resources.grassVertices);
    input(resources.grassIndices);
    return resources;
}

RTFrameBuffers RTAccelStructManager::ResolveScene(const framegraph::FrameGraph& graph,
    const RTFrameResources& resources)
{
    auto buffer = [&](framegraph::VirtualResourceHandle handle)
    {
        return handle.is_valid() ? graph.GetPhysicalBuffer(handle) : nullptr;
    };
    RTFrameBuffers result;
    result.tlas = graph.GetPhysicalAccelerationStructure(resources.tlas);
    result.batchInfo = buffer(resources.batchInfo);
    result.vertices = buffer(resources.vertices);
    result.indices = buffer(resources.indices);
    result.materials = buffer(resources.materials);
    result.terrainMaterials = buffer(resources.terrainMaterials);
    result.variants = buffer(resources.variants);
    result.skinnedVertices = buffer(resources.skinnedVertices);
    result.skinnedIndices = buffer(resources.skinnedIndices);
    result.grassVertices = buffer(resources.grassVertices);
    result.grassIndices = buffer(resources.grassIndices);
    result.textures = resources.textures.Get();
    return result;
}

RTMemoryStats RTAccelStructManager::GetMemoryStats(const GPUCullingManager* gpu) const
{
    RTMemoryStats result;
    result.pendingLeases = u32(m_leases.size());
    result.generations = u32(m_generations.size());
    auto bufferBytes = [](nvrhi::IBuffer* buffer)
    {
        return buffer ? buffer->getDesc().byteSize : u64(0);
    };
    auto* sourceVertices = gpu ? gpu->GetRTVertexBuffer() : nullptr;
    auto* sourceIndices = gpu ? gpu->GetRTIndexBuffer() : nullptr;
    result.sourceBytes = bufferBytes(sourceVertices) + bufferBytes(sourceIndices);
    auto acceleration = [&](nvrhi::rt::IAccelStruct* structure)
    {
        if (!structure)
            return;
        const auto bytes = m_device->GetNVRHIDevice()->getAccelStructMemoryRequirements(structure).size;
        result.accelerationBytes += bytes;
        result.accelerationBytesKnown &= bytes != 0;
    };
    for (u32 i = 0; i < m_generations.size(); ++i)
    {
        const auto& scene = *m_generations[i];
        result.generationBytes += bufferBytes(scene.batchInfo) + bufferBytes(scene.materials) +
            bufferBytes(scene.terrainMaterials) + bufferBytes(scene.variants) +
            bufferBytes(scene.skinnedVertices) +
            bufferBytes(scene.skinnedIndices) + bufferBytes(scene.grassVertices) + bufferBytes(scene.grassIndices);
        acceleration(scene.tlas);
        acceleration(scene.skinBuild.handle);
        acceleration(scene.grassBuild.handle);
        bool first = scene.geometry != nullptr;
        for (u32 j = 0; j < i; ++j)
            first &= m_generations[j]->geometry != scene.geometry;
        if (first)
        {
            if (scene.geometry->vertices != sourceVertices)
                result.sourceBytes += bufferBytes(scene.geometry->vertices);
            if (scene.geometry->indices != sourceIndices)
                result.sourceBytes += bufferBytes(scene.geometry->indices);
            for (const auto& build : scene.geometry->builds)
                acceleration(build.handle);
        }
    }
    return result;
}
}
