#include "stdafx.h"
#include "control_direction.h"
#include "BaseMonster/base_monster.h"
#include "control_manager.h"

#include "../../detail_path_manager.h"
#include "../../level_graph.h"
#include "../../ai_space.h"
#include "../../ai_object_location.h"

#include "../../detail_path_manager_space.h"

void CControlDirection::reinit()
{	
	inherited::reinit			();

	m_heading.init				();
	m_heading.current_angle		= m_man->path_builder().m_body.current.yaw;

	m_pitch.init				();	
	m_pitch.current_angle		= m_man->path_builder().m_body.current.pitch;

	m_data.heading.target_angle	= m_man->path_builder().m_body.target.yaw;
	m_data.pitch.target_angle	= m_man->path_builder().m_body.target.pitch;
	m_data.linear_dependency	= true;
}

//////////////////////////////////////////////////////////////////////////
// Update 
//////////////////////////////////////////////////////////////////////////

void CControlDirection::update_frame()
{
	pitch_correction			();	

	SRotationEventData			event_data;
	event_data.angle			= 0;

	bool heading_similar		= false;
	bool pitch_similar			= false;

	// difference
	float diff = angle_difference(m_pitch.current_angle, m_data.pitch.target_angle) * 4.0f;
	clamp(diff, PI_DIV_6, 5 * PI_DIV_6);

	m_data.pitch.target_speed = m_pitch.current_speed = diff;

	// поправка угловой скорости в соответствии с текущей и таргетовой линейной скоростями
	// heading speed correction
	if (!fis_zero(m_man->movement().velocity_current()) && !fis_zero(m_man->movement().velocity_target()) && m_data.linear_dependency)
		m_heading.current_speed	= m_data.heading.target_speed * m_man->movement().velocity_current() / (m_man->movement().velocity_target() + EPS_L);
	else 
		velocity_lerp			(m_heading.current_speed, m_data.heading.target_speed, m_heading.current_acc, m_object->client_update_fdelta());

	m_heading.current_angle		= angle_normalize(m_heading.current_angle);
	m_data.heading.target_angle	= angle_normalize(m_data.heading.target_angle);
	
	if (fsimilar(m_heading.current_angle, m_data.heading.target_angle)) heading_similar = true;
	angle_lerp(m_heading.current_angle, m_data.heading.target_angle, m_heading.current_speed, m_object->client_update_fdelta());
	if (!heading_similar && fsimilar(m_heading.current_angle, m_data.heading.target_angle)) {
		event_data.angle |= SRotationEventData::eHeading;
	}

	// update pitch
	velocity_lerp				(m_pitch.current_speed, m_data.pitch.target_speed, m_pitch.current_acc, m_object->client_update_fdelta());

	m_pitch.current_angle		= angle_normalize_signed	(m_pitch.current_angle);
	m_data.pitch.target_angle	= angle_normalize_signed	(m_data.pitch.target_angle);

	if (fsimilar(m_pitch.current_angle, m_data.pitch.target_angle)) pitch_similar = true;
	angle_lerp					(m_pitch.current_angle, m_data.pitch.target_angle, m_pitch.current_speed, m_object->client_update_fdelta());
	if (!pitch_similar && fsimilar(m_pitch.current_angle, m_data.pitch.target_angle)) {
		event_data.angle |= SRotationEventData::ePitch;
	}

	// set
	m_man->path_builder().m_body.speed			= m_heading.current_speed;
	m_man->path_builder().m_body.current.yaw	= m_heading.current_angle;
	m_man->path_builder().m_body.target.yaw		= m_heading.current_angle;
	m_man->path_builder().m_body.current.pitch	= m_pitch.current_angle;
	m_man->path_builder().m_body.target.pitch	= m_pitch.current_angle;

	// save object position
	Fvector P					= m_object->Position();
	// set angles
	if(!m_object->animation_movement_controlled())
		m_object->XFORM().setHPB	(-m_man->path_builder().m_body.current.yaw,-m_man->path_builder().m_body.current.pitch,0);
	// restore object position
	m_object->Position()		= P;

	
	// if there is an event
	if (event_data.angle)		m_man->notify(ControlCom::eventRotationEnd, &event_data);
}

void CControlDirection::pitch_correction()
{
	if (!m_object->ability_pitch_correction()) return;

	// extended feature to pitch by path (wall climbing)
	// distance between two travel point must be more than 1.f
	if (m_object->control().path_builder().is_moving_on_path() && 
		(m_object->movement().detail().path().size() > m_object->movement().detail().curr_travel_point_index() + 1)) {
		
		const DetailPathManager::STravelPathPoint	cur_point	= m_object->movement().detail().path()[m_object->movement().detail().curr_travel_point_index()];
		const DetailPathManager::STravelPathPoint	next_point	= m_object->movement().detail().path()[m_object->movement().detail().curr_travel_point_index()+1];
		
		if (cur_point.position.distance_to_sqr(next_point.position) > 1) {
			// получаем искомый вектор направления
			Fvector						target_dir;
			target_dir.sub				(next_point.position,cur_point.position);
			m_data.pitch.target_angle	= -target_dir.getP();
			return;
		}
	}
	
	// get current plane
	u32					node = m_object->ai_location().level_vertex_id();
	Fplane				P;	
	pvDecompress		(P.n,ai().level_graph().vertex(node)->plane());
	P.d					= -P.n.dotproduct(ai().level_graph().vertex_position(node));

	Fvector				position_on_plane;
	P.project			(position_on_plane,m_object->Position());

	// находим проекцию точки, лежащей на векторе текущего направления
	Fvector				dir_point, proj_point;
	dir_point.mad		(position_on_plane, m_object->Direction(), 1.f);
	P.project			(proj_point,dir_point);

	// получаем искомый вектор направления
	Fvector				target_dir;
	target_dir.sub		(proj_point,position_on_plane);

	float				yaw,pitch;
	target_dir.getHP	(yaw,pitch);

	m_data.pitch.target_angle = -pitch;
}

//////////////////////////////////////////////////////////////////////////
// Services
//////////////////////////////////////////////////////////////////////////

bool CControlDirection::is_face_target(const Fvector &position, float eps_angle)
{
	float target_h	= Fvector().sub(position, m_object->Position()).getH();
	float my_h		= m_object->Direction().getH();

	if (angle_difference(target_h,my_h) > eps_angle) return false;

	return true;
}

bool CControlDirection::is_face_target(const CObject *obj, float eps_angle) 
{
	return is_face_target(obj->Position(), eps_angle);
}

bool CControlDirection::is_from_right(const Fvector &position)
{
	float			yaw, pitch;
	Fvector().sub	(position, m_object->Position()).getHP(yaw,pitch);
	yaw				*= -1;

	return (from_right(yaw,m_heading.current_angle));
}
bool CControlDirection::is_from_right(float yaw)
{
	return (from_right(yaw,m_heading.current_angle));
}

bool CControlDirection::is_turning(float eps_angle)
{
	return (!fsimilar(m_heading.current_angle,m_data.heading.target_angle, eps_angle));
}
void CControlDirection::get_heading(float &current, float &target)
{
	current = m_heading.current_angle;
	target	= m_data.heading.target_angle;
}

float CControlDirection::get_heading_current()
{
	return m_heading.current_angle;
}

float CControlDirection::angle_to_target(const Fvector &position)
{
	float		angle = Fvector().sub(position, m_object->Position()).getH();
	angle		*= -1;
	
	return		(angle_normalize(angle));
}
