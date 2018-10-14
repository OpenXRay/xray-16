#ifndef STALKER_FLAIR_INCLUDED
#define STALKER_FLAIR_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class stalker_flair : public player_state_param
{
    typedef player_state_param inherited;

public:
    stalker_flair(game_state_accumulator* owner);
    virtual ~stalker_flair(){};

    virtual void update(){};

    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return -1.0f; };
    virtual void reset_game();

    virtual void OnArtefactSpawned();
    virtual void OnPlayerTakeArtefact(game_PlayerState const* ps);

protected:
    u32 m_art_spawn_time;
    u32 m_art_take_time;
}; // class stalker_flair

} // namespace award_system

#endif //#ifndef PLAYER_STATE_BLITZKRIEG_INCLUDED
