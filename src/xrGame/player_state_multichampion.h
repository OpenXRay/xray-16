#ifndef PLAYER_STATE_MULTICHAMPION_INCLUDED
#define PLAYER_STATE_MULTICHAMPION_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{

class player_multichampion : public player_state_param
{
	typedef player_state_param inherited;
public:
						player_multichampion		(game_state_accumulator* owner);
	virtual				~player_multichampion		() {};

	virtual void		update						() {};
	virtual	u32 const	get_u32_param				();
	virtual float const get_float_param				() { return 0.0f; };
	virtual void		reset_game					();

	virtual void		OnRoundEnd					();
protected:
	bool				m_can_be_multichampion;
};//class player_multichampion

ADD_ACCUMULATIVE_STATE(acpv_multi_champion, player_multichampion);
#undef ACCUMULATIVE_STATE_LIST
#define ACCUMULATIVE_STATE_LIST SAVE_TYPE_LIST(acpv_multi_champion, player_multichampion)


} //namespace award_system

#endif //#ifndef PLAYER_STATE_MULTICHAMPION_INCLUDED