#ifndef KILLER_VICTIM_VELOCITY_ANGLE_INCLUDED
#define KILLER_VICTIM_VELOCITY_ANGLE_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class killer_victim_angle : public player_state_param
{
    typedef player_state_param inherited;

public:
    killer_victim_angle(game_state_accumulator* owner);
    virtual ~killer_victim_angle(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return m_killer_victim_angle_cos; };
    virtual void reset_game();
    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

protected:
    float m_killer_victim_angle_cos;
}; // class killer_victim_angle

} // namespace award_system

#endif //#ifndef KILLER_VICTIM_VELOCITY_ANGLE_INCLUDED
