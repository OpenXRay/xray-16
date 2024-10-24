#pragma once

#include "xrCore/Containers/AssociativeVector.hpp"

namespace gamespy_profile
{
enum enum_awards_t
{
    at_award_massacre = 0x00,
    at_award_paranoia,
    at_award_overwhelming_superiority,
    at_award_blitzkrieg,
    at_award_dry_victory,
    at_award_multichampion,
    at_award_mad,
    at_award_achilles_heel,
    at_award_fater_than_bullets,
    at_award_harvest_time,
    at_award_skewer,
    at_award_double_shot_double_kill,
    at_award_climber,
    at_award_opener,
    at_award_toughy,
    at_award_invincible_fury,
    at_award_oculist,
    at_award_lightning_reflexes,
    at_award_sprinter_stopper,
    at_award_marksman,
    at_award_peace_ambassador,
    at_award_deadly_accuracy,
    at_award_remembrance,
    at_award_avenger,
    at_award_cherub,
    at_award_dignity,
    at_award_stalker_flair,
    at_award_lucky,
    at_award_black_list,
    at_award_silent_death,
    // at_award_okulist	=	0x00,
    at_awards_count
}; // enum enum_awards_t

enum enum_award_params
{
    ap_award_id = 0x00,
    ap_award_rdate,
    ap_award_params_count
}; // enum enum_award_params

struct award_data
{
    award_data(u16 count, u32 const& rdate) : m_count(count), m_last_reward_date(rdate) {}
    u16 m_count;
    u32 m_last_reward_date;
};

using all_awards_t = AssociativeVector<enum_awards_t, award_data>;

enum enum_best_score_type
{
    bst_kills_in_row = 0x00,
    bst_kinife_kills_in_row,
    bst_backstabs_in_row,
    bst_head_shots_in_row,
    bst_eye_kills_in_row,
    bst_bleed_kills_in_row,
    bst_explosive_kills_in_row,
    bst_score_types_count
}; // enum enum_best_score_type

using all_best_scores_t = AssociativeVector<enum_best_score_type, s32>;
} // namespace gamespy_profile
