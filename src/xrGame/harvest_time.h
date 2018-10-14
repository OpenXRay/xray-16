#ifndef HARVEST_TIME_INCLUDED
#define HARVEST_TIME_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class harvest_time : public player_state_param
{
    typedef player_state_param inherited;

public:
    harvest_time(game_state_accumulator* owner);
    ~harvest_time(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);
    virtual void OnPlayerSpawned(game_PlayerState const* ps);

protected:
    u32 m_harvest_count;
    u32 m_spawn_time;
}; // class harvest_time

} // namespace award_system

#endif //#ifndef HARVEST_TIME_INCLUDED
