#include "StdAfx.h"
#include "player_state_opener.h"
#include "game_cl_base.h"
#include "Level.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_state_opener::player_state_opener(game_state_accumulator* owner) : inherited(owner) { m_opener_ready = false; }
u32 const player_state_opener::get_u32_param() { return m_opener_ready ? 1 : 0; }
void player_state_opener::reset_game() { m_opener_ready = false; }
void player_state_opener::OnPlayerKilled(
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

    u16 victim_armor = m_owner->get_armor_of_player(victim_player);
    if ((kill_type.second == SKT_KNIFEKILL) &&
        m_owner->is_item_in_group(victim_armor, ammunition_group::gid_exo_outfit))
    {
        m_opener_ready = true;
    }
}

} // namespace award_system
