#pragma once

#include "control_combase.h"
#include "ai_monster_defs.h"

struct SEventVelocityBounce : public ControlCom::IEventData
{
    float m_ratio;

    SEventVelocityBounce(const float ratio) : m_ratio(ratio) {}
};

//////////////////////////////////////////////////////////////////////////
class CControlAnimationBase : public CControl_ComBase
{
    typedef CControl_ComBase inherited;

protected:
    // информация о текущей анимации
    SCurrentAnimationInfo m_cur_anim;

    // -------------------------------------------------------------------------------------
    // Acceleration
    struct
    {
        xr_vector<SEQ_VECTOR> chain;

        bool active;
        bool enable_braking; // не использовать при торможении

        EAccelType type;

        float calm;
        float aggressive;
    } m_accel;

    xr_vector<SAnimItem*> m_anim_storage;

    // анимации подмены
    xr_vector<SReplacedAnim> m_tReplacedAnims;

public:
    EAction m_tAction;

protected:
    EMotionAnim spec_anim;
    EMotionAnim prev_motion;

    EMotionAnim m_override_animation; // used if != eAnimUndefined
    u32 m_override_animation_index; // used if != -1

    // -------------------------------------------------------------------------
    u32 spec_params; // дополнительные параметры
    TTime fx_time_last_play;
    float m_prev_character_velocity;

    bool m_state_attack;
    bool braking_mode;
    // ---------------------------------------------------------------------------------------

    xr_map<EAction, SMotionItem> m_tMotions; // карта соответсвий EAction к SMotionItem
    xr_vector<STransition> m_tTransitions; // вектор переходов из одной анимации в другую
    xr_vector<SAAParam> m_attack_anims; // работа с анимациями атаки

protected:
    void init_anim_storage();
    void free_anim_storage();

public:
    CControlAnimationBase();
    virtual ~CControlAnimationBase();

    // Control_ComBase interface
    virtual void reinit();
    virtual void on_event(ControlCom::EEventType, ControlCom::IEventData*);
    virtual void on_start_control(ControlCom::EControlType type);
    virtual void on_stop_control(ControlCom::EControlType type);
    virtual void update_frame();

    void ScheduledInit();

    // создание карты анимаций (выполнять на Monster::Load)
    ICF bool AddAnim(const EMotionAnim ma, cpcstr tn, const int s_id, SVelocityParam* vel, const EPState p_s, const bool required = true)
    {
        return AddAnim(ma, tn, s_id, vel, p_s, {}, required);
    }

    bool AddAnim(EMotionAnim ma, pcstr tn, int s_id, SVelocityParam* vel, EPState p_s, const SAnimItem::Effects& fxs, bool required = true);

    // -------------------------------------

    // добавить анимацию перехода (A - Animation, S - Position)
    void AddTransition(EMotionAnim from, EMotionAnim to, EMotionAnim trans, bool chain, bool skip_aggressive = false);
    void AddTransition(EMotionAnim from, EPState to, EMotionAnim trans, bool chain, bool skip_aggressive = false);
    void AddTransition(EPState from, EMotionAnim to, EMotionAnim trans, bool chain, bool skip_aggressive = false);
    void AddTransition(EPState from, EPState to, EMotionAnim trans, bool chain, bool skip_aggressive = false);

    // -------------------------------------

    void LinkAction(EAction act, EMotionAnim pmt_motion, EMotionAnim pmt_left, EMotionAnim pmt_right, float pmt_angle);
    void LinkAction(EAction act, EMotionAnim pmt_motion);

    // -------------------------------------

    void AddReplacedAnim(bool* b_flag, EMotionAnim pmt_cur_anim, EMotionAnim pmt_new_anim);

    // -------------------------------------
    bool CheckTransition(EMotionAnim from, EMotionAnim to);

    void SetSpecParams(u32 param) { spec_params |= param; }
    void SetCurAnim(EMotionAnim a);

    EMotionAnim GetCurAnim() const { return cur_anim_info().get_motion(); }
    // работа с анимациями атак
    void AA_reload(LPCSTR section);
    SAAParam& AA_GetParams(LPCSTR anim_name);
    SAAParam& AA_GetParams(MotionID motion, float time_perc);

    // FX's
    void FX_Play(EHitSide side, float amount);

    MotionID get_motion_id(EMotionAnim a, u32 index = u32(-1)) const;

protected:
    void UpdateAnimCount();

    // работа с анимациями атак
    void AA_Clear();
    void AA_SwitchAnimation(EMotionAnim a, u32 i3);

    // дополнительные функции
    EPState GetState(EMotionAnim a);
    void CheckReplacedAnim();

    CMotionDef* get_motion_def(SAnimItem* it, u32 index) const;

public:
    float GetAnimSpeed(EMotionAnim anim);
    bool IsStandCurAnim();
    bool IsTurningCurAnim();
    void ValidateAnimation();

    //////////////////////////////////////////////////////////////////////////
protected:
    void update();

protected:
    void SelectAnimation();
    void SelectVelocities();

    EAction GetActionFromPath();
    EAction VelocityIndex2Action(u32 velocity_index);

    void stop_now();

protected:
    //////////////////////////////////////////////////////////////////////////
    // DEBUG

    [[nodiscard]]
    pcstr GetAnimationName(EMotionAnim anim) const;

    [[nodiscard]]
    static pcstr GetActionName(EAction action);

    // end DEBUG
    //////////////////////////////////////////////////////////////////////////

public:
    //-------------------------------------------------------------------------------
    // Acceleration

    void accel_init();
    void accel_load(pcstr section);

    void accel_activate(EAccelType type);

    void accel_deactivate()
    {
        m_accel.active = false;
        m_accel.enable_braking = false;
    }

    void accel_set_braking(bool val = true) { m_accel.enable_braking = val; }

    [[nodiscard]] float accel_get(EAccelValue val = eAV_Accel) const;

    [[nodiscard]] bool accel_active(const EAccelValue val = eAV_Accel) const
    {
        return (val == eAV_Accel) ? m_accel.active : m_accel.enable_braking;
    }

    void accel_chain_add(EMotionAnim anim1, EMotionAnim anim2);
    bool accel_chain_get(float cur_speed, EMotionAnim target_anim, EMotionAnim& new_anim, float& a_speed);
    bool accel_chain_test();

    bool accel_check_braking(float before_interval, float nominal_speed);

    // --------------------------------------------------------------------------------

    void CheckVelocityBounce();

    // Other
    void SetTurnAnimation();

public:
    [[nodiscard]]
    SCurrentAnimationInfo& cur_anim_info() { return m_cur_anim; }

    [[nodiscard]]
    const SCurrentAnimationInfo& cur_anim_info() const { return m_cur_anim; }

    void select_animation(bool anim_end = false);
    void set_animation_speed();

    void check_hit(MotionID motion, float time_perc);

    float get_animation_length(EMotionAnim anim, u32 index) const; // anim must exist
    bool get_animation_info(EMotionAnim anim, u32 index, MotionID& motion, float& length) const;
    float get_animation_hit_time(EMotionAnim anim, u32 index) const;
    u32 get_animation_variants_count(EMotionAnim anim) const;

    // you need to call it with default arguments to turn it off
    void set_override_animation(EMotionAnim anim = eAnimUndefined, u32 index = -1);
    void set_override_animation(pcstr name);
    void clear_override_animation();
    EMotionAnim get_override_animation() const { return m_override_animation; }
    bool has_override_animation() const { return get_override_animation() != eAnimUndefined; }
};
