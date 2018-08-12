#include "StdAfx.h"
#include "harvest_time.h"
#include "kills_store.h"
#include "game_base.h"
#include "Level.h"
#include "game_state_accumulator.h"

namespace award_system
{
harvest_time::harvest_time(game_state_accumulator* owner) : inherited(owner)
{
    m_harvest_count = 0;
    m_spawn_time = 0;
}

u32 const harvest_time::get_u32_param() { return m_harvest_count; }
void harvest_time::reset_game()
{
    m_harvest_count = 0;
    m_spawn_time = 0;
}

struct victim_raw_kill
{
    victim_raw_kill(shared_str const& killer, shared_str const& victim, u32 const after_time)
        : m_killer(killer), m_victim(victim), m_after_time(after_time)
    {
    }

    victim_raw_kill& operator=(victim_raw_kill const& copy)
    {
        m_killer = copy.m_killer;
        m_victim = copy.m_victim;
        m_after_time = copy.m_after_time;
    }

    bool operator()(shared_str const& killer, shared_str const& victim, kills_store::kill const& kill)
    {
        if ((killer == m_killer) && (victim == m_victim) && (kill.m_kill_time >= m_after_time))
        {
            return true;
        }
        return false;
    }

    shared_str m_killer;
    shared_str m_victim;
    u32 m_after_time;
}; // struct victim_raw_kill

void harvest_time::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if (!m_spawn_time)
        return;

    if (killer_id != tmp_local_player->GameID)
        return;

    IGameObject* victim_obj = Level().Objects.net_Find(target_id);
    if (!victim_obj)
        return;

    victim_raw_kill tmp_predicate(tmp_local_player->getName(), victim_obj->cName(), m_spawn_time);

    buffer_vector<kills_store::kill> tmp_fake_buffer(NULL, 0);

    m_harvest_count = m_owner->get_kills_store().fetch_kills(tmp_predicate, tmp_fake_buffer);
}

void harvest_time::OnPlayerSpawned(game_PlayerState const* ps)
{
    if (ps == m_owner->get_local_player())
    {
        m_spawn_time = Device.dwTimeGlobal;
    }
}

} // namespace award_system
