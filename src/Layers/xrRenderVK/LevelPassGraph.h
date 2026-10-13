#pragma once

#include "LevelModels.h"

#include <array>
#include <cstdint>
#include <string>

namespace xray::render::vulkan
{
enum class LevelPassTarget : uint8_t { GBuffer, Forward };
enum class LevelPassBlend : uint8_t { Replace, Alpha, Multiply2x, AlphaAdd };
enum LevelTextureRole : uint16_t
{
    BaseRole = 1u << 0, LightmapRole = 1u << 1, DetailRole = 1u << 2,
    MaskRole = 1u << 3, TerrainRRole = 1u << 4, TerrainGRole = 1u << 5,
    TerrainBRole = 1u << 6, TerrainARole = 1u << 7, EnvironmentRole = 1u << 8
};

struct LevelPassNode
{
    std::string vertex, fragment;
    LevelPassTarget target{LevelPassTarget::GBuffer};
    LevelPassBlend blend{LevelPassBlend::Replace};
    uint16_t textures{BaseRole};
    bool depth_test{true}, depth_write{true};
    // The second LaEmB draw loads the first draw's attachments in the same
    // render pass. This edge is ordered before the next visual is recorded.
    int8_t previous_color_pass{-1};

    const char* vertex_for_visual(bool fast, uint8_t visual_type) const
    {
        if (vertex != "vk\\level_opaque.vs" || fragment != "vk\\level_opaque.ps")
            return vertex.c_str();
        if (fast) return "vk\\progressive_opaque.vs";
        if (visual_type == 7 || visual_type == 11) return "vk\\tree_opaque.vs";
        return vertex.c_str();
    }
};

struct LevelPassGraph
{
    std::array<LevelPassNode, 2> passes{};
    uint8_t count{};
};

struct LevelPassSelection
{
    SurfaceMode mode{SurfaceMode::Unresolved};
    bool lightmapped{}, terrain{}, detail{}, environment{}, laemb{};
    bool wallmark_multiply{}, glow_alpha_add{}, fog{};
};

inline bool build_level_pass_graph(const LevelPassSelection& input,
    LevelPassGraph& graph, std::string& error)
{
    graph = {};
    if (input.mode == SurfaceMode::Unresolved ||
        (input.laemb && (input.mode != SurfaceMode::Opaque || !input.lightmapped || !input.environment)) ||
        (input.terrain && (input.mode != SurfaceMode::Opaque || !input.lightmapped ||
            input.detail || input.environment)) ||
        (input.detail && input.environment) ||
        ((input.wallmark_multiply || input.glow_alpha_add || input.fog) &&
            (input.detail || input.terrain || input.environment)))
    {
        error = "level pass graph: incompatible mode or missing required texture roles";
        return false;
    }
    auto& first = graph.passes[0];
    const bool transparent = input.mode == SurfaceMode::Transparent;
    first.target = transparent ? LevelPassTarget::Forward : LevelPassTarget::GBuffer;
    first.blend = input.wallmark_multiply ? LevelPassBlend::Multiply2x :
        input.glow_alpha_add ? LevelPassBlend::AlphaAdd :
        transparent ? LevelPassBlend::Alpha : LevelPassBlend::Replace;
    first.depth_write = !transparent;
    first.textures = BaseRole;
    if (input.lightmapped) first.textures |= LightmapRole;
    if (input.detail) first.textures |= DetailRole;
    if (input.environment) first.textures |= EnvironmentRole;
    if (input.terrain) first.textures |= MaskRole | TerrainRRole | TerrainGRole | TerrainBRole | TerrainARole;
    const char* family = input.wallmark_multiply || input.glow_alpha_add || input.fog ? "level_opaque" :
        transparent ? input.detail ? "level_detail_blended" :
            input.environment ? "level_environment_blended" : "object_blended" :
        input.terrain ? "level_terrain" :
        input.detail ? input.mode == SurfaceMode::AlphaTest ? "level_detail_cutout" :
            input.lightmapped ? "level_detail" : "level_detail_unlit" :
        input.laemb ? "level_laemb_base" :
        input.environment ? "level_environment" :
        input.lightmapped ? input.mode == SurfaceMode::AlphaTest ? "level_lightmap_cutout" : "level_lightmap" :
        input.mode == SurfaceMode::AlphaTest ? "level_cutout" : "level_opaque";
    first.vertex = std::string("vk\\") + family + ".vs";
    first.fragment = input.wallmark_multiply ? "vk\\wallmark_multiply_2x.ps" :
        input.glow_alpha_add ? "vk\\glow_alpha_add.ps" : input.fog ? "vk\\level_fog.ps" :
        transparent && input.detail && !input.lightmapped ? "vk\\level_detail_unlit_blended.ps" :
        input.mode == SurfaceMode::AlphaTest && input.detail && !input.lightmapped ?
            "vk\\level_detail_unlit_cutout.ps" : std::string("vk\\") + family + ".ps";
    graph.count = 1;
    if (input.laemb)
    {
        auto& second = graph.passes[1];
        second.vertex = "vk\\level_laemb_base.vs";
        second.fragment = "vk\\level_laemb_multiply.ps";
        second.target = LevelPassTarget::GBuffer;
        second.blend = LevelPassBlend::Multiply2x;
        second.textures = BaseRole;
        second.depth_test = true;
        second.depth_write = false;
        second.previous_color_pass = 0;
        graph.count = 2;
    }
    error.clear();
    return true;
}
}
