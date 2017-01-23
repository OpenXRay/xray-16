#ifndef REWARDING_EVENTS_INCLUDED
#define REWARDING_EVENTS_INCLUDED

#include "event_conditions_collection.h"

namespace award_system
{

class rewarding_state_events : public event_conditions_collection
{
	typedef	event_conditions_collection inherited;
public:
					rewarding_state_events	(game_state_accumulator* pstate_accum,
											 event_action_delegate_t ea_delegate);
	virtual			~rewarding_state_events	();
	virtual void	init					();
}; //class rewarding_state_events

} //namespace award_system

#endif //#ifndef REWARDING_EVENTS_INCLUDED