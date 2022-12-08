#include "stdafx.h"
#include "xrSheduler.h"
#include "xr_object.h"
// XXX: rename this file to ScheduledBase.cpp
ScheduledBase::ScheduledBase()
{
    shedule.t_min = 20;
    shedule.t_max = 1000;
    shedule.b_locked = false;
#ifdef DEBUG
    shedule.dbg_startframe = 1;
    shedule.dbg_update_shedule = 0;
#endif
}

ScheduledBase::~ScheduledBase()
{
    VERIFY2(!Engine.Sheduler.Registered(this), make_string("0x%08x : %s", this, *shedule_Name()));

// XXX: WTF???
// sad, but true
// we need this to become MASTER_GOLD
#ifndef DEBUG
    Engine.Sheduler.Unregister(this);
#endif // DEBUG
}

void ScheduledBase::shedule_register() { Engine.Sheduler.Register(this); }
void ScheduledBase::shedule_unregister() { Engine.Sheduler.Unregister(this); }
void ScheduledBase::shedule_Update(u32 dt)
{
#ifdef DEBUG
    if (shedule.dbg_startframe == shedule.dbg_update_shedule)
    {
        pcstr name = "unknown";
        const auto object = dynamic_cast<IGameObject*>(this);
        if (object)
            name = object->cName().c_str();
        xrDebug::Fatal(DEBUG_INFO, "'shedule_Update' called twice per frame for %s", name);
    }
    shedule.dbg_update_shedule = shedule.dbg_startframe;
#endif
}
