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
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#include <nvrhi/utils.h>

extern ENGINE_API float ps_r3_grass_blade_width;
extern ENGINE_API float ps_r3_grass_blade_height;
extern ENGINE_API float ps_r3_grass_wind_displacement;

namespace xray::render::RENDER_NAMESPACE {
    class CRender;
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE {

extern int ps_r__detail_gpu;

nvrhi::ComputePipelineHandle RTAccelStructManager::s_skinPipeline;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_skinLayout;
ng::BufferHandle RTAccelStructManager::s_skinCB;
bool RTAccelStructManager::s_skinInitialized = false;

nvrhi::ComputePipelineHandle RTAccelStructManager::s_grassPipeline;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_grassLayout;
ng::BufferHandle RTAccelStructManager::s_grassCB;
nvrhi::SamplerHandle RTAccelStructManager::s_grassSampler;
bool RTAccelStructManager::s_grassInitialized = false;

nvrhi::ComputePipelineHandle RTAccelStructManager::s_billboardPipeline;
nvrhi::BindingLayoutHandle RTAccelStructManager::s_billboardLayout;
ng::BufferHandle RTAccelStructManager::s_billboardCB;
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
    u32 pad[3];
};
static_assert(sizeof(BillboardRTCB) == 16, "BillboardRTCB must be 16 bytes");

static void FmatrixToRTTransform(const Fmatrix& m, nvrhi::rt::AffineTransform& out)
{
    out[0]  = m._11; out[1]  = m._21; out[2]  = m._31; out[3]  = m._41;
    out[4]  = m._12; out[5]  = m._22; out[6]  = m._32; out[7]  = m._42;
    out[8]  = m._13; out[9]  = m._23; out[10] = m._33; out[11] = m._43;
}

void RTAccelStructManager::Initialize(ng::RenderDevice* device)
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

void RTAccelStructManager::Shutdown()
{
    m_staticBlas = nullptr;
    m_uniqueGeometries.clear();
    m_tlas = nullptr;
    m_batchInfoBuffer = nullptr;
    m_skinnedOutputVB = nullptr;
    m_skinnedIB = nullptr;
    m_skinnedBlas = nullptr;
    m_skinnedBatchData.clear();
    m_skinnedReady = false;
    m_grassOutputVB = nullptr;
    m_grassIB = nullptr;
    m_grassBlas = nullptr;
    m_grassTotalVerts = 0;
    m_grassTotalIndices = 0;
    m_grassReady = false;
    m_isReady = false;
    m_batchCount = 0;
    m_batchCounts = {};

    s_skinPipeline = nullptr;
    s_skinLayout = nullptr;
    s_skinCB = ng::BufferHandle();
    s_skinInitialized = false;

    s_grassPipeline = nullptr;
    s_grassLayout = nullptr;
    s_grassCB = ng::BufferHandle();
    s_grassSampler = nullptr;
    s_grassInitialized = false;

    s_billboardPipeline = nullptr;
    s_billboardLayout = nullptr;
    s_billboardCB = ng::BufferHandle();
    s_billboardInitialized = false;
}

void RTAccelStructManager::InvalidateShaderPipelines()
{
    s_skinPipeline = nullptr;
    s_skinLayout = nullptr;
    s_skinCB = ng::BufferHandle();
    s_skinInitialized = false;

    s_grassPipeline = nullptr;
    s_grassLayout = nullptr;
    s_grassCB = ng::BufferHandle();
    s_grassSampler = nullptr;
    s_grassInitialized = false;

    s_billboardPipeline = nullptr;
    s_billboardLayout = nullptr;
    s_billboardCB = ng::BufferHandle();
    s_billboardInitialized = false;

    Msg("* [RTAccel] Shader pipelines invalidated for hot-reload");
}

void RTAccelStructManager::BuildIfNeeded(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    if (m_isReady || !m_rtSupported || !gpuCulling)
        return;

    if (!gpuCulling->IsMegaDataUploaded())
        return;

    const auto& drawArgs = gpuCulling->GetStaticDrawArgsData();
    if (drawArgs.empty())
        return;

    m_megaVB = gpuCulling->GetMegaVertexBuffer();
    m_megaIB = gpuCulling->GetMegaIndexBuffer();

    BuildStaticBLAS(cmdList, gpuCulling);
    BuildInstancedBLAS(cmdList, gpuCulling);
    BuildTLAS(cmdList);
    if (!m_tlas) return;

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
    const xr_vector<bool>* skipMask = nullptr)
{
    u32 count = 0;
    for (u32 i = 0; i < (u32)drawArgs.size(); i++) {
        if (skipMask && i < skipMask->size() && (*skipMask)[i])
            continue;

        const auto& args = drawArgs[i];
        if (args.indexCountPerInstance == 0)
            continue;

        u32 baseVert = static_cast<u32>(args.baseVertexLocation);
        u32 batchVertexCount = (vertexCounts && i < vertexCounts->size()) ? (*vertexCounts)[i] : (totalVertexCount - baseVert);

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

        nvrhi::rt::GeometryDesc geom;
        geom.setTriangles(tri)
            .setFlags(nvrhi::rt::GeometryFlags::Opaque);

        blasDesc.addBottomLevelGeometry(geom);
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
        if (i < staticInstances.size() && memcmp(&staticInstances[i].world, &Fidentity, sizeof(Fmatrix)) != 0)
            isTransformed[i] = true;
    }

    nvrhi::rt::AccelStructDesc blasDesc;
    blasDesc.debugName = "StaticSceneBLAS";
    blasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace |
                          nvrhi::rt::AccelStructBuildFlags::AllowCompaction;

    m_batchCounts.identityStatic = AddBatchesToBLAS(blasDesc, staticDrawArgs,
        m_megaVB, m_megaIB, totalVerts, vertexStride, &staticVertCounts, &isTransformed);
    m_batchCounts.terrain = AddBatchesToBLAS(blasDesc, gpuCulling->GetTerrainDrawArgsData(),
        m_megaVB, m_megaIB, totalVerts, vertexStride);
    m_batchCounts.transparent = AddBatchesToBLAS(blasDesc, gpuCulling->GetTransparentDrawArgsData(),
        m_megaVB, m_megaIB, totalVerts, vertexStride);

    u32 staticTotal = m_batchCounts.identityStatic + m_batchCounts.terrain + m_batchCounts.transparent;
    if (staticTotal == 0) return;

    m_staticBlas = nvDevice->createAccelStruct(blasDesc);
    if (!m_staticBlas) return;

    nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, m_staticBlas, blasDesc);
    cmdList->compactBottomLevelAccelStructs();
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
        if (memcmp(&staticInstances[i].world, &Fidentity, sizeof(Fmatrix)) == 0) continue;

        const auto& args = staticDrawArgs[i];
        if (args.indexCountPerInstance == 0) continue;

        GeometryKey key = { args.startIndexLocation, args.baseVertexLocation, args.indexCountPerInstance };
        InstanceInfo inst;
        inst.world = staticInstances[i].world;
        inst.materialID = (i < staticMaterialIDs.size()) ? staticMaterialIDs[i] : 0;

        auto it = keyToIndex.find(key);
        if (it != keyToIndex.end()) {
            m_uniqueGeometries[it->second].instances.push_back(inst);
        } else {
            u32 baseVert = static_cast<u32>(args.baseVertexLocation);
            u32 batchVertexCount = (i < staticVertCounts.size()) ? staticVertCounts[i] : (totalVerts - baseVert);

            UniqueGeometry ug;
            ug.key = key;
            ug.vertexCount = batchVertexCount;
            ug.instances.push_back(inst);
            keyToIndex[key] = (u32)m_uniqueGeometries.size();
            m_uniqueGeometries.push_back(std::move(ug));
        }
    }

    for (auto& ug : m_uniqueGeometries) {
        nvrhi::rt::AccelStructDesc blasDesc;
        blasDesc.debugName = "InstancedBLAS";
        blasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace |
                              nvrhi::rt::AccelStructBuildFlags::AllowCompaction;

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
        geom.setTriangles(tri).setFlags(nvrhi::rt::GeometryFlags::Opaque);
        blasDesc.addBottomLevelGeometry(geom);

        ug.blas = nvDevice->createAccelStruct(blasDesc);
        if (!ug.blas) continue;

        nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, ug.blas, blasDesc);
    }

    cmdList->compactBottomLevelAccelStructs();

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
    u32 instanceCount = (m_staticBlas ? 1 : 0) + totalInstances + (m_skinnedBlas ? 1 : 0) + (m_grassBlas ? 1 : 0);
    if (instanceCount == 0) {
        Msg("! [RT] No BLAS to build TLAS from");
        return;
    }

    nvrhi::rt::AccelStructDesc tlasDesc;
    tlasDesc.debugName = "SceneTLAS";
    tlasDesc.isTopLevel = true;
    tlasDesc.topLevelMaxInstances = instanceCount;
    tlasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastTrace;

    m_tlas = nvDevice->createAccelStruct(tlasDesc);
    if (!m_tlas) {
        Msg("! [RT] Failed to create TLAS");
        return;
    }

    xr_vector<nvrhi::rt::InstanceDesc> instances;
    instances.reserve(instanceCount);

    u32 staticGeomCount = m_batchCounts.identityStatic + m_batchCounts.terrain + m_batchCounts.transparent;

    if (m_staticBlas) {
        nvrhi::rt::InstanceDesc inst;
        inst.setTransform(nvrhi::rt::c_IdentityTransform)
            .setInstanceID(0)
            .setInstanceMask(0x01)
            .setFlags(nvrhi::rt::InstanceFlags::ForceOpaque)
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
                .setInstanceMask(0x01)
                .setFlags(nvrhi::rt::InstanceFlags::ForceOpaque)
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
            .setInstanceMask(0x01)
            .setFlags(nvrhi::rt::InstanceFlags::ForceOpaque)
            .setBLAS(m_skinnedBlas);
        instances.push_back(inst);
    }

    u32 grassBatchOffset = skinnedBatchOffset + skinnedCount;
    if (m_grassBlas && grassCount > 0) {
        auto grassInstFlags = m_grassBillboardMode
            ? nvrhi::rt::InstanceFlags::TriangleCullDisable
            : nvrhi::rt::InstanceFlags::ForceOpaque;
        nvrhi::rt::InstanceDesc inst;
        inst.setTransform(nvrhi::rt::c_IdentityTransform)
            .setInstanceID(grassBatchOffset)
            .setInstanceMask(0x01)
            .setFlags(grassInstFlags)
            .setBLAS(m_grassBlas);
        instances.push_back(inst);
    }

    m_batchCount = grassBatchOffset + grassCount;
    cmdList->buildTopLevelAccelStruct(m_tlas, instances.data(), (u32)instances.size());
}

static void AppendBatchInfos(
    xr_vector<RTBatchInfo>& out,
    const xr_vector<IndirectDrawArgs>& drawArgs,
    const xr_vector<u32>& materialIDs,
    const xr_vector<bool>* skipMask = nullptr)
{
    for (u32 i = 0; i < (u32)drawArgs.size(); i++) {
        if (skipMask && i < skipMask->size() && (*skipMask)[i])
            continue;
        if (drawArgs[i].indexCountPerInstance == 0)
            continue;
        RTBatchInfo info;
        info.materialID = (i < materialIDs.size()) ? materialIDs[i] : 0;
        info.startIndex = drawArgs[i].startIndexLocation;
        info.baseVertex = drawArgs[i].baseVertexLocation;
        info.indexCount = drawArgs[i].indexCountPerInstance;
        out.push_back(info);
    }
}

void RTAccelStructManager::CreateBatchInfoBuffer(nvrhi::ICommandList* cmdList, GPUCullingManager* gpuCulling)
{
    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();

    const auto& staticDrawArgs = gpuCulling->GetStaticDrawArgsData();
    const auto& staticInstances = gpuCulling->GetStaticInstanceData();

    xr_vector<bool> isTransformed(staticDrawArgs.size(), false);
    for (u32 i = 0; i < (u32)staticDrawArgs.size(); i++) {
        if (i < staticInstances.size() && memcmp(&staticInstances[i].world, &Fidentity, sizeof(Fmatrix)) != 0)
            isTransformed[i] = true;
    }

    xr_vector<RTBatchInfo> batchInfos;
    batchInfos.reserve(m_batchCount);

    AppendBatchInfos(batchInfos, staticDrawArgs, gpuCulling->GetStaticMaterialIDData(), &isTransformed);
    AppendBatchInfos(batchInfos, gpuCulling->GetTerrainDrawArgsData(), gpuCulling->GetTerrainMaterialIDData());
    AppendBatchInfos(batchInfos, gpuCulling->GetTransparentDrawArgsData(), gpuCulling->GetTransparentMaterialIDData());

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

    if (batchInfos.empty()) return;

    nvrhi::BufferDesc desc;
    desc.debugName = "RTBatchInfoBuffer";
    desc.byteSize = batchInfos.size() * sizeof(RTBatchInfo);
    desc.structStride = sizeof(RTBatchInfo);
    desc.initialState = nvrhi::ResourceStates::ShaderResource;
    desc.keepInitialState = true;

    m_batchInfoBuffer = nvDevice->createBuffer(desc);
    cmdList->writeBuffer(m_batchInfoBuffer, batchInfos.data(), batchInfos.size() * sizeof(RTBatchInfo));
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

    auto skinResult = RImplementation.m_shaderLoader->LoadComputeShader("rt_skin_vertices");
    if (!skinResult.handle) {
        Msg("! [RT] Failed to load rt_skin_vertices shader");
        return;
    }

    ng::RenderDevice::BufferDesc cbDesc;
    cbDesc.debugName = "RTSkinningCB";
    cbDesc.byteSize = sizeof(RTSkinningCB);
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.maxVersions = ng::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
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

    if (worldBatches.empty() && hudBatches.empty()) {
        InvalidateSkinned();
        return;
    }

    nvrhi::IDevice* nvDevice = m_device->GetNVRHIDevice();
    constexpr u32 SKINNED_VERTEX_STRIDE = 24;

    m_skinnedBatchData.clear();
    m_skinnedBatchData.reserve(worldBatches.size() + hudBatches.size());

    xr_vector<u32> consolidatedIndices;
    u32 currentVertexOffset = 0;
    u32 currentIndexOffset = 0;
    u32 totalVerts = 0;
    u32 totalIndices = 0;

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

    for (const auto& b : worldBatches) processBatch(b);
    for (const auto& b : hudBatches) processBatch(b);

    u64 vbSize = static_cast<u64>(totalVerts) * SKINNED_VERTEX_STRIDE;
    u64 ibSize = static_cast<u64>(totalIndices) * sizeof(u32);

    if (!m_skinnedOutputVB || m_skinnedOutputVB->getDesc().byteSize < vbSize) {
        nvrhi::BufferDesc desc;
        desc.debugName = "SkinnedOutputVB";
        desc.byteSize = vbSize;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = true;
        desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
        desc.keepInitialState = true;
        desc.canHaveUAVs = true;
        m_skinnedOutputVB = nvDevice->createBuffer(desc);
    }

    if (!m_skinnedIB || m_skinnedIB->getDesc().byteSize < ibSize) {
        nvrhi::BufferDesc desc;
        desc.debugName = "SkinnedConsolidatedIB";
        desc.byteSize = ibSize;
        desc.canHaveRawViews = true;
        desc.isAccelStructBuildInput = true;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        m_skinnedIB = nvDevice->createBuffer(desc);
    }

    cmdList->writeBuffer(m_skinnedIB, consolidatedIndices.data(), consolidatedIndices.size() * sizeof(u32));

    nvrhi::IBuffer* boneBuffer = gpuCulling->GetGlobalBoneBuffer();
    xr_map<nvrhi::IBuffer*, nvrhi::BindingSetHandle> bindingSetCache;

    nvrhi::ComputeState state;
    state.pipeline = s_skinPipeline;

    for (const auto& sb : m_skinnedBatchData) {
        auto it = bindingSetCache.find(sb.srcVB);
        if (it == bindingSetCache.end()) {
            auto* skinRefl = RImplementation.m_shaderLoader->GetCachedReflection("rt_skin_vertices", ".cs");
            framegraph::BindingSetBuilder bsb(*skinRefl, nvDevice, "RT.SkinVertices");
            bsb.BufferSRV("g_SrcVB", sb.srcVB)
               .BufferSRV("g_BoneMatrices", boneBuffer)
               .BufferUAV("g_Output", m_skinnedOutputVB)
               .ConstantBuffer("RTSkinningCB", m_device->GetNativeBuffer(s_skinCB));
            it = bindingSetCache.emplace(sb.srcVB, nvDevice->createBindingSet(bsb.Build(), s_skinLayout)).first;
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
        tri.setIndexBuffer(m_skinnedIB)
           .setIndexFormat(nvrhi::Format::R32_UINT)
           .setIndexOffset(static_cast<uint64_t>(sb.indexOffset) * sizeof(u32))
           .setIndexCount(sb.indexCount)
           .setVertexBuffer(m_skinnedOutputVB)
           .setVertexFormat(nvrhi::Format::RGB32_FLOAT)
           .setVertexStride(SKINNED_VERTEX_STRIDE)
           .setVertexOffset(static_cast<uint64_t>(sb.vertexOffset) * SKINNED_VERTEX_STRIDE)
           .setVertexCount(sb.vertexCount);

        nvrhi::rt::GeometryDesc geom;
        geom.setTriangles(tri).setFlags(nvrhi::rt::GeometryFlags::Opaque);
        blasDesc.addBottomLevelGeometry(geom);
    }

    m_skinnedBlas = nvDevice->createAccelStruct(blasDesc);
    if (m_skinnedBlas)
        nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, m_skinnedBlas, blasDesc);

    m_batchCounts.skinned = (u32)m_skinnedBatchData.size();
    m_skinnedReady = true;

    Msg("* [RT] Built %u skinned BLAS (%u verts, %u indices)",
        (u32)m_skinnedBatchData.size(), totalVerts, totalIndices);
}

void RTAccelStructManager::InvalidateSkinned()
{
    m_skinnedBlas = nullptr;
    m_skinnedBatchData.clear();
    m_batchCounts.skinned = 0;
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

    auto grassResult = RImplementation.m_shaderLoader->LoadComputeShader("rt_grass_vertices");
    if (!grassResult.handle) {
        Msg("! [RT] Failed to load rt_grass_vertices shader");
        return;
    }

    ng::RenderDevice::BufferDesc grassCBDesc;
    grassCBDesc.debugName = "GrassRTCB";
    grassCBDesc.byteSize = sizeof(GrassRTCB);
    grassCBDesc.isConstantBuffer = true;
    grassCBDesc.isVolatile = true;
    grassCBDesc.maxVersions = ng::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
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

    auto billboardResult = RImplementation.m_shaderLoader->LoadComputeShader("rt_grass_billboard");
    if (!billboardResult.handle) {
        Msg("! [RT] Failed to load rt_grass_billboard shader");
        return;
    }

    ng::RenderDevice::BufferDesc bbCBDesc;
    bbCBDesc.debugName = "BillboardRTCB";
    bbCBDesc.byteSize = sizeof(BillboardRTCB);
    bbCBDesc.isConstantBuffer = true;
    bbCBDesc.isVolatile = true;
    bbCBDesc.maxVersions = ng::RenderDevice::BufferDesc::VOLATILE_CB_MAX_VERSIONS;
    s_billboardCB = m_device->CreateBuffer(bbCBDesc);

    s_billboardLayout = cache.GetOrCreateBindingLayoutFromReflection("RTBillboard", *billboardResult.reflection, nvDevice);

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

    if (billboardMode) {
        InitBillboardPipeline();
        if (!s_billboardInitialized) return;

        u32 maxVPB = detailMgr->maxPulledIndexCount;
        maxVPB = (maxVPB / 3) * 3;
        if (maxVPB == 0 || !detailMgr->billboardDrawArgsBuffer) { InvalidateGrass(); return; }

        u32 capacity = detailMgr->visibleBufferCapacity;
        totalVerts = capacity * maxVPB;
        totalIndices = totalVerts;
        u64 vbSize = static_cast<u64>(totalVerts) * GRASS_VERTEX_STRIDE;
        u64 ibSize = static_cast<u64>(totalIndices) * sizeof(u32);

        if (!m_grassOutputVB || m_grassOutputVB->getDesc().byteSize < vbSize) {
            nvrhi::BufferDesc desc;
            desc.debugName = "GrassOutputVB";
            desc.byteSize = vbSize;
            desc.canHaveRawViews = true;
            desc.isAccelStructBuildInput = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;
            desc.canHaveUAVs = true;
            m_grassOutputVB = nvDevice->createBuffer(desc);
        }

        if (!m_grassIB || m_grassIB->getDesc().byteSize < ibSize) {
            nvrhi::BufferDesc desc;
            desc.debugName = "GrassConsolidatedIB";
            desc.byteSize = ibSize;
            desc.canHaveRawViews = true;
            desc.isAccelStructBuildInput = true;
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::ShaderResource;
            desc.keepInitialState = true;
            m_grassIB = nvDevice->createBuffer(desc);
        }

        BillboardRTCB cb;
        cb.maxVertsPerBillboard = maxVPB;
        cb.pad[0] = cb.pad[1] = cb.pad[2] = 0;

        auto* billboardRefl = RImplementation.m_shaderLoader->GetCachedReflection("rt_grass_billboard", ".cs");
        framegraph::BindingSetBuilder bsb(*billboardRefl, nvDevice, "RT.Billboard");
        bsb.BufferSRV("g_AllInstances", detailMgr->generatedInstancesBuffer)
           .BufferSRV("g_VisibleIndices", detailMgr->visibleBillboardInstancesBuffer)
           .BufferSRV("g_DetailModels", detailMgr->detailModelsBuffer)
           .BufferSRV("g_PulledVerts", detailMgr->pulledVertexBuffer)
           .BufferSRV("g_DrawArgs", detailMgr->billboardDrawArgsBuffer)
           .BufferUAV("g_Output", m_grassOutputVB)
           .BufferUAV("g_OutputIB", m_grassIB)
           .ConstantBuffer("BillboardRTCB", m_device->GetNativeBuffer(s_billboardCB));
        auto bindingSet = nvDevice->createBindingSet(bsb.Build(), s_billboardLayout);

        cmdList->writeBuffer(m_device->GetNativeBuffer(s_billboardCB), &cb, sizeof(BillboardRTCB));
        nvrhi::ComputeState state;
        state.pipeline = s_billboardPipeline;
        state.bindings = { bindingSet };
        cmdList->setComputeState(state);
        cmdList->dispatch((capacity + 255) / 256, 1, 1);
    } else {
        InitGrassPipeline();
        if (!s_grassInitialized) return;

        u32 lodCounts[FGDetailManager::LOD_COUNT] = {
            stats.visibleLOD0Count, stats.visibleLOD1Count, stats.visibleLOD2Count
        };
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

        u64 vbSize = static_cast<u64>(totalVerts) * GRASS_VERTEX_STRIDE;
        u64 ibSize = static_cast<u64>(totalIndices) * sizeof(u32);

        if (!m_grassOutputVB || m_grassOutputVB->getDesc().byteSize < vbSize) {
            nvrhi::BufferDesc desc;
            desc.debugName = "GrassOutputVB";
            desc.byteSize = vbSize;
            desc.canHaveRawViews = true;
            desc.isAccelStructBuildInput = true;
            desc.initialState = nvrhi::ResourceStates::UnorderedAccess;
            desc.keepInitialState = true;
            desc.canHaveUAVs = true;
            m_grassOutputVB = nvDevice->createBuffer(desc);
        }

        if (!m_grassIB || m_grassIB->getDesc().byteSize < ibSize) {
            nvrhi::BufferDesc desc;
            desc.debugName = "GrassConsolidatedIB";
            desc.byteSize = ibSize;
            desc.canHaveRawViews = true;
            desc.isAccelStructBuildInput = true;
            desc.canHaveUAVs = true;
            desc.initialState = nvrhi::ResourceStates::ShaderResource;
            desc.keepInitialState = true;
            m_grassIB = nvDevice->createBuffer(desc);
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

            auto* grassRefl = RImplementation.m_shaderLoader->GetCachedReflection("rt_grass_vertices", ".cs");
            framegraph::BindingSetBuilder bsb(*grassRefl, nvDevice, "RT.Grass");
            bsb.BufferSRV("g_AllInstances", detailMgr->generatedInstancesBuffer)
               .BufferSRV("g_SlotData", detailMgr->slotDataBuffer)
               .BufferSRV("g_VisibleIndices", detailMgr->visibleInstancesBuffer[lod])
               .Texture("g_WindTexture", detailMgr->perlin4dTexture)
               .BufferUAV("g_Output", m_grassOutputVB)
               .BufferUAV("g_OutputIB", m_grassIB)
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

        Msg("* [RT] Built blade grass BLAS (%u blades, %u verts, %u indices, LODs: %u/%u/%u)",
            totalBlades, totalVerts, totalIndices, lodCounts[0], lodCounts[1], lodCounts[2]);
    }

    nvrhi::rt::AccelStructDesc blasDesc;
    blasDesc.debugName = "GrassBLAS";
    blasDesc.buildFlags = nvrhi::rt::AccelStructBuildFlags::PreferFastBuild;

    nvrhi::rt::GeometryTriangles tri;
    tri.setIndexBuffer(m_grassIB)
       .setIndexFormat(nvrhi::Format::R32_UINT)
       .setIndexOffset(0)
       .setIndexCount(totalIndices)
       .setVertexBuffer(m_grassOutputVB)
       .setVertexFormat(nvrhi::Format::RGB32_FLOAT)
       .setVertexStride(GRASS_VERTEX_STRIDE)
       .setVertexOffset(0)
       .setVertexCount(totalVerts);

    auto geomFlags = billboardMode ? nvrhi::rt::GeometryFlags::None : nvrhi::rt::GeometryFlags::Opaque;
    nvrhi::rt::GeometryDesc geom;
    geom.setTriangles(tri).setFlags(geomFlags);
    blasDesc.addBottomLevelGeometry(geom);

    if (!m_grassBlas || m_grassTotalVerts != totalVerts || m_grassTotalIndices != totalIndices)
        m_grassBlas = nvDevice->createAccelStruct(blasDesc);

    if (m_grassBlas)
        nvrhi::utils::BuildBottomLevelAccelStruct(cmdList, m_grassBlas, blasDesc);

    m_grassTotalVerts = totalVerts;
    m_grassTotalIndices = totalIndices;
    m_batchCounts.grass = 1;
    m_grassReady = true;
    m_grassBillboardMode = billboardMode;
    m_detailAtlasIndex = billboardMode ? detailMgr->buildDetailsBindlessIndex : 0;
}

}
