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
#include <nvrhi/utils.h>
#include <tuple>


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

static bool EnsureRTBuffer(nvrhi::IDevice* device, nvrhi::BufferHandle& buffer,
    const nvrhi::BufferDesc& desc, bool* reallocated = nullptr)
{
    R_ASSERT(desc.byteSize != 0);
    if (reallocated)
        *reallocated = false;
    const bool strideMatches = buffer && buffer->getDesc().structStride == desc.structStride;
    if (strideMatches && buffer->getDesc().byteSize >= desc.byteSize)
        return true;
    nvrhi::BufferDesc grown = desc;
    if (strideMatches)
    {
        const u64 previous = buffer->getDesc().byteSize;
        grown.byteSize = std::max(desc.byteSize, previous + previous / 2);
    }
    if (desc.structStride)
        grown.byteSize = (grown.byteSize + desc.structStride - 1) / desc.structStride * desc.structStride;
    buffer = device->createBuffer(grown);
    if (!buffer)
        return false;
    if (reallocated)
        *reallocated = true;
    return true;
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

static nvrhi::BufferDesc RTTableBufferDesc(const char* name, u32 stride, size_t count)
{
    nvrhi::BufferDesc desc;
    desc.debugName = name;
    desc.byteSize = u64(count > 0 ? count : 1) * stride;
    desc.structStride = stride;
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;
    return desc;
}

bool RTAccelStructManager::AccelStructShapeMatches(const nvrhi::rt::AccelStructDesc& cached,
    const nvrhi::rt::AccelStructDesc& requested)
{
    if (cached.isTopLevel != requested.isTopLevel || cached.buildFlags != requested.buildFlags ||
        cached.bottomLevelGeometries.size() != requested.bottomLevelGeometries.size())
        return false;
    for (size_t i = 0; i < cached.bottomLevelGeometries.size(); ++i)
    {
        const nvrhi::rt::GeometryDesc& lhs = cached.bottomLevelGeometries[i];
        const nvrhi::rt::GeometryDesc& rhs = requested.bottomLevelGeometries[i];
        if (lhs.geometryType != nvrhi::rt::GeometryType::Triangles ||
            rhs.geometryType != nvrhi::rt::GeometryType::Triangles)
            return false;
        if (lhs.flags != rhs.flags || lhs.useTransform != rhs.useTransform)
            return false;
        const nvrhi::rt::GeometryTriangles& a = lhs.geometryData.triangles;
        const nvrhi::rt::GeometryTriangles& b = rhs.geometryData.triangles;
        if (a.indexFormat != b.indexFormat || a.indexCount != b.indexCount ||
            a.vertexFormat != b.vertexFormat || a.vertexStride != b.vertexStride ||
            a.vertexCount != b.vertexCount)
            return false;
    }
    return true;
}

bool RTAccelStructManager::AccelStructUpdateMatches(const nvrhi::rt::AccelStructDesc& cached,
    const nvrhi::rt::AccelStructDesc& requested)
{
    if (!AccelStructShapeMatches(cached, requested) || cached.buildFlags != requested.buildFlags)
        return false;
    for (size_t i = 0; i < cached.bottomLevelGeometries.size(); ++i)
    {
        const nvrhi::rt::GeometryTriangles& a = cached.bottomLevelGeometries[i].geometryData.triangles;
        const nvrhi::rt::GeometryTriangles& b = requested.bottomLevelGeometries[i].geometryData.triangles;
        if (a.vertexBuffer != b.vertexBuffer || a.indexBuffer != b.indexBuffer ||
            a.vertexOffset != b.vertexOffset || a.vertexCount != b.vertexCount ||
            a.indexOffset != b.indexOffset)
            return false;
    }
    return true;
}

void RTAccelStructManager::AcquireGeometryBuild(const nvrhi::rt::AccelStructDesc& requested, RTGeometryBuild& slot,
    bool topologyStable, u64 topologyKey)
{
    R_ASSERT(!requested.bottomLevelGeometries.empty());
    nvrhi::rt::AccelStructDesc desc = requested;
    const bool retain = slot.handle && AccelStructShapeMatches(slot.desc, desc);
    const bool update = retain && m_inPlaceUpdates && topologyStable && slot.built &&
        (desc.buildFlags & nvrhi::rt::AccelStructBuildFlags::AllowUpdate) != nvrhi::rt::AccelStructBuildFlags::None &&
        slot.topologyKey == topologyKey && AccelStructUpdateMatches(slot.desc, desc);
    if (!retain)
    {
        slot.desc = std::move(desc);
        slot.handle = m_device->GetNVRHIDevice()->createAccelStruct(slot.desc);
        slot.built = false;
    }
    else
        slot.desc = std::move(desc);
    slot.topologyKey = topologyKey;
    slot.update = slot.handle && update;
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

static constexpr u32 RT_MASK_WORLD = 0x01;
static constexpr u32 RT_MASK_HUD = 0x02;

static nvrhi::rt::InstanceDesc RTInstance(nvrhi::rt::IAccelStruct* blas,
    u32 firstBatch, const RTBatchTransform& transform, bool opaque, u32 mask, bool cullDisable = false)
{
    R_ASSERT(firstBatch < (1u << 24));
    R_ASSERT(mask);
    nvrhi::rt::AffineTransform affine;
    memcpy(&affine, &transform.rows, sizeof(affine));
    nvrhi::rt::InstanceDesc result;
    result.setFlags(nvrhi::rt::InstanceFlags::None);
    if (opaque)
        result.setFlags(nvrhi::rt::InstanceFlags::ForceOpaque);
    if (cullDisable)
        result.setFlags(result.flags | nvrhi::rt::InstanceFlags::TriangleCullDisable);
    result.setTransform(affine).setInstanceID(firstBatch).setInstanceMask(mask).setBLAS(blas);
    return result;
}

bool RTAccelStructManager::s_skinAttempted = false;
bool RTAccelStructManager::s_grassAttempted = false;
bool RTAccelStructManager::s_billboardAttempted = false;

static constexpr u64 RT_IDENTITY_SEED = 14695981039346656037ull;
static constexpr u64 RT_IDENTITY_DOMAIN = 1ull;
static constexpr u32 RT_SOURCE_STATIC = 0;
static constexpr u32 RT_SOURCE_TERRAIN = 1;
static constexpr u32 RT_SOURCE_TRANSPARENT = 2;
static constexpr u32 RT_SOURCE_COUNT = 3;
static constexpr u32 RT_TABLE_EMISSIVE = 1u << 0;
static constexpr u32 RT_TABLE_TRANSFORMS = 1u << 1;
static constexpr u32 RT_TABLE_OFFSETS = 1u << 2;

static u64 RTIdentity(u64 seed, u64 value)
{
    const auto* bytes = reinterpret_cast<const u8*>(&value);
    for (size_t i = 0; i < sizeof(value); ++i)
    {
        seed ^= bytes[i];
        seed *= 1099511628211ull;
    }
    return seed;
}

static u64 RTInstanceIdentity(u64 renderable, u64 visual, u32 subset)
{
    u64 signature = RTIdentity(RT_IDENTITY_SEED, renderable);
    signature = RTIdentity(signature, visual);
    signature = RTIdentity(signature, u64(subset));
    return RTIdentity(signature, RT_IDENTITY_DOMAIN);
}

static u64 RTInstanceIdentity(const GeometryInstanceKey& key)
{
    return RTInstanceIdentity(key.renderable, key.visual, key.subset);
}

static bool IsEmissiveMaterial(u32 materialID)
{
    const auto* material = bindless::MaterialBuffer::Instance().GetMaterial(materialID);
    if (!material)
        return false;
    const auto* variant = bindless::VariantBuffer::Instance().Get(material->shaderVariant);
    if (!variant || variant->emissive <= 0.0f)
        return false;
    return (variant->flags &
        (bindless::VARIANT_FLAG_EMISSIVE | bindless::VARIANT_FLAG_ADDITIVE_EMISSION)) != 0;
}

bool RTAccelStructManager::IsOpaqueMaterialForRT(u32 materialID, bool terrain)
{
    constexpr u32 rejectedMaterialFlags = bindless::MAT_FLAG_ALPHA_TEST | bindless::MAT_FLAG_ALPHA_BLEND |
        bindless::MAT_FLAG_WATER;
    constexpr u32 rejectedVariantFlags = bindless::VARIANT_FLAG_NO_SHADOW |
        bindless::VARIANT_FLAG_ADDITIVE_EMISSION;
    if (terrain)
        return true;
    const auto* material = bindless::MaterialBuffer::Instance().GetMaterial(materialID);
    if (!material || (material->flags & rejectedMaterialFlags))
        return false;
    const auto* variant = bindless::VariantBuffer::Instance().Get(material->shaderVariant);
    if (!variant)
        return false;
    return (variant->flags & rejectedVariantFlags) == 0;
}

static void AppendEmissiveBatch(xr_vector<u32>& offsets, xr_vector<RTEmissiveTriangle>& triangles,
    u32 batchIndex, u32 materialID, u32 indexCount, u64 geometryID)
{
    R_ASSERT(batchIndex == offsets.size());
    const u32 triangleCount = indexCount / 3;
    if (!triangleCount || !IsEmissiveMaterial(materialID))
    {
        offsets.push_back(UINT32_MAX);
        return;
    }
    R_ASSERT2(u64(triangles.size()) + triangleCount <= UINT32_MAX,
        "[RT] emissive triangle table exceeds the addressable range");
    offsets.push_back(u32(triangles.size()));
    RTEmissiveTriangle entry = {};
    entry.batchIndex = batchIndex;
    entry.idLow = u32(geometryID & 0xFFFFFFFFull);
    entry.idHigh = u32(geometryID >> 32);
    for (u32 primitive = 0; primitive < triangleCount; ++primitive)
    {
        entry.primitiveIndex = primitive;
        triangles.push_back(entry);
    }
}

static RTBatchTransform RTBatchTransformOf(const Fmatrix& world)
{
    RTBatchTransform transform = {};
    transform.rows[0][0] = world._11; transform.rows[0][1] = world._21;
    transform.rows[0][2] = world._31; transform.rows[0][3] = world._41;
    transform.rows[1][0] = world._12; transform.rows[1][1] = world._22;
    transform.rows[1][2] = world._32; transform.rows[1][3] = world._42;
    transform.rows[2][0] = world._13; transform.rows[2][1] = world._23;
    transform.rows[2][2] = world._33; transform.rows[2][3] = world._43;
    return transform;
}

static bool IsIdentityWorld(const Fmatrix& world)
{
    return memcmp(&world, &Fidentity, sizeof(Fmatrix)) == 0;
}

static u32 NormalizeDetailTextureIndex(u32 index)
{
    return index == bindless::INVALID_TEXTURE_INDEX ? 0u : index;
}

static bool HasBindlessTexture(u32 index)
{
    return GEnv.Backend && GEnv.Backend->GetBindlessTexture(index) != nullptr;
}

static bool HasPulledDetailTextures(const FGDetailManager* detail)
{
    const u32 atlas = detail->buildDetailsBindlessIndex;
    if (atlas == 0 || atlas == bindless::INVALID_TEXTURE_INDEX || !HasBindlessTexture(atlas))
        return false;
    for (u32 index : { detail->buildDetailsPbrBindlessIndex, detail->buildDetailsBumpBindlessIndex })
    {
        if (index != 0 && index != bindless::INVALID_TEXTURE_INDEX && !HasBindlessTexture(index))
            return false;
    }
    return true;
}

void RTAccelStructManager::Initialize(RenderDevice* device)
{
    m_device = device;
    auto* native = device->GetNVRHIDevice();
    m_rtSupported = native->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct) &&
        native->queryFeatureSupport(nvrhi::Feature::RayQuery);
    const auto* backend = device->GetBackend();
    m_inPlaceUpdates = m_rtSupported && backend && backend->GetCapabilities().rayTracingUpdates;
    m_compaction = m_rtSupported && backend && backend->GetCapabilities().rayTracingCompaction;
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
    m_dynamicGeometry.reset();
    m_dynamicSourceVertices = nullptr;
    m_dynamicSourceIndices = nullptr;
    m_generations.clear();
    m_skinTopologies.clear();
    m_textureScratch.clear();
    m_textureScratch.shrink_to_fit();
    m_staticIdentityHash = 0;
    m_staticIdentityBuildCount = UINT32_MAX;
    m_staticArraysHash = 0;
    m_staticArraysBuildCount = UINT32_MAX;
    m_staticArraysMaterialRevision = 0;
    m_staticArraysVariantRevision = 0;
    InvalidateShaderPipelines();
    m_device = nullptr;
    m_rtSupported = false;
    m_inPlaceUpdates = false;
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

bool RTAccelStructManager::IsSceneReady(const RTSceneGeneration& scene) const
{
    if (scene.failed)
        return false;
    if (!scene.tlas ||
        !scene.geometry ||
        !scene.geometry->vertices ||
        !scene.geometry->indices ||
        !scene.batchInfo ||
        !scene.materials ||
        !scene.terrainMaterials ||
        !scene.variants ||
        !scene.grassMaterials ||
        !scene.emissiveTriangleBuffer ||
        !scene.batchTransformBuffer ||
        !scene.emissiveBatchOffsetBuffer ||
        !scene.textures.GetTable())
        return false;
    if (!scene.skinJobs.empty() && (!scene.skinnedVertices || !scene.skinnedIndices || !scene.skinTopology ||
        !scene.skinBuild.handle))
        return false;
    if (!scene.hudSkinJobs.empty() && (!scene.skinnedVertices || !scene.skinnedIndices || !scene.skinTopology ||
        !scene.hudSkinBuild.handle))
        return false;
    if (scene.counts.grass && (!scene.grassVertices || !scene.grassIndices || !scene.grassBuild.handle))
        return false;
    if (scene.counts.grass && (!scene.grassWind || !scene.grassInteraction[0] || !scene.grassInteraction[1]))
        return false;
    if (!scene.grassJobs.empty() && (!scene.grassPipeline || !scene.grassLayout || !scene.grassWind ||
        !scene.grassFrame || !scene.grassFrame->source))
        return false;
    if (!scene.detailMeshJobs.empty() || !scene.staticDetailJobs.empty())
    {
        if (!scene.pulledPipeline || !scene.pulledLayout || !scene.grassFrame || !scene.grassFrame->source ||
            !scene.grassFrame->source->models || !scene.grassFrame->source->pulledVertices)
            return false;
    }
    return true;
}

bool RTAccelStructManager::IsReady() const
{
    return m_scene && IsSceneReady(*m_scene);
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
            it->scene->failed = true;
            it->scene->skinBuild.built = false;
            it->scene->skinBuild.update = false;
            it->scene->hudSkinBuild.built = false;
            it->scene->hudSkinBuild.update = false;
            it->scene->grassBuild.built = false;
            it->scene->grassBuild.update = false;
            it->scene->tlasBuilt = false;
            it->scene->tlasUpdate = false;
            RTSkinTopology* failedTopology = it->scene->skinTopology.get();
            if (failedTopology)
            {
                failedTopology->uploadPending = false;
                failedTopology->staging.clear();
                failedTopology->staging.shrink_to_fit();
                for (auto cached = m_skinTopologies.begin(); cached != m_skinTopologies.end(); ++cached)
                {
                    if (cached->second.get() == failedTopology)
                    {
                        m_skinTopologies.erase(cached);
                        break;
                    }
                }
                for (auto& generation : m_generations)
                {
                    if (generation->skinTopology.get() == failedTopology)
                    {
                        generation->failed = true;
                        generation->recorded = false;
                    }
                }
            }
            if (it->scene->dynamicGeometry)
            {
                auto failedDynamic = it->scene->dynamicGeometry;
                failedDynamic->recorded = false;
                if (m_dynamicGeometry == failedDynamic)
                    m_dynamicGeometry.reset();
                for (auto& generation : m_generations)
                {
                    if (generation->dynamicGeometry == failedDynamic)
                    {
                        generation->failed = true;
                        generation->recorded = false;
                    }
                }
            }
            if (it->scene->geometry == m_staticGeometry)
            {
                m_scene.reset();
                m_staticGeometry.reset();
                m_dynamicGeometry.reset();
            }
            else if (it->scene == m_scene)
                m_scene.reset();
        }
        if (it->scene != m_scene && it->scene->leases == 0 && it->scene->recorded && it->scene->retention == 0)
        {
            it->scene->skinTopology.reset();
            it->scene->skinnedIndices = nullptr;
            it->scene->skinJobs.clear();
            it->scene->hudSkinJobs.clear();
            it->scene->skinSources.clear();
            it->scene->grassJobs.clear();
            it->scene->detailMeshJobs.clear();
            it->scene->staticDetailJobs.clear();
            it->scene->emissiveTriangles.clear();
            it->scene->emissiveBatchOffsets.clear();
            it->scene->batchTransforms.clear();
            it->scene->batchIdentities.clear();
            it->scene->dynamicGeometry.reset();
            it->scene->bones = nullptr;
            it->scene->sourceMaterials = nullptr;
            it->scene->sourceTerrainMaterials = nullptr;
            it->scene->sourceVariants = nullptr;
            it->scene->grassFrame.reset();
            it->scene->grassPipeline = nullptr;
            it->scene->grassLayout = nullptr;
            it->scene->pulledPipeline = nullptr;
            it->scene->pulledLayout = nullptr;
            it->scene->grassWind = nullptr;
            for (auto& texture : it->scene->grassInteraction)
                texture = nullptr;
            it->scene->detailMeshBatchStart = UINT32_MAX;
            it->scene->staticDetailBatchStart = UINT32_MAX;
            it->scene->staticDetailInstanceCount = 0;
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

u64 RTAccelStructManager::ComputeStaticSignature(const GPUCullingManager* gpu)
{
    u64 signature = 14695981039346656037ull;
    const auto* vertices = gpu->GetRTVertexBuffer();
    const auto* indices = gpu->GetRTIndexBuffer();
    HashSceneData(signature, &vertices, sizeof(vertices));
    HashSceneData(signature, &indices, sizeof(indices));
    auto append = [](u64& target, const auto& values)
    {
        const size_t count = values.size();
        HashSceneData(target, &count, sizeof(count));
        HashSceneData(target, values.data(), count * sizeof(*values.data()));
    };
    auto appendInstanceShape = [](u64& target, const xr_vector<GPUInstanceData>& instances)
    {
        const size_t count = instances.size();
        HashSceneData(target, &count, sizeof(count));
        for (const auto& instance : instances)
        {
            const u8 identityTransform = IsIdentityWorld(instance.world) ? 1u : 0u;
            HashSceneData(target, &identityTransform, sizeof(identityTransform));
        }
    };
    auto appendOpacity = [](u64& target, const xr_vector<u32>& materials, bool terrain)
    {
        const size_t count = materials.size();
        HashSceneData(target, &count, sizeof(count));
        for (u32 materialID : materials)
        {
            const u8 opaque = IsOpaqueMaterialForRT(materialID, terrain) ? 1u : 0u;
            HashSceneData(target, &opaque, sizeof(opaque));
        }
    };
    const u64 materialRevision = bindless::MaterialBuffer::Instance().GetRevision();
    const u64 variantRevision = bindless::VariantBuffer::Instance().GetRevision();
    if (m_staticArraysBuildCount != gpu->GetStaticBuildCount() || m_staticArraysMaterialRevision != materialRevision ||
        m_staticArraysVariantRevision != variantRevision)
    {
        u64 arrays = RT_IDENTITY_SEED;
        append(arrays, gpu->GetStaticDrawArgsData());
        appendInstanceShape(arrays, gpu->GetStaticInstanceData());
        append(arrays, gpu->GetStaticBatchVertexCounts());
        append(arrays, gpu->GetStaticMaterialIDData());
        appendOpacity(arrays, gpu->GetStaticMaterialIDData(), false);
        append(arrays, gpu->GetTerrainDrawArgsData());
        append(arrays, gpu->GetTerrainMaterialIDData());
        appendOpacity(arrays, gpu->GetTerrainMaterialIDData(), true);
        appendInstanceShape(arrays, gpu->GetTerrainInstanceData());
        m_staticArraysHash = arrays;
        m_staticArraysBuildCount = gpu->GetStaticBuildCount();
        m_staticArraysMaterialRevision = materialRevision;
        m_staticArraysVariantRevision = variantRevision;
    }
    HashSceneData(signature, &m_staticArraysHash, sizeof(m_staticArraysHash));
    append(signature, gpu->GetTransparentDrawArgsData());
    append(signature, gpu->GetTransparentMaterialIDData());
    appendOpacity(signature, gpu->GetTransparentMaterialIDData(), false);
    appendInstanceShape(signature, gpu->GetTransparentInstanceData());
    return signature;
}

static CKinematics* RTBatchSkeleton(const GeometryBatch& batch)
{
    if (batch.visual->getType() == MT_SKELETON_GEOMDEF_ST)
        return static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
    if (batch.visual->getType() == MT_SKELETON_GEOMDEF_PM)
        return static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
    return nullptr;
}

static void RTSkinBatchSources(const GeometryBatch& batch, const SkinnedGeometryPools& pools,
    const void*& vertices, const void*& staging)
{
    vertices = nullptr;
    staging = nullptr;
    const u32 format = batch.skinnedPoolFormat;
    if (format >= SkinnedGeometryPools::FIRST_FORMAT && format < SkinnedGeometryPools::FORMAT_COUNT)
    {
        vertices = pools.GetVertexBuffer(format);
        return;
    }
    auto* mesh = static_cast<IRender_Mesh*>(static_cast<Fvisual*>(batch.visual));
    if (mesh->p_rm_Vertices)
        vertices = mesh->p_rm_Vertices->GetBufferHandle().Get();
    staging = mesh->p_rm_Indices;
}

static bool RTGrassWaves(const FGDetailManager::VisibilityFrame& frame)
{
    const auto& stats = frame.stats;
    const bool procedural = frame.source->params.grassMode &&
        (stats.visibleLOD0Count || stats.visibleLOD1Count || stats.visibleLOD2Count);
    const bool waved = !frame.source->params.grassMode && stats.visibleBillboardCount;
    return procedural || waved;
}

u64 RTAccelStructManager::ComputeTopologySignature(const GPUCullingManager* gpu, const FGDetailManager* detail,
    const xr_vector<GeometryBatch>& world, const xr_vector<GeometryBatch>& hud)
{
    u64 signature = m_staticSignature;
    auto append = [&](const auto& value)
    {
        HashSceneData(signature, &value, sizeof(value));
    };
    append(bindless::MaterialBuffer::Instance().GetRevision());
    append(bindless::TerrainMaterialBuffer::Instance().GetRevision());
    append(bindless::VariantBuffer::Instance().GetRevision());
    auto appendIdentities = [](u64& target, const xr_vector<GeometryInstanceKey>& identities)
    {
        const size_t count = identities.size();
        HashSceneData(target, &count, sizeof(count));
        for (const auto& identity : identities)
        {
            const u64 value = RTInstanceIdentity(identity);
            HashSceneData(target, &value, sizeof(value));
        }
    };
    if (m_staticIdentityBuildCount != gpu->GetStaticBuildCount())
    {
        m_staticIdentityHash = RT_IDENTITY_SEED;
        appendIdentities(m_staticIdentityHash, gpu->GetStaticInstanceIdentities());
        appendIdentities(m_staticIdentityHash, gpu->GetTerrainInstanceIdentities());
        m_staticIdentityBuildCount = gpu->GetStaticBuildCount();
    }
    append(m_staticIdentityHash);
    appendIdentities(signature, gpu->GetTransparentInstanceIdentities());
    appendIdentities(signature, gpu->GetDynamicInstanceIdentities());
    const auto& dynamicKeys = gpu->GetDynamicMeshKeys();
    const size_t dynamicKeyCount = dynamicKeys.size();
    HashSceneData(signature, &dynamicKeyCount, sizeof(dynamicKeyCount));
    HashSceneData(signature, dynamicKeys.data(), dynamicKeyCount * sizeof(ClusterMeshKey));
    const auto& pools = gpu->GetSkinnedPools();
    auto appendBatches = [&](const xr_vector<GeometryBatch>& batches)
    {
        append(batches.size());
        for (const auto& batch : batches)
        {
            append(batch.visualLifetimeID);
            append(batch.renderableLifetimeID);
            append(batch.geometrySubset);
            append(batch.bindlessMaterialID);
            append(batch.indexCount);
            append(batch.startIndex);
            append(batch.vertexCount);
            append(batch.skinnedPoolFormat);
            append(batch.skinnedPoolBaseVertex);
            append(batch.skinnedPoolFirstIndex);
            append(batch.skinningRenderMode);
            const void* vertices;
            const void* staging;
            RTSkinBatchSources(batch, pools, vertices, staging);
            append(vertices);
            append(staging);
        }
    };
    appendBatches(world);
    appendBatches(hud);
    const auto frame = detail ? detail->GetCompletedRayVisibilityFrame() : nullptr;
    if (frame && frame->source)
    {
        append(frame->source->id);
        append(frame->statsReady ? frame->contentSignature : frame->id);
        append(frame->stats.visibleBillboardCount);
        append(frame->stats.visibleDecalCount);
        append(frame->stats.visibleLOD0Count);
        append(frame->stats.visibleLOD1Count);
        append(frame->stats.visibleLOD2Count);
        append(detail->buildDetailsBindlessIndex);
        append(detail->buildDetailsPbrBindlessIndex);
        append(detail->buildDetailsBumpBindlessIndex);
    }
    return signature;
}

RTPoseSignature RTAccelStructManager::ComputePoseSignature(const GPUCullingManager* gpu, const FGDetailManager* detail,
    const xr_vector<GeometryBatch>& world, const xr_vector<GeometryBatch>& hud) const
{
    u64 motion = RT_IDENTITY_SEED;
    auto append = [&](const auto& value)
    {
        HashSceneData(motion, &value, sizeof(value));
    };
    auto appendWorlds = [&](const xr_vector<GPUInstanceData>& instances)
    {
        append(instances.size());
        for (const auto& instance : instances)
            HashSceneData(motion, &instance.world, sizeof(instance.world));
    };
    appendWorlds(gpu->GetTransparentInstanceData());
    appendWorlds(gpu->GetDynamicInstanceData());
    auto appendBatches = [&](const xr_vector<GeometryBatch>& batches)
    {
        append(batches.size());
        for (const auto& batch : batches)
        {
            append(batch.worldMatrix);
            CKinematics* skeleton = RTBatchSkeleton(batch);
            R_ASSERT(skeleton);
            append(gpu->GetPreparedSkeletonOffset(skeleton));
            u32 boneCount = 0;
            const Fmatrix* bones = gpu->GetPreparedSkeletonMatrices(skeleton, boneCount);
            append(boneCount);
            HashSceneData(motion, bones, size_t(boneCount) * sizeof(Fmatrix));
        }
    };
    appendBatches(world);
    appendBatches(hud);
    RTPoseSignature result;
    result.motion = motion;
    result.refresh = motion;
    const auto frame = detail ? detail->GetCompletedRayVisibilityFrame() : nullptr;
    if (frame && frame->source && frame->statsReady && RTGrassWaves(*frame))
        HashSceneData(result.refresh, &Device.fTimeGlobal, sizeof(Device.fTimeGlobal));
    return result;
}

u64 RTAccelStructManager::GetSceneRevision() const
{
    return m_sceneRevision;
}

u64 RTAccelStructManager::GetPoseRevision() const
{
    return m_poseRevision;
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
        if (!EnsureRTBuffer(m_device->GetNVRHIDevice(), geometry->vertices, desc))
        {
            geometry->failed = true;
            m_staticGeometry = std::move(geometry);
            return;
        }
        desc.debugName = "RT_EmptySourceIndices";
        desc.byteSize = sizeof(u32);
        desc.structStride = 0;
        desc.canHaveRawViews = true;
        desc.isIndexBuffer = true;
        if (!EnsureRTBuffer(m_device->GetNVRHIDevice(), geometry->indices, desc))
        {
            geometry->failed = true;
            m_staticGeometry = std::move(geometry);
            return;
        }
    }
    const auto& staticArgs = gpu->GetStaticDrawArgsData();
    const auto& staticInstances = gpu->GetStaticInstanceData();
    const auto& staticCounts = gpu->GetStaticBatchVertexCounts();
    const auto& staticMaterials = gpu->GetStaticMaterialIDData();
    const auto& staticIdentities = gpu->GetStaticInstanceIdentities();
    const auto& terrainArgs = gpu->GetTerrainDrawArgsData();
    const auto& terrainInstances = gpu->GetTerrainInstanceData();
    const auto& terrainMaterials = gpu->GetTerrainMaterialIDData();
    const auto& terrainIdentities = gpu->GetTerrainInstanceIdentities();
    const auto& transparentArgs = gpu->GetTransparentDrawArgsData();
    const auto& transparentInstances = gpu->GetTransparentInstanceData();
    const auto& transparentMaterials = gpu->GetTransparentMaterialIDData();
    const auto& transparentIdentities = gpu->GetTransparentInstanceIdentities();
    R_ASSERT(staticArgs.size() == staticInstances.size());
    R_ASSERT(staticArgs.size() == staticCounts.size());
    R_ASSERT(staticArgs.size() == staticMaterials.size());
    R_ASSERT(staticArgs.size() == staticIdentities.size());
    R_ASSERT(terrainArgs.size() == terrainInstances.size());
    R_ASSERT(terrainArgs.size() == terrainMaterials.size());
    R_ASSERT(terrainArgs.size() == terrainIdentities.size());
    R_ASSERT(transparentArgs.size() == transparentInstances.size());
    R_ASSERT(transparentArgs.size() == transparentMaterials.size());
    R_ASSERT(transparentArgs.size() == transparentIdentities.size());
    auto* device = m_device->GetNVRHIDevice();
    xr_map<std::tuple<u32, s32, u32, u32, bool>, u32> unique;
    auto resolveBatch = [&](const IndirectDrawArgs& args, u32 index, const xr_vector<u32>* batchCounts,
        u32& startIndex, s32& baseVertex, u32& indexCount, u32& vertices)
    {
        indexCount = args.indexCountPerInstance;
        if (!indexCount)
            return false;
        if (batchCounts)
            R_ASSERT(args.baseVertexLocation >= 0);
        else if (args.baseVertexLocation < 0)
            return false;
        baseVertex = args.baseVertexLocation;
        startIndex = args.startIndexLocation;
        const u32 base = u32(baseVertex);
        if (base >= gpu->GetRTVertexCount() || u64(startIndex) + indexCount > gpu->GetRTIndexCount())
            return false;
        vertices = batchCounts ? (*batchCounts)[index] : gpu->GetRTVertexCount() - base;
        if (u64(base) + vertices > gpu->GetRTVertexCount())
            return false;
        return true;
    };
    auto appendSharedBatches = [&](const char* name, u32 arrayIndex, const xr_vector<IndirectDrawArgs>& args,
        const xr_vector<u32>& materials, const xr_vector<GPUInstanceData>& instances,
        const xr_vector<u32>* batchCounts)
    {
        RTGeometryBuild shared;
        shared.desc.debugName = name;
        shared.desc.buildFlags = StaticBuildFlags();
        const u32 firstBatch = u32(geometry->batches.size());
        bool opaque = true;
        for (u32 i = 0; i < args.size(); ++i)
        {
            if (!IsIdentityWorld(instances[i].world))
                continue;
            u32 startIndex = 0, indexCount = 0, vertices = 0;
            s32 baseVertex = 0;
            if (!resolveBatch(args[i], i, batchCounts, startIndex, baseVertex, indexCount, vertices))
                continue;
            const bool batchOpaque = IsOpaqueMaterialForRT(materials[i], arrayIndex == RT_SOURCE_TERRAIN);
            shared.desc.addBottomLevelGeometry(RTTriangles(geometry->vertices, geometry->indices,
                GPUCullingManager::RT_VERTEX_STRIDE, u32(baseVertex), vertices, startIndex, indexCount, batchOpaque));
            opaque = opaque && batchOpaque;
            geometry->batches.push_back({ materials[i], startIndex, baseVertex, indexCount });
            geometry->batchSources.push_back({ arrayIndex, i });
        }
        if (geometry->batches.size() == firstBatch)
            return;
        shared.handle = device->createAccelStruct(shared.desc);
        if (!shared.handle)
        {
            geometry->failed = true;
            return;
        }
        geometry->instances.push_back(
            RTInstance(shared.handle, firstBatch, RTBatchTransformOf(Fidentity), opaque, RT_MASK_WORLD));
        geometry->instanceBatches.push_back(-1);
        geometry->builds.push_back(std::move(shared));
    };
    auto appendInstanceBatches = [&](u32 arrayIndex, const xr_vector<IndirectDrawArgs>& args,
        const xr_vector<u32>& materials, const xr_vector<GPUInstanceData>& instances,
        const xr_vector<u32>* batchCounts)
    {
        for (u32 i = 0; i < args.size(); ++i)
        {
            if (IsIdentityWorld(instances[i].world))
                continue;
            u32 startIndex = 0, indexCount = 0, vertices = 0;
            s32 baseVertex = 0;
            if (!resolveBatch(args[i], i, batchCounts, startIndex, baseVertex, indexCount, vertices))
                continue;
            const bool batchOpaque = IsOpaqueMaterialForRT(materials[i], arrayIndex == RT_SOURCE_TERRAIN);
            const auto key = std::make_tuple(startIndex, baseVertex, indexCount, vertices, batchOpaque);
            auto found = unique.find(key);
            u32 buildIndex;
            if (found == unique.end())
            {
                RTGeometryBuild build;
                build.desc.debugName = "RT_InstancedBLAS";
                build.desc.buildFlags = StaticBuildFlags();
                build.desc.addBottomLevelGeometry(RTTriangles(geometry->vertices, geometry->indices,
                    GPUCullingManager::RT_VERTEX_STRIDE, u32(baseVertex), vertices, startIndex, indexCount, batchOpaque));
                build.handle = device->createAccelStruct(build.desc);
                if (!build.handle)
                {
                    geometry->failed = true;
                    return;
                }
                buildIndex = u32(geometry->builds.size());
                geometry->builds.push_back(std::move(build));
                unique.emplace(key, buildIndex);
            }
            else
                buildIndex = found->second;
            const u32 batchIndex = u32(geometry->batches.size());
            geometry->instances.push_back(RTInstance(geometry->builds[buildIndex].handle,
                batchIndex, RTBatchTransformOf(Fidentity), batchOpaque, RT_MASK_WORLD));
            geometry->instanceBatches.push_back(s32(batchIndex));
            geometry->batches.push_back({ materials[i], startIndex, baseVertex, indexCount });
            geometry->batchSources.push_back({ arrayIndex, i });
        }
    };
    const u32 identityStaticFirst = u32(geometry->batches.size());
    appendSharedBatches("RT_IdentityBLAS", RT_SOURCE_STATIC, staticArgs, staticMaterials, staticInstances, &staticCounts);
    geometry->counts.identityStatic = u32(geometry->batches.size()) - identityStaticFirst;
    const u32 terrainFirst = u32(geometry->batches.size());
    appendSharedBatches("RT_TerrainBLAS", RT_SOURCE_TERRAIN, terrainArgs, terrainMaterials, terrainInstances, nullptr);
    appendInstanceBatches(RT_SOURCE_TERRAIN, terrainArgs, terrainMaterials, terrainInstances, nullptr);
    geometry->counts.terrain = u32(geometry->batches.size()) - terrainFirst;
    const u32 transparentFirst = u32(geometry->batches.size());
    appendSharedBatches("RT_TransparentBLAS", RT_SOURCE_TRANSPARENT, transparentArgs, transparentMaterials,
        transparentInstances, nullptr);
    appendInstanceBatches(RT_SOURCE_TRANSPARENT, transparentArgs, transparentMaterials, transparentInstances, nullptr);
    geometry->counts.transparent = u32(geometry->batches.size()) - transparentFirst;
    const u32 instancedFirst = u32(geometry->batches.size());
    appendInstanceBatches(RT_SOURCE_STATIC, staticArgs, staticMaterials, staticInstances, &staticCounts);
    geometry->counts.instancedTotal = u32(geometry->batches.size()) - instancedFirst;
    if (geometry->failed)
    {
        m_staticGeometry = std::move(geometry);
        return;
    }
    R_ASSERT(geometry->batchSources.size() == geometry->batches.size());
    R_ASSERT(geometry->instanceBatches.size() == geometry->instances.size());
    m_textureScratch.clear();
    const u32 terrainEnd = geometry->counts.identityStatic + geometry->counts.terrain;
    for (u32 i = 0; i < geometry->batches.size(); ++i)
        AppendMaterialTextures(geometry->batches[i].materialID,
            i >= geometry->counts.identityStatic && i < terrainEnd);
    geometry->textures.Capture(m_textureScratch);
    m_staticGeometry = std::move(geometry);
    m_staticSignature = signature;
}

bool RTDynamicRange::operator<(const RTDynamicRange& other) const
{
    if (vertexOffset != other.vertexOffset)
        return vertexOffset < other.vertexOffset;
    if (indexOffset != other.indexOffset)
        return indexOffset < other.indexOffset;
    if (vertexCount != other.vertexCount)
        return vertexCount < other.vertexCount;
    if (indexCount != other.indexCount)
        return indexCount < other.indexCount;
    return opaque < other.opaque;
}

bool RTAccelStructManager::EnsureDynamicGeometry(GPUCullingManager* gpu)
{
    auto* vertices = gpu->GetRTVertexBuffer();
    auto* indices = gpu->GetRTIndexBuffer();
    R_ASSERT(vertices && indices);
    if (!m_dynamicGeometry || m_dynamicSourceVertices != vertices || m_dynamicSourceIndices != indices)
    {
        m_dynamicGeometry = std::make_shared<RTDynamicGeometry>();
        m_dynamicSourceVertices = vertices;
        m_dynamicSourceIndices = indices;
    }
    if (m_dynamicGeometry->failed)
        return false;
    auto* device = m_device->GetNVRHIDevice();
    const auto& keys = gpu->GetDynamicMeshKeys();
    const auto& instances = gpu->GetDynamicInstanceData();
    R_ASSERT(instances.size() == keys.size());
    for (u32 index = 0; index < keys.size(); ++index)
    {
        const auto& key = keys[index];
        if (!key.vertexCount || !key.indexCount)
            continue;
        R_ASSERT(key.indexCount % 3 == 0);
        R_ASSERT(key.vertexOffset <= 0x7FFFFFFFu);
        R_ASSERT(u64(key.vertexOffset) + key.vertexCount <= gpu->GetRTVertexCount());
        R_ASSERT(u64(key.indexOffset) + key.indexCount <= gpu->GetRTIndexCount());
        RTDynamicRange range = {};
        range.vertexOffset = key.vertexOffset;
        range.indexOffset = key.indexOffset;
        range.vertexCount = key.vertexCount;
        range.indexCount = key.indexCount;
        range.opaque = IsOpaqueMaterialForRT(instances[index].materialID, false);
        if (m_dynamicGeometry->recordBuilds.find(range) != m_dynamicGeometry->recordBuilds.end())
            continue;
        RTGeometryBuild build;
        build.desc.debugName = "RT_DynamicBLAS";
        build.desc.buildFlags = StaticBuildFlags();
        build.desc.addBottomLevelGeometry(RTTriangles(vertices, indices, GPUCullingManager::RT_VERTEX_STRIDE,
            range.vertexOffset, range.vertexCount, range.indexOffset, range.indexCount, range.opaque));
        build.handle = device->createAccelStruct(build.desc);
        if (!build.handle)
        {
            m_dynamicGeometry->failed = true;
            return false;
        }
        m_dynamicGeometry->recordBuilds.emplace(range, u32(m_dynamicGeometry->builds.size()));
        m_dynamicGeometry->builds.push_back(std::move(build));
        m_dynamicGeometry->recorded = false;
    }
    return true;
}

void RTAccelStructManager::PrepareDynamic(RTSceneGeneration& scene, GPUCullingManager* gpu)
{
    if (!m_dynamicGeometry)
        return;
    const auto& instances = gpu->GetDynamicInstanceData();
    const auto& identities = gpu->GetDynamicInstanceIdentities();
    const auto& keys = gpu->GetDynamicMeshKeys();
    R_ASSERT(instances.size() == identities.size());
    R_ASSERT(instances.size() == keys.size());
    const u32 firstBatch = u32(scene.batches.size());
    for (u32 i = 0; i < keys.size(); ++i)
    {
        RTDynamicRange range = {};
        range.vertexOffset = keys[i].vertexOffset;
        range.indexOffset = keys[i].indexOffset;
        range.vertexCount = keys[i].vertexCount;
        range.indexCount = keys[i].indexCount;
        range.opaque = IsOpaqueMaterialForRT(instances[i].materialID, false);
        auto found = m_dynamicGeometry->recordBuilds.find(range);
        if (found == m_dynamicGeometry->recordBuilds.end())
            continue;
        const u32 batchIndex = u32(scene.batches.size());
        const RTBatchTransform transform = RTBatchTransformOf(instances[i].world);
        const u64 identity = RTInstanceIdentity(identities[i]);
        scene.instances.push_back(RTInstance(m_dynamicGeometry->builds[found->second].handle,
            batchIndex, transform, range.opaque, RT_MASK_WORLD));
        scene.batches.push_back({ instances[i].materialID, range.indexOffset, s32(range.vertexOffset), range.indexCount });
        scene.batchTransforms.push_back(transform);
        scene.batchIdentities.push_back(identity);
        AppendEmissiveBatch(scene.emissiveBatchOffsets, scene.emissiveTriangles, batchIndex,
            instances[i].materialID, range.indexCount, identity);
    }
    const u32 count = u32(scene.batches.size()) - firstBatch;
    scene.counts.instancedTotal += count;
    scene.counts.dynamic = count;
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
    RTGeometryBuild retainedSkinBuild = std::move(scene.skinBuild);
    RTGeometryBuild retainedHudSkinBuild = std::move(scene.hudSkinBuild);
    scene.skinBuild = {};
    scene.hudSkinBuild = {};
    if (world.empty() && hud.empty())
        return;
    const auto& pools = gpu->GetSkinnedPools();
    u64 vertexCount = 0;
    u64 indexCount = 0;
    u64 topologyKey = RT_IDENTITY_SEED;
    xr_vector<RTSkinSourcePlan> plans;
    plans.reserve(world.size() + hud.size());
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
    auto append = [&](const GeometryBatch& batch, xr_vector<RTSkinJob>& jobs)
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
            && indexCount + u64(batch.indexCount) <= UINT32_MAX);
        nvrhi::IBuffer* source = nullptr;
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
            if (!pools.GetIndexRange(format, batch.skinnedPoolFirstIndex, batch.indexCount))
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
        job.indexOffset = u32(indexCount);
        job.indexCount = batch.indexCount;
        job.materialID = batch.bindlessMaterialID;
        job.geometryID = RTInstanceIdentity(batch.renderableLifetimeID, batch.visualLifetimeID,
            batch.geometrySubset);
        RTSkinSourcePlan plan;
        plan.source = source;
        plan.staging = mapped;
        plan.format = format;
        plan.firstIndex = pooled ? batch.skinnedPoolFirstIndex : batch.startIndex;
        plan.indexCount = batch.indexCount;
        plan.indexOffset = u32(indexCount);
        plan.vertexCount = vertices;
        plan.baseVertex = baseVertex;
        plan.pooled = pooled;
        topologyKey = RTIdentity(topologyKey, batch.renderableLifetimeID);
        topologyKey = RTIdentity(topologyKey, batch.visualLifetimeID);
        topologyKey = RTIdentity(topologyKey, u64(batch.geometrySubset));
        topologyKey = RTIdentity(topologyKey, u64(pooled ? format : 0xFFFFFFFFu));
        topologyKey = RTIdentity(topologyKey, u64(plan.firstIndex));
        topologyKey = RTIdentity(topologyKey, u64(batch.indexCount));
        topologyKey = RTIdentity(topologyKey, u64(vertices));
        topologyKey = RTIdentity(topologyKey, u64(baseVertex));
        topologyKey = RTIdentity(topologyKey, u64(reinterpret_cast<size_t>(source)));
        topologyKey = RTIdentity(topologyKey, u64(reinterpret_cast<size_t>(mapped)));
        plans.push_back(plan);
        indexCount += batch.indexCount;
        vertexCount += vertices;
        jobs.push_back(std::move(job));
    };
    for (const auto& batch : world)
        append(batch, scene.skinJobs);
    for (const auto& batch : hud)
        append(batch, scene.hudSkinJobs);
    topologyKey = RTIdentity(topologyKey, indexCount);
    topologyKey = RTIdentity(topologyKey, u64(plans.size()));
    std::shared_ptr<RTSkinTopology> topology;
    auto cached = m_skinTopologies.find(topologyKey);
    if (cached != m_skinTopologies.end() && cached->second->indexCount == u32(indexCount))
        topology = cached->second;
    const bool topologyStable = topology && !topology->uploadPending;
    for (auto it = m_skinTopologies.begin(); it != m_skinTopologies.end();)
    {
        if (it->second.use_count() == 1)
            it = m_skinTopologies.erase(it);
        else
            ++it;
    }
    auto* device = m_device->GetNVRHIDevice();
    if (!topology)
    {
        topology = std::make_shared<RTSkinTopology>();
        topology->indexCount = u32(indexCount);
        topology->staging.resize(size_t(indexCount));
        for (const auto& plan : plans)
        {
            const u16* indices = nullptr;
            if (plan.pooled)
            {
                indices = pools.GetIndexRange(plan.format, plan.firstIndex, plan.indexCount);
                if (!indices)
                    FATAL_F("[RT] pooled skinned index range %u+%u escapes format %u",
                        plan.firstIndex, plan.indexCount, plan.format);
            }
            else
            {
                R_ASSERT(plan.staging);
                const auto* host = static_cast<const u16*>(plan.staging->Map(0, 0, true));
                R_ASSERT(host);
                indices = host + plan.firstIndex;
            }
            for (u32 i = 0; i < plan.indexCount; ++i)
            {
                const u32 index = indices[i];
                R_ASSERT(index < plan.vertexCount);
                topology->staging[size_t(plan.indexOffset) + i] = index;
            }
            if (plan.staging)
                plan.staging->Unmap();
        }
        if (!EnsureRTBuffer(device, topology->indices,
            RTVertexBufferDesc("RT_SkinIndices", indexCount * sizeof(u32))))
        {
            scene.failed = true;
            return;
        }
        topology->uploadPending = true;
        m_skinTopologies[topologyKey] = topology;
    }
    scene.skinTopology = topology;
    scene.skinnedIndices = topology->indices;
    if (!EnsureRTBuffer(device, scene.skinnedVertices,
        RTVertexBufferDesc("RT_SkinVertices", vertexCount * SKIN_VERTEX_STRIDE)))
    {
        scene.failed = true;
        return;
    }
    scene.bones = gpu->GetGlobalBoneBuffer();
    if (!scene.bones)
    {
        scene.failed = true;
        return;
    }
    const RTBatchTransform identityTransform = RTBatchTransformOf(Fidentity);
    auto buildSkin = [&](xr_vector<RTSkinJob>& jobs, RTGeometryBuild& build, const char* name, u32 mask)
    {
        if (jobs.empty())
        {
            build = {};
            return;
        }
        nvrhi::rt::AccelStructDesc desc;
        desc.debugName = name;
        desc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastBuild;
        if (m_inPlaceUpdates)
            desc.buildFlags = desc.buildFlags | nvrhi::rt::AccelStructBuildFlags::AllowUpdate;
        const u32 firstBatch = u32(scene.batches.size());
        bool opaque = true;
        for (const auto& job : jobs)
        {
            const bool jobOpaque = IsOpaqueMaterialForRT(job.materialID, false);
            desc.addBottomLevelGeometry(RTTriangles(scene.skinnedVertices, topology->indices,
                SKIN_VERTEX_STRIDE, job.constants.outputOffset, job.constants.vertexCount,
                job.indexOffset, job.indexCount, jobOpaque));
            opaque = opaque && jobOpaque;
            const u32 batchIndex = u32(scene.batches.size());
            scene.batches.push_back({ job.materialID, job.indexOffset, s32(job.constants.outputOffset), job.indexCount });
            scene.batchTransforms.push_back(identityTransform);
            scene.batchIdentities.push_back(job.geometryID);
            if (mask == RT_MASK_WORLD)
                AppendEmissiveBatch(scene.emissiveBatchOffsets, scene.emissiveTriangles, batchIndex,
                    job.materialID, job.indexCount, job.geometryID);
            else
                scene.emissiveBatchOffsets.push_back(UINT32_MAX);
        }
        AcquireGeometryBuild(desc, build, topologyStable, topologyKey);
        if (!build.handle)
        {
            scene.failed = true;
            return;
        }
        scene.instances.push_back(RTInstance(build.handle, firstBatch, identityTransform, opaque, mask));
    };
    buildSkin(scene.skinJobs, retainedSkinBuild, "RT_SkinBLAS", RT_MASK_WORLD);
    if (scene.failed)
        return;
    buildSkin(scene.hudSkinJobs, retainedHudSkinBuild, "RT_HudSkinBLAS", RT_MASK_HUD);
    if (scene.failed)
        return;
    scene.skinBuild = std::move(retainedSkinBuild);
    scene.hudSkinBuild = std::move(retainedHudSkinBuild);
    scene.counts.skinned = u32(scene.skinJobs.size() + scene.hudSkinJobs.size());
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
    const auto frame = detail ? detail->GetCompletedRayVisibilityFrame() : nullptr;
    if (detail && detail->IsRayTracingCoverageEnabled() && (!frame || !frame->source || !frame->statsReady))
        return false;
    if (!frame || !frame->source || frame->source->chunks.empty())
        return true;
    const auto& source = *frame->source;
    const auto& stats = frame->stats;
    const bool waved = !source.params.grassMode && stats.visibleBillboardCount != 0;
    const bool staticDetail = stats.visibleDecalCount != 0;
    const bool procedural = source.params.grassMode &&
        (stats.visibleLOD0Count || stats.visibleLOD1Count || stats.visibleLOD2Count);
    if (waved || staticDetail || procedural)
    {
        if (!detail->visibilityFrame || !detail->perlin4dTexture ||
            !detail->interactionTexture[0] || !detail->interactionTexture[1])
            return false;
    }
    if (waved || procedural)
    {
        if (!detail->perlin4dPipeline || !detail->perlin4dCB.IsValid() ||
            !m_device->GetNativeBuffer(detail->perlin4dCB) || !detail->interactionPipeline ||
            !detail->interactionCB.IsValid() || !m_device->GetNativeBuffer(detail->interactionCB) ||
            !detail->interactionEntityBuffer || !detail->heightmapTexture)
            return false;
    }
    if (waved || staticDetail)
    {
        if (source.maxPulledIndexCount / 3 * 3 == 0)
            return false;
        if (!source.bindingLayout || !source.descriptorTable || !source.models || !source.pulledVertices)
            return false;
        if (waved && !frame->visible[FGDetailManager::VIS_KIND_MESH])
            return false;
        if (staticDetail && !frame->visible[FGDetailManager::VIS_KIND_DECAL])
            return false;
        if (!HasPulledDetailTextures(detail))
            return false;
        if (!InitBillboardPipeline(source))
            return false;
    }
    if (!source.params.grassMode)
        return true;
    const u32 counts[] = { stats.visibleLOD0Count, stats.visibleLOD1Count, stats.visibleLOD2Count };
    if (!counts[0] && !counts[1] && !counts[2])
        return true;
    if (!source.bindingLayout || !source.descriptorTable || !detail->perlin4dTexture)
        return false;
    for (u32 i = 0; i < 3; ++i)
    {
        if (counts[i] && !frame->visible[i])
            return false;
    }
    return InitGrassPipeline(source);
}

void RTAccelStructManager::PrepareGrass(RTSceneGeneration& scene, FGDetailManager* detail)
{
    RTGeometryBuild retainedGrassBuild = std::move(scene.grassBuild);
    scene.grassBuild = {};
    scene.grassFrame.reset();
    scene.grassPipeline = nullptr;
    scene.grassLayout = nullptr;
    scene.pulledPipeline = nullptr;
    scene.pulledLayout = nullptr;
    scene.grassWind = nullptr;
    for (auto& texture : scene.grassInteraction)
        texture = nullptr;
    scene.detailMeshBatchStart = UINT32_MAX;
    scene.staticDetailBatchStart = UINT32_MAX;
    scene.detailPbrIndex = 0;
    scene.detailBumpIndex = 0;
    scene.staticDetailInstanceCount = 0;
    if (!detail)
        return;
    auto frame = detail->GetCompletedRayVisibilityFrame();
    if (detail->IsRayTracingCoverageEnabled() && (!frame || !frame->source || !frame->statsReady))
    {
        scene.failed = true;
        return;
    }
    if (!frame || !frame->source || frame->source->chunks.empty())
        return;
    const auto& source = *frame->source;
    const auto& stats = frame->stats;
    scene.billboard = !source.params.grassMode;
    const u32 lodCounts[] = { stats.visibleLOD0Count, stats.visibleLOD1Count, stats.visibleLOD2Count };
    const bool procedural = source.params.grassMode && (lodCounts[0] || lodCounts[1] || lodCounts[2]);
    const u32 maximum = source.maxPulledIndexCount / 3 * 3;
    const u32 wavedCount = scene.billboard ? stats.visibleBillboardCount : 0;
    const u32 staticCount = stats.visibleDecalCount;
    const bool waved = wavedCount != 0;
    const bool staticDetail = staticCount != 0;
    const bool pulled = waved || staticDetail;
    if (!procedural && !pulled)
        return;
    scene.grassWind = detail->perlin4dTexture;
    for (u32 i = 0; i < 2; ++i)
        scene.grassInteraction[i] = detail->interactionTexture[i];
    if (pulled && maximum == 0)
    {
        scene.failed = true;
        return;
    }
    if (pulled && !HasPulledDetailTextures(detail))
    {
        scene.failed = true;
        return;
    }
    if (procedural)
    {
        scene.grassPipeline = s_grassPipeline;
        scene.grassLayout = s_grassLayout;
        if (!scene.grassPipeline || !scene.grassLayout)
        {
            scene.failed = true;
            return;
        }
    }
    if (pulled)
    {
        scene.pulledPipeline = s_billboardPipeline;
        scene.pulledLayout = s_billboardLayout;
        if (!scene.pulledPipeline || !scene.pulledLayout || !source.bindingLayout || !source.descriptorTable ||
            !source.models || !source.pulledVertices)
        {
            scene.failed = true;
            return;
        }
        scene.detailAtlasIndex = detail->buildDetailsBindlessIndex;
        scene.detailPbrIndex = NormalizeDetailTextureIndex(detail->buildDetailsPbrBindlessIndex);
        scene.detailBumpIndex = NormalizeDetailTextureIndex(detail->buildDetailsBumpBindlessIndex);
    }
    if (waved && !frame->visible[FGDetailManager::VIS_KIND_MESH])
    {
        scene.failed = true;
        return;
    }
    if (staticDetail && !frame->visible[FGDetailManager::VIS_KIND_DECAL])
    {
        scene.failed = true;
        return;
    }
    u64 proceduralVertices = 0;
    u64 proceduralIndices = 0;
    if (procedural)
    {
        for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; ++lod)
        {
            if (!lodCounts[lod])
                continue;
            const u32 vertsPerBlade = FGDetailManager::LOD_SEGMENTS[lod] * 2 + 1;
            const u32 indicesPerBlade = (FGDetailManager::LOD_SEGMENTS[lod] - 1) * 6 + 3;
            proceduralVertices += u64(lodCounts[lod]) * vertsPerBlade;
            proceduralIndices += u64(lodCounts[lod]) * indicesPerBlade;
        }
    }
    const u64 streamPerInstance = maximum;
    const u64 wavedVertices = streamPerInstance * wavedCount;
    const u64 wavedIndices = wavedVertices;
    const u64 staticVertices = streamPerInstance * staticCount;
    const u64 staticIndices = staticVertices;
    const u64 vertices = proceduralVertices + wavedVertices + staticVertices;
    const u64 indices = proceduralIndices + wavedIndices + staticIndices;
    const auto memory = m_device->GetBackend()->GetMemoryBudget();
    const u64 rawRange = u64(UINT32_MAX) + 1;
    const u64 range = memory.bufferRangeBytes ? std::min(rawRange, memory.bufferRangeBytes) : rawRange;
    if (!vertices || !indices || vertices > range / 24 || indices > range / sizeof(u32))
    {
        scene.failed = true;
        return;
    }
    if (procedural)
    {
        u64 vertexOffset = 0;
        u64 indexOffset = 0;
        for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; ++lod)
        {
            if (!lodCounts[lod])
                continue;
            RTGrassJob job = {};
            job.visible = frame->visible[lod];
            job.lod = lod;
            job.constants.segments = FGDetailManager::LOD_SEGMENTS[lod];
            job.constants.vertsPerBlade = job.constants.segments * 2 + 1;
            job.constants.indicesPerBlade = (job.constants.segments - 1) * 6 + 3;
            job.constants.bladeCount = lodCounts[lod];
            job.constants.outputVertexOffset = u32(vertexOffset);
            job.constants.outputIndexOffset = u32(indexOffset);
            vertexOffset += u64(lodCounts[lod]) * job.constants.vertsPerBlade;
            indexOffset += u64(lodCounts[lod]) * job.constants.indicesPerBlade;
            scene.grassJobs.push_back(std::move(job));
        }
        R_ASSERT(vertexOffset == proceduralVertices && indexOffset == proceduralIndices);
    }
    if (waved)
    {
        RTPulledJob job;
        job.visible = frame->visible[FGDetailManager::VIS_KIND_MESH];
        job.constants = {};
        job.constants.maxVertsPerBillboard = maximum;
        job.constants.billboardCount = wavedCount;
        job.constants.outputVertexOffset = u32(proceduralVertices);
        job.constants.outputIndexOffset = u32(proceduralIndices);
        job.constants.detailKind = FGDetailManager::VIS_KIND_MESH;
        scene.detailMeshJobs.push_back(std::move(job));
    }
    if (staticDetail)
    {
        RTPulledJob job;
        job.visible = frame->visible[FGDetailManager::VIS_KIND_DECAL];
        job.constants = {};
        job.constants.maxVertsPerBillboard = maximum;
        job.constants.billboardCount = staticCount;
        job.constants.outputVertexOffset = u32(proceduralVertices + wavedVertices);
        job.constants.outputIndexOffset = u32(proceduralIndices + wavedIndices);
        job.constants.detailKind = FGDetailManager::VIS_KIND_DECAL;
        scene.staticDetailJobs.push_back(std::move(job));
        scene.staticDetailInstanceCount = staticCount;
    }
    scene.grassFrame = std::move(frame);
    scene.grassVertexCount = u32(vertices);
    scene.grassIndexCount = u32(indices);
    auto* device = m_device->GetNVRHIDevice();
    if (!EnsureRTBuffer(device, scene.grassVertices, RTVertexBufferDesc("RT_GrassVertices", vertices * 24)) ||
        !EnsureRTBuffer(device, scene.grassIndices, RTVertexBufferDesc("RT_GrassIndices", indices * sizeof(u32))))
    {
        scene.failed = true;
        return;
    }
    nvrhi::rt::AccelStructDesc grassDesc;
    grassDesc.debugName = "RT_GrassBLAS";
    grassDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastBuild;
    const RTBatchTransform identityTransform = RTBatchTransformOf(Fidentity);
    const u32 grassBatch = u32(scene.batches.size());
    auto appendRange = [&](u64 indexStart, u64 indexCount, u64 vertexEnd, bool opaque)
    {
        grassDesc.addBottomLevelGeometry(RTTriangles(scene.grassVertices, scene.grassIndices,
            24, 0, u32(vertexEnd), u32(indexStart), u32(indexCount), opaque));
        scene.batches.push_back({ 0, u32(indexStart), 0, u32(indexCount) });
        scene.batchTransforms.push_back(identityTransform);
        scene.batchIdentities.push_back(0);
        scene.emissiveBatchOffsets.push_back(UINT32_MAX);
    };
    if (procedural)
        appendRange(0, proceduralIndices, proceduralVertices, true);
    if (waved)
    {
        scene.detailMeshBatchStart = u32(scene.batches.size());
        appendRange(proceduralIndices, wavedIndices, proceduralVertices + wavedVertices, false);
    }
    if (staticDetail)
    {
        scene.staticDetailBatchStart = u32(scene.batches.size());
        appendRange(proceduralIndices + wavedIndices, staticIndices,
            proceduralVertices + wavedVertices + staticVertices, false);
    }
    AcquireGeometryBuild(grassDesc, retainedGrassBuild, false);
    if (!retainedGrassBuild.handle)
    {
        scene.failed = true;
        return;
    }
    scene.grassBuild = std::move(retainedGrassBuild);
    scene.instances.push_back(RTInstance(scene.grassBuild.handle, grassBatch,
        identityTransform, !pulled, RT_MASK_WORLD, pulled));
    R_ASSERT(scene.emissiveBatchOffsets.size() == scene.batches.size());
    scene.counts.grass = u32(scene.batches.size()) - grassBatch;
    R_ASSERT(scene.detailMeshBatchStart == UINT32_MAX || scene.detailMeshBatchStart < scene.batches.size());
    R_ASSERT(scene.staticDetailBatchStart == UINT32_MAX || scene.staticDetailBatchStart < scene.batches.size());
}

void RTAccelStructManager::PrepareScene(GPUCullingManager* gpu, FGDetailManager* detail,
    const xr_vector<GeometryBatch>& world, const xr_vector<GeometryBatch>& hud)
{
    std::shared_ptr<RTSceneGeneration> next;
    for (const auto& candidate : m_generations)
    {
        if (candidate->leases == 0 && candidate->retention == 0)
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
    if (scene.failed)
    {
        scene.skinBuild.built = false;
        scene.hudSkinBuild.built = false;
        scene.grassBuild.built = false;
        scene.tlasBuilt = false;
    }
    scene.geometry = m_staticGeometry;
    scene.dynamicGeometry.reset();
    scene.skinJobs.clear();
    scene.hudSkinJobs.clear();
    scene.skinSources.clear();
    scene.grassJobs.clear();
    scene.detailMeshJobs.clear();
    scene.staticDetailJobs.clear();
    scene.pulledPipeline = nullptr;
    scene.pulledLayout = nullptr;
    scene.skinTopology.reset();
    scene.skinnedIndices = nullptr;
    scene.batches = m_staticGeometry->batches;
    scene.instances = m_staticGeometry->instances;
    scene.counts = m_staticGeometry->counts;
    scene.detailAtlasIndex = 0;
    scene.detailMeshBatchStart = UINT32_MAX;
    scene.staticDetailBatchStart = UINT32_MAX;
    scene.detailPbrIndex = 0;
    scene.detailBumpIndex = 0;
    scene.staticDetailInstanceCount = 0;
    scene.grassVertexCount = 0;
    scene.grassIndexCount = 0;
    scene.recorded = false;
    scene.failed = false;
    scene.emissiveTriangles.clear();
    scene.emissiveBatchOffsets.clear();
    scene.emissiveCount = 0;
    const xr_vector<GPUInstanceData>* worlds[RT_SOURCE_COUNT] = { &gpu->GetStaticInstanceData(),
        &gpu->GetTerrainInstanceData(), &gpu->GetTransparentInstanceData() };
    const xr_vector<GeometryInstanceKey>* identities[RT_SOURCE_COUNT] = { &gpu->GetStaticInstanceIdentities(),
        &gpu->GetTerrainInstanceIdentities(), &gpu->GetTransparentInstanceIdentities() };
    const auto& geometry = *m_staticGeometry;
    R_ASSERT(geometry.batchSources.size() == scene.batches.size());
    scene.batchTransforms.resize(scene.batches.size());
    scene.batchIdentities.resize(scene.batches.size());
    for (u32 i = 0; i < scene.batches.size(); ++i)
    {
        const RTBatchSource& source = geometry.batchSources[i];
        R_ASSERT(source.array < RT_SOURCE_COUNT);
        R_ASSERT(source.index < worlds[source.array]->size());
        R_ASSERT(source.index < identities[source.array]->size());
        scene.batchTransforms[i] = RTBatchTransformOf((*worlds[source.array])[source.index].world);
        scene.batchIdentities[i] = RTInstanceIdentity((*identities[source.array])[source.index]);
    }
    for (u32 i = 0; i < scene.instances.size(); ++i)
    {
        const s32 batch = geometry.instanceBatches[i];
        if (batch < 0)
            continue;
        R_ASSERT(u32(batch) < scene.batchTransforms.size());
        nvrhi::rt::AffineTransform affine;
        memcpy(&affine, &scene.batchTransforms[batch].rows, sizeof(affine));
        scene.instances[i].setTransform(affine);
    }
    const u32 staticTerrainEnd = scene.counts.identityStatic + scene.counts.terrain;
    for (u32 i = 0; i < scene.batches.size(); ++i)
    {
        const auto& batch = scene.batches[i];
        if (i >= scene.counts.identityStatic && i < staticTerrainEnd)
        {
            scene.emissiveBatchOffsets.push_back(UINT32_MAX);
            continue;
        }
        AppendEmissiveBatch(scene.emissiveBatchOffsets, scene.emissiveTriangles, i, batch.materialID,
            batch.indexCount, scene.batchIdentities[i]);
    }
    if (gpu->GetRTVertexBuffer() && gpu->GetRTIndexBuffer())
    {
        if (!EnsureDynamicGeometry(gpu))
        {
            scene.failed = true;
            m_scene.reset();
            return;
        }
        scene.dynamicGeometry = m_dynamicGeometry;
        PrepareDynamic(scene, gpu);
    }
    PrepareSkin(scene, gpu, world, hud);
    PrepareGrass(scene, detail);
    if (scene.failed || scene.instances.empty())
    {
        m_scene.reset();
        return;
    }
    auto* device = m_device->GetNVRHIDevice();
    const u32 tlasInstances = u32(scene.instances.size());
    if (!scene.tlas || scene.tlasCapacity < tlasInstances)
    {
        nvrhi::rt::AccelStructDesc tlas;
        tlas.debugName = "RT_SceneTLAS";
        tlas.isTopLevel = true;
        tlas.topLevelMaxInstances = tlasInstances;
        tlas.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;
        if (m_inPlaceUpdates)
            tlas.buildFlags = tlas.buildFlags | nvrhi::rt::AccelStructBuildFlags::AllowUpdate;
        scene.tlas = device->createAccelStruct(tlas);
        scene.tlasCapacity = tlasInstances;
        scene.tlasBuildCount = 0;
        scene.tlasBuilt = false;
    }
    if (!scene.tlas)
    {
        scene.failed = true;
        m_scene.reset();
        return;
    }
    scene.tlasUpdate = m_inPlaceUpdates && scene.tlasBuilt && scene.tlasBuildCount == tlasInstances;
    nvrhi::BufferDesc batches;
    batches.debugName = "RT_BatchInfo";
    batches.byteSize = scene.batches.size() * sizeof(RTBatchInfo);
    batches.structStride = sizeof(RTBatchInfo);
    batches.initialState = nvrhi::ResourceStates::ShaderResource;
    batches.keepInitialState = true;
    R_ASSERT(scene.batchTransforms.size() == scene.batches.size());
    R_ASSERT(scene.batchIdentities.size() == scene.batches.size());
    R_ASSERT(scene.emissiveBatchOffsets.size() == scene.batches.size());
    scene.emissiveCount = u32(scene.emissiveTriangles.size());
    bool emissiveReallocated = false;
    bool transformsReallocated = false;
    bool offsetsReallocated = false;
    if (!EnsureRTBuffer(device, scene.batchInfo, batches) ||
        !EnsureRTBuffer(device, scene.emissiveTriangleBuffer, RTTableBufferDesc("RT_EmissiveTriangles",
            sizeof(RTEmissiveTriangle), scene.emissiveTriangles.size()), &emissiveReallocated) ||
        !EnsureRTBuffer(device, scene.batchTransformBuffer, RTTableBufferDesc("RT_BatchTransforms",
            sizeof(RTBatchTransform), scene.batchTransforms.size()), &transformsReallocated) ||
        !EnsureRTBuffer(device, scene.emissiveBatchOffsetBuffer, RTTableBufferDesc("RT_EmissiveBatchOffsets",
            sizeof(u32), scene.emissiveBatchOffsets.size()), &offsetsReallocated) ||
        !EnsureRTBuffer(device, scene.grassMaterials, RTTableBufferDesc("RT_GrassMaterials",
            sizeof(Fvector4), sizeof(FGDetailManager::GrassMaterialConstants) / sizeof(Fvector4))))
    {
        scene.failed = true;
        m_scene.reset();
        return;
    }
    if (emissiveReallocated)
        scene.tableClearMask |= RT_TABLE_EMISSIVE;
    if (transformsReallocated)
        scene.tableClearMask |= RT_TABLE_TRANSFORMS;
    if (offsetsReallocated)
        scene.tableClearMask |= RT_TABLE_OFFSETS;
    scene.sourceMaterials = bindless::MaterialBuffer::Instance().GetBuffer();
    scene.sourceTerrainMaterials = bindless::TerrainMaterialBuffer::Instance().GetBuffer();
    scene.sourceVariants = bindless::VariantBuffer::Instance().GetBuffer();
    if (!scene.sourceMaterials || !scene.sourceTerrainMaterials || !scene.sourceVariants)
    {
        scene.failed = true;
        m_scene.reset();
        return;
    }
    auto materialDesc = scene.sourceMaterials->getDesc();
    materialDesc.debugName = "RT_MaterialSnapshot";
    bool snapshotReady = EnsureRTBuffer(device, scene.materials, materialDesc);
    materialDesc = scene.sourceTerrainMaterials->getDesc();
    materialDesc.debugName = "RT_TerrainMaterialSnapshot";
    snapshotReady &= EnsureRTBuffer(device, scene.terrainMaterials, materialDesc);
    auto variantDesc = scene.sourceVariants->getDesc();
    variantDesc.debugName = "RT_VariantSnapshot";
    snapshotReady &= EnsureRTBuffer(device, scene.variants, variantDesc);
    if (!snapshotReady)
    {
        scene.failed = true;
        m_scene.reset();
        return;
    }
    m_textureScratch.clear();
    const u32 skinEnd = u32(scene.geometry->batches.size()) + scene.counts.dynamic + scene.counts.skinned;
    const u32 terrainEnd = scene.counts.identityStatic + scene.counts.terrain;
    for (u32 i = 0; i < skinEnd; ++i)
        AppendMaterialTextures(scene.batches[i].materialID, i >= scene.counts.identityStatic && i < terrainEnd);
    if (scene.counts.grass && (!scene.detailMeshJobs.empty() || !scene.staticDetailJobs.empty()))
    {
        for (u32 index : { scene.detailAtlasIndex, scene.detailPbrIndex, scene.detailBumpIndex })
        {
            if (index != 0 && index != bindless::INVALID_TEXTURE_INDEX)
                m_textureScratch.push_back(index);
        }
    }
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
    resources.grassMaterials = ImportRTBuffer(graph, "RT_GrassMaterials", scene.grassMaterials);
    resources.emissiveTriangles = ImportRTBuffer(graph, "RT_EmissiveTriangles", scene.emissiveTriangleBuffer);
    resources.batchTransforms = ImportRTBuffer(graph, "RT_BatchTransforms", scene.batchTransformBuffer);
    resources.emissiveBatchOffsets = ImportRTBuffer(graph, "RT_EmissiveBatchOffsets", scene.emissiveBatchOffsetBuffer);
    resources.emissiveCount = scene.emissiveCount;
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

void RTAccelStructManager::RefreshGeometryBuild(RTGeometryBuild& slot) const
{
    slot.update = slot.handle && slot.built && m_inPlaceUpdates &&
        (slot.desc.buildFlags & nvrhi::rt::AccelStructBuildFlags::AllowUpdate) != nvrhi::rt::AccelStructBuildFlags::None;
}

bool RTAccelStructManager::RefreshPose(GPUCullingManager* gpu, FGDetailManager* detail,
    const xr_vector<GeometryBatch>& world, const xr_vector<GeometryBatch>& hud)
{
    auto& scene = *m_scene;
    if (scene.geometry != m_staticGeometry || scene.failed || !scene.recorded)
        return false;
    if (scene.counts.skinned && (!s_skinPipeline || !s_skinLayout))
        return false;
    if (!scene.grassJobs.empty() && (!s_grassPipeline || scene.grassPipeline != s_grassPipeline ||
        scene.grassLayout != s_grassLayout))
        return false;
    if ((!scene.detailMeshJobs.empty() || !scene.staticDetailJobs.empty()) &&
        (!s_billboardPipeline || scene.pulledPipeline != s_billboardPipeline || scene.pulledLayout != s_billboardLayout))
        return false;
    const auto& geometry = *m_staticGeometry;
    const xr_vector<GPUInstanceData>* worlds[RT_SOURCE_COUNT] = { &gpu->GetStaticInstanceData(),
        &gpu->GetTerrainInstanceData(), &gpu->GetTransparentInstanceData() };
    const u32 staticBatchCount = u32(geometry.batches.size());
    if (geometry.batchSources.size() != staticBatchCount || scene.batchTransforms.size() < staticBatchCount ||
        scene.instances.size() < geometry.instances.size())
        return false;
    for (u32 i = 0; i < staticBatchCount; ++i)
    {
        const RTBatchSource& source = geometry.batchSources[i];
        if (source.array >= RT_SOURCE_COUNT || source.index >= worlds[source.array]->size())
            return false;
        scene.batchTransforms[i] = RTBatchTransformOf((*worlds[source.array])[source.index].world);
    }
    auto applyTransform = [&](u32 instance, u32 batch)
    {
        nvrhi::rt::AffineTransform affine;
        memcpy(&affine, &scene.batchTransforms[batch].rows, sizeof(affine));
        scene.instances[instance].setTransform(affine);
    };
    for (u32 i = 0; i < geometry.instances.size(); ++i)
    {
        const s32 batch = geometry.instanceBatches[i];
        if (batch >= 0)
            applyTransform(i, u32(batch));
    }
    u32 batchIndex = staticBatchCount;
    u32 instanceIndex = u32(geometry.instances.size());
    if (scene.counts.dynamic)
    {
        if (!scene.dynamicGeometry || scene.dynamicGeometry != m_dynamicGeometry)
            return false;
        const auto& instances = gpu->GetDynamicInstanceData();
        const auto& keys = gpu->GetDynamicMeshKeys();
        if (instances.size() != keys.size())
            return false;
        const u32 dynamicEnd = staticBatchCount + scene.counts.dynamic;
        if (scene.batchTransforms.size() < dynamicEnd || scene.instances.size() < instanceIndex + scene.counts.dynamic)
            return false;
        for (u32 i = 0; i < keys.size(); ++i)
        {
            RTDynamicRange range = {};
            range.vertexOffset = keys[i].vertexOffset;
            range.indexOffset = keys[i].indexOffset;
            range.vertexCount = keys[i].vertexCount;
            range.indexCount = keys[i].indexCount;
            range.opaque = IsOpaqueMaterialForRT(instances[i].materialID, false);
            if (scene.dynamicGeometry->recordBuilds.find(range) == scene.dynamicGeometry->recordBuilds.end())
                continue;
            if (batchIndex >= dynamicEnd)
                return false;
            scene.batchTransforms[batchIndex] = RTBatchTransformOf(instances[i].world);
            applyTransform(instanceIndex, batchIndex);
            ++batchIndex;
            ++instanceIndex;
        }
        if (batchIndex != dynamicEnd)
            return false;
    }
    if (scene.counts.skinned)
    {
        if (scene.skinJobs.size() != world.size() || scene.hudSkinJobs.size() != hud.size())
            return false;
        auto refreshJobs = [&](xr_vector<RTSkinJob>& jobs, const xr_vector<GeometryBatch>& batches)
        {
            for (u32 i = 0; i < jobs.size(); ++i)
            {
                const GeometryBatch& batch = batches[i];
                CKinematics* skeleton = RTBatchSkeleton(batch);
                if (!skeleton)
                    return false;
                RTSkinningCB& constants = jobs[i].constants;
                constants.worldMatrix = batch.worldMatrix;
                Fmatrix inverse;
                inverse.invert(batch.worldMatrix);
                constants.normalMatrix.transpose(inverse);
                constants.boneOffset = gpu->GetPreparedSkeletonOffset(skeleton);
            }
            return true;
        };
        if (!refreshJobs(scene.skinJobs, world) || !refreshJobs(scene.hudSkinJobs, hud))
            return false;
        scene.bones = gpu->GetGlobalBoneBuffer();
        if (!scene.bones || !scene.skinTopology || scene.skinTopology->uploadPending)
            return false;
        RefreshGeometryBuild(scene.skinBuild);
        RefreshGeometryBuild(scene.hudSkinBuild);
    }
    if (scene.counts.grass)
    {
        if (!detail)
            return false;
        auto frame = detail->GetCompletedRayVisibilityFrame();
        if (!frame || !frame->source || !frame->statsReady || !scene.grassFrame || !scene.grassFrame->source ||
            frame->source->id != scene.grassFrame->source->id)
            return false;
        const auto& stats = frame->stats;
        const u32 lodCounts[] = { stats.visibleLOD0Count, stats.visibleLOD1Count, stats.visibleLOD2Count };
        for (auto& job : scene.grassJobs)
        {
            if (job.lod >= FGDetailManager::LOD_COUNT || lodCounts[job.lod] != job.constants.bladeCount ||
                !frame->visible[job.lod])
                return false;
            job.visible = frame->visible[job.lod];
        }
        auto refreshPulled = [&](xr_vector<RTPulledJob>& jobs, u32 kind, u32 count)
        {
            for (auto& job : jobs)
            {
                if (job.constants.detailKind != kind || job.constants.billboardCount != count || !frame->visible[kind])
                    return false;
                job.visible = frame->visible[kind];
            }
            return true;
        };
        if (!refreshPulled(scene.detailMeshJobs, FGDetailManager::VIS_KIND_MESH, stats.visibleBillboardCount) ||
            !refreshPulled(scene.staticDetailJobs, FGDetailManager::VIS_KIND_DECAL, stats.visibleDecalCount))
            return false;
        scene.grassFrame = std::move(frame);
        scene.grassWind = detail->perlin4dTexture;
        for (u32 i = 0; i < 2; ++i)
            scene.grassInteraction[i] = detail->interactionTexture[i];
        if (!scene.grassWind || !scene.grassInteraction[0] || !scene.grassInteraction[1])
            return false;
        scene.grassBuild.update = false;
    }
    scene.tlasUpdate = m_inPlaceUpdates && scene.tlasBuilt && scene.tlasBuildCount == u32(scene.instances.size());
    return true;
}

bool RTAccelStructManager::SetupBuildPass(framegraph::FrameGraph& graph, GPUCullingManager* gpu, FGDetailManager* detail, const xr_vector<GeometryBatch>& world,
    const xr_vector<GeometryBatch>& hud)
{
    if (!m_rtSupported || !gpu || !GEnv.Backend || !GEnv.Backend->SupportsSubmissionLeases())
        return false;
    RetireScenes();
    if ((gpu->GetRTVertexBuffer() || gpu->GetRTIndexBuffer()) && !gpu->IsRTSourceReady())
        return false;
    PrepareStatic(gpu);
    if (!m_staticGeometry || m_staticGeometry->failed)
        return false;
    const u64 topology = ComputeTopologySignature(gpu, detail, world, hud);
    const RTPoseSignature pose = ComputePoseSignature(gpu, detail, world, hud);
    RTBuildScope scope = RTBuildScope::None;
    bool rebuild = !m_scene || topology != m_sceneSignature || m_scene->geometry != m_staticGeometry ||
        !m_scene->recorded;
    if (!rebuild && pose.refresh != m_poseSignature)
    {
        if (m_scene->retention == 0 && RefreshPose(gpu, detail, world, hud))
        {
            scope = RTBuildScope::Pose;
            m_poseSignature = pose.refresh;
            if (pose.motion != m_motionSignature)
            {
                m_motionSignature = pose.motion;
                ++m_poseRevision;
            }
        }
        else
            rebuild = true;
    }
    if (rebuild)
    {
        if (!EnsureBuildResources(detail, !world.empty() || !hud.empty()))
            return false;
        PrepareScene(gpu, detail, world, hud);
        if (IsReady())
        {
            scope = RTBuildScope::Full;
            m_sceneSignature = topology;
            m_poseSignature = pose.refresh;
            m_motionSignature = pose.motion;
            ++m_sceneRevision;
            ++m_poseRevision;
        }
    }
    if (!IsReady())
        return false;
    const u64 lease = GEnv.Backend->OpenSubmissionLease();
    if (!lease)
        return false;
    ++m_scene->leases;
    m_leases.push_back({ m_scene, lease });
    if (scope == RTBuildScope::None)
    {
        R_ASSERT(m_scene->recorded);
        return true;
    }
    RegisterBuildPasses(graph, detail, scope);
    return true;
}

void RTAccelStructManager::RegisterBuildPasses(framegraph::FrameGraph& graph, FGDetailManager* detail, RTBuildScope scope)
{
    using namespace framegraph;
    const bool full = scope == RTBuildScope::Full;
    const auto scene = m_scene;
    const auto resources = ImportScene(graph, *scene);
    graph.addCallbackPass<RTBuildPassData>("RT Source and Pose",
        [&](FrameGraph& builder, PassHandle pass, RTBuildPassData& data)
        {
            RenderPassBuilder pb(builder, pass);
            data.manager = this;
            data.detailManager = detail;
            data.scene = scene;
            data.resources = resources;
            data.scope = scope;
            if (full)
            {
                pb.write(resources.batchInfo, ResourceState::CopyDest);
                pb.write(resources.materials, ResourceState::CopyDest);
                pb.write(resources.terrainMaterials, ResourceState::CopyDest);
                pb.write(resources.variants, ResourceState::CopyDest);
                pb.write(resources.emissiveTriangles, ResourceState::CopyDest);
                pb.write(resources.emissiveBatchOffsets, ResourceState::CopyDest);
                data.sourceMaterials = pb.read(ImportRTBuffer(builder, "bindless_materials",
                    scene->sourceMaterials), ResourceState::CopySource);
                data.sourceTerrainMaterials = pb.read(ImportRTBuffer(builder, "terrain_materials",
                    scene->sourceTerrainMaterials), ResourceState::CopySource);
                data.sourceVariants = pb.read(ImportRTBuffer(builder, "bindless_variants",
                    scene->sourceVariants), ResourceState::CopySource);
            }
            pb.write(resources.grassMaterials, ResourceState::CopyDest);
            pb.write(resources.batchTransforms, ResourceState::CopyDest);
            if (scene->counts.skinned)
            {
                pb.write(resources.skinnedVertices, ResourceState::UnorderedAccess);
                if (full || scene->skinTopology->uploadPending)
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
                for (const auto& job : scene->detailMeshJobs)
                    data.detailMeshVisible.push_back(pb.read(ImportRTBuffer(builder, "RT_DetailMeshVisible", job.visible),
                        ResourceState::ShaderResource));
                for (const auto& job : scene->staticDetailJobs)
                    data.staticDetailVisible.push_back(pb.read(ImportRTBuffer(builder, "RT_StaticDetailVisible", job.visible),
                        ResourceState::ShaderResource));
                if (!scene->detailMeshJobs.empty() || !scene->staticDetailJobs.empty())
                {
                    R_ASSERT(frame.source->models && frame.source->pulledVertices);
                    data.detailModels = pb.read(ImportRTBuffer(builder, "RT_DetailModels", frame.source->models),
                        ResourceState::ShaderResource);
                    data.detailPulledVertices = pb.read(ImportRTBuffer(builder, "RT_DetailPulledVertices",
                        frame.source->pulledVertices), ResourceState::ShaderResource);
                }
                const auto readDeformationTexture = [&](nvrhi::ITexture* texture)
                {
                    const auto& native = texture->getDesc();
                    ResourceDesc desc;
                    desc.type = native.dimension == nvrhi::TextureDimension::Texture3D ?
                        ResourceDesc::Type::Texture3D : ResourceDesc::Type::Texture2D;
                    desc.width = native.width;
                    desc.height = native.height;
                    desc.depth = native.depth;
                    desc.mipLevels = native.mipLevels;
                    desc.arraySize = native.arraySize;
                    desc.format = native.format;
                    desc.isUAV = native.isUAV;
                    desc.isTransient = false;
                    desc.debugName = native.debugName.c_str();
                    return pb.read(builder.ImportTexture(native.debugName.c_str(), texture, desc),
                        ResourceState::ShaderResource);
                };
                data.wind = readDeformationTexture(scene->grassWind);
                for (u32 i = 0; i < 2; ++i)
                    data.interaction[i] = readDeformationTexture(scene->grassInteraction[i]);
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
            data.scope = scope;
            auto input = [&](VirtualResourceHandle handle)
            {
                if (handle.is_valid())
                    data.buffers.push_back(pb.read(handle, ResourceState::AccelStructBuildInput));
            };
            auto output = [&](const RTGeometryBuild& build)
            {
                if (build.handle)
                {
                    const auto handle = builder.ImportAccelerationStructure(build.desc.debugName.c_str(), build.handle);
                    data.structures.push_back(build.update ? pb.readWrite(handle, ResourceState::AccelStructWrite) :
                        pb.write(handle, ResourceState::AccelStructWrite));
                }
            };
            bool sourceDeclared = false;
            if (!scene->geometry->recorded)
            {
                input(resources.vertices);
                input(resources.indices);
                sourceDeclared = true;
                for (const auto& build : scene->geometry->builds)
                    output(build);
            }
            if (scene->dynamicGeometry && !scene->dynamicGeometry->recorded)
            {
                if (!sourceDeclared)
                {
                    input(resources.vertices);
                    input(resources.indices);
                }
                for (const auto& build : scene->dynamicGeometry->builds)
                    output(build);
            }
            if (scene->counts.skinned)
            {
                input(resources.skinnedVertices);
                input(resources.skinnedIndices);
                output(scene->skinBuild);
                output(scene->hudSkinBuild);
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
            data.scope = scope;
            if (scene->tlasUpdate)
                pb.readWrite(resources.tlas, ResourceState::AccelStructWrite);
            else
                pb.write(resources.tlas, ResourceState::AccelStructWrite);
            auto input = [&](const RTGeometryBuild& build)
            {
                if (build.handle)
                    data.structures.push_back(pb.read(builder.ImportAccelerationStructure(
                        build.desc.debugName.c_str(), build.handle), ResourceState::AccelStructBuildBlas));
            };
            for (const auto& build : scene->geometry->builds)
                input(build);
            if (scene->dynamicGeometry)
            {
                for (const auto& build : scene->dynamicGeometry->builds)
                    input(build);
            }
            input(scene->skinBuild);
            input(scene->hudSkinBuild);
            input(scene->grassBuild);
        },
        [](const RTBuildPassData& data, const FrameGraph& fg, RenderContext* ctx)
        {
            data.manager->RecordTLAS(data, fg, ctx->GetCommandList());
        });
}

void RTAccelStructManager::RecordInputs(const RTBuildPassData& data,
    const framegraph::FrameGraph& graph, nvrhi::ICommandList* commandList)
{
    auto& scene = *data.scene;
    const bool full = data.scope == RTBuildScope::Full;
    auto buffer = [&](framegraph::VirtualResourceHandle handle)
    {
        return graph.GetPhysicalBuffer(handle);
    };
    auto upload = [&](framegraph::VirtualResourceHandle handle, const void* contents, size_t bytes, u32 clearBit)
    {
        auto* destination = buffer(handle);
        if (scene.tableClearMask & clearBit)
        {
            static const u8 zeros[65536] = {};
            const u64 capacity = destination->getDesc().byteSize;
            for (u64 offset = 0; offset < capacity;)
            {
                const u64 remaining = capacity - offset;
                const u64 chunk = remaining < sizeof(zeros) ? remaining : sizeof(zeros);
                commandList->writeBuffer(destination, zeros, size_t(chunk), offset);
                offset += chunk;
            }
            scene.tableClearMask &= ~clearBit;
        }
        if (bytes)
            commandList->writeBuffer(destination, contents, bytes);
    };
    if (full)
    {
        commandList->writeBuffer(buffer(data.resources.batchInfo), scene.batches.data(),
            scene.batches.size() * sizeof(RTBatchInfo));
        upload(data.resources.emissiveTriangles, scene.emissiveTriangles.data(),
            scene.emissiveTriangles.size() * sizeof(RTEmissiveTriangle), RT_TABLE_EMISSIVE);
        upload(data.resources.emissiveBatchOffsets, scene.emissiveBatchOffsets.data(),
            scene.emissiveBatchOffsets.size() * sizeof(u32), RT_TABLE_OFFSETS);
        auto* source = buffer(data.sourceMaterials);
        commandList->copyBuffer(buffer(data.resources.materials), 0, source, 0, source->getDesc().byteSize);
        source = buffer(data.sourceTerrainMaterials);
        commandList->copyBuffer(buffer(data.resources.terrainMaterials), 0, source, 0, source->getDesc().byteSize);
        source = buffer(data.sourceVariants);
        commandList->copyBuffer(buffer(data.resources.variants), 0, source, 0, source->getDesc().byteSize);
    }
    upload(data.resources.batchTransforms, scene.batchTransforms.data(),
        scene.batchTransforms.size() * sizeof(RTBatchTransform), RT_TABLE_TRANSFORMS);
    FGDetailManager::GrassMaterialConstants grassMaterialConstants = {};
    if (data.detailManager)
        data.detailManager->FillGrassMaterialConstants(grassMaterialConstants);
    commandList->writeBuffer(buffer(data.resources.grassMaterials), &grassMaterialConstants,
        sizeof(grassMaterialConstants));
    auto* device = m_device->GetNVRHIDevice();
    if (scene.counts.skinned)
    {
        auto* output = buffer(data.resources.skinnedVertices);
        R_ASSERT(scene.skinTopology);
        if (scene.skinTopology->uploadPending)
        {
            commandList->writeBuffer(buffer(data.resources.skinnedIndices),
                scene.skinTopology->staging.data(), scene.skinTopology->staging.size() * sizeof(u32));
            scene.skinTopology->staging.clear();
            scene.skinTopology->staging.shrink_to_fit();
            scene.skinTopology->uploadPending = false;
        }
        const auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("rt_skin_vertices", ".cs");
        R_ASSERT(reflection);
        auto* bones = buffer(data.bones);
        for (u32 list = 0; list < 2; ++list)
        {
            const xr_vector<RTSkinJob>& jobs = list == 0 ? scene.skinJobs : scene.hudSkinJobs;
            for (const auto& job : jobs)
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
    }
    if (!scene.counts.grass)
        return;
    auto* vertices = buffer(data.resources.grassVertices);
    auto* indices = buffer(data.resources.grassIndices);
    const auto& grassSource = *scene.grassFrame->source;
    for (auto handle : data.grassSources)
        commandList->setBufferState(buffer(handle), nvrhi::ResourceStates::NonPixelShaderResource);
    R_ASSERT(data.detailManager);
    FGDetailManager::DetailFrameConstants frameConstants = {};
    data.detailManager->FillFrameConstants(frameConstants);
    auto* interaction = graph.GetPhysicalTexture(data.interaction[data.detailManager->interactionCurrent]);
    if (!scene.grassJobs.empty())
    {
        const auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("rt_grass_vertices", ".cs");
        R_ASSERT(reflection);
        for (u32 i = 0; i < scene.grassJobs.size(); ++i)
        {
            const auto& job = scene.grassJobs[i];
            GrassRTCB constants = job.constants;
            constants.wind_direction = frameConstants.g_wind_direction;
            constants.wave = frameConstants.wave;
            constants.grass_wind_displacement = frameConstants.grass_wind_displacement;
            constants.grass_blade_height = frameConstants.grass_blade_height;
            constants.grass_blade_width = frameConstants.grass_blade_width;
            constants.interaction_window = frameConstants.interaction_window;
            constants.grass_interaction_displacement = frameConstants.grass_interaction_displacement;
            constants.grass_interaction_max_angle = frameConstants.grass_interaction_max_angle;
            framegraph::BindingSetBuilder bindings(*reflection, device, "RT.Grass");
            bindings.BufferSRV("g_VisibleIndices", buffer(data.grassVisible[i]))
                .Texture("g_WindTexture", graph.GetPhysicalTexture(data.wind))
                .Texture("g_Interaction", interaction)
                .BufferUAV("g_Output", vertices).BufferUAV("g_OutputIB", indices)
                .ConstantBuffer("GrassRTCB", m_device->GetNativeBuffer(s_grassCB));
            auto set = device->createBindingSet(bindings.Build(), scene.grassLayout);
            R_ASSERT(set);
            commandList->writeBuffer(m_device->GetNativeBuffer(s_grassCB), &constants, sizeof(constants));
            nvrhi::ComputeState state;
            state.pipeline = scene.grassPipeline;
            state.bindings = { set, m_device->GetBackend()->GetBindlessDescriptorTable(), grassSource.descriptorTable };
            commandList->setComputeState(state);
            const u32 jobVertices = job.constants.bladeCount * job.constants.vertsPerBlade;
            const u32 groups = (jobVertices + 255u) / 256u;
            commandList->dispatch(std::min(groups, 1024u), (groups + 1023u) / 1024u, 1);
        }
    }
    if (scene.detailMeshJobs.empty() && scene.staticDetailJobs.empty())
        return;
    {
        const auto* reflection = GEnv.Render->GetShaderLoader()->GetCachedReflection("rt_grass_billboard", ".cs");
        R_ASSERT(reflection);
        auto dispatch = [&](const xr_vector<RTPulledJob>& jobs,
            const xr_vector<framegraph::VirtualResourceHandle>& visible)
        {
            for (u32 i = 0; i < jobs.size(); ++i)
            {
                const auto& job = jobs[i];
                BillboardRTCB constants = job.constants;
                constants.wind_direction = frameConstants.g_wind_direction;
                constants.wave = frameConstants.wave;
                constants.interaction_window = frameConstants.interaction_window;
                constants.grass_wind_displacement = frameConstants.grass_wind_displacement;
                constants.grass_interaction_displacement = frameConstants.grass_interaction_displacement;
                constants.grass_interaction_max_angle = frameConstants.grass_interaction_max_angle;
                framegraph::BindingSetBuilder bindings(*reflection, device, "RT.Pulled");
                bindings.BufferSRV("g_VisibleIndices", buffer(visible[i]))
                    .BufferSRV("g_DetailModels", buffer(data.detailModels))
                    .BufferSRV("g_PulledVerts", buffer(data.detailPulledVertices))
                    .Texture("g_WindTexture", graph.GetPhysicalTexture(data.wind))
                    .Texture("g_Interaction", interaction)
                    .BufferUAV("g_Output", vertices).BufferUAV("g_OutputIB", indices)
                    .ConstantBuffer("BillboardRTCB", m_device->GetNativeBuffer(s_billboardCB));
                auto set = device->createBindingSet(bindings.Build(), scene.pulledLayout);
                R_ASSERT(set);
                commandList->writeBuffer(m_device->GetNativeBuffer(s_billboardCB), &constants, sizeof(constants));
                nvrhi::ComputeState state;
                state.pipeline = scene.pulledPipeline;
                state.bindings = { set, m_device->GetBackend()->GetBindlessDescriptorTable(), grassSource.descriptorTable };
                commandList->setComputeState(state);
                const u32 groups = (job.constants.billboardCount + 255u) / 256u;
                commandList->dispatch(std::min(groups, 1024u), (groups + 1023u) / 1024u, 1);
            }
        };
        dispatch(scene.detailMeshJobs, data.detailMeshVisible);
        dispatch(scene.staticDetailJobs, data.staticDetailVisible);
    }
}

void RTAccelStructManager::RecordBLAS(const RTBuildPassData& data,
    const framegraph::FrameGraph& graph, nvrhi::ICommandList* commandList)
{
    for (const auto handle : data.buffers)
        R_ASSERT(graph.GetPhysicalBuffer(handle));
    u32 index = 0;
    auto build = [&](RTGeometryBuild& item)
    {
        if (!item.handle)
            return;
        auto* destination = graph.GetPhysicalAccelerationStructure(data.structures[index++]);
        nvrhi::rt::AccelStructDesc desc = item.desc;
        if (item.update)
            desc.buildFlags = desc.buildFlags | nvrhi::rt::AccelStructBuildFlags::PerformUpdate;
        nvrhi::utils::BuildBottomLevelAccelStruct(commandList, destination, desc);
        item.built = true;
        item.update = false;
    };
    auto& scene = *data.scene;
    if (!scene.geometry->recorded)
    {
        for (auto& item : scene.geometry->builds)
            build(item);
        scene.geometry->recorded = true;
    }
    if (scene.dynamicGeometry && !scene.dynamicGeometry->recorded)
    {
        for (auto& item : scene.dynamicGeometry->builds)
            build(item);
        scene.dynamicGeometry->recorded = true;
    }
    if (scene.counts.skinned)
    {
        build(scene.skinBuild);
        build(scene.hudSkinBuild);
    }
    if (scene.counts.grass)
        build(scene.grassBuild);
    R_ASSERT(index == data.structures.size());
}

static constexpr u32 RT_TLAS_MAX_REFITS = 32;

nvrhi::rt::AccelStructBuildFlags RTAccelStructManager::StaticBuildFlags() const
{
    nvrhi::rt::AccelStructBuildFlags flags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;
    if (m_compaction)
        flags = flags | nvrhi::rt::AccelStructBuildFlags::AllowCompaction;
    return flags;
}

bool RTAccelStructManager::AnySceneRetained() const
{
    for (const auto& generation : m_generations)
    {
        if (generation->retention != 0)
            return true;
    }
    return false;
}

bool RTAccelStructManager::RecordCompaction(RTSceneGeneration& scene, nvrhi::ICommandList* commandList)
{
    if (!m_compaction || AnySceneRetained())
        return false;
    commandList->compactBottomLevelAccelStructs();
    bool swapped = false;
    auto observe = [&](xr_vector<RTGeometryBuild>& builds)
    {
        for (auto& build : builds)
        {
            if (build.handle && build.built && !build.compacted && build.handle->isCompacted())
            {
                build.compacted = true;
                swapped = true;
            }
        }
    };
    if (scene.geometry)
        observe(scene.geometry->builds);
    if (scene.dynamicGeometry)
        observe(scene.dynamicGeometry->builds);
    return swapped;
}

void RTAccelStructManager::RecordTLAS(const RTBuildPassData& data,
    const framegraph::FrameGraph& graph, nvrhi::ICommandList* commandList)
{
    for (const auto handle : data.structures)
        R_ASSERT(graph.GetPhysicalAccelerationStructure(handle));
    auto* tlas = graph.GetPhysicalAccelerationStructure(data.resources.tlas);
    auto& scene = *data.scene;
    if (RecordCompaction(scene, commandList) || scene.tlasRefits >= RT_TLAS_MAX_REFITS)
        scene.tlasUpdate = false;
    nvrhi::rt::AccelStructBuildFlags flags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;
    if (m_inPlaceUpdates)
        flags = flags | nvrhi::rt::AccelStructBuildFlags::AllowUpdate;
    if (scene.tlasUpdate)
        flags = flags | nvrhi::rt::AccelStructBuildFlags::PerformUpdate;
    commandList->buildTopLevelAccelStruct(tlas, scene.instances.data(), u32(scene.instances.size()), flags);
    scene.tlasRefits = scene.tlasUpdate ? scene.tlasRefits + 1 : 0;
    scene.tlasBuildCount = u32(scene.instances.size());
    scene.tlasBuilt = true;
    scene.tlasUpdate = false;
    scene.recorded = true;
}

static void DeclareSceneReads(framegraph::RenderPassBuilder& builder, const RTFrameResources& resources)
{
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
    input(resources.grassMaterials);
    input(resources.emissiveTriangles);
    input(resources.batchTransforms);
    input(resources.emissiveBatchOffsets);
    input(resources.skinnedVertices);
    input(resources.skinnedIndices);
    input(resources.grassVertices);
    input(resources.grassIndices);
}

RTFrameResources RTAccelStructManager::UseScene(framegraph::FrameGraph& graph,
    framegraph::RenderPassBuilder& builder) const
{
    R_ASSERT(IsReady());
    return UseScene(graph, builder, m_scene);
}

RTFrameResources RTAccelStructManager::UseScene(framegraph::FrameGraph& graph,
    framegraph::RenderPassBuilder& builder, const std::shared_ptr<RTSceneGeneration>& scene) const
{
    R_ASSERT(scene);
    auto resources = ImportScene(graph, *scene);
    DeclareSceneReads(builder, resources);
    return resources;
}

std::shared_ptr<RTSceneGeneration> RTAccelStructManager::GetScene() const
{
    return m_scene;
}

void RTAccelStructManager::RetainScene(const std::shared_ptr<RTSceneGeneration>& scene)
{
    if (scene)
        ++scene->retention;
}

void RTAccelStructManager::ReleaseScene(const std::shared_ptr<RTSceneGeneration>& scene)
{
    if (scene && scene->retention)
        --scene->retention;
}

bool RTAccelStructManager::IsSceneValid(const RTSceneGeneration& scene) const
{
    return scene.recorded && !scene.failed && IsSceneReady(scene);
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
    result.grassMaterials = buffer(resources.grassMaterials);
    result.skinnedVertices = buffer(resources.skinnedVertices);
    result.skinnedIndices = buffer(resources.skinnedIndices);
    result.grassVertices = buffer(resources.grassVertices);
    result.grassIndices = buffer(resources.grassIndices);
    result.emissiveTriangles = buffer(resources.emissiveTriangles);
    result.batchTransforms = buffer(resources.batchTransforms);
    result.emissiveBatchOffsets = buffer(resources.emissiveBatchOffsets);
    result.emissiveCount = resources.emissiveCount;
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
    auto compactable = [&](const RTGeometryBuild& build)
    {
        acceleration(build.handle);
        if (!build.handle)
            return;
        if ((build.desc.buildFlags & nvrhi::rt::AccelStructBuildFlags::AllowCompaction) != nvrhi::rt::AccelStructBuildFlags::None)
            ++result.compactableStructures;
        if (build.compacted)
            ++result.compactedStructures;
    };
    xr_set<RTSkinTopology*> countedTopologies;
    for (u32 i = 0; i < m_generations.size(); ++i)
    {
        const auto& scene = *m_generations[i];
        result.generationBytes += bufferBytes(scene.batchInfo) + bufferBytes(scene.materials) +
            bufferBytes(scene.terrainMaterials) + bufferBytes(scene.variants) +
            bufferBytes(scene.grassMaterials) + bufferBytes(scene.skinnedVertices) +
            bufferBytes(scene.grassVertices) + bufferBytes(scene.grassIndices) +
            bufferBytes(scene.emissiveTriangleBuffer) + bufferBytes(scene.batchTransformBuffer) +
            bufferBytes(scene.emissiveBatchOffsetBuffer);
        if (scene.skinTopology && countedTopologies.insert(scene.skinTopology.get()).second)
            result.generationBytes += bufferBytes(scene.skinTopology->indices);
        acceleration(scene.tlas);
        acceleration(scene.skinBuild.handle);
        acceleration(scene.hudSkinBuild.handle);
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
                compactable(build);
        }
        bool firstDynamic = scene.dynamicGeometry != nullptr;
        for (u32 j = 0; j < i; ++j)
            firstDynamic &= m_generations[j]->dynamicGeometry != scene.dynamicGeometry;
        if (firstDynamic)
        {
            for (const auto& build : scene.dynamicGeometry->builds)
                compactable(build);
        }
    }
    for (const auto& topology : m_skinTopologies)
    {
        if (countedTopologies.insert(topology.second.get()).second)
            result.generationBytes += bufferBytes(topology.second->indices);
    }
    return result;
}
}
