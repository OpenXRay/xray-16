#ifndef PLAYER_STATE_SKEWER_INCLUDED
#define PLAYER_STATE_SKEWER_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_state_skewer : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_skewer(game_state_accumulator* owner);
    virtual ~player_state_skewer(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnWeapon_Fire(u16 sender, u16 sender_weapon_id);
    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

protected:
    u32 m_shot;
    u32 m_kills_count;
}; // class player_state_skewer

} // namespace award_system

#endif //#ifndef PLAYER_STATE_SKEWER_INCLUDED
