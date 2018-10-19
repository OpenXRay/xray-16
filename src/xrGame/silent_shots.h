#ifndef SILENT_SHOTS_INCLUDED
#define SILENT_SHOTS_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class silent_shots : public player_state_param
{
    typedef player_state_param inherited;

public:
    silent_shots(game_state_accumulator* owner);
    virtual ~silent_shots(){};

    virtual void update(){};
    virtual u32 const get_u32_param() { return m_thunder_count; };
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnWeapon_Fire(u16 sender, u16 sender_weapon_id);

protected:
    u32 m_thunder_count;
    u16 m_last_shoot_weapon;
}; // class silent_shots

} // namespace award_system

#endif //#ifndef SILENT_SHOTS_INCLUDED
