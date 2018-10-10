#include "StdAfx.h"
#include "control_movement.h"
#include "basemonster/base_monster.h"
#include "control_manager.h"
#include "PHMovementControl.h"
#include "CharacterPhysicsSupport.h"

void CControlMovement::reinit()
{
    inherited::reinit();

    m_velocity_current = 0.f;

    m_data.velocity_target = 0.f;
    m_data.acc = flt_max;
}

void CControlMovement::update_frame()
{
    velocity_lerp(m_velocity_current, m_data.velocity_target, m_data.acc, m_object->client_update_fdelta());

    m_object->m_fCurSpeed = m_velocity_current;
    m_man->path_builder().set_desirable_speed(m_velocity_current);
}

float CControlMovement::real_velocity()
{
    CPHMovementControl* movement_control = m_object->character_physics_support()->movement();
    VERIFY(movement_control);

    if (movement_control->IsCharacterEnabled())
    {
        float tmp = movement_control->GetXZActVelInGoingDir();
#ifdef DEBUG
        if (_abs(tmp) > 1000)
        {
            Log("! GetVelocity", movement_control->GetVelocity());
            Log("! GetPathDir", movement_control->GetPathDir());
        }
#endif // DEBUG
        clamp(tmp, 0.0f, 15.0f);
        VERIFY2(_abs(tmp) < 1000, "movement_control->GetXZActVelInGoingDir() returns too big speed");
        return tmp;
    }

    return m_velocity_current;
}
