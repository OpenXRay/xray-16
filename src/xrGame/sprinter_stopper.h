#ifndef SPRINTER_STOPPER_INCLUDED
#define SPRINTER_STOPPER_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class spritnter_stopper : public player_state_param
{
    typedef player_state_param inherited;

public:
    spritnter_stopper(game_state_accumulator* owner);
    virtual ~spritnter_stopper(){};

    virtual void update(){};
    virtual u32 const get_u32_param() { return 0; };
    virtual float const get_float_param();
    virtual void reset_game();
    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

protected:
    float m_sprinter_victim_velocity;
}; // class spritnter_stopper

} // namespace award_system

#endif //#ifndef SPRINTER_STOPPER_INCLUDED
