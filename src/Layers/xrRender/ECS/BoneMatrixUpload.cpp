#include "stdafx.h"

#include "BoneMatrixUpload.h"
#include "Components.h"
#include "RenderWorld.h"

#include "xrAnimation/OzzConversion.h"

namespace xray::render::bone_upload {

void Run(xray::ecs::World&)
{
    auto& reg = xray::render::RenderWorld::Get().Registry();

    auto view = reg.view<xray::ecs::COzzMeshSkinning, xray::ecs::CSkeletonBones, xray::ecs::CExtractedBoneModels>();
    for (auto e : view)
    {
        auto& skinning  = view.get<xray::ecs::COzzMeshSkinning>(e);
        auto& bones     = view.get<xray::ecs::CSkeletonBones>(e);
        const auto& extracted = view.get<xray::ecs::CExtractedBoneModels>(e);

        bones.frame_uploaded            = 0;
        bones.frame_uploaded_bone_offset = 0;
        bones.bone_count                = static_cast<u32>(skinning.joint_remaps.size());

        if (skinning.joint_remaps.empty())
            continue;

        const auto& models = extracted.models;
        const std::size_t skel_joint_count = models.size();

        if (skinning.inverse_bind_poses.size() != skinning.joint_remaps.size())
            continue;

        skinning.palette_staging.resize(skinning.joint_remaps.size());
        for (std::size_t i = 0; i < skinning.joint_remaps.size(); ++i)
        {
            const std::uint16_t skel_idx = skinning.joint_remaps[i];
            if (static_cast<std::size_t>(skel_idx) >= skel_joint_count)
            {
                skinning.palette_staging[i].identity();
                continue;
            }

            const ozz::math::Float4x4 combined = models[skel_idx] * skinning.inverse_bind_poses[i];
            skinning.palette_staging[i] = XRay::Animation::ConvertOzzMatrixToXRay(combined);
        }
    }
}

}
