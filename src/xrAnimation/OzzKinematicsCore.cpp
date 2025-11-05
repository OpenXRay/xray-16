#include "stdafx.h"

#include "OzzKinematicsCore.h"

#include "OzzConversion.h"
#include "xrCore/FS.h"
#include "xrCore/FS_impl.h"
#include "xrEngine/device.h"
#include "xrMaterialSystem/GameMtlLib.h"

#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/maths/simd_math.h"

#include <algorithm>
#include <cmath>

namespace XRay
{
namespace Animation
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
} // namespace

OzzKinematicsCore::OzzKinematicsCore()
    : userData(nullptr), rootBone(BI_NONE), visibleMask(0), updateCallback(nullptr), updateCallbackParam(nullptr), ownerKinematics(nullptr),
      ownerVisual(nullptr), lastUpdateTime(0), visibilityCounter(0), initialized(false)
{
    cachedBox.invalidate();
}

OzzKinematicsCore::~OzzKinematicsCore()
{
    SetOwner(nullptr);
    SetVisualOwner(nullptr);
    updateCallback = nullptr;
    updateCallbackParam = nullptr;
    ownerKinematics = nullptr;
}

bool OzzKinematicsCore::InitializeFromOzz(pcstr skeletonPath)
{
    if (!skeletonPath || !skeletonPath[0])
    {
        Msg("[OzzKinematicsCore] InitializeFromOzz received empty skeleton path");
        return false;
    }

    ResetRuntimeState();

    ozz::io::File file(skeletonPath, "rb");
    if (!file.opened())
    {
        Msg("[OzzKinematicsCore] Failed to open skeleton file: %s", skeletonPath);
        return false;
    }

    return LoadSkeletonFromStream(&file, skeletonPath);
}

bool OzzKinematicsCore::InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData)
{
    ResetRuntimeState();

    if (skeletonData.empty())
    {
        Msg("[OzzKinematicsCore] InitializeFromOzzBuffer received empty skeleton buffer");
        return false;
    }

    ozz::io::MemoryStream memoryStream;
    if (!memoryStream.Write(skeletonData.data(), skeletonData.size()))
    {
        Msg("[OzzKinematicsCore] Failed to copy skeleton buffer into memory stream");
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

bool OzzKinematicsCore::LoadUserDataFromBuffer(const std::vector<std::uint8_t>& buffer)
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

bool OzzKinematicsCore::LoadSkeletonFromStream(ozz::io::Stream* stream, pcstr debug_source)
{
    if (!stream)
    {
        Msg("[OzzKinematicsCore] LoadSkeletonFromStream received null stream for %s", debug_source ? debug_source : "<unknown>");
        return false;
    }

    ozz::io::IArchive archive(stream);
    if (!archive.TestTag<ozz::animation::Skeleton>())
    {
        Msg("[OzzKinematicsCore] Stream does not contain a skeleton: %s", debug_source ? debug_source : "<unknown>");
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

bool OzzKinematicsCore::FinalizeSkeletonInitialization(pcstr debug_source)
{
    const int joint_count = skeleton.num_joints();
    if (joint_count <= 0)
    {
        Msg("[OzzKinematicsCore] Skeleton contains no joints: %s", debug_source ? debug_source : "<unknown>");
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

    cachedTransformsPreCallbacks.clear();
    cachedTransformsPreCallbacks.resize(static_cast<size_t>(joint_count));

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

    if (joint_count >= 64)
        visibleMask = u64(-1);
    else if (joint_count == 0)
        visibleMask = 0;
    else
        visibleMask = (u64(1) << joint_count) - 1;

    if (!BuildBoneMetadata())
        return false;

    boneOffsets.clear();
    sampledLocals.clear();
    samplingContext.Resize(0);

    cachedBox.invalidate();
    lastUpdateTime = 0;
    visibilityCounter = 0;

    initialized = true;

    OnSkeletonLoaded();

    return true;
}

using XRay::Animation::ConvertOzzMatrixToXRay;

bool OzzKinematicsCore::BuildBoneMetadata()
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

bool OzzKinematicsCore::ApplyExtendedBoneMetadata(const ExtendedBoneMetadataCollection& metadata)
{
    if (metadata.empty())
        return true;

    if (metadata.size() != bones.size())
    {
        Msg("[OzzKinematicsCore] Bone metadata count mismatch (metadata=%zu, bones=%zu)", metadata.size(), bones.size());
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
        bone->rest_length = source.rest_length;
        bone->dominant_axis = source.dominant_axis;
        bone->local_aabb_min = source.local_aabb_min;
        bone->local_aabb_max = source.local_aabb_max;
        bone->inverse_global_transform = source.inverse_global_transform;
        bone->inertia_tensor = source.inertia_tensor;
        bone->volume = source.volume;
        bone->collision_layers.assign(source.collision_layers);
        bone->ground_contact_candidate = source.ground_contact_candidate;
        bone->weapon_anchor_candidate = source.weapon_anchor_candidate;

        bone->game_mtl_name = shared_str();
        bone->game_mtl_idx = 0;
        if (!source.game_material.empty())
        {
            const char* material_name = source.game_material.c_str();
            bone->game_mtl_name = shared_str(material_name);

            if (GMLib.GetMaterial(material_name))
                bone->game_mtl_idx = GMLib.GetMaterialIdx(material_name);
            else
                Msg("[OzzKinematicsCore] Unknown material '%s' for bone '%s'", material_name, bone->name.c_str());
        }
    }

    return true;
}

bool OzzKinematicsCore::SetPoseLocals(ozz::span<const ozz::math::SoaTransform> locals)
{
    if (!initialized)
        return false;

    if (locals.empty())
    {
        sampledLocals.clear();
        InvalidateCache();
        return true;
    }

    if (static_cast<int>(locals.size()) != skeleton.num_soa_joints())
    {
        Msg("[OzzKinematicsCore] SetPoseLocals received %zu soa joints, expected %d", locals.size(), skeleton.num_soa_joints());
        return false;
    }

    sampledLocals.assign(locals.begin(), locals.end());
    InvalidateCache();
    return true;
}

void OzzKinematicsCore::ClearPose()
{
    if (!initialized)
        return;

    sampledLocals.clear();
    InvalidateCache();
}

void OzzKinematicsCore::BuildSkinningPalette(xr_vector<Fmatrix>& out_matrices, bool render_space) const
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
        Msg("[OzzKinematicsCore] BuildSkinningPalette requested before bone buffers ready (instances=%zu, model=%zu, expected=%zu)", instance_count,
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

void OzzKinematicsCore::ResetRuntimeState()
{
    boneInstances.clear();
    boneBoxes.clear();
    bones.clear();
    boneStorage.clear();
    boneMapByName.clear();
    boneMapByPtr.clear();
    cachedTransformsPreCallbacks.clear();
    boneOffsets.clear();
    sampledLocals.clear();
    modelTransforms.clear();
    samplingContext.Resize(0);
    userDataOwner.reset();
    userData = nullptr;
    rootBone = BI_NONE;
    visibleMask = 0;
    cachedBox.invalidate();
    lastUpdateTime = 0;
    visibilityCounter = 0;
    skeleton = ozz::animation::Skeleton();
    initialized = false;

    OnResetRuntime();
}

bool OzzKinematicsCore::IsBoneVisible(u16 bone_id) const
{
    if (bone_id >= boneInstances.size())
        return false;
    if (bone_id >= 64)
        return true;
    const u64 mask = u64(1) << bone_id;
    return (visibleMask & mask) != 0;
}

void OzzKinematicsCore::ApplyAdditionalBoneTransforms(u16 bone_id, Fmatrix& transform) const
{
    for (const auto& offset : boneOffsets)
    {
        if (offset.m_bone_id != bone_id)
            continue;

        const Fvector original_position = transform.c;
        transform.mulB_43(offset.m_transform);
        transform.c.add(original_position, offset.m_transform.c);
    }
}

u16 OzzKinematicsCore::GetBoneCount() const
{
    return static_cast<u16>(boneInstances.size());
}

void OzzKinematicsCore::SetRootBone(u16 bone_id)
{
    rootBone = bone_id;
}

void OzzKinematicsCore::SetBoneVisible(u16 bone_id, bool visible, bool /*recursive*/)
{
    if (bone_id < 64)
    {
        const u64 mask = u64(1) << bone_id;
        if (visible)
            visibleMask |= mask;
        else
            visibleMask &= ~mask;
    }

    if (bone_id < boneInstances.size())
    {
        if (visible)
        {
            InvalidateCache();
        }
        else
        {
            boneInstances[bone_id].mTransform.scale(0.f, 0.f, 0.f);
            boneInstances[bone_id].mRenderTransform.scale(0.f, 0.f, 0.f);
            if (bone_id < cachedTransformsPreCallbacks.size())
                cachedTransformsPreCallbacks[bone_id].scale(0.f, 0.f, 0.f);
        }
    }
}

void OzzKinematicsCore::SetVisibilityMask(u64 mask)
{
    visibleMask = mask;

    const size_t count = boneInstances.size();
    for (size_t idx = 0; idx < count && idx < 64; ++idx)
    {
        const u64 bit = u64(1) << idx;
        if ((visibleMask & bit) == 0)
        {
            boneInstances[idx].mTransform.scale(0.f, 0.f, 0.f);
            boneInstances[idx].mRenderTransform.scale(0.f, 0.f, 0.f);
            if (idx < cachedTransformsPreCallbacks.size())
                cachedTransformsPreCallbacks[idx].scale(0.f, 0.f, 0.f);
        }
    }
    InvalidateCache();
}

void OzzKinematicsCore::AddBoneTransform(const KinematicsABT::additional_bone_transform& transform)
{
    boneOffsets.push_back(transform);
    InvalidateCache();
}

void OzzKinematicsCore::ClearBoneTransform(u16 bone_id)
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

    InvalidateCache();
}

void OzzKinematicsCore::CalculateTransforms(bool force_exact)
{
    if (!initialized)
        return;

    if (skeleton.num_joints() == 0 || boneInstances.empty())
        return;

    const u32 current_time = Device.dwTimeGlobal;
    if (!force_exact && current_time == lastUpdateTime)
        return;
    if (!force_exact && lastUpdateTime != 0 && current_time < lastUpdateTime + UCalc_Interval)
        return;

    const size_t expected_joint_count = static_cast<size_t>(skeleton.num_joints());
    if (boneInstances.size() != expected_joint_count)
    {
        Msg("[OzzKinematicsCore] Bone instance array size mismatch (have %zu, expected %zu). Reinitializing.", boneInstances.size(), expected_joint_count);
        boneInstances.resize(expected_joint_count);
        for (CBoneInstance& instance : boneInstances)
            instance.construct();
    }

    if (modelTransforms.size() != expected_joint_count)
    {
        modelTransforms.resize(expected_joint_count);
        std::fill(modelTransforms.begin(), modelTransforms.end(), ozz::math::Float4x4::identity());
    }
    if (cachedTransformsPreCallbacks.size() != expected_joint_count)
    {
        cachedTransformsPreCallbacks.resize(expected_joint_count);
        std::fill(cachedTransformsPreCallbacks.begin(), cachedTransformsPreCallbacks.end(), Fidentity);
    }

    ozz::animation::LocalToModelJob job;
    job.skeleton = &skeleton;
    if (!sampledLocals.empty() && sampledLocals.size() == static_cast<size_t>(skeleton.num_soa_joints()))
        job.input = ozz::span<const ozz::math::SoaTransform>(sampledLocals.data(), sampledLocals.size());
    else
        job.input = skeleton.joint_rest_poses();

    job.output = ozz::span<ozz::math::Float4x4>(modelTransforms.data(), modelTransforms.size());

    if (!job.Run())
    {
        Msg("[OzzKinematicsCore] LocalToModelJob failed during CalculateTransforms");
        return;
    }

    const size_t bone_count = boneInstances.size();
    Fbox box;
    box.invalidate();

    for (size_t i = 0; i < bone_count; ++i)
    {
        CBoneInstance& instance = boneInstances[i];
        const u16 bone_id = static_cast<u16>(i);
        const bool visible = IsBoneVisible(bone_id);

        Fmatrix transform = ConvertOzzMatrixToXRay(modelTransforms[i]);
        ApplyAdditionalBoneTransforms(bone_id, transform);

        if (!visible)
        {
            instance.mTransform.scale(0.f, 0.f, 0.f);
            instance.mRenderTransform.scale(0.f, 0.f, 0.f);
            if (i < cachedTransformsPreCallbacks.size())
                cachedTransformsPreCallbacks[i].scale(0.f, 0.f, 0.f);
            continue;
        }

        cachedTransformsPreCallbacks[i] = transform;

        const bool has_callback = instance.callback() != nullptr;
        if (!instance.callback_overwrite() || !has_callback)
            instance.mTransform = transform;

        if (has_callback)
            instance.callback()(&instance);
        else if (instance.callback_overwrite())
            instance.mTransform = transform;

        if (i < bones.size() && bones[i])
            instance.mRenderTransform.mul_43(instance.mTransform, bones[i]->m2b_transform);
        else
            instance.mRenderTransform = instance.mTransform;

        box.modify(instance.mTransform.c);
    }

    cachedBox = box;
    lastUpdateTime = current_time;

    if (updateCallback && ownerKinematics)
        updateCallback(ownerKinematics);

    OnCalculateBones();
}

void OzzKinematicsCore::InvalidateCache()
{
    lastUpdateTime = 0;
    visibilityCounter = 0;
    cachedBox.invalidate();
}

void OzzKinematicsCore::SetUpdateCallback(UpdateCallback callback, void* param)
{
    updateCallback = callback;
    updateCallbackParam = param;
}

bool OzzKinematicsCore::GetBoneInfo(u16 bone_id, BoneInfo& info) const
{
    if (bone_id >= boneInstances.size())
        return false;

    info.id = bone_id;
    info.data = bone_id < bones.size() ? bones[bone_id] : nullptr;
    info.instance = const_cast<CBoneInstance*>(&boneInstances[bone_id]);
    info.parent_id = info.data ? info.data->GetParentID() : BI_NONE;
    info.name = info.data ? info.data->name : shared_str();

    return true;
}

u16 OzzKinematicsCore::FindBoneID(pcstr name) const
{
    if (!name)
        return BI_NONE;

    const auto it = std::lower_bound(boneMapByName.begin(), boneMapByName.end(), name,
        [](const IKinematics::accel::value_type& entry, pcstr value)
        {
            return xr_strcmp(entry.first.c_str(), value) < 0;
        });

    if (it == boneMapByName.end())
        return BI_NONE;

    return xr_strcmp(it->first.c_str(), name) == 0 ? it->second : BI_NONE;
}

u16 OzzKinematicsCore::FindBoneID(const shared_str& name) const
{
    if (!name._get())
        return BI_NONE;

    const auto it = std::lower_bound(boneMapByPtr.begin(), boneMapByPtr.end(), name,
        [](const IKinematics::accel::value_type& entry, const shared_str& value)
        {
            return entry.first._get() < value._get();
        });

    if (it == boneMapByPtr.end())
        return BI_NONE;

    return it->first._get() == name._get() ? it->second : BI_NONE;
}

pcstr OzzKinematicsCore::GetBoneName(u16 bone_id) const
{
    if (bone_id >= boneInstances.size())
        return nullptr;

    const auto it = std::find_if(boneMapByName.begin(), boneMapByName.end(),
        [bone_id](const IKinematics::accel::value_type& entry)
        {
            return entry.second == bone_id;
        });

    return it != boneMapByName.end() ? it->first.c_str() : nullptr;
}

const Fmatrix& OzzKinematicsCore::GetBoneTransform(u16 bone_id) const
{
    static Fmatrix stub = Fidentity;
    if (bone_id < boneInstances.size())
        return boneInstances[bone_id].mTransform;
    return stub;
}

const Fmatrix& OzzKinematicsCore::GetBoneRenderTransform(u16 bone_id) const
{
    static Fmatrix stub = Fidentity;
    if (bone_id < boneInstances.size())
        return boneInstances[bone_id].mRenderTransform;
    return stub;
}

void OzzKinematicsCore::GetBindTransforms(xr_vector<Fmatrix>& matrices) const
{
    matrices.clear();
    matrices.reserve(bones.size());
    for (CBoneData* bone : bones)
    {
        if (bone)
            matrices.push_back(bone->bind_transform);
    }
}

const Fbox& OzzKinematicsCore::GetBoundingBox() const
{
    auto* self = const_cast<OzzKinematicsCore*>(this);

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

Fobb& OzzKinematicsCore::GetBoneBox(u16 bone_id)
{
    static Fobb stub;
    if (bone_id < boneBoxes.size())
        return boneBoxes[bone_id];
    return stub;
}
} // namespace Animation
} // namespace XRay
