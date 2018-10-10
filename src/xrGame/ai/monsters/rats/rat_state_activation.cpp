#include "pch_script.h"
#include "ai/monsters/rats/ai_rat.h"
#include "ai/ai_monsters_misc.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "xrAICore/Navigation/game_graph.h"
#include "ai/monsters/rats/ai_rat_space.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "detail_path_manager.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "item_manager.h"
#include "memory_space.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "movement_manager.h"
#include "sound_player.h"
#include "ai/monsters/rats/ai_rat_impl.h"
#include "ai_space.h"

using namespace RatSpace;

void CAI_Rat::activate_state_free_active()
{
    m_tSpawnPosition.set(m_home_position);
    m_fGoalChangeDelta = m_fSafeGoalChangeDelta;
    m_tVarGoal.set(m_tGoalVariation);
    m_fASpeed = m_fAngleSpeed;

    if (bfCheckIfGoalChanged())
    {
        if ((Position().distance_to(m_tSpawnPosition) > m_fStableDistance) ||
            (::Random.randF(0, 1) > m_fChangeActiveStateProbability))
            if (Position().distance_to(m_home_position) > m_fMaxHomeRadius)
                m_fSpeed = m_fSafeSpeed = m_fMaxSpeed;
            else
                vfChooseNewSpeed();
        else
        {
            if (can_stand_here())
                vfRemoveActiveMember();
        }
    }

    if ((fis_zero(m_fSpeed) &&
            (angle_difference(movement().m_body.target.yaw, movement().m_body.current.yaw) < PI_DIV_6)))
        vfChooseNewSpeed();

    vfUpdateTime(m_fTimeUpdateDelta);

    set_movement_type(false);

    sound().play(eRatSoundVoice, 45 * 1000, 15 * 1000);
}

void CAI_Rat::activate_state_free_passive()
{
    if (memory().enemy().selected())
    {
        m_fGoalChangeTime = 0;
        add_active_member(true);
        return;
    }

    if (m_fMorale < m_fMoraleNormalValue)
    {
        add_active_member(true);
        return;
    }

    if ((m_tLastSound.dwTime >= m_dwLastUpdateTime) &&
        ((!m_tLastSound.tpEntity) || (m_tLastSound.tpEntity->g_Team() != g_Team())))
    {
        add_active_member(true);
        return;
    }

    m_fSpeed = 0.f;

    vfAddStandingMember();
    add_active_member();

    sound().play(eRatSoundVoice, 45 * 1000, 15 * 1000);
}

void CAI_Rat::activate_turn()
{
    float m_heading, m_pitch;
    Fvector m_dest_direction;
    Fvector m_enemy_position = get_enemy()->Position();
    Fvector m_temp = Position();
    m_dest_direction.x = (m_enemy_position.x - m_temp.x) / m_temp.distance_to(m_enemy_position);
    m_dest_direction.y = (m_enemy_position.y - m_temp.y) / m_temp.distance_to(m_enemy_position);
    m_dest_direction.z = (m_enemy_position.z - m_temp.z) / m_temp.distance_to(m_enemy_position);
    m_dest_direction.getHP(m_heading, m_pitch);
    set_pitch(m_pitch, m_heading);
    m_tGoalDir = m_enemy_position;
}

void CAI_Rat::activate_state_move()
{
    vfUpdateTime(m_fTimeUpdateDelta);

    m_fSpeed = m_fAttackSpeed;

    set_movement_type(true, true);
}

void CAI_Rat::activate_move()
{
    vfUpdateTime(m_fTimeUpdateDelta);
    m_fSpeed = m_fSafeSpeed = m_fMaxSpeed;
    set_movement_type(m_bWayCanAdjustSpeed, m_bWayStraightForward);
}

void CAI_Rat::activate_state_attack_range()
{
    if (!m_attack_rebuild)
    {
        time_attack_rebuild = Device.dwTimeGlobal;
        m_attack_rebuild = true;
    }

    if (m_attack_rebuild && Device.dwTimeGlobal - time_attack_rebuild > 5000)
    {
        m_attack_rebuild = false;
    }

    m_fSpeed = 0.f;

    fire(true);

    set_movement_type(false);
}

void CAI_Rat::activate_state_free_recoil()
{
    m_fSpeed = m_fSafeSpeed = m_fMaxSpeed;

    set_movement_type(true, true);

    sound().play(eRatSoundVoice, 45 * 1000, 15 * 1000);
}
void CAI_Rat::activate_state_home()
{
    m_tSpawnPosition.set(m_home_position);
    m_fGoalChangeDelta = m_fSafeGoalChangeDelta;
    m_tVarGoal.set(m_tGoalVariation);
    m_fASpeed = m_fAngleSpeed;
    m_fSpeed = m_fSafeSpeed = m_fAttackSpeed;
    vfUpdateTime(m_fTimeUpdateDelta);

    m_fSpeed = m_fAttackSpeed;

    set_movement_type();
}

void CAI_Rat::activate_state_eat()
{
    Fvector temp_position;
    memory().item().selected()->Center(temp_position);

    if ((Device.dwTimeGlobal - m_previous_query_time > TIME_TO_GO) || !m_previous_query_time)
        m_tGoalDir.set(temp_position);

    vfUpdateTime(m_fTimeUpdateDelta);

    bool a = temp_position.distance_to(Position()) <= m_fAttackDistance;
    Fvector direction;
    direction.sub(temp_position, Position());
    float y, p;
    direction.getHP(y, p);
    if (a && angle_difference(y, -movement().m_body.current.yaw) < PI_DIV_6)
    {
        m_fSpeed = 0;
        if (Device.dwTimeGlobal - m_previous_query_time > m_dwHitInterval)
        {
            m_previous_query_time = Device.dwTimeGlobal;
            const CEntityAlive* const_corpse = smart_cast<const CEntityAlive*>(memory().item().selected());
            VERIFY(const_corpse);
            CEntityAlive* corpse = const_cast<CEntityAlive*>(const_corpse);
            VERIFY(corpse);
            corpse->m_fFood -= m_fHitPower / 10.f;
        }
        m_bFiring = true;
        set_movement_type(false);
        sound().play(eRatSoundEat);
    }
    else
    {
        sound().remove_active_sounds(u32(-1));
        if (!a)
            m_fSpeed = m_fMaxSpeed;
        else
            m_fSpeed = 0.f;
        set_movement_type(true, true);
    }
}
