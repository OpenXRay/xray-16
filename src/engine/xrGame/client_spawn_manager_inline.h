////////////////////////////////////////////////////////////////////////////
//	Module 		: client_spawn_manager_inline.h
//	Created 	: 08.10.2004
//  Modified 	: 08.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Seniority hierarchy holder inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CClientSpawnManager::CClientSpawnManager	()
{
}

#ifdef DEBUG
IC	const CClientSpawnManager::REQUEST_REGISTRY &CClientSpawnManager::registry	() const
{
	return		(m_registry);
}
#endif // DEBUG