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
}
