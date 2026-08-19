#include "stdafx.h"
#include "RTAccelStructManager.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FBasicVisual.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/ShaderVariant/VariantPSOCache.h"
#include "Layers/xrRender/FGDetailManager.h"
#include "Layers/xrRender/FrameGraphPasses/ParticlePassSetup.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include <nvrhi/utils.h>

extern ENGINE_API float ps_r3_grass_blade_width;
extern ENGINE_API float ps_r3_grass_blade_height;
extern ENGINE_API float ps_r3_grass_wind_displacement;
extern ENGINE_API float ps_r_rt_detail_dist;

namespace xray::render::fg {
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg {

extern int ps_r__detail_gpu;

static bool MatrixEquals(const Fmatrix& a, const Fmatrix& b)
{
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            if (a.m[r][c] != b.m[r][c])
                return false;
        }
    }
    return true;
}

nvrhi::ComputePipelineHandle RTAccelStructManager::s_skinPipeline;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_skinLayout;
fg::BufferHandle RTAccelStructManager::s_skinCB;
bool RTAccelStructManager::s_skinInitialized = false;

nvrhi::ComputePipelineHandle RTAccelStructManager::s_grassPipeline;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_grassLayout;
fg::BufferHandle RTAccelStructManager::s_grassCB;
nvrhi::SamplerHandle RTAccelStructManager::s_grassSampler;
bool RTAccelStructManager::s_grassInitialized = false;

nvrhi::ComputePipelineHandle RTAccelStructManager::s_billboardPipeline;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_billboardLayout;
fg::BufferHandle RTAccelStructManager::s_billboardCB;
nvrhi::BufferHandle s_billboardCount;
bool RTAccelStructManager::s_billboardInitialized = false;

struct RTSkinningCB {
    Fmatrix worldMatrix;
    u32 vertexCount;
    u32 vertexStride;
    u32 formatID;
    u32 boneOffset;
    u32 outputOffset;
    u32 inputBaseVertex;
    u32 pad[2];
};
static_assert(sizeof(RTSkinningCB) == 96, "RTSkinningCB must be 96 bytes");

struct GrassRTCB {
    Fvector4 detail_params;
    Fvector4 wind_direction;
    Fvector4 wave;
    float grass_wind_displacement;
    float grass_blade_height;
    float grass_blade_width;
    u32 segments;
    u32 vertsPerBlade;
    u32 bladeCount;
    u32 outputVertexOffset;
    u32 indicesPerBlade;
    u32 outputIndexOffset;
    u32 pad[3];
};
static_assert(sizeof(GrassRTCB) == 96, "GrassRTCB must be 96 bytes");

struct BillboardRTCB {
    u32 maxVertsPerBillboard;
    u32 maxBillboards;
    float windAngleDeg;
    float windSpeed;
    float time;
    float windDisplacement;
    u32 pad[2];
    float cameraPos[3];
    float maxDistanceSq;
};
static_assert(sizeof(BillboardRTCB) == 48, "BillboardRTCB must be 48 bytes");

static void FmatrixToRTTransform(const Fmatrix& m, nvrhi::rt::AffineTransform& out)
{
    out[0]  = m._11; out[1]  = m._21; out[2]  = m._31; out[3]  = m._41;
    out[4]  = m._12; out[5]  = m._22; out[6]  = m._32; out[7]  = m._42;
    out[8]  = m._13; out[9]  = m._23; out[10] = m._33; out[11] = m._43;
}

void RTAccelStructManager::Initialize(fg::RenderDevice* device)
{
    m_device = device;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    m_rtSupported = nvDevice->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct) &&
                    nvDevice->queryFeatureSupport(nvrhi::Feature::RayQuery);

    if (m_rtSupported)
        Msg("* [RT] Ray tracing supported (AccelStruct + RayQuery)");
    else
        Msg("* [RT] Ray tracing NOT supported on this device");
}

nvrhi::rt::IAccelStruct* RTAccelStructManager::GetOrCreateEmptyTLAS(nvrhi::ICommandList* cmd)
{
    if (m_emptyTlas)
        return m_emptyTlas;
    if (!m_rtSupported || !m_device)
        return nullptr;
    nvrhi::IDevice* nv = m_device->GetNVRHIDevice();
    if (!nv)
        return nullptr;
    nvrhi::rt::AccelStructDesc desc;
    desc.debugName = "EmptyTLAS";
    desc.isTopLevel = true;
    desc.topLevelMaxInstances = 1;
    desc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;
    m_emptyTlas = nv->createAccelStruct(desc);
    if (m_emptyTlas && cmd)
        cmd->buildTopLevelAccelStruct(m_emptyTlas, nullptr, 0);
    return m_emptyTlas;
}

void RTAccelStructManager::Shutdown()
{
    m_staticBlas = nullptr;
    m_uniqueGeometries.clear();
    m_tlas = nullptr;
    m_emptyTlas = nullptr;
    for (u32 i = 0; i < 3; ++i) {
        m_tlasSlots[i] = nullptr;
        m_tlasMaxInstances[i] = 0;
        m_batchInfoSlots[i] = nullptr;
        m_skinnedSlots[i] = {};
        m_grassSlots[i] = {};
    }
    m_tlasSlot = 0;
    m_batchInfoSlot = 0;
    m_skinnedSlot = 0;
    m_grassSlot = 0;
    m_batchInfoBuffer = nullptr;
    m_skinnedBlas = nullptr;
    m_skinnedBatchData.clear();
    m_skinnedTotalVerts = 0;
    m_skinnedTotalIndices = 0;
    m_skinnedTopoHash = 0;
    m_skinnedReady = false;
    m_grassOutputVB = nullptr;
    m_grassIB = nullptr;
    m_grassBlas = nullptr;
    m_grassTotalVerts = 0;
    m_grassTotalIndices = 0;
    m_grassReady = false;
    m_particleOutputVB = nullptr;
    m_particleIB = nullptr;
    m_particleBlas = nullptr;
    m_particleTotalVerts = 0;
    m_particleTotalIndices = 0;
    m_particleReady = false;
    m_isReady = false;
    m_batchCount = 0;
    m_batchCounts = {};

    s_skinPipeline = nullptr;
    s_skinLayout = nullptr;
    s_skinCB = fg::BufferHandle();
    s_skinInitialized = false;

    s_grassPipeline = nullptr;
    s_grassLayout = nullptr;
    s_grassCB = fg::BufferHandle();
    s_grassSampler = nullptr;
    s_grassInitialized = false;

    s_billboardPipeline = nullptr;
    s_billboardLayout = nullptr;
    s_billboardCB = fg::BufferHandle();
    s_billboardCount = nullptr;
    s_billboardInitialized = false;
}

void RTAccelStructManager::InvalidateShaderPipelines()
{
    s_skinPipeline = nullptr;
    s_skinLayout = nullptr;
    s_skinCB = fg::BufferHandle();
    s_skinInitialized = false;

    s_grassPipeline = nullptr;
    s_grassLayout = nullptr;
    s_grassCB = fg::BufferHandle();
    s_grassSampler = nullptr;
    s_grassInitialized = false;

    s_billboardPipeline = nullptr;
    s_billboardLayout = nullptr;
    s_billboardCB = fg::BufferHandle();
    s_billboardCount = nullptr;
    s_billboardInitialized = false;

    Msg("* [RTAccel] Shader pipelines invalidated for hot-reload");
}

void RTAccelStructManager::Invalidate()
{
    m_staticBlas = nullptr;
    m_uniqueGeometries.clear();
    m_tlas = nullptr;
    for (u32 i = 0; i < 3; ++i) {
        m_tlasSlots[i] = nullptr;
        m_tlasMaxInstances[i] = 0;
        m_batchInfoSlots[i] = nullptr;
        m_skinnedSlots[i] = {};
        m_grassSlots[i] = {};
    }
    m_tlasSlot = 0;
    m_batchInfoSlot = 0;
    m_skinnedSlot = 0;
    m_grassSlot = 0;
    m_batchInfoBuffer = nullptr;
    m_megaVB = nullptr;
    m_megaIB = nullptr;
    m_skinnedBlas = nullptr;
    m_skinnedBatchData.clear();
    m_skinnedTotalVerts = 0;
    m_skinnedTotalIndices = 0;
    m_skinnedTopoHash = 0;
    m_skinnedReady = false;
    m_grassOutputVB = nullptr;
    m_grassIB = nullptr;
    m_grassBlas = nullptr;
    m_grassTotalVerts = 0;
    m_grassTotalIndices = 0;
    m_grassReady = false;
    m_particleOutputVB = nullptr;
    m_particleIB = nullptr;
    m_particleBlas = nullptr;
    m_particleTotalVerts = 0;
    m_particleTotalIndices = 0;
    m_particleReady = false;
    m_isReady = false;
    m_batchCount = 0;
    m_batchCounts = {};
    Msg("* [RT] Acceleration structures invalidated");
}

void RTAccelStructManager::BuildIfNeeded(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    if (m_isReady || !m_rtSupported || !gpuCulling)
        return;

    if (!gpuCulling->IsMegaDataUploaded()) {
        static bool s_logged = false;
        if (!s_logged) {
            s_logged = true;
            Msg("* [RT] BuildIfNeeded waiting: mega data not uploaded yet");
        }
        return;
    }

    const auto& drawArgs = gpuCulling->GetStaticDrawArgsData();
    if (drawArgs.empty()) {
        static bool s_logged = false;
        if (!s_logged) {
            s_logged = true;
            Msg("* [RT] BuildIfNeeded waiting: static draw args empty");
        }
        return;
    }

    m_megaVB = gpuCulling->GetMegaVertexBuffer();
    m_megaIB = gpuCulling->GetMegaIndexBuffer();
    if (!m_megaVB || !m_megaIB) {
        Msg("! [RT] BuildIfNeeded: mega VB/IB null");
        return;
    }

    cmdList->setBufferState(m_megaVB, nvrhi::ResourceStates::AccelStructBuildInput);
    cmdList->setBufferState(m_megaIB, nvrhi::ResourceStates::AccelStructBuildInput);
    cmdList->commitBarriers();

    BuildStaticBLAS(cmdList, gpuCulling);
    BuildInstancedBLAS(cmdList, gpuCulling);
    BuildTLAS(cmdList);
    if (!m_tlas) {
        m_staticBlas = nullptr;
        m_uniqueGeometries.clear();
        Msg("! [RT] BuildIfNeeded: TLAS create/build failed");
        return;
    }

    CreateBatchInfoBuffer(cmdList, gpuCulling);

    m_isReady = true;

    u32 totalInstances = 0;
    for (const auto& ug : m_uniqueGeometries)
        totalInstances += (u32)ug.instances.size();

    Msg("* [RT] Acceleration structures ready (identity: %u static + %u terrain + %u transparent, instanced: %u unique BLAS x %u instances, TLAS: %u instances)",
        m_batchCounts.identityStatic, m_batchCounts.terrain, m_batchCounts.transparent,
        (u32)m_uniqueGeometries.size(), totalInstances,
        (m_staticBlas ? 1u : 0u) + totalInstances);
}

static u32 AddBatchesToBLAS(
    nvrhi::rt::AccelStructDesc& blasDesc,
    const xr_vector<IndirectDrawArgs>& drawArgs,
    nvrhi::IBuffer* megaVB, nvrhi::IBuffer* megaIB,
    u32 totalVertexCount, u32 vertexStride,
    const xr_vector<u32>* vertexCounts = nullptr,
    const xr_vector<bool>* skipMask = nullptr,
    const xr_vector<GPUInstanceData>* instances = nullptr,
    bool forceNonOpaque = false,
    const xr_vector<u32>* materialIDs = nullptr,
    xr_vector<RTBatchInfo>* outInfos = nullptr)
{
    u32 count = 0;
    for (u32 i = 0; i < (u32)drawArgs.size(); i++) {
        if (skipMask && i < skipMask->size() && (*skipMask)[i])
            continue;

        const auto& args = drawArgs[i];
        if (args.indexCountPerInstance < 3)
            continue;

        u32 baseVert = static_cast<u32>(args.baseVertexLocation);
        u32 batchVertexCount = (vertexCounts && i < vertexCounts->size()) ? (*vertexCounts)[i] : (totalVertexCount - baseVert);
        if (batchVertexCount == 0 || baseVert >= totalVertexCount || batchVertexCount > (totalVertexCount - baseVert))
            continue;

        nvrhi::rt::GeometryTriangles tri;
        tri.setIndexBuffer(megaIB)
           .setIndexFormat(nvrhi::Format::R32_UINT)
           .setIndexOffset(static_cast<uint64_t>(args.startIndexLocation) * sizeof(u32))
           .setIndexCount(args.indexCountPerInstance)
           .setVertexBuffer(megaVB)
           .setVertexFormat(nvrhi::Format::RGB32_FLOAT)
           .setVertexStride(vertexStride)
           .setVertexOffset(static_cast<uint64_t>(baseVert) * vertexStride)
           .setVertexCount(batchVertexCount);

        if (instances && i < instances->size()
            && ((*instances)[i].flags & GPU_OBJECT_WMARK) != 0)
            continue;

        bool nonOpaque = forceNonOpaque;
        if (!nonOpaque && instances && i < instances->size()) {
            const u32 flags = (*instances)[i].flags;
            nonOpaque = (flags & (GPU_OBJECT_ALPHA_TEST | GPU_OBJECT_TRANSPARENT)) != 0;
        }

        nvrhi::rt::GeometryDesc geom;
        geom.setTriangles(tri)
            .setFlags(nonOpaque ? nvrhi::rt::GeometryFlags::None : nvrhi::rt::GeometryFlags::Opaque);

        blasDesc.addBottomLevelGeometry(geom);
        if (outInfos) {
            RTBatchInfo info;
            info.materialID = (materialIDs && i < materialIDs->size()) ? (*materialIDs)[i] : 0;
            info.startIndex = args.startIndexLocation;
            info.baseVertex = args.baseVertexLocation;
            info.indexCount = args.indexCountPerInstance;
            outInfos->push_back(info);
        }
        count++;
    }
    return count;
}

void RTAccelStructManager::BuildStaticBLAS(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    constexpr u32 vertexStride = sizeof(bindless::UnifiedVertex);
    const u32 totalVerts = gpuCulling->GetTotalVertexCount();
    const auto& staticVertCounts = gpuCulling->GetStaticBatchVertexCounts();
    const auto& staticDrawArgs = gpuCulling->GetStaticDrawArgsData();
    const auto& staticInstances = gpuCulling->GetStaticInstanceData();

    xr_vector<bool> isTransformed(staticDrawArgs.size(), false);
    for (u32 i = 0; i < (u32)staticDrawArgs.size(); i++) {
        if (i < staticInstances.size() && !MatrixEquals(staticInstances[i].world, Fidentity))
            isTransformed[i] = true;
    }

    nvrhi::rt::AccelStructDesc blasDesc;
    blasDesc.debugName = "StaticSceneBLAS";
    blasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;

    m_staticGeomInfos.clear();
    m_batchCounts.identityStatic = AddBatchesToBLAS(blasDesc, staticDrawArgs,
        m_megaVB, m_megaIB, totalVerts, vertexStride, &staticVertCounts, &isTransformed,
        &staticInstances, false, &gpuCulling->GetStaticMaterialIDData(), &m_staticGeomInfos);
    m_batchCounts.terrain = AddBatchesToBLAS(blasDesc, gpuCulling->GetTerrainDrawArgsData(),
        m_megaVB, m_megaIB, totalVerts, vertexStride, nullptr, nullptr, nullptr, false,
        &gpuCulling->GetTerrainMaterialIDData(), &m_staticGeomInfos);
    m_batchCounts.transparent = AddBatchesToBLAS(blasDesc, gpuCulling->GetTransparentDrawArgsData(),
        m_megaVB, m_megaIB, totalVerts, vertexStride, nullptr, nullptr,
        &gpuCulling->GetTransparentInstanceData(), true,
        &gpuCulling->GetTransparentMaterialIDData(), &m_staticGeomInfos);

    m_staticHasAlpha = m_batchCounts.transparent > 0;
    for (u32 i = 0; i < (u32)staticInstances.size(); i++) {
        if (i < isTransformed.size() && isTransformed[i])
            continue;
        if ((staticInstances[i].flags & (GPU_OBJECT_ALPHA_TEST | GPU_OBJECT_TRANSPARENT)) != 0) {
            m_staticHasAlpha = true;
            break;
        }
    }

    u32 staticTotal = m_batchCounts.identityStatic + m_batchCounts.terrain + m_batchCounts.transparent;
    if (staticTotal == 0) return;

    m_staticBlas = nvDevice->createAccelStruct(blasDesc);
    if (!m_staticBlas) return;

    nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, m_staticBlas, blasDesc);
}

void RTAccelStructManager::BuildInstancedBLAS(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    constexpr u32 vertexStride = sizeof(bindless::UnifiedVertex);
    const u32 totalVerts = gpuCulling->GetTotalVertexCount();
    const auto& staticDrawArgs = gpuCulling->GetStaticDrawArgsData();
    const auto& staticInstances = gpuCulling->GetStaticInstanceData();
    const auto& staticMaterialIDs = gpuCulling->GetStaticMaterialIDData();
    const auto& staticVertCounts = gpuCulling->GetStaticBatchVertexCounts();

    m_uniqueGeometries.clear();
    xr_map<GeometryKey, u32> keyToIndex;

    for (u32 i = 0; i < (u32)staticDrawArgs.size(); i++) {
        if (i >= staticInstances.size()) continue;
        if ((staticInstances[i].flags & GPU_OBJECT_WMARK) != 0) continue;
        if (MatrixEquals(staticInstances[i].world, Fidentity)) continue;

        const auto& args = staticDrawArgs[i];
        if (args.indexCountPerInstance == 0) continue;

        GeometryKey key = { args.startIndexLocation, args.baseVertexLocation, args.indexCountPerInstance };
        InstanceInfo inst;
        inst.world = staticInstances[i].world;
        inst.materialID = (i < staticMaterialIDs.size()) ? staticMaterialIDs[i] : 0;
        inst.flags = staticInstances[i].flags;

        auto it = keyToIndex.find(key);
        if (it != keyToIndex.end()) {
            m_uniqueGeometries[it->second].instances.push_back(inst);
            if ((inst.flags & (GPU_OBJECT_ALPHA_TEST | GPU_OBJECT_TRANSPARENT)) != 0)
                m_uniqueGeometries[it->second].alphaGeometry = true;
        } else {
            u32 baseVert = static_cast<u32>(args.baseVertexLocation);
            u32 batchVertexCount = (i < staticVertCounts.size()) ? staticVertCounts[i] : (totalVerts - baseVert);
            if (batchVertexCount == 0 || args.indexCountPerInstance < 3)
                continue;
            if (baseVert >= totalVerts || batchVertexCount > (totalVerts - baseVert))
                continue;

            UniqueGeometry ug;
            ug.key = key;
            ug.vertexCount = batchVertexCount;
            ug.alphaGeometry = (inst.flags & (GPU_OBJECT_ALPHA_TEST | GPU_OBJECT_TRANSPARENT)) != 0;
            ug.instances.push_back(inst);
            keyToIndex[key] = (u32)m_uniqueGeometries.size();
            m_uniqueGeometries.push_back(std::move(ug));
        }
    }

    for (auto& ug : m_uniqueGeometries) {
        nvrhi::rt::AccelStructDesc blasDesc;
        blasDesc.debugName = "InstancedBLAS";
        blasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;

        nvrhi::rt::GeometryTriangles tri;
        tri.setIndexBuffer(m_megaIB)
           .setIndexFormat(nvrhi::Format::R32_UINT)
           .setIndexOffset(static_cast<uint64_t>(ug.key.startIndex) * sizeof(u32))
           .setIndexCount(ug.key.indexCount)
           .setVertexBuffer(m_megaVB)
           .setVertexFormat(nvrhi::Format::RGB32_FLOAT)
           .setVertexStride(vertexStride)
           .setVertexOffset(static_cast<uint64_t>(static_cast<u32>(ug.key.baseVertex)) * vertexStride)
           .setVertexCount(ug.vertexCount);

        nvrhi::rt::GeometryDesc geom;
        geom.setTriangles(tri).setFlags(ug.alphaGeometry ? nvrhi::rt::GeometryFlags::None : nvrhi::rt::GeometryFlags::Opaque);
        blasDesc.addBottomLevelGeometry(geom);

        ug.blas = nvDevice->createAccelStruct(blasDesc);
        if (!ug.blas) continue;

        nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, ug.blas, blasDesc);
    }

    u32 totalInstances = 0;
    for (const auto& ug : m_uniqueGeometries)
        totalInstances += (u32)ug.instances.size();
    m_batchCounts.instancedTotal = totalInstances;

    if (!m_uniqueGeometries.empty())
        Msg("* [RT] Built %u unique instanced BLAS (%u total instances)", (u32)m_uniqueGeometries.size(), totalInstances);
}

void RTAccelStructManager::BuildTLAS(nvrhi::ICommandList* cmdList)
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    u32 totalInstances = 0;
    for (const auto& ug : m_uniqueGeometries)
        totalInstances += (u32)ug.instances.size();

    u32 skinnedCount = m_skinnedReady ? (u32)m_skinnedBatchData.size() : 0;
    u32 grassCount = m_grassReady ? 1 : 0;
    u32 particleCount = m_particleReady ? 1 : 0;
    u32 instanceCount = (m_staticBlas ? 1 : 0) + totalInstances + (m_skinnedBlas ? 1 : 0)
        + (m_grassBlas ? 1 : 0) + (m_particleBlas ? 1 : 0);
    if (instanceCount == 0) {
        Msg("! [RT] No BLAS to build TLAS from");
        return;
    }

    m_tlasSlot = (m_tlasSlot + 1u) % 3u;
    auto& tlasSlot = m_tlasSlots[m_tlasSlot];
    u32& maxInst = m_tlasMaxInstances[m_tlasSlot];
    u32 neededMax = instanceCount + 64u;
    if (neededMax < 4096u)
        neededMax = 4096u;
    if (!tlasSlot || maxInst < instanceCount) {
        maxInst = neededMax;
        nvrhi::rt::AccelStructDesc tlasDesc;
        tlasDesc.debugName = "SceneTLAS";
        tlasDesc.isTopLevel = true;
        tlasDesc.topLevelMaxInstances = maxInst;
        tlasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastBuild;
        tlasSlot = nvDevice->createAccelStruct(tlasDesc);
        if (!tlasSlot) {
            Msg("! [RT] Failed to create TLAS");
            m_tlas = nullptr;
            return;
        }
    }

    xr_vector<nvrhi::rt::InstanceDesc> instances;
    instances.reserve(instanceCount);

    u32 staticGeomCount = m_batchCounts.identityStatic + m_batchCounts.terrain + m_batchCounts.transparent;

    if (m_staticBlas) {
        nvrhi::rt::InstanceDesc inst;
        inst.setTransform(nvrhi::rt::c_IdentityTransform)
            .setInstanceID(0)
            .setInstanceMask(kRTMaskScene)
            .setFlags(nvrhi::rt::InstanceFlags::TriangleCullDisable)
            .setBLAS(m_staticBlas);
        instances.push_back(inst);
    }

    u32 instancedBatchOffset = staticGeomCount;
    for (const auto& ug : m_uniqueGeometries) {
        if (!ug.blas) {
            instancedBatchOffset += (u32)ug.instances.size();
            continue;
        }
        for (const auto& instInfo : ug.instances) {
            nvrhi::rt::InstanceDesc inst;
            nvrhi::rt::AffineTransform xform;
            FmatrixToRTTransform(instInfo.world, xform);
            inst.setTransform(xform)
                .setInstanceID(instancedBatchOffset)
                .setInstanceMask(kRTMaskScene)
                .setFlags(nvrhi::rt::InstanceFlags::TriangleCullDisable)
                .setBLAS(ug.blas);
            instances.push_back(inst);
            instancedBatchOffset++;
        }
    }

    u32 skinnedBatchOffset = instancedBatchOffset;
    if (m_skinnedBlas && skinnedCount > 0) {
        nvrhi::rt::InstanceDesc inst;
        inst.setTransform(nvrhi::rt::c_IdentityTransform)
            .setInstanceID(skinnedBatchOffset)
            .setInstanceMask(kRTMaskScene)
            .setFlags(nvrhi::rt::InstanceFlags::ForceOpaque)
            .setBLAS(m_skinnedBlas);
        instances.push_back(inst);
    }

    u32 grassBatchOffset = skinnedBatchOffset + skinnedCount;
    if (m_grassBlas && grassCount > 0) {
        nvrhi::rt::InstanceDesc inst;
        inst.setTransform(nvrhi::rt::c_IdentityTransform)
            .setInstanceID(grassBatchOffset)
            .setInstanceMask(kRTMaskGrass)
            .setFlags(nvrhi::rt::InstanceFlags::TriangleCullDisable)
            .setBLAS(m_grassBlas);
        instances.push_back(inst);
    }

    u32 particleBatchOffset = grassBatchOffset + grassCount;
    if (m_particleBlas && particleCount > 0) {
        nvrhi::rt::InstanceDesc inst;
        inst.setTransform(nvrhi::rt::c_IdentityTransform)
            .setInstanceID(particleBatchOffset)
            .setInstanceMask(kRTMaskParticles)
            .setFlags(nvrhi::rt::InstanceFlags::TriangleCullDisable)
            .setBLAS(m_particleBlas);
        instances.push_back(inst);
    }

    m_batchCount = particleBatchOffset + particleCount;
    cmdList->buildTopLevelAccelStruct(tlasSlot, instances.data(), (u32)instances.size());
    m_tlas = tlasSlot;
}

void RTAccelStructManager::CreateBatchInfoBuffer(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    (void)gpuCulling;
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    xr_vector<RTBatchInfo> batchInfos;
    batchInfos.reserve(m_batchCount);

    batchInfos.insert(batchInfos.end(), m_staticGeomInfos.begin(), m_staticGeomInfos.end());

    for (const auto& ug : m_uniqueGeometries) {
        for (const auto& instInfo : ug.instances) {
            RTBatchInfo info;
            info.materialID = instInfo.materialID;
            info.startIndex = ug.key.startIndex;
            info.baseVertex = ug.key.baseVertex;
            info.indexCount = ug.key.indexCount;
            batchInfos.push_back(info);
        }
    }

    for (const auto& sb : m_skinnedBatchData) {
        RTBatchInfo info;
        info.materialID = sb.materialID;
        info.startIndex = sb.indexOffset;
        info.baseVertex = static_cast<s32>(sb.vertexOffset);
        info.indexCount = sb.indexCount;
        batchInfos.push_back(info);
    }

    if (m_grassReady && m_grassTotalIndices > 0) {
        RTBatchInfo info;
        info.materialID = 0;
        info.startIndex = 0;
        info.baseVertex = 0;
        info.indexCount = m_grassTotalIndices;
        batchInfos.push_back(info);
    }

    if (m_particleReady && m_particleTotalIndices > 0) {
        RTBatchInfo info;
        info.materialID = 0;
        info.startIndex = 0;
        info.baseVertex = 0;
        info.indexCount = m_particleTotalIndices;
        batchInfos.push_back(info);
    }

    if (batchInfos.empty()) return;

    m_batchInfoSlot = (m_batchInfoSlot + 1u) % 3u;
    auto& slot = m_batchInfoSlots[m_batchInfoSlot];
    const u64 needed = batchInfos.size() * sizeof(RTBatchInfo);
    if (!slot || slot->getDesc().byteSize < needed) {
        nvrhi::BufferDesc desc;
        desc.debugName = "RTBatchInfoBuffer";
        desc.byteSize = needed;
        desc.structStride = sizeof(RTBatchInfo);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        slot = nvDevice->createBuffer(desc);
        if (!slot) {
            Msg("! [RT] BatchInfo buffer create failed");
            m_batchInfoBuffer = nullptr;
            return;
        }
    }

    cmdList->writeBuffer(slot, batchInfos.data(), needed);
    m_batchInfoBuffer = slot;
}

u32 RTAccelStructManager::GetSkinningFormatID(u16 renderMode, u32 stride)
{
    enum { RM_SINGLE = 1, RM_SINGLE_HQ, RM_1B, RM_1B_HQ, RM_2B, RM_2B_HQ, RM_3B, RM_3B_HQ, RM_4B, RM_4B_HQ };
    if (renderMode == RM_3B || renderMode == RM_3B_HQ) return 3;
    if (renderMode == RM_2B || renderMode == RM_2B_HQ) return 2;
    if (renderMode == RM_4B || renderMode == RM_4B_HQ) return 4;
    if (renderMode == RM_1B_HQ || renderMode == RM_SINGLE_HQ) return 1;
    if (renderMode == RM_1B || renderMode == RM_SINGLE) return 0;
    if (stride == 36) return 1;
    if (stride == 40) return 4;
    if (stride == 44) return 2;
    return 0;
}

void RTAccelStructManager::InitSkinningPipeline()
{
    if (s_skinInitialized) return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    auto& cache = framegraph::GetPassResourceCache();

    auto skinResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("rt_skin_vertices");
    if (!skinResult.handle) {
        Msg("! [RT] Failed to load rt_skin_vertices shader");
        return;
    }

    fg::RenderDevice::BufferDesc cbDesc;
    cbDesc.debugName = "RTSkinningCB";
    cbDesc.byteSize = sizeof(RTSkinningCB);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = 8192;
    s_skinCB = m_device->CreateBuffer(cbDesc);

    s_skinLayout = cache.GetOrCreateBindingLayoutFromReflection("RTSkinning", *skinResult.reflection, nvDevice);

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = skinResult.handle;
    pipeDesc.bindingLayouts = { s_skinLayout };
    s_skinPipeline = nvDevice->createComputePipeline(pipeDesc);

    s_skinInitialized = s_skinPipeline != nullptr;

    if (s_skinInitialized)
        Msg("* [RT] Skinning compute pipeline created");
}

static CKinematics* GetSkeletonParent(const GeometryBatch& batch)
{
    if (!batch.visual) return nullptr;
    u32 visualType = batch.visual->getType();
    if (visualType == MT_SKELETON_GEOMDEF_ST)
        return static_cast<CSkeletonX_ST*>(batch.visual)->GetParent();
    if (visualType == MT_SKELETON_GEOMDEF_PM)
        return static_cast<CSkeletonX_PM*>(batch.visual)->GetParent();
    return nullptr;
}

void RTAccelStructManager::BuildSkinnedBLAS(
    nvrhi::ICommandList* cmdList,
    GPUCullingManager* gpuCulling,
    const xr_vector<GeometryBatch>& worldBatches,
    const xr_vector<GeometryBatch>& hudBatches)
{
    if (!m_rtSupported || !m_isReady || !gpuCulling)
        return;

    InitSkinningPipeline();
    if (!s_skinInitialized) return;

    if (worldBatches.empty()) {
        InvalidateSkinned();
        return;
    }

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    constexpr u32 SKINNED_VERTEX_STRIDE = 24;

    m_skinnedBatchData.clear();
    m_skinnedBatchData.reserve(worldBatches.size());

    xr_vector<u32> consolidatedIndices;
    u32 currentVertexOffset = 0;
    u32 currentIndexOffset = 0;
    u32 totalVerts = 0;
    u32 totalIndices = 0;
    u32 worldBatchCount = 0;

    auto processBatch = [&](const GeometryBatch& batch) {
        auto* mesh = static_cast<IRender_Mesh*>(static_cast<Fvisual*>(batch.visual));
        CKinematics* parent = GetSkeletonParent(batch);
        u32 boneOffset = parent ? gpuCulling->GetOrUploadSkeleton(cmdList, parent) : 0;

        u16* srcIndices = static_cast<u16*>(mesh->p_rm_Indices->Map(0, mesh->dwPrimitives * 3 * sizeof(u16), true));
        for (u32 i = batch.startIndex; i < batch.startIndex + batch.indexCount; i++)
            consolidatedIndices.push_back(static_cast<u32>(srcIndices[i]));
        mesh->p_rm_Indices->Unmap();

        SkinnedBatchRT sb;
        sb.vertexOffset = currentVertexOffset;
        sb.vertexCount = mesh->vCount;
        sb.indexOffset = currentIndexOffset;
        sb.indexCount = batch.indexCount;
        sb.materialID = batch.bindlessMaterialID;
        sb.srcVB = mesh->p_rm_Vertices->GetBufferHandle().Get();
        sb.srcStride = mesh->vStride;
        sb.srcBaseVertex = mesh->vBase;
        sb.formatID = GetSkinningFormatID(batch.skinningRenderMode, mesh->vStride);
        sb.boneOffset = boneOffset;
        sb.worldMatrix = batch.worldMatrix;
        sb.srcIB = mesh->p_rm_Indices->GetBufferHandle().Get();
        sb.srcStartIndex = batch.startIndex;
        m_skinnedBatchData.push_back(sb);

        currentVertexOffset += mesh->vCount;
        currentIndexOffset += batch.indexCount;
        totalVerts += mesh->vCount;
        totalIndices += batch.indexCount;
    };

    for (const auto& b : worldBatches) {
        processBatch(b);
        worldBatchCount++;
    }
    (void)hudBatches;

    constexpr u32 kMaxSkinnedVerts = 500000u;
    if (totalVerts > kMaxSkinnedVerts) {
        Msg("! [RT] Skinned verts capped %u -> %u", totalVerts, kMaxSkinnedVerts);
        InvalidateSkinned();
        return;
    }

    u64 topoHash = m_skinnedBatchData.size();
    topoHash = topoHash * 131ull + 0xA1FBu;
    for (const auto& sb : m_skinnedBatchData) {
        topoHash = topoHash * 131ull + sb.vertexCount;
        topoHash = topoHash * 131ull + sb.indexCount;
        topoHash = topoHash * 131ull + sb.materialID;
        topoHash = topoHash * 131ull + (u64)(uintptr_t)sb.srcVB;
    }

    m_skinnedSlot = (m_skinnedSlot + 1u) % kSkinnedSlots;
    auto& slot = m_skinnedSlots[m_skinnedSlot];

    u64 vbSize = static_cast<u64>(totalVerts) * SKINNED_VERTEX_STRIDE;
    u64 ibSize = static_cast<u64>(totalIndices) * sizeof(u32);

    if (!slot.vb || slot.vb->getDesc().byteSize < vbSize) {
        nvrhi::BufferDesc desc;
        desc.debugName = "SkinnedOutputVB";
        desc.byteSize = vbSize;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        slot.vb = nvDevice->createBuffer(desc);
        if (!slot.vb) {
            Msg("! [RT] SkinnedOutputVB create failed");
            InvalidateSkinned();
            return;
        }
    }

    if (!slot.ib || slot.ib->getDesc().byteSize < ibSize) {
        nvrhi::BufferDesc desc;
        desc.debugName = "SkinnedConsolidatedIB";
        desc.byteSize = ibSize;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        slot.ib = nvDevice->createBuffer(desc);
        if (!slot.ib) {
            Msg("! [RT] SkinnedIB create failed");
            InvalidateSkinned();
            return;
        }
        slot.topoHash = 0;
    }

    if (slot.topoHash != topoHash)
        cmdList->writeBuffer(slot.ib, consolidatedIndices.data(), consolidatedIndices.size() * sizeof(u32));

    nvrhi::IBuffer* boneBuffer = gpuCulling->GetGlobalBoneBuffer();
    if (!boneBuffer)
        return;
    xr_map<nvrhi::IBuffer*, nvrhi::BindingSetHandle> bindingSetCache;

    nvrhi::ComputeState state;
    state.pipeline = s_skinPipeline;
    auto& cache = framegraph::GetPassResourceCache();

    for (const auto& sb : m_skinnedBatchData) {
        auto it = bindingSetCache.find(sb.srcVB);
        if (it == bindingSetCache.end()) {
            auto* skinRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("rt_skin_vertices", ".cs");
            framegraph::BindingSetBuilder bsb(*skinRefl, nvDevice, "RT.SkinVertices");
            bsb.BufferSRV("g_SrcVB", sb.srcVB)
               .BufferSRV("g_BoneMatrices", boneBuffer)
               .BufferUAV("g_Output", slot.vb)
               .ConstantBuffer("RTSkinningCB", m_device->GetNativeBuffer(s_skinCB));
            it = bindingSetCache.emplace(sb.srcVB, cache.GetOrCreateBindingSet(bsb.Build(), s_skinLayout, nvDevice)).first;
        }

        RTSkinningCB cb;
        cb.worldMatrix = sb.worldMatrix;
        cb.vertexCount = sb.vertexCount;
        cb.vertexStride = sb.srcStride;
        cb.formatID = sb.formatID;
        cb.boneOffset = sb.boneOffset;
        cb.outputOffset = sb.vertexOffset;
        cb.inputBaseVertex = sb.srcBaseVertex;
        cb.pad[0] = 0;
        cb.pad[1] = 0;
        cmdList->writeBuffer(m_device->GetNativeBuffer(s_skinCB), &cb, sizeof(RTSkinningCB));

        state.bindings = { it->second };
        cmdList->setComputeState(state);
        cmdList->dispatch((sb.vertexCount + 255) / 256, 1, 1);
    }

    nvrhi::rt::AccelStructDesc blasDesc;
    blasDesc.debugName = "SkinnedBLAS";
    blasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastBuild;

    for (const auto& sb : m_skinnedBatchData) {
        nvrhi::rt::GeometryTriangles tri;
        tri.setIndexBuffer(slot.ib)
           .setIndexFormat(nvrhi::Format::R32_UINT)
           .setIndexOffset(static_cast<uint64_t>(sb.indexOffset) * sizeof(u32))
           .setIndexCount(sb.indexCount)
           .setVertexBuffer(slot.vb)
           .setVertexFormat(nvrhi::Format::RGB32_FLOAT)
           .setVertexStride(SKINNED_VERTEX_STRIDE)
           .setVertexOffset(static_cast<uint64_t>(sb.vertexOffset) * SKINNED_VERTEX_STRIDE)
           .setVertexCount(sb.vertexCount);

        nvrhi::rt::GeometryDesc geom;
        geom.setTriangles(tri).setFlags(nvrhi::rt::GeometryFlags::Opaque);
        blasDesc.addBottomLevelGeometry(geom);
    }

    const bool needCreate = !slot.blas || slot.topoHash != topoHash ||
        slot.totalVerts != totalVerts || slot.totalIndices != totalIndices;
    if (needCreate) {
        slot.blas = nvDevice->createAccelStruct(blasDesc);
        if (!slot.blas) {
            Msg("! [RT] Skinned BLAS create failed");
            InvalidateSkinned();
            return;
        }
        static u32 s_skinnedBuildLog = 0;
        if ((++s_skinnedBuildLog % 120u) == 1u)
            Msg("* [RT] Built %u skinned BLAS (%u verts, %u indices)",
                (u32)m_skinnedBatchData.size(), totalVerts, totalIndices);
    }

    if (slot.blas)
        nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, slot.blas, blasDesc);

    slot.totalVerts = totalVerts;
    slot.totalIndices = totalIndices;
    slot.topoHash = topoHash;
    m_skinnedBlas = slot.blas;
    m_skinnedTotalVerts = totalVerts;
    m_skinnedTotalIndices = totalIndices;
    m_skinnedTopoHash = topoHash;
    m_batchCounts.skinned = (u32)m_skinnedBatchData.size();
    m_batchCounts.skinnedWorld = worldBatchCount;
    m_batchCounts.skinnedHud = m_batchCounts.skinned > worldBatchCount
        ? m_batchCounts.skinned - worldBatchCount
        : 0;
    m_skinnedReady = true;
}

void RTAccelStructManager::InvalidateSkinned()
{
    m_skinnedBlas = nullptr;
    m_skinnedBatchData.clear();
    m_skinnedTotalVerts = 0;
    m_skinnedTotalIndices = 0;
    m_skinnedTopoHash = 0;
    m_batchCounts.skinned = 0;
    m_batchCounts.skinnedWorld = 0;
    m_batchCounts.skinnedHud = 0;
    m_skinnedReady = false;
}

void RTAccelStructManager::InvalidateGrass()
{
    m_grassBlas = nullptr;
    m_grassTotalVerts = 0;
    m_grassTotalIndices = 0;
    m_batchCounts.grass = 0;
    m_grassReady = false;
    m_grassBillboardMode = false;
    m_detailAtlasIndex = 0;
}

void RTAccelStructManager::InvalidateParticles()
{
    m_particleBlas = nullptr;
    m_particleTotalVerts = 0;
    m_particleTotalIndices = 0;
    m_batchCounts.particles = 0;
    m_particleReady = false;
}

void RTAccelStructManager::RebuildDynamic(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    BuildTLAS(cmdList);
    CreateBatchInfoBuffer(cmdList, gpuCulling);
}

void RTAccelStructManager::InitGrassPipeline()
{
    if (s_grassInitialized) return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    auto& cache = framegraph::GetPassResourceCache();

    auto grassResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("rt_grass_vertices");
    if (!grassResult.handle) {
        Msg("! [RT] Failed to load rt_grass_vertices shader");
        return;
    }

    fg::RenderDevice::BufferDesc grassCBDesc;
    grassCBDesc.debugName = "GrassRTCB";
    grassCBDesc.byteSize = sizeof(GrassRTCB);
    grassCBDesc.isConstantBuffer = true;
    grassCBDesc.isVolatile = true;
    grassCBDesc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    s_grassCB = m_device->CreateBuffer(grassCBDesc);

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    s_grassSampler = cache.GetOrCreateSampler("RTGrass", samplerDesc, nvDevice);

    s_grassLayout = cache.GetOrCreateBindingLayoutFromReflection("RTGrass", *grassResult.reflection, nvDevice);

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = grassResult.handle;
    pipeDesc.bindingLayouts = { s_grassLayout };
    s_grassPipeline = nvDevice->createComputePipeline(pipeDesc);

    s_grassInitialized = s_grassPipeline != nullptr;

    if (s_grassInitialized)
        Msg("* [RT] Grass compute pipeline created");
}

void RTAccelStructManager::InitBillboardPipeline()
{
    if (s_billboardInitialized) return;

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    auto& cache = framegraph::GetPassResourceCache();

    auto billboardResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("rt_grass_billboard");
    if (!billboardResult.handle) {
        Msg("! [RT] Failed to load rt_grass_billboard shader");
        return;
    }

    fg::RenderDevice::BufferDesc bbCBDesc;
    bbCBDesc.debugName = "BillboardRTCB";
    bbCBDesc.byteSize = sizeof(BillboardRTCB);
    bbCBDesc.isConstantBuffer = true;
    bbCBDesc.isVolatile = true;
    bbCBDesc.maxVersions = fg::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    s_billboardCB = m_device->CreateBuffer(bbCBDesc);

    nvrhi::BufferDesc countDesc;
    countDesc.debugName = "BillboardPackedCount";
    countDesc.byteSize = 16;
    countDesc.canHaveUAVs = true;
    countDesc.canHaveRawViews = true;
    countDesc.initialState = nvrhi::ResourceStates::UnorderedAccess;
    countDesc.keepInitialState = true;
    s_billboardCount = nvDevice->createBuffer(countDesc);

    s_billboardLayout = cache.GetOrCreateBindingLayoutFromReflection("RTBillboardCompact", *billboardResult.reflection, nvDevice);

    nvrhi::ComputePipelineDesc pipeDesc;
    pipeDesc.CS = billboardResult.handle;
    pipeDesc.bindingLayouts = { s_billboardLayout };
    s_billboardPipeline = nvDevice->createComputePipeline(pipeDesc);

    s_billboardInitialized = s_billboardPipeline != nullptr;

    if (s_billboardInitialized)
        Msg("* [RT] Billboard grass compute pipeline created");
}

void RTAccelStructManager::BuildGrassBLAS(nvrhi::ICommandList* cmdList, FGDetailManager* detailMgr)
{
    if (!m_rtSupported || !m_isReady || !detailMgr)
        return;

    const bool billboardMode = !ps_r__detail_gpu;
    const auto& stats = detailMgr->GetCullingStats();
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    constexpr u32 GRASS_VERTEX_STRIDE = 24;
    u32 totalVerts = 0;
    u32 totalIndices = 0;

    m_grassSlot = (m_grassSlot + 1u) % kGrassSlots;
    auto& slot = m_grassSlots[m_grassSlot];

    auto ensureGrassBuffers = [&](u64 vbSize, u64 ibSize) -> bool {
        if (!slot.vb || slot.vb->getDesc().byteSize < vbSize) {
            nvrhi::BufferDesc desc;
            desc.debugName = "GrassOutputVB";
            desc.byteSize = vbSize;
            desc.canHaveRawViews = true;
            desc.isAccelStructBuildInput = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;
            desc.canHaveUAVs = true;
            slot.vb = nvDevice->createBuffer(desc);
            if (!slot.vb) return false;
        }
        if (!slot.ib || slot.ib->getDesc().byteSize < ibSize) {
            nvrhi::BufferDesc desc;
            desc.debugName = "GrassConsolidatedIB";
            desc.byteSize = ibSize;
            desc.canHaveRawViews = true;
            desc.isAccelStructBuildInput = true;
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;
            slot.ib = nvDevice->createBuffer(desc);
            if (!slot.ib) return false;
        }
        return true;
    };

    if (billboardMode) {
        InitBillboardPipeline();
        if (!s_billboardInitialized) return;

        u32 maxVPB = detailMgr->maxPulledIndexCount;
        maxVPB = (maxVPB / 3) * 3;
        const bool haveBB = detailMgr->billboardDrawArgsBuffer && detailMgr->visibleBillboardInstancesBuffer;
        const bool haveDecal = detailMgr->decalDrawArgsBuffer && detailMgr->visibleDecalInstancesBuffer;
        if (maxVPB == 0 || (!haveBB && !haveDecal)) { InvalidateGrass(); return; }

        const float distScale = std::clamp(ps_r_rt_detail_dist / 40.f, 0.2f, 1.f);
        constexpr u32 kMaxRtBillboards = 16384u;
        u32 visible = (u32)std::max(512u, (u32)(kMaxRtBillboards * distScale));
        totalVerts = visible * maxVPB;
        totalIndices = totalVerts;
        if (!s_billboardCount || !ensureGrassBuffers(
                static_cast<u64>(totalVerts) * GRASS_VERTEX_STRIDE,
                static_cast<u64>(totalIndices) * sizeof(u32))) {
            InvalidateGrass();
            return;
        }

        BillboardRTCB cb;
        cb.maxVertsPerBillboard = maxVPB;
        cb.maxBillboards = visible;
        cb.windAngleDeg = g_pGamePersistent
            ? g_pGamePersistent->Environment().CurrentEnv.wind_direction
            : 0.0f;
        cb.windSpeed = detailMgr->windSpeed;
        cb.time = Device.fTimeGlobal;
        cb.windDisplacement = ps_r3_grass_wind_displacement;
        cb.pad[0] = cb.pad[1] = 0;
        cb.cameraPos[0] = Device.vCameraPosition.x;
        cb.cameraPos[1] = Device.vCameraPosition.y;
        cb.cameraPos[2] = Device.vCameraPosition.z;
        const float casterDist = std::max(ps_r_rt_detail_dist, 32.f);
        cb.maxDistanceSq = casterDist * casterDist;

        auto* billboardRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("rt_grass_billboard", ".cs");
        auto dispatchPacked = [&](nvrhi::IBuffer* visibleIdx, nvrhi::IBuffer* drawArgs) {
            if (!visibleIdx || !drawArgs)
                return;
            framegraph::BindingSetBuilder bsb(*billboardRefl, nvDevice, "RT.Billboard");
            bsb.BufferSRV("g_AllInstances", detailMgr->generatedInstancesBuffer)
               .BufferSRV("g_VisibleIndices", visibleIdx)
               .BufferSRV("g_DetailModels", detailMgr->detailModelsBuffer)
               .BufferSRV("g_PulledVerts", detailMgr->pulledVertexBuffer)
               .BufferSRV("g_DrawArgs", drawArgs)
               .Texture("g_Perlin4D", detailMgr->perlin4dTexture)
               .BufferUAV("g_Output", slot.vb)
               .BufferUAV("g_OutputIB", slot.ib)
               .BufferUAV("g_PackedCount", s_billboardCount)
               .ConstantBuffer("BillboardRTCB", m_device->GetNativeBuffer(s_billboardCB));
            auto bindingSet = nvDevice->createBindingSet(bsb.Build(), s_billboardLayout);
            nvrhi::ComputeState state;
            state.pipeline = s_billboardPipeline;
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);
            cmdList->dispatch((visible + 255) / 256, 1, 1);
        };

        cmdList->writeBuffer(m_device->GetNativeBuffer(s_billboardCB), &cb, sizeof(BillboardRTCB));
        cmdList->setBufferState(slot.vb, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(slot.ib, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->setBufferState(s_billboardCount, nvrhi::ResourceStates::UnorderedAccess);
        cmdList->commitBarriers();
        cmdList->clearBufferUInt(s_billboardCount, 0);
        cmdList->clearBufferUInt(slot.vb, 0);
        cmdList->clearBufferUInt(slot.ib, 0);
        if (haveBB)
            dispatchPacked(detailMgr->visibleBillboardInstancesBuffer, detailMgr->billboardDrawArgsBuffer);
        if (haveBB && haveDecal) {
            cmdList->setBufferState(slot.vb, nvrhi::ResourceStates::UnorderedAccess);
            cmdList->setBufferState(slot.ib, nvrhi::ResourceStates::UnorderedAccess);
            cmdList->setBufferState(s_billboardCount, nvrhi::ResourceStates::UnorderedAccess);
            cmdList->commitBarriers();
        }
        if (haveDecal)
            dispatchPacked(detailMgr->visibleDecalInstancesBuffer, detailMgr->decalDrawArgsBuffer);
    } else {
        InitGrassPipeline();
        if (!s_grassInitialized) return;

        const float distScale = std::clamp(ps_r_rt_detail_dist / 40.f, 0.2f, 1.f);
        constexpr u32 kMaxRtGrassBlades = 6144u;
        u32 budget = (u32)std::max(512u, (u32)(kMaxRtGrassBlades * distScale));

        u32 lodCounts[FGDetailManager::LOD_COUNT] = { 0, 0, 0 };
        u32 take0 = std::min(stats.visibleLOD0Count, budget);
        lodCounts[0] = take0;
        budget -= take0;
        u32 take1 = std::min(stats.visibleLOD1Count, budget);
        lodCounts[1] = take1;
        budget -= take1;
        u32 take2 = std::min(stats.visibleLOD2Count, budget);
        lodCounts[2] = take2;

        u32 totalBlades = lodCounts[0] + lodCounts[1] + lodCounts[2];
        if (totalBlades == 0) { InvalidateGrass(); return; }

        u32 lodVertsPerBlade[FGDetailManager::LOD_COUNT];
        u32 lodIndicesPerBlade[FGDetailManager::LOD_COUNT];
        for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; lod++) {
            u32 seg = FGDetailManager::LOD_SEGMENTS[lod];
            lodVertsPerBlade[lod] = seg * 2 + 1;
            lodIndicesPerBlade[lod] = (seg - 1) * 6 + 3;
            totalVerts += lodCounts[lod] * lodVertsPerBlade[lod];
            totalIndices += lodCounts[lod] * lodIndicesPerBlade[lod];
        }

        if (!ensureGrassBuffers(
                static_cast<u64>(totalVerts) * GRASS_VERTEX_STRIDE,
                static_cast<u64>(totalIndices) * sizeof(u32))) {
            InvalidateGrass();
            return;
        }

        float windAngleDeg = 0.0f;
        float windSpeed = detailMgr->windSpeed;
        if (g_pGamePersistent)
            windAngleDeg = g_pGamePersistent->Environment().CurrentEnv.wind_direction;

        GrassRTCB cbTemplate;
        cbTemplate.detail_params.set(
            float(detailMgr->dtH.x_size()), float(detailMgr->dtH.z_size()),
            float(detailMgr->dtH.x_offs()), float(detailMgr->dtH.z_offs()));
        cbTemplate.wind_direction.set(windAngleDeg, windSpeed, 0.0f, 0.0f);
        cbTemplate.wave.set(1.0f / 5.0f, 1.0f / 7.0f, 1.0f / 3.0f, Device.fTimeGlobal);
        cbTemplate.grass_wind_displacement = ps_r3_grass_wind_displacement;
        cbTemplate.grass_blade_height = ps_r3_grass_blade_height;
        cbTemplate.grass_blade_width = ps_r3_grass_blade_width;
        cbTemplate.pad[0] = cbTemplate.pad[1] = cbTemplate.pad[2] = 0;

        nvrhi::ComputeState state;
        state.pipeline = s_grassPipeline;

        u32 vertexOffset = 0;
        u32 indexOffset = 0;
        for (u32 lod = 0; lod < FGDetailManager::LOD_COUNT; lod++) {
            if (lodCounts[lod] == 0) continue;

            auto* grassRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("rt_grass_vertices", ".cs");
            framegraph::BindingSetBuilder bsb(*grassRefl, nvDevice, "RT.Grass");
            bsb.BufferSRV("g_AllInstances", detailMgr->generatedInstancesBuffer)
               .BufferSRV("g_SlotData", detailMgr->slotDataBuffer)
               .BufferSRV("g_VisibleIndices", detailMgr->visibleInstancesBuffer[lod])
               .Texture("g_WindTexture", detailMgr->perlin4dTexture)
               .BufferUAV("g_Output", slot.vb)
               .BufferUAV("g_OutputIB", slot.ib)
               .ConstantBuffer("GrassRTCB", m_device->GetNativeBuffer(s_grassCB));
            auto bindingSet = nvDevice->createBindingSet(bsb.Build(), s_grassLayout);

            GrassRTCB cb = cbTemplate;
            cb.segments = FGDetailManager::LOD_SEGMENTS[lod];
            cb.vertsPerBlade = lodVertsPerBlade[lod];
            cb.indicesPerBlade = lodIndicesPerBlade[lod];
            cb.bladeCount = lodCounts[lod];
            cb.outputVertexOffset = vertexOffset;
            cb.outputIndexOffset = indexOffset;

            cmdList->writeBuffer(m_device->GetNativeBuffer(s_grassCB), &cb, sizeof(GrassRTCB));
            state.bindings = { bindingSet };
            cmdList->setComputeState(state);

            u32 totalVertsThisLod = lodCounts[lod] * lodVertsPerBlade[lod];
            cmdList->dispatch((totalVertsThisLod + 255) / 256, 1, 1);

            vertexOffset += totalVertsThisLod;
            indexOffset += lodCounts[lod] * lodIndicesPerBlade[lod];
        }
    }

    cmdList->setBufferState(slot.vb, nvrhi::ResourceStates::AccelStructBuildInput);
    cmdList->setBufferState(slot.ib, nvrhi::ResourceStates::AccelStructBuildInput);
    cmdList->commitBarriers();

    nvrhi::rt::AccelStructDesc blasDesc;
    blasDesc.debugName = "GrassBLAS";
    blasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastBuild;

    nvrhi::rt::GeometryTriangles tri;
    tri.setIndexBuffer(slot.ib)
       .setIndexFormat(nvrhi::Format::R32_UINT)
       .setIndexOffset(0)
       .setIndexCount(totalIndices)
       .setVertexBuffer(slot.vb)
       .setVertexFormat(nvrhi::Format::RGB32_FLOAT)
       .setVertexStride(GRASS_VERTEX_STRIDE)
       .setVertexOffset(0)
       .setVertexCount(totalVerts);

    nvrhi::rt::GeometryDesc geom;
    geom.setTriangles(tri).setFlags(nvrhi::rt::GeometryFlags::None);
    blasDesc.addBottomLevelGeometry(geom);

    if (!slot.blas || slot.totalVerts != totalVerts || slot.totalIndices != totalIndices)
        slot.blas = nvDevice->createAccelStruct(blasDesc);

    if (slot.blas)
        nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, slot.blas, blasDesc);

    cmdList->setBufferState(slot.vb, nvrhi::ResourceStates::ShaderResource);
    cmdList->setBufferState(slot.ib, nvrhi::ResourceStates::ShaderResource);
    cmdList->commitBarriers();

    slot.totalVerts = totalVerts;
    slot.totalIndices = totalIndices;
    m_grassOutputVB = slot.vb;
    m_grassIB = slot.ib;
    m_grassBlas = slot.blas;
    m_grassTotalVerts = totalVerts;
    m_grassTotalIndices = totalIndices;
    m_batchCounts.grass = 1;
    m_grassReady = true;
    m_grassBillboardMode = billboardMode;
    m_detailAtlasIndex = billboardMode ? detailMgr->buildDetailsBindlessIndex : 0;
    static u32 s_lastGrassVerts = 0;
    static u32 s_lastGrassAtlas = UINT32_MAX;
    if (s_lastGrassVerts != totalVerts || s_lastGrassAtlas != m_detailAtlasIndex) {
        Msg("* [RT] Detail BLAS ready: mode=%s capacity=%u verts, visible~%u (bb=%u decal=%u) atlas=%u",
            billboardMode ? "billboard" : "blade", totalVerts,
            stats.visibleBillboardCount + stats.visibleDecalCount,
            stats.visibleBillboardCount, stats.visibleDecalCount, m_detailAtlasIndex);
        s_lastGrassVerts = totalVerts;
        s_lastGrassAtlas = m_detailAtlasIndex;
    }
}

void RTAccelStructManager::BuildParticleBLAS(
    nvrhi::ICommandList* cmdList,
    const xr_vector<passes::ParticleBatch>& worldBatches)
{
    if (!m_rtSupported || !m_isReady || !cmdList)
        return;
    if (worldBatches.empty()) {
        InvalidateParticles();
        return;
    }

    xr_vector<passes::ParticleVertex> vertices;
    xr_vector<u32> indices;
    const u32 quads = passes::BuildEmissiveParticleRTGeometry(worldBatches, vertices, indices, 8192u);
    if (quads == 0 || vertices.empty() || indices.empty()) {
        InvalidateParticles();
        return;
    }

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    const u64 vbSize = vertices.size() * sizeof(passes::ParticleVertex);
    const u64 ibSize = indices.size() * sizeof(u32);
    if (!m_particleOutputVB || m_particleOutputVB->getDesc().byteSize < vbSize) {
        nvrhi::BufferDesc desc;
        desc.debugName = "ParticleRTVB";
        desc.byteSize = std::max<u64>(vbSize, 65536);
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_particleOutputVB = nvDevice->createBuffer(desc);
    }
    if (!m_particleIB || m_particleIB->getDesc().byteSize < ibSize) {
        nvrhi::BufferDesc desc;
        desc.debugName = "ParticleRTIB";
        desc.byteSize = std::max<u64>(ibSize, 65536);
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_particleIB = nvDevice->createBuffer(desc);
    }
    if (!m_particleOutputVB || !m_particleIB) {
        InvalidateParticles();
        return;
    }

    cmdList->writeBuffer(m_particleOutputVB, vertices.data(), vbSize);
    cmdList->writeBuffer(m_particleIB, indices.data(), ibSize);

    nvrhi::rt::AccelStructDesc blasDesc;
    blasDesc.debugName = "ParticleBLAS";
    blasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastBuild;

    nvrhi::rt::GeometryTriangles tri;
    tri.setIndexBuffer(m_particleIB)
       .setIndexFormat(nvrhi::Format::R32_UINT)
       .setIndexOffset(0)
       .setIndexCount((u32)indices.size())
       .setVertexBuffer(m_particleOutputVB)
       .setVertexFormat(nvrhi::Format::RGB32_FLOAT)
       .setVertexStride(sizeof(passes::ParticleVertex))
       .setVertexOffset(0)
       .setVertexCount((u32)vertices.size());

    nvrhi::rt::GeometryDesc geom;
    geom.setTriangles(tri).setFlags(nvrhi::rt::GeometryFlags::None);
    blasDesc.addBottomLevelGeometry(geom);

    if (!m_particleBlas || m_particleTotalVerts != (u32)vertices.size() ||
        m_particleTotalIndices != (u32)indices.size())
        m_particleBlas = nvDevice->createAccelStruct(blasDesc);

    if (m_particleBlas)
        nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, m_particleBlas, blasDesc);

    m_particleTotalVerts = (u32)vertices.size();
    m_particleTotalIndices = (u32)indices.size();
    m_batchCounts.particles = 1;
    m_particleReady = m_particleBlas != nullptr;
}

}
