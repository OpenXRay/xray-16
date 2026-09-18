#include "stdafx.h"
#include "OzzKinematics.h"
#include "OzzConversion.h"
#include "OzzMesh.h"
#include "Layers/xrRender/ECS/Components.h"
#include "xrAnimation/Components.hpp"
#include "xrECS/App.hpp"
#include "xrECS/Components.hpp"
#include "xrECS/Hierarchy.hpp"
#include "xrEngine/device.h"

#include "framework/mesh.h"

#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include <memory>

namespace XRay::Animation
{
OzzKinematics::OzzKinematics() : stubBoneData(u16(-1))
{
    stubBoneInstance.construct();
    core.SetOwner(this);
}

OzzKinematics::~OzzKinematics()
{
    auto& reg = xray::ecs::Reg();
    if (m_skeleton_entity != entt::null && reg.valid(m_skeleton_entity))
        reg.destroy(m_skeleton_entity);
    m_skeleton_entity = entt::null;
}

void OzzKinematics::Load(const char*, IReader*, u32)
{
    R_ASSERT2(false, "OzzKinematics::Load not yet implemented");
}

bool OzzKinematics::LoadMeshFromBuffer(const std::vector<std::uint8_t>& meshData)
{
    if (meshData.empty())
        return false;

    ozz::io::MemoryStream stream;
    stream.Write(meshData.data(), static_cast<size_t>(meshData.size()));
    stream.Seek(0, ozz::io::Stream::kSet);

    ozz::io::IArchive archive(&stream);
    bool any_loaded = false;
    Fbox merged_box;
    merged_box.invalidate();

    while (archive.TestTag<ozz::sample::Mesh>())
    {
        ozz::sample::Mesh mesh;
        archive >> mesh;

        if (mesh.vertex_count() <= 0 || mesh.triangle_index_count() <= 0)
            continue;

        auto* meshVisual = xr_new<xray::render::fg::OzzMesh>();
        meshVisual->LoadFromOzzMesh(this, mesh);
        if (dbg_name.c_str())
            meshVisual->dbg_name = dbg_name;
        children.push_back(meshVisual);
        merged_box.merge(meshVisual->vis.box);
        any_loaded = true;
    }

    if (any_loaded)
    {
        vis.box.set(merged_box);
        Fvector center; center.add(merged_box.vMin, merged_box.vMax); center.mul(0.5f);
        Fvector extent; extent.sub(merged_box.vMax, merged_box.vMin); extent.mul(0.5f);
        vis.sphere.P = center;
        vis.sphere.R = extent.magnitude();
    }
    return any_loaded;
}

void OzzKinematics::CacheBundlePayloads(
    std::vector<std::uint8_t> skeleton,
    std::vector<std::uint8_t> mesh,
    xr_vector<xr_string> motion_refs,
    ExtendedBoneMetadataCollection bone_metadata,
    std::vector<std::uint8_t> user_data,
    std::vector<std::uint8_t> embedded_anim)
{
    m_BundleSkeleton    = std::move(skeleton);
    m_BundleMesh        = std::move(mesh);
    m_BundleMotionRefs  = std::move(motion_refs);
    m_BundleBoneMeta    = std::move(bone_metadata);
    m_BundleUserData    = std::move(user_data);
    m_BundleEmbeddedAnim= std::move(embedded_anim);
}

void OzzKinematics::OnSkeletonLoaded()
{
    if (!core.IsInitialized())
        return;

    auto& reg = xray::ecs::Reg();

    if (m_skeleton_entity == entt::null)
        m_skeleton_entity = reg.create();

    auto& vis = reg.get_or_emplace<xray::ecs::CBoneVisibility>(m_skeleton_entity);
    core.BindBoneVisibility(&vis);

    auto& bufs = reg.get_or_emplace<xray::ecs::AnimationBuffers>(m_skeleton_entity);
    const int num_joints     = core.Skeleton().num_joints();
    const int num_soa_joints = core.Skeleton().num_soa_joints();

    bufs.locals.resize(static_cast<std::size_t>(num_soa_joints));
    bufs.models.resize(static_cast<std::size_t>(num_joints));
    bufs.context.Resize(num_joints);

    for (int i = 0; i < num_soa_joints; ++i)
        bufs.locals[i] = core.Skeleton().joint_rest_poses()[i];

    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &core.Skeleton();
    ltm_job.input    = ozz::make_span(bufs.locals);
    ltm_job.output   = ozz::make_span(bufs.models);
    R_ASSERT2(ltm_job.Run(), "ozz LocalToModelJob (bind-pose) failed");
}

void OzzKinematics::Copy(xray::render::fg::dxRender_Visual* pFrom)
{
    xray::render::fg::FHierrarhyVisual::Copy(pFrom);

    auto* src = dynamic_cast<OzzKinematics*>(pFrom);
    R_ASSERT2(src, "OzzKinematics::Copy: source is not OzzKinematics");

    m_BundleSkeleton     = src->m_BundleSkeleton;
    m_BundleMesh         = src->m_BundleMesh;
    m_BundleMotionRefs   = src->m_BundleMotionRefs;
    m_BundleBoneMeta     = src->m_BundleBoneMeta;
    m_BundleUserData     = src->m_BundleUserData;
    m_BundleEmbeddedAnim = src->m_BundleEmbeddedAnim;

    if (m_BundleSkeleton.empty())
        return;

    const auto skeleton_span = ozz::span<const std::byte>(
        reinterpret_cast<const std::byte*>(m_BundleSkeleton.data()),
        m_BundleSkeleton.size());

    R_ASSERT2(InitializeFromOzzBuffer(skeleton_span),
        "OzzKinematics::Copy: re-init from cached skeleton failed");

    if (!m_BundleBoneMeta.empty())
        ApplyExtendedBoneMetadata(m_BundleBoneMeta);

    if (!m_BundleUserData.empty())
        LoadUserDataFromBuffer(m_BundleUserData);

    OnSkeletonLoaded();

    auto& reg = xray::ecs::Reg();
    for (auto* child : children)
    {
        if (auto* mesh_child = dynamic_cast<xray::render::fg::OzzMesh*>(child))
        {
            mesh_child->SetParent(this);
            const entt::entity mesh_entity = mesh_child->GetEntity();
            if (mesh_entity != entt::null && reg.valid(mesh_entity))
            {
                if (auto* skinning = reg.try_get<xray::ecs::COzzMeshSkinning>(mesh_entity))
                {
                    skinning->skeleton_owner = m_skeleton_entity;
                    skinning->palette_staging.clear();
                }
                if (m_skeleton_entity != entt::null)
                    xray::ecs::SetParent(mesh_entity, m_skeleton_entity);
            }
        }
    }
}

bool OzzKinematics::InitializeFromOzz(pcstr skeletonPath)
{
    bool result = core.InitializeFromOzz(skeletonPath);
    if (result)
        OnSkeletonLoaded();
    return result;
}

bool OzzKinematics::InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData)
{
    bool result = core.InitializeFromOzzBuffer(skeletonData);
    if (result)
        OnSkeletonLoaded();
    return result;
}

bool OzzKinematics::ApplyExtendedBoneMetadata(const ExtendedBoneMetadataCollection& metadata)
{
    return core.ApplyExtendedBoneMetadata(metadata);
}

bool OzzKinematics::LoadUserDataFromBuffer(const std::vector<std::uint8_t>& buffer)
{
    return core.LoadUserDataFromBuffer(buffer);
}

bool OzzKinematics::SetPoseLocals(ozz::span<const ozz::math::SoaTransform> locals)
{
    return core.SetPoseLocals(locals);
}

void OzzKinematics::ClearPose()
{
    core.ClearPose();
}

void OzzKinematics::BuildSkinningPalette(xr_vector<Fmatrix>& out_matrices, bool render_space) const
{
    core.BuildSkinningPalette(out_matrices, render_space);
}

void OzzKinematics::Bone_Calculate(CBoneData* bd, Fmatrix* parent)
{
    core.CalculateTransforms(TRUE);
}

void OzzKinematics::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 channel_mask, bool ignore_callbacks)
{
    if (!core.IsInitialized() || id >= core.GetBoneCount())
    {
        pos.identity();
        return;
    }

    core.CalculateTransforms(TRUE);
    pos = core.GetBoneTransform(id);
}

bool OzzKinematics::PickBone(const Fmatrix& parent_xform, pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id)
{
    return false;
}

void OzzKinematics::EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id) {}

u16 OzzKinematics::LL_BoneID(LPCSTR B)
{
    return core.FindBoneID(B);
}

u16 OzzKinematics::LL_BoneID(const shared_str& B)
{
    return core.FindBoneID(B);
}

LPCSTR OzzKinematics::LL_BoneName_dbg(u16 ID)
{
    return core.GetBoneName(ID);
}

CInifile* OzzKinematics::LL_UserData()
{
    return core.GetUserData();
}

IKinematics::accel* OzzKinematics::LL_Bones()
{
    return core.GetBoneMapByName();
}

CBoneInstance& OzzKinematics::LL_GetBoneInstance(u16 bone_id)
{
    OzzKinematicsCore::BoneInfo info;
    if (core.GetBoneInfo(bone_id, info) && info.instance)
        return *info.instance;

    NotImplemented(__FUNCTION__);
    return StubBoneInstance();
}

CBoneData& OzzKinematics::LL_GetData(u16 bone_id)
{
    OzzKinematicsCore::BoneInfo info;
    if (core.GetBoneInfo(bone_id, info) && info.data)
        return *info.data;

    return StubBoneData();
}

const IBoneData& OzzKinematics::GetBoneData(u16 bone_id) const
{
    OzzKinematicsCore::BoneInfo info;
    if (core.GetBoneInfo(bone_id, info) && info.data)
        return *info.data;

    return StubBoneData();
}

u16 OzzKinematics::LL_BoneCount() const
{
    return core.GetBoneCount();
}

u16 OzzKinematics::LL_VisibleBoneCount()
{
    u16 count = 0;
    const u16 total = core.GetBoneCount();
    for (u16 i = 0; i < total; ++i)
    {
        if (core.IsBoneVisible(i))
            ++count;
    }
    return count;
}

Fmatrix& OzzKinematics::LL_GetTransform(u16 bone_id)
{
    if (bone_id < core.GetBoneCount())
        return const_cast<Fmatrix&>(core.GetBoneTransform(bone_id));

    NotImplemented(__FUNCTION__);
    return StubMatrix();
}

const Fmatrix& OzzKinematics::LL_GetTransform(u16 bone_id) const
{
    if (bone_id < core.GetBoneCount())
        return core.GetBoneTransform(bone_id);

    const_cast<OzzKinematics*>(this)->NotImplemented(__FUNCTION__);
    return StubMatrix();
}

Fmatrix& OzzKinematics::LL_GetTransform_R(u16 bone_id)
{
    if (bone_id < core.GetBoneCount())
        return const_cast<Fmatrix&>(core.GetBoneRenderTransform(bone_id));

    NotImplemented(__FUNCTION__);
    return StubMatrix();
}

Fobb& OzzKinematics::LL_GetBox(u16 bone_id)
{
    return core.GetBoneBox(bone_id);
}

const Fbox& OzzKinematics::GetBox() const
{
    return core.GetBoundingBox();
}

void OzzKinematics::LL_GetBindTransform(xr_vector<Fmatrix>& matrices)
{
    core.GetBindTransforms(matrices);
}

int OzzKinematics::LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups)
{
    groups.clear();
    return 0;
}

u16 OzzKinematics::LL_GetBoneRoot()
{
    return core.GetRootBone();
}

void OzzKinematics::LL_SetBoneRoot(u16 bone_id)
{
    core.SetRootBone(bone_id);
}

BOOL OzzKinematics::LL_GetBoneVisible(u16 bone_id)
{
    return core.IsBoneVisible(bone_id) ? TRUE : FALSE;
}

void OzzKinematics::LL_SetBoneVisible(u16 bone_id, BOOL val, BOOL bRecursive)
{
    core.SetBoneVisible(bone_id, val != FALSE, bRecursive != FALSE);
}

u64 OzzKinematics::LL_GetBonesVisible()
{
    return core.GetVisibilityMask();
}

void OzzKinematics::LL_SetBonesVisible(u64 mask)
{
    core.SetVisibilityMask(mask);
}

void OzzKinematics::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset)
{
    core.AddBoneTransform(offset);
}

void OzzKinematics::LL_ClearAdditionalTransform(u16 bone_id)
{
    core.ClearBoneTransform(bone_id);
}

void OzzKinematics::CalculateBones(BOOL)
{
}

void OzzKinematics::CalculateBones_Invalidate()
{
}

void OzzKinematics::CalculateBonesFG(BOOL bForceExact)
{
    core.CalculateTransforms(bForceExact);
}

void OzzKinematics::CalculateBones_InvalidateFG()
{
    core.InvalidateCache();
}

void OzzKinematics::Callback(UpdateCallback C, void* Param)
{
    core.SetUpdateCallback(C, Param);
}

void OzzKinematics::SetUpdateCallback(UpdateCallback pCallback)
{
    core.SetUpdateCallback(pCallback, core.GetUpdateCallbackParam());
}

void OzzKinematics::SetUpdateCallbackParam(void* pCallbackParam)
{
    core.SetUpdateCallback(core.GetUpdateCallback(), pCallbackParam);
}

UpdateCallback OzzKinematics::GetUpdateCallback()
{
    return core.GetUpdateCallback();
}

void* OzzKinematics::GetUpdateCallbackParam()
{
    return core.GetUpdateCallbackParam();
}

#ifdef DEBUG
void OzzKinematics::DebugRender(Fmatrix& XFORM)
{
    NotImplemented(__FUNCTION__);
}

shared_str OzzKinematics::getDebugName()
{
    return shared_str("OzzKinematics");
}
#endif

void OzzKinematics::NotImplemented(pcstr function_name) const
{
    Msg("[OzzKinematics] %s is not implemented for static models", function_name);
}

CBoneInstance& OzzKinematics::StubBoneInstance() const
{
    return stubBoneInstance;
}

CBoneData& OzzKinematics::StubBoneData() const
{
    return stubBoneData;
}

Fmatrix& OzzKinematics::StubMatrix() const
{
    return stubMatrix;
}

Fobb& OzzKinematics::StubObb() const
{
    return stubObb;
}
} // namespace XRay::Animation
