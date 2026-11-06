#include "src/Layers/xrRenderVK/VulkanGameLightingMath.h"
#include "src/Layers/xrRenderVK/LevelLights.h"

#include <cassert>
#include <cmath>
#include <limits>

using namespace xray::render::vulkan;

int main()
{
    std::vector<uint8_t> flight(108),baked(104);
    const auto integer=[](auto& b,size_t at,uint32_t v) { for(unsigned i=0;i<4;++i)b[at+i]=uint8_t(v>>(8*i)); };
    const auto real=[&](auto& b,size_t at,float value) { uint32_t bits;std::memcpy(&bits,&value,4);integer(b,at,bits); };
    integer(flight,4,1);real(flight,8,.5f);real(flight,12,.4f);real(flight,16,.3f);
    real(flight,56,10);real(flight,80,20);
    std::vector<VulkanLightSnapshot> decoded;std::string error;
    assert(decode_level_lights({flight.data(),flight.size()},false,decoded,error));
    assert(decoded.size()==1 && decoded[0].active && decoded[0].position[0]==10 && decoded[0].range==20 && decoded[0].color[1]==.4f);
    std::vector<VulkanLightSnapshot> runtime(1), scene;
    runtime[0].id = 17;
    runtime[0].shadow = true;
    collect_scene_lights(scene, runtime, decoded, false);
    assert(scene.size() == 1 && scene[0].id == 17 && scene[0].shadow);
    collect_scene_lights(scene, runtime, decoded, true);
    assert(scene.size() == 2 && scene[1].id == decoded[0].id);
    collect_scene_lights(scene, runtime, decoded, false);
    assert(scene.size() == 1);
    assert(!decode_level_lights({flight.data(),flight.size()-1},false,decoded,error) && decoded.size()==1);
    real(flight,8,std::numeric_limits<float>::quiet_NaN());
    assert(!decode_level_lights({flight.data(),flight.size()},false,decoded,error) && decoded[0].color[0]==.5f);
    integer(flight,4,3); // Directional sun remains controlled by weather.
    assert(decode_level_lights({flight.data(),flight.size()},false,decoded,error) && decoded.empty());
    integer(baked,0,1);for(unsigned i=0;i<3;++i) real(baked,4+i*4,1);
    real(baked,24,5);real(baked,40,10);real(baked,48,.01f);real(baked,52,2);real(baked,56,.1f);
    assert(decode_level_lights({baked.data(),baked.size()},true,decoded,error));
    assert(decoded.size()==1 && decoded[0].hemi_only && decoded[0].position[2]==5);
    VulkanHemiLightGrid grid;std::vector<VulkanLightSnapshot> many=decoded;
    for(unsigned i=0;i<1000;++i) {auto light=decoded[0];light.position={float(i+10)*64,0,0};many.push_back(light);}
    grid.reset(std::move(many));std::vector<VulkanLightSnapshot> nearby;
    grid.append({0,0,0},nearby);assert(nearby.size()==1);
    nearby.clear();grid.append({0,100,0},nearby);assert(nearby.empty());
    nearby.clear();grid.append({-1,0,0},nearby);assert(nearby.size()==1);
    grid.reset();nearby.clear();grid.append({0,0,0},nearby);assert(nearby.empty());
    const float ambient[]{0.1f, 0.1f, 0.1f};
    const float hemi[]{0.2f, 0.2f, 0.2f};
    const float sun[]{0.5f, 0.5f, 0.5f};
    const std::array<float, 3> origin{0.f, 0.f, 0.f};
    const auto from_baked=evaluate_object_lighting(origin,ambient,hemi,sun,VulkanTraceAll,decoded);
    assert(std::abs(from_baked.hemi_cube[2]-.563f)<1e-5f);
    assert(std::abs(from_baked.hemi_cube[5]-.507f)<1e-5f);

    VulkanLightSnapshot point;
    point.active = true;
    point.position = {0.f, 0.f, 5.f};
    point.color = {0.5f, 0.5f, 0.5f};
    point.range = 10.f;
    const std::vector<VulkanLightSnapshot> point_lights{point};
    const VulkanObjectLighting lit = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, point_lights);
    // The original local-light attenuation is 1 - (distance / range)^2.
    assert(lit.luminocity > 0.77f && lit.luminocity < 0.78f);
    assert(lit.hemi_luminocity > 0.57f && lit.hemi_luminocity < 0.58f);
    assert(lit.hemi_cube[2] > lit.hemi_cube[5]);

    point.active = false;
    const VulkanObjectLighting unlit = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, {point});
    assert(unlit.luminocity < lit.luminocity);
    assert(unlit.hemi_cube[4] == unlit.hemi_cube[5]);
    const float dark_sky[]{.001f,.002f,.003f};
    const auto dark=evaluate_object_lighting(origin,ambient,dark_sky,sun,VulkanTraceAll,{});
    assert(dark.hemi_cube==unlit.hemi_cube);
    assert(dark.hemi_cube[0]==.5f && dark.luminocity<unlit.luminocity);

    // A light on +Z must reach the shader's positive Z face, not negative X.
    assert(lit.hemi_cube[VulkanShaderHemiFaceOrder[2]] >
        lit.hemi_cube[VulkanShaderHemiFaceOrder[5]]);
    assert(lit.hemi_cube[VulkanShaderHemiFaceOrder[3]] ==
        lit.hemi_cube[VulkanShaderHemiFaceOrder[4]]);

    point.active = true;
    point.type = VulkanLightType::Spot;
    point.direction = {0.f, 0.f, -1.f};
    const VulkanObjectLighting spot_lit = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, {point});
    point.direction = {0.f, 0.f, 1.f};
    const VulkanObjectLighting spot_dark = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, {point});
    assert(spot_lit.luminocity > spot_dark.luminocity);

    point.type = VulkanLightType::Direct;
    const VulkanObjectLighting direct_excluded = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceAll, {point});
    assert(direct_excluded.luminocity == unlit.luminocity);

    point.type = VulkanLightType::Point;
    point.color.fill(std::numeric_limits<float>::quiet_NaN());
    const VulkanObjectLighting sanitized = evaluate_object_lighting(origin, ambient, hemi, sun,
        VulkanTraceLights, {point});
    assert(std::isfinite(sanitized.luminocity));
    assert(sanitized.luminocity == 0.f);
}
