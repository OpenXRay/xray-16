#ifndef GAME_EVENTS_HANDLER_INCLUDED
#define GAME_EVENTS_HANDLER_INCLUDED

#include "game_cl_base_weapon_usage_statistic.h"

namespace award_system
{

class game_events_handler
{
public:
						game_events_handler			() {};
	virtual				~game_events_handler		() {};

	virtual void		OnWeapon_Fire				(u16 sender, u16 sender_weapon_id) = 0;
	virtual	void		OnBullet_Fire				(u16 sender, u16 sender_weapon_id, const Fvector& position, const Fvector& direction) = 0;
	virtual void		OnBullet_Hit				(CObject const * hitter, CObject const * victim, CObject const * weapon, u16 const bone) = 0;
	virtual void		OnArtefactSpawned			() = 0;
	virtual void		OnPlayerTakeArtefact		(game_PlayerState const * ps) = 0;
	virtual void		OnPlayerDropArtefact		(game_PlayerState const * ps) = 0;
	virtual void		OnPlayerBringArtefact		(game_PlayerState const * ps) = 0;
	virtual void		OnPlayerSpawned				(game_PlayerState const * ps) = 0;
	virtual void		OnPlayerKilled				(u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type) = 0;
	virtual void		OnPlayerChangeTeam			(s8 team) = 0;
	virtual	void		OnPlayerRankChanged			() = 0;
	virtual void		OnRoundEnd					() = 0;
	virtual void		OnRoundStart				() = 0;
};//class game_events_handler

} //namespace award_system

#endif //#ifndef GAME_EVENTS_HANDLER_INCLUDED