////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_action_inline.h
//	Created 	: 27.12.2003
//  Modified 	: 03.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Sight action inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CSightAction::CSightAction		() :
	m_sight_type		(SightManager::eSightTypeCurrentDirection),
	m_torso_look		(false),
	m_path				(false),
	m_object_to_look	(0),
	m_memory_object		(0),
	m_state_fire_object	(0),
	m_initialized		(false)
{
}

IC	CSightAction::CSightAction		(const ESightType &sight_type, bool	torso_look, bool path) :
	m_sight_type		(sight_type),
	m_torso_look		(torso_look),
	m_path				(path),
	m_object_to_look	(0),
	m_memory_object		(0),
	m_state_fire_object	(0),
	m_initialized		(false)
{
}

IC	CSightAction::CSightAction		(const ESightType &sight_type, const Fvector &vector3d, bool torso_look) :
	m_sight_type		(sight_type),
	m_vector3d			(vector3d),
	m_path				(false),
	m_torso_look		(torso_look),
	m_object_to_look	(0),
	m_memory_object		(0),
	m_state_fire_object	(0),
	m_initialized		(false)
{
}

IC	CSightAction::CSightAction		(const CGameObject *object_to_look, bool torso_look, bool fire_object, bool no_pitch) :
	m_sight_type		(fire_object ? SightManager::eSightTypeFireObject : SightManager::eSightTypeObject),
	m_torso_look		(torso_look),
	m_path				(false),
	m_object_to_look	(object_to_look),
	m_memory_object		(0),
	m_no_pitch			(no_pitch),
	m_state_fire_object	(0),
	m_initialized		(false)
{
}

IC	CSightAction::CSightAction		(const CMemoryInfo *memory_object, bool torso_look) :
	m_sight_type		(SightManager::eSightTypeObject),
	m_torso_look		(torso_look),
	m_path				(false),
	m_memory_object		(memory_object),
	m_object_to_look	(0),
	m_state_fire_object	(0),
	m_initialized		(false)
{
}

IC	CSightAction::CSightAction		(const ESightType &sight_type, const Fvector *vector3d) :
	m_sight_type		(sight_type),
	m_path				(false),
	m_object_to_look	(0),
	m_state_fire_object	(0),
	m_initialized		(false)
{
	if (sight_type == SightManager::eSightTypeFirePosition) {
		m_sight_type	= SightManager::eSightTypePosition;
		m_torso_look	= true;
	}
	else
		m_torso_look	= false;

	if (vector3d)
		m_vector3d		= *vector3d;
}

IC	bool CSightAction::operator==		(const CSightAction &sight_action) const
{
	if (m_sight_type != sight_action.m_sight_type)
		return			(false);

	switch (m_sight_type) {
		case SightManager::eSightTypeCurrentDirection :
			return		(m_torso_look == sight_action.m_torso_look);
		case SightManager::eSightTypePathDirection :
			return		(m_torso_look == sight_action.m_torso_look);
		case SightManager::eSightTypeDirection :
			return		((m_torso_look == sight_action.m_torso_look) && m_vector3d.similar(sight_action.m_vector3d));
		case SightManager::eSightTypePosition :
			return		((m_torso_look == sight_action.m_torso_look) && m_vector3d.similar(sight_action.m_vector3d));
		case SightManager::eSightTypeObject :
			return		((m_torso_look == sight_action.m_torso_look) && (m_object_to_look == sight_action.m_object_to_look));
		case SightManager::eSightTypeCover :
			return		((m_path == sight_action.m_path) && (m_torso_look == sight_action.m_torso_look));
		case SightManager::eSightTypeSearch :
			return		((m_path == sight_action.m_path) && (m_torso_look == sight_action.m_torso_look));
		case SightManager::eSightTypeCoverLookOver :
			return		(m_time == sight_action.m_time);
		case SightManager::eSightTypeFireObject :
			return		((m_torso_look == sight_action.m_torso_look) && (m_object_to_look == sight_action.m_object_to_look));
		case SightManager::eSightTypeAnimationDirection :
			return		(true);
		default	: NODEFAULT;
	}

#ifdef DEBUG
	return				(true);
#endif
}

IC	void CSightAction::set_vector3d			(const Fvector &vector3d)
{
	m_vector3d			= vector3d;
}

IC	void CSightAction::set_object_to_look	(const CGameObject *object_to_look)
{
	m_object_to_look	= object_to_look;
}

IC	void CSightAction::set_memory_object	(const CMemoryInfo *memory_object)
{
	m_memory_object		= memory_object;
}

IC	CSightAction::ESightType CSightAction::sight_type	() const
{
	return				(m_sight_type);
}

IC	const CGameObject *CSightAction::object_to_look		() const
{
	return				(m_object_to_look);
}

IC	const Fvector &CSightAction::vector3d				() const
{
	return				(m_vector3d);
}

IC	u32 const& CSightAction::state_fire_object			() const
{
	VERIFY				(m_sight_type == SightManager::eSightTypeFireObject);
	return				(m_state_fire_object);
}