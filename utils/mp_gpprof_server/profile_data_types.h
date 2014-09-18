#ifndef PLAYER_AWARDS
#define PLAYER_AWARDS

#include "atlas_stalkercoppc_v1.h"
#include <memory.h>
#include <utility>
#include <memory>
#include <string>

namespace gamespy_profile
{

enum enum_awards_t
{
	at_award_massacre	=	0x00,
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
	//at_award_okulist	=	0x00,
	at_awards_count
}; //enum enum_awards_t

enum enum_award_params
{
	ap_award_id				= 0x00,
	ap_award_rdate,
	ap_award_params_count
}; //enum enum_award_params

struct award_data
{
	award_data() :
		m_count(0),
		m_last_reward_date(0)
	{
	}
	award_data(unsigned short count, unsigned int const & rdate) :
		m_count(count),
		m_last_reward_date(rdate)
	{
	}
	unsigned short	m_count;
	unsigned int	m_last_reward_date;
};

extern char const *		get_award_name				(enum_awards_t award);
extern unsigned short	get_award_id_stat			(enum_awards_t award);
extern unsigned short	get_award_reward_date_stat	(enum_awards_t award);
extern enum_awards_t	get_award_by_stat_id_name	(char const * stat_name);
extern enum_awards_t	get_award_by_stat_rdate_name(char const * stat_name);

enum enum_best_score_type
{
	bst_kills_in_row			=	0x00,
	bst_kinife_kills_in_row,
	bst_backstabs_in_row,
	bst_head_shots_in_row,
	bst_eye_kills_in_row,
	bst_bleed_kills_in_row,
	bst_explosive_kills_in_row,
	bst_score_types_count
}; //enum enum_best_score_type

char const *			get_best_score_name				(enum_best_score_type bst);
unsigned short			get_best_score_id_stat			(enum_best_score_type bst);
enum_best_score_type	get_best_score_type_by_sname	(char const * stat_name);

extern char * profile_table_name;


typedef award_data awards_map_t[at_awards_count];
typedef unsigned int bestscores_map_t[bst_score_types_count];

struct profile_data
{
	profile_data()
	{
		memset(&m_awards, 0, sizeof(m_awards));
		memset(&m_best_scores, 0, sizeof(m_best_scores));
	}
	awards_map_t		m_awards;
	bestscores_map_t	m_best_scores;
};

} //namespace gamespy_profile

#endif //#ifndef PLAYER_AWARDS
