#ifndef BEST_SCORES_HELPER_INCLUDED
#define BEST_SCORES_HELPER_INCLUDED

#include "profile_data_types.h"
#include "reward_event_handler.h"

namespace award_system
{

class best_scores_helper : public reward_event_handler
{
	typedef reward_event_handler inherited;
public:
							best_scores_helper			(game_state_accumulator* pstate);
	virtual					~best_scores_helper			();

	virtual bool			OnWeapon_Fire				(u16 sender, u16 sender_weapon_id) { return false;};
	virtual	bool			OnBullet_Fire				(u16 sender, u16 sender_weapon_id, const Fvector& position, const Fvector& direction) { return false; };
	virtual bool			OnBullet_Hit				(CObject const * hitter, CObject const * victim, CObject const * weapon, u16 const bone) { return false; };
	virtual bool			OnArtefactSpawned			() { return false; };
	virtual bool			OnPlayerTakeArtefact		(game_PlayerState const * ps) { return false; };
	virtual bool			OnPlayerDropArtefact		(game_PlayerState const * ps) { return false; };
	virtual bool			OnPlayerBringArtefact		(game_PlayerState const * ps);
	virtual bool			OnPlayerSpawned				(game_PlayerState const * ps);
	virtual bool			OnPlayerKilled				(u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);
	virtual bool			OnPlayerChangeTeam			(s8 team) { return false; };
	virtual bool			OnRoundEnd					() { return false; };
	virtual bool			OnRoundStart				();
	virtual	bool			OnPlayerRankChanged			() { return false; };

			void			fill_best_results			(gamespy_profile::all_best_scores_t & dest_br);
private:
	int		m_kills_in_row;
	int		m_knife_kills_in_row;
	int		m_backstab_kills_in_row;
	int		m_headshots_kills_in_row;
	int		m_eyeshots_kills_in_row;
	int		m_bleed_kills_in_row;
	int		m_explosive_kills_in_row;
	int		m_artefacts;

	int		m_max_kills_in_row;
	int		m_max_knife_kills_in_row;
	int		m_max_backstab_kills_in_row;
	int		m_max_headshots_kills_in_row;
	int		m_max_eyeshots_kills_in_row;
	int		m_max_bleed_kills_in_row;
	int		m_max_explosive_kills_in_row;

			void			reset_stats					();
			void			reset_max					();
			void			write_max					();
}; //class best_scores_helper

}//namespace award_system

#endif //#ifndef BEST_SCORES_HELPER_INCLUDED