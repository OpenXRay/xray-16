#pragma once

#include "LevelModels.h"

#include <array>
#include <cstdint>
#include <string>

namespace xray::render::vulkan
{
enum class LevelPassTarget : uint8_t { GBuffer, Forward, Hud, Ui };
enum class LevelPassBlend : uint8_t { Replace, Alpha, Multiply, Multiply2x, Additive, AlphaAdd };
enum LevelTextureRole : uint16_t
{
    BaseRole = 1u << 0, LightmapRole = 1u << 1, DetailRole = 1u << 2,
    MaskRole = 1u << 3, TerrainRRole = 1u << 4, TerrainGRole = 1u << 5,
    TerrainBRole = 1u << 6, TerrainARole = 1u << 7, EnvironmentRole = 1u << 8,
    BumpRole = 1u << 9, BumpAuxRole = 1u << 10, NormalRole = 1u << 11,
    GlossRole = 1u << 12, EmissiveRole = 1u << 13
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

// A single executable description is shared by level, model, particle, HUD,
// and screen consumers. The recorder selects the target; pipeline names and
// texture roles are resolved here, before command recording begins.
using RenderPassNode = LevelPassNode;
using RenderPassGraph = LevelPassGraph;
enum class RenderConsumer : uint8_t { Model, Particle, Ui };
struct RenderPassSelection
{
    RenderConsumer consumer{RenderConsumer::Model};
    SurfaceMode mode{SurfaceMode::Unresolved};
    bool hud{}, animated{}, environment{}, double_sided{};
    uint8_t skin_weights{};
    int blend_mode{1};
    uint16_t surface_roles{};
};

inline bool build_render_pass_graph(const RenderPassSelection& input,
    RenderPassGraph& graph, std::string& error)
{
    graph = {};
    if (input.mode == SurfaceMode::Unresolved ||
        (input.consumer == RenderConsumer::Ui && (input.blend_mode < 0 || input.blend_mode > 9)) ||
        (input.consumer == RenderConsumer::Particle &&
            (input.blend_mode < 0 || input.blend_mode > 5)) ||
        (input.surface_roles & BumpAuxRole && !(input.surface_roles & BumpRole)) ||
        (input.surface_roles & GlossRole && !(input.surface_roles & (BumpRole | NormalRole))) ||
        (input.consumer == RenderConsumer::Model && input.animated &&
            (input.skin_weights < 1 || input.skin_weights > 4)) ||
        (input.consumer == RenderConsumer::Model && input.environment &&
            input.mode != SurfaceMode::Transparent))
    {
        error = "render pass graph: incompatible surface mode or geometry layout";
        return false;
    }
    auto& node = graph.passes[0];
    node.textures = BaseRole | input.surface_roles;
    if (input.consumer == RenderConsumer::Particle &&
        (input.blend_mode == 2 || input.blend_mode == 5)) node.textures |= EmissiveRole;
    node.target = input.consumer == RenderConsumer::Ui ? LevelPassTarget::Ui :
        input.hud ? LevelPassTarget::Hud :
        input.mode == SurfaceMode::Transparent || input.consumer == RenderConsumer::Particle ?
            LevelPassTarget::Forward : LevelPassTarget::GBuffer;
    node.depth_test = node.target != LevelPassTarget::Ui;
    node.depth_write = node.target == LevelPassTarget::GBuffer || node.target == LevelPassTarget::Hud;
    const bool replace = (input.consumer == RenderConsumer::Ui &&
            (input.blend_mode == 0 || input.blend_mode == 7)) ||
        (input.consumer == RenderConsumer::Particle && input.blend_mode == 0) ||
        node.target == LevelPassTarget::GBuffer;
    node.blend = replace ? LevelPassBlend::Replace :
        input.blend_mode == 4 || input.blend_mode == 6 ? LevelPassBlend::Multiply2x :
        input.blend_mode == 3 ? LevelPassBlend::Multiply :
        input.blend_mode == 2 ? LevelPassBlend::Additive :
        input.blend_mode == 5 ? LevelPassBlend::AlphaAdd : LevelPassBlend::Alpha;
    if (input.consumer == RenderConsumer::Particle)
    {
        const char* mode = input.blend_mode == 0 ? "set" :
            input.blend_mode == 5 ? "alpha_add" : input.blend_mode == 4 ? "multiply_2x" :
            input.blend_mode == 3 ? "multiply" : input.blend_mode == 2 ? "additive" : "blended";
        node.vertex = "vk\\level_opaque.vs";
        node.fragment = std::string("vk\\particle_") + (input.hud ? "hud_" : "world_") + mode + ".ps";
    }
    else if (input.consumer == RenderConsumer::Ui)
    {
        // ScenePass owns the screen shader pair and its pipeline cache.
        node.vertex.clear();
        node.fragment.clear();
    }
    else
    {
        const char* family = input.environment ? "object_environment" :
            input.mode == SurfaceMode::Transparent ? "object_blended" :
            input.mode == SurfaceMode::AlphaTest ? "object_cutout" :
            input.double_sided && !input.animated ? "object_double_sided" : "object_opaque";
        node.vertex = input.hud ? input.animated ?
            "vk\\hud_skinned_" + std::to_string(input.skin_weights) + ".vs" : std::string{} :
            input.animated ? "vk\\skinned_" + std::to_string(input.skin_weights) + ".vs" :
                std::string("vk\\") + family + ".vs";
        node.fragment = input.hud ? input.animated ? "vk\\hud_blended.ps" : std::string{} :
            input.animated && input.environment ? "vk\\skinned_environment.ps" :
            input.animated && input.mode == SurfaceMode::Transparent ? "vk\\skinned_blended.ps" :
                std::string("vk\\") + family + ".ps";
        if (input.environment) node.textures |= EnvironmentRole;
    }
    graph.count = 1;
    error.clear();
    return true;
}

struct LevelPassSelection
{
    SurfaceMode mode{SurfaceMode::Unresolved};
    bool lightmapped{}, terrain{}, detail{}, environment{}, laemb{};
    bool wallmark_multiply{}, glow_alpha_add{}, fog{};
    uint16_t surface_roles{};
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
    if (input.glow_alpha_add) first.textures |= EmissiveRole;
    if (input.terrain) first.textures |= MaskRole | TerrainRRole | TerrainGRole | TerrainBRole | TerrainARole;
    first.textures |= input.surface_roles;
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

inline bool build_render_pass_graph(const LevelPassSelection& input,
    RenderPassGraph& graph, std::string& error)
{
    return build_level_pass_graph(input, graph, error);
}
}
