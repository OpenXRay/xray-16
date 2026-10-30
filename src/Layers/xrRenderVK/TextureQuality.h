#pragma once
#include <string>
#include <algorithm>
#include <cctype>
namespace xray::render::vulkan {
inline unsigned world_texture_limit(std::string name, const std::string& params)
{
    std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
    // Interface atlases, fonts, maps, loading screens and baked-light atlases
    // keep their original texel grids. Cubes are preserved by the DDS decoder.
    if(name.find("lmap") != std::string::npos) return 0;
    for(const char* prefix : {"ui", "intro", "map", "sky", "lmap", "wpn\\wpn_crosshair"})
        if(name.rfind(prefix,0)==0) return 0;
    if(params.find("-android-mobile-preset Minimum")!=std::string::npos) return 512;
    if(params.find("-android-mobile-preset Low")!=std::string::npos) return 1024;
    return 0;
}
}
