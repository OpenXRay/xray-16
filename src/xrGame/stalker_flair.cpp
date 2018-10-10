#include "StdAfx.h"
#include "game_cl_base.h"
#include "stalker_flair.h"
#include "game_state_accumulator.h"

namespace award_system
{
stalker_flair::stalker_flair(game_state_accumulator* owner) : inherited(owner)
{
    m_art_spawn_time = 0;
    m_art_take_time = 0;
}

u32 const stalker_flair::get_u32_param()
{
    if (!m_art_spawn_time || !m_art_take_time)
        return u32(-1);

    return m_art_take_time - m_art_spawn_time;
}

void stalker_flair::reset_game()
{
    m_art_spawn_time = 0;
    m_art_take_time = 0;
}

void stalker_flair::OnArtefactSpawned()
{
    m_art_spawn_time = Device.dwTimeGlobal;
    m_art_take_time = 0;
}

void stalker_flair::OnPlayerTakeArtefact(game_PlayerState const* ps)
{
    if (ps == m_owner->get_local_player())
    {
        m_art_take_time = Device.dwTimeGlobal;
    }
}

} // namespace award_system
