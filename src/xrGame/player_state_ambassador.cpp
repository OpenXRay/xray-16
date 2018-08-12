#include "StdAfx.h"
#include "player_state_ambassador.h"
#include "game_cl_base.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_state_ambassador::player_state_ambassador(game_state_accumulator* owner) : inherited(owner)
{
    m_shots_count = u32(-1);
    m_art_drop_count = 0;
    m_delivered = false;
}

u32 const player_state_ambassador::get_u32_param()
{
    if (m_delivered && (m_shots_count == 0) && (m_art_drop_count == 0))
        return 1;

    return 0;
}

void player_state_ambassador::reset_game()
{
    m_shots_count = u32(-1);
    m_art_drop_count = 0;
    m_delivered = false;
}

void player_state_ambassador::OnWeapon_Fire(u16 sender, u16 sender_weapon_id)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();

    if (!tmp_local_player)
        return;

    if (sender != tmp_local_player->GameID)
        return;

    ++m_shots_count;
}

void player_state_ambassador::OnPlayerTakeArtefact(game_PlayerState const* ps)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();

    if (m_art_drop_count)
        return;

    if (!tmp_local_player)
        return;

    if (ps != tmp_local_player)
        return;

    if (m_art_drop_count > 0)
        return;

    m_shots_count = 0;
    m_delivered = false;
}

void player_state_ambassador::OnPlayerDropArtefact(game_PlayerState const* ps)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
    {
        ++m_art_drop_count;
        return;
    }
    if (tmp_local_player->team == ps->team)
    {
        ++m_art_drop_count;
    }
}

void player_state_ambassador::OnPlayerBringArtefact(game_PlayerState const* ps)
{
    m_art_drop_count = 0;

    game_PlayerState* tmp_local_player = m_owner->get_local_player();

    if (!tmp_local_player)
        return;

    if (ps != tmp_local_player)
        return;

    m_delivered = true;
}

} // namespace award_system
