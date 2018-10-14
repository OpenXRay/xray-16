#ifndef PLAYER_STATE_ACHILLES_HEEL_INCLUDED
#define PLAYER_STATE_ACHILLES_HEEL_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class achilles_heel_kill : public player_state_param
{
    typedef player_state_param inherited;

public:
    achilles_heel_kill(game_state_accumulator* owner);
    virtual ~achilles_heel_kill(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();
    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

protected:
    bool m_achilles_kill_was;
}; // class achilles_heel_kill

} // namespace award_system

#endif //#ifndef PLAYER_STATE_ACHILLES_HEEL_INCLUDED
