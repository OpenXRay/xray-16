#ifndef INVINCIBLE_FURY_INCLUDED
#define INVINCIBLE_FURY_INCLUDED

#include "obsolete_queue.h"
#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_state_invincible_fury : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_invincible_fury(game_state_accumulator* owner);
    virtual ~player_state_invincible_fury(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

private:
    static u32 const max_fury_time = 6000;
    u32 m_last_kills;
}; // class player_state_invincible_fury

} // namespace award_system

#endif //#ifndef INVINCIBLE_FURY_INCLUDED
