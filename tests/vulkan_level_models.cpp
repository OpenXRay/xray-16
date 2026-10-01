#include "src/Layers/xrRenderVK/LevelModels.h"

#include <cassert>
#include <cstring>
#include <limits>
#include <vector>

using namespace xray::render::vulkan;
using Bytes = std::vector<uint8_t>;

static void u32(Bytes& data, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) data.push_back(uint8_t(value >> (8 * i)));
}

static void u16(Bytes& data, uint16_t value)
{
    data.push_back(uint8_t(value));
    data.push_back(uint8_t(value >> 8));
}

static void f32(Bytes& data, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    u32(data, bits);
}

static void decl(Bytes& data, uint16_t offset, uint8_t type, uint8_t usage)
{
    data.insert(data.end(), {0, 0, uint8_t(offset), uint8_t(offset >> 8), type, 0, usage, 0});
}

static void part(Bytes& data, uint32_t id, const Bytes& value)
{
    u32(data, id);
    u32(data, static_cast<uint32_t>(value.size()));
    data.insert(data.end(), value.begin(), value.end());
}

int main()
{
    assert(classify_surface_material("default", "concrete") == SurfaceMode::Opaque);
    assert(classify_surface_material("deffer_aref", "foliage/oak") == SurfaceMode::AlphaTest);
    assert(classify_surface_material("default_blend", "effects/smoke") == SurfaceMode::Transparent);
    assert(classify_surface_material("glass", "window") == SurfaceMode::Transparent);

    Bytes shaders{1, 0, 0, 0};
    const char material[] = "def_shaders\\default/concrete";
    shaders.insert(shaders.end(), material, material + sizeof(material));
    Bytes vb;
    u32(vb, 1);
    decl(vb, 0, 2, 0);
    decl(vb, 12, 2, 3);
    decl(vb, 24, 1, 5);
    vb.insert(vb.end(), {0xff, 0, 0, 0, 17, 0, 0, 0});
    u32(vb, 3);
    for (int i = 0; i < 3; ++i)
    {
        float vertex[]{float(i), 0, 0, 0, 0, 1, float(i) / 2, 0};
        const auto* bytes = reinterpret_cast<const uint8_t*>(vertex);
        vb.insert(vb.end(), bytes, bytes + sizeof(vertex));
    }
    Bytes ib;
    u32(ib, 1);
    u32(ib, 3);
    ib.insert(ib.end(), {0, 0, 1, 0, 2, 0});
    Bytes visual, header(44, 0), container;
    header[0] = 4; // xrOGF_FormatVersion, MT_NORMAL, shader index zero
    part(visual, 1, header);
    u32(container, 0); u32(container, 0); u32(container, 3);
    u32(container, 0); u32(container, 0); u32(container, 3);
    part(visual, 21, container);
    Bytes visuals;
    part(visuals, 0, visual);
    auto input = [](const Bytes& bytes) { return LevelBytes{bytes.data(), bytes.size()}; };
    LevelModelData result;
    std::string error;
    assert(load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
    assert(error.empty() && result.materials.size() == 1 && result.models.size() == 1);
    assert(result.materials[0].textures == "concrete");
    assert(result.models[0].vertices.size() == 3 && result.models[0].vertices[2].position[0] == 2);
    assert(result.models[0].indices[2] == 2);
    assert(result.visuals.size() == 1 && result.roots.size() == 1 && result.roots[0] == 0);

    // Level portal records contain storage for six points even when only a
    // triangle or quad is active; unused point slots need not be initialized.
    result.visuals.push_back(result.visuals[0]);
    Bytes portal_data;
    u16(portal_data, 0); u16(portal_data, 1);
    const float portal_vertices[6][3] = {
        {-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0},
        {std::numeric_limits<float>::quiet_NaN(), 0, 0},
        {std::numeric_limits<float>::quiet_NaN(), 0, 0}};
    for (const auto& vertex : portal_vertices)
        for (float coordinate : vertex)
            f32(portal_data, coordinate);
    u32(portal_data, 4);
    Bytes sector0_links, sector1_links, sector0, sector1, sector_data;
    u16(sector0_links, 0); u16(sector1_links, 0);
    part(sector0, 1, sector0_links);
    part(sector0, 2, Bytes(4));
    part(sector1, 1, sector1_links);
    Bytes sector1_root; u32(sector1_root, 1);
    part(sector1, 2, sector1_root);
    part(sector_data, 0, sector0);
    part(sector_data, 1, sector1);
    assert(parse_level_visibility(input(portal_data), input(sector_data), result, error));
    assert(error.empty() && result.sectors.size() == 2 && result.portals.size() == 1);
    assert(result.sectors[0].root == 0 && result.sectors[1].root == 1);
    assert(result.portals[0].vertices.size() == 4 && result.portals[0].center[0] == 0 &&
        result.portals[0].center[1] == 0 && result.portals[0].radius > 1.4f);
    // Reject one-sided portal links atomically so the runtime can fail open.
    Bytes invalid_sector_data, invalid_sector1;
    part(invalid_sector1, 1, Bytes{});
    Bytes invalid_root; u32(invalid_root, 1);
    part(invalid_sector1, 2, invalid_root);
    part(invalid_sector_data, 0, sector0);
    part(invalid_sector_data, 1, invalid_sector1);
    assert(!parse_level_visibility(input(portal_data), input(invalid_sector_data), result, error));
    assert(result.sectors.size() == 2 && result.portals.size() == 1);

    // A linked hierarchy keeps stable visual IDs and draws its child once.
    Bytes hierarchy, links;
    header[1] = 1;
    part(hierarchy, 1, header);
    u32(links, 1); u32(links, 1);
    part(hierarchy, 10, links);
    header[1] = 2;
    visual.clear();
    part(visual, 1, header); part(visual, 21, container);
    Bytes swi;
    for (int n = 0; n < 4; ++n) u32(swi, 0);
    u32(swi, 1); u32(swi, 0);
    swi.insert(swi.end(), {1, 0, 3, 0});
    part(visual, 6, swi);
    visuals.clear();
    part(visuals, 0, hierarchy); part(visuals, 1, visual);
    assert(load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
    assert(result.models.size() == 1 && result.visuals.size() == 2 &&
        result.roots.size() == 1 && result.roots[0] == 0 &&
        result.visuals[0].children.size() == 1 && result.visuals[0].children[0] == 1 &&
        result.visuals[1].mesh == 0);
    Bytes embedded;
    part(embedded, 0, visual);
    hierarchy.clear(); links.clear();
    header[1] = 1;
    part(hierarchy, 1, header); part(hierarchy, 9, embedded);
    visuals.clear(); part(visuals, 0, hierarchy);
    assert(load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
    assert(result.visuals.size() == 2 && result.visuals[0].children[0] == 1 &&
        result.roots.size() == 1 && result.models.size() == 1);
    // Reject a linked cycle without replacing the last good scene graph.
    hierarchy.clear(); links.clear();
    u32(links, 1); u32(links, 0);
    part(hierarchy, 1, header); part(hierarchy, 10, links);
    visuals.clear(); part(visuals, 0, hierarchy);
    assert(!load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
    visuals.clear(); part(visuals, 0, visual);
    ib.back() = 9;
    assert(!load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
    assert(result.models.size() == 1); // failed load leaves previous data intact
    ib.back() = 0;
    header[1] = 3; // skeletal visuals require an independent path
    visual.clear(); visuals.clear();
    part(visual, 1, header); part(visual, 21, container); part(visuals, 0, visual);
    assert(!load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));

    // A failed replacement preserved the previous hierarchy. A subsequent
    // successful level replacement and an unload/reload must both rebuild IDs.
    assert(result.visuals.size() == 2 && result.visuals[0].children[0] == 1);
    header[1] = 0;
    visual.clear(); visuals.clear();
    part(visual, 1, header); part(visual, 21, container); part(visuals, 0, visual);
    assert(load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
    assert(result.visuals.size() == 1 && result.visuals[0].mesh == 0 &&
        result.visuals[0].children.empty() && result.roots.size() == 1);
    result = {};
    assert(result.visuals.empty() && result.models.empty());
    assert(load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
    assert(result.visuals.size() == 1 && result.models.size() == 1);
}
