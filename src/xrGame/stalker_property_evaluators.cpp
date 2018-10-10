////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_property_evaluators.cpp
//	Created 	: 25.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker property evaluators classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_property_evaluators.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_decision_space.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "ai/ai_monsters_misc.h"
#include "Inventory.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "item_manager.h"
#include "enemy_manager.h"
#include "danger_manager.h"
#include "ai_space.h"
#include "ai/stalker/ai_stalker.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_human_brain.h"
#include "Actor.h"
#include "actor_memory.h"
#include "stalker_movement_manager_smart_cover.h"
#include "agent_manager.h"
#include "agent_enemy_manager.h"
#include "agent_member_manager.h"
#include "cover_point.h"
#include "xrAICore/Navigation/level_graph.h"
#include "stalker_animation_manager.h"
#include "Weapon.h"

using namespace StalkerDecisionSpace;

typedef CStalkerPropertyEvaluator::_value_type _value_type;

const float wounded_enemy_reached_distance = 3.f;

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorALife
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorALife::CStalkerPropertyEvaluatorALife(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorALife::evaluate() { return (!!ai().get_alife()); }
//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorAlive
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorAlive::CStalkerPropertyEvaluatorAlive(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorAlive::evaluate() { return (!!object().g_Alive()); }
//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorItems
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorItems::CStalkerPropertyEvaluatorItems(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorItems::evaluate() { return (!!m_object->memory().item().selected()); }
//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemies
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemies::CStalkerPropertyEvaluatorEnemies(
    CAI_Stalker* object, LPCSTR evaluator_name, u32 time_to_wait, const bool* dont_wait)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
    m_time_to_wait = time_to_wait;
    m_dont_wait = dont_wait;
}

_value_type CStalkerPropertyEvaluatorEnemies::evaluate()
{
    if (m_object->memory().enemy().selected())
        return (true);

    if (m_dont_wait && *m_dont_wait)
        return (false);

    if (Device.dwTimeGlobal < m_object->memory().enemy().last_enemy_time() + m_time_to_wait)
        return (true);

    return (false);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorSeeEnemy
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorSeeEnemy::CStalkerPropertyEvaluatorSeeEnemy(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorSeeEnemy::evaluate()
{
    return (m_object->memory().enemy().selected() ?
            m_object->memory().visual().visible_now(m_object->memory().enemy().selected()) :
            false);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemySeeMe
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemySeeMe::CStalkerPropertyEvaluatorEnemySeeMe(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorEnemySeeMe::evaluate()
{
    const CEntityAlive* enemy = m_object->memory().enemy().selected();
    if (!enemy)
        return (false);

    const CCustomMonster* enemy_monster = smart_cast<const CCustomMonster*>(enemy);
    if (enemy_monster)
        return (enemy_monster->memory().visual().visible_now(m_object));

    const CActor* actor = smart_cast<const CActor*>(enemy);
    if (actor)
        return (actor->memory().visual().visible_now(m_object));

    return (false);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorItemToKill
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorItemToKill::CStalkerPropertyEvaluatorItemToKill(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorItemToKill::evaluate() { return (!!m_object->item_to_kill()); }
//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorItemCanKill
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorItemCanKill::CStalkerPropertyEvaluatorItemCanKill(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorItemCanKill::evaluate() { return (m_object->item_can_kill()); }
//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorFoundItemToKill
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorFoundItemToKill::CStalkerPropertyEvaluatorFoundItemToKill(
    CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorFoundItemToKill::evaluate() { return (m_object->remember_item_to_kill()); }
//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorFoundAmmo
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorFoundAmmo::CStalkerPropertyEvaluatorFoundAmmo(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorFoundAmmo::evaluate() { return (m_object->remember_ammo()); }
//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorReadyToKillSmartCover
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorReadyToKillSmartCover::CStalkerPropertyEvaluatorReadyToKillSmartCover(
    CAI_Stalker* object, LPCSTR evaluator_name, u32 min_ammo_count)
    : inherited(object, evaluator_name, min_ammo_count)
{
}

_value_type CStalkerPropertyEvaluatorReadyToKillSmartCover::evaluate()
{
    if (m_object->movement().current_params().cover() && !m_object->movement().current_params().cover()->can_fire())
        return (true);

    return (inherited::evaluate());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorReadyToKill
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorReadyToKill::CStalkerPropertyEvaluatorReadyToKill(
    CAI_Stalker* object, LPCSTR evaluator_name, u32 min_ammo_count)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name), m_min_ammo_count(min_ammo_count)
{
}

_value_type CStalkerPropertyEvaluatorReadyToKill::evaluate()
{
    if (!m_object->ready_to_kill() || !m_object->best_weapon())
        return (false);

    if (!m_min_ammo_count)
        return (true);

    CWeapon& best_weapon = smart_cast<CWeapon&>(*m_object->best_weapon());
    if (best_weapon.GetAmmoElapsed() <= (int)m_min_ammo_count)
    {
        if (best_weapon.GetAmmoMagSize() <= (int)m_min_ammo_count)
            return (best_weapon.GetState() != CWeapon::eReload);
        else
            return (false);
    }

    return (best_weapon.GetState() != CWeapon::eReload);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorReadyToDetour
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorReadyToDetour::CStalkerPropertyEvaluatorReadyToDetour(
    CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorReadyToDetour::evaluate() { return (m_object->ready_to_detour()); }
//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorAnomaly
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorAnomaly::CStalkerPropertyEvaluatorAnomaly(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorAnomaly::evaluate()
{
    if (!m_object->undetected_anomaly())
        return (false);

    if (!m_object->memory().enemy().selected())
        return (true);

    u32 result = dwfChooseAction(2000, m_object->panic_threshold(), 0.f, 0.f, 0.f, m_object->g_Team(),
        m_object->g_Squad(), m_object->g_Group(), 0, 1, 2, 3, 4, m_object, 300.f);
    return (!result);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorInsideAnomaly
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorInsideAnomaly::CStalkerPropertyEvaluatorInsideAnomaly(
    CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorInsideAnomaly::evaluate()
{
    if (!m_object->inside_anomaly())
        return (false);

    if (!m_object->memory().enemy().selected())
        return (true);

    u32 result = dwfChooseAction(2000, m_object->panic_threshold(), 0.f, 0.f, 0.f, m_object->g_Team(),
        m_object->g_Squad(), m_object->g_Group(), 0, 1, 2, 3, 4, m_object, 300.f);
    return (!result);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorPanic
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorPanic::CStalkerPropertyEvaluatorPanic(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorPanic::evaluate()
{
    if (object().animation().global_selector())
        return (false);

    u32 result = dwfChooseAction(2000, m_object->panic_threshold(), 0.f, 0.f, 0.f, m_object->g_Team(),
        m_object->g_Squad(), m_object->g_Group(), 0, 1, 2, 3, 4, m_object, 300.f);
    return (!!result);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorSmartTerrainTask
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorSmartTerrainTask::CStalkerPropertyEvaluatorSmartTerrainTask(
    CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorSmartTerrainTask::evaluate()
{
    if (!ai().get_alife())
        return (false);

    CSE_ALifeHumanAbstract* stalker =
        smart_cast<CSE_ALifeHumanAbstract*>(ai().alife().objects().object(m_object->ID(), true));
    if (!stalker)
        return (false);

    VERIFY(stalker);
    stalker->brain().select_task();
    return (stalker->m_smart_terrain_id != 0xffff);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemyReached
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemyReached::CStalkerPropertyEvaluatorEnemyReached(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorEnemyReached::evaluate()
{
    const CEntityAlive* enemy = object().memory().enemy().selected();
    if (!enemy)
        return (false);

    ALife::_OBJECT_ID processor_id = object().agent_manager().enemy().wounded_processor(enemy);
    if (processor_id != object().ID())
        return (false);

    return ((object().Position().distance_to_sqr(enemy->Position()) <= _sqr(wounded_enemy_reached_distance)));
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorPlayerOnThePath
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorPlayerOnThePath::CStalkerPropertyEvaluatorPlayerOnThePath(
    CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorPlayerOnThePath::evaluate()
{
    const CEntityAlive* enemy = object().memory().enemy().selected();
    if (!enemy)
        return (false);

    if (!object().is_relation_enemy(Actor()))
        return (false);

    if (!m_object->memory().visual().visible_now(Actor()))
        return (false);

    return (object().movement().is_object_on_the_way(Actor(), 2.f));
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemyCriticallyWounded
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemyCriticallyWounded::CStalkerPropertyEvaluatorEnemyCriticallyWounded(
    CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorEnemyCriticallyWounded::evaluate()
{
    const CEntityAlive* enemy = object().memory().enemy().selected();
    if (!enemy)
        return (false);

    const CAI_Stalker* enemy_stalker = smart_cast<const CAI_Stalker*>(enemy);
    if (!enemy_stalker)
        return (false);

    return (const_cast<CAI_Stalker*>(enemy_stalker)->critically_wounded());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorShouldThrowGrenade
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorShouldThrowGrenade::CStalkerPropertyEvaluatorShouldThrowGrenade(
    CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorShouldThrowGrenade::evaluate()
{
#if 0
	return						(false);
#else // #if 1

    if (m_storage->property(eWorldPropertyStartedToThrowGrenade))
        return (true);

    if (!m_storage->property(eWorldPropertyInCover) && !m_storage->property(eWorldPropertyPositionHolded) &&
        !m_storage->property(eWorldPropertyEnemyDetoured))
        return (false);

    // do not throw grenades too often
    if (object().last_throw_time() + object().throw_time_interval() >= Device.dwTimeGlobal)
        return (false);

    // throw grenades only in case when we have them
    if (object().inventory().ItemFromSlot(GRENADE_SLOT) == 0)
        return (false);

    // do not throw grenades when there is no enemies
    const CEntityAlive* enemy = object().memory().enemy().selected();
    if (!enemy)
        return (false);

    if (!enemy->human_being())
        return (false);

    if (object().memory().visual().visible_now(enemy))
        return (false);

    // do not throw grenades when object is not in our memory (how this can be?)
    CMemoryInfo mem_object = object().memory().memory(enemy);
    if (!mem_object.m_object)
        return (false);

    Fvector const& position = mem_object.m_object_params.m_position;
    u32 const& enemy_vertex_id = mem_object.m_object_params.m_level_vertex_id;
    if (object().Position().distance_to_sqr(position) < _sqr(10.f))
        return (false);

    if (!object().agent_manager().member().can_throw_grenade(position))
        return (false);

    // setup throw target
    object().throw_target(position, enemy_vertex_id, const_cast<CEntityAlive*>(enemy));

    // here we should check if we are unable to stop grenade throwing
    // in this case we should return true
    if (object().inventory().ItemFromSlot(GRENADE_SLOT) == object().inventory().ActiveItem())
        return (true);

    // do not throw grenades when throw trajectory is obstructed
    if (!object().throw_enabled())
        return (false);

    // do throw grenade
    return (true);
#endif // #if 1
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorTooFarToKillEnemy
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorTooFarToKillEnemy::CStalkerPropertyEvaluatorTooFarToKillEnemy(
    CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorTooFarToKillEnemy::evaluate()
{
    if (!object().memory().enemy().selected())
        return (false);

    if (!object().best_weapon())
        return (false);

    CMemoryInfo mem_object = object().memory().memory(object().memory().enemy().selected());
    return (object().too_far_to_kill_enemy(mem_object.m_object_params.m_position));
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorLowCover
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorLowCover::CStalkerPropertyEvaluatorLowCover(CAI_Stalker* object, LPCSTR evaluator_name)
    : inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorLowCover::evaluate()
{
    return (false);

#if 0
	if (!m_storage->property(eWorldPropertyInCover))
		return					(false);

	if (!object().memory().enemy().selected())
		return					(false);

	if (!object().best_weapon())
		return					(false);

	CMemoryInfo					mem_object = object().memory().memory(object().memory().enemy().selected());
	const CCoverPoint			*cover = object().best_cover(mem_object.m_object_params.m_position);
	if (!cover)
		return					(false);

	if (object().Position().distance_to_sqr(cover->position()) > .1f)
		return					(false);

	Fvector						direction;
	float						y,p;
	direction.sub				(mem_object.m_object_params.m_position, cover->position());
	direction.getHP				(y,p);
	float						high_cover_value = ai().level_graph().high_cover_in_direction(y, cover->level_vertex_id());
	float						low_cover_value  = ai().level_graph().low_cover_in_direction (y, cover->level_vertex_id());

	if (low_cover_value >= high_cover_value)
		return					(false);

	// should be several other conditions here
	return						(true);
#endif
}
