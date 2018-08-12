#include "StdAfx.h"
#include "player_state_ammo_elapsed.h"
#include "game_state_accumulator.h"
#include "Weapon.h"

namespace award_system
{
player_state_ammo_elapsed::player_state_ammo_elapsed(game_state_accumulator* owner) : inherited(owner){};

u32 const player_state_ammo_elapsed::get_u32_param()
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return u32(-1);

    CWeapon* tmp_active_weapon = m_owner->get_active_weapon(tmp_local_player);
    if (!tmp_active_weapon)
        return u32(-1);

    return static_cast<u32>(tmp_active_weapon->GetAmmoElapsed());
}

} // namespace award_system
