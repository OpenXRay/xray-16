////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_velocity_holder.cpp
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Stalker velocity holder inline functions
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "stalker_velocity_holder.h"
#include "Common/object_broker.h"
#include "stalker_velocity_collection.h"

CStalkerVelocityHolder* g_stalker_velocity_holder = 0;

CStalkerVelocityHolder::~CStalkerVelocityHolder() { delete_data(m_collections); }
const CStalkerVelocityHolder::COLLECTION& CStalkerVelocityHolder::collection(const shared_str& section)
{
    COLLECTIONS::const_iterator I = m_collections.find(section);
    if (I != m_collections.end())
        return (*(*I).second);

    COLLECTION* collection = new COLLECTION(section);
    m_collections.insert(std::make_pair(section, collection));
    return (*collection);
}
