////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_planner_target_provider.cpp
//	Created 	: 18.09.2007
//	Author		: Alexander Dudin
//	Description : Target provider for target selector
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_planner_target_provider.h"
#include "script_game_object.h"
#include "smart_cover_animation_planner.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "agent_manager.h"
#include "agent_enemy_manager.h"
#include "Weapon.h"

using smart_cover::animation_planner;
using smart_cover::target_provider;
using smart_cover::target_idle;
using smart_cover::target_fire;
using smart_cover::target_fire_no_lookout;

target_provider::target_provider(animation_planner* object, LPCSTR name,
    StalkerDecisionSpace::EWorldProperties const& world_property, u32 const& loophole_value)
    : inherited(object, name), m_world_property(world_property), m_loophole_value(loophole_value)
{
}

void target_provider::setup(animation_planner* object, CPropertyStorage* storage) { inherited::setup(object, storage); }
void target_provider::initialize()
{
    inherited::initialize();
    m_object->target(m_world_property);
    m_storage->set_property(m_world_property, true);
    m_object->decrease_loophole_value(m_loophole_value);
}

void target_provider::finalize() { inherited::finalize(); }
////////////////////////////////////////////////////////////////////////////
//	class target_idle
////////////////////////////////////////////////////////////////////////////

target_idle::target_idle(animation_planner* object, LPCSTR name,
    StalkerDecisionSpace::EWorldProperties const& world_property, u32 const& loophole_value)
    : inherited(object, name, world_property, loophole_value)
{
}

void target_idle::execute()
{
    inherited::execute();

    if (!completed())
        return;

    m_storage->set_property(StalkerDecisionSpace::eWorldPropertyLoopholeTooMuchTimeFiring, false);
}

////////////////////////////////////////////////////////////////////////////
//	class target_fire
////////////////////////////////////////////////////////////////////////////

target_fire::target_fire(animation_planner* object, LPCSTR name,
    StalkerDecisionSpace::EWorldProperties const& world_property, u32 const& loophole_value)
    : inherited(object, name, world_property, loophole_value)
{
}

void target_fire::initialize()
{
    if (this->m_object->m_object->agent_manager().enemy().enemies().size() > 1)
        set_inertia_time(6000 + ::Random.randI(3000));
    else
        set_inertia_time(0);

    inherited::initialize();
}

void target_fire::execute()
{
    inherited::execute();

    if (!m_inertia_time)
        return;

    if (!completed())
        return;

    if (this->m_object->m_object->ready_to_kill())
    {
        CWeapon* weapon = smart_cast<CWeapon*>(this->m_object->m_object->m_best_item_to_kill);
        if (weapon)
        {
            if (weapon->GetAmmoElapsed() <= weapon->GetAmmoMagSize() / 6)
                return;
        }
    }

    m_storage->set_property(StalkerDecisionSpace::eWorldPropertyLoopholeTooMuchTimeFiring, true);
}

////////////////////////////////////////////////////////////////////////////
//	class target_fire_no_lookout
////////////////////////////////////////////////////////////////////////////

target_fire_no_lookout::target_fire_no_lookout(animation_planner* object, LPCSTR name,
    StalkerDecisionSpace::EWorldProperties const& world_property, u32 const& loophole_value)
    : inherited(object, name, world_property, loophole_value)
{
}

void target_fire_no_lookout::initialize()
{
    m_storage->set_property(StalkerDecisionSpace::eWorldPropertyLookedOut, false);
    inherited::initialize();
}
