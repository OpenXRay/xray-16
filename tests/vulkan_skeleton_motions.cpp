#include "src/Layers/xrRenderVK/SkeletonMotions.h"

#include <cassert>
#include <cstring>

using namespace xray::render::vulkan;

namespace
{
using Bytes = std::vector<uint8_t>;

void u16(Bytes& out, uint16_t value)
{
    out.push_back(uint8_t(value));
    out.push_back(uint8_t(value >> 8));
}

void u32(Bytes& out, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i)
        out.push_back(uint8_t(value >> (i * 8)));
}

void f32(Bytes& out, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    u32(out, bits);
}

void name(Bytes& out, const char* value)
{
    while (*value)
        out.push_back(uint8_t(*value++));
    out.push_back(0);
}

void chunk(Bytes& out, uint32_t id, const Bytes& value)
{
    u32(out, id);
    u32(out, static_cast<uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

Bytes omf(bool remap = false)
{
    Bytes params, motions, clip, result;
    u16(params, 4);
    u16(params, 1); // format and partition count
    name(params, "all");
    u16(params, 2);
    name(params, "hand");
    u32(params, 0); // file order differs from skeleton order
    name(params, "root");
    u32(params, 1);
    u16(params, remap ? 2 : 1);
    name(params, "idle");
    u32(params, 0);
    u16(params, UINT16_MAX); // BI_NONE uses the default partition
    u16(params, remap ? 1 : 0); // playback track may differ from lookup index
    for (int i = 0; i < 4; ++i)
        f32(params, 1.f);
    u32(params, 1); // one mark
    params.insert(params.end(), { 's', 't', 'e', 'p', '\r', '\n' });
    u32(params, 1);
    f32(params, 0.f);
    f32(params, 0.03f);
    if (remap)
    {
        name(params, "move");
        u32(params, 0);
        u16(params, UINT16_MAX);
        u16(params, 0);
        for (int i = 0; i < 4; ++i) f32(params, 1.f);
        u32(params, 0);
    }
    u32(motions, 0);
    u32(motions, 4);
    u32(motions, remap ? 2 : 1);
    name(clip, "idle");
    u32(clip, 2);
    clip.push_back(3); // hand: constant rotation, 8-bit translations
    for (int i = 0; i < 4; ++i)
        u16(clip, i == 3 ? 32767 : 0);
    u32(clip, 0x12345678);
    for (int i = 0; i < 6; ++i)
        clip.push_back(uint8_t(i));
    for (int i = 0; i < 3; ++i)
        f32(clip, 1.f);
    for (int i = 0; i < 3; ++i)
        f32(clip, 0.f);
    clip.push_back(0); // root: two rotation keys, fixed translation
    u32(clip, 0x87654321);
    for (int i = 0; i < 8; ++i)
        u16(clip, i % 4 == 3 ? 32767 : 0);
    for (int i = 0; i < 3; ++i)
        f32(clip, 0.f);
    chunk(motions, 1, clip);
    if (remap)
    {
        Bytes other;
        name(other, "move");
        other.insert(other.end(), clip.begin() + 5, clip.end());
        chunk(motions, 2, other);
    }
    chunk(result, 15, params);
    chunk(result, 14, motions);
    return result;
}
} // namespace

int main()
{
    Bytes bytes = omf();
    std::vector<MotionSlot> slots;
    std::string error;
    auto resolve = [&](const std::string& path, std::vector<MotionFile>& files, std::string&)
    {
        assert(path == "actor.omf" || path == "actors\\*.omf");
        files.emplace_back("actor.omf", bytes);
        return true;
    };
    assert(parse_skeleton_motions({ bytes.data(), bytes.size() }, { "root", "hand" }, "actor.ogf", resolve, slots, error));
    assert(slots.size() == 1 && slots[0].clips[0].frames == 2);
    assert(slots[0].definitions[0].bone_or_part == UINT16_MAX);
    assert(slots[0].clips[0].bones[1].rotations.size() == 1);
    assert(slots[0].clips[0].bones[1].translations8.size() == 2);
    assert(slots[0].clips[0].bones[0].rotations.size() == 2);
    assert(slots[0].partitions[0][0] == 1 && slots[0].definitions[0].marks[0].name == "step");

    // Names follow lookup order, while each definition's motion field can
    // select a different numbered track for playback.
    bytes = omf(true);
    assert(parse_skeleton_motions({ bytes.data(), bytes.size() }, { "root", "hand" },
        "actor.ogf", resolve, slots, error));
    assert(slots[0].definitions[0].motion == 1 && slots[0].clips[1].name == "move");
    bytes = omf();

    Bytes refs, ogf;
    name(refs, "actor");
    chunk(ogf, 19, refs);
    assert(parse_skeleton_motions({ ogf.data(), ogf.size() }, { "root", "hand" }, "actor.ogf", resolve, slots, error));
    assert(slots[0].source == "actor.omf");
    refs.clear();
    ogf.clear();
    u32(refs, 1);
    name(refs, "actors\\*.omf");
    chunk(ogf, 24, refs);
    assert(parse_skeleton_motions({ ogf.data(), ogf.size() }, { "root", "hand" }, "actor.ogf", resolve, slots, error));

    const auto previous = slots;
    bytes.pop_back();
    assert(!parse_skeleton_motions({ bytes.data(), bytes.size() }, { "root", "hand" }, "actor.ogf", resolve, slots, error));
    assert(!error.empty() && slots[0].clips[0].frames == previous[0].clips[0].frames);
    bytes = omf();
    auto missing = [](const std::string&, std::vector<MotionFile>&, std::string&)
    {
        return false;
    };
    assert(!parse_skeleton_motions({ ogf.data(), ogf.size() }, { "root", "hand" }, "actor.ogf", missing, slots, error));
    assert(slots[0].source == previous[0].source);
}
