#ifndef MONSTER_STATE_ATTACK_ON_RUN_INLINE_H
#define MONSTER_STATE_ATTACK_ON_RUN_INLINE_H

#include "../monster_velocity_space.h"

#include "../../../../xrCore/_vector3d_ext.h"
#include "../../../level_graph.h"

#define TEMPLATE_SIGNATURE template <typename _Object>

#define ATTACK_ON_RUN_STATE CStateMonsterAttackOnRun<_Object>

#ifdef DEBUG
#define	DEBUG_STATE
#endif

namespace detail {
namespace monsters {
		
	TTime const	max_go_far_time		=	6000;
	TTime const	update_side_period	=	5000;	
	
} // namespace monsters
} // namespace detail

TEMPLATE_SIGNATURE
ATTACK_ON_RUN_STATE::CStateMonsterAttackOnRun (_Object *obj) : inherited(obj) 
{
}

TEMPLATE_SIGNATURE
void ATTACK_ON_RUN_STATE::initialize()
{
	inherited::initialize					();	
	object->m_time_last_attack_success	=	0;
	object->path().prepare_builder			();

	m_target_vertex						=	(u32)(-1);
	m_attacking							=	false;
	
	m_attack_side							=	!(rand()%2) ? left : right;
	m_prepare_side_chosen_time				=	0;

	m_try_min_time_chosen_time			=	0;
	m_try_min_time_period				=	0; 
	m_can_do_rotation_jump				=	!(rand() % 2);

	m_predicted_enemy_velocity			=	cr_fvector3(0.f);
	m_last_prediction_time				=	0;
	m_last_update_time					=	0;

	set_movement_phaze						(go_close);
	m_attack_end_time					=	0;
	
	m_is_jumping						=	object->is_jumping();
	m_reach_old_target					=	false;
	m_attack_side_chosen_time			=	0;

	choose_next_atack_animation				();
}

TEMPLATE_SIGNATURE
void   ATTACK_ON_RUN_STATE::choose_next_atack_animation ()
{
	EMotionAnim animation_types[2]		=	{ eAnimAttackOnRunLeft, eAnimAttackOnRunRight };

	for ( int i=0; i<2; ++i )
	{
		u32 num_animations				=	object->anim().get_animation_variants_count(animation_types[i]);
		VERIFY								(num_animations > 0);
	 	m_animation_index[i]			=	rand() % num_animations;
  		m_animation_hit_time[i]			=	object->anim().get_animation_hit_time
 											(animation_types[i], m_animation_index[i]);
	}
}

TEMPLATE_SIGNATURE
bool   ATTACK_ON_RUN_STATE::check_control_start_conditions	(ControlCom::EControlType type)
{
	if ( type == ControlCom::eAntiAim )
	{
		return								(m_phaze == go_close) && !m_attacking;
	}

	if ( type == ControlCom::eControlRotationJump )
	{
		CEntityAlive const* const enemy	=	object->EnemyMan.get_enemy();
		Fvector const   enemy_pos		=	enemy->Position();
		float const		self2enemy_mag	=	enemy_pos.distance_to(object->Position());

		return								m_phaze == go_close &&
											m_can_do_rotation_jump && 
											self2enemy_mag < 10.f;
	}

	return true;
}

#include "../../../level_debug.h"
#include "../../../debug_text_tree.h"

TEMPLATE_SIGNATURE
void   ATTACK_ON_RUN_STATE::set_movement_phaze (phaze const new_phaze)
{
	m_phaze								=	new_phaze;
	m_phaze_chosen_time					=	current_time();
	m_prepare_side_chosen_time			=	current_time();

	if ( m_phaze == go_close )
	{
		m_attack_side_chosen_time		=	0; // recalc attack_side
	}
	else if ( m_phaze == go_prepare )
	{
		m_go_far_start_point			=	object->Position();
		m_can_do_rotation_jump			=	!(rand() % 2);
		CEntityAlive const* const enemy	=	object->EnemyMan.get_enemy();
		Fvector const   enemy_pos		=	enemy->Position();
		Fvector const   self2enemy		=	enemy_pos - object->Position();
		Fvector const   self_dir		=	object->Direction();

		bool		left_side			=	((self2enemy.x * self_dir.z) - (self2enemy.z * self_dir.x)) > 0.f;
		m_prepare_side					=	left_side ? left : right;
//		m_prepare_side					=   (rand() % 2) ? left : right;
	}
}

TEMPLATE_SIGNATURE
void   ATTACK_ON_RUN_STATE::calculate_predicted_enemy_pos ()
{
	float const		prediction_factor		=	object->get_attack_on_move_prediction_factor();

	float const		epsilon					=	0.0001f;
	CEntityAlive const* const enemy			=	object->EnemyMan.get_enemy();
	Fvector const   enemy_pos				=	enemy->Position();
	float const		self2enemy_mag			=	magnitude(enemy_pos - object->Position());
	float const		far_radius				=	object->get_attack_on_move_far_radius();

	if ( self2enemy_mag > far_radius*2 )
	{
		m_predicted_enemy_pos				=	enemy_pos;
		return;
	}

	float const		self_velocity			=	object->movement().speed();
	float const		self2enemy_time			=	self_velocity > epsilon ? 
												self2enemy_mag / self_velocity : 0;

	float const		predictiton_delta_sec	=	(current_time() - m_last_prediction_time) / 1000.f;
	if ( predictiton_delta_sec > 1.f )
	{
		if ( m_last_prediction_time != 0 )
		{
			if ( predictiton_delta_sec < 2.f )
			{
				Fvector	const move_delta	=	enemy_pos - m_last_update_enemy_pos;
				m_predicted_enemy_velocity	=	move_delta / predictiton_delta_sec;
			}
			else
			{
				m_predicted_enemy_velocity	=	cr_fvector3(0.f);
			}
		}

		m_last_prediction_time				=	current_time();
		m_last_update_enemy_pos				=	enemy_pos;
	}

	m_predicted_enemy_pos					=	enemy_pos + 
												m_predicted_enemy_velocity*self2enemy_time*prediction_factor;

	if ( magnitude(m_predicted_enemy_pos - object->Position()) < 0.01f )
	{
		m_predicted_enemy_pos				=	enemy_pos;

		if ( magnitude(m_predicted_enemy_pos - object->Position()) < 0.01f )
		{
			m_predicted_enemy_pos.x			+=	1.f;
		}
	}
}

TEMPLATE_SIGNATURE
void   ATTACK_ON_RUN_STATE::update_aim_side ()
{
	CEntityAlive const * const enemy		=	m_attacking ? m_enemy_to_attack : object->EnemyMan.get_enemy();
	
	Fvector const self_dir					=	object->Direction();
	Fvector const self_to_enemy				=	enemy->Position() - object->Position();

	aim_side const	new_attack_side			=	(self_dir.x*self_to_enemy.z - self_dir.z*self_to_enemy.x > 0) ?
												right : left;
	
	TTime const update_side_period			=	(TTime)(object->get_attack_on_move_update_side_period() * 1000);

	if ( current_time() > m_attack_side_chosen_time + update_side_period )
	{
		if ( m_attack_side == new_attack_side )
			m_attack_side					=	(new_attack_side == left) ? right : left;
		else
			m_attack_side					=	new_attack_side;

		m_attack_side_chosen_time			=	current_time();
	}

	if ( m_attacking )
	{
		return;
	}
}

inline
bool   is_valid_point_to_move (Fvector const & point, u32 * out_vertex)
{
	if ( !ai().level_graph().valid_vertex_position(point) )
	{
		return									false;
	}

	CLevelGraph::CPosition vertex_pos		=	ai().level_graph().vertex_position(point);
	CLevelGraph::CVertex	* B 			= 	ai().level_graph().vertices();
	CLevelGraph::CVertex	* E 			= 	B + ai().level_graph().header().vertex_count();
	CLevelGraph::CVertex	* I 			= 	std::lower_bound(B, E, vertex_pos.xz());

	for ( ;(I != E) && ((*I).position().xz() == vertex_pos.xz()); ++I )
	{
		if ( abs(ai().level_graph().vertex_plane_y(*I) - point.y) < 4.f )
		{
			if ( out_vertex )
			{
				*out_vertex					=	ai().level_graph().vertex_id(I);
			}
			return								true;
		}
	}

	return										false;
}

TEMPLATE_SIGNATURE
void   ATTACK_ON_RUN_STATE::update_movement_target ()
{
#ifdef DEBUG_STATE
	DBG().get_text_tree().clear					();
	debug::text_tree& text_tree				=	DBG().get_text_tree().find_or_add("ActorView");
	text_tree.add_line							("attacking", m_attacking);
#endif // DEBUG_STATE

	TTime const max_go_close_time			=	(TTime)(1000*object->get_attack_on_move_max_go_close_time());
	float const	far_radius					=	object->get_attack_on_move_far_radius();
	float const	attack_radius				=	object->get_attack_on_move_attack_radius();
	float const	prepare_time				=	object->get_attack_on_move_prepare_time();
	
	CEntityAlive const* const enemy			=	object->EnemyMan.get_enemy();
	Fvector	const		enemy_pos			=	enemy->Position();
	Fvector const		self_pos			=	object->Position();
	Fvector	const		self2enemy			=	enemy_pos - self_pos;
	float const			self2enemy_mag		=	self2enemy.magnitude();
	
	if ( self2enemy_mag > far_radius*2 )
	{
		m_target_vertex						=	enemy->ai_location().level_vertex_id();
		m_target							=	ai().level_graph().vertex_position(m_target_vertex);
		m_predicted_enemy_pos				=	m_target;
		return;
	}

	Fvector	const		self_dir			=	Fvector(object->Direction()).normalize();

	Fvector				self2target;

	Fvector const		self2predicted		=	m_predicted_enemy_pos - self_pos;
	float	const		self2predicted_mag	=	self2predicted.magnitude();

	if ( m_phaze == go_prepare )
	{
		if ( current_time() > m_phaze_chosen_time + prepare_time*1000 )
		{
			set_movement_phaze					(go_close);
		}
		else if ( self2predicted_mag < 3.f && current_time() > m_phaze_chosen_time + 3000  )
		{
			set_movement_phaze					(go_close);			
		}
		else if ( m_go_far_start_point.distance_to(self_pos) > (far_radius * 2) )
		{
			set_movement_phaze					(go_close);
		}
		else if ( self2enemy_mag > far_radius + 3 )
		{
			set_movement_phaze					(go_close);
		}
	}
	else if ( m_phaze == go_close )
	{
		if ( angle_between_vectors(object->Direction(), self2enemy) > deg2rad(140.f) &&
			 self2predicted_mag < 4.f &&
			 current_time() > m_phaze_chosen_time + 3000 )
		{
			set_movement_phaze					(go_prepare);
		}

		if ( current_time() - m_phaze_chosen_time > max_go_close_time )
		{
			set_movement_phaze					(go_prepare);
		}
	}

#ifdef DEBUG_STATE
	text_tree.add_line							("phaze", m_phaze == go_prepare ? "preparation" : "close");
	text_tree.add_line							("reach_old_target", m_reach_old_target);
#endif // DEBUG_STATE

	if ( m_reach_old_target )
	{
		self2target							=	m_target - self_pos;
		if ( magnitude(m_target - self_pos) < 1.f || current_time() > m_reach_old_target_start_time + 1000 )
		{
			m_reach_old_target				=	false;
			set_movement_phaze					(go_prepare);
		}
	}
	else if ( m_phaze == go_prepare )
	{
		float const offs_length				=	5.f;
		float const offs_angle				=	_max(deg2rad(30.f), offs_length/far_radius) * 
												(m_prepare_side == left ? -1 : +1);
		
		float const cos_alpha				=	_cos(offs_angle);
		float const sin_alpha				=	_sin(offs_angle);

		Fvector const predicted2self		=	-self2predicted;

		Fvector const predicted2part		=	
						cr_fvector3(predicted2self.x*cos_alpha - predicted2self.z*sin_alpha, 
									0,
									predicted2self.x*sin_alpha + predicted2self.z*cos_alpha);

		Fvector const predicted2target		=	normalize(predicted2part) * far_radius;
		self2target							=	self2predicted + predicted2target;
	}
	else if ( self2predicted_mag > attack_radius )
	{
		// 90 deg triangle: (self, enemy, atack-point)
		float const dist2atack_point	=	_sqrt( self2predicted_mag*self2predicted_mag - 
													   attack_radius*attack_radius );

		// alpha is the angle between (self, enemy) and (self, atack-point)
		float const cos_alpha			=	dist2atack_point / self2predicted_mag;
		float const sin_alpha			=	(attack_radius / self2predicted_mag)
															*
											(m_attack_side == right ? -1.f : 1.f);

		Fvector const self2atack_point	=	
									cr_fvector3(self2predicted.x*cos_alpha - self2predicted.z*sin_alpha, 
												0,
												self2predicted.x*sin_alpha + self2predicted.z*cos_alpha);

		float const self2atack_mag		=	self2atack_point.magnitude();
		float const attack2target_mag	=	3;
			
		float self2target_mag			=	self2atack_mag + attack2target_mag;
		self2target						=	normalize(self2atack_point)*self2target_mag;
	}
	else
	{
		Fvector  dir2target					=	Fvector().crossproduct
												(self2predicted, cr_fvector3(0,1,0)).normalize();
#ifdef DEBUG_STATE
		text_tree.add_line						("type", 1);
#endif // DEBUG_STATE
		if ( self_dir.dotproduct(dir2target) < 0 )
		{
			dir2target.invert();
		}

		float const	self2target_mag			=	_sqrt(far_radius*far_radius - 
													  self2predicted_mag*self2predicted_mag);
		self2target							=	normalize(dir2target)*self2target_mag;
	}

	m_target								=	self_pos + self2target;

	u32 const enemy_vertex					=	enemy->ai_location().level_vertex_id();
	Fvector const enemy_vertex_pos			=	ai().level_graph().vertex_position(enemy_vertex);
	u32 const target_vertex					=	ai().level_graph().check_position_in_direction(enemy_vertex, 
																							   enemy_vertex_pos, 
																							   m_target);

	bool const target_to_enemy_clear		=	ai().level_graph().valid_vertex_id(target_vertex);

	if ( (m_phaze == go_close && !target_to_enemy_clear) || !is_valid_point_to_move(m_target, &m_target_vertex) )
	{
		if ( m_phaze == go_close )
		{
			m_target_vertex					=	enemy->ai_location().level_vertex_id();
			m_target						=	ai().level_graph().vertex_position(m_target_vertex);
			m_predicted_enemy_pos			=	m_target;

			if ( object->ai_location().level_vertex_id() == m_target_vertex )
				set_movement_phaze(go_prepare);
		}
		else
		{
			select_prepare_fallback_target		();			
		}
	}
}

TEMPLATE_SIGNATURE
void   ATTACK_ON_RUN_STATE::select_prepare_fallback_target ()
{
	float const	far_radius					=	object->get_attack_on_move_far_radius();
	CEntityAlive const* const	enemy		=	object->EnemyMan.get_enemy();
	Fvector	const	enemy_pos				=	enemy->Position();

	float	const	move_scan_points		=	8;
	float	const	move_scan_angle			=	deg2rad(360.f) / move_scan_points;

	for (			u32	index				=	0; 
						index				<	move_scan_points; 
					  ++index	)
	{
		float	const	angle				=	move_scan_angle * index;
		Fvector	const	scan_point			=	enemy_pos + rotate_point(Fvector().set(far_radius, 0, 0), angle);

		if ( is_valid_point_to_move(scan_point, &m_target_vertex) )
		{
			m_target						=	scan_point;
			return;
		}
	}

	m_target_vertex							=	enemy->ai_location().level_vertex_id();
	m_target								=	ai().level_graph().vertex_position(m_target_vertex);
}

#include "../../../ai_debug_variables.h"

TEMPLATE_SIGNATURE
void   ATTACK_ON_RUN_STATE::update_try_min_time ()
{
	if ( current_time() > m_try_min_time_chosen_time + m_try_min_time_period )
 	{
 		m_try_min_time_chosen_time			=	current_time();
 		m_try_min_time_period				=	3000 + (rand() % 3000);
 		m_try_min_time						=	!(rand() % 2);
 	}
}

TEMPLATE_SIGNATURE
void   ATTACK_ON_RUN_STATE::update_attack ()
{
	if ( m_attacking )
	{
		if ( current_time() > m_attack_end_time )
 		{
			choose_next_atack_animation			();
			m_attacking						=	false;
			
			//set_movement_phaze				(go_prepare);

			EMotionAnim override_animation	=	object->anim().get_override_animation();
			if ( override_animation == eAnimAttackOnRunLeft || 
				 override_animation == eAnimAttackOnRunRight )
			{
				object->anim().clear_override_animation	();
			}
 		}
	}
	else if ( !m_is_jumping && !object->anim().has_override_animation() && m_phaze == go_close )
	{	
		ENEMIES_MAP const& memory			=	object->EnemyMemory.get_memory();
		CEntityAlive const* const main_enemy	=	object->EnemyMan.get_enemy();
		m_enemy_to_attack					=	main_enemy;

		bool can_attack						=	false;
		for ( ENEMIES_MAP::const_iterator it	=	memory.begin();
										  it	!=	memory.end();
										++it )
		{
			CEntityAlive const* const enemy	=	it->first;
			Fvector const enemy_pos			=	enemy == main_enemy ? 
												m_predicted_enemy_pos : enemy->Position();

#ifdef DEBUG_STATE
			debug::text_tree& text_tree		=	DBG().get_text_tree().find_or_add("ActorView");
#endif // #ifdef DEBUG_STATE

			float		velocity			=	object->movement().speed();
			Fvector self_to_enemy_xz		=	enemy_pos - object->Position();
			self_to_enemy_xz.y				=	0;
			float const current_atack_dist	=	magnitude(self_to_enemy_xz);

			float const prepare_atack_dist	=	m_animation_hit_time[m_attack_side] * velocity;
			float const melee_max_distance	=	object->MeleeChecker.get_max_distance();
			float const melee_min_distance	=	object->MeleeChecker.get_min_distance();

			float const allowed_atack_distance		=	(prepare_atack_dist + melee_max_distance) * 0.9f ;
			float const disallowed_atack_distance	=	(prepare_atack_dist + melee_min_distance * 0.5f) * 0.9f ;

			Fvector self_direction_xz		=	object->Direction();
			self_direction_xz.y				=	0;
			float const attack_angle		=	angle_between_vectors(self_to_enemy_xz, self_direction_xz);

			bool const good_attack_angle	=	attack_angle < deg2rad(30.f);

			//bool const see_enemy_now		=	object->EnemyMan.see_enemy_now(enemy);
			bool const good_attack_dist		=	current_atack_dist < allowed_atack_distance &&
												current_atack_dist > disallowed_atack_distance;

			if ( current_atack_dist < disallowed_atack_distance && 
				 m_phaze == go_close && 
				 current_time() > m_phaze_chosen_time + 3000 &&
				 enemy == main_enemy )
			{
				set_movement_phaze				(go_prepare);
			}

#ifdef DEBUG_STATE
			text_tree.add_line					("good_attack_angle", good_attack_angle);
			//text_tree.add_line					("see_enemy_now", see_enemy_now);
			text_tree.add_line					("good_attack_dist", good_attack_dist);
#endif // #ifdef DEBUG_STATE

			if ( good_attack_angle && good_attack_dist )
			{
				can_attack					=	true;
				m_enemy_to_attack			=	enemy;						
				break;
			}
		}		

		if ( can_attack )
		{
			object->on_attack_on_run_hit		();
			m_attacking						=	true;
			m_reach_old_target				=	true;
			m_reach_old_target_start_time	=	current_time();

			update_aim_side						();

			float attack_animation_length	=	0;
			MotionID motion;
			EMotionAnim const anim			=	m_attack_side == left ? 
												eAnimAttackOnRunLeft : eAnimAttackOnRunRight;
			bool const got_animation_info	=	object->anim().get_animation_info
												(anim,
												 m_animation_index[m_attack_side], 
												 motion, 
												 attack_animation_length);
					 
			got_animation_info; VERIFY			(got_animation_info);

			m_attack_end_time				=	current_time() + TTime(1000*attack_animation_length);
			object->anim().set_override_animation (anim, m_animation_index[m_attack_side]);
		}
	}

	if ( m_is_jumping && !object->is_jumping() )
	{
		set_movement_phaze						(go_prepare);
	}

	m_is_jumping							=	object->is_jumping();
}

TEMPLATE_SIGNATURE
void   ATTACK_ON_RUN_STATE::execute ()
{
	calculate_predicted_enemy_pos				();
	update_movement_target						();
	update_try_min_time							();
	update_attack								();
	update_aim_side								();

	object->set_action							(ACT_RUN);

	object->anim().accel_activate				(eAT_Aggressive);
	object->anim().accel_set_braking			(false);

	object->path().set_target_point				(m_target, m_target_vertex);
	object->path().set_rebuild_time				(m_attacking ? 20 : 150);//object->get_attack_rebuild_time());

	object->path().set_use_covers				();
	object->path().set_cover_params				(0.1f, 30.f, 1.f, 30.f);
	
	object->path().set_try_min_time				(m_phaze == go_close);

	object->set_state_sound						(MonsterSound::eMonsterSoundAggressive);
	object->path().extrapolate_path				(true);
	
	// обработать squad инфо	
	object->path().set_use_dest_orient			(false);

// 	CMonsterSquad *squad	= monster_squad().get_squad(object);
// 	if (squad && squad->SquadActive())
// 	{
// 		// Получить команду
// 		SSquadCommand command;
// 		squad->GetCommand(object, command);
// 		if (command.type == SC_ATTACK)
// 		{
// 			object->path().set_use_dest_orient	(true);
// 			object->path().set_dest_direction	(command.direction);
// 		}
// 	}
}

TEMPLATE_SIGNATURE
void ATTACK_ON_RUN_STATE::finalize()
{
	object->anim().set_override_animation		();
	inherited::finalize							();
}

TEMPLATE_SIGNATURE
void ATTACK_ON_RUN_STATE::critical_finalize()
{
	object->anim().set_override_animation		();
	inherited::critical_finalize				();
}

TEMPLATE_SIGNATURE
bool ATTACK_ON_RUN_STATE::check_start_conditions()
{
	float dist	=	object->MeleeChecker.distance_to_enemy	(object->EnemyMan.get_enemy());
	
	if ( dist > object->db().m_run_attack_start_dist )
	{
		return									false;
	}
	if ( dist < object->MeleeChecker.get_min_distance() )
	{
		return									false;
	}
	
	// check angle
	if ( !object->control().direction().is_face_target(object->EnemyMan.get_enemy(), deg(30)) )
	{
		return									false;
	}

	return										true;
}

TEMPLATE_SIGNATURE
bool ATTACK_ON_RUN_STATE::check_completion()
{
	//if (!object->control().path_builder().is_moving_on_path() || 
	//	(object->m_time_last_attack_success != 0)) return true;
	return										false;
}


#undef DEBUG_STATE

#undef TEMPLATE_SIGNATURE
#undef ATTACK_ON_RUN_STATE

#endif // MONSTER_STATE_ATTACK_ON_RUN_INLINE_H
