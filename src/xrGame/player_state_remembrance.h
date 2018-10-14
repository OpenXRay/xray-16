#ifndef PLAYER_STATE_REMEMBRANCE
#define PLAYER_STATE_REMEMBRANCE

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_state_remembrance : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_remembrance(game_state_accumulator* owner);
    ~player_state_remembrance(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

protected:
    static int const max_kill_dist = 5;
    bool m_is_remembrance;
}; // class player_state_remembrance

} // namespace award_system

#endif //#ifndef PLAYER_STATE_REMEMBRANCE
