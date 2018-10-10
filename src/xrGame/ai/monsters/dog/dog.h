#pragma once

#include "ai/monsters/basemonster/base_monster.h"
#include "ai/monsters/controlled_entity.h"

class CAI_Dog : public CBaseMonster, public CControlledEntity<CAI_Dog>
{
    typedef CBaseMonster inherited;
    typedef CControlledEntity<CAI_Dog> CControlled;

public:
    CAI_Dog();
    virtual ~CAI_Dog();

    virtual void Load(LPCSTR section);
    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual void UpdateCL();

    virtual void CheckSpecParams(u32 spec_params);
    virtual void HitEntityInJump(const CEntity* pEntity);

    virtual bool ability_can_drag() { return true; }
    virtual u32 get_attack_rebuild_time();
    virtual bool can_use_agressive_jump(const IGameObject*);
    pcstr get_monster_class_name() override { return "dog"; }

public:
    void set_current_animation(u32 curr_anim = -1);
    void start_animation();
    virtual bool check_start_conditions(ControlCom::EControlType type);
    void anim_end_reinit();
    bool get_custom_anim_state();
    void set_custom_anim_state(bool b_state_animation);
    u32 get_number_animation();
    u32 random_anim();
    bool is_night();

    bool b_end_state_eat;
    bool b_state_check;
    bool b_state_end;
    Fvector enemy_position;
    u32 saved_state;
    u32 m_anim_factor;
    u32 m_corpse_use_timeout;
    u32 m_min_life_time;
    u32 m_drive_out_time;
    u32 m_min_sleep_time;
    u32 m_start_smelling;
    u32 m_smelling_count;

private:
    bool b_state_anim;
    bool b_anim_end;
    u32 current_anim;
    u32 min_move_dist;
    u32 max_move_dist;

    LPCSTR get_current_animation();
    static void animation_end(CBlend* B);
#ifdef _DEBUG
    virtual void debug_on_key(int key);
#endif
};
