#include "stdafx.h"

#include "OzzKinematics.h"

#include "OzzConversion.h"
#include "OzzMesh.h"

#include "framework/mesh.h"

#include "xrCore/FS.h"
#include "xrCore/FS_impl.h"
#include "xrEngine/device.h"
#include "xrMaterialSystem/GameMtlLib.h"

#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/maths/simd_math.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace XRay::Animation
{
namespace
{
Fobb BuildFallbackObbFromShape(const SBoneShape& shape)
{
    constexpr float kMinHalfExtent = 0.001f;
    Fobb result;
    result.invalidate();

    switch (shape.type)
    {
    case SBoneShape::stBox:
    {
        result.m_rotate = shape.box.m_rotate;
        result.m_translate = shape.box.m_translate;
        result.m_halfsize = shape.box.m_halfsize;
        break;
    }
    case SBoneShape::stSphere:
    {
        result.identity();
        result.m_translate = shape.sphere.P;
        const float radius = std::max(shape.sphere.R, kMinHalfExtent);
        result.m_halfsize.set(radius, radius, radius);
        break;
    }
    case SBoneShape::stCylinder:
    {
        Fvector axis = shape.cylinder.m_direction;
        if (axis.square_magnitude() <= EPS_L)
            axis.set(0.f, 1.f, 0.f);
        axis.normalize_safe();

        Fvector up;
        up.set(0.f, 1.f, 0.f);
        if (std::fabs(axis.dotproduct(up)) > 0.99f)
            up.set(1.f, 0.f, 0.f);

        Fvector right;
        right.crossproduct(up, axis);
        if (right.square_magnitude() <= EPS_L)
        {
            right.set(1.f, 0.f, 0.f);
            right.crossproduct(up, axis);
        }
        right.normalize_safe();

        Fvector forward;
        forward.crossproduct(axis, right);
        forward.normalize_safe();

        result.m_rotate.i = right;
        result.m_rotate.j = axis;
        result.m_rotate.k = forward;
        result.m_translate = shape.cylinder.m_center;

        const float radius = std::max(shape.cylinder.m_radius, kMinHalfExtent);
        const float half_height = std::max(shape.cylinder.m_height * 0.5f, kMinHalfExtent);
        result.m_halfsize.set(radius, half_height, radius);
        break;
    }
    default:
        result.identity();
        result.m_halfsize.set(kMinHalfExtent, kMinHalfExtent, kMinHalfExtent);
        break;
    }

    result.m_halfsize.x = std::max(result.m_halfsize.x, kMinHalfExtent);
    result.m_halfsize.y = std::max(result.m_halfsize.y, kMinHalfExtent);
    result.m_halfsize.z = std::max(result.m_halfsize.z, kMinHalfExtent);
    return result;
}
}

using XRay::Animation::ConvertOzzMatrixToXRay;

OzzKinematics::OzzKinematics()
    : poseValid(false), rootBone(BI_NONE), lastUpdateTime(0), initialized(false), userData(nullptr), updateCallback(nullptr),
      updateCallbackParam(nullptr)
{
    cachedBox.invalidate();
}

OzzKinematics::~OzzKinematics()
{
    updateCallback = nullptr;
    updateCallbackParam = nullptr;
}

void OzzKinematics::Load(const char*, IReader*, u32)
{
    R_ASSERT2(false, "OzzKinematics::Load not yet implemented");
}

bool OzzKinematics::InitializeFromOzz(pcstr skeletonPath)
{
    if (!skeletonPath || !skeletonPath[0])
    {
        Msg("[OzzKinematics] InitializeFromOzz received empty skeleton path");
        return false;
    }

    ResetRuntimeState();

    ozz::io::File file(skeletonPath, "rb");
    if (!file.opened())
    {
        Msg("[OzzKinematics] Failed to open skeleton file: %s", skeletonPath);
        return false;
    }

    return LoadSkeletonFromStream(&file, skeletonPath);
}

bool OzzKinematics::InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData)
{
    ResetRuntimeState();

    if (skeletonData.empty())
    {
        Msg("[OzzKinematics] InitializeFromOzzBuffer received empty skeleton buffer");
        return false;
    }

    ozz::io::MemoryStream memoryStream;
    if (!memoryStream.Write(skeletonData.data(), skeletonData.size()))
    {
        Msg("[OzzKinematics] Failed to copy skeleton buffer into memory stream");
        ResetRuntimeState();
        return false;
    }

    memoryStream.Seek(0, ozz::io::Stream::kSet);

    if (!LoadSkeletonFromStream(&memoryStream, "memory buffer"))
    {
        ResetRuntimeState();
        return false;
    }

    return true;
}

bool OzzKinematics::LoadSkeletonFromStream(ozz::io::Stream* stream, pcstr debug_source)
{
    if (!stream)
    {
        Msg("[OzzKinematics] LoadSkeletonFromStream received null stream for %s", debug_source ? debug_source : "<unknown>");
        return false;
    }

    ozz::io::IArchive archive(stream);
    if (!archive.TestTag<ozz::animation::Skeleton>())
    {
        Msg("[OzzKinematics] Stream does not contain a skeleton: %s", debug_source ? debug_source : "<unknown>");
        return false;
    }

    archive >> skeleton;

    if (!FinalizeSkeletonInitialization(debug_source))
    {
        ResetRuntimeState();
        return false;
    }

    return true;
}

bool OzzKinematics::FinalizeSkeletonInitialization(pcstr debug_source)
{
    const int joint_count = skeleton.num_joints();
    if (joint_count <= 0)
    {
        Msg("[OzzKinematics] Skeleton contains no joints: %s", debug_source ? debug_source : "<unknown>");
        return false;
    }

    boneInstances.resize(joint_count);
    for (CBoneInstance& instance : boneInstances)
        instance.construct();

    boneBoxes.resize(joint_count);
    for (Fobb& box : boneBoxes)
        box.invalidate();

    bones.assign(joint_count, nullptr);
    boneStorage.clear();
    boneStorage.reserve(joint_count);

    modelTransforms.clear();
    modelTransforms.shrink_to_fit();
    modelTransforms.resize(static_cast<size_t>(joint_count));

    subtreeDelta.resize(static_cast<size_t>(joint_count));
    subtreeDirty.assign(static_cast<size_t>(joint_count), 0);

    boneMapByName.clear();
    boneMapByPtr.clear();
    const auto joint_names = skeleton.joint_names();
    boneMapByName.reserve(static_cast<size_t>(joint_count));
    boneMapByPtr.reserve(static_cast<size_t>(joint_count));
    for (int joint = 0; joint < joint_count; ++joint)
    {
        const size_t joint_index = static_cast<size_t>(joint);
        const char* joint_name = (joint_index < joint_names.size() && joint_names[joint_index]) ? joint_names[joint_index] : "";
        shared_str shared_name(joint_name ? joint_name : "");
        const u16 bone_id = static_cast<u16>(joint);
        boneMapByName.emplace_back(shared_name, bone_id);
        boneMapByPtr.emplace_back(shared_name, bone_id);
    }

    std::sort(boneMapByName.begin(), boneMapByName.end(),
        [](const auto& lhs, const auto& rhs)
        {
            return xr_strcmp(lhs.first.c_str(), rhs.first.c_str()) < 0;
        });

    std::sort(boneMapByPtr.begin(), boneMapByPtr.end(),
        [](const auto& lhs, const auto& rhs)
        {
            return lhs.first._get() < rhs.first._get();
        });

    rootBone = joint_count > 0 ? 0 : BI_NONE;

    {
        u64 mask;
        if (joint_count >= 64) mask = u64(-1);
        else if (joint_count == 0) mask = 0;
        else mask = (u64(1) << joint_count) - 1;
        boneVisibility.setAll(mask);
    }

    if (!BuildBoneMetadata())
        return false;

    boneOffsets.clear();
    sampledLocals.clear();
    poseValid = false;

    cachedBox.invalidate();
    lastUpdateTime = 0;

    initialized = true;

    return true;
}

bool OzzKinematics::BuildBoneMetadata()
{
    const int joint_count = skeleton.num_joints();
    if (joint_count <= 0)
        return false;

    const ozz::span<const int16_t> parents = skeleton.joint_parents();
    const auto joint_names = skeleton.joint_names();

    boneStorage.clear();
    boneStorage.reserve(static_cast<size_t>(joint_count));

    for (int joint = 0; joint < joint_count; ++joint)
    {
        const u16 bone_id = static_cast<u16>(joint);
        auto bone = xr_make_unique<CBoneData>(bone_id);

        const int16_t parent_index = (static_cast<size_t>(joint) < parents.size()) ? parents[joint] : static_cast<int16_t>(-1);
        bone->SetParentID(parent_index >= 0 ? static_cast<u16>(parent_index) : BI_NONE);

        const char* joint_name = (static_cast<size_t>(joint) < joint_names.size()) ? joint_names[joint] : nullptr;
        bone->name = joint_name && joint_name[0] ? shared_str(joint_name) : shared_str();

        bone->shape.Reset();
        bone->obb.invalidate();
        bone->game_mtl_name = shared_str();
        bone->game_mtl_idx = 0;
        bone->mass = 0.f;
        bone->center_of_mass.set(0.f, 0.f, 0.f);
        bone->IK_data.Reset();

        const ozz::math::Transform rest_pose = ozz::animation::GetJointLocalRestPose(skeleton, joint);
        const ozz::math::Float4x4 rest_matrix = ozz::math::Float4x4::FromAffine(rest_pose);
        bone->bind_transform = ConvertOzzMatrixToXRay(rest_matrix);
        bone->m2b_transform.identity();

        boneStorage.push_back(std::move(bone));
    }

    for (size_t idx = 0; idx < boneStorage.size(); ++idx)
        bones[idx] = boneStorage[idx].get();

    for (int joint = 0; joint < joint_count; ++joint)
    {
        const int16_t parent_index = (static_cast<size_t>(joint) < parents.size()) ? parents[joint] : static_cast<int16_t>(-1);
        if (parent_index >= 0 && parent_index < joint_count)
            bones[parent_index]->children.push_back(bones[joint]);
    }

    if (!bones.empty() && rootBone != BI_NONE && rootBone < bones.size())
        bones[rootBone]->CalculateM2B(Fidentity);

    return true;
}

bool OzzKinematics::ApplyExtendedBoneMetadata(const ExtendedBoneMetadataCollection& metadata)
{
    if (metadata.empty())
        return true;

    if (metadata.size() != bones.size())
    {
        Msg("[OzzKinematics] Bone metadata count mismatch (metadata=%zu, bones=%zu)", metadata.size(), bones.size());
        return false;
    }

    for (size_t idx = 0; idx < metadata.size(); ++idx)
    {
        CBoneData* bone = bones[idx];
        if (!bone)
            continue;

        const ExtendedBoneMetadata& source = metadata[idx];

        bone->shape = source.shape;
        if (_valid(source.obb))
        {
            bone->obb = source.obb;
        }
        else
        {
            bone->obb = BuildFallbackObbFromShape(source.shape);
        }
        bone->IK_data = source.joint;
        bone->mass = source.mass;
        bone->center_of_mass = source.center_of_mass;

        bone->game_mtl_name = shared_str();
        bone->game_mtl_idx = 0;
        if (!source.game_material.empty())
        {
            const char* material_name = source.game_material.c_str();
            bone->game_mtl_name = shared_str(material_name);

            if (GMLib.GetMaterial(material_name))
                bone->game_mtl_idx = GMLib.GetMaterialIdx(material_name);
            else
                Msg("[OzzKinematics] Unknown material '%s' for bone '%s'", material_name, bone->name.c_str());
        }
    }

    return true;
}

bool OzzKinematics::LoadUserDataFromBuffer(const std::vector<std::uint8_t>& buffer)
{
    userDataOwner.reset();
    userData = nullptr;

    if (buffer.empty())
        return true;

    IReader reader(const_cast<std::uint8_t*>(buffer.data()), buffer.size());

    pcstr config_path = "";
    if (FS.path_exist("$game_config$"))
    {
        const FS_Path* path = FS.get_path("$game_config$");
        if (path)
            config_path = path->m_Path;
    }

    userDataOwner = xr_make_unique<CInifile>(&reader, config_path);
    userData = userDataOwner.get();
    return userData != nullptr;
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
#ifdef DEBUG
        if (dbg_name.c_str())
            meshVisual->dbg_name = dbg_name;
#endif
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
    std::vector<std::uint8_t> skeleton_payload,
    std::vector<std::uint8_t> mesh,
    xr_vector<xr_string> motion_refs,
    ExtendedBoneMetadataCollection bone_metadata,
    std::vector<std::uint8_t> user_data,
    std::vector<std::uint8_t> embedded_anim)
{
    m_BundleSkeleton    = std::move(skeleton_payload);
    m_BundleMesh        = std::move(mesh);
    m_BundleMotionRefs  = std::move(motion_refs);
    m_BundleBoneMeta    = std::move(bone_metadata);
    m_BundleUserData    = std::move(user_data);
    m_BundleEmbeddedAnim= std::move(embedded_anim);
}

void OzzKinematics::Copy(xray::render::fg::dxRender_Visual* pFrom)
{
    xray::render::fg::FHierrarhyVisual::Copy(pFrom);

    auto* src = static_cast<OzzKinematics*>(pFrom);

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

    for (auto* child : children)
    {
        if (auto* mesh_child = dynamic_cast<xray::render::fg::OzzMesh*>(child))
            mesh_child->SetParent(this);
    }
}

void OzzKinematics::ClearPose()
{
    if (!initialized)
        return;

    sampledLocals.clear();
    poseValid = false;
    CalculateBones_Invalidate();
}

void OzzKinematics::BuildSkinningPalette(xr_vector<Fmatrix>& out_matrices, bool render_space) const
{
    if (!initialized)
    {
        out_matrices.clear();
        return;
    }

    const size_t expected = static_cast<size_t>(skeleton.num_joints());
    if (expected == 0)
    {
        out_matrices.clear();
        return;
    }

    const size_t instance_count = boneInstances.size();
    const size_t model_count = modelTransforms.size();
    const size_t available = std::min(instance_count, std::min(model_count, expected));

    out_matrices.resize(expected);

    if (available != expected)
    {
        Msg("[OzzKinematics] BuildSkinningPalette requested before bone buffers ready (instances=%zu, model=%zu, expected=%zu)", instance_count,
            model_count, expected);

        for (size_t idx = 0; idx < expected; ++idx)
            out_matrices[idx] = Fidentity;
        return;
    }

    if (render_space)
    {
        for (size_t idx = 0; idx < expected; ++idx)
            out_matrices[idx] = boneInstances[idx].mRenderTransform;
    }
    else
    {
        for (size_t idx = 0; idx < expected; ++idx)
            out_matrices[idx] = boneInstances[idx].mTransform;
    }
}

void OzzKinematics::ResetRuntimeState()
{
    boneInstances.clear();
    boneBoxes.clear();
    bones.clear();
    boneStorage.clear();
    boneMapByName.clear();
    boneMapByPtr.clear();
    subtreeDelta.clear();
    subtreeDirty.clear();
    boneOffsets.clear();
    sampledLocals.clear();
    poseValid = false;
    modelTransforms.clear();
    userDataOwner.reset();
    userData = nullptr;
    rootBone = BI_NONE;
    boneVisibility.setAll(0);
    cachedBox.invalidate();
    lastUpdateTime = 0;
    skeleton = ozz::animation::Skeleton();
    initialized = false;
}

bool OzzKinematics::IsBoneVisible(u16 bone_id) const
{
    if (bone_id >= boneInstances.size())
        return false;
    return boneVisibility.get(bone_id);
}

bool OzzKinematics::ApplyAdditionalBoneTransforms(u16 bone_id, Fmatrix& transform) const
{
    bool applied = false;
    for (const auto& offset : boneOffsets)
    {
        if (offset.m_bone_id != bone_id)
            continue;

        const Fvector original_position = transform.c;
        transform.mulB_43(offset.m_transform);
        transform.c.add(original_position, offset.m_transform.c);
        applied = true;
    }
    return applied;
}

void OzzKinematics::Bone_Calculate(CBoneData* bd, Fmatrix* parent)
{
    CalculateBones(TRUE);
}

void OzzKinematics::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 channel_mask, bool ignore_callbacks)
{
    if (!initialized || id >= LL_BoneCount())
    {
        pos.identity();
        return;
    }

    CalculateBones(TRUE);
    pos = boneInstances[id].mTransform;
}

bool OzzKinematics::PickBone(const Fmatrix& parent_xform, pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id)
{
    return false;
}

void OzzKinematics::EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id) {}

u16 OzzKinematics::LL_BoneID(LPCSTR B)
{
    if (!B)
        return BI_NONE;

    const auto it = std::lower_bound(boneMapByName.begin(), boneMapByName.end(), B,
        [](const accel::value_type& entry, pcstr value)
        {
            return xr_strcmp(entry.first.c_str(), value) < 0;
        });

    if (it == boneMapByName.end() || xr_strcmp(it->first.c_str(), B) != 0)
        return BI_NONE;
    return it->second;
}

u16 OzzKinematics::LL_BoneID(const shared_str& B)
{
    if (!B._get())
        return BI_NONE;

    const auto it = std::lower_bound(boneMapByPtr.begin(), boneMapByPtr.end(), B,
        [](const accel::value_type& entry, const shared_str& value)
        {
            return entry.first._get() < value._get();
        });

    if (it != boneMapByPtr.end() && it->first._get() == B._get())
        return it->second;

    return LL_BoneID(B.c_str());
}

LPCSTR OzzKinematics::LL_BoneName_dbg(u16 ID)
{
    if (ID >= boneInstances.size())
        return nullptr;

    const auto it = std::find_if(boneMapByName.begin(), boneMapByName.end(),
        [ID](const accel::value_type& entry)
        {
            return entry.second == ID;
        });

    return it != boneMapByName.end() ? it->first.c_str() : nullptr;
}

CBoneInstance& OzzKinematics::LL_GetBoneInstance(u16 bone_id)
{
    R_ASSERT2(bone_id < boneInstances.size(), "OzzKinematics::LL_GetBoneInstance: bone id out of range");
    return boneInstances[bone_id];
}

CBoneData& OzzKinematics::LL_GetData(u16 bone_id)
{
    R_ASSERT2(bone_id < bones.size() && bones[bone_id], "OzzKinematics::LL_GetData: bone id out of range");
    return *bones[bone_id];
}

const IBoneData& OzzKinematics::GetBoneData(u16 bone_id) const
{
    R_ASSERT2(bone_id < bones.size() && bones[bone_id], "OzzKinematics::GetBoneData: bone id out of range");
    return *bones[bone_id];
}

u16 OzzKinematics::LL_VisibleBoneCount()
{
    u16 count = 0;
    const u16 total = LL_BoneCount();
    for (u16 i = 0; i < total; ++i)
    {
        if (IsBoneVisible(i))
            ++count;
    }
    return count;
}

Fmatrix& OzzKinematics::LL_GetTransform(u16 bone_id)
{
    R_ASSERT2(bone_id < boneInstances.size(), "OzzKinematics::LL_GetTransform: bone id out of range");
    return boneInstances[bone_id].mTransform;
}

const Fmatrix& OzzKinematics::LL_GetTransform(u16 bone_id) const
{
    R_ASSERT2(bone_id < boneInstances.size(), "OzzKinematics::LL_GetTransform: bone id out of range");
    return boneInstances[bone_id].mTransform;
}

Fmatrix& OzzKinematics::LL_GetTransform_R(u16 bone_id)
{
    R_ASSERT2(bone_id < boneInstances.size(), "OzzKinematics::LL_GetTransform_R: bone id out of range");
    return boneInstances[bone_id].mRenderTransform;
}

Fobb& OzzKinematics::LL_GetBox(u16 bone_id)
{
    static Fobb stub;
    if (bone_id < boneBoxes.size())
        return boneBoxes[bone_id];
    return stub;
}

const Fbox& OzzKinematics::GetBox() const
{
    auto* self = const_cast<OzzKinematics*>(this);

    if (!initialized || boneInstances.empty())
    {
        self->cachedBox.invalidate();
        return cachedBox;
    }

    Fbox computed;
    computed.invalidate();

    for (size_t idx = 0; idx < boneInstances.size(); ++idx)
    {
        if (!IsBoneVisible(static_cast<u16>(idx)))
            continue;

        const Fmatrix& transform = boneInstances[idx].mTransform;
        computed.modify(transform.c);
    }

    if (!computed.is_valid())
        computed.set_zero();

    self->cachedBox = computed;
    return cachedBox;
}

void OzzKinematics::LL_GetBindTransform(xr_vector<Fmatrix>& matrices)
{
    matrices.clear();
    matrices.reserve(bones.size());
    for (CBoneData* bone : bones)
    {
        if (bone)
            matrices.push_back(bone->bind_transform);
    }
}

int OzzKinematics::LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups)
{
    groups.clear();
    return 0;
}

BOOL OzzKinematics::LL_GetBoneVisible(u16 bone_id)
{
    return IsBoneVisible(bone_id) ? TRUE : FALSE;
}

void OzzKinematics::LL_SetBoneVisible(u16 bone_id, BOOL val, BOOL bRecursive)
{
    if (bone_id >= boneInstances.size())
        return;

    boneVisibility.set(bone_id, val != FALSE);

    if (val != FALSE)
    {
        CalculateBones_Invalidate();
    }
    else
    {
        boneInstances[bone_id].mTransform.scale(0.f, 0.f, 0.f);
        boneInstances[bone_id].mRenderTransform.scale(0.f, 0.f, 0.f);
    }

    if (bRecursive != FALSE && bone_id < bones.size() && bones[bone_id])
    {
        for (const CBoneData* child : bones[bone_id]->children)
            LL_SetBoneVisible(child->GetSelfID(), val, bRecursive);
    }
}

u64 OzzKinematics::LL_GetBonesVisible()
{
    return boneVisibility.mask;
}

void OzzKinematics::LL_SetBonesVisible(u64 mask)
{
    boneVisibility.setAll(mask);

    const size_t count = boneInstances.size();
    for (size_t idx = 0; idx < count && idx < 64; ++idx)
    {
        const u64 bit = u64(1) << idx;
        if ((mask & bit) == 0)
        {
            boneInstances[idx].mTransform.scale(0.f, 0.f, 0.f);
            boneInstances[idx].mRenderTransform.scale(0.f, 0.f, 0.f);
        }
    }
    CalculateBones_Invalidate();
}

void OzzKinematics::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset)
{
    boneOffsets.push_back(offset);
    CalculateBones_Invalidate();
}

void OzzKinematics::LL_ClearAdditionalTransform(u16 bone_id)
{
    if (bone_id == BI_NONE)
    {
        boneOffsets.clear();
    }
    else
    {
        boneOffsets.erase(std::remove_if(boneOffsets.begin(), boneOffsets.end(),
                              [bone_id](const KinematicsABT::additional_bone_transform& entry)
                              {
                                  return entry.m_bone_id == bone_id;
                              }),
            boneOffsets.end());
    }

    CalculateBones_Invalidate();
}

void OzzKinematics::CalculateBones(BOOL)
{
    if (!initialized || skeleton.num_joints() == 0 || boneInstances.empty())
        return;

    const u32 currentTime = Device.dwTimeGlobal;
    if (currentTime == lastUpdateTime)
        return;

    OnCalculateBones();
    lastUpdateTime = currentTime;

    const size_t jointCount = static_cast<size_t>(skeleton.num_joints());

    ozz::span<const ozz::math::SoaTransform> input =
        (poseValid && !sampledLocals.empty()) ? ozz::make_span(sampledLocals) : skeleton.joint_rest_poses();

    ozz::animation::LocalToModelJob job;
    job.skeleton = &skeleton;
    job.input = input;
    job.output = ozz::span<ozz::math::Float4x4>(modelTransforms.data(), modelTransforms.size());
    if (!job.Run())
        return;

    const ozz::span<const int16_t> parents = skeleton.joint_parents();

    Fbox box;
    box.invalidate();

    for (size_t i = 0; i < jointCount; ++i)
    {
        CBoneInstance& inst = boneInstances[i];
        const Fmatrix anim = ConvertOzzMatrixToXRay(modelTransforms[i]);
        const int16_t parent = parents[i];
        const bool inherited = parent >= 0 && subtreeDirty[parent] != 0;

        bool modified = false;
        if (!IsBoneVisible(static_cast<u16>(i)))
        {
            inst.mTransform.scale(0.f, 0.f, 0.f);
            inst.mRenderTransform.scale(0.f, 0.f, 0.f);
            modified = true;
        }
        else
        {
            if (inst.callback_overwrite())
            {
                if (inst.callback())
                    inst.callback()(&inst);
                modified = true;
            }
            else
            {
                if (inherited)
                    inst.mTransform.mul_43(subtreeDelta[parent], anim);
                else
                    inst.mTransform = anim;
                modified = ApplyAdditionalBoneTransforms(static_cast<u16>(i), inst.mTransform);
                if (inst.callback())
                {
                    inst.callback()(&inst);
                    modified = true;
                }
            }

            inst.mRenderTransform.mul_43(inst.mTransform, bones[i]->m2b_transform);
            box.modify(inst.mTransform.c);
        }

        subtreeDirty[i] = (modified || inherited) ? 1 : 0;
        if (modified)
        {
            Fmatrix inverse;
            inverse.invert(anim);
            subtreeDelta[i].mul_43(inst.mTransform, inverse);
        }
        else if (inherited)
        {
            subtreeDelta[i] = subtreeDelta[parent];
        }
    }

    cachedBox = box;

    if (updateCallback)
        updateCallback(this);
}

void OzzKinematics::CalculateBones_Invalidate()
{
    lastUpdateTime = 0;
    cachedBox.invalidate();
}

#ifdef DEBUG
void OzzKinematics::DebugRender(Fmatrix& XFORM) {}

shared_str OzzKinematics::getDebugName()
{
    return shared_str("OzzKinematics");
}
#endif
}
