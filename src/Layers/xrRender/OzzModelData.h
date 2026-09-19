#pragma once

#include <cstdint>
#include <vector>

#include "Include/xrRender/Kinematics.h"
#include "xrAnimation/ExtendedBoneMetadata.h"
#include "xrCommon/xr_smart_pointers.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrCore/Animation/SkeletonMotions.hpp"

#include "ozz/animation/runtime/skeleton.h"

class CInifile;

namespace XRay::Animation
{
struct OzzxBundle;

struct OzzModelData
{
    using accel = IKinematics::accel;

    ozz::animation::Skeleton skeleton;

    xr_vector<xr_unique_ptr<CBoneData>> boneStorage;
    xr_vector<CBoneData*> bones;
    xr_vector<Fmatrix> bindModel;

    accel boneMapByName;
    accel boneMapByPtr;

    xr_unique_ptr<CInifile> userData;

    xr_vector<xr_string> motionRefs;
    std::vector<std::uint8_t> embeddedAnimation;

    CPartition defaultPartition;

    u16 defaultRoot{0};

    OzzModelData();
    ~OzzModelData();

    OzzModelData(const OzzModelData&) = delete;
    OzzModelData& operator=(const OzzModelData&) = delete;

    bool Load(const OzzxBundle& bundle);

private:
    bool LoadSkeleton(const std::vector<std::uint8_t>& payload);
    bool BuildBoneMetadata();
    void BuildBindTransforms();
    void ApplyExtendedBoneMetadata(const ExtendedBoneMetadataCollection& metadata);
    void LoadUserData(const std::vector<std::uint8_t>& buffer);
    void BuildDefaultPartition();
};
}
