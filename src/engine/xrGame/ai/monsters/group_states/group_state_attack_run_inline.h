#pragma once

#include "../ai_monster_squad.h"
#include "../ai_monster_squad_manager.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateGroupAttackRunAbstract CStateGroupAttackRun<_Object>

TEMPLATE_SPECIALIZATION
CStateGroupAttackRunAbstract::CStateGroupAttackRun	(_Object *obj) : inherited(obj) 
{
	m_next_encircle_tick = 0;
}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackRunAbstract::initialize()
{
	inherited::initialize();
	object->path().prepare_builder	();	

	// interception
	m_intercept_tick = Device.dwTimeGlobal;
	m_intercept.setHP(::Random.randF(M_PI*2.f), 0);
	m_intercept.normalize_safe();
	if ( !m_intercept.magnitude() )
	{
		m_intercept.set(0.f, 0.f, 1.f);
	}

	m_intercept_length = 3000 + rand()%4000;

	// prediction
	m_memorized_tick = Device.dwTimeGlobal;
	m_memorized_pos  = object->EnemyMan.get_enemy()->Position();
	m_predicted_vel  = cr_fvector3(0.f);

	// encirclement
	if ( Device.dwTimeGlobal > m_next_encircle_tick )
	{
		m_encircle_time = 2000 + rand()%4000;
		m_next_encircle_tick = Device.dwTimeGlobal + 8000 + rand()%8000;
	}
	else
	{
		m_encircle_time = 0;	
	}
	
	m_encircle_dir  = m_intercept;
	CMonsterSquad *squad = monster_squad().get_squad(object);
	if ( squad && squad->SquadActive() )
	{
		SSquadCommand command;
		squad->GetCommand(object, command);
		if ( command.type == SC_ATTACK )
		{
			m_encircle_dir = command.direction;
		}
	}
	m_encircle_dir.normalize_safe();
	if ( !m_encircle_dir.magnitude() )
	{
		m_encircle_dir.set(0.f, 0.f, 1.f);
	}
}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackRunAbstract::execute()
{	
 	if ( Device.dwTimeGlobal > m_intercept_tick + m_intercept_length )
 	{
 		m_intercept_tick   = Device.dwTimeGlobal;
 		m_intercept_length = 2000 + (rand()%4000);
 		m_intercept.setHP(::Random.randF(M_PI*2.f), 0);
 		m_intercept.normalize_safe();
		if ( !magnitude(m_intercept) )
		{
			m_intercept.set(0.f, 0.f, 1.f);
		}
 	}
 
 	const Fvector enemy_pos = object->EnemyMan.get_enemy()->Position();
 	const int     memory_update_ms = 250;
 
 	if ( Device.dwTimeGlobal > m_memorized_tick + memory_update_ms )
 	{
 		m_predicted_vel  = (enemy_pos-m_memorized_pos) * (1000.f / (Device.dwTimeGlobal-m_memorized_tick));
		m_predicted_vel.clamp( Fvector().set(10,10,10) );

 		m_memorized_tick = Device.dwTimeGlobal;
 		m_memorized_pos  = enemy_pos;
 	}
 
 	const SVelocityParam  velocity_run = object->move().get_velocity
 		                                 (MonsterMovement::eVelocityParameterRunNormal);
 
 	const float   self_vel  = velocity_run.velocity.linear;
 	const Fvector self_pos  = object->Position();
 
 	const Fvector self2enemy = enemy_pos - self_pos;
 	const float   self2enemy_dist = magnitude(self2enemy);
 
 	const float   epsilon = 0.001f;

 	float prediction_time = 0;
 	if ( self_vel > epsilon )
 	{
 		prediction_time = self2enemy_dist / self_vel;
 		const int max_prediction_time = 5000;
 		if ( prediction_time > max_prediction_time )
 		{
 			prediction_time = max_prediction_time;
 		}
 	}

	// classify if predicted enemy going to the left or right (viewed from our eyes)
 	const Fvector linear_prediction = enemy_pos + m_predicted_vel*prediction_time;
 	const Fvector self2linear       = linear_prediction - self_pos;
 	const bool    predicted_left    = crossproduct(self2linear, self2enemy).z > 0;
 
 	float h_angle, p_angle;
 	self2enemy.getHP(h_angle, p_angle);
 	
	const float angle = self2enemy_dist > epsilon ? 
						0.5f*prediction_time*magnitude(m_predicted_vel) / self2enemy_dist : 0;
 
 	h_angle = angle_normalize( h_angle + (predicted_left ? +1 : -1)*angle );
 
 	const Fvector radial_prediction = self_pos + 
 		                              Fvector().setHP(h_angle, p_angle).normalize_safe()*self2enemy_dist;
 
 	Fvector   target	 = radial_prediction + m_intercept*self2enemy_dist*0.5f;

	const u32 old_vertex = object->ai_location().level_vertex_id();
	
	u32       vertex	 = ai().level_graph().check_position_in_direction(old_vertex, self_pos, target);

 	if ( !ai().level_graph().valid_vertex_id(vertex) )
	{
 		target = enemy_pos;
 		vertex = object->EnemyMan.get_enemy()->ai_location().level_vertex_id();
	}
	
	// установка параметров функциональных блоков
	object->set_action (ACT_RUN);

	object->anim().accel_activate			(eAT_Aggressive);
	object->anim().accel_set_braking		(false);
	object->path().set_target_point			(target, vertex);
	object->path().set_rebuild_time			(object->get_attack_rebuild_time());
	object->path().set_use_covers			(false);
	//object->path().set_cover_params			(0.1f, 30.f, 1.f, 30.f);
	object->set_state_sound					(MonsterSound::eMonsterSoundAggressive);

	object->path().extrapolate_path			(true);

	object->path().set_try_min_time         (self2enemy_dist >= 5.f);

	const bool enemy_at_home = object->Home->at_home(enemy_pos);

	if ( !enemy_at_home || Device.dwTimeGlobal < time_state_started + m_encircle_time )
	{
 		object->path().set_use_dest_orient	(true);
  		object->path().set_dest_direction	(m_encircle_dir);
		object->path().set_try_min_time		(false);
 	}
	else
	{
		object->path().set_use_dest_orient  (false);
	}
}

// TEMPLATE_SPECIALIZATION
// void CStateGroupAttackRunAbstract::execute()
// {
// 	// установка параметров функциональных блоков
// 	object->set_action						(ACT_RUN);
// 	object->anim().accel_activate			(eAT_Aggressive);
// 	object->anim().accel_set_braking		(false);
// 	object->path().set_target_point			(object->EnemyMan.get_enemy_position(), object->EnemyMan.get_enemy_vertex());
// 	object->path().set_rebuild_time			(object->get_attack_rebuild_time());
// 	object->path().set_use_covers			();
// 	object->path().set_cover_params			(0.1f, 30.f, 1.f, 30.f);
// 	object->path().set_try_min_time			(false);
// 	object->set_state_sound					(MonsterSound::eMonsterSoundAggressive);
// 
// 	object->path().extrapolate_path			(true);
// 
// 	// обработать squad инфо	
// 	object->path().set_use_dest_orient		(false);
// 
// 
// 	//	if ( g_bDogsDirMode )
// 	if ( Device.dwTimeGlobal-time_state_started < m_max_encircle_time )
// 	{
//  		CMonsterSquad *squad	= monster_squad().get_squad(object);
//  		if (squad && squad->SquadActive()) {
//  			// Получить команду
//  			SSquadCommand command;
//  			squad->GetCommand(object, command);
//  			
//  			if (command.type == SC_ATTACK) {
//   				object->path().set_use_dest_orient	(true);
//   				//object->path().set_dest_direction	(command.direction);
//  				object->path().set_dest_direction	(m_dir_to_enemy);				
//  			} 
//  		}
// 	}
//}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackRunAbstract::finalize()
{
	inherited::finalize					();
	object->path().extrapolate_path	(false);
}

TEMPLATE_SPECIALIZATION
void CStateGroupAttackRunAbstract::critical_finalize()
{
	inherited::critical_finalize		();
	object->path().extrapolate_path	(false);
}


TEMPLATE_SPECIALIZATION
bool CStateGroupAttackRunAbstract::check_completion()
{
	float m_fDistMin = object->MeleeChecker.get_min_distance();
	float dist		 = object->MeleeChecker.distance_to_enemy(object->EnemyMan.get_enemy());

	if ( dist < m_fDistMin )
	{
		return true;
	}

// 	if ( !ai().level_graph().valid_vertex_id(object->EnemyMan.get_enemy_vertex()) )
// 	{
// 		return true;
// 	}

	return false;
}

TEMPLATE_SPECIALIZATION
bool CStateGroupAttackRunAbstract::check_start_conditions()
{
	float m_fDistMax = object->MeleeChecker.get_max_distance();
	float dist		 = object->MeleeChecker.distance_to_enemy(object->EnemyMan.get_enemy());

	if ( dist > m_fDistMax )
	{
		return true;
	}

	return false;
}
