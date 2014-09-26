////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_online_offline_group_brain_inline.h
//	Created 	: 25.10.2005
//  Modified 	: 25.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife Online Offline Group brain class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CALifeOnlineOfflineGroupBrain::object_type &CALifeOnlineOfflineGroupBrain::object				() const
{
	VERIFY						(m_object);
	return						(*m_object);
}

IC	CALifeOnlineOfflineGroupBrain::movement_manager_type &CALifeOnlineOfflineGroupBrain::movement	() const
{
	VERIFY						(m_movement_manager);
	return						(*m_movement_manager);
}