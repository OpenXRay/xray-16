#include "StdAfx.h"
#include "rewarding_state_events.h"
#include "game_state_accumulator.h"
#include "profile_data_types.h"
#include "ammunition_groups.h"
#include "bone_groups.h"

namespace award_system
{
rewarding_state_events::rewarding_state_events(
    game_state_accumulator* pstate_accum, event_action_delegate_t ea_delegate)
    : inherited(pstate_accum, ea_delegate)
{
}

rewarding_state_events::~rewarding_state_events() {}
void rewarding_state_events::init()
{
    clear_events();

    add_event(
        add_accumm_value_condition(acpv_kill_in_raw, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 20), 1,
        eGameIDDeathmatch, gamespy_profile::at_award_massacre);

    add_event(add_and_condition(add_accumm_value_condition(
                                    acpv_death_count, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 0),
                  add_and_condition(add_accumm_value_condition(acpv_my_team_win_score,
                                        u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1),
                      add_and_condition(add_accumm_value_condition(acpv_command_switch_count,
                                            u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 0),
                          add_accumm_value_condition(
                              acpv_spots, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 15)))),
        1, eGameIDTeamDeathmatch | eGameIDCaptureTheArtefact, gamespy_profile::at_award_paranoia);

    add_event(add_and_condition(add_accumm_value_condition(acpv_my_team_win_score,
                                    u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1),
                  add_accumm_value_condition(
                      acpv_enemy_top_player_div, float_bfunc_cf::get_function(float_bfunc_cf::tt_greater_equal), 2.0f)),
        1, eGameIDTeamDeathmatch, gamespy_profile::at_award_overwhelming_superiority);

    add_event(add_accumm_value_condition(
                  acpv_blitzkrieg_time, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 180000),
        1, eGameIDCaptureTheArtefact, gamespy_profile::at_award_blitzkrieg);

    add_event(add_and_condition(add_accumm_value_condition(acpv_artdeliver_count,
                                    u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1),
                  add_and_condition(add_accumm_value_condition(acpv_my_team_win_score,
                                        u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 10),
                      add_accumm_value_condition(
                          acpv_enemy_team_score, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 0))),
        1, eGameIDCaptureTheArtefact | eGameIDArtefactHunt, gamespy_profile::at_award_dry_victory);

    add_event(
        add_accumm_value_condition(acpv_multi_champion, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1),
        1, eGameIDCaptureTheArtefact | eGameIDArtefactHunt, gamespy_profile::at_award_multichampion);

    add_event(add_accumm_value_condition(acpv_mad, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 3), 1,
        eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_mad);

    add_event(add_accumm_value_condition(
                  acpv_achilles_heel_ready, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_achilles_heel);

    add_event(add_and_condition(add_accumm_value_condition(acpv_faster_than_bullets_time,
                                    u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 3000),
                  add_accumm_value_condition(
                      acpv_faster_than_bullets_time, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 10000)),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_fater_than_bullets);

    add_event(
        add_accumm_value_condition(acpv_harvest_count, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 10),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_harvest_time);

    add_event(
        add_accumm_value_condition(acpv_skewer_count, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 2), 1,
        eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_skewer);

    add_event(
        add_and_condition(add_accumm_value_condition(acpv_double_shot_double_kill_time,
                              u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 1000),
            add_accumm_value_condition(acpv_ammo_elapsed, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 0)),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_double_shot_double_kill);

    add_event(add_accumm_value_condition(acpv_climber, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 4),
        1, eGameIDArtefactHunt | eGameIDCaptureTheArtefact, gamespy_profile::at_award_climber);

    add_event(
        add_accumm_value_condition(acpv_opener_ready, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1), 1,
        eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_opener);

    add_event(add_accumm_value_condition(acpv_toughy, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 5), 1,
        eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_toughy);

    add_event(
        add_accumm_value_condition(acpv_invincible_fury, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 5),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_invincible_fury);

    add_event(add_kill_condition_dist(2, ammunition_group::gid_any, KT_HIT, SKT_EYESHOT), 1,
        eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_oculist);

    add_event(add_and_condition(add_kill_condition_dist(1, ammunition_group::gid_any, KT_HIT, SKT_HEADSHOT, 0),
                  add_and_condition(add_accumm_value_condition(acpv_move_velocity,
                                        float_bfunc_cf::get_function(float_bfunc_cf::tt_greater_equal), 6.0f),
                      add_accumm_value_condition(acpv_move_ang_velocity,
                          float_bfunc_cf::get_function(float_bfunc_cf::tt_greater_equal), 10.0f))),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_lightning_reflexes);

    add_event(add_and_condition(add_accumm_value_condition(acpv_sprinter_victim_velocity,
                                    float_bfunc_cf::get_function(float_bfunc_cf::tt_greater_equal), 6.0f),
                  add_accumm_value_condition(acpv_killer_victim_angle,
                      float_bfunc_cf::get_function(float_bfunc_cf::tt_less_equal),
                      _cos(PI_DIV_2 - 0.345f)) //~ 90 +- 20 degrees
                  ),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_sprinter_stopper);

    add_event(
        add_accumm_value_condition(acpv_marksman_count, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 3),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_marksman);

    add_event(
        add_accumm_value_condition(acpv_ambassador, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1), 1,
        eGameIDCaptureTheArtefact, gamespy_profile::at_award_peace_ambassador);

    add_event(add_hit_condition_dist(1, ammunition_group::gid_sniper_rifels, bone_group::gid_eyes,
                  float_bfunc_cf::get_function(float_bfunc_cf::tt_greater_equal), 120.0f),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_deadly_accuracy);

    add_event(
        add_accumm_value_condition(acpv_remembrance, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1), 1,
        eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_remembrance);

    add_event(add_accumm_value_condition(acpv_avenger, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 3),
        1, eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact, gamespy_profile::at_award_avenger);

    add_event(
        add_accumm_value_condition(acpv_cherub_ready, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 2), 1,
        eGameIDArtefactHunt | eGameIDCaptureTheArtefact, gamespy_profile::at_award_cherub);

    add_event(add_and_condition(add_and_condition(add_accumm_value_condition(acpv_enemy_team_score_now,
                                                      u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 5),
                                    add_accumm_value_condition(acpv_artdeliver_count,
                                        u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1)),
                  add_accumm_value_condition(
                      acpv_my_team_win_score_now, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 1)),
        1, eGameIDArtefactHunt | eGameIDCaptureTheArtefact, gamespy_profile::at_award_dignity);

    add_event(
        add_accumm_value_condition(acpv_stalker_flair, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 2000),
        1, eGameIDArtefactHunt, gamespy_profile::at_award_stalker_flair);

    add_event(add_and_condition(add_accumm_value_condition(
                                    acpv_ammo_elapsed, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 0),
                  add_kill_condition_dist(1, ammunition_group::gid_assault, KT_HIT, SKT_NONE, 0)),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_lucky);

    add_event(
        add_accumm_value_condition(acpv_black_list, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 8), 1,
        eGameIDArtefactHunt | eGameIDCaptureTheArtefact, gamespy_profile::at_award_black_list);

    add_event(add_and_condition(add_accumm_value_condition(
                                    acpv_thunder_count, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_less_equal), 0),
                  add_and_condition(add_accumm_value_condition(acpv_my_team_win_score,
                                        u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 1),
                      add_accumm_value_condition(
                          acpv_spots, u32_bfunc_cf::get_function(u32_bfunc_cf::tt_greater_equal), 15))),
        1, eGameIDDeathmatch | eGameIDTeamDeathmatch | eGameIDArtefactHunt | eGameIDCaptureTheArtefact,
        gamespy_profile::at_award_silent_death);
}

} // namespace award_system
