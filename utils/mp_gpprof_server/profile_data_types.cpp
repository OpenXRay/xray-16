#include "profile_data_types.h"
#include <assert.h>
#include <string.h>

namespace gamespy_profile
{

char * profile_table_name = "PlayerStats_v1";

static char const * awards_names[at_awards_count] = 
{
	"mp_award_massacre",
	"mp_award_paranoia",
	"mp_award_overwhelming_superiority",
	"mp_award_blitzkrieg",
	"mp_award_dry_victory",
	"mp_award_multichampion",
	"mp_award_mad",
	"mp_award_achilles_heel",
	"mp_award_fater_than_bullets",
	"mp_award_harvest_time",
	"mp_award_skewer",
	"mp_award_double_shot_double_kill",
	"mp_award_climber",
	"mp_award_opener",
	"mp_award_toughy",
	"mp_award_invincible_fury",
	"mp_award_oculist",
	"mp_award_lightning_reflexes",
	"mp_award_sprinter_stopper",
	"mp_award_marksman",
	"mp_award_peace_ambassador",
	"mp_award_deadly_accuracy",
	"mp_award_remembrance",
	"mp_award_avenger",
	"mp_award_cherub",
	"mp_award_dignity",
	"mp_award_stalker_flair",
	"mp_award_lucky",
	"mp_award_black_list",
	"mp_award_silent_death",
};


static unsigned short award_atlas_stats_map[at_awards_count][ap_award_params_count] =
{
	{ STAT_AwardMasscare,					STAT_AwardMasscareLastRewardDate				},//mp_award_massacre
	{ STAT_AwardParanoia,					STAT_AwardParanoiaLastRewardDate				},//mp_award_paranoia
	{ STAT_AwardOwerwhelmingSuperiority,	STAT_AwardOwerwhelmingSuperiorityLastRewardDate	},//mp_award_overwhelming_superiority
	{ STAT_AwardBlitzkrieg,					STAT_AwardBlitzkriegLastRewardDate				},//mp_award_blitzkrieg
	{ STAT_AwardDryVictory,					STAT_AwardDryVictoryLastRewardDate				},//mp_award_dry_victory
	{ STAT_AwardMultichampion,				STAT_AwardMultichampionLastRewardDate			},//mp_award_multichampion
	{ STAT_AwardMad,						STAT_AwardMadLastRewardDate						},//mp_award_mad
	{ STAT_AwardAchillesHeel,				STAT_AwardAchillesHeelLastRewardDate			},//mp_award_achilles_heel
	{ STAT_AwardFasterThanBullets,			STAT_AwardFasterThanBulletsLastRewardDate		},//mp_award_fater_than_bullets
	{ STAT_AwardHarvestTime,				STAT_AwardHarvestTimeLastRewardDate				},//mp_award_harvest_time
	{ STAT_AwardSkewer,						STAT_AwardSkewerLastRewardDate					},//mp_award_skewer
	{ STAT_AwardDoubleShotDoubleKill,		STAT_AwardDoubleShotDoubleKillLastRewardDate	},//mp_award_double_shot_double_kill
	{ STAT_AwardClimber,					STAT_AwardClimberLastRewardDate					},//mp_award_climber
	{ STAT_AwardOpener,						STAT_AwardOpenerLastRewardDate					},//mp_award_opener
	{ STAT_AwardToughy,						STAT_AwardToughyLastRewardDate					},//mp_award_toughy
	{ STAT_AwardInvincibleFury,				STAT_AwardInvincibleFuryLastRewardDate			},//mp_award_invincible_fury
	{ STAT_AwardOculist,					STAT_AwardOculistLastRewardDate					},//mp_award_oculist
	{ STAT_AwardLightingReflexes,			STAT_AwardLightingReflexesLastRewardDate		},//mp_award_lightning_reflexes
	{ STAT_AwardSprinterStopper,			STAT_AwardSprinterStopperLastRewardDate			},//mp_award_sprinter_stopper
	{ STAT_AwardMarksman,					STAT_AwardMarksmanLastRewardDate				},//mp_award_marksman
	{ STAT_AwardPeaceAmbassador,			STAT_AwardPeaceAmbassadorLastRewardDate			},//mp_award_peace_ambassador
	{ STAT_AwardDeadlyAccuracy,				STAT_AwardDeadlyAccuracyLastRewardDate			},//mp_award_deadly_accuracy
	{ STAT_AwardRemembrance,				STAT_AwardRemembranceLastRewardDate				},//mp_award_remembrance
	{ STAT_AwardAvenger,					STAT_AwardAvengerLastRewardDate					},//mp_award_avenger
	{ STAT_AwardCherub,						STAT_AwardCherubLastRewardDate					},//mp_award_cherub
	{ STAT_AwardDignity,					STAT_AwardDignityLastRewardDate					},//mp_award_dignity
	{ STAT_AwardStalkerFlair,				STAT_AwardStalkerFlairLastRewardDate			},//mp_award_stalker_flair
	{ STAT_AwardLucky,						STAT_AwardLuckyLastRewardDate					},//mp_award_lucky
	{ STAT_AwardBlackList,					STAT_AwardBlackListLastRewardDate				},//mp_award_black_list
	{ STAT_AwardSilentDeath,				STAT_AwardSilentDeathLastRewardDate				},//mp_award_silent_death
};

char const * get_award_name(enum_awards_t award)
{
	assert(award < at_awards_count);
	return awards_names[award];
}

unsigned short get_award_id_stat(enum_awards_t award)
{
	assert(award < at_awards_count);
	return award_atlas_stats_map[award][ap_award_id];
}
unsigned short get_award_reward_date_stat(enum_awards_t award)
{
	assert(award < at_awards_count);
	return award_atlas_stats_map[award][ap_award_rdate];
}
enum_awards_t get_award_by_stat_id_name(char const * stat_name)
{
	for (int i = 0; i < at_awards_count; ++i)
	{
		if (!strcmp(stat_name, 
			ATLAS_GET_STAT_NAME(award_atlas_stats_map[i][ap_award_id])))
		{
			return static_cast<enum_awards_t>(i);
		}
	}
	return static_cast<enum_awards_t>(at_awards_count);
}

enum_awards_t get_award_by_stat_rdate_name(char const * stat_name)
{
	for (int i = 0; i < at_awards_count; ++i)
	{
		if (!strcmp(stat_name, 
			ATLAS_GET_STAT_NAME(award_atlas_stats_map[i][ap_award_rdate])))
		{
			return static_cast<enum_awards_t>(i);
		}
	}
	return static_cast<enum_awards_t>(at_awards_count);
}



// best scores -----

static char const * best_score_names[bst_score_types_count] = 
{
	"mp_bst_kills_in_row",
	"mp_bst_kinife_kills_in_row",
	"mp_bst_backstabs_in_row",
	"mp_bst_head_shots_in_row",
	"mp_bst_eye_kills_in_row",
	"mp_bst_bleed_kills_in_row",
	"mp_bst_explosive_kills_in_row"
}; //static char const * best_score_names[bst_score_types_count]

static unsigned short best_scores_atlas_stats_map[bst_score_types_count] =
{
	STAT_BestScore_KillsInRow,
	STAT_BestScore_KnifeKillsInRow,
	STAT_BestScore_BackstabsKillsInRow,
	STAT_BestScore_HeadshotsKillsInRow,
	STAT_BestScore_EyeKillsInRow,
	STAT_BestScore_BleedKillsInRow,
	STAT_BestScore_ExplosiveKillsInRow
}; //static u16 best_scores_atlas_stats_map[bst_score_types_count]

char const * get_best_score_name				(enum_best_score_type bst)
{
	assert(bst < bst_score_types_count);
	return best_score_names[bst];
}

unsigned short get_best_score_id_stat		(enum_best_score_type bst)
{
	assert(bst < bst_score_types_count);
	return best_scores_atlas_stats_map[bst];
}

enum_best_score_type	get_best_score_type_by_sname(char const * stat_name)
{
	for (int i = 0; i < bst_score_types_count; ++i)
	{
		if (!strcmp(
				stat_name, 
				ATLAS_GET_STAT_NAME(best_scores_atlas_stats_map[i])
			))
		{
			return static_cast<enum_best_score_type>(i);
		}
	}
	return static_cast<enum_best_score_type>(bst_score_types_count);
}

} //namespace gamespy_profile
