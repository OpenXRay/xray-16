#include "StdAfx.h"

#include "xrEngine/IGame_Persistent.h"

#include "Physics.h"
#include "PHObject.h"
#include "PHCharacter.h"
#include "PHWorld.h"
#include "PHCollideValidator.h"
#include "console_vars.h"
#ifdef DEBUG
#include "debug_output.h"
#endif

extern CPHWorld* ph_world;

CPHObject::CPHObject() : SpatialBase(g_pGamePersistent->SpatialSpacePhysic)
{
    m_flags.flags = 0;
    spatial.type |= STYPE_PHYSIC;
    m_check_count = 0;
    CPHCollideValidator::InitObject(*this);
}

void CPHObject::activate()
{
    if (m_flags.test(st_activated))
        return;
    if (m_flags.test(st_freezed))
    {
        UnFreeze();
        return;
    }
    if (m_flags.test(st_recently_deactivated))
        remove_from_recently_deactivated();
    ph_world->AddObject(this);
    vis_update_activate();
    m_flags.set(st_activated, TRUE);
}

void CPHObject::EnableObject(CPHObject* obj) { activate(); }

void CPHObject::deactivate()
{
    if (!m_flags.test(st_activated))
        return;
    ph_world->RemoveObject(PH_OBJECT_I(this));
    vis_update_deactivate();
    m_flags.set(st_activated, FALSE);
}

void CPHObject::put_in_recently_deactivated()
{
    VERIFY(!m_flags.test(st_activated) && !m_flags.test(st_freezed));
    if (m_flags.test(st_recently_deactivated))
        return;
    m_check_count = u8(ph_console::ph_tri_clear_disable_count);
    m_flags.set(st_recently_deactivated, TRUE);
    ph_world->AddRecentlyDisabled(this);
}

void CPHObject::remove_from_recently_deactivated()
{
    if (!m_flags.test(st_recently_deactivated))
        return;
    m_check_count = 0;
    m_flags.set(st_recently_deactivated, FALSE);
    ph_world->RemoveFromRecentlyDisabled(PH_OBJECT_I(this));
}

void CPHObject::check_recently_deactivated()
{
    if (m_check_count == 0)
    {
        ClearRecentlyDeactivated();
        remove_from_recently_deactivated();
    }
    else
        m_check_count--;
}

void CPHObject::spatial_move()
{
    get_spatial_params();
    SpatialBase::spatial_move();
    m_flags.set(st_dirty, TRUE);
}

void CPHObject::Collide()
{
    m_flags.set(st_dirty, FALSE);
}

void CPHObject::CollideDynamics()
{

}

void CPHObject::reinit_single()
{
    ph_world->r_spatial.clear();
}

void CPHObject::step_prediction(float time)
{
}

bool CPHObject::step_single(float step)
{
    if (CastType() != tpCharacter) return false;
    PhTune(step);
    PhDataUpdate(step);
    spatial_move();
    return true;
}

void CPHObject::step(float time)
{
    if (CastType() != tpCharacter || time <= 0) return;
    // CharacterVirtual can advance independently without stepping unrelated
    // native bodies. The movement controller uses this for forced placement.
    PhDataUpdate(time);
    spatial_move();
}

bool CPHObject::DoCollideObj()
{
    return CastType() == tpCharacter &&
        GetPhysicsCore()->CharacterHasDynamicContact(static_cast<CPHCharacter*>(this)->get_body());
}

void CPHObject::FreezeContent()
{
    R_ASSERT(!m_flags.test(st_freezed));
    m_flags.set(st_freezed, TRUE);
    m_flags.set(st_activated, FALSE);
    vis_update_deactivate();
}

void CPHObject::UnFreezeContent()
{
    R_ASSERT(m_flags.test(st_freezed));
    m_flags.set(st_freezed, FALSE);
    m_flags.set(st_activated, TRUE);
    vis_update_activate();
}

void CPHObject::spatial_register()
{
    get_spatial_params();
    SpatialBase::spatial_register();
    m_flags.set(st_dirty, TRUE);
}

void CPHObject::collision_disable() { SpatialBase::spatial_unregister(); }
void CPHObject::collision_enable() { SpatialBase::spatial_register(); }

void CPHObject::Freeze()
{
    if (!m_flags.test(st_activated))
        return;
    ph_world->RemoveObject(this);
    ph_world->AddFreezedObject(this);
    FreezeContent();
}

void CPHObject::UnFreeze()
{
    if (!m_flags.test(st_freezed))
        return;
    UnFreezeContent();
    ph_world->RemoveFreezedObject(this);
    ph_world->AddObject(this);
}

CPHUpdateObject::CPHUpdateObject() { b_activated = false; }

void CPHUpdateObject::Activate()
{
    if (b_activated)
        return;
    ph_world->AddUpdateObject(this);
    b_activated = true;
}

void CPHUpdateObject::Deactivate()
{
    if (!b_activated)
        return;
    ph_world->RemoveUpdateObject(this);
    b_activated = false;
}
