#ifndef PLAYER_STATE_CLIMBER_INCLUDED
#define PLAYER_STATE_CLIMBER_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_state_climber : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_climber(game_state_accumulator* owner);
    ~player_state_climber(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game() { m_player_is_climber = false; };
    virtual void OnPlayerRankChanged();

protected:
    bool m_player_is_climber;
}; // class player_state_climber

} // namespace award_system

#endif //#ifndef PLAYER_STATE_CLIMBER_INCLUDED
