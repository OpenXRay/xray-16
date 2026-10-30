#include "src/Layers/xrRenderVK/ParticleSprite.h"
#include "src/Layers/xrRenderVK/ShadowStability.h"
#include "src/Layers/xrRenderVK/TextureQuality.h"
#include "src/Layers/xrRenderVK/LevelLightmap.h"
#include <cassert>
#include <array>
#include <cmath>
using namespace xray::render::vulkan;
int main() {
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
