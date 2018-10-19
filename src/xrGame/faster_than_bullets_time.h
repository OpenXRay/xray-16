#ifndef FASTER_THAN_BULLETS_INCLUDED
#define FASTER_THAN_BULLETS_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class faster_than_bullets_time : public player_state_param
{
    typedef player_state_param inherited;

public:
    faster_than_bullets_time(game_state_accumulator* owner);
    virtual ~faster_than_bullets_time(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

protected:
    u32 m_no_demag_time;
}; // class faster_than_bullets_time

} // namespace award_system

#endif //#ifndef FASTER_THAN_BULLETS_INCLUDED
