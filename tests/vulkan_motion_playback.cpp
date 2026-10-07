#include "src/Layers/xrRenderVK/MotionPlayback.h"

#include <cassert>
#include <cmath>

using namespace xray::render::vulkan;

int main()
{
    auto slots = std::make_shared<std::vector<MotionSlot>>();
    auto& slot = slots->emplace_back();
    slot.partition_names = {"all"};
    slot.partitions = {{0, 1}};
    MotionDefinition def;
    def.name = "idle";
    def.bone_or_part = 0;
    def.motion = 0;
    def.parameters = {1.f, 1.f, 10.f, 10.f};
    slot.definitions.push_back(def);
    auto& clip = slot.clips.emplace_back();
    clip.name = "idle";
    clip.frames = 2;
    clip.bones.resize(2);
    for (auto& bone : clip.bones)
    {
        bone.flags = 3;
        bone.rotations.push_back({0, 0, 0, 32767});
        bone.translation_size = {0.01f, 0.f, 0.f};
        bone.translations8 = {{{0, 0, 0}}, {{100, 0, 0}}};
    }
    MotionPlayback first(slots), second(slots);
    MotionPlayback::Handle id;
    assert(first.find("idle", false, id) && id.slot == 0 && id.definition == 0);
    assert(first.play(id, false) && second.play(id, false));
    first.advance(1.f / 30.f);
    second.advance(0.01f);
    MotionKey a, b;
    assert(first.sample(0, a) && second.sample(0, b));
    assert(a.translation[0] > b.translation[0]);
    assert(std::abs(a.rotation[3] - 1.f) < 0.001f);
    assert(!first.sample(2, a));
    first.stop_cycles();
    first.advance(0.2f);
    assert(first.active_count() == 0 && second.active_count() == 1);
    def.name = "hit";
    def.flags = 1;
    def.bone_or_part = 1;
    def.parameters = {1.f, 1.f, 10.f, 10.f};
    slot.definitions.push_back(def);
    assert(second.find("hit", true, id));
    int finished = 0;
    assert(second.play(id, true, true, 1.f, [&] { ++finished; }));
    second.advance(1.f);
    assert(finished == 1);
    second.advance(1.f);
    assert(finished == 1);
    assert(!sample_motion_key(clip, 3, 0.f, a));
    slot.partitions = {{0}, {1}};
    MotionPlayback overridden(slots);
    MotionPlayback::Handle cycle{0, 0};
    assert(overridden.play(cycle, false, false, 1.f, {}, -1.f, -1.f, -1.f, false, 0, 1));
    overridden.advance(1.f / 30.f);
    assert(!overridden.sample(0, a) && overridden.sample(1, b));
    overridden.stop_cycles(1);
    overridden.advance(0.2f);
    assert(overridden.active_count() == 0);
}
