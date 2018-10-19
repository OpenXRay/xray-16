#ifndef PLAYER_STATE_AMBASSADOR_INCLUDED
#define PLAYER_STATE_AMBASSADOR_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_state_ambassador : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_ambassador(game_state_accumulator* owner);
    virtual ~player_state_ambassador(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return -1.0f; };
    virtual void reset_game();

    virtual void OnWeapon_Fire(u16 sender, u16 sender_weapon_id);
    virtual void OnPlayerTakeArtefact(game_PlayerState const* ps);
    virtual void OnPlayerDropArtefact(game_PlayerState const* ps);
    virtual void OnPlayerBringArtefact(game_PlayerState const* ps);

protected:
    u32 m_shots_count;
    u32 m_art_drop_count;
    bool m_delivered;
}; // class player_state_ambassador

} // namespace award_system

#endif //#ifndef PLAYER_STATE_AMBASSADOR_INCLUDED
