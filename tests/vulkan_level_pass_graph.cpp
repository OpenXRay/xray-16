#include "src/Layers/xrRenderVK/LevelPassGraph.h"

#include <cassert>
#include <string>

using namespace xray::render::vulkan;

int main()
{
    LevelPassGraph graph;
    std::string error;
    LevelPassSelection base;
    base.mode = SurfaceMode::Opaque;
    base.lightmapped = true;
    assert(build_level_pass_graph(base, graph, error));
    assert(graph.count == 1);
    assert(graph.passes[0].fragment == "vk\\level_lightmap.ps");
    assert(graph.passes[0].textures == (BaseRole | LightmapRole));
    base.surface_roles = BumpRole | BumpAuxRole | NormalRole | GlossRole | EmissiveRole;
    assert(build_level_pass_graph(base, graph, error));
    assert((graph.passes[0].textures & base.surface_roles) == base.surface_roles);
    base.surface_roles = 0;
    assert(graph.passes[0].depth_test && graph.passes[0].depth_write);

    base.detail = true;
    base.mode = SurfaceMode::Transparent;
    assert(build_level_pass_graph(base, graph, error));
    assert(graph.passes[0].fragment == "vk\\level_detail_blended.ps");
    assert(graph.passes[0].target == LevelPassTarget::Forward);
    assert(graph.passes[0].blend == LevelPassBlend::Alpha);
    assert(graph.passes[0].depth_test && !graph.passes[0].depth_write);
    assert(graph.passes[0].textures == (BaseRole | LightmapRole | DetailRole));
    base.lightmapped = false;
    assert(build_level_pass_graph(base, graph, error));
    assert(graph.passes[0].fragment == "vk\\level_detail_unlit_blended.ps");
    base.mode = SurfaceMode::AlphaTest;
    assert(build_level_pass_graph(base, graph, error));
    assert(graph.passes[0].fragment == "vk\\level_detail_unlit_cutout.ps");
    assert(graph.passes[0].depth_write);

    base = {};
    base.mode = SurfaceMode::Opaque;
    base.lightmapped = true;
    base.environment = true;
    base.laemb = true;
    assert(build_level_pass_graph(base, graph, error));
    assert(graph.count == 2);
    assert(graph.passes[0].fragment == "vk\\level_laemb_base.ps");
    assert(graph.passes[0].textures == (BaseRole | LightmapRole | EnvironmentRole));
    assert(graph.passes[1].fragment == "vk\\level_laemb_multiply.ps");
    assert(graph.passes[1].blend == LevelPassBlend::Multiply2x);
    assert(graph.passes[1].previous_color_pass == 0);
    assert(graph.passes[1].depth_test && !graph.passes[1].depth_write);
    base.mode = SurfaceMode::Transparent;
    assert(!build_level_pass_graph(base, graph, error));
    assert(graph.count == 0 && error.find("incompatible") != std::string::npos);

    base = {};
    base.mode = SurfaceMode::Opaque;
    base.terrain = true;
    base.lightmapped = true;
    assert(build_level_pass_graph(base, graph, error));
    assert(graph.count == 1);
    assert(graph.passes[0].textures ==
        (BaseRole | LightmapRole | MaskRole | TerrainRRole | TerrainGRole | TerrainBRole | TerrainARole));
    base.lightmapped = false;
    assert(!build_level_pass_graph(base, graph, error));
    base.lightmapped = true;
    base.mode = SurfaceMode::Transparent;
    assert(!build_level_pass_graph(base, graph, error));

    base = {};
    base.mode = SurfaceMode::Opaque;
    assert(build_level_pass_graph(base, graph, error));
    assert(std::string(graph.passes[0].vertex_for_visual(false, 7)) == "vk\\tree_opaque.vs");
    assert(std::string(graph.passes[0].vertex_for_visual(true, 7)) == "vk\\progressive_opaque.vs");

    RenderPassGraph common;
    assert(build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Opaque}, common, error));
    assert(common.passes[0].target == LevelPassTarget::GBuffer);
    assert(common.passes[0].fragment == "vk\\object_opaque.ps");
    assert(build_render_pass_graph({RenderConsumer::Model, SurfaceMode::AlphaTest,
        false, true, false, false, 2}, common, error));
    assert(common.passes[0].vertex == "vk\\skinned_2.vs");
    assert(common.passes[0].fragment == "vk\\object_cutout.ps");
    assert(build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Opaque,
        true, true, false, false, 4}, common, error));
    assert(common.passes[0].target == LevelPassTarget::Hud);
    assert(common.passes[0].vertex == "vk\\hud_skinned_4.vs");
    assert(!build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Opaque,
        false, true, false, false, 0}, common, error));
    assert(common.count == 0);
    assert(build_render_pass_graph({RenderConsumer::Particle, SurfaceMode::Transparent,
        true, false, false, false, 0, 5}, common, error));
    assert(common.passes[0].fragment == "vk\\particle_hud_alpha_add.ps");
    assert(common.passes[0].textures & EmissiveRole);
    assert(common.passes[0].target == LevelPassTarget::Hud);
    assert(build_render_pass_graph({RenderConsumer::Ui, SurfaceMode::Transparent}, common, error));
    assert(common.passes[0].target == LevelPassTarget::Ui);
    assert(!common.passes[0].depth_test);
    for (SurfaceMode mode : {SurfaceMode::Opaque, SurfaceMode::AlphaTest, SurfaceMode::Transparent})
        for (bool animated : {false, true})
        {
            assert(build_render_pass_graph({RenderConsumer::Model, mode, false,
                animated, false, false, static_cast<uint8_t>(animated ? 1 : 0)}, common, error));
            assert(common.count == 1);
            assert(common.passes[0].target == (mode == SurfaceMode::Transparent ?
                LevelPassTarget::Forward : LevelPassTarget::GBuffer));
            assert(common.passes[0].depth_write == (mode != SurfaceMode::Transparent));
        }
    for (int blend = 0; blend <= 5; ++blend)
        for (bool hud : {false, true})
        {
            assert(build_render_pass_graph({RenderConsumer::Particle, SurfaceMode::Transparent,
                hud, false, false, false, 0, blend}, common, error));
            assert(common.passes[0].fragment.find(hud ? "particle_hud_" : "particle_world_") != std::string::npos);
            assert(common.passes[0].blend == (blend == 0 ? LevelPassBlend::Replace :
                blend == 2 ? LevelPassBlend::Additive : blend == 3 ? LevelPassBlend::Multiply :
                blend == 4 ? LevelPassBlend::Multiply2x :
                blend == 5 ? LevelPassBlend::AlphaAdd : LevelPassBlend::Alpha));
            assert(common.passes[0].depth_write == hud);
        }
    for (int blend = 0; blend <= 9; ++blend)
    {
        assert(build_render_pass_graph({RenderConsumer::Ui, SurfaceMode::Transparent,
            false, false, false, false, 0, blend}, common, error));
        assert(common.passes[0].target == LevelPassTarget::Ui);
        assert(common.passes[0].blend == (blend == 0 || blend == 7 ? LevelPassBlend::Replace :
            blend == 2 ? LevelPassBlend::Additive : blend == 3 ? LevelPassBlend::Multiply :
            blend == 4 || blend == 6 ? LevelPassBlend::Multiply2x :
            blend == 5 ? LevelPassBlend::AlphaAdd : LevelPassBlend::Alpha));
    }
    assert(!build_render_pass_graph({RenderConsumer::Ui, SurfaceMode::Transparent,
        false, false, false, false, 0, 10}, common, error));
    assert(!build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Opaque,
        false, false, false, false, 0, 1, BumpAuxRole}, common, error));
    for (SurfaceMode mode : {SurfaceMode::Opaque, SurfaceMode::AlphaTest, SurfaceMode::Transparent})
        for (uint16_t roles : {uint16_t(BumpRole | BumpAuxRole | GlossRole),
                uint16_t(NormalRole | GlossRole), uint16_t(EmissiveRole)})
        {
            assert(build_render_pass_graph({RenderConsumer::Model, mode,
                false, false, false, false, 0, 1, roles}, common, error));
            assert((common.passes[0].textures & roles) == roles);
        }
}
