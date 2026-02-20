#include "stdafx.h"
#include "RTAccelStructManager.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Bindless/UnifiedVertex.h"
#include <nvrhi/utils.h>

namespace xray::render::RENDER_NAMESPACE {

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
    m_isReady = false;
    m_batchCount = 0;
    m_batchCounts = {};
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

    u32 instanceCount = (m_staticBlas ? 1 : 0) + totalInstances;
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

    m_batchCount = instancedBatchOffset;
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

}
