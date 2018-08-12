////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_group_registry.cpp
//	Created 	: 28.10.2005
//  Modified 	: 28.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife group registry
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_group_registry.h"
#include "xrServer_Objects_ALife_Monsters.h"

CALifeGroupRegistry::~CALifeGroupRegistry() {}
void CALifeGroupRegistry::add(CSE_ALifeDynamicObject* object)
{
    CSE_ALifeOnlineOfflineGroup* group = smart_cast<CSE_ALifeOnlineOfflineGroup*>(object);
    if (!group)
        return;

    OBJECTS::const_iterator I = objects().find(group->ID);
    VERIFY(I == objects().end());
    m_objects.insert(std::make_pair(group->ID, group));
}

void CALifeGroupRegistry::remove(CSE_ALifeDynamicObject* object)
{
    CSE_ALifeOnlineOfflineGroup* group = smart_cast<CSE_ALifeOnlineOfflineGroup*>(object);
    if (!group)
        return;

    OBJECTS::iterator I = m_objects.find(group->ID);
    VERIFY(I != m_objects.end());
    m_objects.erase(I);
}

CALifeGroupRegistry::OBJECT& CALifeGroupRegistry::object(const ALife::_OBJECT_ID& id) const
{
    OBJECTS::const_iterator I = objects().find(id);
    VERIFY(I != objects().end());
    return (*(*I).second);
}

void CALifeGroupRegistry::on_after_game_load()
{
    OBJECTS::iterator I = m_objects.begin();
    OBJECTS::iterator E = m_objects.end();
    for (; I != E; ++I)
        (*I).second->on_after_game_load();
}
