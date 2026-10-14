#include "src/Layers/xrRenderVK/LevelPassGraph.h"
#include "src/Layers/xrRenderVK/TransparentDrawOrder.h"

#include <cassert>
#include <string>
#include <vector>

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
    base.surface_roles = BumpAuxRole;
    assert(!build_level_pass_graph(base, graph, error));
    assert(error.find("BumpAux texture without Bump") != std::string::npos);
    base.surface_roles = 0;

    // World S_SET uses its own depth-tested forward shaders and blend factors.
    base = {};
    base.mode = SurfaceMode::Transparent;
    base.screen_blend = 0;
    assert(build_level_pass_graph(base, graph, error));
    assert(graph.passes[0].vertex == "vk\\level_opaque.vs" &&
        graph.passes[0].fragment == "vk\\world_set_set.ps" &&
        graph.passes[0].blend == LevelPassBlend::Replace &&
        graph.passes[0].depth_test && graph.passes[0].depth_write);
    base.screen_blend = 1;
    assert(build_level_pass_graph(base, graph, error));
    assert(graph.passes[0].vertex == "vk\\level_opaque.vs" &&
        graph.passes[0].fragment == "vk\\world_set_blended.ps" &&
        graph.passes[0].blend == LevelPassBlend::Alpha);
    for (int blend = 2; blend <= 4; ++blend)
    {
        base.screen_blend = blend;
        assert(build_level_pass_graph(base, graph, error));
        assert(graph.count == 1 && graph.passes[0].vertex == "vk\\level_opaque.vs");
        assert(graph.passes[0].fragment == (blend == 2 ? "vk\\world_set_additive.ps" :
            blend == 3 ? "vk\\world_set_multiply.ps" : "vk\\world_set_multiply_2x.ps"));
        assert(graph.passes[0].blend == (blend == 2 ? LevelPassBlend::Additive :
            blend == 3 ? LevelPassBlend::Multiply : LevelPassBlend::Multiply2x));
        assert(graph.passes[0].target == LevelPassTarget::Forward &&
            graph.passes[0].depth_test && !graph.passes[0].depth_write);
    }
    base.fog = true;
    assert(!build_level_pass_graph(base, graph, error));
    assert(error.find("S_SET blending") != std::string::npos);
    base.fog = false;
    for (int blend = 7; blend <= 9; ++blend)
    {
        base.screen_blend = blend;
        assert(build_level_pass_graph(base, graph, error));
        assert(graph.passes[0].vertex == "vk\\level_opaque.vs");
        assert(graph.passes[0].fragment == (blend == 7 ? "vk\\world_set_set_2r.ps" :
            blend == 8 ? "vk\\world_set_blend_2r.ps" : "vk\\world_set_blend_4r.ps"));
        assert(graph.passes[0].blend == (blend == 7 ? LevelPassBlend::Replace : LevelPassBlend::Alpha));
        assert(graph.passes[0].depth_test && !graph.passes[0].depth_write);
    }
    base = {};
    base.mode = SurfaceMode::Opaque;
    base.lightmapped = true;

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

    for (bool detail : {false, true})
    {
        assert(build_vegetation_pass_graph(SurfaceMode::Transparent, detail, 0, graph, error));
        assert(graph.count == 1 && graph.passes[0].target == LevelPassTarget::Forward);
        assert(graph.passes[0].depth_test && !graph.passes[0].depth_write);
        assert(graph.passes[0].blend == LevelPassBlend::Alpha);
        assert(graph.passes[0].vertex == "vk\\level_detail_blended.vs");
        assert(graph.passes[0].fragment == (detail ?
            "vk\\level_vegetation_detail_blended.ps" : "vk\\level_vegetation_blended.ps"));
        assert(graph.passes[0].textures == (BaseRole | (detail ? DetailRole : 0)));
        assert(build_vegetation_pass_graph(SurfaceMode::AlphaTest, detail, 0, graph, error));
        assert(graph.passes[0].depth_test && graph.passes[0].depth_write);
        assert(graph.passes[0].fragment == (detail ?
            "vk\\level_detail_unlit_cutout.ps" : "vk\\level_cutout.ps"));
    }

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
    assert(!build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Opaque,
        false, false, true}, common, error));
    assert(error.find("does not sample environment") != std::string::npos);
    assert(!build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Opaque,
        false, true, true, false, 2}, common, error));
    assert(error.find("does not sample environment") != std::string::npos);
    assert(build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Transparent,
        false, true, true, false, 2}, common, error));
    assert(common.passes[0].fragment == "vk\\skinned_environment.ps");
    assert(!build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Transparent,
        true, true, true, false, 2}, common, error));
    assert(error.find("HUD model environment cube") != std::string::npos);
    const char* world_model_fragments[]{"set", "blended", "additive", "multiply",
        "multiply_2x", "alpha_add", "wallmark_multiply_2x", "set_2r",
        "blend_2r", "blend_4r"};
    for (int blend = 0; blend <= 9; ++blend)
    {
        RenderPassSelection model_set{RenderConsumer::Model, SurfaceMode::Transparent};
        model_set.screen_blend = blend;
        assert(build_render_pass_graph(model_set, common, error));
        assert(common.count == 1 && common.passes[0].target == LevelPassTarget::Forward &&
            common.passes[0].depth_test && common.passes[0].depth_write == (blend == 0) &&
            common.passes[0].vertex == "vk\\object_blended.vs" &&
            common.passes[0].fragment == std::string("vk\\model_set_") + world_model_fragments[blend] + ".ps");
        model_set.animated = true;
        model_set.skin_weights = 2;
        assert(build_render_pass_graph(model_set, common, error));
        assert(common.passes[0].vertex == "vk\\skinned_2.vs");
        model_set.hud = true;
        assert(build_render_pass_graph(model_set, common, error));
        assert(common.passes[0].target == LevelPassTarget::Hud &&
            common.passes[0].depth_test && common.passes[0].depth_write == (blend == 0) &&
            common.passes[0].vertex == "vk\\hud_skinned_2.vs" &&
            common.passes[0].fragment == std::string("vk\\hud_model_set_") +
                world_model_fragments[blend] + ".ps");
        model_set.animated = false;
        assert(build_render_pass_graph(model_set, common, error));
        assert(common.passes[0].vertex == "vk\\object_blended.vs");
    }
    assert(build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Opaque,
        false, false, false, false, 0, 1, 0, true}, common, error));
    assert(common.passes[0].fragment == "vk\\object_detail_opaque.ps");
    assert(common.passes[0].textures & DetailRole);
    assert(build_render_pass_graph({RenderConsumer::Model, SurfaceMode::AlphaTest,
        false, true, false, false, 2, 1, 0, true}, common, error));
    assert(common.passes[0].fragment == "vk\\skinned_detail_cutout.ps");
    assert(build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Transparent,
        false, false, false, true, 0, 1, 0, true}, common, error));
    assert(common.passes[0].fragment == "vk\\object_detail_double_sided_blended.ps");
    assert(!build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Transparent,
        false, false, true, false, 0, 1, 0, true}, common, error));
    assert(error.find("diffuse detail with environment") != std::string::npos);
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
            assert(common.passes[0].depth_write == (hud || blend == 0));
            assert(build_render_pass_graph({RenderConsumer::Particle, SurfaceMode::Transparent,
                hud, false, false, false, 0, blend, 0, false, -1, true}, common, error));
            assert(common.passes[0].depth_test && common.passes[0].depth_write == (blend == 0));
            assert(common.passes[0].fragment == (hud ?
                std::string("vk\\hud_particle_screen_") +
                    (blend == 0 ? "set" : blend == 1 ? "blended" : blend == 2 ? "additive" :
                        blend == 3 ? "multiply" : blend == 4 ? "multiply_2x" : "alpha_add") + ".ps" :
                blend == 0 ? "vk\\world_set_set.ps" : blend == 1 ? "vk\\world_set_blended.ps" :
                blend == 5 ? "vk\\glow_alpha_add.ps" :
                    std::string("vk\\particle_world_") +
                        (blend == 2 ? "additive" : blend == 3 ? "multiply" : "multiply_2x") + ".ps"));
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
    assert(error.find("S_SET Blending") != std::string::npos);
    assert(!build_render_pass_graph({RenderConsumer::Model, SurfaceMode::Opaque,
        false, false, false, false, 0, 1, BumpAuxRole}, common, error));
    assert(error.find("BumpAux texture without Bump") != std::string::npos);
    for (SurfaceMode mode : {SurfaceMode::Opaque, SurfaceMode::AlphaTest, SurfaceMode::Transparent})
        for (uint16_t roles : {uint16_t(BumpRole | BumpAuxRole | GlossRole),
                uint16_t(NormalRole | GlossRole), uint16_t(EmissiveRole)})
        {
            assert(build_render_pass_graph({RenderConsumer::Model, mode,
                false, false, false, false, 0, 1, roles}, common, error));
            assert((common.passes[0].textures & roles) == roles);
        }

    // Three overlapping translucent quads stand for a grass detail, an OGF
    // model and a particle. The real forward recorder uses this exact sort;
    // its stable tie rule also keeps repeated frames deterministic.
    std::vector<TransparentDrawRef> overlap = {
        {4.f, false, 0, false, true},
        {16.f, false, 1, false, false},
        {9.f, true, 2, false, false},
        {9.f, false, 3, true, false},
    };
    sort_transparent_draws(overlap);
    assert(overlap[0].index == 1 && overlap[1].index == 2 &&
        overlap[2].index == 3 && overlap[3].index == 0);
    // Alpha compositing a solid red model behind half-transparent blue
    // particle and green grass yields all three channels at this pixel.
    float pixel[3]{};
    for (const auto& draw : overlap)
    {
        const float color[3] = {draw.model ? 1.f : 0.f,
            draw.detail ? 1.f : 0.f, draw.particle ? 1.f : 0.f};
        for (int channel = 0; channel < 3; ++channel)
            pixel[channel] = color[channel] * .5f + pixel[channel] * .5f;
    }
    assert(pixel[0] == .125f && pixel[1] == .5f && pixel[2] == .25f);
}
