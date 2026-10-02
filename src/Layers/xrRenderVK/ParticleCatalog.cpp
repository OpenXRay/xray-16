#include "ParticleCatalog.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace xray::render::vulkan
{
namespace
{
uint16_t get16(const uint8_t* p) { return uint16_t(p[0]) | (uint16_t(p[1]) << 8); }
uint32_t get32(const uint8_t* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
        (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

struct Cursor
{
    LevelBytes bytes;
    size_t at{};
    bool take(size_t size, const uint8_t*& data)
    {
        if (!bytes.data || at > bytes.size || size > bytes.size - at) return false;
        data = bytes.data + at;
        at += size;
        return true;
    }
    bool word(uint16_t& value)
    {
        const uint8_t* p;
        if (!take(2, p)) return false;
        value = get16(p);
        return true;
    }
    bool integer(uint32_t& value)
    {
        const uint8_t* p;
        if (!take(4, p)) return false;
        value = get32(p);
        return true;
    }
    bool real(float& value)
    {
        uint32_t bits;
        if (!integer(bits)) return false;
        std::memcpy(&value, &bits, 4);
        return std::isfinite(value);
    }
    bool name(std::string& value)
    {
        size_t end = at;
        while (end < bytes.size && bytes.data[end] && end - at < 256) ++end;
        if (end >= bytes.size || end - at >= 256) return false;
        value.assign(reinterpret_cast<const char*>(bytes.data + at), end - at);
        at = end + 1;
        return true;
    }
    bool done() const { return at == bytes.size; }
};

bool chunks(LevelBytes bytes, std::vector<std::pair<uint32_t, LevelBytes>>& result)
{
    Cursor r{bytes};
    while (!r.done())
    {
        uint32_t id, size;
        const uint8_t* p;
        if (!r.integer(id) || !r.integer(size) || (id & 0x80000000u) || !r.take(size, p)) return false;
        result.emplace_back(id, LevelBytes{p, size});
    }
    return true;
}
LevelBytes find(const std::vector<std::pair<uint32_t, LevelBytes>>& chunks, uint32_t id)
{
    for (const auto& chunk : chunks) if (chunk.first == id) return chunk.second;
    return {};
}
bool unique_chunks(const std::vector<std::pair<uint32_t, LevelBytes>>& chunks)
{
    for (size_t i = 0; i < chunks.size(); ++i)
        for (size_t j = i + 1; j < chunks.size(); ++j)
            if (chunks[i].first == chunks[j].first) return false;
    return true;
}

bool effect(LevelBytes bytes, ParticleEffectDef& def)
{
    std::vector<std::pair<uint32_t, LevelBytes>> parts;
    if (!chunks(bytes, parts) || !unique_chunks(parts)) return false;
    Cursor version{find(parts, 1)}, name{find(parts, 2)}, maximum{find(parts, 3)}, flags{find(parts, 5)};
    uint16_t number;
    if (!version.word(number) || number != 1 || !version.done() ||
        !name.name(def.name) || def.name.empty() || !name.done() ||
        !maximum.integer(def.max_particles) || !maximum.done() ||
        !flags.integer(def.flags) || !flags.done() ||
        !def.max_particles || def.max_particles > 65535) return false;
    auto actions = find(parts, 4);
    if (!actions.data || actions.size < 4 || actions.size > 8 * 1024 * 1024 ||
        get32(actions.data) > 4096 || get32(actions.data) > (actions.size - 4) / 4) return false;
    def.actions.assign(actions.data, actions.data + actions.size);
    if (def.flags & 1u)
    {
        Cursor sprite{find(parts, 7)};
        if (!sprite.name(def.shader) || !sprite.name(def.texture) || !sprite.done() || def.texture.empty()) return false;
    }
    if (def.flags & (1u << 10))
    {
        Cursor frame{find(parts, 6)};
        float unused[2];
        int32_t columns, count;
        uint32_t bits;
        if (!frame.real(def.frame_size[0]) || !frame.real(def.frame_size[1]) ||
            !frame.real(unused[0]) || !frame.real(unused[1]) ||
            !frame.integer(bits)) return false;
        columns = static_cast<int32_t>(bits);
        if (!frame.integer(bits)) return false;
        count = static_cast<int32_t>(bits);
        if (!frame.real(def.frame_rate) || !frame.done() || columns <= 0 || count <= 0 ||
            count > 256 || def.frame_size[0] <= 0.f || def.frame_size[1] <= 0.f ||
            def.frame_rate < 0.f) return false;
        def.frame_columns = columns;
        def.frame_count = count;
    }
    if (def.flags & (1u << 14))
    {
        Cursor limit{find(parts, 8)};
        if (!limit.real(def.time_limit) || !limit.done() || def.time_limit < 0.f) return false;
    }
    return true;
}

bool group(LevelBytes bytes, ParticleGroupDef& def)
{
    std::vector<std::pair<uint32_t, LevelBytes>> parts;
    if (!chunks(bytes, parts) || !unique_chunks(parts)) return false;
    Cursor version{find(parts, 1)}, name{find(parts, 2)}, flags{find(parts, 3)};
    uint16_t number;
    if (!version.word(number) || number != 3 || !version.done() ||
        !name.name(def.name) || def.name.empty() || !name.done() ||
        !flags.integer(def.flags) || !flags.done()) return false;
    const auto limit = find(parts, 5);
    if (limit.data)
    {
        Cursor r{limit};
        if (!r.real(def.time_limit) || !r.done()) return false;
    }
    const auto effects = find(parts, 4);
    if (effects.data)
    {
        Cursor r{effects};
        uint32_t count;
        if (!r.integer(count) || count > 1024) return false;
        for (uint32_t i = 0; i < count; ++i)
        {
            ParticleGroupItem item;
            if (!r.name(item.effect) || !r.name(item.on_play) || !r.name(item.on_birth) ||
                !r.name(item.on_dead) || !r.real(item.begin) || !r.real(item.end) ||
                !r.integer(item.flags) || item.begin > item.end) return false;
            def.effects.push_back(std::move(item));
        }
        if (!r.done()) return false;
    }
    if (def.time_limit <= 0.f)
        for (const auto& item : def.effects) def.time_limit = std::max(def.time_limit, item.end);
    return true;
}
}

const ParticleEffectDef* ParticleCatalog::effect(const std::string& name) const
{
    for (const auto& def : effects) if (def.name == name) return &def;
    return nullptr;
}
const ParticleGroupDef* ParticleCatalog::group(const std::string& name) const
{
    for (const auto& def : groups) if (def.name == name) return &def;
    return nullptr;
}

bool parse_particle_catalog(LevelBytes bytes, ParticleCatalog& result, std::string& error)
{
    if (!bytes.data || bytes.size > 64 * 1024 * 1024)
    { error = "particle catalog is missing or too large"; return false; }
    std::vector<std::pair<uint32_t, LevelBytes>> root;
    Cursor version;
    if (!chunks(bytes, root) || !unique_chunks(root) || !(version.bytes = find(root, 1)).data)
    { error = "invalid particle library chunk table"; return false; }
    uint16_t number;
    if (!version.word(number) || number != 1 || !version.done())
    { error = "unsupported particle library version"; return false; }
    ParticleCatalog parsed;
    for (uint32_t kind : {3u, 4u})
    {
        const auto container = find(root, kind);
        if (!container.data) continue;
        std::vector<std::pair<uint32_t, LevelBytes>> entries;
        if (!chunks(container, entries) || entries.size() > 100000)
        { error = "invalid particle library entries"; return false; }
        for (size_t i = 0; i < entries.size(); ++i)
        {
            if (entries[i].first != i || (kind == 3 ?
                    !effect(entries[i].second, parsed.effects.emplace_back()) :
                    !group(entries[i].second, parsed.groups.emplace_back())))
            { error = "invalid particle effect or group definition"; return false; }
        }
    }
    for (const auto& def : parsed.groups)
        for (const auto& item : def.effects)
            if (!parsed.effect(item.effect) ||
                ((item.flags & (1u << 1)) && !item.on_play.empty() && !parsed.effect(item.on_play)) ||
                ((item.flags & (1u << 5)) && !item.on_birth.empty() && !parsed.effect(item.on_birth)) ||
                ((item.flags & (1u << 6)) && !item.on_dead.empty() && !parsed.effect(item.on_dead)))
            { error = "particle group references an unknown effect"; return false; }
    result = std::move(parsed);
    error.clear();
    return true;
}
} // namespace xray::render::vulkan
