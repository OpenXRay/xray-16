#ifndef PLAYER_STATE_MAD_INCLUDED
#define PLAYER_STATE_MAD_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_state_mad : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_mad(game_state_accumulator* owner);
    virtual ~player_state_mad(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();
    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

    static u32 const mad_time_period = 20000; // 20 seconds
protected:
    typedef xr_vector<u32> kill_times_t;

    kill_times_t m_kill_times;
    void clear_old_kills();
}; // class player_state_mad

} // namespace award_system

#endif //#ifndef PLAYER_STATE_MAD_INCLUDED
