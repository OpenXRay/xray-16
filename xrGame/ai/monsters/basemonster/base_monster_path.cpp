////////////////////////////////////////////////////////////////////////////
//	Module 		: base_monster_path.cpp
//	Created 	: 26.05.2003
//  Modified 	: 26.05.2003
//	Author		: Serge Zhem
//	Description : Path finding, curve building, position prediction
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "base_monster.h"
#include "../corpse_cover.h"
#include "../../../cover_manager.h"
#include "../../../cover_point.h"
#include "../../../ai_space.h"
#include "../control_direction_base.h"

// каждый монстр может по-разному реализвать эту функ (e.g. кровосос с поворотом головы и т.п.)
void CBaseMonster::LookPosition(Fvector to_point, float angular_speed)
{
	// по-умолчанию просто изменить movement().m_body.target.yaw
	Fvector	d;
	d.set(to_point);
	d.sub(Position());	
	
	// установить текущий угол
	dir().set_heading(angle_normalize(-d.getH()));
}

//////////////////////////////////////////////////////////////////////////
// Covers
//////////////////////////////////////////////////////////////////////////

bool CBaseMonster::GetCorpseCover(Fvector &position, u32 &vertex_id) 
{
	m_corpse_cover_evaluator->setup(10.f,50.f);
	const CCoverPoint *point = ai().cover_manager().best_cover(Position(),30.f,*m_corpse_cover_evaluator);
	if (!point) return false;
	
	position	= point->m_position;
	vertex_id	= point->m_level_vertex_id;
	return true;
}

bool CBaseMonster::GetCoverFromEnemy(const Fvector &enemy_pos, Fvector &position, u32 &vertex_id) 
{
	m_enemy_cover_evaluator->setup(enemy_pos, 30.f,50.f);
	const CCoverPoint	 *point = ai().cover_manager().best_cover(Position(),40.f,*m_enemy_cover_evaluator);
	if (!point) return false;

	position	= point->m_position;
	vertex_id	= point->m_level_vertex_id;
	return true;
}

bool CBaseMonster::GetCoverFromPoint(const Fvector &pos, Fvector &position, u32 &vertex_id, float min_dist, float max_dist, float radius) 
{
	m_enemy_cover_evaluator->setup(pos, min_dist,max_dist);
	const CCoverPoint	 *point = ai().cover_manager().best_cover(Position(),radius,*m_enemy_cover_evaluator);
	if (!point) return false;

	position	= point->m_position;
	vertex_id	= point->m_level_vertex_id;
	return true;
}

bool CBaseMonster::GetCoverCloseToPoint(const Fvector &dest_pos, float min_dist, float max_dist, float deviation, float radius ,Fvector &position, u32 &vertex_id) 
{
	m_cover_evaluator_close_point->setup(dest_pos,min_dist, max_dist,deviation);
	const CCoverPoint	 *point = ai().cover_manager().best_cover(Position(),radius,*m_cover_evaluator_close_point);
	if (!point) return false;

	position	= point->m_position;
	vertex_id	= point->m_level_vertex_id;
	return true;
}
