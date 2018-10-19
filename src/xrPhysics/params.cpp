#include "StdAfx.h"

#include "params.h"
float object_damage_factor = 1.f; // times increace damage from object collision
void LoadParams()
{
    if (!pSettings)
        return;

    // collide_volume_min=pSettings->r_float("sound","snd_collide_min_volume");
    // collide_volume_max=pSettings->r_float("sound","snd_collide_max_volume");
    object_damage_factor = pSettings->r_float("physics", "object_damage_factor");
    object_damage_factor *= object_damage_factor;
}
