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
    ib.back() = 9;
    assert(!load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
    assert(result.models.size() == 1); // failed load leaves previous data intact
    ib.back() = 0;
    header[1] = 3; // skeletal visuals require an independent path
    visual.clear(); visuals.clear();
    part(visual, 1, header); part(visual, 21, container); part(visuals, 0, visual);
    assert(!load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
}
