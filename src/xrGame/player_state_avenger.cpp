#include "StdAfx.h"
#include "player_state_avenger.h"
#include "kills_store.h"
#include "game_cl_base.h"
#include "Level.h"
#include "game_state_accumulator.h"

namespace award_system
{
player_state_avenger::player_state_avenger(game_state_accumulator* owner) : inherited(owner) { m_aveng_count = 0; };
void player_state_avenger::reset_game()
{
    m_player_spawns.clear();
    m_aveng_count = 0;
}

void player_state_avenger::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    struct need_revenge : Noncopyable
    {
        need_revenge(shared_str* buff_store, u32 store_size) : m_my_team_players(buff_store, store_size){};

        bool operator()(shared_str const& killer, shared_str const& victim, kills_store::kill const& kill)
        {
            if ((killer == m_killer_name) && (kill.m_kill_time >= m_killer_spawn_time))
            {
                if (std::find(m_my_team_players.begin(), m_my_team_players.end(), victim) != m_my_team_players.end())
                {
                    return true;
                }
            }
            return false;
        }
        shared_str m_killer_name;
        u32 m_killer_spawn_time;
        buffer_vector<shared_str> m_my_team_players;
    }; // struct need_revenge

    game_PlayerState const* tmp_local_player = m_owner->get_local_player();
    if (!tmp_local_player || tmp_local_player->GameID != killer_id)
        return;

    game_PlayerState const* tmp_victim = Game().GetPlayerByGameID(target_id);
    if (!tmp_victim)
        return;

    shared_str team_players_store[MAX_PLAYERS_COUNT];
    need_revenge tmp_predicate(team_players_store, sizeof(team_players_store) / sizeof(shared_str));
    tmp_predicate.m_killer_name = tmp_victim->getName();

    player_spawn_times_t::const_iterator tmp_spawn_time = m_player_spawns.find(tmp_predicate.m_killer_name);

    if (tmp_spawn_time == m_player_spawns.end())
        return;

    tmp_predicate.m_killer_spawn_time = tmp_spawn_time->second;
    feel_my_team_players(tmp_local_player, tmp_predicate.m_my_team_players);

    buffer_vector<kills_store::kill> tmp_fake_buffer(NULL, 0);
    if (m_owner->get_kills_store().fetch_kills(tmp_predicate, tmp_fake_buffer) > 0)
    {
        ++m_aveng_count;
    }
}

void player_state_avenger::feel_my_team_players(game_PlayerState const* of_player, buffer_vector<shared_str>& dest)
{
    game_PlayerState const* tmp_local_player = m_owner->get_local_player();
    for (game_cl_GameState::PLAYERS_MAP_CIT i = Game().players.begin(), ie = Game().players.end(); i != ie; ++i)
    {
        if (i->second == tmp_local_player)
            continue;

        if (!m_owner->is_enemies(of_player, i->second))
        {
            dest.push_back(i->second->getName());
        }
    };
}

void player_state_avenger::OnPlayerSpawned(game_PlayerState const* ps)
{
    game_PlayerState const* tmp_local_player = m_owner->get_local_player();
    if (ps == tmp_local_player)
    {
        m_aveng_count = 0;
        return;
    }
    if (tmp_local_player && m_owner->is_enemies(tmp_local_player, ps))
    {
        m_player_spawns.insert(std::make_pair(shared_str(ps->getName()), Device.dwTimeGlobal));
    }
}

} // namespace award_system
