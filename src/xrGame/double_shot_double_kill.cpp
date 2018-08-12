#include "StdAfx.h"
#include "double_shot_double_kill.h"
#include "game_base.h"
#include "Level.h"
#include "game_state_accumulator.h"

namespace award_system
{
double_shot_double_kill::double_shot_double_kill(game_state_accumulator* owner) : inherited(owner) { m_shot_count = 0; }
u32 const double_shot_double_kill::get_u32_param()
{
    if (m_kills.size() == 2)
    {
        kills_times_t::const_iterator last_shot = m_kills.begin();
        ++last_shot;
        kills_times_t::const_iterator first_shot = m_kills.begin();
        if ((last_shot->m_shot_number - first_shot->m_shot_number) != 1)
            return u32(-1);

        return (last_shot->m_shot_time - first_shot->m_shot_time);
    }
    return u32(-1);
}

void double_shot_double_kill::reset_game()
{
    m_kills.clear();
    m_shot_count = 0;
}

void double_shot_double_kill::OnWeapon_Fire(u16 sender, u16 sender_weapon_id)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (sender != tmp_local_player->GameID)
        return;

    ++m_shot_count;
}

void double_shot_double_kill::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (killer_id != tmp_local_player->GameID)
        return;

    if (!m_owner->is_item_in_group(m_owner->get_object_id(weapon_id), ammunition_group::gid_double_barred))
    {
        return;
    }
    kill_shot_id tmp_kill_shot;
    tmp_kill_shot.m_shot_number = m_shot_count;
    tmp_kill_shot.m_shot_time = Device.dwTimeGlobal;
    m_kills.push_obsolete(tmp_kill_shot);
}
}
