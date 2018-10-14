#ifndef PLAYER_STATE_AVENGER
#define PLAYER_STATE_AVENGER

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class player_state_avenger : public player_state_param
{
    typedef player_state_param inherited;

public:
    player_state_avenger(game_state_accumulator* owner);
    ~player_state_avenger(){};

    virtual void update(){};
    virtual u32 const get_u32_param() { return m_aveng_count; };
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);
    virtual void OnPlayerSpawned(game_PlayerState const* ps);

protected:
    void feel_my_team_players(game_PlayerState const* of_player, buffer_vector<shared_str>& dest);
    typedef AssociativeVector<shared_str, u32> player_spawn_times_t;
    player_spawn_times_t m_player_spawns;
    u32 m_aveng_count;
}; // class player_state_avenger

} // namespace award_system

#endif //#ifndef PLAYER_STATE_AVENGER
