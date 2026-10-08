#include "src/Layers/xrRenderVK/LevelModels.h"
#include "src/Layers/xrRenderVK/VisualCatalog.h"
#include "src/Layers/xrRenderVK/SpecialVisuals.h"

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

static void decl(Bytes& data, uint16_t offset, uint8_t type, uint8_t usage, uint8_t index = 0)
{
    data.insert(data.end(), {0, 0, uint8_t(offset), uint8_t(offset >> 8), type, 0, usage, index});
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
    auto input = [](const Bytes &bytes) { return LevelBytes{bytes.data(), bytes.size()}; };
    LevelModelData result;
    std::string error;
    // Game profiles enter the same R2 decoder. Exercise float (SoC) and
    // packed (CS/CoP) declarations, material names and atomic failures.
    const Bytes level_header{14, 0, 1, 0};
    for (auto profile : {LevelGameProfile::SoC, LevelGameProfile::CS, LevelGameProfile::CoP})
    {
        Bytes profile_vb = vb;
        if (profile != LevelGameProfile::SoC)
        {
            profile_vb.clear();
            u32(profile_vb, 1);
            decl(profile_vb, 0, 2, 0);
            decl(profile_vb, 12, 4, 3);
            decl(profile_vb, 16, 4, 6);
            decl(profile_vb, 20, 4, 7);
            decl(profile_vb, 24, 6, 5);
            profile_vb.insert(profile_vb.end(), {0xff, 0, 0, 0, 17, 0, 0, 0});
            u32(profile_vb, 3);
            for (unsigned v = 0; v < 3; ++v)
            {
                f32(profile_vb, float(v));
                f32(profile_vb, 0);
                f32(profile_vb, 0);
                u32(profile_vb, 0x008080ff);
                u32(profile_vb, 0x7f000000);
                u32(profile_vb, 0x3f000000);
                u16(profile_vb, 1024);
                u16(profile_vb, uint16_t(-1024));
            }
        }
        assert(load_game_level_models(profile, input(level_header), input(shaders), input(profile_vb), input(ib), input(visuals), result, error));
        assert(result.models.size() == 1 && result.materials[0].textures == "concrete");
        if (profile != LevelGameProfile::SoC)
        {
            assert(result.models[0].vertices[0].uv[0] > 1.f);
            assert(result.models[0].vertices[0].uv[1] < -0.999f);
        }
        auto truncated = profile_vb;
        truncated.pop_back();
        assert(!load_game_level_models(profile, input(level_header), input(shaders), input(truncated), input(ib), input(visuals), result, error));
        assert(error.find(level_profile_name(profile)) != std::string::npos && error.find("level.geom") != std::string::npos);
        assert(result.models.size() == 1);
        assert(!load_game_level_models(profile, input(Bytes{13, 0, 1, 0}), input(shaders), input(profile_vb), input(ib), input(visuals), result, error));
        assert(error.find("header") != std::string::npos);
        assert(!load_game_level_models(profile, input(level_header), {}, input(profile_vb), input(ib), input(visuals), result, error));
        assert(error.find("shader") != std::string::npos);
    }
    assert(load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
    assert(error.empty() && result.materials.size() == 1 && result.models.size() == 1);
    assert(result.materials[0].textures == "concrete");
    assert(result.models[0].vertices.size() == 3 && result.models[0].vertices[2].position[0] == 2);
    assert(result.models[0].indices[2] == 2);
    Bytes lit_vb;
    u32(lit_vb, 1);
    decl(lit_vb, 0, 2, 0); decl(lit_vb, 12, 2, 3);
    decl(lit_vb, 24, 1, 5); decl(lit_vb, 32, 1, 5, 1);
    decl(lit_vb, 40, 4, 10);
    lit_vb.insert(lit_vb.end(), {0xff, 0, 0, 0, 17, 0, 0, 0});
    u32(lit_vb, 3);
    for (int i = 0; i < 3; ++i)
    {
        for (float value : {float(i), 0.f, 0.f, 0.f, 1.f, 0.f,
                0.f, 0.f, .25f, .75f}) f32(lit_vb, value);
        u32(lit_vb, 0xff4080c0u);
    }
    assert(load_level_models(input(shaders), input(lit_vb), input(ib), input(visuals), result, error));
    assert(result.models[0].lightmap_uv && result.models[0].vertices[1].lightmap_uv[1] == .75f);
    assert(result.models[0].vertices[0].baked[0] > .24f && result.models[0].vertices[0].baked[2] > .74f);
    Bytes packed_lit_vb;
    u32(packed_lit_vb, 1);
    decl(packed_lit_vb, 0, 2, 0); decl(packed_lit_vb, 12, 2, 3);
    decl(packed_lit_vb, 24, 1, 5); decl(packed_lit_vb, 32, 6, 5, 1);
    packed_lit_vb.insert(packed_lit_vb.end(), {0xff, 0, 0, 0, 17, 0, 0, 0});
    u32(packed_lit_vb, 3);
    for (int i = 0; i < 3; ++i)
    {
        for (float value : {float(i), 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f})
            f32(packed_lit_vb, value);
        u16(packed_lit_vb, 16384);
        u16(packed_lit_vb, 8192);
    }
    assert(load_level_models(input(shaders), input(packed_lit_vb), input(ib), input(visuals), result, error));
    assert(result.models[0].vertices[0].lightmap_uv[0] == .5f);
    assert(result.models[0].vertices[0].lightmap_uv[1] == .25f);
    assert(result.visuals.size() == 1 && result.roots.size() == 1 && result.roots[0] == 0);
    VisualRecord standalone;
    assert(parse_ogf_visual(input(visual), standalone, error));
    LevelModel container_model;
    assert(load_container_model(input(vb), input(ib), standalone, container_model, error));
    assert(container_model.vertices.size() == 3 && container_model.indices[2] == 2);
    Bytes corrupt_ib = ib;
    corrupt_ib.back() = 9;
    assert(!load_container_model(input(vb), input(corrupt_ib), standalone, container_model, error));
    assert(container_model.indices[2] == 2);
    Bytes packed_vb;
    u32(packed_vb, 1);
    decl(packed_vb, 0, 2, 0);
    decl(packed_vb, 12, 4, 3);
    decl(packed_vb, 16, 6, 5);
    packed_vb.insert(packed_vb.end(), { 0xff, 0, 0, 0, 17, 0, 0, 0 });
    u32(packed_vb, 3);
    for (int i = 0; i < 3; ++i)
    {
        f32(packed_vb, float(i));
        f32(packed_vb, 0.f);
        f32(packed_vb, 0.f);
        u32(packed_vb, 0xff808080);
        u16(packed_vb, 512);
        u16(packed_vb, 1024);
    }
    assert(load_container_model(input(packed_vb), input(ib), standalone, container_model, error));
    assert(container_model.vertices[1].uv[0] == 0.5f && container_model.vertices[1].normal[0] > 0.f);

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
    Bytes progressive_ib, progressive_container, progressive_swi, progressive_visual, progressive_visuals;
    u32(progressive_ib, 1); u32(progressive_ib, 6);
    for (uint16_t index : {0, 1, 2, 0, 1, 2}) u16(progressive_ib, index);
    u32(progressive_container, 0); u32(progressive_container, 0); u32(progressive_container, 3);
    u32(progressive_container, 0); u32(progressive_container, 0); u32(progressive_container, 6);
    for (unsigned n = 0; n < 4; ++n) u32(progressive_swi, 0);
    u32(progressive_swi, 2);
    u32(progressive_swi, 0); u16(progressive_swi, 2); u16(progressive_swi, 3);
    u32(progressive_swi, 3); u16(progressive_swi, 1); u16(progressive_swi, 3);
    part(progressive_visual, 1, header);
    part(progressive_visual, 21, progressive_container);
    part(progressive_visual, 6, progressive_swi);
    part(progressive_visuals, 0, progressive_visual);
    assert(load_level_models(input(shaders), input(vb), input(progressive_ib),
        input(progressive_visuals), result, error));
    assert(result.models[0].indices.size() == 6 && result.models[0].windows.size() == 2);
    assert(select_slide_window(result.models[0].windows, 0.f, 6).offset == 3);
    Bytes fast_chunks, fast_visual, fast_visuals;
    part(fast_chunks, 21, progressive_container);
    part(fast_chunks, 6, progressive_swi);
    fast_visual = progressive_visual;
    part(fast_visual, 22, fast_chunks);
    part(fast_visuals, 0, fast_visual);
    assert(load_level_models(input(shaders), input(vb), input(progressive_ib), input(fast_visuals), result, error, input(vb), input(progressive_ib)));
    assert(result.models[0].fast && result.models[0].fast->windows.size() == 2);
    assert(select_slide_window(result.models[0].fast->windows, 0.f, 6).offset == 3);
    Bytes incompatible_fast_vb = vb;
    incompatible_fast_vb[0] = 0xff; // The normal geometry remains usable.
    bool skipped_fast_geometry = false;
    assert(load_level_models(input(shaders), input(vb), input(progressive_ib), input(fast_visuals),
        result, error, input(incompatible_fast_vb), input(progressive_ib), {}, &skipped_fast_geometry));
    assert(skipped_fast_geometry && result.models.size() == 1 && !result.models[0].fast && result.models[0].indices.size() == 6);
    assert(load_level_models(input(shaders), input(vb), input(progressive_ib), input(fast_visuals),
        result, error, input(vb), input(progressive_ib)));
    fast_chunks.pop_back(); // Corrupt fast SWI: fail rather than draw a partial model.
    fast_visual = progressive_visual;
    fast_visuals.clear();
    part(fast_visual, 22, fast_chunks);
    part(fast_visuals, 0, fast_visual);
    assert(!load_level_models(input(shaders), input(vb), input(progressive_ib), input(fast_visuals), result, error, input(vb), input(progressive_ib)));
    assert(result.models[0].fast && result.models[0].fast->windows.size() == 2);
    progressive_swi[20 + 8 + 6] = 2; // Index 2 is inactive with only two vertices.
    progressive_visual.clear(); progressive_visuals.clear();
    part(progressive_visual, 1, header);
    part(progressive_visual, 21, progressive_container);
    part(progressive_visual, 6, progressive_swi);
    part(progressive_visuals, 0, progressive_visual);
    assert(!load_level_models(input(shaders), input(vb), input(progressive_ib),
        input(progressive_visuals), result, error));
    assert(result.models[0].windows.size() == 2); // Failed load keeps the old model.
    assert(load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
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
    for (uint8_t independent_type : {uint8_t(8), uint8_t(9), uint8_t(12)})
    {
        header[1] = independent_type;
        visual.clear(); visuals.clear();
        part(visual, 1, header); part(visual, 21, container); part(visuals, 0, visual);
        assert(!load_level_models(input(shaders), input(vb), input(ib), input(visuals), result, error));
        assert(result.visuals.size() == 2);
    }
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

    // Type 6 preserves eight camera-facing impostor facets and its near child.
    Bytes facets;
    for (int face = 0; face < 8; ++face)
        for (int vertex = 0; vertex < 4; ++vertex)
        {
            f32(facets, vertex == 1 || vertex == 2 ? 1.f : 0.f);
            f32(facets, vertex >= 2 ? 1.f : 0.f);
            f32(facets, float(face));
            f32(facets, float(vertex & 1)); f32(facets, float(vertex >> 1));
            u32(facets, 0); u32(facets, 0);
        }
    Bytes child, children, lod, lod_visuals;
    part(child, 1, header); part(child, 21, container);
    part(children, 0, child);
    header[1] = 6;
    part(lod, 1, header); part(lod, 9, children); part(lod, 11, facets);
    part(lod_visuals, 0, lod);
    assert(load_level_models(input(shaders), input(vb), input(ib), input(lod_visuals), result, error));
    assert(result.visuals.size() == 2 && result.models.size() == 9 &&
        result.visuals[0].children[0] == 1 && result.visuals[0].lod_facets[7] == 7);
    assert(select_lod_facet(result.visuals[0].lod_normals, {0, 0, -1}) == 0);
    auto directional = result.visuals[0].lod_normals;
    directional[3] = {1.f, 0.f, 0.f};
    assert(select_lod_facet(directional, {1.f, 0.f, 0.f}) == 3);
    facets.pop_back();
    lod.clear(); lod_visuals.clear();
    part(lod, 1, header); part(lod, 9, children); part(lod, 11, facets);
    part(lod_visuals, 0, lod);
    assert(!load_level_models(input(shaders), input(vb), input(ib), input(lod_visuals), result, error));
    assert(result.models.size() == 9);

    // Tree type 7 has a per-instance transform; tree type 11 additionally
    // selects windows from level.geom's fsL_SWIS table.
    Bytes tree_def, tree_visual, tree_visuals, tree_table, tree_ref;
    for (int i = 0; i < 16; ++i)
        f32(tree_def, i == 0 || i == 5 || i == 10 || i == 15 ? 1.f :
            i == 12 ? 3.f : 0.f);
    tree_def.insert(tree_def.end(), 40, 0);
    header[1] = 7;
    part(tree_visual, 1, header); part(tree_visual, 21, container);
    part(tree_visual, 12, tree_def); part(tree_visuals, 0, tree_visual);
    assert(load_level_models(input(shaders), input(vb), input(ib), input(tree_visuals), result, error));
    assert(result.models[0].vertices[1].position[0] == 4.f);
    assert(result.visuals[0].bounds[9] >= 1.f);
    header[1] = 11;
    tree_visual.clear(); tree_visuals.clear();
    part(tree_visual, 1, header); part(tree_visual, 21, container);
    part(tree_visual, 12, tree_def);
    u32(tree_ref, 0); part(tree_visual, 20, tree_ref);
    part(tree_visuals, 0, tree_visual);
    u32(tree_table, 1);
    tree_table.insert(tree_table.end(), 16, 0);
    u32(tree_table, 1); u32(tree_table, 0); u16(tree_table, 1); u16(tree_table, 3);
    assert(load_level_models(input(shaders), input(vb), input(ib), input(tree_visuals),
        result, error, {}, {}, input(tree_table)));
    assert(result.models[0].windows.size() == 1 && result.models[0].indices.size() == 3);
    tree_table.pop_back();
    assert(!load_level_models(input(shaders), input(vb), input(ib), input(tree_visuals),
        result, error, {}, {}, input(tree_table)));
    assert(result.models[0].windows.size() == 1);

    // The OGF model pool has no type-12 case. DX11 loads its separate
    // level.fog_vol stream; an OGF type-12 must fail without partial visuals.
    header[1] = 12; tree_visual.clear(); tree_visuals.clear();
    part(tree_visual, 1, header); part(tree_visual, 21, container);
    part(tree_visuals, 0, tree_visual);
    assert(!load_level_models(input(shaders), input(vb), input(ib), input(tree_visuals), result, error));
    assert(result.models[0].windows.size() == 1);
}
