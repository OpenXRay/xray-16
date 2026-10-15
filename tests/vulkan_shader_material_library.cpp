#include "xrEngine/stdafx.h"
#include "src/Layers/xrRenderVK/ShaderMaterialLibrary.h"
#include "src/Layers/xrRenderVK/LevelPassGraph.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <cstdio>

using namespace xray::render::vulkan;
using Bytes = std::vector<uint8_t>;

static void write32(Bytes& bytes, uint32_t n)
{ for (unsigned i = 0; i < 4; ++i) bytes.push_back(static_cast<uint8_t>(n >> (i * 8))); }
static void chunk(Bytes& bytes, uint32_t id, const Bytes& payload)
{ write32(bytes, id); write32(bytes, static_cast<uint32_t>(payload.size())); bytes.insert(bytes.end(), payload.begin(), payload.end()); }
static void property(Bytes& bytes, uint32_t id, const char* name, uint32_t value)
{ write32(bytes, id); bytes.insert(bytes.end(), name, name + std::strlen(name) + 1); write32(bytes, value); }
static void texture_property(Bytes& bytes, const char* name, const char* value)
{
    write32(bytes, 3);
    bytes.insert(bytes.end(), name, name + std::strlen(name) + 1);
    const size_t offset = bytes.size();
    bytes.resize(offset + 64);
    assert(std::strlen(value) < 64);
    std::memcpy(bytes.data() + offset, value, std::strlen(value));
}
static void matrix_property(Bytes& bytes, const char* name, const char* value)
{
    write32(bytes, 1);
    bytes.insert(bytes.end(), name, name + std::strlen(name) + 1);
    const size_t offset = bytes.size();
    bytes.resize(offset + 64);
    assert(std::strlen(value) < 64);
    std::memcpy(bytes.data() + offset, value, std::strlen(value));
}
static void tessellation_property(Bytes& bytes, uint32_t selected)
{
    write32(bytes, 7);
    const char* name = "Tessellation";
    bytes.insert(bytes.end(), name, name + std::strlen(name) + 1);
    write32(bytes, selected);
    write32(bytes, 4);
    constexpr const char* labels[]{"NO_TESS", "TESS_PN", "TESS_HM", "TESS_PN+HM"};
    for (uint32_t i = 0; i < 4; ++i)
    {
        write32(bytes, i);
        const auto start = bytes.size();
        bytes.resize(start + 64);
        std::memcpy(bytes.data() + start, labels[i], std::strlen(labels[i]));
    }
}
static void replace_saved_string_property(Bytes& bytes, uint32_t type,
    const char* name, const char* value)
{
    Bytes marker;
    write32(marker, type);
    marker.insert(marker.end(), name, name + std::strlen(name) + 1);
    auto position = std::search(bytes.begin() + 176, bytes.end(), marker.begin(), marker.end());
    assert(position != bytes.end() && std::strlen(value) < 64);
    const size_t offset = size_t(position - bytes.begin()) + marker.size();
    assert(offset + 64 <= bytes.size());
    std::fill(bytes.begin() + offset, bytes.begin() + offset + 64, 0);
    std::memcpy(bytes.data() + offset, value, std::strlen(value));
}
static Bytes record(const char* cls, const char* name, bool blend, uint32_t alpha_ref = 128)
{
    Bytes result(176, 0);
    for (size_t i = 0; i < 8; ++i) result[i] = static_cast<uint8_t>(cls[7 - i]);
    std::memcpy(result.data() + 8, name, std::strlen(name));
    result[172] = std::strcmp(cls, "PARTICLE") == 0 ||
        std::strcmp(cls, "D_STILL ") == 0 ? 0 :
        std::strcmp(cls, "S_SET   ") == 0 ? 4 : 1;
    property(result, 0, "General", 0); result.resize(result.size() - 4);
    write32(result, 4); result.insert(result.end(), {'P','r','i','o','r','i','t','y',0});
    write32(result, 1); write32(result, 0); write32(result, 3);
    property(result, 6, "Strict sorting", 0);
    property(result, 0, "Base Texture", 0); result.resize(result.size() - 4);
    for (const char* prop : {"Name", "Transform"})
    {
        write32(result, prop[0] == 'N' ? 3 : 1);
        result.insert(result.end(), prop, prop + std::strlen(prop) + 1);
        result.resize(result.size() + 64);
    }
    const bool aref = std::strcmp(cls, "LM_AREF ") == 0 ||
        std::strcmp(cls, "V_AREF  ") == 0 || std::strcmp(cls, "MODEL   ") == 0 ||
        std::strcmp(cls, "S_SET   ") == 0 || std::strcmp(cls, "PARTICLE") == 0;
    if (aref)
    {
        write32(result, 4); const char* alpha = "Alpha ref";
        result.insert(result.end(), alpha, alpha + std::strlen(alpha) + 1);
        write32(result, alpha_ref); write32(result, 0); write32(result, 255);
    }
    if (std::strcmp(cls, "LM_AREF ") == 0 || std::strcmp(cls, "V_AREF  ") == 0 ||
        std::strcmp(cls, "D_TREE  ") == 0 || std::strcmp(cls, "D_STILL ") == 0)
        property(result, 6, "Alpha-blend", blend ? 1 : 0);
    if (std::strcmp(cls, "MODEL   ") == 0)
        property(result, 6, "Use alpha-channel", blend ? 1 : 0);
    if (std::strcmp(cls, "LmEbB   ") == 0 || std::strcmp(cls, "MODELEbB") == 0)
    {
        write32(result, 0);
        result.insert(result.end(), {'E','n','v','i','r','o','n','m','e','n','t',' ','m','a','p',0});
        property(result, 6, "Alpha-Blend", blend ? 1 : 0);
    }
    return result;
}

static void animated_texture_matrices()
{
    Bytes table, records, file;
    const auto add_matrix = [&](const char* name, uint32_t mode, uint32_t flags)
    {
        table.insert(table.end(), name, name + std::strlen(name) + 1);
        write32(table, mode); write32(table, flags);
        for (size_t i = 0; i < 5; ++i)
        {
            WaveForm wave;
            wave.arg[0] = i == 0 ? 2.f : i == 1 ? 3.f : i == 3 ? .25f : 0.f;
            const auto* begin = reinterpret_cast<const uint8_t*>(&wave);
            table.insert(table.end(), begin, begin + sizeof(wave));
        }
    };
    add_matrix("mod\\moving", 1, 5); // scale + scroll
    add_matrix("mod\\reflection", 2, 0);
    auto base = record("LM      ", "mod\\moving_base", false);
    matrix_property(base, "Transform", "mod\\moving");
    chunk(records, 1, base);
    auto detail = record("BmmDold ", "mod\\moving_detail", false);
    detail[172] = 3;
    write32(detail, 0);
    detail.insert(detail.end(), {'D','e','t','a','i','l',' ','m','a','p',0});
    texture_property(detail, "Name", "mod\\sand");
    matrix_property(detail, "Transform", "mod\\moving");
    for (const char* channel : {"R2-R", "R2-G", "R2-B", "R2-A"})
        texture_property(detail, channel, "$null");
    chunk(records, 2, detail);
    auto environment = record("MODELEbB", "mod\\moving_environment", true);
    texture_property(environment, "Name", "mod\\sky");
    matrix_property(environment, "Transform", "mod\\reflection");
    chunk(records, 3, environment);
    chunk(file, 1, table); chunk(file, 2, records);
    IReader reader(file.data(), file.size());
    ShaderMaterialLibrary library;
    std::string error;
    assert(library.load(reader, error));
    for (const auto& [name, slot] : {std::pair<const char*, size_t>{"mod\\moving_base", 0},
             {"mod\\moving_detail", 1}, {"mod\\moving_environment", 2}})
    {
        const auto* material = library.find(name);
        assert(material && material->pipeline_supported && material->matrices.enabled[slot]);
    }
    const auto& moving = library.find("mod\\moving_base")->matrices.values[0];
    const auto first = moving.evaluate(1.f, nullptr);
    const auto second = moving.evaluate(2.f, nullptr);
    assert(first[0][0] == 2.f && first[1][1] == 3.f &&
        second[0][3] > first[0][3] && second[1][3] == first[1][3]);
    const float view[16]{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    const auto reflection = library.find("mod\\moving_environment")->matrices.values[2].evaluate(2.f, view);
    assert(reflection[0][0] == .5f && reflection[0][3] == .5f &&
        reflection[1][1] == -.5f && reflection[1][3] == .5f);
    Bytes damaged;
    table.pop_back();
    chunk(damaged, 1, table); chunk(damaged, 2, records);
    IReader bad(damaged.data(), damaged.size());
    assert(!library.load(bad, error) && error.find("matrix table is truncated") != std::string::npos);
}

// Exhaust the five independently serialized S_SET flags for each mode and
// consumer, including the contexts that must reject the same definition.
static void screen_context_matrix()
{
    Bytes records, file;
    constexpr const char* fields[]{"Texture clamp", "Z-test", "Z-write", "Lighting", "Fog"};
    for (unsigned blend = 0; blend != 10; ++blend)
        for (unsigned flags = 0; flags != 32; ++flags)
        {
            const std::string name = "matrix\\set_" + std::to_string(blend) + "_" + std::to_string(flags);
            Bytes item = record("S_SET   ", name.c_str(), false, 73);
            write32(item, 7);
            item.insert(item.end(), {'B','l','e','n','d','i','n','g',0});
            write32(item, blend); write32(item, 0);
            for (unsigned bit = 0; bit != 5; ++bit)
                property(item, 6, fields[bit], (flags >> bit) & 1u);
            chunk(records, blend * 32 + flags, item);
        }
    chunk(file, 2, records);
    IReader reader(file.data(), file.size());
    ShaderMaterialLibrary materials;
    std::string error;
    assert(materials.load(reader, error) && materials.size() == 320);
    for (unsigned blend = 0; blend != 10; ++blend)
        for (unsigned flags = 0; flags != 32; ++flags)
        {
            const std::string name = "matrix\\set_" + std::to_string(blend) + "_" + std::to_string(flags);
            const auto* definition = materials.find(name);
            assert(definition && definition->blending == int(blend));
            assert(definition->pass.clamp == bool(flags & 1) &&
                definition->pass.depth_test == bool(flags & 2) &&
                definition->pass.depth_write == bool(flags & 4) &&
                definition->pass.lighting == bool(flags & 8) &&
                definition->pass.fog == bool(flags & 16));
            for (unsigned context = 0; context != 4; ++context)
            {
                // UI, particle, level/decal, OGF. Particle and OGF also use HUD.
                const bool ui = context == 0, particle = context == 1;
                const bool world = context >= 2;
                const bool expected = ui ? (flags & ~1u) == 0 :
                    particle ? blend <= 5 && (flags & ~(1u | 2u | 4u)) == 0 &&
                        (flags & 2u) && bool(flags & 4u) == (blend == 0) :
                    (flags & 2u) && (flags & ~(1u | 2u | (blend == 0 ? 4u : 0u) |
                        (blend == 1 && context == 2 ? 16u : 0u))) == 0 &&
                        (blend != 0 || (flags & 4u)) &&
                        (context != 3 || !(flags & 16u));
                SurfaceMode mode{};
                int alpha = -1, selected = -1;
                uint32_t extracted_flags = 0;
                MaterialDefinition extracted;
                const bool resolved = materials.resolve(name, mode, error, &alpha, &selected,
                    particle, ui, world, false, &extracted_flags, nullptr, &extracted, context == 3);
                if (resolved != expected)
                {
                    std::fprintf(stderr, "S_SET mode=%u flags=%u context=%u: expected=%d got=%d %s\n",
                        blend, flags, context, expected, resolved, error.c_str());
                    assert(false);
                }
                if (!resolved)
                {
                    assert(error.find("property=") != std::string::npos ||
                        error.find("blending=") != std::string::npos);
                    continue;
                }
                assert(extracted_flags == definition->screen_flags && selected == int(blend) &&
                    alpha == (blend == 7 ? 0 : 73) &&
                    extracted.pass.clamp == definition->pass.clamp);
                if (ui)
                {
                    RenderPassGraph graph;
                    assert(build_render_pass_graph({RenderConsumer::Ui, mode,
                        false, false, false, false, 0, int(blend)}, graph, error));
                    assert(graph.count == 1 && graph.passes[0].target == LevelPassTarget::Ui &&
                        !graph.passes[0].depth_test && !graph.passes[0].depth_write);
                }
                else if (particle)
                {
                    for (bool hud : {false, true})
                    {
                        RenderPassGraph graph;
                        RenderPassSelection selection{RenderConsumer::Particle, mode};
                        selection.hud = hud;
                        selection.blend_mode = int(blend);
                        selection.particle_screen_set = true;
                        assert(build_render_pass_graph(selection, graph, error));
                        assert(graph.count == 1 && graph.passes[0].depth_test &&
                            graph.passes[0].depth_write == (blend == 0) &&
                            graph.passes[0].target == (hud ? LevelPassTarget::Hud : LevelPassTarget::Forward));
                    }
                }
                else if (context == 2)
                {
                    LevelPassGraph graph;
                    LevelPassSelection selection;
                    selection.mode = mode;
                    selection.screen_blend = int(blend);
                    selection.wallmark_multiply = blend == 6;
                    selection.glow_alpha_add = blend == 5;
                    selection.fog = bool(flags & 16);
                    assert(build_level_pass_graph(selection, graph, error));
                    assert(graph.count == 1 && graph.passes[0].depth_test &&
                        graph.passes[0].depth_write == (blend == 0) &&
                        graph.passes[0].target == LevelPassTarget::Forward);
                }
                else
                {
                    for (bool hud : {false, true})
                    {
                        RenderPassGraph graph;
                        RenderPassSelection selection{RenderConsumer::Model, mode};
                        selection.hud = hud;
                        selection.screen_blend = int(blend);
                        assert(build_render_pass_graph(selection, graph, error));
                        assert(graph.count == 1 && graph.passes[0].depth_test &&
                            graph.passes[0].depth_write == (blend == 0) &&
                            graph.passes[0].target == (hud ? LevelPassTarget::Hud : LevelPassTarget::Forward));
                    }
                }
            }
        }
}

static void particle_context_matrix()
{
    Bytes records, file;
    for (unsigned blend = 0; blend < 6; ++blend)
        for (unsigned clamp = 0; clamp < 2; ++clamp)
        {
            const std::string name = "matrix\\particle_" + std::to_string(blend) + "_" +
                std::to_string(clamp);
            Bytes item = record("PARTICLE", name.c_str(), false, 91);
            write32(item, 7);
            item.insert(item.end(), {'B','l','e','n','d','i','n','g',0});
            write32(item, blend); write32(item, 0);
            property(item, 6, "Texture clamp", clamp);
            chunk(records, blend * 2 + clamp, item);
        }
    chunk(file, 2, records);
    IReader reader(file.data(), file.size());
    ShaderMaterialLibrary materials;
    std::string error;
    assert(materials.load(reader, error) && materials.size() == 12);
    for (unsigned blend = 0; blend < 6; ++blend)
        for (unsigned clamp = 0; clamp < 2; ++clamp)
        {
            const std::string name = "matrix\\particle_" + std::to_string(blend) + "_" +
                std::to_string(clamp);
            SurfaceMode mode{};
            int alpha = -1, selected = -1;
            MaterialDefinition definition;
            assert(materials.resolve(name, mode, error, &alpha, &selected,
                true, false, false, false, nullptr, nullptr, &definition));
            assert(selected == int(blend) && alpha == (blend == 0 ? 200 : 91) &&
                definition.pass.clamp == bool(clamp));
            for (bool hud : {false, true})
            {
                RenderPassGraph graph;
                RenderPassSelection selection{RenderConsumer::Particle,
                    blend == 0 && !hud ? SurfaceMode::AlphaTest : SurfaceMode::Transparent};
                selection.blend_mode = int(blend);
                selection.hud = hud;
                assert(build_render_pass_graph(selection, graph, error));
                assert(graph.count == 1 && graph.passes[0].depth_test &&
                    graph.passes[0].depth_write == (hud || blend == 0) &&
                    graph.passes[0].target == (hud ? LevelPassTarget::Hud :
                        blend == 0 ? LevelPassTarget::GBuffer : LevelPassTarget::Forward));
            }
        }
}

int main()
{
    animated_texture_matrices();
    screen_context_matrix();
    particle_context_matrix();
    Bytes list;
    chunk(list, 0, record("LM_AREF ", "def_shaders\\leaf", false));
    chunk(list, 1, record("LM_AREF ", "def_shaders\\glass", true));
    chunk(list, 2, record("D_TREE  ", "trees\\leaf", false));
    Bytes static_tree = record("D_TREE  ", "trees\\object_lod", false);
    property(static_tree, 6, "Object LOD", 1);
    chunk(list, 84, static_tree);
    chunk(list, 3, record("D_TREE  ", "trees\\blended", true));
    chunk(list, 4, record("D_STILL ", "detail\\solid", false));
    Bytes modded = record("CUSTOM  ", "mod\\unrecognized", false);
    modded[172] = 17; // An archive can contain blender revisions newer than the engine.
    chunk(list, 5, modded);
    chunk(list, 6, record("LM_AREF ", "mod\\cutout", false, 200));
    Bytes screen_blend = record("S_SET   ", "effects\\wallmarkblend", false, 0);
    write32(screen_blend, 7);
    const char* blend_name = "Blending";
    screen_blend.insert(screen_blend.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_blend, 6); write32(screen_blend, 0);
    property(screen_blend, 6, "Z-test", 1);
    chunk(list, 7, screen_blend);
    Bytes screen_set = record("S_SET   ", "hud\\opaque", false, 0);
    write32(screen_set, 7);
    screen_set.insert(screen_set.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_set, 0); write32(screen_set, 0);
    chunk(list, 8, screen_set);
    Bytes additive = record("S_SET   ", "effects\\additive", false, 32);
    write32(additive, 7);
    additive.insert(additive.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(additive, 2); write32(additive, 0);
    chunk(list, 9, additive);
    Bytes particle_set = record("PARTICLE", "effects\\sprite_set", false, 42);
    write32(particle_set, 7);
    particle_set.insert(particle_set.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(particle_set, 0); write32(particle_set, 0);
    chunk(list, 42, particle_set); // Modded archives may leave gaps in chunk IDs.
    Bytes screen_alpha = record("S_SET   ", "hud\\alpha", false, 64);
    write32(screen_alpha, 7);
    screen_alpha.insert(screen_alpha.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_alpha, 1); write32(screen_alpha, 0);
    chunk(list, 43, screen_alpha);
    Bytes terrain = record("BmmDold ", "levels\\modded_ground", false);
    terrain[172] = 3;
    write32(terrain, 0);
    terrain.insert(terrain.end(), {'D','e','t','a','i','l',' ','m','a','p',0});
    for (const char* property_name : {"R2-R", "R2-G", "R2-B", "R2-A"})
    {
        write32(terrain, 3);
        terrain.insert(terrain.end(), property_name, property_name + std::strlen(property_name) + 1);
        const size_t start = terrain.size();
        terrain.resize(start + 64);
        std::memcpy(terrain.data() + start, "detail\\modded", 13);
    }
    chunk(list, 51, terrain);
    Bytes modern_terrain = record("BmmD    ", "mod\\unusual_four_channel", false);
    modern_terrain[172] = 3;
    write32(modern_terrain, 0);
    modern_terrain.insert(modern_terrain.end(), {'D','e','t','a','i','l',' ','m','a','p',0});
    for (const char* channel : {"R2-R", "R2-G", "R2-B", "R2-A"})
        texture_property(modern_terrain, channel, channel);
    chunk(list, 81, modern_terrain);
    Bytes named_terrain = record("BmmDold ", "levels\\zaton_asfalt_fixture", false);
    named_terrain[172] = 3;
    write32(named_terrain, 0);
    named_terrain.insert(named_terrain.end(), {'D','e','t','a','i','l',' ','m','a','p',0});
    texture_property(named_terrain, "Name", "detail\\serialized_but_unused");
    matrix_property(named_terrain, "Transform", "terrain_scroll_unused");
    for (const char* channel : {"R2-R", "R2-G", "R2-B", "R2-A"})
        texture_property(named_terrain, channel, "detail\\asfalt_channel");
    chunk(list, 110, named_terrain);
    Bytes null_terrain = record("BmmD    ", "mod\\single_with_null_channels", false);
    null_terrain[172] = 3;
    write32(null_terrain, 0);
    null_terrain.insert(null_terrain.end(), {'D','e','t','a','i','l',' ','m','a','p',0});
    texture_property(null_terrain, "Name", "mod\\one_detail");
    for (const char* channel : {"R2-R", "R2-G", "R2-B", "R2-A"})
        texture_property(null_terrain, channel, "$null");
    chunk(list, 82, null_terrain);
    Bytes particle_add = record("PARTICLE", "effects\\flash_add", false, 32);
    write32(particle_add, 7);
    particle_add.insert(particle_add.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(particle_add, 2); write32(particle_add, 0);
    chunk(list, 44, particle_add);
    Bytes particle_alpha_add = record("PARTICLE", "effects\\flash_alpha_add", false, 32);
    write32(particle_alpha_add, 7);
    particle_alpha_add.insert(particle_alpha_add.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(particle_alpha_add, 5); write32(particle_alpha_add, 0);
    chunk(list, 45, particle_alpha_add);
    Bytes particle_multiply = record("PARTICLE", "effects\\multiply", false, 32);
    write32(particle_multiply, 7);
    particle_multiply.insert(particle_multiply.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(particle_multiply, 3); write32(particle_multiply, 0);
    chunk(list, 46, particle_multiply);
    Bytes screen_multiply_2x = record("S_SET   ", "effects\\screen_multiply_2x", false, 32);
    write32(screen_multiply_2x, 7);
    screen_multiply_2x.insert(screen_multiply_2x.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_multiply_2x, 4); write32(screen_multiply_2x, 0);
    chunk(list, 47, screen_multiply_2x);
    Bytes wallmark_mult = record("S_SET   ", "effects\\wallmarkmult", false, 0);
    write32(wallmark_mult, 7);
    wallmark_mult.insert(wallmark_mult.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(wallmark_mult, 6); write32(wallmark_mult, 0);
    property(wallmark_mult, 6, "Z-test", 1);
    chunk(list, 51, wallmark_mult);
    chunk(list, 52, record("MODEL   ", "models\\fur", true, 32));
    for (uint32_t selected = 0; selected <= 4; ++selected)
    {
        Bytes tessellated = record("MODEL   ",
            ("models\\tess_" + std::to_string(selected)).c_str(), false);
        tessellated[172] = 2;
        tessellation_property(tessellated, selected);
        chunk(list, 120 + selected, tessellated);
    }
    chunk(list, 53, record("MODEL   ", "models\\glass", true, 4));
    Bytes reflection = record("LmEbB   ", "mod\\arbitrary_reflection", false);
    write32(reflection, 0);
    const char env_scope[] = "Environment map";
    reflection.insert(reflection.end(), env_scope, env_scope + sizeof(env_scope));
    write32(reflection, 3);
    reflection.insert(reflection.end(), {'N','a','m','e',0});
    const size_t env_value = reflection.size();
    reflection.resize(env_value + 64);
    std::memcpy(reflection.data() + env_value, "mod\\cube", 9);
    chunk(list, 54, reflection);
    Bytes single_detail = record("BmmDold ", "mod\\plain_stone", false);
    write32(single_detail, 0);
    const char detail_scope[] = "Detail map";
    single_detail.insert(single_detail.end(), detail_scope, detail_scope + sizeof(detail_scope));
    texture_property(single_detail, "Name", "mod\\unusual_detail");
    chunk(list, 55, single_detail);
    chunk(list, 66, record("V       ", "mod\\plain_vertex", false));
    Bytes lightmapped_detail = record("LmBmmD  ", "mod\\lightmapped_detail", false);
    write32(lightmapped_detail, 0);
    lightmapped_detail.insert(lightmapped_detail.end(), detail_scope, detail_scope + sizeof(detail_scope));
    texture_property(lightmapped_detail, "Name", "mod\\another_detail");
    chunk(list, 67, lightmapped_detail);
    Bytes environment_constant = record("LaEmB   ", "mod\\glowing_wall", false);
    environment_constant[172] = 0;
    write32(environment_constant, 0);
    environment_constant.insert(environment_constant.end(), env_scope, env_scope + sizeof(env_scope));
    texture_property(environment_constant, "Name", "mod\\glow_map");
    write32(environment_constant, 2);
    environment_constant.insert(environment_constant.end(), {'C','o','n','s','t','a','n','t',0});
    const size_t constant_value = environment_constant.size();
    environment_constant.resize(constant_value + 64);
    std::memcpy(environment_constant.data() + constant_value, "pulse", 5);
    chunk(list, 56, environment_constant);
    chunk(list, 57, record("BmmDold ", "mod\\legacy_ground", false));
    Bytes model_environment = record("MODELEbB", "mod\\reflective_npc", true);
    write32(model_environment, 0);
    model_environment.insert(model_environment.end(), env_scope, env_scope + sizeof(env_scope));
    texture_property(model_environment, "Name", "mod\\npc_cube");
    chunk(list, 58, model_environment);
    Bytes missing_environment = record("LaEmB   ", "mod\\missing_env", false);
    missing_environment[172] = 0;
    chunk(list, 59, missing_environment);
    Bytes reflection_blended = record("LmEbB   ", "mod\\blended_reflection", true);
    write32(reflection_blended, 0);
    reflection_blended.insert(reflection_blended.end(), env_scope, env_scope + sizeof(env_scope));
    texture_property(reflection_blended, "Name", "mod\\other_cube");
    chunk(list, 60, reflection_blended);
    Bytes model_opaque = record("MODELEbB", "mod\\plain_npc", false);
    chunk(list, 61, model_opaque);
    Bytes model_opaque_environment = record("MODELEbB", "mod\\reflective_opaque_npc", false);
    write32(model_opaque_environment, 0);
    model_opaque_environment.insert(model_opaque_environment.end(), env_scope, env_scope + sizeof(env_scope));
    texture_property(model_opaque_environment, "Name", "mod\\npc_cube");
    chunk(list, 84, model_opaque_environment);
    Bytes environment_plain = record("LaEmB   ", "mod\\plain_glow", false);
    environment_plain[172] = 0;
    write32(environment_plain, 0);
    environment_plain.insert(environment_plain.end(), env_scope, env_scope + sizeof(env_scope));
    texture_property(environment_plain, "Name", "mod\\glow_plain");
    chunk(list, 63, environment_plain);
    chunk(list, 64, record("V_AREF  ", "mod\\vertex_cutout_detail", false, 72));
    chunk(list, 65, record("V_AREF  ", "mod\\vertex_transparent_detail", true, 40));
    for (uint32_t blend : {2u, 3u, 4u, 7u, 8u, 9u})
    {
        const std::string name = "mod\\world_set_" + std::to_string(blend);
        Bytes world = record("S_SET   ", name.c_str(), false);
        write32(world, 7); world.insert(world.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
        write32(world, blend); write32(world, 0);
        property(world, 6, "Z-test", 1);
        chunk(list, 85 + blend, world);
    }
    Bytes world_set = record("S_SET   ", "mod\\world_set_0", false);
    write32(world_set, 7);
    world_set.insert(world_set.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(world_set, 0); write32(world_set, 0);
    property(world_set, 6, "Z-test", 1);
    property(world_set, 6, "Z-write", 1);
    chunk(list, 85, world_set);
    for (uint32_t blend = 0; blend <= 5; ++blend)
    {
        const std::string name = "mod\\particle_screen_" + std::to_string(blend);
        Bytes screen = record("S_SET   ", name.c_str(), false, 53);
        write32(screen, 7);
        screen.insert(screen.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
        write32(screen, blend); write32(screen, 0);
        property(screen, 6, "Z-test", 1);
        property(screen, 6, "Z-write", blend == 0 ? 1 : 0);
        chunk(list, 95 + blend, screen);
    }
    Bytes file;
    Bytes constants;
    constants.insert(constants.end(), {'p', 'u', 'l', 's', 'e', 0});
    for (unsigned channel = 0; channel < 4; ++channel)
    {
        WaveForm wave;
        wave.arg[0] = .25f * float(channel + 1);
        const auto* bytes = reinterpret_cast<const uint8_t*>(&wave);
        constants.insert(constants.end(), bytes, bytes + sizeof(wave));
    }
    chunk(file, 0, constants);
    chunk(file, 2, list);
    IReader reader(file.data(), file.size());
    ShaderMaterialLibrary library;
    std::string error;
    assert(library.load(reader, error) && library.size() == 56);
    const MaterialDefinition* parsed = library.find("MOD/arbitrary_reflection");
    assert(parsed && parsed->properties.size() >= 2);
    const BlenderProperty& second_name = parsed->properties.back();
    assert(second_name.scope == "Environment map" && second_name.name == "Name" &&
        second_name.type == 3 && second_name.data.size() == 64 &&
        std::strcmp(reinterpret_cast<const char*>(second_name.data.data()), "mod\\cube") == 0);
    assert(library.contains("HUD/ALPHA"));
    const auto* world_state = library.find("mod\\world_set_2");
    assert(world_state && world_state->pass.depth_test && !world_state->pass.depth_write &&
        world_state->pass.clamp && !world_state->pass.lighting && !world_state->pass.fog);
    const auto* set_state = library.find("mod\\world_set_0");
    assert(set_state && set_state->pass.depth_test && set_state->pass.depth_write);
    const auto* reflective_state = library.find("mod\\reflective_opaque_npc");
    assert(reflective_state && reflective_state->pass.depth_test && reflective_state->pass.depth_write);
    assert(!library.contains("hud\\font")); // Built-in UI shaders are not serialized blenders.
    SurfaceMode mode{};
    int screen_alpha_ref = -1;
    for (int blend = 0; blend <= 5; ++blend)
    {
        const std::string name = "mod\\particle_screen_" + std::to_string(blend);
        int selected = -1;
        assert(library.resolve(name, mode, error, &screen_alpha_ref,
            &selected, true) && screen_alpha_ref == 53 && selected == blend);
    }
    assert(!library.resolve("hud\\alpha", mode, error, nullptr, nullptr, true) &&
        error.find("Z-test") != std::string::npos);
    assert(library.resolve("DEF_SHADERS/LEAF", mode, error) && mode == SurfaceMode::AlphaTest);
    assert(library.resolve("def_shaders\\glass", mode, error) && mode == SurfaceMode::Transparent);
    assert(library.resolve("trees\\leaf", mode, error) && mode == SurfaceMode::AlphaTest);
    MaterialDefinition object_lod_definition;
    assert(library.resolve("trees\\object_lod", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, nullptr, &object_lod_definition));
    assert(object_lod_definition.object_lod && mode == SurfaceMode::AlphaTest &&
        object_lod_definition.pipeline_supported);
    assert(!library.find("trees\\leaf")->object_lod);
    assert(library.resolve("trees\\blended", mode, error) && mode == SurfaceMode::AlphaTest);
    assert(library.resolve("models\\fur", mode, error) && mode == SurfaceMode::AlphaTest);
    for (uint32_t selected = 0; selected < 4; ++selected)
    {
        MaterialDefinition tessellated;
        const std::string name = "models\\tess_" + std::to_string(selected);
        assert(library.resolve(name, mode, error, nullptr, nullptr, false,
            false, true, false, nullptr, nullptr, &tessellated, true));
        assert(tessellated.pipeline_supported && tessellated.tessellation == selected &&
            mode == SurfaceMode::Opaque);
    }
    assert(!library.resolve("models\\tess_4", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, nullptr, nullptr, true) &&
        error.find("invalid Tessellation token") != std::string::npos);
    assert(library.resolve("models\\glass", mode, error) && mode == SurfaceMode::Transparent);
    assert(library.resolve("detail\\solid", mode, error) && mode == SurfaceMode::Opaque);
    assert(library.resolve("mod\\plain_vertex", mode, error) && mode == SurfaceMode::Opaque);
    MaterialDefinition lightmapped_detail_definition;
    assert(library.resolve("mod\\lightmapped_detail", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, nullptr, &lightmapped_detail_definition) &&
        mode == SurfaceMode::Opaque &&
        lightmapped_detail_definition.detail_texture == "mod\\another_detail");
    int vertex_alpha_ref = -1;
    assert(library.resolve("mod\\vertex_cutout_detail", mode, error, &vertex_alpha_ref) &&
        mode == SurfaceMode::AlphaTest && vertex_alpha_ref == 72);
    assert(library.resolve("mod\\vertex_transparent_detail", mode, error) &&
        mode == SurfaceMode::Transparent);
    assert(!library.resolve("unknown", mode, error) && error.find("unknown") != std::string::npos);
    assert(!library.resolve("mod\\unrecognized", mode, error) &&
        error.find("shaders.xr blender id=5") != std::string::npos &&
        error.find("CUSTOM") != std::string::npos && error.find("version=17") != std::string::npos);
    MaterialDefinition reflection_definition;
    assert(library.resolve("mod\\arbitrary_reflection", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, nullptr, &reflection_definition) &&
        reflection_definition.environment_texture == "mod\\cube");
    const MaterialDefinition* parsed_detail = library.find("mod\\plain_stone");
    assert(parsed_detail && parsed_detail->pipeline_supported &&
        parsed_detail->properties.back().scope == "Detail map" &&
        parsed_detail->detail_texture == "mod\\unusual_detail" &&
        std::strcmp(reinterpret_cast<const char*>(parsed_detail->properties.back().data.data()),
            "mod\\unusual_detail") == 0);
    assert(library.resolve("mod\\plain_stone", mode, error) && mode == SurfaceMode::Opaque);
    MaterialDefinition constant_definition;
    assert(library.resolve("mod\\glowing_wall", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, nullptr, &constant_definition) &&
        constant_definition.has_environment_constant &&
        constant_definition.environment_texture == "mod\\glow_map" &&
        constant_definition.constant_waveforms[0].Calculate(0.f) == .25f);
    assert(library.resolve("mod\\reflective_npc", mode, error) && mode == SurfaceMode::Transparent);
    assert(library.resolve("mod\\blended_reflection", mode, error) && mode == SurfaceMode::Transparent);
    assert(library.resolve("mod\\plain_npc", mode, error) && mode == SurfaceMode::Opaque);
    MaterialDefinition opaque_environment;
    assert(library.resolve("mod\\reflective_opaque_npc", mode, error, nullptr, nullptr,
        false, false, false, false, nullptr, nullptr, &opaque_environment) &&
        mode == SurfaceMode::Opaque &&
        opaque_environment.environment_texture == "mod\\npc_cube");
    assert(library.resolve("mod\\plain_glow", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, nullptr, &constant_definition) &&
        mode == SurfaceMode::Opaque && !constant_definition.has_environment_constant &&
        constant_definition.environment_texture == "mod\\glow_plain");
    assert(!library.resolve("mod\\missing_env", mode, error) &&
        error.find("id=59") != std::string::npos && error.find("Environment map / Name") != std::string::npos);
    assert(!library.resolve("mod\\legacy_ground", mode, error) &&
        error.find("id=57") != std::string::npos && error.find("single detail variant") != std::string::npos);
    assert(library.resolve("mod\\cutout", mode, error) && mode == SurfaceMode::AlphaTest);
    int alpha_ref = -1;
    assert(library.resolve("mod\\cutout", mode, error, &alpha_ref) && alpha_ref == 200);
    int wallmark_blend = -1;
    assert(library.resolve("effects\\wallmarkblend", mode, error, nullptr, &wallmark_blend,
        false, false, true) && mode == SurfaceMode::Transparent && wallmark_blend == 6);
    assert(library.resolve("hud\\opaque", mode, error) && mode == SurfaceMode::Opaque);
    assert(library.resolve("hud\\alpha", mode, error) && mode == SurfaceMode::Transparent);
    assert(!library.resolve("effects\\additive", mode, error) &&
        error.find("blending=2") != std::string::npos && error.find("id=9") != std::string::npos);
    assert(library.resolve("mod\\world_set_0", mode, error, nullptr, nullptr,
        false, false, true) && mode == SurfaceMode::Transparent);
    for (int blend : {2, 3, 4, 7, 8, 9})
    {
        const std::string name = "mod\\world_set_" + std::to_string(blend);
        int selected = -1;
        assert(library.resolve(name, mode, error, nullptr, &selected,
            false, false, true) && mode == SurfaceMode::Transparent && selected == blend);
    }
    int screen_particle_blend = -1;
    assert(!library.resolve("effects\\additive", mode, error, nullptr,
        &screen_particle_blend, true) && error.find("Z-test") != std::string::npos);
    assert(library.resolve("effects\\sprite_set", mode, error, &alpha_ref) &&
        mode == SurfaceMode::AlphaTest && alpha_ref == 200);
    int blend_mode = -1;
    TerrainMaterialTextures terrain_textures;
    assert(library.resolve("levels\\modded_ground", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, &terrain_textures));
    assert(terrain_textures.enabled && terrain_textures.detail[0] == "detail\\modded" &&
        terrain_textures.detail[3] == "detail\\modded");
    assert(library.resolve("mod\\unusual_four_channel", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, &terrain_textures));
    assert(terrain_textures.enabled && terrain_textures.detail[0] == "r2-r" &&
        terrain_textures.detail[1] == "r2-g" && terrain_textures.detail[2] == "r2-b" &&
        terrain_textures.detail[3] == "r2-a");
    MaterialDefinition named_terrain_definition;
    assert(library.resolve("levels\\zaton_asfalt_fixture", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, &terrain_textures, &named_terrain_definition));
    assert(terrain_textures.enabled && named_terrain_definition.terrain.enabled &&
        named_terrain_definition.detail_texture == "detail\\serialized_but_unused");
    const auto transform = std::find_if(named_terrain_definition.properties.begin(),
        named_terrain_definition.properties.end(), [](const BlenderProperty& property)
        { return property.scope == "Detail map" && property.name == "Transform"; });
    assert(transform != named_terrain_definition.properties.end() && transform->type == 1 &&
        std::strcmp(reinterpret_cast<const char*>(transform->data.data()), "terrain_scroll_unused") == 0);
    LevelPassGraph named_terrain_graph;
    assert(build_render_pass_graph(LevelPassSelection{mode, true, true, false},
        named_terrain_graph, error) && named_terrain_graph.count == 1 &&
        named_terrain_graph.passes[0].fragment == "vk\\level_terrain.ps" &&
        (named_terrain_graph.passes[0].textures & DetailRole) == 0);
    MaterialDefinition null_channel_definition;
    assert(library.resolve("mod\\single_with_null_channels", mode, error, nullptr, nullptr,
        false, false, true, false, nullptr, &terrain_textures, &null_channel_definition));
    assert(!terrain_textures.enabled && null_channel_definition.detail_texture == "mod\\one_detail");
    Bytes missing_terrain = record("BmmDold ", "levels\\incomplete", false);
    missing_terrain[172] = 3;
    write32(missing_terrain, 3);
    missing_terrain.insert(missing_terrain.end(), {'R','2','-','R',0});
    missing_terrain.resize(missing_terrain.size() + 64);
    std::memcpy(missing_terrain.data() + missing_terrain.size() - 64, "detail\\red", 11);
    Bytes missing_terrain_list, missing_terrain_file;
    chunk(missing_terrain_list, 62, missing_terrain);
    chunk(missing_terrain_file, 2, missing_terrain_list);
    IReader missing_terrain_reader(missing_terrain_file.data(), missing_terrain_file.size());
    ShaderMaterialLibrary incomplete_library;
    assert(!incomplete_library.load(missing_terrain_reader, error) &&
        error.find("id=62") != std::string::npos &&
        error.find("R2-R/G/B/A") != std::string::npos);
    Bytes incomplete_modern = record("BmmD    ", "mod\\incomplete_modern", false);
    incomplete_modern[172] = 3;
    write32(incomplete_modern, 0);
    incomplete_modern.insert(incomplete_modern.end(), {'D','e','t','a','i','l',' ','m','a','p',0});
    texture_property(incomplete_modern, "R2-R", "mod\\red");
    texture_property(incomplete_modern, "R2-G", "$null");
    Bytes incomplete_modern_list, incomplete_modern_file;
    chunk(incomplete_modern_list, 83, incomplete_modern);
    chunk(incomplete_modern_file, 2, incomplete_modern_list);
    IReader incomplete_modern_reader(incomplete_modern_file.data(), incomplete_modern_file.size());
    assert(!incomplete_library.load(incomplete_modern_reader, error) &&
        error.find("id=83") != std::string::npos &&
        error.find("R2-R/G/B/A") != std::string::npos);
    assert(library.resolve("effects\\flash_add", mode, error, nullptr, &blend_mode) &&
        mode == SurfaceMode::Transparent && blend_mode == 2);
    assert(library.resolve("effects\\flash_alpha_add", mode, error, nullptr, &blend_mode) &&
        mode == SurfaceMode::Transparent && blend_mode == 5);
    assert(!library.resolve("effects\\multiply", mode, error) &&
        error.find("blending=3") != std::string::npos);
    assert(library.resolve("effects\\multiply", mode, error, nullptr, &blend_mode, true) &&
        mode == SurfaceMode::Transparent && blend_mode == 3);
    assert(!library.resolve("effects\\screen_multiply_2x", mode, error) &&
        error.find("blending=4") != std::string::npos);
    assert(!library.resolve("effects\\screen_multiply_2x", mode, error, nullptr, &blend_mode, true) &&
        error.find("Z-test") != std::string::npos);
    assert(library.resolve("effects\\wallmarkmult", mode, error, nullptr, &blend_mode) &&
        mode == SurfaceMode::Transparent && blend_mode == 6);
    assert(!library.resolve("effects\\wallmarkmult", mode, error, nullptr, &blend_mode, true) &&
        error.find("blending=6") != std::string::npos);
    assert(!library.resolve("effects\\wallmarkmult", mode, error, nullptr, &blend_mode, false, true) &&
        error.find("Z-test") != std::string::npos);
    for (const auto& name : {"hud\\opaque", "hud\\alpha", "effects\\additive",
             "effects\\screen_multiply_2x"})
    {
        assert(library.resolve(name, mode, error, &alpha_ref, &blend_mode, false, true));
        assert(blend_mode >= 0 && blend_mode <= 9);
    }
    assert(!library.resolve("effects\\wallmarkblend", mode, error, nullptr, nullptr,
        false, true) && error.find("Z-test") != std::string::npos);
    assert(!library.resolve("trees\\leaf", mode, error, nullptr, nullptr, false, true) &&
        error.find("screen class") != std::string::npos);
    Bytes screen_depth = record("S_SET   ", "mod\\depth_ui", false);
    write32(screen_depth, 7);
    screen_depth.insert(screen_depth.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_depth, 1); write32(screen_depth, 0);
    property(screen_depth, 6, "Z-test", 1);
    Bytes depth_list, depth_file;
    chunk(depth_list, 48, screen_depth);
    chunk(depth_file, 2, depth_list);
    IReader depth_reader(depth_file.data(), depth_file.size());
    assert(library.load(depth_reader, error));
    assert(library.resolve("mod\\depth_ui", mode, error, nullptr, &blend_mode, true) &&
        mode == SurfaceMode::Transparent);
    assert(!library.resolve("mod\\depth_ui", mode, error, nullptr, &blend_mode, false, true) &&
        error.find("Z-test") != std::string::npos && error.find("id=48") != std::string::npos);
    Bytes depth_write = record("S_SET   ", "mod\\depth_write", false);
    write32(depth_write, 7);
    depth_write.insert(depth_write.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(depth_write, 1); write32(depth_write, 0);
    property(depth_write, 6, "Z-test", 1);
    property(depth_write, 6, "Z-write", 1);
    Bytes depth_write_list, depth_write_file;
    chunk(depth_write_list, 50, depth_write);
    chunk(depth_write_file, 2, depth_write_list);
    IReader depth_write_reader(depth_write_file.data(), depth_write_file.size());
    assert(library.load(depth_write_reader, error));
    assert(!library.resolve("mod\\depth_write", mode, error, nullptr, &blend_mode, true) &&
        error.find("Z-write") != std::string::npos);
    Bytes screen_wrap = record("S_SET   ", "mod\\wrap_ui", false);
    replace_saved_string_property(screen_wrap, 3, "Name", "ui\\saved_but_unused");
    replace_saved_string_property(screen_wrap, 1, "Transform", "ui_scroll_unused");
    write32(screen_wrap, 7);
    screen_wrap.insert(screen_wrap.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(screen_wrap, 1); write32(screen_wrap, 0);
    property(screen_wrap, 6, "Texture clamp", 0);
    Bytes wrap_list, wrap_file;
    chunk(wrap_list, 49, screen_wrap);
    chunk(wrap_file, 2, wrap_list);
    IReader wrap_reader(wrap_file.data(), wrap_file.size());
    assert(library.load(wrap_reader, error));
    const auto* wrapped_state = library.find("mod\\wrap_ui");
    assert(wrapped_state && !wrapped_state->pass.clamp);
    assert(library.resolve("mod\\wrap_ui", mode, error)); // Scene sampler repeats.
    MaterialDefinition ui_wrap_definition;
    assert(library.resolve("mod\\wrap_ui", mode, error, nullptr, nullptr,
        false, true, false, false, nullptr, nullptr, &ui_wrap_definition) &&
        !ui_wrap_definition.pass.clamp && mode == SurfaceMode::Transparent);
    Bytes water = record("S_SET   ", "mod\\arbitrary_liquid", false, 0);
    replace_saved_string_property(water, 3, "Name", "water\\saved_but_unused");
    replace_saved_string_property(water, 1, "Transform", "water_scroll_unused");
    write32(water, 7); water.insert(water.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(water, 0); write32(water, 0);
    write32(water, 0);
    water.insert(water.end(), {'G','e','n','e','r','a','l',0});
    property(water, 6, "Strict sorting", 1);
    write32(water, 0);
    water.insert(water.end(), {'B','a','s','e',' ','T','e','x','t','u','r','e',0});
    property(water, 6, "Texture clamp", 0);
    property(water, 6, "Z-test", 1);
    property(water, 6, "Z-write", 1);
    property(water, 6, "Lighting", 1);
    Bytes lod = record("S_SET   ", "details\\lod", false, 16);
    write32(lod, 7); lod.insert(lod.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(lod, 1); write32(lod, 0);
    property(lod, 6, "Texture clamp", 0);
    property(lod, 6, "Z-test", 1);
    property(lod, 6, "Fog", 1);
    Bytes glow = record("S_SET   ", "effects\\glow", false, 0);
    write32(glow, 7); glow.insert(glow.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(glow, 5); write32(glow, 0);
    property(glow, 6, "Z-test", 1);
    Bytes level_list, level_file;
    chunk(level_list, 52, water); chunk(level_list, 25, lod); chunk(level_list, 36, glow);
    Bytes unsupported_fog = record("S_SET   ", "mod\\fog_add", false, 64);
    write32(unsupported_fog, 7);
    unsupported_fog.insert(unsupported_fog.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(unsupported_fog, 5); write32(unsupported_fog, 0);
    property(unsupported_fog, 6, "Z-test", 1);
    property(unsupported_fog, 6, "Fog", 1);
    chunk(level_list, 39, unsupported_fog);
    Bytes misleading = record("S_SET   ", "water\\ordinary_overlay", false, 0);
    write32(misleading, 7); misleading.insert(misleading.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(misleading, 1); write32(misleading, 0);
    chunk(level_list, 37, misleading);
    Bytes scaled_replace = record("S_SET   ", "mod\\scaled_replace", false, 173);
    write32(scaled_replace, 7); scaled_replace.insert(scaled_replace.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(scaled_replace, 7); write32(scaled_replace, 0);
    property(scaled_replace, 6, "Z-test", 1);
    chunk(level_list, 38, scaled_replace);
    chunk(level_file, 2, level_list);
    IReader level_reader(level_file.data(), level_file.size());
    assert(library.load(level_reader, error));
    uint32_t flags = 0;
    MaterialDefinition water_definition;
    assert(library.resolve("mod\\arbitrary_liquid", mode, error, nullptr, &blend_mode,
        false, false, true, false, &flags, nullptr, &water_definition) &&
        mode == SurfaceMode::Transparent && blend_mode == 0 &&
        (flags & 15u) == 15u && water_definition.role == MaterialDefinition::Role::Water);
    const auto saved_base_name = std::find_if(water_definition.properties.begin(),
        water_definition.properties.end(), [](const BlenderProperty& property)
        { return property.scope == "Base Texture" && property.name == "Name"; });
    assert(saved_base_name != water_definition.properties.end() &&
        std::strcmp(reinterpret_cast<const char*>(saved_base_name->data.data()),
            "water\\saved_but_unused") == 0);
    MaterialDefinition ordinary_definition;
    assert(!library.resolve("water\\ordinary_overlay", mode, error, nullptr, nullptr,
        false, false, true) && error.find("Z-test=0") != std::string::npos);
    assert(library.resolve("water\\ordinary_overlay", mode, error, nullptr, nullptr,
        false, true, false, false, nullptr, nullptr, &ordinary_definition) &&
        ordinary_definition.role == MaterialDefinition::Role::Standard);
    assert(!library.resolve("details\\lod", mode, error, nullptr, nullptr, false, false, false) &&
        error.find("Fog") != std::string::npos);
    assert(library.resolve("details\\lod", mode, error, nullptr, &blend_mode,
        false, false, true, false, &flags) && mode == SurfaceMode::Transparent &&
        blend_mode == 1 && (flags & 16u));
    assert(!library.resolve("effects\\glow", mode, error) &&
        error.find("blending=5") != std::string::npos);
    assert(library.resolve("effects\\glow", mode, error, nullptr, &blend_mode,
        false, false, true) && mode == SurfaceMode::Transparent && blend_mode == 5);
    assert(!library.resolve("mod\\fog_add", mode, error, nullptr, nullptr,
        false, false, true) && error.find("Fog") != std::string::npos &&
        error.find("id=39") != std::string::npos);
    assert(library.resolve("mod\\scaled_replace", mode, error, &alpha_ref, &blend_mode,
        false, false, true) && mode == SurfaceMode::Transparent && blend_mode == 7 &&
        alpha_ref == 0);
    Bytes newer = record("LM_AREF ", "mod\\future", false);
    newer[172] = 42;
    Bytes future_list, future_file;
    chunk(future_list, 19, newer);
    chunk(future_file, 2, future_list);
    IReader future(future_file.data(), future_file.size());
    assert(library.load(future, error));
    assert(!library.resolve("mod\\future", mode, error) &&
        error.find("id=19") != std::string::npos &&
        error.find("version=42") != std::string::npos);
    Bytes invalid_alpha = record("LM_AREF ", "mod\\bad_aref", false, 256);
    Bytes alpha_list, alpha_file;
    chunk(alpha_list, 20, invalid_alpha);
    chunk(alpha_file, 2, alpha_list);
    IReader bad_alpha(alpha_file.data(), alpha_file.size());
    assert(!library.load(bad_alpha, error) && error.find("Alpha ref") != std::string::npos &&
        error.find("id=20") != std::string::npos);
    Bytes missing_blending = record("PARTICLE", "mod\\missing_blend", false);
    Bytes missing_list, missing_file;
    chunk(missing_list, 21, missing_blending);
    chunk(missing_file, 2, missing_list);
    IReader missing(missing_file.data(), missing_file.size());
    assert(!library.load(missing, error) && error.find("Blending") != std::string::npos &&
        error.find("id=21") != std::string::npos);
    Bytes invalid_token = record("S_SET   ", "mod\\bad_token", false);
    write32(invalid_token, 7);
    invalid_token.insert(invalid_token.end(), blend_name, blend_name + std::strlen(blend_name) + 1);
    write32(invalid_token, 2); write32(invalid_token, 1);
    write32(invalid_token, 1); invalid_token.resize(invalid_token.size() + 64);
    Bytes token_list, token_file;
    chunk(token_list, 22, invalid_token);
    chunk(token_file, 2, token_list);
    IReader bad_token(token_file.data(), token_file.size());
    assert(!library.load(bad_token, error) && error.find("Blending token") != std::string::npos &&
        error.find("id=22") != std::string::npos);
    Bytes malformed, broken;
    chunk(broken, 0, Bytes(5, 0));
    chunk(malformed, 2, broken);
    IReader truncated(malformed.data(), malformed.size());
    assert(!library.load(truncated, error));
    assert(error.find("shaders.xr blender id=0") != std::string::npos);
    Bytes wrong_property = record("LM      ", "invalid\\property", false);
    write32(wrong_property, 99);
    wrong_property.insert(wrong_property.end(), {'M','o','d',0});
    Bytes invalid_list, invalid_file;
    chunk(invalid_list, 0, wrong_property);
    chunk(invalid_file, 2, invalid_list);
    IReader unknown_property(invalid_file.data(), invalid_file.size());
    assert(!library.load(unknown_property, error));
    assert(error.find("type=99") != std::string::npos &&
        error.find("invalid\\property") != std::string::npos);
    // Unknown properties and named matrices must not acquire the default
    // pipeline solely because the serialized class and version are familiar.
    const auto rejected_semantics = [&](Bytes source, uint32_t id,
        const char* shader_name, const char* expected)
    {
        Bytes records, file;
        chunk(records, id, source);
        chunk(file, 2, records);
        IReader reader(file.data(), file.size());
        ShaderMaterialLibrary tested;
        assert(tested.load(reader, error));
        assert(!tested.resolve(shader_name, mode, error));
        assert(error.find("id=" + std::to_string(id)) != std::string::npos);
        assert(error.find(expected) != std::string::npos);
    };
    Bytes unknown_semantics = record("LM      ", "mod\\opaque_by_name", false);
    property(unknown_semantics, 6, "Mystery lighting", 1);
    rejected_semantics(unknown_semantics, 70, "mod\\opaque_by_name", "Mystery lighting");
    Bytes false_aref = record("LM      ", "mod\\false_aref", false);
    write32(false_aref, 4);
    false_aref.insert(false_aref.end(), {'A','l','p','h','a',' ','r','e','f',0});
    write32(false_aref, 32); write32(false_aref, 0); write32(false_aref, 255);
    rejected_semantics(false_aref, 75, "mod\\false_aref", "Alpha ref");
    Bytes foreign_scope = record("LM      ", "mod\\false_scope", false);
    write32(foreign_scope, 0);
    foreign_scope.insert(foreign_scope.end(), {'D','e','t','a','i','l',' ','m','a','p',0});
    rejected_semantics(foreign_scope, 80, "mod\\false_scope", "Detail map");
    Bytes false_clamp = record("LM_AREF ", "mod\\false_clamp", false);
    property(false_clamp, 6, "Texture clamp", 1);
    rejected_semantics(false_clamp, 76, "mod\\false_clamp", "Texture clamp");
    Bytes false_environment = record("LmEbB   ", "mod\\false_constant", false);
    texture_property(false_environment, "Name", "mod\\cube");
    write32(false_environment, 2);
    false_environment.insert(false_environment.end(), {'C','o','n','s','t','a','n','t',0});
    false_environment.resize(false_environment.size() + 64);
    rejected_semantics(false_environment, 77, "mod\\false_constant", "Constant");
    Bytes old_aref = record("LM_AREF ", "mod\\old_aref", false);
    old_aref[172] = 0;
    rejected_semantics(old_aref, 78, "mod\\old_aref", "Alpha-blend");
    Bytes old_terrain = record("BmmDold ", "mod\\old_terrain", false);
    old_terrain[172] = 2;
    write32(old_terrain, 0);
    old_terrain.insert(old_terrain.end(), {'D','e','t','a','i','l',' ','m','a','p',0});
    for (const char* channel : {"R2-R", "R2-G", "R2-B", "R2-A"})
        texture_property(old_terrain, channel, "mod\\sand");
    rejected_semantics(old_terrain, 79, "mod\\old_terrain", "R2-R");
    Bytes one_channel_matrix = record("BmmDold ", "mod\\one_channel_matrix", false);
    one_channel_matrix[172] = 3;
    write32(one_channel_matrix, 0);
    one_channel_matrix.insert(one_channel_matrix.end(), {'D','e','t','a','i','l',' ','m','a','p',0});
    texture_property(one_channel_matrix, "Name", "detail\\actual_single_detail");
    matrix_property(one_channel_matrix, "Transform", "animated_single_detail");
    for (const char* channel : {"R2-R", "R2-G", "R2-B", "R2-A"})
        texture_property(one_channel_matrix, channel, "$null");
    rejected_semantics(one_channel_matrix, 111, "mod\\one_channel_matrix", "missing shaders.xr matrix");
    Bytes base_matrix = record("LM      ", "mod\\double_sided", false);
    write32(base_matrix, 1);
    base_matrix.insert(base_matrix.end(), {'T','r','a','n','s','f','o','r','m',0});
    const size_t matrix_value = base_matrix.size();
    base_matrix.resize(matrix_value + 64);
    std::memcpy(base_matrix.data() + matrix_value, "spin", 5);
    rejected_semantics(base_matrix, 71, "mod\\double_sided", "missing shaders.xr matrix");
    Bytes base_selector = record("LM      ", "mod\\base_selector", false);
    replace_saved_string_property(base_selector, 3, "Name", "textures\\nondefault");
    rejected_semantics(base_selector, 112, "mod\\base_selector", "base texture selector");
    {
        Bytes records, file;
        chunk(records, 71, base_matrix);
        chunk(file, 2, records);
        IReader reader(file.data(), file.size());
        ShaderMaterialLibrary parsed_library;
        assert(parsed_library.load(reader, error));
        const auto* parsed_matrix = parsed_library.find("mod\\double_sided");
        assert(parsed_matrix && !parsed_matrix->pipeline_supported &&
            parsed_matrix->properties.back().scope == "Base Texture" &&
            parsed_matrix->properties.back().type == 1 &&
            std::strcmp(reinterpret_cast<const char*>(parsed_matrix->properties.back().data.data()),
                "spin") == 0);
    }
    Bytes priority = record("V       ", "mod\\priority", false);
    write32(priority, 0);
    priority.insert(priority.end(), {'G','e','n','e','r','a','l',0});
    write32(priority, 4);
    priority.insert(priority.end(), {'P','r','i','o','r','i','t','y',0});
    write32(priority, 3); write32(priority, 0); write32(priority, 3);
    {
        Bytes records, file;
        chunk(records, 72, priority);
        chunk(file, 2, records);
        IReader reader(file.data(), file.size());
        ShaderMaterialLibrary parsed_library;
        assert(parsed_library.load(reader, error));
        const auto* parsed_priority = parsed_library.find("mod\\priority");
        assert(parsed_priority && parsed_priority->pipeline_supported &&
            parsed_priority->priority == 3 &&
            parsed_priority->properties.back().scope == "General" &&
            parsed_priority->properties.back().type == 4 &&
            parsed_priority->properties.back().data.size() == 12 &&
            parsed_priority->properties.back().data[0] == 3);
        SurfaceMode selected{};
        MaterialDefinition resolved;
        assert(parsed_library.resolve("mod\\priority", selected, error,
            nullptr, nullptr, false, false, true, false,
            nullptr, nullptr, &resolved));
        assert(selected == SurfaceMode::Opaque && resolved.priority == 3);
        assert(!parsed_library.resolve("mod\\priority", selected, error,
            nullptr, nullptr, false, false, false, false,
            nullptr, nullptr, &resolved, true) &&
            error.find("Priority=3") != std::string::npos);
    }
    priority[priority.size() - 12] = 4;
    rejected_semantics(priority, 72, "mod\\priority", "invalid render priority");
    Bytes particle_priority = record("PARTICLE", "mod\\priority_particle", false, 114);
    write32(particle_priority, 7);
    particle_priority.insert(particle_priority.end(), {'B','l','e','n','d','i','n','g',0});
    write32(particle_priority, 1); write32(particle_priority, 0);
    write32(particle_priority, 0);
    particle_priority.insert(particle_priority.end(), {'G','e','n','e','r','a','l',0});
    write32(particle_priority, 4);
    particle_priority.insert(particle_priority.end(), {'P','r','i','o','r','i','t','y',0});
    write32(particle_priority, 3); write32(particle_priority, 0); write32(particle_priority, 3);
    {
        Bytes records, file;
        chunk(records, 114, particle_priority);
        chunk(file, 2, records);
        IReader reader(file.data(), file.size());
        ShaderMaterialLibrary parsed_library;
        assert(parsed_library.load(reader, error));
        SurfaceMode selected{};
        MaterialDefinition resolved;
        assert(parsed_library.resolve("mod\\priority_particle", selected, error,
            nullptr, nullptr, true, false, false, false,
            nullptr, nullptr, &resolved) && resolved.priority == 3);
        assert(!parsed_library.resolve("mod\\priority_particle", selected, error,
            nullptr, nullptr, false, false, false, false,
            nullptr, nullptr, &resolved, true));
    }
    Bytes renamed = record("LM_AREF ", "mod\\ordinary_surface", false);
    Bytes deceptive = record("LM_AREF ", "water\\double_sided_alpha_add", false);
    Bytes rename_records, rename_file;
    chunk(rename_records, 73, renamed);
    chunk(rename_records, 74, deceptive);
    chunk(rename_file, 2, rename_records);
    IReader renamed_reader(rename_file.data(), rename_file.size());
    assert(library.load(renamed_reader, error));
    SurfaceMode a{}, b{};
    assert(library.resolve("mod\\ordinary_surface", a, error));
    assert(library.resolve("water\\double_sided_alpha_add", b, error));
    assert(a == b && a == SurfaceMode::AlphaTest);
    Bytes duplicate_list, duplicate_file;
    chunk(duplicate_list, 0, record("LM      ", "duplicate", false));
    chunk(duplicate_list, 1, record("LM      ", "DUPLICATE", false));
    chunk(duplicate_file, 2, duplicate_list);
    IReader duplicate(duplicate_file.data(), duplicate_file.size());
    assert(!library.load(duplicate, error) && error.find("blender id=1") != std::string::npos);
    Bytes oversized_file;
    Bytes oversized_list;
    write32(oversized_list, 7); write32(oversized_list, 9999);
    chunk(oversized_file, 2, oversized_list);
    IReader oversized(oversized_file.data(), oversized_file.size());
    assert(!library.load(oversized, error) && error.find("truncated chunk") != std::string::npos);
}
