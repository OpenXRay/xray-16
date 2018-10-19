#ifndef DEATH_COUNT_INCLUDED
#define DEATH_COUNT_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_death_counter : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_death_counter(game_state_accumulator* owner);
    virtual ~player_death_counter(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return -1.0f; };
    virtual void reset_game(){};
}; // class player_death_counter

class player_artdeliver_counter : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_artdeliver_counter(game_state_accumulator* owner);
    virtual ~player_artdeliver_counter(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game(){};
}; // class player_artdeliver_counter

class player_rawkill_counter : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_rawkill_counter(game_state_accumulator* owner);
    virtual ~player_rawkill_counter(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnPlayerSpawned(game_PlayerState const* ps);
    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);

protected:
    u32 m_raw_kills;
}; // class player_rawkill_counter

class player_state_move : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_move(game_state_accumulator* owner);
    virtual ~player_state_move(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game(){};
}; // class player_state_move

class player_state_velocity : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_velocity(game_state_accumulator* owner);
    virtual ~player_state_velocity(){};

    virtual void update(){};
    virtual u32 const get_u32_param() { return 0; }
    virtual float const get_float_param();
    virtual void reset_game(){};
}; // class player_state_velocity

class player_state_ang_velocity : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_ang_velocity(game_state_accumulator* owner);
    virtual ~player_state_ang_velocity(){};

    virtual void update(){};
    virtual u32 const get_u32_param() { return 0; }
    virtual float const get_float_param();
    virtual void reset_game(){};
}; // class player_state_ang_velocity

} // namespace award_system

#endif //#ifndef DEATH_COUNT_INCLUDED
