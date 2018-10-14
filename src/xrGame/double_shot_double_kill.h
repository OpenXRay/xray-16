#ifndef DOUBLE_SHOT_DOUBLE_KILL_INCLUDED
#define DOUBLE_SHOT_DOUBLE_KILL_INCLUDED

#include "player_state_param.h"
#include "obsolete_queue.h"
#include "accumulative_states.h"

namespace award_system
{
class double_shot_double_kill : public player_state_param
{
    typedef player_state_param inherited;

public:
    double_shot_double_kill(game_state_accumulator* owner);
    virtual ~double_shot_double_kill(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnWeapon_Fire(u16 sender, u16 sender_weapon_id);
    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

protected:
    struct kill_shot_id
    {
        u32 m_shot_number;
        u32 m_shot_time;
    };

    typedef obsolete_queue<buffer_vector<kill_shot_id>, 2> kills_times_t;

    kills_times_t m_kills;
    u32 m_shot_count;
}; // class double_shot_double_kill

} // namespace award_system

#endif //#ifndef DOUBLE_SHOT_DOUBLE_KILL_INCLUDED
