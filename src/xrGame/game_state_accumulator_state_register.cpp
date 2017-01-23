#include "stdafx.h"
#include "game_state_accumulator.h"

//typelist:
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

namespace award_system
{

#ifdef DEBUG
char* player_values_strtable[] =
{
	"command_switch_count",
	"kill_in_raw_count",
	"death_count",
	"spots",
	"my_team_win_score",
	"my_team_win_score_now",
	"enemy_top_player_div",
	"blitzkrieg_time",
	"enemy_team_score",
	"enemy_team_score_now",
	"artdeliver_counter",
	"multi_champion",
	"mad",
	"achilles_heel_ready",
	"faster_than_bullets_time",
	"harvest_count",
	"skewer_count",
	"double_shot_double_kill_time",
	"ammo_elapsed",
	"climber",
	"opener_ready",
	"invincible_fury",
	"move_state",
	"move_velocity",
	"move_angular_velocity",
	"sprinter_victim_velocity",
	"marksman_count",
	"ambassador",
	"remembrance",
	"avenger",
	"cherub_ready",
	"stalker_flair",
	"black_list",
	"thunder_count",
	"killer_victim_angle_cos",
};
#endif //#ifdef DEBUG

void game_state_accumulator::init_accumulative_values()
{
	STATIC_CHECK(Loki::TL::Length<ACCUMULATIVE_STATE_LIST>::value == acpv_count,
		Not_all_accumulative_values_has_been_added_to_a__ACCUMULATIVE_STATE_LIST__type_list);
	
	init_acpv_list<ACCUMULATIVE_STATE_LIST>();
}

} //namespace award_system
