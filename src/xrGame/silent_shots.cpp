#include "StdAfx.h"
#include "silent_shots.h"
#include "game_base.h"
#include "game_state_accumulator.h"
#include "Weapon.h"
#include "WeaponKnife.h"

namespace award_system
{
silent_shots::silent_shots(game_state_accumulator* owner) : inherited(owner)
{
    m_thunder_count = 0;
    m_last_shoot_weapon = 0;
}

void silent_shots::reset_game()
{
    m_thunder_count = 0;
    m_last_shoot_weapon = 0;
}

void silent_shots::OnWeapon_Fire(u16 sender, u16 sender_weapon_id)
{
    if (m_last_shoot_weapon == sender_weapon_id)
        return;

    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (sender != tmp_local_player->GameID)
        return;

    CWeapon* shoot_weapon = m_owner->get_active_weapon(tmp_local_player);
    if (!shoot_weapon || (sender_weapon_id != shoot_weapon->ID()))
        return;

    if (shoot_weapon->IsSilencerAttached())
    {
        m_last_shoot_weapon = sender_weapon_id;
        return;
    }

    if (smart_cast<CWeaponKnife*>(shoot_weapon))
    {
        m_last_shoot_weapon = sender_weapon_id;
        return;
    }

    ++m_thunder_count;
}

} // namespace award_system
