#ifndef PLAYER_SPOT_PARAMS_INCLUDED
#define PLAYER_SPOT_PARAMS_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
extern u32 const calculate_spots(game_PlayerState* ps);

class player_spots_counter : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_spots_counter(game_state_accumulator* owner);
    virtual ~player_spots_counter(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game(){};
}; // class player_spots_counter

class player_spots_with_top_enemy_divider : public player_spots_counter
{
    typedef player_spots_counter inherited;

public:
    player_spots_with_top_enemy_divider(game_state_accumulator* owner) : inherited(owner){};
    virtual ~player_spots_with_top_enemy_divider(){};
    virtual u32 const get_u32_param() { return 0; };
    virtual float const get_float_param();

private:
    u32 const get_top_enemy_player_score();
}; // player_spots_with_top_enemy_divider

} // namespace award_system

#endif //#ifndef PLAYER_SPOT_PARAMS_INCLUDED
