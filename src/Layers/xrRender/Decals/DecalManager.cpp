#include "stdafx.h"
#include "DecalManager.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/SkeletonX.h"
#include "xrCDB/Intersect.hpp"
#include "xrEngine/device.h"

namespace xray::render::RENDER_NAMESPACE::decals {

static const Fvector s_cubeVerts[CUBE_VERTEX_COUNT] = {
    { -0.5f, -0.5f, -0.5f },
    {  0.5f, -0.5f, -0.5f },
    {  0.5f,  0.5f, -0.5f },
    { -0.5f,  0.5f, -0.5f },
    { -0.5f, -0.5f,  0.5f },
    {  0.5f, -0.5f,  0.5f },
    {  0.5f,  0.5f,  0.5f },
    { -0.5f,  0.5f,  0.5f },
};

static const u16 s_cubeIndices[CUBE_INDEX_COUNT] = {
    0,1,2, 0,2,3,
    4,6,5, 4,7,6,
    0,4,5, 0,5,1,
    2,6,7, 2,7,3,
    0,3,7, 0,7,4,
    1,5,6, 1,6,2,
};

void DecalManager::Initialize(ng::RenderDevice* device)
{
    m_device = device;
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();

    CreateCubeMesh(nvDevice);

    nvrhi::BufferDesc bufDesc;
    bufDesc.byteSize = MAX_DECALS * sizeof(GPUDecalData);
    bufDesc.structStride = sizeof(GPUDecalData);
    bufDesc.debugName = "DecalInstanceBuffer";
    bufDesc.initialState = nvrhi::ResourceStates::ShaderResource;
    bufDesc.keepInitialState = true;
    m_decalBuffer = nvDevice->createBuffer(bufDesc);

    m_staticDecals.reserve(256);
    m_skeletonDecals.reserve(64);
    m_gpuData.reserve(MAX_DECALS);
}

void DecalManager::Shutdown()
{
    m_decalBuffer = nullptr;
    m_cubeVB = nullptr;
    m_cubeIB = nullptr;
    m_staticDecals.clear();
    m_skeletonDecals.clear();
    m_gpuData.clear();
}

void DecalManager::CreateCubeMesh(nvrhi::IDevice* device)
{
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = CUBE_VERTEX_COUNT * sizeof(Fvector);
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "DecalCubeVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    m_cubeVB = device->createBuffer(vbDesc);

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = CUBE_INDEX_COUNT * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.debugName = "DecalCubeIB";
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;
    m_cubeIB = device->createBuffer(ibDesc);

    nvrhi::CommandListHandle cmdList = device->createCommandList();
    cmdList->open();
    cmdList->writeBuffer(m_cubeVB, s_cubeVerts, sizeof(s_cubeVerts));
    cmdList->writeBuffer(m_cubeIB, s_cubeIndices, sizeof(s_cubeIndices));
    cmdList->close();
    device->executeCommandList(cmdList);
}

Fmatrix DecalManager::BuildOBBMatrix(const Fvector& pos, const Fvector& normal, float size, float angle)
{
    Fvector N = normal;
    N.normalize_safe();

    Fvector up = { 0, 1, 0 };
    if (_abs(N.y) > 0.99f)
        up.set(1, 0, 0);

    Fvector right;
    right.crossproduct(up, N).normalize();
    up.crossproduct(N, right).normalize();

    float sa = _sin(angle), ca = _cos(angle);
    Fvector rotRight, rotUp;
    rotRight.x = right.x * ca + up.x * sa;
    rotRight.y = right.y * ca + up.y * sa;
    rotRight.z = right.z * ca + up.z * sa;
    rotUp.x = -right.x * sa + up.x * ca;
    rotUp.y = -right.y * sa + up.y * ca;
    rotUp.z = -right.z * sa + up.z * ca;

    float depth = size * 4.0f;
    Fvector center;
    center.mad(pos, N, -depth * 0.1f);

    Fmatrix m;
    m.identity();
    m.i.set(rotRight.x * size, rotRight.y * size, rotRight.z * size);
    m._14 = 0;
    m.j.set(N.x * depth, N.y * depth, N.z * depth);
    m._24 = 0;
    m.k.set(rotUp.x * size, rotUp.y * size, rotUp.z * size);
    m._34 = 0;
    m.c.set(center);
    m._44 = 1;
    return m;
}

void DecalManager::BuildOBB(DecalInstance& decal, const Fvector& pos, const Fvector& normal, float size, float angle)
{
    decal.decalToWorld = BuildOBBMatrix(pos, normal, size, angle);
    decal.worldToDecal.invert(decal.decalToWorld);
    decal.position = decal.decalToWorld.c;
    decal.radius = size * 1.5f;
    Fvector N = normal;
    N.normalize_safe();
    decal.normal = N;
}

void DecalManager::AddStaticDecal(const Fvector& pos, const Fvector& normal, float size, u32 materialID)
{
    m_lock.Enter();

    if (m_staticDecals.size() + m_skeletonDecals.size() >= MAX_DECALS) {
        m_lock.Leave();
        return;
    }

    for (auto& existing : m_staticDecals) {
        if (existing.materialID == materialID && existing.position.similar(pos, 0.02f)) {
            float angle = ::Random.randF(deg2rad(-20.f), deg2rad(20.f));
            BuildOBB(existing, pos, normal, size, angle);
            existing.creationTime = Device.fTimeGlobal;
            existing.ttl = ps_r__WallmarkTTL;
            m_lock.Leave();
            return;
        }
    }

    DecalInstance decal;
    float angle = ::Random.randF(deg2rad(-20.f), deg2rad(20.f));
    BuildOBB(decal, pos, normal, size, angle);
    decal.materialID = materialID;
    decal.creationTime = Device.fTimeGlobal;
    decal.ttl = ps_r__WallmarkTTL;

    m_staticDecals.push_back(decal);
    m_lock.Leave();
}

void DecalManager::AddSkeletonDecal(CKinematics* parent, const Fmatrix* parentXForm,
                                     u16 boneID, const Fvector& localPos,
                                     const Fvector& localNormal, float size, u32 materialID)
{
    m_lock.Enter();

    if (m_staticDecals.size() + m_skeletonDecals.size() >= MAX_DECALS) {
        m_lock.Leave();
        return;
    }

    float angle = ::Random.randF(deg2rad(-20.f), deg2rad(20.f));
    Fmatrix newOBB = BuildOBBMatrix(localPos, localNormal, size, angle);

    for (auto& existing : m_skeletonDecals) {
        if (existing.parent == parent && existing.materialID == materialID &&
            existing.localOBB.c.similar(newOBB.c, 0.05f)) {
            existing.creationTime = Device.fTimeGlobal;
            existing.ttl = ps_r__WallmarkTTL;
            m_lock.Leave();
            return;
        }
    }

    SkeletonDecalInstance decal;
    decal.parent = parent;
    decal.parentXForm = parentXForm;
    decal.boneID = boneID;
    decal.localOBB = newOBB;
    decal.size = size;
    decal.materialID = materialID;
    decal.creationTime = Device.fTimeGlobal;
    decal.ttl = ps_r__WallmarkTTL;

    m_skeletonDecals.push_back(decal);
    m_lock.Leave();
}

void DecalManager::AddSkeletonDecalFromRay(const Fmatrix* xf, CKinematics* obj,
                                            const Fvector& start, const Fvector& dir,
                                            float size, u32 materialID)
{
    VERIFY(obj && xf && (size > EPS_L));

    Fmatrix P;
    P.invert(*xf);
    Fvector S, D;
    P.transform_tiny(S, start);
    P.transform_dir(D, dir);

    float dist = flt_max;
    BOOL picked = FALSE;
    Fvector normal = {0, 0, 0};
    u16 hitBone = u16(-1);

    for (u16 k = 0; k < obj->LL_BoneCount(); k++) {
        CBoneData& BD = obj->LL_GetData(k);
        if (!obj->LL_GetBoneVisible(k) || BD.shape.flags.is(SBoneShape::sfNoPickable))
            continue;
        Fobb obb;
        obb.transform(BD.obb, obj->LL_GetBoneInstance(k).mTransform);
        if (!CDB::TestRayOBB(S, D, obb))
            continue;
        IKinematics::pick_result r;
        r.normal = normal;
        r.dist = dist;
        for (u32 i = 0; i < obj->children.size(); i++) {
            auto* skelChild = dynamic_cast<CSkeletonX*>(obj->children[i]);
            if (skelChild && skelChild->PickBone(r, dist, S, D, k)) {
                picked = TRUE;
                dist = r.dist;
                normal = r.normal;
                hitBone = k;
            }
        }
    }

    if (!picked || hitBone == u16(-1)) {
        Msg("[Decal] SKELETON ray missed all bones (tested %u bones)", obj->LL_BoneCount());
        return;
    }

    Msg("[Decal] SKELETON ray HIT bone %u at dist=%.3f", hitBone, dist);

    Fvector cp;
    cp.mad(S, D, dist);

    Fmatrix boneInv;
    boneInv.invert(obj->LL_GetBoneInstance(hitBone).mTransform);
    Fvector localPos, localNormal;
    boneInv.transform_tiny(localPos, cp);
    boneInv.transform_dir(localNormal, normal);
    localNormal.normalize_safe();

    AddSkeletonDecal(obj, xf, hitBone, localPos, localNormal, size, materialID);
}

void DecalManager::UpdateSkeletonDecals(float currentTime)
{
    for (auto it = m_skeletonDecals.begin(); it != m_skeletonDecals.end(); ) {
        auto& sd = *it;
        float age = currentTime - sd.creationTime;
        if (age >= sd.ttl || !sd.parent) {
            it = m_skeletonDecals.erase(it);
            continue;
        }

        const auto& boneInst = sd.parent->LL_GetBoneInstance(sd.boneID);
        Fmatrix boneWorld;
        boneWorld.mul_43(*sd.parentXForm, boneInst.mTransform);

        Fmatrix decalToWorld;
        decalToWorld.mul_43(boneWorld, sd.localOBB);

        Fvector worldPos = decalToWorld.c;

        float opacity = 1.0f - (age / sd.ttl);
        if (Device.vCameraPosition.distance_to_sqr(worldPos) <= _sqr(50.f)) {
            GPUDecalData data;
            data.decalToWorld = decalToWorld;
            data.worldToDecal.invert(decalToWorld);
            data.materialID = sd.materialID;
            data.opacity = opacity;
            data.normalThreshold = DEFAULT_NORMAL_THRESHOLD;
            data.flags = DECAL_FLAG_SKELETON;
            m_gpuData.push_back(data);
        }

        ++it;
    }
}

void DecalManager::Update(float dt, float currentTime)
{
    m_lock.Enter();

    for (auto it = m_staticDecals.begin(); it != m_staticDecals.end(); ) {
        float age = currentTime - it->creationTime;
        if (age >= it->ttl) {
            it = m_staticDecals.erase(it);
        } else {
            ++it;
        }
    }

    m_gpuData.clear();

    for (auto& decal : m_staticDecals) {
        if (Device.vCameraPosition.distance_to_sqr(decal.position) > _sqr(100.f))
            continue;

        float age = currentTime - decal.creationTime;
        float opacity = 1.0f - (age / decal.ttl);

        GPUDecalData data;
        data.worldToDecal = decal.worldToDecal;
        data.decalToWorld = decal.decalToWorld;
        data.materialID = decal.materialID;
        data.opacity = opacity;
        data.normalThreshold = DEFAULT_NORMAL_THRESHOLD;
        data.flags = DECAL_FLAG_STATIC;
        m_gpuData.push_back(data);
    }

    UpdateSkeletonDecals(currentTime);

    m_gpuDecalCount = (u32)m_gpuData.size();

    m_lock.Leave();
}

void DecalManager::Upload(ng::RenderContext* ctx)
{
    if (m_gpuDecalCount == 0 || !m_decalBuffer)
        return;

    nvrhi::ICommandList* cmdList = ctx->GetCommandList();
    cmdList->writeBuffer(m_decalBuffer, m_gpuData.data(), m_gpuDecalCount * sizeof(GPUDecalData));
}

} // namespace xray::render::RENDER_NAMESPACE::decals
