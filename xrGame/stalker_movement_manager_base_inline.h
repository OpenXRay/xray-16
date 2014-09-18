////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_movement_manager_base_inline.h
//	Created 	: 27.12.2003
//	Modified	: 13.02.2008
//	Author		: Dmitriy Iassenev
//	Description : stalker movement manager base class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef STALKER_MOVEMENT_MANAGER_BASE_INLINE_H_INCLUDED
#define STALKER_MOVEMENT_MANAGER_BASE_INLINE_H_INCLUDED

IC const MonsterSpace::SBoneRotation &stalker_movement_manager_base::head_orientation() const
{
	return						(m_head);
}

IC void stalker_movement_manager_base::set_head_orientation(const MonsterSpace::SBoneRotation &orientation)
{
	m_head						= orientation;
}

IC	void stalker_movement_manager_base::set_desired_direction(Fvector const* direction)
{
	m_target.desired_direction	(direction);
}

IC	void stalker_movement_manager_base::add_velocity		(int mask, float linear, float compute_angular)
{
	add_velocity				(mask,linear,compute_angular,compute_angular);
}

IC	bool stalker_movement_manager_base::turn_in_place			() const
{
	return						(!path_completed() && fis_zero(speed()) && (angle_difference(body_orientation().current.yaw,body_orientation().target.yaw) > EPS_L));
}

IC	void stalker_movement_manager_base::set_body_state(EBodyState body_state)
{
	THROW2						(
		(body_state != eBodyStateCrouch) || (m_target.m_mental_state != eMentalStateFree),
		make_string("object %s", object().cName().c_str()).c_str()
	);
	m_target.m_body_state		= body_state;
}

IC	void stalker_movement_manager_base::set_movement_type(EMovementType movement_type)
{
	m_target.m_movement_type	= movement_type;
}

IC	void stalker_movement_manager_base::set_mental_state(EMentalState mental_state)
{
	THROW						((m_target.m_body_state != eBodyStateCrouch) || (mental_state != eMentalStateFree));
	m_target.m_mental_state		= mental_state;
//#pragma todo("Dima to Dima: this is correct, commented just because of the October presentation, no time right now to fix it correctly, should be fixed sometimes later")
//.	m_path_actuality			= m_path_actuality && (m_target.m_mental_state == m_current.m_mental_state);
}

IC	void stalker_movement_manager_base::set_path_type(EPathType path_type)
{
	m_target.m_path_type		= path_type;
}

IC	void stalker_movement_manager_base::set_detail_path_type(EDetailPathType detail_path_type)
{
	m_target.m_detail_path_type	= detail_path_type;
}

IC	MonsterSpace::EBodyState const& stalker_movement_manager_base::body_state() const
{
	return						(m_current.m_body_state);
}

IC	MonsterSpace::EBodyState const& stalker_movement_manager_base::target_body_state() const
{
	return						(m_target.m_body_state);
}

IC MonsterSpace::EMovementType const& stalker_movement_manager_base::movement_type() const
{
	return						(m_current.m_movement_type);
}

IC MonsterSpace::EMentalState const& stalker_movement_manager_base::mental_state() const
{
	return						(m_current.m_mental_state);
}

IC MonsterSpace::EMentalState const& stalker_movement_manager_base::target_mental_state() const
{
	return						(m_target.m_mental_state);
}

IC	MovementManager::EPathType const& stalker_movement_manager_base::path_type	() const
{
	return						(m_current.m_path_type);
}

IC	DetailPathManager::EDetailPathType const& stalker_movement_manager_base::detail_path_type	() const
{
	return						(m_current.m_detail_path_type);
}

IC	Fvector const* stalker_movement_manager_base::desired_position			() const
{
	return						(m_current.desired_position());
}

IC	Fvector const* stalker_movement_manager_base::desired_direction			() const
{
	return						(m_current.desired_direction());
}

IC	CAI_Stalker &stalker_movement_manager_base::object						() const
{
	VERIFY						(m_object);
	return						(*m_object);
}

IC	MonsterSpace::EMovementType const& stalker_movement_manager_base::target_movement_type	() const
{
	return						(m_target.m_movement_type);
}

IC	void stalker_movement_manager_base::danger_head_speed						(const float &speed)
{
	m_danger_head_speed			= speed;
}

IC	stalker_movement_params const& stalker_movement_manager_base::current_params() const
{
	return						(m_current);
}

IC	stalker_movement_params& stalker_movement_manager_base::target_params		()
{
	return						(m_target);
}

#endif // #ifndef STALKER_MOVEMENT_MANAGER_BASE_INLINE_H_INCLUDED