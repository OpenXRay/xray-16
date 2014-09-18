#ifndef CHIMERA_ATTACK_STATE_INLINE_H_INCLUDED
#define CHIMERA_ATTACK_STATE_INLINE_H_INCLUDED

#include "chimera_attack_state.h"

#include "../../../debug_renderer.h"

#ifdef DEBUG
#define	DEBUG_STATE
#endif

float	const	num_scan_points				=	10;
float	const	scan_angle					=	deg2rad(360.f) / num_scan_points;

template <class Object>
ChimeraAttackState<Object>::ChimeraAttackState (Object *obj) : inherited(obj)
{
}

template <class Object>
float   ChimeraAttackState<Object>::calculate_min_run_distance () const
{
	float	const	cos_half_scan_angle		=	_cos(scan_angle);
	float	const	sin_half_scan_angle		=	_sin(scan_angle);

	float	const	attack_radius			=	object->get_attack_params().attack_radius;

	float	const	h						=	attack_radius * sin_half_scan_angle;
	float	const	min_run_part1			=	attack_radius * cos_half_scan_angle;

	float	const	jump_max_radius			=	object->com_man().get_jump_control()->get_max_distance();

	VERIFY										(h < jump_max_radius);
	float	const	min_run_part2			=	_sqrt(jump_max_radius*jump_max_radius - h*h);

	return										min_run_part1 + min_run_part2;
}

template <class Object>
void   ChimeraAttackState<Object>::initialize ()
{
	inherited::initialize						();
	object->MeleeChecker.init_attack			();
	m_target_vertex							=	(u32)(-1);
	m_allow_jump							=	false;
	m_last_jump_time						=	current_time();
	m_attack_jump							=	false;
	m_min_run_distance						=	calculate_min_run_distance();
	m_capturer								=	object->com_man().get_jump_control();
	m_stealth_end_tick						=	0;
	m_run_side								=	run_side_undefined;
	m_state									=	state_undefined;
	m_state_end_tick						=	0;
	m_num_attack_jumps						=	0;
	m_num_prepare_jumps						=	0;

	R_ASSERT									(m_capturer);
}

template <class Object>
void   ChimeraAttackState<Object>::finalize	()
{
	if ( m_state == state_rotate || m_state == state_prepare_jump )
	{
		if ( object->control().get_capturer(ControlCom::eControlPath) == m_capturer )
		{
			object->control().release			(m_capturer, ControlCom::eControlPath);
		}
		if ( object->control().get_capturer(ControlCom::eControlMovement) == m_capturer )
		{
			object->control().release			(m_capturer, ControlCom::eControlMovement);
		}
		if ( object->anim().has_override_animation() )
		{
			object->anim().clear_override_animation ();
		}
	}
}

template <class Object>
void   ChimeraAttackState<Object>::critical_finalize ()
{
	finalize();
}

template <class Object>
bool   ChimeraAttackState<Object>::check_control_start_conditions (ControlCom::EControlType type)
{
   	if ( type == ControlCom::eControlJump )
   	{
   		return									m_allow_jump;
	}

	return										true;
}

bool   is_valid_point_to_move (Fvector const & point, u32 * out_vertex);

template <class Object>
bool   ChimeraAttackState<Object>::check_if_jump_possible (Fvector const& target)
{
	m_allow_jump							=	true;
	bool const possible						=	object->com_man().check_if_jump_possible(target, true);
	m_allow_jump							=	false;
	if ( !possible )
		return									false;
	
	if ( !ai().level_graph().valid_vertex_position(target) )
		return									false;

	if ( object->com_man().get_jump_control()->jump_intersect_geometry(target, (CObject*)object->EnemyMan.get_enemy()) )
		return									false;

	return										true;
}

template <class Object>
bool   ChimeraAttackState<Object>::jump (Fvector const& target, bool attack_jump)
{
	// получить вектор направления и его мир угол
	float dir_yaw							=	(target - object->Position()).getH();
	dir_yaw									=	angle_normalize(-dir_yaw);
	float yaw_current, yaw_target;
	object->control().direction().get_heading	(yaw_current, yaw_target);
	if ( angle_difference(yaw_current, dir_yaw) > 1.f )
		return									false;

	m_allow_jump							=	true;
	bool const jumped						=	object->com_man().jump_if_possible
												(target, 
												 (CEntityAlive * )object->EnemyMan.get_enemy(), 
												 attack_jump,	// use_direction_to_target
												 true,			// use velocity bounce
												 true);

	
	m_allow_jump							=	false;
	return										jumped;
}

template <class Object>
float   ChimeraAttackState<Object>::get_attack_radius () const
{
	return										object->get_attack_params().attack_radius;
}

template <class Object>
bool   ChimeraAttackState<Object>::select_target_for_move ()
{
	CEntityAlive const* const	enemy		=	object->EnemyMan.get_enemy();
	Fvector	const	enemy_pos				=	enemy->Position();
	Fvector	const	self_pos				=	object->Position();
	
	Fvector	const	self2enemy				=	enemy_pos - self_pos;

	float	const	attack_radius			=	get_attack_radius();

	Fvector	const	enemy_dir				=	normalize( enemy->Direction() );
	Fvector	const	behind_point			=	enemy_pos - (enemy_dir * attack_radius);

 	float	const	dist2attack_pos			=	magnitude(behind_point - self_pos);
 	
 	if ( dist2attack_pos < 5.f )
 	{
 		m_target							=	behind_point;
		return									true;
 	}
	else
	{
		Fvector	const	self2behind			=	behind_point - self_pos;

		if ( m_run_side == run_side_undefined || current_time() > m_run_side_select_tick )
		{
			bool		left_side			=	self2enemy.x*self2behind.z - 
												self2enemy.z*self2behind.x > 0.f;
			if ( !(rand() % 2) )
			{
				left_side					^=	true;
			}

			m_run_side						=	left_side ? run_side_left : run_side_right;
			m_run_side_select_tick			=	current_time() + 4000;
		}

		Fvector const 	enemy2self			=	-attack_radius * normalize(self2enemy) ;

		float	const	move_scan_points	=	8;
		float	const	move_scan_angle		=	deg2rad(360.f) / move_scan_points;

		for (			u32	index			=	0; 
							index			<	move_scan_points; 
						  ++index	)
		{
			float	const	angle			=	move_scan_angle * 
												(index+1) * 
												(m_run_side == run_side_left ? -1.f : +1.f);

			Fvector	const	scan_point		=	enemy_pos + rotate_point(enemy2self, angle);

			if ( ai().level_graph().valid_vertex_position(scan_point) )
			{
				m_target					=	scan_point;
				break;
			}
		}

		return									index < num_scan_points;
	}
}

template <class Object>
Fvector   ChimeraAttackState<Object>::correct_jump_pos (Fvector const& pos)
{
//	return										pos;
	Fvector const		self_pos			=	object->Position();
	Fvector const		self2pos_norm		=	normalize(pos - self_pos);

	return										pos + self2pos_norm * 1.f;	
}

template <class Object>
bool   ChimeraAttackState<Object>::select_target_for_attack_jump ()
{
	CEntityAlive const* const	enemy		=	object->EnemyMan.get_enemy();
	Fvector	const	enemy_pos				=	enemy->Position();

	Fvector const &	corrected_enemy_pos		=	correct_jump_pos(enemy_pos);
	if ( check_if_jump_possible(corrected_enemy_pos) )
	{
		m_target							=	corrected_enemy_pos;
		m_attack_jump						=	true;
		return									true;
	}

	if ( check_if_jump_possible(enemy_pos) )
	{
		m_target							=	enemy_pos;
		m_attack_jump						=	true;
		return									true;
	}

	return										false;
}

template <class Object>
bool   ChimeraAttackState<Object>::select_target_for_jump (enum_action const	action)
{
	CEntityAlive const* const	enemy		=	object->EnemyMan.get_enemy();
	Fvector	const	enemy_pos				=	enemy->Position();

	if ( action == action_attack )
	{
		if ( select_target_for_attack_jump() )
			return								true;
	}

	Fvector	const	self_pos				=	object->Position();
	Fvector	const	self2enemy				=	enemy_pos - self_pos;
	float	const	self2enemy_mag			=	magnitude(self2enemy);

	m_attack_jump							=	false;

	VERIFY										(self2enemy_mag < m_min_run_distance);
	VERIFY										(get_attack_radius() >= 4);
	float	const	attack_radius			=	3 + float(rand() % (u32)(get_attack_radius() - 3) );

	Fvector	const	enemy_dir				=	normalize(enemy->Direction());
	Fvector	const	behind_point			=	enemy_pos - (enemy_dir * attack_radius);

	if ( check_if_jump_possible(behind_point) )
	{
		m_target							=	behind_point;
		return									true;
	}

	bool	left_side_first					=	true;
	for (				u32	index			=	0; 
							index			<	num_scan_points; 
						  ++index	)
	{
		if ( !(index % 2) )
		{
			left_side_first					=	!(rand() % 2);
		}
		
		bool	const	left_side			=	(!(index % 2)) ^ left_side_first;
		float	const	angle				=	scan_angle * ((index / 2) + 1) *  (left_side ? -1.f : +1.f);

		Fvector	const	scan_point			=	enemy_pos + rotate_point(behind_point - enemy_pos, angle);

		if ( magnitude(self_pos - scan_point) >= 7.f &&
			 check_if_jump_possible(scan_point) )
		{
			m_target						=	scan_point;
			return								true;
		}

		++index;				
	}

	return										select_target_for_attack_jump();
}

template <class Object>
void   ChimeraAttackState<Object>::set_turn_animation ()
{
	bool const rotate_right					=	object->control().direction().is_from_right(m_target);
	object->anim().set_override_animation		(rotate_right ? 
												 eAnimFastStandTurnRight : eAnimFastStandTurnLeft, 0);
}

template <class Object>
void   ChimeraAttackState<Object>::execute ()
{
#ifdef DEBUG_STATE
	DBG().get_text_tree().clear					();
	debug::text_tree& text_tree				=	DBG().get_text_tree().find_or_add("ActorView");
#endif // DEBUG_STATE

	CEntityAlive*		enemy				=	const_cast<CEntityAlive*>( object->EnemyMan.get_enemy() );
	Fvector	const		enemy_pos			=	enemy->Position();
	Fvector const		self_pos			=	object->Position();
	Fvector	const		self2enemy			=	enemy_pos - self_pos;
	float	const		self2enemy_mag		=	magnitude(self2enemy);
	Fvector const		self2enemy_norm		=	normalize(self2enemy);

	u32		const		max_attack_jumps	=	object->get_attack_params().num_attack_jumps;
	u32		const		max_prepare_jumps	=	object->get_attack_params().num_prepare_jumps;

	Fvector	const		enemy_dir			=	normalize(enemy->Direction());
	float				behind_angle_cos	=	dotproduct(self2enemy_norm, enemy_dir);
	clamp										(behind_angle_cos, -1.f, +1.f);
	bool  	const		behind_enemy		=	acosf(behind_angle_cos) < deg2rad(30.f);
	bool	const		can_attack_from_behind	=	behind_enemy && self2enemy_mag >= 6;

	
	bool	const		preparing_state		=	(m_num_attack_jumps == max_attack_jumps);

	bool	const		can_prepare_jump	=	preparing_state &&
												current_time() > m_last_jump_time + object->get_attack_params().prepare_jump_timeout;
	bool	const		can_attack_jump		=	!preparing_state &&
												current_time() > m_last_jump_time + object->get_attack_params().attack_jump_timeout;

	bool	do_move							=	false;

	if ( m_state == state_rotate )
	{
		m_target							=	m_jump_target;
		if ( m_attack_jump )
		{
			m_target						=	correct_jump_pos(enemy_pos);
			if ( !ai().level_graph().valid_vertex_position(m_target) )
			{
				m_target_vertex				=	enemy->ai_location().level_vertex_id();
				m_target					=	ai().level_graph().vertex_position(m_target_vertex);
			}
			else
			{
				m_target_vertex				=	ai().level_graph().vertex_id(m_target);
			}
		}

		Fvector	const	self2target_norm	=	normalize(m_target - self_pos);
		Fvector	const	self_dir			=	normalize(object->Direction());

		float	const	self2target_yaw		=	self2target_norm.getH();
		float	const	self_dir_yaw		=	self_dir.getH();
		
		bool  	const 	good_aiming			=	_abs(self2target_yaw - self_dir_yaw) < deg2rad(20.f);
		bool	const	in_stealth			=	current_time() < m_stealth_end_tick && 
												!object->EnemyMan.enemy_see_me_now ();
		if ( !in_stealth )
			m_stealth_end_tick				=	0;

		if ( object->anim().has_override_animation() )
			object->anim().clear_override_animation();

		if ( !good_aiming || in_stealth )
		{
			if ( good_aiming && in_stealth )
				object->anim().set_override_animation	(eAnimPrepareAttack, 0);
			else
				set_turn_animation				();

			object->dir().face_target			(m_target);
		}
		else
		{
			object->control().release			(m_capturer, ControlCom::eControlPath);
			object->control().release			(m_capturer, ControlCom::eControlMovement);

			bool	valid_target			=	check_if_jump_possible(m_target);

			if ( !valid_target && m_attack_jump )
			{
				m_target					=	m_jump_target;
				m_target_vertex				=	ai().level_graph().vertex_id(m_target);
				valid_target				=	check_if_jump_possible(m_target);
			}

			if ( valid_target )
			{
				m_state						=	state_prepare_jump;
				float		length			=	object->anim().get_animation_length(eAnimUpperAttack, 0);
				m_state_end_tick			=	current_time() + u32(length * 1000);
			}
			else
			{
				m_state						=	state_undefined;
			}

			m_run_side						=	run_side_undefined;
			m_stealth_end_tick				=	0;
		}
	}
	else if ( m_state == state_prepare_jump )
	{
		if ( object->anim().has_override_animation() )
		{
			object->anim().clear_override_animation();
		}

		if ( current_time() > m_state_end_tick )
		{
			if ( jump(m_target, m_attack_jump) )
			{
				m_last_jump_time			=	current_time();
				if ( m_attack_jump )
				{
					++m_num_attack_jumps;
				}
				else if ( preparing_state )
				{
					++m_num_prepare_jumps;
					if ( m_num_prepare_jumps == max_prepare_jumps )
					{
						m_num_attack_jumps	=	0;
						m_num_prepare_jumps	=	0;
					}
				}
			}

			m_target_vertex					=	enemy->ai_location().level_vertex_id();
			m_target						=	ai().level_graph().vertex_position(m_target_vertex);

			m_state							=	state_undefined;
		}
		else
		{
			object->anim().set_override_animation	(eAnimUpperAttack, 0);
		}
	}
	else if ( object->is_jumping() )
	{

	}
	else if ( self2enemy_mag >= m_min_run_distance )
	{
 		m_target_vertex						= 	enemy->ai_location().level_vertex_id();
		m_target							=	ai().level_graph().vertex_position(m_target_vertex);
	}
	else if ( can_prepare_jump || can_attack_jump )
	{
		if ( select_target_for_jump(can_attack_jump ? action_attack : action_jump) )
		{
			m_target_vertex					=	ai().level_graph().vertex_id(m_target);
			m_jump_target					=	m_target;
			m_state							=	state_rotate;

			if ( can_attack_from_behind )
			{
				m_stealth_end_tick			=	current_time() + object->get_attack_params().stealth_timeout;
			}
	
			object->control().capture			(m_capturer, ControlCom::eControlPath);
			object->control().capture			(m_capturer, ControlCom::eControlMovement);
			object->control().path_stop			(m_capturer);
			object->control().move_stop			(m_capturer);

			set_turn_animation					();
		}
		else
		{
			do_move							=	true;
		}
	}
	else
	{
		do_move								=	true;
	}

	if ( do_move )
	{
		if ( select_target_for_move() )
		{
			m_target_vertex					=	ai().level_graph().vertex_id(m_target);
		}
		else
		{
			m_target_vertex					=	enemy->ai_location().level_vertex_id();
			m_target						=	ai().level_graph().vertex_position(m_target_vertex);
		}
	}

	#ifdef DEBUG_STATE
	text_tree.add_line						("num_prepare_jumps", m_num_prepare_jumps);
	text_tree.add_line						("num_attack_jumps", m_num_attack_jumps);
	#endif // #ifdef DEBUG_STATE

	object->set_action							(ACT_RUN);
	object->path().set_use_dest_orient			(false);
	object->path().set_try_min_time				(false);
	object->anim().accel_activate				(eAT_Aggressive);
	object->anim().accel_set_braking			(false);
	object->path().set_rebuild_time				(250);
	object->path().extrapolate_path				(true);
	object->path().set_target_point				(m_target, m_target_vertex);
}

#endif // #ifdef CHIMERA_ATTACK_STATE_INLINE_H_INCLUDED