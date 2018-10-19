#ifndef PLAYER_STATE_CHERUB_INCLUDED
#define PLAYER_STATE_CHERUB_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_state_cherub : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_cherub(game_state_accumulator* owner);
    virtual ~player_state_cherub(){};

    virtual void update(){};
    virtual u32 const get_u32_param() { return m_kill_count; };
    virtual float const get_float_param() { return -1.0f; };
    virtual void reset_game();

    virtual void OnPlayerTakeArtefact(game_PlayerState const* ps);
    virtual void OnPlayerDropArtefact(game_PlayerState const* ps);
    virtual void OnPlayerBringArtefact(game_PlayerState const* ps);

    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);
    virtual void OnPlayerSpawned(game_PlayerState const* ps);

protected:
    u32 m_kill_count;
    u32 m_art_take_time;
    u16 m_bearer_id;
    shared_str m_bearer_name;
}; // class player_state_cherub

}

#endif //#ifndef PLAYER_STATE_CHERUB_INCLUDED
