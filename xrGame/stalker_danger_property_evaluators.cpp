////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_danger_property_evaluators.cpp
//	Created 	: 31.05.2005
//  Modified 	: 31.05.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker danger property evaluators classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_danger_property_evaluators.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "stalker_decision_space.h"
#include "memory_manager.h"
#include "danger_manager.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "stalker_movement_manager_smart_cover.h"
#include "cover_evaluators.h"
#include "ai_space.h"
#include "cover_manager.h"
#include "cover_point.h"
#include "stalker_movement_restriction.h"
#include "enemy_manager.h"
#include "stalker_animation_manager.h"

using namespace StalkerDecisionSpace;

typedef CStalkerPropertyEvaluator::_value_type _value_type;

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorDangers
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorDangers::CStalkerPropertyEvaluatorDangers	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited			(object ? object->lua_game_object() : 0,evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorDangers::evaluate	()
{
	if (!m_object->memory().danger().selected())
		return			(false);
	return				(true);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorDangerUnknown
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorDangerUnknown::CStalkerPropertyEvaluatorDangerUnknown	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited			(object ? object->lua_game_object() : 0,evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorDangerUnknown::evaluate	()
{
	if (!m_object->memory().danger().selected())
		return			(false);

	switch (m_object->memory().danger().selected()->type()) {
		case CDangerObject::eDangerTypeBulletRicochet :
		case CDangerObject::eDangerTypeEntityDeath :
		case CDangerObject::eDangerTypeFreshEntityCorpse :
			return		(true);
		default :
			return		(false);
	};
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorDangerInDirection
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorDangerInDirection::CStalkerPropertyEvaluatorDangerInDirection	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited			(object ? object->lua_game_object() : 0,evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorDangerInDirection::evaluate	()
{
	if (!m_object->memory().danger().selected())
		return			(false);

	switch (m_object->memory().danger().selected()->type()) {
		case CDangerObject::eDangerTypeAttackSound :
		case CDangerObject::eDangerTypeEntityAttacked :
		case CDangerObject::eDangerTypeAttacked :
		// fakes, temporarily
//		case CDangerObject::eDangerTypeBulletRicochet :
//		case CDangerObject::eDangerTypeEntityDeath :
//		case CDangerObject::eDangerTypeFreshEntityCorpse :
		case CDangerObject::eDangerTypeEnemySound :
			return		(true);
		default :
			return		(false);
	};
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorDangerWithGrenade
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorDangerWithGrenade::CStalkerPropertyEvaluatorDangerWithGrenade	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited			(object ? object->lua_game_object() : 0,evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorDangerWithGrenade::evaluate	()
{
	if (!m_object->memory().danger().selected())
		return			(false);

	return				(CDangerObject::eDangerTypeGrenade == m_object->memory().danger().selected()->type());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorDangerBySound
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorDangerBySound::CStalkerPropertyEvaluatorDangerBySound	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited			(object ? object->lua_game_object() : 0,evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorDangerBySound::evaluate	()
{
	if (!m_object->memory().danger().selected())
		return			(false);

	return				(false);
//	return				(CDangerObject::eDangerTypeEnemySound == m_object->memory().danger().selected()->type());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorDangerUnknownCoverActual
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorDangerUnknownCoverActual::CStalkerPropertyEvaluatorDangerUnknownCoverActual	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited			(object ? object->lua_game_object() : 0,evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorDangerUnknownCoverActual::evaluate	()
{
	if (!object().memory().danger().selected())
		return							(false);

	if (!object().agent_manager().member().member(&object()).cover())
		m_cover_selection_position		= object().Position();

	if (!property(StalkerDecisionSpace::eWorldPropertyCoverReached) && object().movement().path_completed())
		m_cover_selection_position		= object().Position();

	if (object().m_ce_best->selected() && !object().agent_manager().member().member(&object()).cover())
		object().m_ce_best->invalidate	();

	bool								result = false, first_time = true;
	const CCoverPoint					*point, *last_cover = object().agent_manager().member().member(m_object).cover();
	Fvector								position = object().memory().danger().selected()->position();
	for (;;) {
		object().m_ce_best->setup		(position,10.f,170.f,10.f);
		point							= ai().cover_manager().best_cover(m_cover_selection_position,10.f,*object().m_ce_best,CStalkerMovementRestrictor(m_object,true,false));
		if (!point) {
			object().m_ce_best->setup	(position,10.f,170.f,10.f);
			point						= ai().cover_manager().best_cover(m_cover_selection_position,30.f,*object().m_ce_best,CStalkerMovementRestrictor(m_object,true,false));
		}

		if (!first_time)
			break;

		if (point == last_cover) {
			result						= true;
			break;
		}

		if (last_cover && point &&(point->position().distance_to_sqr(last_cover->position()) <= 1.f)) {
			point						= last_cover;
			result						= true;
			break;
		}

		if (m_cover_selection_position.similar(object().Position()))
			break;

		m_cover_selection_position		= object().Position();
		result							= false;
		first_time						= false;
	}

	object().agent_manager().location().make_suitable(m_object,point);

	if (!result)
		m_storage->set_property			(eWorldPropertyCoverReached,false);
	
	return								(result);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorDangerGrenadeExploded
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorDangerGrenadeExploded::CStalkerPropertyEvaluatorDangerGrenadeExploded	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited			(object ? object->lua_game_object() : 0,evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorDangerGrenadeExploded::evaluate	()
{
	if (!m_object->memory().danger().selected())
		return			(false);

	if (CDangerObject::eDangerTypeGrenade != m_object->memory().danger().selected()->type())
		return			(false);

	return				(!m_object->memory().danger().selected()->dependent_object());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorGrenadeToExplode
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorGrenadeToExplode::CStalkerPropertyEvaluatorGrenadeToExplode	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited			(object ? object->lua_game_object() : 0,evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorGrenadeToExplode::evaluate	()
{
	if (object().animation().global_selector())
		return			(false);

	if (!m_object->memory().danger().selected())
		return			(false);

	if (CDangerObject::eDangerTypeGrenade != m_object->memory().danger().selected()->type())
		return			(false);

	return				(!!m_object->memory().danger().selected()->dependent_object());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemyWounded
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemyWounded::CStalkerPropertyEvaluatorEnemyWounded	(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited		(object ? object->lua_game_object() : 0,evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorEnemyWounded::evaluate	()
{
	const CEntityAlive			*enemy = object().memory().enemy().selected();
	if (!enemy)
		return					(false);

	const CAI_Stalker			*stalker = smart_cast<const CAI_Stalker *>(enemy);
	if (!stalker)
		return					(false);

	return						(stalker->wounded(&object().movement().restrictions()));
}