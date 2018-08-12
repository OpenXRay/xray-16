////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_schedule_registry.сзз
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife schedule registry
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_schedule_registry.h"

CALifeScheduleRegistry::~CALifeScheduleRegistry() {}
void CALifeScheduleRegistry::add(CSE_ALifeDynamicObject* object)
{
    CSE_ALifeSchedulable* schedulable = smart_cast<CSE_ALifeSchedulable*>(object);
    if (!schedulable)
        return;

    if (!schedulable->need_update(object))
        return;

    inherited::add(object->ID, schedulable);
}

void CALifeScheduleRegistry::remove(CSE_ALifeDynamicObject* object, bool no_assert)
{
    CSE_ALifeSchedulable* schedulable = smart_cast<CSE_ALifeSchedulable*>(object);
    if (!schedulable)
        return;

    inherited::remove(object->ID, no_assert || !schedulable->need_update(object));
}
