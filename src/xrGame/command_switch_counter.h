#ifndef COMMAND_SWITCH_COUNTER_INCLUDED
#define COMMAND_SWITCH_COUNTER_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class command_switch_counter : public player_state_param
{
    typedef player_state_param inherited;

public:
    command_switch_counter(game_state_accumulator* owner) : inherited(owner) { m_counter = 0; };
    virtual ~command_switch_counter(){};

    virtual void update(){};
    virtual u32 const get_u32_param() { return m_counter; };
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game() { m_counter = 0; };
    virtual void OnPlayerChangeTeam(s8 team) { ++m_counter; };
private:
    u32 m_counter;
}; // class command_switch_counter

} // namespace award_system

#endif //#ifndef COMMAND_SWITCH_COUNTER_INCLUDED
