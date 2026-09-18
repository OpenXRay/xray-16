#pragma once

#include "xrCore/Animation/Bone.hpp"
#include "xrCore/_matrix.h"
#include "xrCore/_obb.h"
#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"

namespace XRay
{
namespace Animation
{
enum BoneCollisionLayer : u32
{
    BoneCollisionLayerNone       = 0u,
    BoneCollisionLayerSoftTissue = 1u << 0u,
    BoneCollisionLayerRigidBody  = 1u << 1u,
    BoneCollisionLayerWeapon     = 1u << 2u,
};

struct ExtendedBoneMetadata
{
    SBoneShape shape{};
    Fobb obb{};
    SJointIKData joint{};
    xr_string game_material;

    float mass{ 0.f };
    Fvector center_of_mass{};

    float rest_length{ 0.f };
    Fvector dominant_axis{};
    Fvector local_aabb_min{};
    Fvector local_aabb_max{};
    Fmatrix inverse_global_transform{};

    Fvector inertia_tensor{};
    float volume{ 0.f };
    Flags32 collision_layers{};
    bool ground_contact_candidate{ false };
    bool weapon_anchor_candidate{ false };
};

using ExtendedBoneMetadataCollection = xr_vector<ExtendedBoneMetadata>;
} // namespace Animation
} // namespace XRay
