#include "StdAfx.h"
#include "player_state_skewer.h"
#include "game_base.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_state_skewer::player_state_skewer(game_state_accumulator* owner) : inherited(owner)
{
    m_shot = 1;
    m_kills_count = 0;
}

u32 const player_state_skewer::get_u32_param() { return m_kills_count; }
void player_state_skewer::reset_game()
{
    m_shot = 1;
    m_kills_count = 0;
}

void player_state_skewer::OnWeapon_Fire(u16 sender, u16 sender_weapon_id)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (sender != tmp_local_player->GameID)
        return;

    ++m_shot;
}

void player_state_skewer::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (killer_id != tmp_local_player->GameID)
        return;

    if (m_owner->is_item_in_group(m_owner->get_object_id(weapon_id), ammunition_group::gid_gauss_rifle))
    {
        if (m_shot)
        {
            m_kills_count = 1;
            m_shot = 0;
        }
        else
        {
            ++m_kills_count;
        }
    }
}

} // namespace award_system
