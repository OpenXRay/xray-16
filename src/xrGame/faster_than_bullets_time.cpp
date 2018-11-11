#include "StdAfx.h"
#include "faster_than_bullets_time.h"
#include "hits_store.h"
#include "game_state_accumulator.h"
#include "Level.h"

namespace award_system
{
faster_than_bullets_time::faster_than_bullets_time(game_state_accumulator* owner) : inherited(owner)
{
    m_no_demag_time = u32(-1);
}

u32 const faster_than_bullets_time::get_u32_param() { return m_no_demag_time; }
void faster_than_bullets_time::reset_game() { m_no_demag_time = u32(-1); }
struct last_hits_fetcher
{
    typedef hits_store::bullet_hits_map_t::key_type key_type;
    last_hits_fetcher(shared_str const& killer_name, shared_str const& victim_name)
        : m_killer_name(killer_name), m_victim_name(victim_name)
    {
        m_last_hit_time = 0;
    };

    last_hits_fetcher& operator=(last_hits_fetcher const& copy)
    {
        m_killer_name = copy.m_killer_name;
        m_victim_name = copy.m_victim_name;
        m_last_hit_time = copy.m_last_hit_time;
        return *this;
    }

    bool operator()(shared_str const& k_name, shared_str const& v_name, hits_store::bullet_hit const& hit)
    {
        if ((k_name == m_killer_name) && (v_name == m_victim_name))
        {
            if (hit.m_hit_time > m_last_hit_time)
            {
                m_last_hit_time = hit.m_hit_time;
                return true;
            }
        }
        return false;
    }

    shared_str m_killer_name;
    shared_str m_victim_name;
    u32 m_last_hit_time;
};

void faster_than_bullets_time::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player)
        return;

    if ((killer_id != tmp_local_player->GameID) || (kill_type.second != SKT_KNIFEKILL))
    {
        return;
    }

    IGameObject* victim_obj = Level().Objects.net_Find(target_id);
    if (!victim_obj)
        return;

    last_hits_fetcher tmp_predicate(victim_obj->cName(), tmp_local_player->getName());
    buffer_vector<hits_store::bullet_hit> tmp_hits_result(NULL, 0);

    if (m_owner->get_hits_store().fetch_hits(tmp_predicate, tmp_hits_result))
    {
        VERIFY(tmp_predicate.m_last_hit_time);
        m_no_demag_time = Device.dwTimeGlobal - tmp_predicate.m_last_hit_time;
    }
}

} // namespace award_system
