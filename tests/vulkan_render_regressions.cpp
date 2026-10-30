#include "src/Layers/xrRenderVK/ParticleSprite.h"
#include "src/Layers/xrRenderVK/ShadowStability.h"
#include "src/Layers/xrRenderVK/TextureQuality.h"
#include "src/Layers/xrRenderVK/LevelLightmap.h"
#include "src/Layers/xrRenderVK/ModelPoseConstants.h"
#include "src/Layers/xrRenderVK/ShaderPairCache.h"
#include "src/Layers/xrRenderVK/VulkanSettings.h"
#include <cassert>
#include <array>
#include <cmath>
using namespace xray::render::vulkan;
int main() {
    quality_preset=4;detail_radius=80;ambient_scale=.1f;hemisphere_scale=.2f;
    apply_vulkan_launch_settings("-cop -android-mobile-preset Minimum -vk-detail-distance 0 -vk-ambient-scale 1 -vk-hemi-scale 1");
    assert(quality_preset==0 && detail_radius==0 && ambient_scale==1 && hemisphere_scale==1);
    quality_preset=0;
    apply_vulkan_launch_settings("-android-mobile-preset Default");assert(quality_preset==2);
    apply_vulkan_launch_settings("-android-mobile-preset Extreme -vk-detail-distance 20");
    assert(quality_preset==4 && detail_radius==20);
    apply_vulkan_launch_settings("-vk-hemi-scale nan -vk-detail-distance invalid");
    assert(hemisphere_scale==1 && detail_radius==20);
    ShaderPairCache<int> pair_cache;int first=11, replacement=22;
    assert(!pair_cache.find("model.vs","flat.ps"));
    pair_cache.remember("model.vs","flat.ps",&first);
    for(unsigned i=0;i<5000;++i)assert(pair_cache.find("model.vs","flat.ps")==&first);
    assert(!pair_cache.find("model.vs","other.ps"));
    for(unsigned i=0;i<1000;++i) {
        const std::string name="collision_"+std::to_string(i);
        pair_cache.remember(name.c_str(),"flat.ps",&replacement);
        const int* hit=pair_cache.find("model.vs","flat.ps");assert(!hit || hit==&first);
    }
    pair_cache.clear();assert(!pair_cache.find("model.vs","flat.ps"));
    pair_cache.remember("model.vs","flat.ps",&replacement);
    assert(pair_cache.find("model.vs","flat.ps")==&replacement);

    const float rotation_rows[]{0,0,1,.2f, 0,1,0,.4f, -1,0,0,.6f};
    const auto q=model_rotation_quaternion(rotation_rows);
    assert(std::abs(q[1]-.70710678f)<1e-5f && std::abs(q[3]-.70710678f)<1e-5f);
    for (float pos : {0.f,.2f,.9f,1.f}) for (float neg : {0.f,.3f,.8f,1.f}) {
        const float packed=pack_model_hemi(pos,neg);
        assert(std::abs(std::floor(packed)/1023.f-pos)<.001f);
        assert(std::abs((packed-std::floor(packed))*1024.f/1023.f-neg)<.001f);
    }
    assert(sun_shadow_span(0) == 64.f && sun_shadow_span(1) == 96.f);
    assert(sun_shadow_span(2) == 180.f && sun_shadow_span(4) == 180.f);
    assert(select_level_lightmap("base, color_lmap, hemi_lmap") == "hemi_lmap");
    assert(select_level_lightmap("base, color_lmap, hemi_lmap",true) == "color_lmap");
    assert(select_level_lightmap("base, legacy_lmap") == "legacy_lmap");
    assert(select_level_lightmap("base, legacy_lmap, $null") == "legacy_lmap");
    assert(select_level_lightmap("base").empty());
    for(unsigned corner=0;corner<4;++corner) {
        auto p=particle_sprite_corner(corner,2,3,0);
        assert((p.up>0)==(p.v==0));
        assert((p.right>0)==(p.u==1));
        auto rotated=particle_sprite_corner(corner,2,3,1.57079632679f);
        assert(std::abs(rotated.right+p.up)<1e-5f && std::abs(rotated.up-p.right)<1e-5f);
    }
    std::array<float,16> a{};a[0]=a[5]=a[10]=a[15]=1;
    auto b=a;a[12]=.0001f;b[12]=.0002f;a[13]=-.0003f;b[13]=-.0004f;
    stabilize_shadow_projection(a.data(),1024);stabilize_shadow_projection(b.data(),1024);
    assert(a==b); // Small camera motion does not slide the shadow texel grid.
    b[12]+=.002f;stabilize_shadow_projection(b.data(),1024);
    assert(std::abs(b[12]-a[12]-2.f/1024.f)<1e-6f);
    const std::string minimum="-cop -android-mobile-preset Minimum -renderer-vulkan";
    for(const char* name:{"ui\\ui_actor_menu","UI\\ui_font","intro\\intro_zaton_1","map\\zaton","lmap#1","sky\\sky_1_cube","wpn\\wpn_crosshair"})
        assert(world_texture_limit(name,minimum)==0);
    assert(world_texture_limit("act\\stalker",minimum)==512);
    assert(world_texture_limit("act\\stalker","-android-mobile-preset Low")==1024);
    assert(world_texture_limit("act\\stalker","-android-mobile-preset High")==0);
}
