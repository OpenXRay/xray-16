#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>
#define CStateControlCampAbstract CStateControlCamp<_Object>

#define ANGLE_DISP					PI_DIV_2
#define ANGLE_DISP_STEP				deg(10)
#define TRACE_STATIC_DIST			3.f
#define TIME_POINT_CHANGE_MIN		2000
#define TIME_POINT_CHANGE_MAX		5000


TEMPLATE_SPECIALIZATION
void CStateControlCampAbstract::initialize()
{
	inherited::initialize			();

	float angle			= ai().level_graph().vertex_low_cover_angle(object->ai_location().level_vertex_id(),deg(10), std::greater<float>());
	
	collide::rq_result	l_rq;

	m_angle_from		= angle_normalize(angle - ANGLE_DISP);
	m_angle_to			= angle_normalize(angle + ANGLE_DISP);

	Fvector				trace_from;
	object->Center		(trace_from);
	Fvector				direction;

	// trace discretely left
	for (float ang = angle; angle_difference(ang, angle) < ANGLE_DISP; ang = angle_normalize(ang - ANGLE_DISP_STEP)) {
		
		direction.setHP	(ang, 0.f);
		
		if (Level().ObjectSpace.RayPick(trace_from, direction, TRACE_STATIC_DIST, collide::rqtStatic, l_rq, object)) {
			if ((l_rq.range < TRACE_STATIC_DIST)) {
				m_angle_from = ang;
				break;
			}
		}
	}
	
	// trace discretely right
	for (float ang = angle; angle_difference(ang, angle) < ANGLE_DISP; ang = angle_normalize(ang + ANGLE_DISP_STEP)) {
		
		direction.setHP	(ang, 0.f);

		if (Level().ObjectSpace.RayPick(trace_from, direction, TRACE_STATIC_DIST, collide::rqtStatic, l_rq, object)) {
			if ((l_rq.range < TRACE_STATIC_DIST)) {
				m_angle_to = ang;
				break;
			}
		}
	}
	
	m_time_next_updated	= 0;

	m_target_angle		= m_angle_from;

	Fvector pos;
	pos.mad(object->Position(), Fvector().setHP(angle,0.f), 3.f);
	object->dir().face_target(pos);
}

TEMPLATE_SPECIALIZATION
void CStateControlCampAbstract::execute()
{
	update_target_angle						();

	Fvector point;
	point.mad								(object->Position(),Fvector().setHP(m_target_angle, 0.f), 3.f);
	
	object->custom_dir().head_look_point	(point);
	object->custom_anim().set_body_state	(CControllerAnimation::eTorsoIdle,CControllerAnimation::eLegsTypeSteal);
}

TEMPLATE_SPECIALIZATION
bool CStateControlCampAbstract::check_start_conditions()
{
	if (object->EnemyMan.see_enemy_now()) return false;
	return true;
}

TEMPLATE_SPECIALIZATION
bool CStateControlCampAbstract::check_completion()
{
	if (object->EnemyMan.see_enemy_now()) return true;
	if (time_state_started + 2000 < time()) return true;

	return false;
}

TEMPLATE_SPECIALIZATION
void CStateControlCampAbstract::update_target_angle()
{
	if (m_time_next_updated > time()) return;
	m_time_next_updated = time() + Random.randI(TIME_POINT_CHANGE_MIN,TIME_POINT_CHANGE_MAX);

	if (fsimilar(m_target_angle, m_angle_from)) 
		m_target_angle = m_angle_to;
	else 
		m_target_angle = m_angle_from;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateControlCampAbstract
