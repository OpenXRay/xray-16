#ifndef BLACK_LIST_INCLUDED
#define BLACK_LIST_INCLUDED

#include "player_state_param.h"
#include "accumulative_states.h"

namespace award_system
{
class black_list : public player_state_param
{
    typedef player_state_param inherited;

public:
    black_list(game_state_accumulator* owner);
    ~black_list(){};

    virtual void update(){};
    virtual u32 const get_u32_param();
    virtual float const get_float_param() { return 0.0f; };
    virtual void reset_game();

    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);
    virtual void OnPlayerSpawned(game_PlayerState const* ps);

protected:
    // data of the vector is u32 - kill time
    AssociativeVector<shared_str, u32> m_victims;
}; // class black_list

} // namespace award_system

#endif //#ifndef BLACK_LIST_INCLUDED
