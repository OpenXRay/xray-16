#ifndef PLAYER_STATE_PARAM_INCLUDED
#define PLAYER_STATE_PARAM_INCLUDED

#include "game_events_handler.h"

namespace award_system
{

class game_state_accumulator;

class player_state_param : public game_events_handler
{
public:
						player_state_param			(game_state_accumulator* owner) : m_owner(owner) {}
	virtual				~player_state_param			()	{};
	virtual void		update						() = 0;
	virtual void		init_player					(game_PlayerState* new_local_player) {};
	virtual	u32 const	get_u32_param				() = 0;
	virtual float const get_float_param				() = 0;
	virtual void		reset_game					() = 0;

	virtual void		OnWeapon_Fire				(u16 sender, u16 sender_weapon_id) {};
	virtual	void		OnBullet_Fire				(u16 sender, u16 sender_weapon_id, const Fvector& position, const Fvector& direction) {};
	virtual void		OnBullet_Hit				(CObject const * hitter, CObject const * victim, CObject const * weapon, u16 const bone) {};
	virtual void		OnArtefactSpawned			() {};
	virtual void		OnPlayerTakeArtefact		(game_PlayerState const * ps) {};
	virtual void		OnPlayerDropArtefact		(game_PlayerState const * ps) {};
	virtual void		OnPlayerBringArtefact		(game_PlayerState const * ps) {};
	virtual void		OnPlayerSpawned				(game_PlayerState const * ps) {};
	virtual void		OnPlayerKilled				(u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type) {};
	virtual void		OnPlayerChangeTeam			(s8 team) {};
	virtual	void		OnPlayerRankChanged			() {};
	virtual void		OnRoundEnd					() {};
	virtual void		OnRoundStart				() {};
protected:
	game_state_accumulator*	m_owner;
};//class player_state_param

}//namespace award_system

#endif //#ifndef PLAYER_STATE_PARAM_INCLUDED