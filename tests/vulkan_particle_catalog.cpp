#include "src/Layers/xrRenderVK/ParticleCatalog.h"

#include <cassert>
#include <cstring>

using namespace xray::render::vulkan;
using Bytes = std::vector<uint8_t>;

static void u16(Bytes& b, uint16_t n) { b.push_back(uint8_t(n)); b.push_back(uint8_t(n >> 8)); }
static void u32(Bytes& b, uint32_t n)
{
    for (int i = 0; i < 4; ++i) b.push_back(uint8_t(n >> (8 * i)));
}
static void real(Bytes& b, float n)
{
    uint32_t bits;
    std::memcpy(&bits, &n, 4);
    u32(b, bits);
}
static void name(Bytes& b, const char* value)
{
    while (*value) b.push_back(uint8_t(*value++));
    b.push_back(0);
}
static void chunk(Bytes& b, uint32_t id, const Bytes& contents)
{
    u32(b, id); u32(b, static_cast<uint32_t>(contents.size()));
    b.insert(b.end(), contents.begin(), contents.end());
}

int main()
{
    Bytes effect, group, effects, groups, library, data;
    Bytes version; u16(version, 1);
    chunk(effect, 1, version);
    name(data, "campfire"); chunk(effect, 2, data); data.clear();
    u32(data, 64); chunk(effect, 3, data); data.clear();
    chunk(effect, 4, Bytes{0, 0, 0, 0});
    u32(data, 1); chunk(effect, 5, data); data.clear();
    name(data, "particle"); name(data, "fire"); chunk(effect, 7, data); data.clear();
    chunk(effects, 0, effect);
    Bytes group_version; u16(group_version, 3);
    chunk(group, 1, group_version);
    name(data, "campfire_group"); chunk(group, 2, data); data.clear();
    u32(data, 0); chunk(group, 3, data); data.clear();
    u32(data, 1);
    name(data, "campfire"); name(data, ""); name(data, ""); name(data, "");
    real(data, 0.f); real(data, 2.f); u32(data, 5);
    chunk(group, 4, data);
    chunk(groups, 0, group);
    chunk(library, 1, version);
    chunk(library, 3, effects);
    chunk(library, 4, groups);
    ParticleCatalog catalog;
    std::string error;
    assert(parse_particle_catalog({library.data(), library.size()}, catalog, error));
    assert(catalog.effect("campfire") && catalog.effect("campfire")->max_particles == 64);
    assert(catalog.group("campfire_group") && catalog.group("campfire_group")->effects.size() == 1);
    Bytes wrong_group = group;
    wrong_group[8] = 1;
    Bytes wrong_groups, wrong_library;
    chunk(wrong_groups, 0, wrong_group);
    chunk(wrong_library, 1, version);
    chunk(wrong_library, 3, effects);
    chunk(wrong_library, 4, wrong_groups);
    assert(!parse_particle_catalog({wrong_library.data(), wrong_library.size()}, catalog, error));
    const auto previous = catalog;
    library.pop_back();
    assert(!parse_particle_catalog({library.data(), library.size()}, catalog, error));
    assert(catalog.effects.size() == previous.effects.size() && catalog.groups.size() == previous.groups.size());
}
