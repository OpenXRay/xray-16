#pragma once

#include <string_view>

// These two impact atlases encode surface refraction. Other liquid effects
// contain airborne splashes and keep their authored particle alignment.
inline bool is_water_contact_effect(std::string_view name)
{
    return name == "hit_fx\\effects\\hit_water_hit_distort" ||
           name == "hit_fx\\effects\\hit_water_hit_distort_big";
}
