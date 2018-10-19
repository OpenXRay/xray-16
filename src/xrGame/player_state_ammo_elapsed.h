#ifndef PLAYER_STATE_AMMO_ELAPSED_INCLUDED
#define PLAYER_STATE_AMMO_ELAPSED_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_state_ammo_elapsed : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_ammo_elapsed(game_state_accumulator* owner);
    virtual ~player_state_ammo_elapsed(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game(){};
}; // class player_state_ammo_elapsed

} // namespace award_system

#endif //#ifndef PLAYER_STATE_AMMO_ELAPSED_INCLUDED
