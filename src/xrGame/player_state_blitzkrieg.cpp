#include "StdAfx.h"
#include "game_cl_base.h"
#include "player_state_blitzkrieg.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_blitzkrieg::player_blitzkrieg(game_state_accumulator* owner) : inherited(owner)
{
    m_art_drop_count = 0;
    m_deliver_time = u32(-1);
}

void player_blitzkrieg::reset_game()
{
    m_art_drop_count = 0;
    m_deliver_time = u32(-1);
    m_take_time = 0;
}

void player_blitzkrieg::OnPlayerTakeArtefact(game_PlayerState const* ps)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if ((m_art_drop_count == 0) && (ps == tmp_local_player))
    {
        m_take_time = Device.dwTimeGlobal;
    }
    else
    {
        m_take_time = 0;
    }
    m_deliver_time = u32(-1);
}

void player_blitzkrieg::OnPlayerDropArtefact(game_PlayerState const* ps)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    // if there was dropping before delivering then, artefacts not on base ..
    if (ps->team == tmp_local_player->team)
    {
        ++m_art_drop_count;
    }
}

void player_blitzkrieg::OnPlayerBringArtefact(game_PlayerState const* ps)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if ((m_art_drop_count <= 1) && (ps == tmp_local_player))
    {
        m_deliver_time = Device.dwTimeGlobal - m_take_time;
    }
    else
    {
        m_deliver_time = u32(-1);
    }
    m_art_drop_count = 0;
}

u32 const player_blitzkrieg::get_u32_param() { return m_deliver_time; };
} // namespace award_system
