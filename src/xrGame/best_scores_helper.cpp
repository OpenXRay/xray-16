#include "StdAfx.h"
#include "best_scores_helper.h"
#include "game_base.h"

namespace award_system
{
best_scores_helper::best_scores_helper(game_state_accumulator* pstate) : inherited(pstate)
{
    reset_stats();
    reset_max();
}

best_scores_helper::~best_scores_helper() {}
void best_scores_helper::reset_max()
{
    m_max_kills_in_row = 0;
    m_max_knife_kills_in_row = 0;
    m_max_backstab_kills_in_row = 0;
    m_max_headshots_kills_in_row = 0;
    m_max_eyeshots_kills_in_row = 0;
    m_max_bleed_kills_in_row = 0;
    m_max_explosive_kills_in_row = 0;
}

void best_scores_helper::reset_stats()
{
    m_kills_in_row = 0;
    m_knife_kills_in_row = 0;
    m_backstab_kills_in_row = 0;
    m_headshots_kills_in_row = 0;
    m_eyeshots_kills_in_row = 0;
    m_bleed_kills_in_row = 0;
    m_explosive_kills_in_row = 0;
    m_artefacts = 0;
}

void best_scores_helper::write_max()
{
    m_max_kills_in_row = std::max(m_kills_in_row, m_max_kills_in_row);
    m_max_knife_kills_in_row = std::max(m_max_knife_kills_in_row, m_knife_kills_in_row);
    m_max_backstab_kills_in_row = std::max(m_max_backstab_kills_in_row, m_backstab_kills_in_row);
    m_max_headshots_kills_in_row = std::max(m_max_headshots_kills_in_row, m_headshots_kills_in_row);
    m_max_eyeshots_kills_in_row = std::max(m_max_eyeshots_kills_in_row, m_eyeshots_kills_in_row);
    m_max_bleed_kills_in_row = std::max(m_max_bleed_kills_in_row, m_bleed_kills_in_row);
    m_max_explosive_kills_in_row = std::max(m_max_explosive_kills_in_row, m_explosive_kills_in_row);
}

void best_scores_helper::fill_best_results(gamespy_profile::all_best_scores_t& dest_br)
{
    using namespace gamespy_profile;
    write_max();
    dest_br.clear();
    dest_br.insert(std::make_pair(bst_kills_in_row, m_max_kills_in_row));
    dest_br.insert(std::make_pair(bst_kinife_kills_in_row, m_max_knife_kills_in_row));
    dest_br.insert(std::make_pair(bst_backstabs_in_row, m_max_backstab_kills_in_row));
    dest_br.insert(std::make_pair(bst_head_shots_in_row, m_max_headshots_kills_in_row));
    dest_br.insert(std::make_pair(bst_eye_kills_in_row, m_max_eyeshots_kills_in_row));
    dest_br.insert(std::make_pair(bst_bleed_kills_in_row, m_max_bleed_kills_in_row));
    dest_br.insert(std::make_pair(bst_explosive_kills_in_row, m_max_explosive_kills_in_row));
}

bool best_scores_helper::OnPlayerBringArtefact(game_PlayerState const* ps)
{
    game_PlayerState* local_player = m_player_state_accum->get_local_player();
    if (!local_player)
        return false;

    if (ps == local_player)
        ++m_artefacts;

    return false;
}

bool best_scores_helper::OnPlayerSpawned(game_PlayerState const* ps)
{
    game_PlayerState* local_player = m_player_state_accum->get_local_player();
    if (!local_player)
        return false;

    if (ps == local_player)
    {
        write_max();
        reset_stats();
    }

    return false;
}

bool best_scores_helper::OnRoundStart()
{
    reset_stats();
    reset_max();
    return false;
}

bool best_scores_helper::OnPlayerKilled(
    u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type)
{
    game_PlayerState* local_player = m_player_state_accum->get_local_player();
    if (!local_player)
        return false;

    if (killer_id != local_player->GameID)
        return false;

    if (!m_player_state_accum->is_enemies(killer_id, target_id))
        return false;

    if (kill_type.first == KT_BLEEDING)
    {
        ++m_bleed_kills_in_row;
    }

    u16 kill_weapon_id = m_player_state_accum->get_object_id(weapon_id);
    if (m_player_state_accum->is_item_in_group(kill_weapon_id, ammunition_group::gid_hand_grenades))
    {
        ++m_explosive_kills_in_row;
    }

    switch (kill_type.second)
    {
    case SKT_HEADSHOT: { ++m_headshots_kills_in_row;
    }
    break;
    case SKT_BACKSTAB: { ++m_backstab_kills_in_row;
    }
    break;
    case SKT_KNIFEKILL: { ++m_knife_kills_in_row;
    }
    break;
    case SKT_EYESHOT: { ++m_eyeshots_kills_in_row;
    }
    break;
    }; // switch (kill_type.second)

    ++m_kills_in_row;
    return false;
}

} // namespace award_system
