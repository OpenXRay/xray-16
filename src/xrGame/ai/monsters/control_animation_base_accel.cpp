#include "StdAfx.h"
#include "control_animation_base.h"
#include "basemonster/base_monster.h"
#include "detail_path_manager.h"
#include "monster_velocity_space.h"

void CControlAnimationBase::accel_init() { m_accel.active = false; }
void CControlAnimationBase::accel_load(LPCSTR section)
{
    m_accel.calm = pSettings->r_float(section, "Accel_Calm");
    m_accel.aggressive = pSettings->r_float(section, "Accel_Aggressive");
}

void CControlAnimationBase::accel_activate(EAccelType type)
{
    m_accel.active = true;
    m_accel.type = type;

    m_accel.enable_braking = true;
}

float CControlAnimationBase::accel_get(EAccelValue val)
{
    if (!accel_active(val))
        return flt_max;

    switch (m_accel.type)
    {
    case eAT_Calm: return m_accel.calm;
    case eAT_Aggressive: return m_accel.aggressive;
    default: return m_accel.calm;
    }
}

// -----------------------------------------------------------------------------------------

void CControlAnimationBase::accel_chain_add(EMotionAnim anim1, EMotionAnim anim2)
{
    SEQ_VECTOR v_temp;
    v_temp.push_back(anim1);
    v_temp.push_back(anim2);

    m_accel.chain.push_back(v_temp);
}

bool CControlAnimationBase::accel_chain_get(
    float cur_speed, EMotionAnim target_anim, EMotionAnim& new_anim, float& a_speed)
{
    VERIFY2(_abs(cur_speed) < 1000, "CControlAnimationBase cur_speed too big");

    auto B = m_accel.chain.begin();
    auto E = m_accel.chain.end();

    // пройти по всем Chain-векторам
    for (auto I = B; I != E; I++)
    {
        auto IT_B = I->begin();
        auto IT_E = I->end();
        auto best_anim = IT_E;
        SVelocityParam* best_param = 0;

        bool found = false;

        // Пройти по текущему вектору
        for (auto IT = IT_B; IT != IT_E; IT++)
        {
            SAnimItem* item_it = m_anim_storage[*IT];
            VERIFY(item_it);

            SVelocityParam* param = &item_it->velocity;
            float from = param->velocity.linear * param->min_factor;
            float to = param->velocity.linear * param->max_factor;

            if (((from <= cur_speed + EPS_L) && (cur_speed <= to + EPS_L)) ||
                ((cur_speed < from) && (IT == I->begin())) || ((cur_speed + EPS_L >= to) && (IT + 1 == I->end())))
            {
                best_anim = IT;
                best_param = &item_it->velocity;
            }

            if ((*IT) == target_anim)
                found = true;
            if (found && best_param)
                break;
        }

        if (!found)
            continue;

        R_ASSERT2(best_param, "probably incompatible speed ranges");
        // calc anim_speed
        new_anim = *best_anim;
        float tmp = GetAnimSpeed(new_anim);
        VERIFY2(_abs(tmp) < 1000, "CControlAnimationBase GetAnimSpeed returns too big speed");
        a_speed = tmp * cur_speed / best_param->velocity.linear;
        VERIFY2(_abs(a_speed) < 1000, "CControlAnimationBase a_speed too big");
        return true;
    }
    return false;
}

bool CControlAnimationBase::accel_chain_test()
{
    string256 error_msg;

    // пройти по всем Chain-векторам
    for (auto I = m_accel.chain.begin(); I != m_accel.chain.end(); I++)
    {
        VERIFY2(I->size() >= 2, error_msg);

        SAnimItem* anim_from = m_anim_storage[*(I->begin())];
        SAnimItem* anim_to;
        VERIFY(anim_from);

        // Пройти по текущему вектору
        for (auto IT = I->begin() + 1; IT != I->end(); IT++)
        {
            anim_to = m_anim_storage[*IT];

            float from = anim_from->velocity.velocity.linear * anim_from->velocity.max_factor;
            float to = anim_to->velocity.velocity.linear * anim_to->velocity.min_factor;

            xr_sprintf(error_msg, "Incompatible speed ranges. Monster[%s] From animation  [%s] To animation [%s]",
                *m_object->cName(), *anim_from->target_name, *anim_to->target_name);
            VERIFY2(to < from, error_msg);

            anim_from = anim_to;
        }
    }

    return true;
}

bool CControlAnimationBase::accel_check_braking(float before_interval, float nominal_speed)
{
    if (!m_man->path_builder().is_moving_on_path())
        return (braking_mode = false);
    if (!accel_active(eAV_Braking))
        return (braking_mode = false);

    float acceleration = accel_get(eAV_Braking);
    float braking_dist =
        (nominal_speed * ((braking_mode) ? nominal_speed : m_man->movement().velocity_current())) / (2 * acceleration);

    braking_dist += before_interval;
    if (m_man->path_builder().is_path_end(braking_dist))
        return (braking_mode = true);

    // проверить точки пути, где необходимо остановиться
    float dist = 0.f; // дистанция до найденной точки
    for (u32 i = m_man->path_builder().detail().curr_travel_point_index() + 1;
         i < m_man->path_builder().detail().path().size(); i++)
    {
        dist += m_man->path_builder().detail().path()[i].position.distance_to(
            m_man->path_builder().detail().path()[i - 1].position);

        if (m_man->path_builder().detail().path()[i].velocity == MonsterMovement::eVelocityParameterStand)
        {
            if (dist < braking_dist)
                return (braking_mode = true);
            else
                break;
        }
    }

    return (braking_mode = false);
}
