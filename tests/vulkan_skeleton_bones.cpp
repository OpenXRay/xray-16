#include "src/Layers/xrRenderVK/SkeletonBones.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

using namespace xray::render::vulkan;

namespace
{
void u32(std::vector<uint8_t>& out, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(value >> (i * 8)));
}

void bone(std::vector<uint8_t>& out, const char* name, const char* parent)
{
    for (const char* p = name; *p; ++p) out.push_back(*p);
    out.push_back(0);
    for (const char* p = parent; *p; ++p) out.push_back(*p);
    out.push_back(0);
    out.insert(out.end(), 60, 0);
}

std::vector<uint8_t> skeleton(const char* parent = "ROOT", bool with_ik = false)
{
    std::vector<uint8_t> names, result;
    u32(names, 2);
    bone(names, "Root", "");
    bone(names, "Child", parent);
    u32(result, 13); u32(result, static_cast<uint32_t>(names.size()));
    result.insert(result.end(), names.begin(), names.end());
    if (with_ik)
    {
        std::vector<uint8_t> ik;
        for (unsigned i = 0; i < 2; ++i)
        {
            u32(ik, 1);
            ik.push_back(0); // material name
            ik.insert(ik.end(), 0x70 + 0x4c + 0x28, 0);
        }
        u32(result, 16); u32(result, static_cast<uint32_t>(ik.size()));
        result.insert(result.end(), ik.begin(), ik.end());
    }
    return result;
}

bool parse(const std::vector<uint8_t>& bytes, SkeletonBones& out, std::string& error)
{
    return parse_skeleton_bones({bytes.data(), bytes.size()}, out, error);
}
}

int main()
{
    SkeletonBones bones;
    std::string error;
    auto bytes = skeleton("ROOT", true);
    assert(parse(bytes, bones, error) && error.empty());
    assert(bones.root == 0 && bones.bones.size() == 2);
    assert(bones.bones[0].name == "root" && bones.bones[1].parent == 0);
    assert(!bones.bones[0].ik_data.empty() && !bones.bones[1].ik_data.empty());
    const std::vector<uint8_t> user_data{ '[', 'i', 'n', 'f', 'o', ']', '\n' };
    u32(bytes, 17);
    u32(bytes, static_cast<uint32_t>(user_data.size()));
    bytes.insert(bytes.end(), user_data.begin(), user_data.end());
    assert(parse(bytes, bones, error) && bones.user_data == user_data);

    auto bad = skeleton("missing");
    assert(!parse(bad, bones, error) && !error.empty());
    assert(bones.bones.size() == 2); // failed reload leaves the original intact
    bad = skeleton("Child");
    assert(!parse(bad, bones, error));
    bad = skeleton("ROOT", true);
    bad.pop_back();
    assert(!parse(bad, bones, error));
    bad = skeleton();
    bad[8] = 255; // invalid bone count
    assert(!parse(bad, bones, error));
}
