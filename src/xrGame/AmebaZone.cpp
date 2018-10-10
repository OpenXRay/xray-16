#include "StdAfx.h"

#include "AmebaZone.h"
#include "ZoneVisual.h"
#include "CustomZone.h"
#include "xrEngine/xr_collide_form.h"
#include "Include/xrRender/Kinematics.h"
#include "PhysicsShellHolder.h"
#include "PHMovementControl.h"
#include "CharacterPhysicsSupport.h"
#include "entity_alive.h"


CAmebaZone::CAmebaZone() : m_fVelocityLimit(1.f) {}

CAmebaZone::~CAmebaZone() {}

void CAmebaZone::Load(LPCSTR section)
{
    inherited::Load(section);
    m_fVelocityLimit = pSettings->r_float(section, "max_velocity_in_zone");
}

bool CAmebaZone::BlowoutState()
{
    bool result = inherited::BlowoutState();
    if (!result)
        UpdateBlowout();

    for (auto& it : m_ObjectInfoMap)
        Affect(&it);

    return result;
}

void CAmebaZone::Affect(SZoneObjectInfo* O)
{
    CPhysicsShellHolder* pGameObject = smart_cast<CPhysicsShellHolder*>(O->object);
    if (!pGameObject) return;

    if (O->zone_ignore) return;

    Fvector hit_dir;
    hit_dir.set(Random.randF(-.5f, .5f),
                Random.randF(.0f, 1.f),
                Random.randF(-.5f, .5f));
    hit_dir.normalize();

    Fvector position_in_bone_space;

    float power = Power(distance_to_center(O->object), m_fEffectiveRadius);
    float power_critical = 0.0f;
    float impulse = m_fHitImpulseScale * power * pGameObject->GetMass();

    if (power > 0.01f)
    {
        //m_dwDeltaTime = 0;
        position_in_bone_space.set(0.f, 0.f, 0.f);

        CreateHit(pGameObject->ID(), ID(), hit_dir, power, 0, position_in_bone_space, impulse, m_eHitTypeBlowout);

        PlayHitParticles(pGameObject);
    }
}

void CAmebaZone::PhTune(float step)
{
    for (auto& it : m_ObjectInfoMap)
    {
        auto EA = smart_cast<CEntityAlive*>(it.object);
        if (EA)
        {
            CPHMovementControl* mc = EA->character_physics_support()->movement();
            if (mc)
            {
                if (distance_to_center(EA) < effective_radius(m_fEffectiveRadius))
                    mc->SetVelocityLimit(m_fVelocityLimit);
            }
        }
    }
}

void CAmebaZone::SwitchZoneState(EZoneState new_state)
{
    if (new_state == eZoneStateBlowout && m_eZoneState != eZoneStateBlowout)
        Activate();

    if (new_state != eZoneStateBlowout && m_eZoneState == eZoneStateBlowout)
        Deactivate();

    inherited::SwitchZoneState(new_state);
}

float CAmebaZone::distance_to_center(CGameObject* O)
{
    Fvector P;
    XFORM().transform_tiny(P, CForm->getSphere().P);
    Fvector OP;
    OP.set(O->Position());
    return _sqrt((P.x - OP.x) * (P.x - OP.x) + (P.x - OP.x) * (P.x - OP.x));
}
