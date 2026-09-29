#include "LevelModels.h"

#include <cstring>
#include <utility>

namespace xray::render::vulkan
{
namespace
{
constexpr uint32_t ogf_header = 1, ogf_vertices = 3, ogf_indices = 4,
    ogf_swi = 6, ogf_children = 9, ogf_links = 10, ogf_container = 21;
constexpr size_t max_vertices = 4'000'000, max_indices = 12'000'000;

struct Cursor
{
    LevelBytes bytes;
    size_t offset{};

    bool take(size_t size, LevelBytes& out)
    {
        if (offset > bytes.size || size > bytes.size - offset) return false;
        out = {bytes.data + offset, size};
        offset += size;
        return true;
    }
    bool u32(uint32_t& value)
    {
        LevelBytes part;
        if (!take(4, part)) return false;
        value = uint32_t(part.data[0]) | (uint32_t(part.data[1]) << 8) |
            (uint32_t(part.data[2]) << 16) | (uint32_t(part.data[3]) << 24);
        return true;
    }
    bool done() const { return offset == bytes.size; }
};

bool chunk(LevelBytes bytes, uint32_t id, LevelBytes& out)
{
    Cursor c{bytes};
    while (!c.done())
    {
        uint32_t key, size;
        if (!c.u32(key) || !c.u32(size)) return false;
        LevelBytes body;
        if (!c.take(size, body)) return false;
        if (key == id) { out = body; return true; }
    }
    return false;
}

struct VertexBuffer
{
    std::vector<LevelVertex> vertices;
};

bool read_vertices(LevelBytes bytes, std::vector<VertexBuffer>& buffers)
{
    Cursor c{bytes};
    uint32_t count;
    if (!c.u32(count) || count > 65536) return false;
    buffers.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        int position = -1, normal = -1, uv = -1;
        size_t stride = 0;
        bool ended = false;
        // D3DVERTEXELEMENT9: stream u16, offset u16, type, method, usage, index.
        for (size_t d = 0; d <= 64; ++d)
        {
            LevelBytes element;
            if (!c.take(8, element)) return false;
            const uint16_t stream = uint16_t(element.data[0]) | (uint16_t(element.data[1]) << 8);
            if (stream == 0xff) { ended = true; break; }
            if (stream != 0) return false;
            const size_t offset = size_t(element.data[2]) | (size_t(element.data[3]) << 8);
            const uint8_t type = element.data[4], usage = element.data[6], index = element.data[7];
            // Decode the common static declarations; reject unknown sizes to
            // avoid calculating a wrong stride for subsequent vertices.
            const size_t type_sizes[]{4, 8, 12, 16, 4, 4, 4, 8, 4, 4, 4, 8, 8, 4, 4, 4, 8};
            if (type >= sizeof(type_sizes) / sizeof(*type_sizes)) return false;
            const size_t end = offset + type_sizes[type];
            if (end > 4096 || end < offset) return false;
            if (end > stride) stride = end;
            if (usage == 0 && index == 0 && type == 2) position = static_cast<int>(offset);
            if (usage == 3 && index == 0 && type == 2) normal = static_cast<int>(offset);
            if (usage == 5 && index == 0 && type == 1) uv = static_cast<int>(offset);
        }
        uint32_t vertex_count;
        if (!ended || position < 0 || normal < 0 || !stride || !c.u32(vertex_count) ||
            vertex_count > max_vertices) return false;
        LevelBytes data;
        if (!c.take(stride * vertex_count, data)) return false;
        VertexBuffer buffer;
        buffer.vertices.reserve(vertex_count);
        for (uint32_t v = 0; v < vertex_count; ++v)
        {
            LevelVertex vertex;
            const uint8_t* source = data.data + stride * v;
            std::memcpy(vertex.position, source + position, sizeof(vertex.position));
            std::memcpy(vertex.normal, source + normal, sizeof(vertex.normal));
            if (uv >= 0) std::memcpy(vertex.uv, source + uv, sizeof(vertex.uv));
            buffer.vertices.push_back(vertex);
        }
        buffers.push_back(std::move(buffer));
    }
    return c.done();
}

bool read_indices(LevelBytes bytes, std::vector<std::vector<uint16_t>>& buffers)
{
    Cursor c{bytes};
    uint32_t count;
    if (!c.u32(count) || count > 65536) return false;
    buffers.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t index_count;
        if (!c.u32(index_count) || index_count > max_indices) return false;
        LevelBytes data;
        if (!c.take(size_t(index_count) * 2, data)) return false;
        std::vector<uint16_t> indices;
        indices.reserve(index_count);
        for (uint32_t n = 0; n < index_count; ++n)
            indices.push_back(uint16_t(data.data[n * 2]) | (uint16_t(data.data[n * 2 + 1]) << 8));
        buffers.push_back(std::move(indices));
    }
    return c.done();
}

bool read_materials(LevelBytes bytes, std::vector<LevelMaterial>& materials)
{
    Cursor c{bytes};
    uint32_t count;
    if (!c.u32(count) || count > 65536) return false;
    materials.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        const size_t begin = c.offset;
        while (c.offset < c.bytes.size && c.bytes.data[c.offset]) ++c.offset;
        if (c.offset == c.bytes.size) return false;
        std::string name(reinterpret_cast<const char*>(c.bytes.data + begin), c.offset - begin);
        ++c.offset;
        const size_t slash = name.find('/');
        if (!name.empty() && (slash == std::string::npos || slash == 0 || slash == name.size() - 1))
            return false;
        materials.push_back(slash == std::string::npos ? LevelMaterial{} :
            LevelMaterial{name.substr(0, slash), name.substr(slash + 1)});
    }
    return c.done();
}

bool decode_visual(LevelBytes visual, const std::vector<VertexBuffer>& vertices,
    const std::vector<std::vector<uint16_t>>& indices, size_t material_count,
    std::vector<LevelModel>& models, std::vector<LevelVisual>& nodes,
    size_t node_index, unsigned depth)
{
    if (depth > 32 || node_index >= nodes.size()) return false;
    LevelBytes header;
    if (!chunk(visual, ogf_header, header) || header.size != 44 || header.data[0] != 4)
        return false;
    const uint8_t type = header.data[1];
    const uint16_t material = uint16_t(header.data[2]) | (uint16_t(header.data[3]) << 8);
    nodes[node_index].type = type;
    std::memcpy(nodes[node_index].bounds.data(), header.data + 4, 10 * sizeof(float));
    if (type == 1) // MT_HIERRARHY
    {
        LevelBytes links, embedded;
        const bool has_links = chunk(visual, ogf_links, links);
        const bool has_embedded = chunk(visual, ogf_children, embedded);
        if (has_links == has_embedded) return false;
        if (has_links)
        {
            Cursor c{links};
            uint32_t count;
            if (!c.u32(count) || count > nodes.size() || links.size != 4 + size_t(count) * 4)
                return false;
            nodes[node_index].children.reserve(count);
            for (uint32_t i = 0; i < count; ++i)
            {
                uint32_t child;
                if (!c.u32(child) || child >= nodes.size()) return false;
                nodes[node_index].children.push_back(child);
            }
        }
        else
        {
            Cursor c{embedded};
            uint32_t expected = 0;
            while (!c.done())
            {
                uint32_t id, size;
                LevelBytes child;
                if (!c.u32(id) || !c.u32(size) || id != expected++ ||
                    !c.take(size, child) || nodes.size() >= 1'000'000) return false;
                const size_t index = nodes.size();
                nodes.emplace_back();
                nodes[node_index].children.push_back(static_cast<uint32_t>(index));
                if (!decode_visual(child, vertices, indices, material_count, models,
                        nodes, index, depth + 1)) return false;
            }
        }
        return true;
    }
    // Progressive meshes use a sliding window; the first window is full
    // detail. LOD selection can later choose among validated windows.
    if (type != 0 && type != 2) return false;
    if (material >= material_count) return false;
    LevelBytes container, unused;
    if (!chunk(visual, ogf_container, container) || container.size != 24 ||
        chunk(visual, ogf_vertices, unused) || chunk(visual, ogf_indices, unused)) return false;
    Cursor geometry{container};
    uint32_t vb, vbase, vcount, ib, ibase, icount;
    if (!geometry.u32(vb) || !geometry.u32(vbase) || !geometry.u32(vcount) ||
        !geometry.u32(ib) || !geometry.u32(ibase) || !geometry.u32(icount) ||
        vb >= vertices.size() || ib >= indices.size() || icount % 3 ||
        vbase > vertices[vb].vertices.size() ||
        vcount > vertices[vb].vertices.size() - vbase ||
        ibase > indices[ib].size() || icount > indices[ib].size() - ibase) return false;
    if (type == 2)
    {
        LevelBytes swi;
        if (!chunk(visual, ogf_swi, swi) || swi.size < 28) return false;
        Cursor c{swi};
        uint32_t ignored, count, offset;
        for (unsigned n = 0; n < 4; ++n)
            if (!c.u32(ignored)) return false;
        if (!c.u32(count) || !count || count > 65536 ||
            swi.size != 20 + size_t(count) * 8 || !c.u32(offset)) return false;
        LevelBytes first;
        if (!c.take(4, first)) return false;
        const uint32_t tris = uint32_t(first.data[0]) | (uint32_t(first.data[1]) << 8);
        const uint32_t verts = uint32_t(first.data[2]) | (uint32_t(first.data[3]) << 8);
        if (offset > icount || tris > (icount - offset) / 3 || verts > vcount) return false;
        ibase += offset;
        icount = tris * 3;
    }
    LevelModel model;
    model.material = material;
    model.vertices.assign(vertices[vb].vertices.begin() + vbase,
        vertices[vb].vertices.begin() + vbase + vcount);
    model.indices.reserve(icount);
    for (size_t n = ibase; n < size_t(ibase) + icount; ++n)
    {
        if (indices[ib][n] >= vcount) return false;
        model.indices.push_back(indices[ib][n]);
    }
    nodes[node_index].mesh = static_cast<int32_t>(models.size());
    models.push_back(std::move(model));
    return true;
}

bool read_visuals(LevelBytes bytes, const std::vector<VertexBuffer>& vertices,
    const std::vector<std::vector<uint16_t>>& indices, size_t material_count,
    LevelModelData& loaded)
{
    Cursor c{bytes};
    std::vector<LevelBytes> records;
    while (!c.done())
    {
        uint32_t id, size;
        LevelBytes body;
        if (!c.u32(id) || !c.u32(size) || id != records.size() ||
            (id & 0x80000000u) || !c.take(size, body) || records.size() >= 1'000'000)
            return false;
        records.push_back(body);
    }
    loaded.visuals.resize(records.size());
    for (size_t i = 0; i < records.size(); ++i)
        if (!decode_visual(records[i], vertices, indices, material_count,
                loaded.models, loaded.visuals, i, 0)) return false;
    std::vector<uint8_t> state(loaded.visuals.size()), referenced(loaded.visuals.size());
    auto visit = [&](auto&& self, size_t index) -> bool
    {
        if (state[index] == 1) return false;
        if (state[index] == 2) return true;
        state[index] = 1;
        for (uint32_t child : loaded.visuals[index].children)
        {
            if (child >= loaded.visuals.size()) return false;
            referenced[child] = 1;
            if (!self(self, child)) return false;
        }
        state[index] = 2;
        return true;
    };
    for (size_t i = 0; i < loaded.visuals.size(); ++i)
        if (!visit(visit, i)) return false;
    for (size_t i = 0; i < records.size(); ++i)
        if (!referenced[i]) loaded.roots.push_back(static_cast<uint32_t>(i));
    return true;
}
}

bool load_level_models(LevelBytes shaders, LevelBytes vertex_buffers,
    LevelBytes index_buffers, LevelBytes visuals, LevelModelData& result,
    std::string& error)
{
    LevelModelData loaded;
    std::vector<VertexBuffer> vertices;
    std::vector<std::vector<uint16_t>> indices;
    if (!read_materials(shaders, loaded.materials)) error = "invalid level shader table";
    else if (!read_vertices(vertex_buffers, vertices)) error = "unsupported or invalid level vertex buffers";
    else if (!read_indices(index_buffers, indices)) error = "invalid level index buffers";
    else if (!read_visuals(visuals, vertices, indices, loaded.materials.size(), loaded))
        error = "unsupported or invalid level OGF visuals";
    else
    {
        result = std::move(loaded);
        error.clear();
        return true;
    }
    return false;
}
}
