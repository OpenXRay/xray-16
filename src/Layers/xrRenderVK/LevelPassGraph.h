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
    bool detail{};
    int screen_blend{-1}; // Serialized S_SET mode when used by an OGF model.
    bool particle_screen_set{};
};

inline bool build_render_pass_graph(const RenderPassSelection& input,
    RenderPassGraph& graph, std::string& error)
{
    graph = {};
    const auto reject = [&](const char* reason)
    { error = std::string("render pass graph: incompatible ") + reason; return false; };
    if (input.mode == SurfaceMode::Unresolved) return reject("unresolved material mode");
    if (input.consumer == RenderConsumer::Ui && (input.blend_mode < 0 || input.blend_mode > 9))
        return reject("S_SET Blending (expected 0..9)");
    if (input.consumer == RenderConsumer::Particle && (input.blend_mode < 0 || input.blend_mode > 5))
        return reject("PARTICLE Blending (expected 0..5)");
    if (input.particle_screen_set && input.consumer != RenderConsumer::Particle)
        return reject("S_SET particle requires particle consumer");
    if (input.surface_roles & BumpAuxRole && !(input.surface_roles & BumpRole))
        return reject("BumpAux texture without Bump");
    if (input.surface_roles & GlossRole && !(input.surface_roles & (BumpRole | NormalRole)))
        return reject("Gloss texture without Bump or Normal");
    if (input.consumer == RenderConsumer::Model && input.animated &&
        (input.skin_weights < 1 || input.skin_weights > 4))
        return reject("animated model skin weights (expected 1..4)");
    if (input.consumer == RenderConsumer::Model && input.detail && input.environment)
        return reject("model diffuse detail with environment cube");
    if (input.consumer == RenderConsumer::Model && input.environment &&
        input.mode != SurfaceMode::Transparent)
        return reject("opaque MODELEbB does not sample environment in original deferred pass");
    if (input.consumer == RenderConsumer::Model && input.hud && input.environment)
        return reject("HUD model environment cube (HUD reflection shader missing)");
    if (input.consumer == RenderConsumer::Model && input.screen_blend >= 0)
    {
        if (input.screen_blend > 9) return reject("OGF S_SET Blending (expected 0..9)");
        if (input.mode != SurfaceMode::Transparent)
            return reject("OGF S_SET requires world forward pass");
        if (input.detail || input.environment)
            return reject("OGF S_SET with detail or environment texture roles");
        auto& world = graph.passes[0];
        world.target = input.hud ? LevelPassTarget::Hud : LevelPassTarget::Forward;
        world.depth_test = true;
        world.depth_write = input.screen_blend == 0;
        world.blend = input.screen_blend == 0 || input.screen_blend == 7 ? LevelPassBlend::Replace :
            input.screen_blend == 2 ? LevelPassBlend::Additive :
            input.screen_blend == 3 ? LevelPassBlend::Multiply :
            input.screen_blend == 4 || input.screen_blend == 6 ? LevelPassBlend::Multiply2x :
            input.screen_blend == 5 ? LevelPassBlend::AlphaAdd : LevelPassBlend::Alpha;
        world.textures = BaseRole | (input.screen_blend == 5 ? EmissiveRole : 0);
        world.vertex = input.animated ? std::string(input.hud ? "vk\\hud_skinned_" : "vk\\skinned_") +
            std::to_string(input.skin_weights) + ".vs" :
            std::string("vk\\object_blended.vs");
        const char* fragments[]{"set", "blended", "additive", "multiply",
            "multiply_2x", "alpha_add", "wallmark_multiply_2x", "set_2r",
            "blend_2r", "blend_4r"};
        world.fragment = std::string(input.hud ? "vk\\hud_model_set_" : "vk\\model_set_") +
            fragments[input.screen_blend] + ".ps";
        graph.count = 1;
        error.clear();
        return true;
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
    // PARTICLE SET is depth tested and writes depth even in the forward
    // attachment, per CBlender_Particle::Compile (AREF 200).
    node.depth_write = node.target == LevelPassTarget::GBuffer || node.target == LevelPassTarget::Hud ||
        (input.consumer == RenderConsumer::Particle && input.blend_mode == 0);
    if (input.particle_screen_set) node.depth_write = input.blend_mode == 0;
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
        node.fragment = input.particle_screen_set && input.hud ?
            std::string("vk\\hud_particle_screen_") + mode + ".ps" :
            input.particle_screen_set && input.blend_mode == 0 ? "vk\\world_set_set.ps" :
            input.particle_screen_set && input.blend_mode == 1 ? "vk\\world_set_blended.ps" :
            input.particle_screen_set && input.blend_mode == 5 ? "vk\\glow_alpha_add.ps" :
            std::string("vk\\particle_") + (input.hud ? "hud_" : "world_") + mode + ".ps";
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
        if (input.detail && !input.hud && !input.environment)
        {
            const char* suffix = input.mode == SurfaceMode::Transparent ? "blended" :
                input.mode == SurfaceMode::AlphaTest ? "cutout" : "opaque";
            node.fragment = std::string("vk\\") + (input.animated ? "skinned_detail_" :
                input.double_sided ? "object_detail_double_sided_" : "object_detail_") + suffix + ".ps";
            node.textures |= DetailRole;
        }
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
    int screen_blend{-1};
};

inline bool build_level_pass_graph(const LevelPassSelection& input,
    LevelPassGraph& graph, std::string& error)
{
    graph = {};
    const auto reject = [&](const char* reason)
    { error = std::string("level pass graph: incompatible ") + reason; return false; };
    if (input.mode == SurfaceMode::Unresolved) return reject("unresolved material mode");
    if (input.screen_blend > 9) return reject("S_SET Blending (expected 0..9)");
    if (input.screen_blend == 0 && input.mode != SurfaceMode::Transparent)
        return reject("S_SET Blending=0 requires depth-writing forward pass");
    if (input.laemb && (input.mode != SurfaceMode::Opaque || !input.lightmapped || !input.environment))
        return reject("LaEmB requires opaque mode, lightmap and environment");
    if (input.terrain && (input.mode != SurfaceMode::Opaque || !input.lightmapped ||
        input.detail || input.environment))
        return reject("terrain requires opaque mode and lightmap without single detail or environment");
    if (input.detail && input.environment) return reject("detail and environment texture roles");
    if ((input.wallmark_multiply || input.glow_alpha_add || input.fog) &&
        (input.detail || input.terrain || input.environment))
        return reject("world effect with detail, terrain or environment");
    if (((input.screen_blend >= 2 && input.screen_blend <= 4) ||
            input.screen_blend >= 7) &&
        (input.mode != SurfaceMode::Transparent || input.lightmapped || input.detail ||
            input.terrain || input.environment || input.fog))
        return reject("S_SET blending requires unlit transparent world geometry");
    if (input.surface_roles & BumpAuxRole && !(input.surface_roles & BumpRole))
        return reject("BumpAux texture without Bump");
    if (input.surface_roles & GlossRole && !(input.surface_roles & (BumpRole | NormalRole)))
        return reject("Gloss texture without Bump or Normal");
    auto& first = graph.passes[0];
    const bool transparent = input.mode == SurfaceMode::Transparent;
    first.target = transparent ? LevelPassTarget::Forward : LevelPassTarget::GBuffer;
    first.blend = input.screen_blend == 0 || input.screen_blend == 7 ? LevelPassBlend::Replace :
        input.screen_blend == 2 ? LevelPassBlend::Additive :
        input.screen_blend == 3 ? LevelPassBlend::Multiply :
        input.screen_blend == 4 || input.wallmark_multiply ? LevelPassBlend::Multiply2x :
        input.glow_alpha_add ? LevelPassBlend::AlphaAdd :
        transparent ? LevelPassBlend::Alpha : LevelPassBlend::Replace;
    first.depth_write = !transparent || input.screen_blend == 0;
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
    if ((input.screen_blend >= 0 && input.screen_blend <= 4) || input.screen_blend >= 7)
        first.vertex = "vk\\level_opaque.vs";
    first.fragment = input.screen_blend == 0 ? "vk\\world_set_set.ps" :
        input.wallmark_multiply ? "vk\\wallmark_multiply_2x.ps" :
        input.glow_alpha_add ? "vk\\glow_alpha_add.ps" : input.fog ? "vk\\level_fog.ps" :
        input.screen_blend == 1 && !input.fog ? "vk\\world_set_blended.ps" :
        input.screen_blend == 2 ? "vk\\world_set_additive.ps" :
        input.screen_blend == 3 ? "vk\\world_set_multiply.ps" :
        input.screen_blend == 4 ? "vk\\world_set_multiply_2x.ps" :
        input.screen_blend == 7 ? "vk\\world_set_set_2r.ps" :
        input.screen_blend == 8 ? "vk\\world_set_blend_2r.ps" :
        input.screen_blend == 9 ? "vk\\world_set_blend_4r.ps" :
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

// Detail prototypes use LevelVertex without a baked lightmap. Their forward
// variant must retain world lighting/fog and the optional diffuse detail slot.
inline bool build_vegetation_pass_graph(SurfaceMode mode, bool detail,
    uint16_t surface_roles,
    LevelPassGraph& graph, std::string& error)
{
    LevelPassSelection selection;
    selection.mode = mode;
    selection.detail = detail;
    selection.surface_roles = surface_roles;
    if (!build_level_pass_graph(selection, graph, error))
        return false;
    if (mode == SurfaceMode::Transparent)
    {
        graph.passes[0].vertex = "vk\\level_detail_blended.vs";
        graph.passes[0].fragment = detail ? "vk\\level_vegetation_detail_blended.ps" :
            "vk\\level_vegetation_blended.ps";
    }
    return true;
}
}
