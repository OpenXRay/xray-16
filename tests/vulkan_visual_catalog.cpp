#include "src/Layers/xrRenderVK/VisualCatalog.h"

#include <cassert>
#include <cstring>

using namespace xray::render::vulkan;
using Bytes = std::vector<uint8_t>;

static void u32(Bytes& data, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) data.push_back(uint8_t(value >> (8 * i)));
}

static void chunk(Bytes& data, uint32_t id, const Bytes& body)
{
    u32(data, id);
    u32(data, static_cast<uint32_t>(body.size()));
    data.insert(data.end(), body.begin(), body.end());
}

static Bytes visual(uint8_t type)
{
    Bytes header(44);
    header[0] = 4;
    header[1] = type;
    const float radius = 7.f;
    std::memcpy(header.data() + 4 + 9 * sizeof(float), &radius, sizeof(radius));
    Bytes data;
    chunk(data, 1, header);
    return data;
}

int main()
{
    Bytes table;
    for (uint8_t type = 0; type <= 12; ++type)
    {
        Bytes record = visual(type);
        if (type == 1)
        {
            Bytes links;
            u32(links, 1);
            u32(links, 0);
            chunk(record, 10, links);
        }
        if (type == 3)
        {
            Bytes embedded;
            chunk(embedded, 0, visual(5));
            chunk(record, 9, embedded);
        }
        chunk(table, type, record);
    }
    auto bytes = [](const Bytes& data) { return LevelBytes{data.data(), data.size()}; };
    std::vector<VisualRecord> records;
    std::string error;
    assert(parse_level_visuals(bytes(table), records, error));
    assert(error.empty() && records.size() == 13);
    assert(records[1].linked_children.size() == 1 && records[1].linked_children[0] == 0);
    assert(records[1].bounds[9] == 7.f);
    assert(records[3].embedded_children.size() == 1 && records[3].embedded_children[0].type == 5);
    Bytes cyclic = visual(1), links;
    u32(links, 1); u32(links, 0);
    chunk(cyclic, 10, links);
    Bytes bad_table;
    chunk(bad_table, 0, cyclic);
    assert(!parse_level_visuals(bytes(bad_table), records, error));
    assert(records.size() == 13);
    Bytes invalid = visual(13);
    VisualRecord record;
    assert(!parse_ogf_visual(bytes(invalid), record, error));
}
