#ifndef GAME_STATE_ACCUMULATOR_INCLUDED
#define GAME_STATE_ACCUMULATOR_INCLUDED

#include "game_cl_base_weapon_usage_statistic.h"
#include "state_arguments_functions.h"
#include "player_state_param.h"
#include "game_events_handler.h"
#include "bone_groups.h"
#include "ammunition_groups.h"
#include "hits_store.h"
#include "kills_store.h"
#include "accumulative_states.h"

class CWeapon;

namespace award_system
{
#ifdef DEBUG
constexpr pcstr player_values_strtable[] = {
    "command_switch_count", "kill_in_raw_count", "death_count", "spots", "my_team_win_score", "my_team_win_score_now",
    "enemy_top_player_div", "blitzkrieg_time", "enemy_team_score", "enemy_team_score_now", "artdeliver_counter",
    "multi_champion", "mad", "achilles_heel_ready", "faster_than_bullets_time", "harvest_count", "skewer_count",
    "double_shot_double_kill_time", "ammo_elapsed", "climber", "opener_ready", "invincible_fury", "move_state",
    "move_velocity", "move_angular_velocity", "sprinter_victim_velocity", "marksman_count", "ambassador", "remembrance",
    "avenger", "cherub_ready", "stalker_flair", "black_list", "thunder_count", "killer_victim_angle_cos",
};
#endif //#ifdef DEBUG

class game_state_accumulator : public game_events_handler
{
public:
    game_state_accumulator();
    virtual ~game_state_accumulator();

    void update();
    void init();
    void init_player(game_PlayerState* local_player);
    void init_bone_groups(CActor* first_spawned_actor);

    u16 get_object_id(u16 item_object_id);
    u16 get_object_id(IGameObject const* obj);
    bool is_item_in_group(u16 item_id, ammunition_group::enum_group_id gid);
    bool is_bone_in_group(u16 bone_id, bone_group::enum_group_id gid);
    u16 get_armor_of_player(game_PlayerState* player);
    u16 get_active_weapon_of_player(game_PlayerState* player);
    CWeapon* get_active_weapon(game_PlayerState* player);
    CActor* get_players_actor(u16 game_id);
    game_PlayerState* get_local_player() const { return m_local_player; };
    hits_store& get_hits_store() { return m_hits; };
    kills_store& get_kills_store() { return m_kills; };
    bool is_enemies(u16 left_pid, u16 right_pid) const;
    bool is_enemies(game_PlayerState const* left_player, game_PlayerState const* right_player) const;

    void reset_player_game();

    virtual void OnWeapon_Fire(u16 sender, u16 sender_weapon_id);
    virtual void OnBullet_Fire(u16 sender, u16 sender_weapon_id, const Fvector& position, const Fvector& direction);
    virtual void OnBullet_Hit(
        IGameObject const* hitter, IGameObject const* victim, IGameObject const* weapon, u16 const bone);
    virtual void OnArtefactSpawned();
    virtual void OnPlayerTakeArtefact(game_PlayerState const* ps);
    virtual void OnPlayerDropArtefact(game_PlayerState const* ps);
    virtual void OnPlayerBringArtefact(game_PlayerState const* ps);
    virtual void OnPlayerSpawned(game_PlayerState const* ps);
    virtual void OnPlayerKilled(
        u16 killer_id, u16 target_id, u16 weapon_id, std::pair<KILL_TYPE, SPECIAL_KILL_TYPE> kill_type);
    virtual void OnPlayerChangeTeam(s8 team);
    virtual void OnPlayerRankChanged();
    virtual void OnRoundEnd();
    virtual void OnRoundStart();

    bool check_hit_params(u32 count, ammunition_group::enum_group_id weapon_group_id,
        bone_group::enum_group_id bone_group_id, float_binary_function* func, float right_dist_arg);

    bool check_kill_params(u32 count, ammunition_group::enum_group_id weapon_group_id, KILL_TYPE kill_type,
        SPECIAL_KILL_TYPE special_kill_type, u32 time_period);

    bool check_accumulative_value(
        enum_accumulative_player_values param_id, float_binary_function* func, float right_arg);

    bool check_accumulative_value(enum_accumulative_player_values param_id, u32_binary_function* func, u32 right_arg);

    using accumulative_values_collection_t = AssociativeVector<enum_accumulative_player_values, player_state_param*>;
private:
    // average_values_collection_t			m_average_values;
    accumulative_values_collection_t m_accumulative_values;
    CItemMgr const* m_item_mngr;
    game_PlayerState* m_local_player;
    u32 m_last_player_spawn_time;

    // void	init_average_values				();
    void init_accumulative_values();
    void init_player_accum_values(game_PlayerState* new_local_player);

    void update_average_values();
    void update_accumulative_values();

    hits_store m_hits;
    kills_store m_kills;

    ammunition_group m_amm_groups;
    bone_group m_bone_groups;
}; // class game_state_accumulator
} // namespace award_system

#endif //#ifndef GAME_STATE_ACCUMULATOR_INCLUDED
