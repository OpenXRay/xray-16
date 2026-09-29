#include "src/Layers/xrRenderVK/LevelModels.h"

#include <cassert>
#include <cstring>
#include <vector>

using namespace xray::render::vulkan;
using Bytes = std::vector<uint8_t>;

static void u32(Bytes& data, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) data.push_back(uint8_t(value >> (8 * i)));
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
}
