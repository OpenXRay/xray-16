////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_smart_terrain_registry.cpp
//	Created 	: 20.09.2005
//  Modified 	: 20.09.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife smart terrain registry
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_smart_terrain_registry.h"
#include "xrServer_Objects_ALife_Monsters.h"

CALifeSmartTerrainRegistry::~CALifeSmartTerrainRegistry() {}
void CALifeSmartTerrainRegistry::add(CSE_ALifeDynamicObject* object)
{
    CSE_ALifeSmartZone* zone = smart_cast<CSE_ALifeSmartZone*>(object);
    if (!zone)
        return;

    VERIFY(objects().find(object->ID) == objects().end());
    m_objects.insert(std::make_pair(object->ID, zone));
}

void CALifeSmartTerrainRegistry::remove(CSE_ALifeDynamicObject* object)
{
    CSE_ALifeSmartZone* zone = smart_cast<CSE_ALifeSmartZone*>(object);
    if (!zone)
        return;

    OBJECTS::iterator I = m_objects.find(object->ID);
    VERIFY(I != m_objects.end());
    m_objects.erase(I);
}
