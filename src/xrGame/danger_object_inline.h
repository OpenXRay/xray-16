////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_object_inline.h
//	Created 	: 14.02.2005
//  Modified 	: 14.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger object inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CDangerObject::CDangerObject											(
	const CEntityAlive	*object,
	const Fvector &position,
	u32 time,
	const EDangerType &type,
	const EDangerPerceiveType &perceive_type,
	const CObject *dependent_object
)
{
	m_object			= object;
	m_dependent_object	= dependent_object;
	m_position			= position;
	m_time				= time;
	m_type				= type;
	m_perceive_type		= perceive_type;
}

IC	const CEntityAlive *CDangerObject::object								() const
{
	return				(m_object);
}

IC	const Fvector &CDangerObject::position									() const
{
	return				(m_position);
}

IC	u32	CDangerObject::time													() const
{
	return				(m_time);
}

IC	CDangerObject::EDangerType CDangerObject::type							() const
{
	return				(m_type);
}

IC	CDangerObject::EDangerPerceiveType CDangerObject::perceive_type			() const
{
	return				(m_perceive_type);
}

IC	const CObject *CDangerObject::dependent_object							() const
{
	return				(m_dependent_object);
}

IC	void CDangerObject::clear_dependent_object								()
{
	m_dependent_object	= 0;
}

IC	bool CDangerObject::operator==											(const CDangerObject &object) const
{
	if (!m_object && object.object())
		return			(false);

	if (m_object && !object.object())
		return			(false);

	if (m_object && (m_object->ID() != object.object()->ID()))
		return			(false);

	return				((type() == object.type()) &&(perceive_type() == object.perceive_type()));
}
