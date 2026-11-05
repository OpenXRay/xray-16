#include "src/Layers/xrRenderVK/DetailInstance.h"
#include "src/Layers/xrRenderVK/LevelLightmap.h"
#include "src/Layers/xrRenderVK/LevelModels.h"
#include "src/Layers/xrRenderVK/ModelPoseConstants.h"
#include "src/Layers/xrRenderVK/ObjectVisibility.h"
#include "src/Layers/xrRenderVK/ParticleSprite.h"
#include "src/Layers/xrRenderVK/ResourceRetirement.h"
#include "src/Layers/xrRenderVK/ShaderPairCache.h"
#include "src/Layers/xrRenderVK/ShadowStability.h"
#include "src/Layers/xrRenderVK/TangentBasis.h"
#include "src/Layers/xrRenderVK/TextureQuality.h"
#include "src/Layers/xrRenderVK/VulkanSettings.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
using namespace xray::render::vulkan;
int main()
{
    const auto blade = detail_instance(10, 2, 3, 1.57079632679f, 2);
    const float point[]{1, 0, 0};
    const auto transformed = detail_transform(blade, point);
    assert(std::abs(transformed[0] - 10) < 1e-5f && std::abs(transformed[1] - 2) < 1e-5f &&
           std::abs(transformed[2] - 1) < 1e-5f);
    assert(sizeof(DetailInstance) * 10000 < sizeof(LevelVertex) * 4 * 10000);
    struct Retired
    {
        int *count;
        ~Retired()
        {
            ++*count;
        }
    };
    int deleted = 0;
    ResourceRetirement retirement(2);
    retirement.retire(std::unique_ptr<Retired>(new Retired{&deleted}));
    retirement.complete(0);
    retirement.complete(0);
    assert(deleted == 0);
    retirement.retire(std::unique_ptr<Retired>(new Retired{&deleted}));
    retirement.complete(1);
    assert(deleted == 1 && retirement.size() == 1);
    retirement.complete(1);
    assert(deleted == 1);
    retirement.complete(0);
    assert(deleted == 2 && retirement.size() == 0);
    retirement.retire(std::unique_ptr<Retired>(new Retired{&deleted}));
    retirement.drain();
    assert(deleted == 3);
    retirement.defer([&] { ++deleted; });
    retirement.complete(0);
    assert(deleted == 3);
    retirement.complete(1);
    assert(deleted == 4);
    assert(glow_facing({0, 0, 0}, {0, 0, 1}, {0, 0, -1}, .1f) == 0);
    assert(glow_facing({0, 0, 0}, {0, 0, 1}, {0, 0, .05f}, .1f) == 0);
    assert(glow_facing({0, 0, 0}, {0, 0, 1}, {0, 0, 1}, .1f) == 1);
    ObjectVisibility visibility;
    visibility.position({0, 0, 0});
    unsigned budget = 2, calls = 0;
    auto blocked = [&](const auto &, const auto &) {
        ++calls;
        return true;
    };
    assert(visibility.light(1, {1, 0, 0}, 1, budget, blocked) == 0 && budget == 1 && calls == 1);
    assert(visibility.light(1, {1, 0, 0}, 2, budget, blocked) == 0 && budget == 1 && calls == 1);
    assert(visibility.light(1, {2, 0, 0}, 3, budget, blocked) == 0 && budget == 0 && calls == 2);
    budget = 0;
    visibility.sky(budget, blocked);
    assert(calls == 2);
    budget = 26;
    for (unsigned i = 0; i < 26; ++i)
        visibility.sky(budget, blocked);
    for (float face : visibility.faces(.08f))
        assert(face > 0 && face < .01f);
    std::vector<LevelVertex> frame_vertices(3);
    frame_vertices[1].position[0] = 1;
    frame_vertices[2].position[1] = 1;
    frame_vertices[1].uv[0] = 1;
    frame_vertices[2].uv[1] = 1;
    prepare_tangent_basis(frame_vertices, {0, 1, 2});
    for (const auto &v : frame_vertices)
        assert(v.tangent[0] == 127 && v.bitangent[1] == 127);
    for (auto &v : frame_vertices)
    {
        v.uv[0] *= -1;
        v.tangent[0] = v.bitangent[1] = 0;
    }
    prepare_tangent_basis(frame_vertices, {0, 1, 2});
    for (const auto &v : frame_vertices)
        assert(v.tangent[0] == -127 && v.bitangent[1] == 127);
    // Nonzero authored vectors can still be parallel to the geometric normal.
    // Keeping them forces a camera-derivative fallback at grazing angles.
    for (auto &v : frame_vertices)
    {
        v.normal[0] = v.normal[1] = 0;
        v.normal[2] = 1;
        for (unsigned k = 0; k < 3; ++k)
            v.tangent[k] = v.bitangent[k] = k == 2 ? 127 : 0;
    }
    prepare_tangent_basis(frame_vertices, {0, 1, 2});
    for (const auto &v : frame_vertices)
    {
        assert(valid_tangent_basis(v));
        assert(v.tangent[0] == -127 && v.bitangent[1] == 127);
    }
    const auto valid_frames = frame_vertices;
    prepare_tangent_basis(frame_vertices, {0, 1, 2});
    assert(std::memcmp(frame_vertices.data(), valid_frames.data(), frame_vertices.size() * sizeof(LevelVertex)) ==
           0);
    assert(render_quality(0).texture_limit < render_quality(2).texture_limit);
    assert(render_quality(4).point_shadows == 2 && render_quality(0).point_shadows == 0);

    for (float pitch : {0.f, .5f, 1.4f, 1.57079632679f})
    {
        const auto basis = particle_path_basis({0, 1, 0}, {0, -std::sin(pitch), std::cos(pitch)});
        assert(basis.up[0] == 0 && basis.up[1] == 1 && basis.up[2] == 0);
        float length = 0, dot = 0;
        for (unsigned i = 0; i < 3; ++i)
        {
            length += basis.right[i] * basis.right[i];
            dot += basis.right[i] * basis.up[i];
        }
        assert(std::abs(length - 1) < 1e-5f && std::abs(dot) < 1e-5f);
    }
    LevelModel decal;
    decal.vertices.resize(1);
    decal.vertices[0].position[0] = 12;
    decal.vertices[0].normal[1] = 1;
    decal.vertices[0].uv[0] = .3f;
    offset_level_wallmark(decal);
    assert(decal.vertices[0].position[0] == 12 && std::abs(decal.vertices[0].position[1] - .007f) < 1e-7f &&
           decal.vertices[0].uv[0] == .3f);
    quality_preset = 4;
    detail_radius = 80;
    ambient_scale = .1f;
    hemisphere_scale = .2f;
    apply_vulkan_launch_settings(
        "-cop -android-mobile-preset Minimum -vk-detail-distance 0 -vk-ambient-scale 1 -vk-hemi-scale 1");
    assert(quality_preset == 0 && detail_radius == 0 && ambient_scale == 1 && hemisphere_scale == 1);
    quality_preset = 0;
    apply_vulkan_launch_settings("-android-mobile-preset Default");
    assert(quality_preset == 2);
    apply_vulkan_launch_settings("-android-mobile-preset Extreme -vk-detail-distance 20");
    assert(quality_preset == 4 && detail_radius == 20);
    apply_vulkan_launch_settings("-vk-hemi-scale nan -vk-detail-distance invalid");
    assert(hemisphere_scale == 1 && detail_radius == 20);
    ShaderPairCache<int> pair_cache;
    int first = 11, replacement = 22;
    assert(!pair_cache.find("model.vs", "flat.ps"));
    pair_cache.remember("model.vs", "flat.ps", &first);
    for (unsigned i = 0; i < 5000; ++i)
        assert(pair_cache.find("model.vs", "flat.ps") == &first);
    assert(!pair_cache.find("model.vs", "other.ps"));
    for (unsigned i = 0; i < 1000; ++i)
    {
        const std::string name = "collision_" + std::to_string(i);
        pair_cache.remember(name.c_str(), "flat.ps", &replacement);
        const int *hit = pair_cache.find("model.vs", "flat.ps");
        assert(!hit || hit == &first);
    }
    pair_cache.clear();
    assert(!pair_cache.find("model.vs", "flat.ps"));
    pair_cache.remember("model.vs", "flat.ps", &replacement);
    assert(pair_cache.find("model.vs", "flat.ps") == &replacement);

    const float rotation_rows[]{0, 0, 1, .2f, 0, 1, 0, .4f, -1, 0, 0, .6f};
    const auto q = model_rotation_quaternion(rotation_rows);
    assert(std::abs(q[1] - .70710678f) < 1e-5f && std::abs(q[3] - .70710678f) < 1e-5f);
    for (float pos : {0.f, .2f, .9f, 1.f})
        for (float neg : {0.f, .3f, .8f, 1.f})
        {
            const float packed = pack_model_hemi(pos, neg);
            assert(std::abs(std::floor(packed) / 1023.f - pos) < .001f);
            assert(std::abs((packed - std::floor(packed)) * 1024.f / 1023.f - neg) < .001f);
        }
    assert(sun_shadow_span(0) == 64.f && sun_shadow_span(1) == 96.f);
    assert(sun_shadow_span(2) == 120.f && sun_shadow_span(4) == 180.f);
    assert(select_level_lightmap("base, color_lmap, hemi_lmap") == "hemi_lmap");
    assert(select_level_lightmap("base, color_lmap, hemi_lmap", true) == "color_lmap");
    assert(select_level_lightmap("base, legacy_lmap") == "legacy_lmap");
    assert(select_level_lightmap("base, legacy_lmap, $null") == "legacy_lmap");
    assert(select_level_lightmap("base").empty());
    for (unsigned corner = 0; corner < 4; ++corner)
    {
        auto p = particle_sprite_corner(corner, 2, 3, 0);
        assert((p.up > 0) == (p.v == 0));
        assert((p.right > 0) == (p.u == 1));
        auto rotated = particle_sprite_corner(corner, 2, 3, 1.57079632679f);
        assert(std::abs(rotated.right + p.up) < 1e-5f && std::abs(rotated.up - p.right) < 1e-5f);
    }
    std::array<float, 16> a{};
    a[0] = a[5] = a[10] = a[15] = 1;
    auto b = a;
    a[12] = .0001f;
    b[12] = .0002f;
    a[13] = -.0003f;
    b[13] = -.0004f;
    stabilize_shadow_projection(a.data(), 1024);
    stabilize_shadow_projection(b.data(), 1024);
    assert(a == b); // Small camera motion does not slide the shadow texel grid.
    b[12] += .002f;
    stabilize_shadow_projection(b.data(), 1024);
    assert(std::abs(b[12] - a[12] - 2.f / 1024.f) < 1e-6f);
    for (const auto &direction : {std::array<float, 3>{0, 1, 0}, std::array<float, 3>{0, 0, 0}})
    {
        const auto plane = particle_face_basis(direction);
        for (unsigned corner = 0; corner < 4; ++corner)
        {
            const auto vertex = particle_sprite_corner(corner, 2, 3, .7f);
            assert(std::abs(plane.right[1] * vertex.right + plane.up[1] * vertex.up) < 1e-6f);
        }
    }
    const std::string minimum = "-cop -android-mobile-preset Minimum -renderer-vulkan";
    for (const char *name : {"ui\\ui_actor_menu", "UI\\ui_font", "intro\\intro_zaton_1", "map\\zaton", "lmap#1",
                             "sky\\sky_1_cube", "wpn\\wpn_crosshair"})
        assert(world_texture_limit(name, minimum) == 0);
    assert(world_texture_limit("act\\stalker", minimum) == 512);
    assert(world_texture_limit("act\\stalker", "-android-mobile-preset Low") == 1024);
    assert(world_texture_limit("act\\stalker", "-android-mobile-preset High") == 0);
}
