#include "xrEngine/stdafx.h"
#include "src/Layers/xrRenderVK/VulkanModelVisual.h"

#include <cassert>

using namespace xray::render::vulkan;

int main()
{
    SkeletonBones bones;
    bones.root = 0;
    bones.bones.resize(2);
    bones.bones[0].name = "root";
    bones.bones[1].name = "hand";
    bones.bones[1].parent = 0;

    VisualRecord record;
    record.type = 3;
    auto visual = std::make_unique<VulkanModelVisual>(record, "", nullptr);
    std::string error;
    auto kinematics = VulkanKinematics::create(bones, visual.get(), error);
    assert(kinematics && error.empty());
    visual->set_skeleton(std::move(kinematics));
    auto* pose = visual->dcast_PKinematics();
    assert(pose && pose->LL_BoneCount() == 2 && pose->LL_GetBoneRoot() == 0);
    assert(pose->LL_BoneID("HAND") == 1 && pose->LL_BoneID("missing") == BI_NONE);
    assert(pose->LL_GetData(1).GetParentID() == 0);
    xr_vector<Fmatrix> bind;
    pose->LL_GetBindTransform(bind);
    assert(bind.size() == 2 && bind[1].c.x == 0.f);
    assert(pose->LL_VisibleBoneCount() == 2);
    pose->LL_SetBoneVisible(1, FALSE, FALSE);
    assert(pose->LL_VisibleBoneCount() == 1 && !pose->LL_GetBoneVisible(1));

    auto duplicate = std::make_unique<VulkanModelVisual>(*visual);
    auto* copied = duplicate->dcast_PKinematics();
    assert(copied && copied->dcast_RenderVisual() == duplicate.get());
    assert(copied->LL_BoneCount() == 2 && !copied->LL_GetBoneVisible(1));
    copied->LL_SetBoneVisible(1, TRUE, FALSE);
    assert(copied->LL_GetBoneVisible(1) && !pose->LL_GetBoneVisible(1));
    copied->LL_GetBoneInstance(1).mTransform.c.x = 7.f;
    assert(pose->LL_GetBoneInstance(1).mTransform.c.x != 7.f);
}
