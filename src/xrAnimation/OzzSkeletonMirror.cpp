#include "stdafx.h"

#include "OzzSkeletonMirror.h"

#include "xrCore/_quaternion.h"
#include "xrCommon/xr_unordered_map.h"

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/base/maths/quaternion.h>
#include <ozz/base/maths/transform.h>
#include <ozz/base/maths/vec_float.h>

namespace XRay
{
namespace Animation
{
namespace
{
using JointChildren = xr_vector<xr_vector<u16>>;

ozz::math::Transform MakeJointTransform(const Fmatrix& bind_local)
{
    Fquaternion q;
    q.set(bind_local);
    q.normalize();

    ozz::math::Transform t;
    t.translation = ozz::math::Float3(bind_local.c.x, bind_local.c.y, bind_local.c.z);
    t.rotation = ozz::math::Quaternion(q.x, q.y, q.z, q.w);
    t.scale = ozz::math::Float3::one();
    return t;
}

void FillJoint(ozz::animation::offline::RawSkeleton::Joint& joint, u16 bone,
    ozz::span<const OzzBoneDesc> bones, const JointChildren& children)
{
    const OzzBoneDesc& desc = bones[bone];
    joint.name = desc.name.c_str() ? desc.name.c_str() : "";
    joint.transform = MakeJointTransform(desc.bind_local);

    const xr_vector<u16>& kids = children[bone];
    joint.children.resize(kids.size());
    for (size_t i = 0; i < kids.size(); ++i)
        FillJoint(joint.children[i], kids[i], bones, children);
}
}

std::shared_ptr<const OzzSkeletonMirror> BuildOzzSkeletonMirror(ozz::span<const OzzBoneDesc> bones)
{
    const size_t boneCount = bones.size();
    if (boneCount == 0)
        return nullptr;

    JointChildren children(boneCount);
    xr_vector<u16> roots;
    roots.reserve(boneCount);

    for (size_t i = 0; i < boneCount; ++i)
    {
        const u16 parent = bones[i].parent;
        if (parent == BI_NONE || size_t(parent) >= boneCount || size_t(parent) == i)
            roots.push_back(u16(i));
        else
            children[parent].push_back(u16(i));
    }

    if (roots.empty())
        return nullptr;

    ozz::animation::offline::RawSkeleton raw;
    raw.roots.resize(roots.size());
    for (size_t i = 0; i < roots.size(); ++i)
        FillJoint(raw.roots[i], roots[i], bones, children);

    if (!raw.Validate())
        return nullptr;

    const ozz::animation::offline::SkeletonBuilder builder;
    ozz::unique_ptr<ozz::animation::Skeleton> built = builder(raw);
    if (!built)
        return nullptr;

    auto mirror = std::make_shared<OzzSkeletonMirror>();
    mirror->skeleton = std::move(*built);

    const int jointCount = mirror->skeleton.num_joints();
    const ozz::span<const char* const> jointNames = mirror->skeleton.joint_names();
    const ozz::span<const int16_t> jointParents = mirror->skeleton.joint_parents();

    mirror->boneToJoint.assign(boneCount, BI_NONE);
    mirror->jointToBone.assign(size_t(jointCount), BI_NONE);

    xr_unordered_map<shared_str, u16> byName;
    byName.reserve(boneCount);
    for (size_t i = 0; i < boneCount; ++i)
        byName.emplace(bones[i].name, u16(i));

    for (int j = 0; j < jointCount; ++j)
    {
        const shared_str key(jointNames[j]);
        const auto it = byName.find(key);
        if (it == byName.end())
            continue;
        mirror->jointToBone[size_t(j)] = it->second;
        mirror->boneToJoint[it->second] = u16(j);
    }

    u32 crc = 0u;
    for (int j = 0; j < jointCount; ++j)
    {
        const char* n = jointNames[j];
        crc = crc32(n, u32(xr_strlen(n) + 1), crc);
    }
    if (jointCount > 0)
        crc = crc32(jointParents.data(), u32(sizeof(int16_t) * size_t(jointCount)), crc);

    mirror->fingerprint = crc;

    return mirror;
}
}
}
