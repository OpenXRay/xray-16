#include "StdAfx.h"
#include "invincible_fury.h"
#include "kills_store.h"
#include "game_cl_base.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_state_invincible_fury::player_state_invincible_fury(game_state_accumulator* owner) : inherited(owner)
{
    m_last_kills = 0;
}

u32 const player_state_invincible_fury::get_u32_param() { return m_last_kills; }
void player_state_invincible_fury::reset_game() { m_last_kills = 0; }
struct fury_killer
{
    fury_killer(shared_str const& killer_name, u32 after_time) : m_killer_name(killer_name), m_after_time(after_time) {}
    fury_killer& operator=(fury_killer const& copy)
    {
        m_killer_name = copy.m_killer_name;
        m_after_time = copy.m_after_time;
    }

    bool operator()(shared_str const& killer, shared_str const& victim, kills_store::kill const& kill)
    {
        if ((killer == m_killer_name) && (kill.m_kill_time >= m_after_time))
        {
            return true;
        }
        return false;
    }
    shared_str m_killer_name;
    u32 m_after_time;
}; // struct fury_killer

void player_state_invincible_fury::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();

    if (!tmp_local_player)
        return;

    if (killer_id != tmp_local_player->GameID)
        return;

    fury_killer tmp_predicate(tmp_local_player->getName(), Device.dwTimeGlobal - max_fury_time);
    kills_store::kill tmp_kills_store[kills_store::max_kills_count];
    buffer_vector<kills_store::kill> tmp_buffer(tmp_kills_store, kills_store::max_kills_count);

    m_owner->get_kills_store().fetch_kills(tmp_predicate, tmp_buffer);
    m_last_kills = 0;
    for (buffer_vector<kills_store::kill>::const_iterator i = tmp_buffer.begin(), ie = tmp_buffer.end(); i != ie; ++i)
    {
        if (m_owner->is_item_in_group(i->m_weapon_id, ammunition_group::gid_assault) ||
            m_owner->is_item_in_group(i->m_weapon_id, ammunition_group::gid_sniper_rifels) ||
            m_owner->is_item_in_group(i->m_weapon_id, ammunition_group::gid_shotguns) ||
            m_owner->is_item_in_group(i->m_weapon_id, ammunition_group::gid_pistols) ||
            m_owner->is_item_in_group(i->m_weapon_id, ammunition_group::gid_knife))
        {
            ++m_last_kills;
        }
    }
}

} // namespace award_system
