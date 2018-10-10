#include "StdAfx.h"
#include "player_state_cherub.h"
#include "kills_store.h"
#include "game_cl_base.h"
#include "Level.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_state_cherub::player_state_cherub(game_state_accumulator* owner) : inherited(owner)
{
    m_kill_count = 0;
    m_art_take_time = 0;
    m_bearer_id = u16(-1);
}

void player_state_cherub::reset_game()
{
    m_kill_count = 0;
    m_art_take_time = 0;
    m_bearer_id = u16(-1);
    m_bearer_name = "";
}

void player_state_cherub::OnPlayerTakeArtefact(game_PlayerState const* ps)
{
    if (m_owner->is_enemies(ps, m_owner->get_local_player()))
        return;

    m_bearer_id = ps->GameID;
    m_bearer_name = ps->getName();
    m_art_take_time = Device.dwTimeGlobal;
}

void player_state_cherub::OnPlayerDropArtefact(game_PlayerState const* ps)
{
    if (ps->GameID == m_bearer_id)
    {
        m_bearer_id = u16(-1);
        m_bearer_name = "";
    }
}

void player_state_cherub::OnPlayerBringArtefact(game_PlayerState const* ps)
{
    if (ps->GameID == m_bearer_id)
    {
        m_bearer_id = u16(-1);
        m_bearer_name = "";
    }
}

void player_state_cherub::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    struct hit_fetcher
    {
        bool operator()(shared_str const& hitter, shared_str const& victim, hits_store::bullet_hit const& hit)
        {
            if ((hitter == m_hitter_name) && (victim == m_victim_name) && (hit.m_hit_time >= m_art_take_time))
            {
                return true;
            }
            return false;
        }
        shared_str m_hitter_name;
        shared_str m_victim_name;
        u32 m_art_take_time;
    }; // struct hit_fetcher

    game_PlayerState* tmp_player_state = m_owner->get_local_player();
    if (!tmp_player_state)
        return;

    if (killer_id != tmp_player_state->GameID)
        return;

    if (m_bearer_id == u16(-1))
        return;

    if (m_bearer_name == tmp_player_state->getName())
        return;

    game_PlayerState* tmp_victim_player = Game().GetPlayerByGameID(target_id);
    if (!tmp_victim_player)
        return;

    hit_fetcher tmp_fetcher;
    tmp_fetcher.m_victim_name = m_bearer_name;
    tmp_fetcher.m_hitter_name = tmp_victim_player->getName();
    tmp_fetcher.m_art_take_time = m_art_take_time;

    buffer_vector<hits_store::bullet_hit> tmp_fake_buffer(NULL, 0);

    if (m_owner->get_hits_store().fetch_hits(tmp_fetcher, tmp_fake_buffer) > 0)
    {
        ++m_kill_count;
    }
}

void player_state_cherub::OnPlayerSpawned(game_PlayerState const* ps)
{
    if (ps == m_owner->get_local_player())
    {
        m_kill_count = 0;
    }
}

} // namespace award_system
