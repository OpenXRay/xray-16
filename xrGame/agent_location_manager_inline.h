////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_location_manager_inline.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Agent location manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

struct CRemoveDangerObject {
	const CObject				*m_object;

	IC			CRemoveDangerObject	(const CObject *object)
	{
		m_object				= object;
	}

	IC	bool	operator()	(const CAgentLocationManager::CDangerLocationPtr &location) const
	{
		return					(*location == m_object);
	}
};

IC	CAgentLocationManager::CAgentLocationManager(CAgentManager *object)
{
	VERIFY						(object);
	m_object					= object;
}

IC	CAgentManager &CAgentLocationManager::object() const
{
	VERIFY						(m_object);
	return						(*m_object);
}

IC	void CAgentLocationManager::clear			()
{
	m_danger_locations.clear	();
}

IC	CAgentLocationManager::CDangerLocationPtr CAgentLocationManager::location	(const CObject *object)
{
	LOCATIONS::iterator			I = std::find_if(m_danger_locations.begin(),m_danger_locations.end(),CRemoveDangerObject(object));
	if (I != m_danger_locations.end())
		return					(*I);
	return						(0);
}

const CAgentLocationManager::LOCATIONS &CAgentLocationManager::locations	() const
{
	return						(m_danger_locations);
}

