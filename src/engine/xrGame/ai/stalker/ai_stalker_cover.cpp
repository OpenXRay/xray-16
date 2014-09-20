////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_cover.cpp
//	Created 	: 25.04.2006
//  Modified 	: 25.04.2006
//	Author		: Dmitriy Iassenev
//	Description : 
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_stalker.h"
#include "../../cover_point.h"
#include "../../cover_evaluators.h"
#include "../../ai_space.h"
#include "../../cover_manager.h"
#include "../../stalker_movement_restriction.h"
#include "../../level_graph.h"
#include "../../inventory_item.h"
#include "../../agent_member_manager.h"
#include "../../memory_manager.h"
#include "../../enemy_manager.h"
#include "../../stalker_movement_manager_smart_cover.h"
#include "../../smart_cover.h"
#include "../../smart_cover_planner_target_selector.h"
#include "../../smart_cover_animation_planner.h"
#include "../../script_game_object.h"
#include "../../stalker_decision_space.h"
#include "../../weapon.h"

extern const float MIN_SUITABLE_ENEMY_DISTANCE = 3.f;

#ifdef _DEBUG
static int g_advance_search_count		= 0;
static int g_near_cover_search_count	= 0;
static int g_far_cover_search_count		= 0;
#endif // _DEBUG

void CAI_Stalker::subscribe_on_best_cover_changed	(const on_best_cover_changed_delegate &delegate)
{
	VERIFY								(m_cover_delegates.end() == std::find(m_cover_delegates.begin(),m_cover_delegates.end(),delegate));
	m_cover_delegates.push_back			(delegate);
}

void CAI_Stalker::unsubscribe_on_best_cover_changed	(const on_best_cover_changed_delegate &delegate)
{
	cover_delegates::iterator			I = std::find(m_cover_delegates.begin(),m_cover_delegates.end(),delegate);
	VERIFY								(I != m_cover_delegates.end());
	m_cover_delegates.erase				(I);
}

void CAI_Stalker::on_best_cover_changed				(const CCoverPoint *new_cover, const CCoverPoint *old_cover)
{
#if 0
	if (new_cover) {
		if (!new_cover->m_is_smart_cover)
			Msg							("[%6d][%s], now it is cover", Device.dwTimeGlobal, cName().c_str());
		else
			Msg							(
				"[%6d][%s], now it is smart cover %s",
				Device.dwTimeGlobal,
				cName().c_str(),
				static_cast<smart_cover::cover const *>(new_cover)->object().cName().c_str()
			);
	}
#endif

	cover_delegates::const_iterator		I = m_cover_delegates.begin();
	cover_delegates::const_iterator		E = m_cover_delegates.end();
	for ( ; I != E; ++I)
		(*I)							(new_cover,old_cover);
}

void CAI_Stalker::compute_enemy_distances			(float &minimum_enemy_distance, float &maximum_enemy_distance)
{
	minimum_enemy_distance				= MIN_SUITABLE_ENEMY_DISTANCE;
	maximum_enemy_distance				= 170.f;

	if (!best_weapon())
		return;
	
	int									weapon_type = best_weapon()->object().ef_weapon_type();
	switch (weapon_type) {
		// pistols
		case 5 : {
			maximum_enemy_distance		= 10.f;
			break;
		}
		// shotguns
		case 9 : {
			maximum_enemy_distance		= 5.f;
			break;
		}
		// sniper rifles
		case 11 :
		case 12 : {
			minimum_enemy_distance		= 20.f;
			break;
		}
		default: {
			maximum_enemy_distance		= 20.f;
			break;
		}
	}

	minimum_enemy_distance				= _min(minimum_enemy_distance, maximum_enemy_distance);
	maximum_enemy_distance				= _max(minimum_enemy_distance, maximum_enemy_distance);
}

const CCoverPoint *CAI_Stalker::find_best_cover		(const Fvector &position_to_cover_from)
{
#ifdef _DEBUG
//	Msg									("* [%6d][%s] search for new cover performed",Device.dwTimeGlobal,*cName());
#endif
#ifdef _DEBUG
	++g_near_cover_search_count;
#endif
	float								minimum_enemy_distance, maximum_enemy_distance;
	compute_enemy_distances				(minimum_enemy_distance, maximum_enemy_distance);

	if (!best_weapon())
		m_ce_best->can_use_smart_covers	(false);
	else {
		CWeapon* weapon					= smart_cast<CWeapon*>(best_weapon());
		if (!weapon)
			m_ce_best->can_use_smart_covers	(false);
		else {
			if (weapon->BaseSlot() != INV_SLOT_3)
				m_ce_best->can_use_smart_covers	(false);
			else
				m_ce_best->can_use_smart_covers	(true);
		}
	}

	m_ce_best->setup					(position_to_cover_from, minimum_enemy_distance,maximum_enemy_distance,minimum_enemy_distance);
	const CCoverPoint					*point = ai().cover_manager().best_cover(Position(),10.f,*m_ce_best,CStalkerMovementRestrictor(this,true));
	if (point)
		return							(point);

#ifdef _DEBUG
	++g_far_cover_search_count;
#endif
	m_ce_best->setup					(position_to_cover_from, minimum_enemy_distance,maximum_enemy_distance,minimum_enemy_distance);
	point								= ai().cover_manager().best_cover(Position(),30.f,*m_ce_best,CStalkerMovementRestrictor(this,true));
	return								(point);
}

float CAI_Stalker::best_cover_value					(const Fvector &position_to_cover_from)
{
	m_ce_best->setup					(position_to_cover_from,MIN_SUITABLE_ENEMY_DISTANCE,170.f,MIN_SUITABLE_ENEMY_DISTANCE);
	m_ce_best->initialize				(Position(),true);
	m_ce_best->evaluate					(m_best_cover,CStalkerMovementRestrictor(this,true).weight(m_best_cover));
	return								(m_ce_best->best_value());
}

void CAI_Stalker::best_cover_can_try_advance		()
{
	if (!m_best_cover_actual)
		return;

	if (m_best_cover_advance_cover == m_best_cover)
		return;

	m_best_cover_can_try_advance		= true;
}

void CAI_Stalker::update_best_cover_actuality		(const Fvector &position_to_cover_from)
{
	if (!m_best_cover_actual)
		return;

	if (!m_best_cover) {
		m_best_cover_actual = false;
		return;
	}

	if (m_best_cover->m_is_smart_cover) {
		float							value;
		smart_cover::cover const		*cover = static_cast<smart_cover::cover const*>(m_best_cover);
		smart_cover::loophole			*loophole = cover->best_loophole(position_to_cover_from, value, false, movement().current_params().cover() == m_best_cover );
		if (!loophole) {
			m_ce_best->invalidate		();
			m_best_cover_actual			= false;
			return;
		}
	}

	if (m_best_cover->position().distance_to_sqr(position_to_cover_from) < _sqr(MIN_SUITABLE_ENEMY_DISTANCE)) {
		m_best_cover_actual				= false;
#if 0//def _DEBUG
		Msg								("* [%6d][%s] enemy too close",Device.dwTimeGlobal,*cName());
#endif
		return;
	}

	float								cover_value = best_cover_value(position_to_cover_from);
	if (cover_value >= m_best_cover_value + 1.f) {
		m_best_cover_actual				= false;
#if 0//def _DEBUG
		Msg								("* [%6d][%s] cover became too bad",Device.dwTimeGlobal,*cName());
#endif
		return;
	}

//	if (cover_value >= 1.5f*m_best_cover_value) {
//		m_best_cover_actual				= false;
//		Msg								("* [%6d][%s] cover became too bad2",Device.dwTimeGlobal,*cName());
//		return;
//	}

	if (false)//!m_best_cover_can_try_advance)
		return;

	if (m_best_cover_advance_cover == m_best_cover)
		return;

	m_best_cover_advance_cover			= m_best_cover;
	m_best_cover_can_try_advance		= false;

#ifdef _DEBUG
//	Msg									("* [%6d][%s] advance search performed",Device.dwTimeGlobal,*cName());
#endif
#ifdef _DEBUG
	++g_advance_search_count;
#endif
	m_ce_best->setup					(position_to_cover_from,MIN_SUITABLE_ENEMY_DISTANCE,170.f,MIN_SUITABLE_ENEMY_DISTANCE);
	m_best_cover						= ai().cover_manager().best_cover(Position(),10.f,*m_ce_best,CStalkerMovementRestrictor(this,true));
}

const CCoverPoint *CAI_Stalker::best_cover			(const Fvector &position_to_cover_from)
{
//	shared_str const					&cover_id = movement().current_params().cover_id();
//	if (cover_id != "")
//		return							(ai().cover_manager().smart_cover(cover_id));

	update_best_cover_actuality			(position_to_cover_from);

	if (m_best_cover_actual) {
		agent_manager().member().member(this).cover(m_best_cover);
		return							(m_best_cover);
	}

	m_best_cover_actual					= true;
	
	const CCoverPoint					*best_cover = find_best_cover(position_to_cover_from);
	if (best_cover != m_best_cover) {
		on_best_cover_changed			(best_cover,m_best_cover);
		m_best_cover					= best_cover;
		m_best_cover_advance_cover		= 0;
		m_best_cover_can_try_advance	= false;
	}

	m_best_cover_value					= m_best_cover ? best_cover_value(position_to_cover_from) : flt_max;

	agent_manager().member().member(this).cover(m_best_cover);

	return								(m_best_cover);
}

void CAI_Stalker::on_restrictions_change			()
{
	inherited::on_restrictions_change	();
	m_best_cover_actual					= false;
#ifdef _DEBUG
	Msg									("* [%6d][%s] on_restrictions_change",Device.dwTimeGlobal,*cName());
#endif
}

void CAI_Stalker::on_enemy_change					(const CEntityAlive *enemy)
{
	inherited::on_enemy_change			(enemy);
	m_item_actuality					= false;
	m_best_cover_actual					= false;
#ifdef _DEBUG
//	Msg									("* [%6d][%s] on_enemy_change",Device.dwTimeGlobal,*cName());
#endif
}

void CAI_Stalker::on_danger_location_add			(const CDangerLocation &location)
{
	if (!m_best_cover)
		return;

	if (m_best_cover->position().distance_to_sqr(location.position()) <= _sqr(location.m_radius)) {
#ifdef _DEBUG
//		Msg								("* [%6d][%s] on_danger_add",Device.dwTimeGlobal,*cName());
#endif
		m_best_cover_actual				= false;
	}
}

void CAI_Stalker::on_danger_location_remove			(const CDangerLocation &location)
{
	if (!m_best_cover) {
		if (Position().distance_to_sqr(location.position()) <= _sqr(location.m_radius)) {
#ifdef _DEBUG
//			Msg							("* [%6d][%s] on_danger_remove",Device.dwTimeGlobal,*cName());
#endif
			m_best_cover_actual			= false;
		}

		return;
	}

	if (m_best_cover->position().distance_to_sqr(location.position()) <= _sqr(location.m_radius)) {
#ifdef _DEBUG
//		Msg								("* [%6d][%s] on_danger_remove",Device.dwTimeGlobal,*cName());
#endif
		m_best_cover_actual				= false;
	}
}

void CAI_Stalker::on_cover_blocked					(const CCoverPoint *cover)
{
#ifdef _DEBUG
//	Msg									("* [%6d][%s] cover is blocked",Device.dwTimeGlobal,*cName());
#endif
	m_best_cover_actual					= false;
}

void CAI_Stalker::best_cover_invalidate				()
{
	m_best_cover_actual					= false;
}

bool CAI_Stalker::use_smart_covers_only				() const
{
	VERIFY								(m_ce_best);
	return								(m_ce_best->use_smart_covers_only());
}

void CAI_Stalker::use_smart_covers_only				(bool value)
{
	VERIFY								(m_ce_best);
	m_ce_best->use_smart_covers_only	(value);
}
