#include "xrEngine/stdafx.h"
#include "src/Layers/xrRenderVK/VulkanModelVisual.h"
#include "xrEngine/EnnumerateVertices.h"

#include <cassert>

using namespace xray::render::vulkan;

static void move_bone(CBoneInstance* instance)
{
    instance->mTransform.c.x = 2.f;
}

struct VertexCounter : SEnumVerticesCallback
{
    int count = 0;
    float first_x = 0.f;

    void operator()(const Fvector& position) override
    {
        if (!count) first_x = position.x;
        ++count;
    }
};

int main()
{
    Core.Initialize("vulkan_kinematics_test", nullptr, false);
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
    MotionSlot slot;
    slot.source = "actor.omf";
    slot.partition_names = {"all"};
    slot.partitions = {{0, 1}};
    MotionDefinition idle;
    idle.name = "idle";
    idle.bone_or_part = 0;
    idle.parameters = {1.f, 1.f, 10.f, 10.f};
    slot.definitions.push_back(idle);
    auto& clip = slot.clips.emplace_back();
    clip.name = "idle";
    clip.frames = 2;
    clip.bones.resize(2);
    for (auto& track : clip.bones)
    {
        track.flags = 3;
        track.rotations.push_back({0, 0, 0, 32767});
        track.translation_size = {0.01f, 0.f, 0.f};
        track.translations8 = {{{0, 0, 0}}, {{100, 0, 0}}};
    }
    static_cast<VulkanKinematics*>(pose)->set_motions({ slot });
    ModelGeometry geometry;
    geometry.type = 3;
    geometry.children.resize(1);
    auto& child = geometry.children[0];
    child.type = 5;
    child.vertices.resize(3);
    child.indices = { 0, 1, 2 };
    child.vertices[0].position[0] = 0.f;
    child.vertices[1].position[0] = 1.f;
    child.vertices[2].position[1] = 1.f;
    for (auto& vertex : child.vertices)
        vertex.bones[0] = 1;
    assert(static_cast<VulkanKinematics*>(pose)->attach_geometry(geometry, error));
    xr_vector<xr_vector<u16>> groups;
    assert(pose->LL_GetBoneGroups(groups) == 1 && groups[0].size() == 1 && groups[0][0] == 1);
    VertexCounter counter;
    pose->EnumBoneVertices(counter, 1);
    assert(counter.count == 3);
    Fmatrix identity;
    identity.identity();
    IKinematics::pick_result hit{};
    Fvector start, direction;
    start.set(0.25f, 0.25f, -1.f);
    direction.set(0.f, 0.f, 1.f);
    assert(pose->PickBone(identity, hit, 5.f, start, direction, 1));
    assert(hit.dist == 1.f);
    pose->LL_GetBoneInstance(1).set_callback(0, move_bone, nullptr);
    pose->CalculateBones_Invalidate();
    pose->CalculateBones(TRUE);
    assert(pose->LL_GetTransform_R(1).c.x == 2.f);
    VertexCounter moved;
    pose->EnumBoneVertices(moved, 1);
    assert(moved.count == 3 && moved.first_x == 2.f);
    pose->LL_GetBoneInstance(1).set_callback(0, move_bone, nullptr, TRUE);
    pose->CalculateBones(TRUE);
    assert(pose->LL_GetTransform_R(1).c.x == 2.f);
    static_cast<VulkanKinematics*>(pose)->reset_instance_state();
    assert(!pose->LL_GetBoneInstance(1).callback() && pose->LL_GetTransform_R(1).c.x == 0.f);
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
    assert(static_cast<VulkanKinematics*>(copied)->motions()[0].clips[0].name == "idle");
    assert(copied && copied->dcast_RenderVisual() == duplicate.get());
    assert(copied->LL_BoneCount() == 2 && !copied->LL_GetBoneVisible(1));
    copied->LL_SetBoneVisible(1, TRUE, FALSE);
    assert(copied->LL_GetBoneVisible(1) && !pose->LL_GetBoneVisible(1));
    copied->LL_GetBoneInstance(1).mTransform.c.x = 7.f;
    assert(pose->LL_GetBoneInstance(1).mTransform.c.x != 7.f);
    auto* animated = pose->dcast_PKinematicsAnimated();
    auto* copied_animated = copied->dcast_PKinematicsAnimated();
    pose->LL_SetBoneVisible(1, TRUE, FALSE);
    assert(animated && copied_animated && animated->LL_MotionsSlotCount() == 1);
    assert(animated->ID_Cycle_Safe("idle").valid());
    assert(animated->LL_PartID("all") == 0);
    assert(animated->PlayCycle("idle", FALSE, nullptr, nullptr, 0));
    assert(copied_animated->PlayCycle("idle", FALSE, nullptr, nullptr, 0));
    animated->LL_UpdateTracks(1.f / 30.f, true, false);
    copied_animated->LL_UpdateTracks(0.01f, true, false);
    pose->CalculateBones(TRUE);
    copied->CalculateBones(TRUE);
    assert(pose->LL_GetTransform(1).c.x > copied->LL_GetTransform(1).c.x);
    assert(animated->LL_PartBlendsCount(0) == 1);
    animated->LL_CloseCycle(0, 1);
    animated->LL_UpdateTracks(0.2f, true, false);
    assert(animated->LL_PartBlendsCount(0) == 0);
    return 0;
}
