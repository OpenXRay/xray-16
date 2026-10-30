#pragma once
#include <string>
#include <vector>
namespace xray::render::vulkan {
inline std::string select_level_lightmap(const std::string& textures, bool color_lightmap = false)
{
    std::vector<std::string> slots;
    for(size_t start=0;start<=textures.size();) {
        const auto comma=textures.find(',',start);
        std::string slot=textures.substr(start,comma==std::string::npos?comma:comma-start);
        const auto first=slot.find_first_not_of(" \t");
        if(first==std::string::npos)slot.clear();
        else slot=slot.substr(first,slot.find_last_not_of(" \t")-first+1);
        slots.push_back(std::move(slot));
        if(comma==std::string::npos)break;
        start=comma+1;
    }
    // R2/CoP: base, RGB lightmap, hemi/sun lightmap. LaEmB's color pass
    // intentionally uses RGB slot 1. Preserve legacy two-slot level archives.
    if(!color_lightmap && slots.size()>2 && !slots[2].empty() && slots[2]!="$null") return slots[2];
    return slots.size()>1?slots[1]:std::string{};
}
}
