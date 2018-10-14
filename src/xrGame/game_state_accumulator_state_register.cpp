#include "StdAfx.h"
#include "game_state_accumulator.h"

// typelist:
#include "command_switch_counter.h"
#include "player_state_params.h"
#include "player_team_win_score.h"
#include "player_spot_params.h"
#include "player_state_blitzkrieg.h"
#include "player_state_multichampion.h"
#include "player_state_mad.h"
#include "player_state_achilles_heel.h"
#include "faster_than_bullets_time.h"
#include "harvest_time.h"
#include "player_state_skewer.h"
#include "double_shot_double_kill.h"
#include "player_state_climber.h"
#include "player_state_ammo_elapsed.h"
#include "player_state_opener.h"
#include "player_state_toughy.h"
#include "invincible_fury.h"
#include "sprinter_stopper.h"
#include "player_state_marksman.h"
#include "player_state_ambassador.h"
#include "player_state_remembrance.h"
#include "player_state_avenger.h"
#include "player_state_cherub.h"
#include "stalker_flair.h"
#include "black_list.h"
#include "silent_shots.h"
#include "killer_victim_velocity_angle.h"

#define INIT_ACCUMULATIVE_VALUE(Kind, SomeClass) \
    m_accumulative_values.insert(std::make_pair(Kind, new SomeClass(this)));

namespace award_system
{

void game_state_accumulator::init_accumulative_values()
{
    INIT_ACCUMULATIVE_VALUE(acpv_mad,                          player_state_mad);
    INIT_ACCUMULATIVE_VALUE(acpv_spots,                        player_spots_counter);
    INIT_ACCUMULATIVE_VALUE(acpv_toughy,                       player_state_toughy);
    INIT_ACCUMULATIVE_VALUE(acpv_avenger,                      player_state_avenger);
    INIT_ACCUMULATIVE_VALUE(acpv_climber,                      player_state_climber);
    INIT_ACCUMULATIVE_VALUE(acpv_move_state,                   player_state_move);
    INIT_ACCUMULATIVE_VALUE(acpv_ambassador,                   player_state_ambassador);
    INIT_ACCUMULATIVE_VALUE(acpv_black_list,                   black_list);
    INIT_ACCUMULATIVE_VALUE(acpv_kill_in_raw,                  player_rawkill_counter);
    INIT_ACCUMULATIVE_VALUE(acpv_death_count,                  player_death_counter);
    INIT_ACCUMULATIVE_VALUE(acpv_remembrance,                  player_state_remembrance);
    INIT_ACCUMULATIVE_VALUE(acpv_cherub_ready,                 player_state_cherub);
    INIT_ACCUMULATIVE_VALUE(acpv_ammo_elapsed,                 player_state_ammo_elapsed);
    INIT_ACCUMULATIVE_VALUE(acpv_opener_ready,                 player_state_opener);
    INIT_ACCUMULATIVE_VALUE(acpv_skewer_count,                 player_state_skewer);
    INIT_ACCUMULATIVE_VALUE(acpv_stalker_flair,                stalker_flair);
    INIT_ACCUMULATIVE_VALUE(acpv_thunder_count,                silent_shots);
    INIT_ACCUMULATIVE_VALUE(acpv_move_velocity,                player_state_velocity);
    INIT_ACCUMULATIVE_VALUE(acpv_harvest_count,                harvest_time);
    INIT_ACCUMULATIVE_VALUE(acpv_marksman_count,               player_state_marksman);
    INIT_ACCUMULATIVE_VALUE(acpv_multi_champion,               player_multichampion);
    INIT_ACCUMULATIVE_VALUE(acpv_invincible_fury,              player_state_invincible_fury);
    INIT_ACCUMULATIVE_VALUE(acpv_blitzkrieg_time,              player_blitzkrieg);
    INIT_ACCUMULATIVE_VALUE(acpv_enemy_team_score,             player_enemy_team_score);
    INIT_ACCUMULATIVE_VALUE(acpv_artdeliver_count,             player_artdeliver_counter);
    INIT_ACCUMULATIVE_VALUE(acpv_my_team_win_score,            player_team_win_score);
    INIT_ACCUMULATIVE_VALUE(acpv_move_ang_velocity,            player_state_ang_velocity);
    INIT_ACCUMULATIVE_VALUE(acpv_achilles_heel_ready,          achilles_heel_kill);
    INIT_ACCUMULATIVE_VALUE(acpv_killer_victim_angle,          killer_victim_angle);
    INIT_ACCUMULATIVE_VALUE(acpv_enemy_top_player_div,         player_spots_with_top_enemy_divider);
    INIT_ACCUMULATIVE_VALUE(acpv_command_switch_count,         command_switch_counter);
    INIT_ACCUMULATIVE_VALUE(acpv_enemy_team_score_now,         player_runtime_enemy_team_score);
    INIT_ACCUMULATIVE_VALUE(acpv_my_team_win_score_now,        player_runtime_win_score);
    INIT_ACCUMULATIVE_VALUE(acpv_sprinter_victim_velocity,     spritnter_stopper);
    INIT_ACCUMULATIVE_VALUE(acpv_faster_than_bullets_time,     faster_than_bullets_time);
    INIT_ACCUMULATIVE_VALUE(acpv_double_shot_double_kill_time, double_shot_double_kill);
}
} // namespace award_system
