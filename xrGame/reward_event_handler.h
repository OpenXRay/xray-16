#ifndef REWARD_EVENT_HANDLER
#define REWARD_EVENT_HANDLER

#include "game_state_accumulator.h"

namespace award_system
{

class game_state_accumulator;

class reward_event_handler
{
public:
	reward_event_handler	(game_state_accumulator* pstate) :
		m_player_state_accum(pstate)
	{
	}
	~reward_event_handler	() {};
	virtual bool			OnWeapon_Fire				(u16 sender, u16 sender_weapon_id) = 0;
	virtual	bool			OnBullet_Fire				(u16 sender, u16 sender_weapon_id, const Fvector& position, const Fvector& direction) = 0;
	virtual bool			OnBullet_Hit				(CObject const * hitter, CObject const * victim, CObject const * weapon, u16 const bone) = 0;
	virtual bool			OnArtefactSpawned			() = 0;
	virtual bool			OnPlayerTakeArtefact		(game_PlayerState const * ps) = 0;
	virtual bool			OnPlayerDropArtefact		(game_PlayerState const * ps) = 0;
	virtual bool			OnPlayerBringArtefact		(game_PlayerState const * ps) = 0;
	virtual bool			OnPlayerSpawned				(game_PlayerState const * ps) = 0;
	virtual bool			OnPlayerKilled				(u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type) = 0;
	virtual bool			OnPlayerChangeTeam			(s8 team) = 0;
	virtual bool			OnRoundEnd					() = 0;
	virtual bool			OnRoundStart				() = 0;
	virtual	bool			OnPlayerRankChanged			() = 0;
protected:
	game_state_accumulator*		m_player_state_accum;
}; //class reward_event_handler

} //namespace award_system

#endif //#ifndef REWARD_EVENT_HANDLER