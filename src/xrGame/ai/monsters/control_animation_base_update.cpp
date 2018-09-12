#include "StdAfx.h"
#include "control_animation_base.h"
#include "control_direction_base.h"
#include "control_movement_base.h"
#include "basemonster/base_monster.h"
#include "CharacterPhysicsSupport.h"
#include "PHMovementControl.h"
#include "detail_path_manager.h"
#include "monster_velocity_space.h"
#include "ai/monsters/control_path_builder_base.h"

// DEBUG purpose only
constexpr pcstr dbg_anim_name_table[] = {"eAnimStandIdle", "eAnimStandTurnLeft", "eAnimStandTurnRight",

    "eAnimSitIdle", "eAnimLieIdle",

    "eAnimSitToSleep", "eAnimLieToSleep", "eAnimStandSitDown", "eAnimStandLieDown", "eAnimLieStandUp",
    "eAnimSitStandUp", "eAnimStandLieDownEat", "eAnimSitLieDown", "eAnimLieSitUp", "eAnimSleepStandUp",

    "eAnimWalkFwd", "eAnimWalkBkwd", "eAnimWalkTurnLeft", "eAnimWalkTurnRight",

    "eAnimRun", "eAnimRunTurnLeft", "eAnimRunTurnRight", "eAnimFastTurn",

    "eAnimAttack", "eAnimAttackFromBack", "eAnimAttackRun",

    "eAnimEat", "eAnimSleep", "eAnimDie",

    "eAnimDragCorpse", "eAnimCheckCorpse", "eAnimScared", "eAnimAttackJump",

    "eAnimLookAround",

    "eAnimJump", "eAnimSteal",

    "eAnimJumpStart", "eAnimJumpGlide", "eAnimJumpFinish",

    "eAnimJumpLeft", "eAnimJumpRight",

    "eAnimStandDamaged", "eAnimWalkDamaged", "eAnimRunDamaged",

    "eAnimSniff", "eAnimHowling", "eAnimThreaten",

    "eAnimMiscAction_00", "eAnimMiscAction_01",

    "eAnimUpperStandIdle", "eAnimUpperStandTurnLeft", "eAnimUpperStandTurnRight",

    "eAnimStandToUpperStand", "eAnimUppperStandToStand",

    "eAnimUpperWalkFwd", "eAnimUpperThreaten", "eAnimUpperAttack",

    "eAnimAttackPsi",

    "eAnimTeleRaise", "eAnimTeleFire", "eAnimGraviPrepare", "eAnimGraviFire",

    "eAnimCount", "eAnimUndefined"};

//////////////////////////////////////////////////////////////////////////
// m_tAction processing
//////////////////////////////////////////////////////////////////////////

void CControlAnimationBase::update_frame()
{
    update();

    // raise event on velocity bounce
    CheckVelocityBounce();
}

void CControlAnimationBase::update()
{
    if (m_state_attack)
        return;

    // Установка Yaw
    if (m_object->control().path_builder().is_moving_on_path() && m_object->path().enabled())
        m_object->dir().use_path_direction(((spec_params & ASP_MOVE_BKWD) == ASP_MOVE_BKWD));

    SelectAnimation();
    SelectVelocities();

    // применить
    if (prev_motion != cur_anim_info().get_motion())
    {
        prev_motion = cur_anim_info().get_motion();
        select_animation();
    }
}

void CControlAnimationBase::clear_override_animation()
{
    m_override_animation = eAnimUndefined;
    m_override_animation_index = (u32)-1;
}

void CControlAnimationBase::set_override_animation(EMotionAnim anim, u32 index)
{
    if (m_override_animation == anim)
        return;

    if (anim != eAnimUndefined)
    {
        VERIFY2(m_override_animation == eAnimUndefined, "animation already overriden, call clear_override_animation");
    }

    m_override_animation = anim;
    m_override_animation_index = index;
}

void CControlAnimationBase::set_override_animation(pcstr name)
{
    for (u32 anim_type = 0; anim_type < m_anim_storage.size(); ++anim_type)
    {
        SAnimItem const* const anim_item = m_anim_storage[anim_type];

        if (!anim_item)
            continue;

        pcstr anim_name = anim_item->target_name.c_str();
        if (strstr(name, anim_name ? anim_name : "") == name)
        {
            pcstr const anim_index_string = name + anim_item->target_name.size();

            u32 anim_index = 0;
            sscanf(anim_index_string, "%d", &anim_index);
            set_override_animation((EMotionAnim)anim_type, anim_index);

            return;
        }
    }

    NODEFAULT;
}

//////////////////////////////////////////////////////////////////////////
// SelectAnimation
// In:	path, target_yaw, m_tAction
// Out:	установить анимацию в cur_anim_info().motion
void CControlAnimationBase::SelectAnimation()
{
    // Lain: added
    if (m_override_animation != eAnimUndefined)
    {
        SetCurAnim(m_override_animation);
        return;
    }

    EAction action = m_tAction;

    if (m_object->control().path_builder().is_moving_on_path() && m_object->path().enabled())
    {
        action = GetActionFromPath();
    }

    cur_anim_info().set_motion(m_tMotions[action].anim);

    m_object->CheckSpecParams(spec_params);

    if (prev_motion != cur_anim_info().get_motion())
        if (CheckTransition(prev_motion, cur_anim_info().get_motion()))
            return;

    CheckReplacedAnim();
    SetTurnAnimation();
}

#define MOVE_TURN_ANGLE deg(30)

void CControlAnimationBase::SetTurnAnimation()
{
    float yaw_current, yaw_target;
    m_man->direction().get_heading(yaw_current, yaw_target);
    float delta_yaw = angle_difference(yaw_target, yaw_current);

    bool turn_left = true;
    if (from_right(yaw_target, yaw_current))
        turn_left = false;

    EPState anim_state = GetState(cur_anim_info().get_motion());
    if (IsStandCurAnim() && (anim_state == PS_STAND) && (!fis_zero(delta_yaw)))
    {
        m_object->SetTurnAnimation(turn_left);
        return;
    }

    if (m_object->control().path_builder().is_moving_on_path() && m_object->path().enabled() &&
        (delta_yaw > MOVE_TURN_ANGLE))
    {
        m_object->SetTurnAnimation(turn_left);
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
// SelectVelocities
// In:	path, target_yaw, анимация
// Out:	установить linear и angular velocities,
//		по скорости движения выбрать финальную анимацию из Velocity_Chain
//		установить скорость анимации в соответствие с физ скоростью
void CControlAnimationBase::SelectVelocities()
{
    // получить скорости движения по пути
    bool b_moving = m_object->control().path_builder().is_moving_on_path();
    SMotionVel path_vel;
    path_vel.set(0.f, 0.f);
    SMotionVel anim_vel;
    anim_vel.set(0.f, 0.f);

    if (b_moving)
    {
        u32 cur_point_velocity_index =
            m_object->movement().detail().path()[m_object->movement().detail().curr_travel_point_index()].velocity;

        u32 next_point_velocity_index = u32(-1);
        if (m_object->movement().detail().path().size() > m_object->movement().detail().curr_travel_point_index() + 1)
            next_point_velocity_index = m_object->movement()
                                            .detail()
                                            .path()[m_object->movement().detail().curr_travel_point_index() + 1]
                                            .velocity;

        // если сейчас стоит на месте и есть след точка (т.е. должен быть в движении),
        // то реализовать поворот на месте, а дальше форсировать скорость со следующей точки
        if ((cur_point_velocity_index == MonsterMovement::eVelocityParameterStand) &&
            (next_point_velocity_index != u32(-1)))
        {
            if (!m_object->control().direction().is_turning())
                cur_point_velocity_index = next_point_velocity_index;
        }

        const CDetailPathManager::STravelParams& current_velocity =
            m_object->movement().detail().velocity(cur_point_velocity_index);
        path_vel.set(_abs(current_velocity.linear_velocity), current_velocity.real_angular_velocity);
    }

    SAnimItem* item_it = m_anim_storage[cur_anim_info().get_motion()];
    VERIFY(item_it);

    // получить скорости движения по анимации
    anim_vel.set(item_it->velocity.velocity.linear, item_it->velocity.velocity.angular_real);

    //	// проверить на совпадение
    //	R_ASSERT(fsimilar(path_vel.linear,	anim_vel.linear));
    //	R_ASSERT(fsimilar(path_vel.angular,	anim_vel.angular));

    // установка линейной скорости
    if (m_object->state_invisible)
    {
        // если невидимый, то установить скорость из пути
        m_object->move().set_velocity(_abs(path_vel.linear));
    }
    else
    {
        if (fis_zero(_abs(anim_vel.linear)))
            stop_now();
        else
        {
            // - проверить на возможность торможения
            if (!accel_check_braking(-2.f, _abs(anim_vel.linear)))
            {
                m_object->move().set_velocity(_abs(anim_vel.linear));
                // no braking mode
            }
            else
            {
                m_object->move().stop_accel();
                // braking mode
            }
        }
    }

    // финальная корректировка скорости анимации по физической скорости

    if (!m_object->state_invisible && !fis_zero(anim_vel.linear))
    {
        EMotionAnim new_anim;
        float a_speed;

        if (accel_chain_get(m_man->movement().real_velocity(), cur_anim_info().get_motion(), new_anim, a_speed))
        {
            cur_anim_info().set_motion(new_anim);

            if (a_speed < 0.5f)
                a_speed += 0.5f;

            cur_anim_info().speed._set_target(a_speed);
        }
        else
            cur_anim_info().speed._set_target(-1.f);
    }
    else
        cur_anim_info().speed._set_target(-1.f);

    set_animation_speed();

    // установка угловой скорости
    if (m_object->state_invisible)
        m_object->dir().set_heading_speed(path_vel.angular);
    else
    {
        item_it = m_anim_storage[cur_anim_info().get_motion()];
        VERIFY(item_it);

        // Melee?
        if (m_tAction == ACT_ATTACK)
        {
            float vel = item_it->velocity.velocity.angular_real;
            m_object->dir().set_heading_speed(
                vel * m_object->m_melee_rotation_factor); // todo: make as an external factor
        }
        else
            m_object->dir().set_heading_speed(item_it->velocity.velocity.angular_real);
    }
}

#define VELOCITY_BOUNCE_THRESHOLD 1.5f

void CControlAnimationBase::CheckVelocityBounce()
{
    Fvector temp_vec;
    m_object->character_physics_support()->movement()->GetCharacterVelocity(temp_vec);
    float prev_speed = m_prev_character_velocity;
    float cur_speed = temp_vec.magnitude();

    // prepare
    if (fis_zero(prev_speed))
        prev_speed = 0.01f;
    if (fis_zero(cur_speed))
        cur_speed = 0.01f;

    float ratio = ((prev_speed > cur_speed) ? (prev_speed / cur_speed) : (cur_speed / prev_speed));

    if (ratio > VELOCITY_BOUNCE_THRESHOLD)
    {
        if (prev_speed > cur_speed)
            ratio = -ratio;

        // prepare event
        SEventVelocityBounce event(ratio);
        m_man->notify(ControlCom::eventVelocityBounce, &event);
    }
    m_prev_character_velocity = cur_speed;
}

void CControlAnimationBase::ScheduledInit()
{
    spec_params = 0;
    accel_deactivate();
}
