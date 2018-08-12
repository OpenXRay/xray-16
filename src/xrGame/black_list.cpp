#include "StdAfx.h"
#include "black_list.h"
#include "Level.h"
#include "game_cl_base.h"
#include "game_state_accumulator.h"

namespace award_system
{
black_list::black_list(game_state_accumulator* owner) : inherited(owner) {}
u32 const black_list::get_u32_param() { return m_victims.size(); }
void black_list::reset_game() { m_victims.clear(); }
void black_list::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (killer_id != tmp_local_player->GameID)
        return;

    game_PlayerState* victim_player = Game().GetPlayerByGameID(target_id);
    if (!victim_player)
        return;

    m_victims.insert(std::make_pair(shared_str(victim_player->getName()), Device.dwTimeGlobal));
}

void black_list::OnPlayerSpawned(game_PlayerState const* ps)
{
    if (ps == m_owner->get_local_player())
    {
        m_victims.clear();
    }
}

} // namespace award_system
