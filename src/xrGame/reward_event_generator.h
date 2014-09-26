#ifndef REWARD_EVENT_GENERATOR_INCLUDED
#define REWARD_EVENT_GENERATOR_INCLUDED

#include "game_cl_base_weapon_usage_statistic.h"
#include "profile_data_types.h"
#include <boost/noncopyable.hpp>

class atlas_submit_queue;

namespace award_system
{

class game_state_accumulator;
class rewarding_state_events;
class rewarding_event_handlers;
class best_scores_helper;

class reward_event_generator : public boost::noncopyable
{
public:
	explicit					reward_event_generator		(u32 const max_rewards_per_game);
	virtual						~reward_event_generator		();

	void						init_player					(game_PlayerState* local_player);	//must be called when buy menu and local_player will be initialized
	void						init_bone_groups			(CActor* first_spawned_actor);
	void						update						();

			void				OnWeapon_Fire				(u16 sender, u16 sender_weapon_id);
			void				OnBullet_Fire				(u16 sender, u16 sender_weapon_id, const Fvector& position, const Fvector& direction);
			void				OnBullet_Hit				(CObject const * hitter, CObject const * victim, CObject* weapon, u16 const bone);
			void				OnArtefactSpawned			();
			void				OnPlayerTakeArtefact		(game_PlayerState const * ps);
			void				OnPlayerDropArtefact		(game_PlayerState const * ps);
			void				OnPlayerBringArtefact		(game_PlayerState const * ps);
			void				OnPlayerSpawned				(game_PlayerState const * ps);
			void				OnPlayerKilled				(u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);
			void				OnPlayerChangeTeam			(s8 team);
			void				OnPlayerRankdChanged		();
			void				OnRoundEnd					();
			void				OnRoundStart				();
			
			void				CommitBestResults			();
private:
	game_PlayerState*						m_local_player;
	game_state_accumulator*					m_state_accum;
	rewarding_state_events*					m_state_event_checker;
	
	best_scores_helper*						m_best_scores_helper;

	rewarding_event_handlers*				m_event_handlers;
	atlas_submit_queue*						m_submit_queue;
	
	u32 const								m_max_rewards;
	u32										m_rewarded;
			void	__stdcall	AddRewardTask				(u32 award_id);
}; //class reward_event_generator

} //namespace award_system

#endif //#ifndef REWARD_EVENT_GENERATOR_INCLUDED