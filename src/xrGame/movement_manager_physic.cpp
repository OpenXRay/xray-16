////////////////////////////////////////////////////////////////////////////
//	Module 		: movement_manager_physic.cpp
//	Created 	: 03.12.2003
//  Modified 	: 03.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Movement manager : physic character movement
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "movement_manager.h"
#include "PHMovementControl.h"
#include "detail_path_manager.h"
#include "Level.h"
#include "CustomMonster.h"
#include "xrPhysics/IColisiondamageInfo.h"

#include "xrEngine/profiler.h"

// Lain: added
#include "steering_behaviour.h"

#ifdef DEBUG
#include "PHDebug.h"

#define DBG_PH_MOVE_CONDITIONS(c) c
#else // DEBUG
#define DBG_PH_MOVE_CONDITIONS(c)
#endif // DEBUG

#define DISTANCE_PHISICS_ENABLE_CHARACTERS 2.f

float CMovementManager::speed(CPHMovementControl* movement_control) const
{
    VERIFY(movement_control);
    if (fis_zero(m_speed))
        return (0.f);

    if (movement_control->IsCharacterEnabled())
        return (movement_control->GetXZActVelInGoingDir());

    return (m_speed);
}
#ifdef DEBUG
BOOL dbg_dump_collision_hit = FALSE;
void dump_collision_hit(CPHMovementControl* movement_control)
{
    if (!dbg_dump_collision_hit)
        return;
    VERIFY(movement_control);
    // CPHCharacter * phch = movement_control->PHCharacter();
    // VERIFY( phch );

    IPhysicsShellHolder* iobj = movement_control->PhysicsRefObject();
    VERIFY(iobj);
    VERIFY(smart_cast<CPhysicsShellHolder*>(iobj));
    CPhysicsShellHolder* obj = static_cast<CPhysicsShellHolder*>(iobj);
    Msg("ai unit: %s hited by collision; power: %f, spawn frame %d, current frame %d ", obj->cName().c_str(),
        movement_control->gcontact_HealthLost, obj->spawn_time(), Device.dwFrame);
    // CPhysicsShellHolder* object
    // =static_cast<CPhysicsShellHolder*>(Level().Objects.net_Find(m_collision_damage_info.m_obj_id));
    // const ICollisionDamageInfo * di=movement_control->CollisionDamageInfo();
    // VERIFY( di );
    // di->
}
#endif
void CMovementManager::apply_collision_hit(CPHMovementControl* movement_control)
{
    VERIFY(movement_control);
    if (object().g_Alive() && !fsimilar(0.f, movement_control->gcontact_HealthLost))
    {
#ifdef DEBUG
        dump_collision_hit(movement_control);
#endif
        const ICollisionDamageInfo* di = movement_control->CollisionDamageInfo();
        VERIFY(di);
        Fvector dir;
        di->HitDir(dir);

        SHit HDS = SHit(movement_control->gcontact_HealthLost, dir, di->DamageInitiator(),
            movement_control->ContactBone(), di->HitPos(), 0.f, di->HitType(), 0.0f, false);
        object().Hit(&HDS);
    }
}

bool CMovementManager::move_along_path() const
{
    if (!enabled())
        return (false);

    if (!actual())
        return (false);

    //	if (path_completed())
    //		return			(true);

    if (detail().path().empty())
        return (false);

    if (detail().completed(object().Position(), true))
        return (false);

    if (detail().curr_travel_point_index() >= detail().path().size() - 1)
        return (false);

    if (fis_zero(old_desirable_speed()))
        return (false);

    return (true);
}

Fvector CMovementManager::path_position(const float& velocity, const Fvector& position, const float& time_delta,
    u32& current_travel_point, float& dist, float& dist_to_target, Fvector& dir_to_target)
{
    VERIFY(current_travel_point < (detail().path().size() - 1));

    Fvector dest_position = position;

    // Вычислить пройденную дистанцию, определить целевую позицию на маршруте,
    //			 изменить detail().m_current_travel_point

    float desirable_speed = velocity; // желаемая скорость объекта
    dist = desirable_speed * time_delta; // пройденное расстояние в соостветствие с желаемой скоростью

    // определить целевую точку
    Fvector target;

    // обновить detail().m_current_travel_point в соответствие с текущей позицией
    while (current_travel_point < detail().path().size() - 2)
    {
        float pos_dist_to_cur_point = dest_position.distance_to(detail().path()[current_travel_point].position);
        float pos_dist_to_next_point = dest_position.distance_to(detail().path()[current_travel_point + 1].position);
        float cur_point_dist_to_next_point = detail().path()[current_travel_point].position.distance_to(
            detail().path()[current_travel_point + 1].position);

        if ((pos_dist_to_cur_point > cur_point_dist_to_next_point) && (pos_dist_to_cur_point > pos_dist_to_next_point))
        {
            ++current_travel_point;
        }
        else
            break;
    }

    target.set(detail().path()[current_travel_point + 1].position);
    // определить направление к целевой точке
    dir_to_target.sub(target, dest_position);

    // дистанция до целевой точки
    dist_to_target = dir_to_target.magnitude();

    while (dist > dist_to_target)
    {
        dest_position.set(target);
        dist -= dist_to_target;

        if (current_travel_point + 1 >= detail().path().size())
        {
            //			VERIFY				(dist <= dist_to_target);
            return (dest_position);
        }

        ++current_travel_point;
        if ((current_travel_point + 1) >= detail().path().size())
        {
            //			VERIFY				(dist <= dist_to_target);
            dist = 0.f;
            return (dest_position);
        }

        target.set(detail().path()[current_travel_point + 1].position);
        dir_to_target.sub(target, dest_position);
        dist_to_target = dir_to_target.magnitude();
    }

    VERIFY(dist <= dist_to_target);
    return (dest_position);
}

Fvector CMovementManager::path_position(const float& time_to_check)
{
    if (path_completed())
        return (object().Position());

    if (detail().path().empty())
        return (object().Position());

    if (detail().completed(object().Position(), true))
        return (object().Position());

    Fvector dir_to_target;
    float dist_to_target;
    float dist;
    u32 current_travel_point = detail().m_current_travel_point;
    return (path_position(old_desirable_speed(), object().Position(), time_to_check, current_travel_point, dist,
        dist_to_target, dir_to_target));
}

void CMovementManager::move_along_path(CPHMovementControl* movement_control, Fvector& dest_position, float time_delta)
{
    START_PROFILE("Build Path/Move Along Path")

    VERIFY(movement_control);

    Fvector motion;
    dest_position = object().Position();

    float precision = 0.5f;

    // Если нет движения по пути
    if (!move_along_path())
    {
        m_speed = 0.f;

        DBG_PH_MOVE_CONDITIONS(if (ph_dbg_draw_mask.test(phDbgNeverUseAiPhMove)) {
            movement_control->SetPosition(dest_position);
            if (movement_control->CharacterExist())
                movement_control->DisableCharacter();
        })
        if (movement_control->IsCharacterEnabled())
        {
            movement_control->Calculate(detail().path(), 0.f, detail().m_current_travel_point, precision);
            movement_control->GetPosition(dest_position);
        }

        // проверка на хит
        apply_collision_hit(movement_control);
        //		Msg				("[%6d][%s] no move,
        // curr_tp=%d",Device.dwFrame,*object().cName(),detail().m_current_travel_point);
        return;
    }

    //. 	VERIFY2(movement_control->CharacterExist() || object().animation_movement_controlled() , "! Can not move -
    // physics movement shell does not exist. Try to move in wonded state?" );
    if (!movement_control->CharacterExist())
        return;

    if (time_delta < EPS)
        return;

    float desirable_speed = old_desirable_speed(); // желаемая скорость объекта
    float desirable_dist = desirable_speed * time_delta;
    float dist;

    // position_computation
    Fvector dir_to_target;
    float dist_to_target;
    u32 current_travel_point = detail().m_current_travel_point;
    dest_position = path_position(old_desirable_speed(), object().Position(), time_delta, current_travel_point, dist,
        dist_to_target, dir_to_target);

    // Lain: added steering behaviour
    // 	Fvector target;
    // 	target.add(dest_position, dir_to_target);
    // 	Fvector steer_offs = m_steer_manager->calc_acceleration();
    // 	steer_offs.mul(time_delta*10.f);
    // 	target.add(steer_offs);
    // 	dir_to_target.sub(target, dest_position);
    // 	dist_to_target = dir_to_target.magnitude();
    // 	Fvector steer_offs = m_steer_manager->calc_acceleration();
    // 	steer_offs.mul(time_delta*1000.f);
    // 	movement_control->AddControlVel(steer_offs);

    if (detail().m_current_travel_point != current_travel_point)
        on_travel_point_change(detail().m_current_travel_point);
    detail().m_current_travel_point = current_travel_point;

    if (dist_to_target < EPS_L)
    {
#pragma todo("Dima to ? : is this correct?")
        if (current_travel_point + 1 < detail().path().size())
            detail().m_current_travel_point = current_travel_point + 1;
        else
            detail().m_current_travel_point = detail().path().size() - 1;
        m_speed = 0.f;
        // Msg				("[%6d][%s] strange exit,
        // curr_tp=%d",Device.dwFrame,*object().cName(),detail().m_current_travel_point);
        return;
    }
    //	Msg					("[%6d][%s] curr_tp=%d",Device.dwFrame,*object().cName(),detail().m_current_travel_point);

    // Физика устанавливает новую позицию
    // получить физ. объекты в радиусе
    m_nearest_objects.clear();
    Level().ObjectSpace.GetNearest(m_nearest_objects, dest_position,
        DISTANCE_PHISICS_ENABLE_CHARACTERS + (movement_control->IsCharacterEnabled() ? 0.5f : 0.f), &object());

    // установить позицию
    VERIFY(dist >= 0.f);
    VERIFY(dist_to_target >= 0.f);
    //	VERIFY				(dist <= dist_to_target);
    motion.mul(dir_to_target, dist / dist_to_target);
    dest_position.add(motion);

    Fvector velocity = dir_to_target;
    velocity.normalize_safe();
    if (velocity.y > 0.9f)
        velocity.y = 0.8f;
    if (velocity.y < -0.9f)
        velocity.y = -0.8f;
    velocity.normalize_safe(); //как не странно, mdir - не нормирован
    velocity.mul(desirable_speed); //*1.25f

    if (!movement_control->PhysicsOnlyMode())
        movement_control->SetCharacterVelocity(velocity);

    if (DBG_PH_MOVE_CONDITIONS(ph_dbg_draw_mask.test(phDbgNeverUseAiPhMove) ||
            !ph_dbg_draw_mask.test(phDbgAlwaysUseAiPhMove) &&) !(m_nearest_objects.empty()))
    { //  физ. объект

        if (DBG_PH_MOVE_CONDITIONS(!ph_dbg_draw_mask.test(phDbgNeverUseAiPhMove) &&) !movement_control->TryPosition(
                dest_position))
        {
            movement_control->GetPosition(dest_position);
            movement_control->Calculate(detail().path(), desirable_speed, detail().m_current_travel_point, precision);

            // проверка на хит
            apply_collision_hit(movement_control);
        }
        else
        {
            DBG_PH_MOVE_CONDITIONS(if (ph_dbg_draw_mask.test(phDbgNeverUseAiPhMove)) {
                movement_control->SetPosition(dest_position);
                movement_control->DisableCharacter();
            })
            movement_control->b_exect_position = true;
        }
        movement_control->GetPosition(dest_position);
    }
    else
    {
        // DBG_PH_MOVE_CONDITIONS(
        // if(ph_dbg_draw_mask.test(phDbgNeverUseAiPhMove)){movement_control->SetPosition(dest_position);movement_control->DisableCharacter();})
        movement_control->SetPosition(dest_position);
        movement_control->DisableCharacter();
        movement_control->b_exect_position = true;
    }
    /*
    } else { // есть физ. объекты

        movement_control->Calculate				(detail().path(), desirable_speed, detail().m_current_travel_point,
    precision);
        movement_control->GetPosition			(dest_position);

        // проверка на хит
        apply_collision_hit						(movement_control);
    }
        */

    // установить скорость
    float real_motion = motion.magnitude() + desirable_dist - dist;
    float real_speed = real_motion / time_delta;

    m_speed = 0.5f * desirable_speed + 0.5f * real_speed;

    // Физика устанавливает позицию в соответствии с нулевой скоростью
    if (detail().completed(dest_position, true))
    {
        if (!movement_control->PhysicsOnlyMode())
        {
            Fvector velocity = {0.f, 0.f, 0.f};
            movement_control->SetVelocity(velocity);
            m_speed = 0.f;
        }
    }
    STOP_PROFILE
}
