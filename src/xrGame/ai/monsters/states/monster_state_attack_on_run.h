#ifndef MONSTER_STATE_ATTACK_ON_RUN_H
#define MONSTER_STATE_ATTACK_ON_RUN_H

#include "ai/monsters/state.h"

#include "ai/weighted_random.h"

template <typename _Object>
class CStateMonsterAttackOnRun : public CState<_Object>
{
    typedef CState<_Object> inherited;

public:
    CStateMonsterAttackOnRun(_Object* obj);

    virtual void initialize();
    virtual void execute();
    virtual void finalize();
    virtual void critical_finalize();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    virtual bool check_completion();
    virtual bool check_start_conditions();

private:
    enum phaze
    {
        go_close,
        go_far,
        go_prepare
    };
    phaze m_phaze;
    Fvector m_go_far_start_point;
    TTime m_phaze_chosen_time;

    enum aim_side
    {
        left = 0,
        right = 1
    }; // coupled
    aim_side m_attack_side;
    aim_side m_prepare_side;
    TTime m_prepare_side_chosen_time;
    TTime m_attack_side_chosen_time;

    bool m_can_do_rotation_jump;
    bool m_attacking;
    TTime m_attack_end_time;

    bool m_try_min_time;
    TTime m_try_min_time_period;
    TTime m_try_min_time_chosen_time;

    Fvector m_target;
    u32 m_target_vertex;

    u32 m_animation_index[3];
    float m_animation_hit_time[3];

    TTime m_last_update_time;
    TTime m_last_prediction_time;
    Fvector m_last_update_enemy_pos;
    Fvector m_predicted_enemy_velocity;
    Fvector m_predicted_enemy_pos;
    bool m_can_do_preparation;
    bool m_is_jumping;
    bool m_reach_old_target;
    TTime m_reach_old_target_start_time;
    CEntityAlive const* m_enemy_to_attack;

private:
    void update_try_min_time();
    void update_attack();
    void update_movement_target();
    void update_aim_side();
    void calculate_predicted_enemy_pos();
    void set_movement_phaze(phaze new_phaze);
    void choose_next_atack_animation();
    void select_prepare_fallback_target();
    virtual bool check_control_start_conditions(ControlCom::EControlType type);
};

inline bool is_valid_point_to_move(Fvector const& point);

#include "monster_state_attack_on_run_inline.h"

#endif // MONSTER_STATE_ATTACK_ON_RUN_H
