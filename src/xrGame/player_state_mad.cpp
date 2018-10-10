#include "StdAfx.h"
#include "player_state_mad.h"
#include "game_base.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_state_mad::player_state_mad(game_state_accumulator* owner) : inherited(owner) {}
void player_state_mad::reset_game() { m_kill_times.clear(); }
void player_state_mad::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if ((tmp_local_player->GameID == killer_id) &&
        ((kill_type.second == SKT_KNIFEKILL) || (kill_type.second == SKT_BACKSTAB)))
    {
        m_kill_times.push_back(Device.dwTimeGlobal);
    }
    clear_old_kills();
}

void player_state_mad::clear_old_kills()
{
    u32 current_time = Device.dwTimeGlobal;

    while (!m_kill_times.empty() && ((current_time - m_kill_times.front()) > mad_time_period))
    {
        m_kill_times.erase(m_kill_times.begin());
    }
}

u32 const player_state_mad::get_u32_param() { return m_kill_times.size(); }
} // namespace award_system
