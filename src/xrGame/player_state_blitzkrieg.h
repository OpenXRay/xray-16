#ifndef PLAYER_STATE_BLITZKRIEG_INCLUDED
#define PLAYER_STATE_BLITZKRIEG_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_blitzkrieg : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_blitzkrieg(game_state_accumulator* owner) : inherited(owner) {};
    virtual ~player_blitzkrieg(){};

    virtual void update(){};
    virtual u32 get_u32_param();
    virtual float get_float_param() { return -1.0f; };
    virtual void reset_game();

    virtual void OnPlayerTakeArtefact(game_PlayerState const* ps);
    virtual void OnPlayerDropArtefact(game_PlayerState const* ps);
    virtual void OnPlayerBringArtefact(game_PlayerState const* ps);

protected:
    u32 m_deliver_time{ u32(-1) };
    u32 m_take_time{};
    u32 m_art_drop_count{};
}; // class player_blitzkrieg

} // namespace award_system

#endif //#ifndef PLAYER_STATE_BLITZKRIEG_INCLUDED
