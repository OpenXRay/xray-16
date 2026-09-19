#include "stdafx.h"

#include "OzzModelData.h"

#include "OzzConversion.h"

#include "xrAnimation/OzzBundle.h"
#include "xrCore/FS.h"
#include "xrMaterialSystem/GameMtlLib.h"

#include "ozz/animation/runtime/skeleton_utils.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"

#include <algorithm>
#include <cmath>

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

OzzModelData::OzzModelData() = default;

OzzModelData::~OzzModelData() = default;

bool OzzModelData::Load(const OzzxBundle& bundle)
{
    if (!LoadSkeleton(bundle.skeleton))
        return false;

    if (!BuildBoneMetadata())
        return false;

    ApplyExtendedBoneMetadata(bundle.bone_metadata);
    LoadUserData(bundle.user_data);

    motionRefs = bundle.motion_refs;
    embeddedAnimation = bundle.embedded_animation_data;

    BuildDefaultPartition();
    return true;
}

bool OzzModelData::LoadSkeleton(const std::vector<std::uint8_t>& payload)
{
    if (payload.empty())
    {
        Msg("[OzzModelData] Received empty skeleton payload");
        return false;
    }

    ozz::io::MemoryStream memoryStream;
    if (!memoryStream.Write(payload.data(), payload.size()))
    {
        Msg("[OzzModelData] Failed to copy skeleton buffer into memory stream");
        return false;
    }

    memoryStream.Seek(0, ozz::io::Stream::kSet);

    ozz::io::IArchive archive(&memoryStream);
    if (!archive.TestTag<ozz::animation::Skeleton>())
    {
        Msg("[OzzModelData] Payload does not contain a skeleton");
        return false;
    }

    archive >> skeleton;

    const int joint_count = skeleton.num_joints();
    if (joint_count <= 0)
    {
        Msg("[OzzModelData] Skeleton contains no joints");
        return false;
    }

    const auto joint_names = skeleton.joint_names();

    boneMapByName.clear();
    boneMapByPtr.clear();
    boneMapByName.reserve(static_cast<size_t>(joint_count));
    boneMapByPtr.reserve(static_cast<size_t>(joint_count));

    for (int joint = 0; joint < joint_count; ++joint)
    {
        const size_t joint_index = static_cast<size_t>(joint);
        const char* joint_name = (joint_index < joint_names.size() && joint_names[joint_index]) ? joint_names[joint_index] : "";
        shared_str shared_name(joint_name);
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

    defaultRoot = 0;
    return true;
}

bool OzzModelData::BuildBoneMetadata()
{
    const int joint_count = skeleton.num_joints();
    if (joint_count <= 0)
        return false;

    const ozz::span<const int16_t> parents = skeleton.joint_parents();
    const auto joint_names = skeleton.joint_names();

    boneStorage.clear();
    boneStorage.reserve(static_cast<size_t>(joint_count));
    bones.assign(static_cast<size_t>(joint_count), nullptr);

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

    if (!bones.empty() && defaultRoot < bones.size())
        bones[defaultRoot]->CalculateM2B(Fidentity);

    return true;
}

void OzzModelData::ApplyExtendedBoneMetadata(const ExtendedBoneMetadataCollection& metadata)
{
    if (metadata.empty())
        return;

    if (metadata.size() != bones.size())
    {
        Msg("[OzzModelData] Bone metadata count mismatch (metadata=%zu, bones=%zu)", metadata.size(), bones.size());
        return;
    }

    for (size_t idx = 0; idx < metadata.size(); ++idx)
    {
        CBoneData* bone = bones[idx];
        if (!bone)
            continue;

        const ExtendedBoneMetadata& source = metadata[idx];

        bone->shape = source.shape;
        if (_valid(source.obb))
            bone->obb = source.obb;
        else
            bone->obb = BuildFallbackObbFromShape(source.shape);

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
                Msg("[OzzModelData] Unknown material '%s' for bone '%s'", material_name, bone->name.c_str());
        }
    }
}

void OzzModelData::LoadUserData(const std::vector<std::uint8_t>& buffer)
{
    userData.reset();

    if (buffer.empty())
        return;

    IReader reader(const_cast<std::uint8_t*>(buffer.data()), buffer.size());

    pcstr config_path = "";
    if (FS.path_exist("$game_config$"))
    {
        const FS_Path* path = FS.get_path("$game_config$");
        if (path)
            config_path = path->m_Path;
    }

    userData = xr_make_unique<CInifile>(&reader, config_path);
}

void OzzModelData::BuildDefaultPartition()
{
    defaultPartition[0].Name = "default";

    const u16 bone_count = static_cast<u16>(bones.size());
    defaultPartition[0].bones.resize(bone_count);
    for (u16 i = 0; i < bone_count; ++i)
        defaultPartition[0].bones[i] = i;
}
}
