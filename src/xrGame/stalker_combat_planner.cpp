////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_combat_planner.cpp
//	Created 	: 25.03.2004
//  Modified 	: 27.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker combat planner
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_combat_planner.h"
#include "stalker_combat_actions.h"
#include "stalker_danger_property_evaluators.h"
#include "stalker_decision_space.h"
#include "stalker_property_evaluators.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "cover_evaluators.h"
#include "cover_manager.h"
#include "cover_point.h"
#include "stalker_movement_restriction.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "enemy_manager.h"
#include "danger_manager.h"
#include "sound_player.h"
#include "Missile.h"
#include "Explosive.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "member_order.h"
#include "ai/stalker/ai_stalker_space.h"
#include "stalker_planner.h"
#include "stalker_kill_wounded_planner.h"
#include "stalker_movement_manager_smart_cover.h"
#include "stalker_get_distance_planner.h"
#include "stalker_low_cover_planner.h"
#include "stalker_search_planner.h"
#include "smart_cover_evaluators.h"
#include "Inventory.h"
#include "WeaponMagazined.h"

using namespace StalkerSpace;
using namespace StalkerDecisionSpace;

CStalkerCombatPlanner::CStalkerCombatPlanner(CAI_Stalker* object, LPCSTR action_name) : inherited(object, action_name)
{
}

CStalkerCombatPlanner::~CStalkerCombatPlanner()
{
    CAI_Stalker::on_best_cover_changed_delegate temp;
    temp.bind(this, &CStalkerCombatPlanner::on_best_cover_changed);
    object().unsubscribe_on_best_cover_changed(temp);
}

void CStalkerCombatPlanner::on_best_cover_changed(const CCoverPoint* new_cover, const CCoverPoint* old_cover)
{
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyInCover, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyLookedOut, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyPositionHolded, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyEnemyDetoured, false);
}

void CStalkerCombatPlanner::setup(CAI_Stalker* object, CPropertyStorage* storage)
{
    inherited::setup(object, storage);
#ifdef LOG_ACTION
    inherited_planner::m_use_log = true;
    inherited_action::m_use_log = true;
#endif
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyInCover, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyLookedOut, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyPositionHolded, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyEnemyDetoured, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyUseSuddenness, true);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyUseCrouchToLookOut, true);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyKilledWounded, false);
    CScriptActionPlanner::m_storage.set_property(eWorldPropertyStartedToThrowGrenade, false);

    this->object().brain().CStalkerPlanner::m_storage.set_property(eWorldPropertyCriticallyWounded, false);

    clear();
    add_evaluators();
    add_actions();

    CAI_Stalker::on_best_cover_changed_delegate temp;
    temp.bind(this, &CStalkerCombatPlanner::on_best_cover_changed);
    this->object().subscribe_on_best_cover_changed(temp);

    object->movement().property_storage(storage);
}

void CStalkerCombatPlanner::execute() { inherited::execute(); }
void CStalkerCombatPlanner::update()
{
    inherited::update();

    object().react_on_grenades();
    object().react_on_member_death();

    //	const CEntityAlive				*enemy = object().memory().enemy().selected();
    //	VERIFY							(enemy);
    //	const CAI_Stalker				*stalker = smart_cast<const CAI_Stalker*>(enemy);
    //	m_last_wounded					= stalker && stalker->wounded();
}

void CStalkerCombatPlanner::initialize()
{
    inherited::initialize();

#ifdef DEBUG
//	inherited_planner::m_use_log = true;
#endif // DEBUG

    if (!m_loaded)
    {
        CScriptActionPlanner::m_storage.set_property(eWorldPropertyInCover, false);
        CScriptActionPlanner::m_storage.set_property(eWorldPropertyLookedOut, false);
        CScriptActionPlanner::m_storage.set_property(eWorldPropertyPositionHolded, false);
        CScriptActionPlanner::m_storage.set_property(eWorldPropertyEnemyDetoured, false);
        CScriptActionPlanner::m_storage.set_property(eWorldPropertyUseSuddenness, true);
        CScriptActionPlanner::m_storage.set_property(eWorldPropertyKilledWounded, false);

        object().brain().CStalkerPlanner::m_storage.set_property(eWorldPropertyCriticallyWounded, false);
    }

    CScriptActionPlanner::m_storage.set_property(eWorldPropertyStartedToThrowGrenade, false);

    object().agent_manager().member().member(m_object).cover(0);
    // this is fake, should be revisited
    // we must clear path, since it can be built using eMentalStateFree velocities
    // and our new path may not be ready
    object().movement().clear_path();

    object().m_clutched_hammer_enabled = true;

    m_last_enemy_id = u16(-1);
    m_last_level_time = 0;
    m_last_wounded = false;

    if (!m_loaded && object().memory().enemy().selected())
    {
        CVisualMemoryManager* visual_memory_manager = object().memory().enemy().selected()->visual_memory();
        VERIFY(visual_memory_manager);
        CScriptActionPlanner::m_storage.set_property(
            eWorldPropertyUseSuddenness, !visual_memory_manager->visible_now(&object()));
    }

    m_loaded = false;

    if (!object().agent_manager().member().combat_members().empty())
        CScriptActionPlanner::m_storage.set_property(eWorldPropertyUseSuddenness, false);

    //  this is possible when i enter combat when it is wait after combat stage
    //	VERIFY					(object().memory().enemy().selected());

    if (m_object->memory().visual().visible_now(m_object->memory().enemy().selected()))
    {
        if (m_object->memory().enemy().selected()->human_being())
            if (object().agent_manager().member().can_cry_noninfo_phrase())
                if (object().agent_manager().member().members().size() > 1)
                    if (!CScriptActionPlanner::m_storage.property(eWorldPropertyUseSuddenness))
                        object().sound().play(eStalkerSoundAlarm);
    }

    object().agent_manager().member().register_in_combat(m_object);
}

void CStalkerCombatPlanner::finalize()
{
    inherited::finalize();

    if (!object().g_Alive())
        return;

    object().memory().danger().time_line(Device.dwTimeGlobal + 3000);
    if (object().agent_manager().member().registered_in_combat(m_object))
        object().agent_manager().member().unregister_in_combat(m_object);

    object().m_clutched_hammer_enabled = false;

    //	object().sound().remove_active_sounds					(eStalkerSoundMaskNoDanger);

    if (object().inventory().ItemFromSlot(INV_SLOT_2))
    {
        CWeaponMagazined* temp = smart_cast<CWeaponMagazined*>(object().inventory().ItemFromSlot(INV_SLOT_2));
        if (object().inventory().ActiveItem() && temp &&
            (object().inventory().ActiveItem()->object().ID() == temp->ID()))
            object().set_goal(eObjectActionIdle, object().inventory().ItemFromSlot(INV_SLOT_2));
    }
}

void CStalkerCombatPlanner::add_evaluators()
{
    add_evaluator(eWorldPropertyPureEnemy, new CStalkerPropertyEvaluatorEnemies(m_object, "is_there_enemies", 0));
    add_evaluator(eWorldPropertyEnemy, new CStalkerPropertyEvaluatorEnemies(m_object, "is_there_enemies_delayed",
                                           POST_COMBAT_WAIT_INTERVAL, &m_last_wounded));
    add_evaluator(eWorldPropertySeeEnemy, new CStalkerPropertyEvaluatorSeeEnemy(m_object, "see enemy"));
    add_evaluator(eWorldPropertyEnemySeeMe, new CStalkerPropertyEvaluatorEnemySeeMe(m_object, "enemy see me"));
    add_evaluator(eWorldPropertyItemToKill, new CStalkerPropertyEvaluatorItemToKill(m_object, "item to kill"));
    add_evaluator(eWorldPropertyItemCanKill, new CStalkerPropertyEvaluatorItemCanKill(m_object, "item can kill"));
    add_evaluator(
        eWorldPropertyFoundItemToKill, new CStalkerPropertyEvaluatorFoundItemToKill(m_object, "found item to kill"));
    add_evaluator(eWorldPropertyFoundAmmo, new CStalkerPropertyEvaluatorFoundAmmo(m_object, "found ammo"));
    add_evaluator(eWorldPropertyReadyToKill, new CStalkerPropertyEvaluatorReadyToKill(m_object, "ready to kill"));
    add_evaluator(eWorldPropertyReadyToDetour, new CStalkerPropertyEvaluatorReadyToDetour(m_object, "ready to detour"));
    add_evaluator(eWorldPropertyPanic, new CStalkerPropertyEvaluatorPanic(m_object, "panic"));
    add_evaluator(eWorldPropertyDangerGrenade,
        new CStalkerPropertyEvaluatorGrenadeToExplode(m_object, "is there grenade to explode"));
    add_evaluator(eWorldPropertyEnemyWounded, new CStalkerPropertyEvaluatorEnemyWounded(m_object, "is enemy wounded"));
    add_evaluator(
        eWorldPropertyPlayerOnThePath, new CStalkerPropertyEvaluatorPlayerOnThePath(m_object, "player on the path"));
    add_evaluator(eWorldPropertyEnemyCriticallyWounded,
        new CStalkerPropertyEvaluatorEnemyCriticallyWounded(m_object, "enemy_critically_wounded"));
    add_evaluator(
        eWorldPropertyTooFarToKillEnemy, new CStalkerPropertyEvaluatorTooFarToKillEnemy(m_object, "too far to kill"));

    add_evaluator(eWorldPropertyInCover,
        new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0, eWorldPropertyInCover, true, true, "in cover"));
    add_evaluator(eWorldPropertyLookedOut,
        new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0, eWorldPropertyLookedOut, true, true, "looked out"));
    add_evaluator(eWorldPropertyPositionHolded, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                                    eWorldPropertyPositionHolded, true, true, "position holded"));
    add_evaluator(eWorldPropertyEnemyDetoured, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                                   eWorldPropertyEnemyDetoured, true, true, "enemy detoured"));
    add_evaluator(eWorldPropertyUseSuddenness, new CStalkerPropertyEvaluatorMember((CPropertyStorage*)0,
                                                   eWorldPropertyUseSuddenness, true, true, "use suddenness"));
    add_evaluator(eWorldPropertyCriticallyWounded,
        new CStalkerPropertyEvaluatorMember(&object().brain().CStalkerPlanner::m_storage,
            eWorldPropertyCriticallyWounded, true, true, "critically wounded"));
    add_evaluator(
        eWorldPropertyKilledWounded, new CStalkerPropertyEvaluatorMember(&object().brain().CStalkerPlanner::m_storage,
                                         eWorldPropertyKilledWounded, true, true, "killed critically wounded"));

    add_evaluator(eWorldPropertyShouldThrowGrenade,
        new CStalkerPropertyEvaluatorShouldThrowGrenade(m_object, "should throw grenade"));
    add_evaluator(eWorldPropertyLowCover, new CStalkerPropertyEvaluatorLowCover(m_object, "using low cover"));

    add_evaluator(
        eWorldPropertyInSmartCover, new smart_cover::evaluators::in_cover_evaluator(m_object, "in smart cover"));
}

void CStalkerCombatPlanner::add_actions()
{
    CStalkerActionBase* action;

    action = new CStalkerActionGetItemToKill(m_object, "get_item_to_kill");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyFoundItemToKill, true);
    add_condition(action, eWorldPropertyItemToKill, false);
    add_effect(action, eWorldPropertyItemToKill, true);
    add_effect(action, eWorldPropertyItemCanKill, true);
    add_operator(eWorldOperatorGetItemToKill, action);

    action = new CStalkerActionMakeItemKilling(m_object, "make_item_killing");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyFoundAmmo, true);
    add_condition(action, eWorldPropertyItemCanKill, false);
    add_effect(action, eWorldPropertyItemCanKill, true);
    add_operator(eWorldOperatorMakeItemKilling, action);

    action = new CStalkerActionRetreatFromEnemy(m_object, "retreat_from_enemy");
    add_effect(action, eWorldPropertyPureEnemy, false);
    add_operator(eWorldOperatorRetreatFromEnemy, action);

    action = new CStalkerActionGetReadyToKill(true, m_object, "get_ready_to_kill");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyItemToKill, true);
    add_condition(action, eWorldPropertyItemCanKill, true);
    add_condition(action, eWorldPropertyReadyToKill, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyPlayerOnThePath, false);
    add_condition(action, eWorldPropertyShouldThrowGrenade, false);
    add_condition(action, eWorldPropertyLowCover, false);
    add_condition(action, eWorldPropertyInSmartCover, false);
    add_effect(action, eWorldPropertyReadyToKill, true);
    add_effect(action, eWorldPropertyInCover, false);
    add_effect(action, eWorldPropertyLookedOut, false);
    add_effect(action, eWorldPropertyPositionHolded, false);
    add_effect(action, eWorldPropertyEnemyDetoured, false);
    add_operator(eWorldOperatorGetReadyToKill, action);

    action = new CStalkerActionGetReadyToKill(false, m_object, "get_ready_to_detour");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyInSmartCover, false);
    add_condition(action, eWorldPropertyItemToKill, true);
    add_condition(action, eWorldPropertyItemCanKill, true);
    add_condition(action, eWorldPropertyReadyToDetour, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyPlayerOnThePath, false);
    add_condition(action, eWorldPropertyLowCover, false);
    //	add_condition			(action,eWorldPropertyShouldThrowGrenade,false);
    add_effect(action, eWorldPropertyReadyToDetour, true);
    add_operator(eWorldOperatorGetReadyToDetour, action);

    action = new CStalkerActionKillEnemy(m_object, "kill_enemy");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertySeeEnemy, true);
    add_condition(action, eWorldPropertyInCover, true);
    add_condition(action, eWorldPropertyInSmartCover, false);
    add_condition(action, eWorldPropertyPanic, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyLowCover, false);
    //	add_condition			(action,eWorldPropertyShouldThrowGrenade,false);
    add_condition(action, eWorldPropertyTooFarToKillEnemy, false);
    add_effect(action, eWorldPropertyPureEnemy, false);
    add_effect(action, eWorldPropertyLookedOut, false);
    add_effect(action, eWorldPropertyPositionHolded, false);
    add_effect(action, eWorldPropertyEnemyDetoured, false);
    add_operator(eWorldOperatorKillEnemy, action);

    action = new CStalkerActionTakeCover(m_object, "take_cover");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyItemToKill, true);
    add_condition(action, eWorldPropertyItemCanKill, true);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertyInCover, false);
    add_condition(action, eWorldPropertyInSmartCover, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyPlayerOnThePath, false);
    //	add_condition			(action,eWorldPropertyTooFarToKillEnemy,false);
    add_effect(action, eWorldPropertyInCover, true);
    add_effect(action, eWorldPropertyLookedOut, false);
    add_effect(action, eWorldPropertyPositionHolded, false);
    add_effect(action, eWorldPropertyEnemyDetoured, false);
    add_operator(eWorldOperatorTakeCover, action);

    action = new CStalkerActionLookOut(m_object, "look_out");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertyInCover, true);
    add_condition(action, eWorldPropertyInSmartCover, false);
    add_condition(action, eWorldPropertyLookedOut, false);
    add_condition(action, eWorldPropertySeeEnemy, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyPlayerOnThePath, false);
    add_condition(action, eWorldPropertyShouldThrowGrenade, false);
    add_condition(action, eWorldPropertyTooFarToKillEnemy, false);
    add_condition(action, eWorldPropertyLowCover, false);
    add_effect(action, eWorldPropertyLookedOut, true);
    add_operator(eWorldOperatorLookOut, action);

    action = new CStalkerActionHoldPosition(m_object, "hold_position");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertyInCover, true);
    add_condition(action, eWorldPropertyInSmartCover, false);
    add_condition(action, eWorldPropertyLookedOut, true);
    add_condition(action, eWorldPropertySeeEnemy, false);
    add_condition(action, eWorldPropertyPositionHolded, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyPlayerOnThePath, false);
    add_condition(action, eWorldPropertyShouldThrowGrenade, false);
    add_condition(action, eWorldPropertyTooFarToKillEnemy, false);
    add_condition(action, eWorldPropertyLowCover, false);
    add_effect(action, eWorldPropertyInCover, false);
    add_effect(action, eWorldPropertyPositionHolded, true);
    add_operator(eWorldOperatorHoldPosition, action);

    action = new CStalkerActionDetourEnemy(m_object, "detour_enemy");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertyReadyToDetour, true);
    add_condition(action, eWorldPropertyInCover, false);
    //	add_condition			(action,eWorldPropertyInSmartCover,		false);
    add_condition(action, eWorldPropertyEnemyDetoured, false);
    add_condition(action, eWorldPropertySeeEnemy, false);
    add_condition(action, eWorldPropertyLookedOut, true);
    add_condition(action, eWorldPropertyPositionHolded, true);
    add_condition(action, eWorldPropertyPanic, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyPlayerOnThePath, false);
    add_condition(action, eWorldPropertyShouldThrowGrenade, false);
    add_condition(action, eWorldPropertyTooFarToKillEnemy, false);
    add_effect(action, eWorldPropertyEnemyDetoured, true);
    add_operator(eWorldOperatorDetourEnemy, action);

    {
        CStalkerSearchPlanner* action = new CStalkerSearchPlanner(m_object, "search enemy planner");
        add_condition(action, eWorldPropertyCriticallyWounded, false);
        add_condition(action, eWorldPropertyDangerGrenade, false);
        add_condition(action, eWorldPropertyUseSuddenness, false);
        add_condition(action, eWorldPropertyReadyToKill, true);
        add_condition(action, eWorldPropertySeeEnemy, false);
        add_condition(action, eWorldPropertyInCover, false);
        add_condition(action, eWorldPropertyLookedOut, true);
        add_condition(action, eWorldPropertyPositionHolded, true);
        add_condition(action, eWorldPropertyEnemyDetoured, true);
        add_condition(action, eWorldPropertyPanic, false);
        add_condition(action, eWorldPropertyEnemyWounded, false);
        add_condition(action, eWorldPropertyPlayerOnThePath, false);
        add_condition(action, eWorldPropertyShouldThrowGrenade, false);
        add_condition(action, eWorldPropertyTooFarToKillEnemy, false);
        add_effect(action, eWorldPropertyPureEnemy, false);
        add_operator(eWorldOperatorSearchEnemy, action);
    }

    action = new CStalkerActionKillEnemy(m_object, "kill_if_not_visible");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertySeeEnemy, true);
    add_condition(action, eWorldPropertyEnemySeeMe, false);
    add_condition(action, eWorldPropertyInSmartCover, false);
    add_condition(action, eWorldPropertyPanic, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyTooFarToKillEnemy, false);
    add_effect(action, eWorldPropertyPureEnemy, false);
    add_operator(eWorldOperatorKillEnemyIfNotVisible, action);

    action = new CStalkerActionKillEnemy(m_object, "kill_if_critically_wounded");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyReadyToKill, true);
    add_condition(action, eWorldPropertyInSmartCover, false);
    add_condition(action, eWorldPropertySeeEnemy, true);
    add_condition(action, eWorldPropertyEnemyCriticallyWounded, true);
    add_condition(action, eWorldPropertyPanic, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyTooFarToKillEnemy, false);
    add_effect(action, eWorldPropertyPureEnemy, false);
    add_operator(eWorldOperatorKillEnemyIfCriticallyWounded, action);

    action = new CStalkerActionPostCombatWait(m_object, "post_combat_wait");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyPureEnemy, false);
    add_condition(action, eWorldPropertyEnemy, true);
    add_effect(action, eWorldPropertyEnemy, false);
    add_operator(eWorldOperatorPostCombatWait, action);

    action = new CStalkerActionHideFromGrenade(m_object, "hide from grenade");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, true);
    add_effect(action, eWorldPropertyEnemy, false);
    add_effect(action, eWorldPropertyInCover, false);
    add_effect(action, eWorldPropertyLookedOut, false);
    add_effect(action, eWorldPropertyPositionHolded, false);
    add_effect(action, eWorldPropertyEnemyDetoured, false);
    add_operator(eWorldOperatorHideFromGrenade, action);

    action = new CStalkerActionSuddenAttack(m_object, "sudden attack");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, true);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyInSmartCover, false);
    add_effect(action, eWorldPropertyEnemy, false);
    add_operator(eWorldOperatorSuddenAttack, action);

    action = new CStalkerActionKillEnemyIfPlayerOnThePath(m_object, "kill enemy, if player is on my path");
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    //	add_condition			(action,eWorldPropertySeeEnemy,			true);
    add_condition(action, eWorldPropertyPanic, false);
    add_condition(action, eWorldPropertyPlayerOnThePath, true);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyTooFarToKillEnemy, false);
    add_effect(action, eWorldPropertyEnemy, false);
    add_effect(action, eWorldPropertyInCover, false);
    add_effect(action, eWorldPropertyLookedOut, false);
    add_effect(action, eWorldPropertyPositionHolded, false);
    add_effect(action, eWorldPropertyEnemyDetoured, false);
    add_operator(eWorldOperatorKillEnemyIfPlayerOnThePath, action);

    {
        CStalkerKillWoundedPlanner* planner = new CStalkerKillWoundedPlanner(m_object, "kill wounded enemy");
        add_condition(planner, eWorldPropertyCriticallyWounded, false);
        add_condition(planner, eWorldPropertyDangerGrenade, false);
        add_condition(planner, eWorldPropertyEnemyWounded, true);
        add_condition(planner, eWorldPropertyEnemy, true);
        add_effect(planner, eWorldPropertyEnemy, false);
        add_operator(eWorldOperatorKillWoundedEnemy, planner);
    }

    action = new CStalkerActionCriticalHit(m_object, "critical hit");
    add_condition(action, eWorldPropertyCriticallyWounded, true);
    add_condition(action, eWorldPropertyPanic, false);
    add_effect(action, eWorldPropertyCriticallyWounded, false);
    add_operator(eWorldOperatorCriticallyWounded, action);

    action = new CStalkerCombatActionThrowGrenade(m_object, "throw_grenade");
    //	add_condition			(action,eWorldPropertyInCover,				true);
    //	add_condition			(action,eWorldPropertySeeEnemy,				false);
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyPanic, false);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyPureEnemy, true);
    add_condition(action, eWorldPropertyShouldThrowGrenade, true);
    add_condition(action, eWorldPropertyLowCover, false);
    add_effect(action, eWorldPropertyShouldThrowGrenade, false);
    add_operator(eWorldOperatorThrowGrenade, action);

    {
        CStalkerGetDistancePlanner* planner = new CStalkerGetDistancePlanner(m_object, "get distance");
        add_condition(planner, eWorldPropertyCriticallyWounded, false);
        add_condition(planner, eWorldPropertyDangerGrenade, false);
        add_condition(planner, eWorldPropertyUseSuddenness, false);
        add_condition(planner, eWorldPropertyReadyToKill, true);
        add_condition(planner, eWorldPropertyInCover, true);
        add_condition(planner, eWorldPropertyPanic, false);
        add_condition(planner, eWorldPropertyEnemyWounded, false);
        add_condition(planner, eWorldPropertyShouldThrowGrenade, false);
        add_condition(planner, eWorldPropertyLowCover, false);
        add_condition(planner, eWorldPropertyTooFarToKillEnemy, true);
        add_effect(planner, eWorldPropertyTooFarToKillEnemy, false);
        add_operator(eWorldOperatorGetDistance, planner);
    }

    {
        stalker_low_cover_planner* planner = new stalker_low_cover_planner(m_object, "use low cover");
        add_condition(planner, eWorldPropertyCriticallyWounded, false);
        add_condition(planner, eWorldPropertyDangerGrenade, false);
        add_condition(planner, eWorldPropertyUseSuddenness, false);
        add_condition(planner, eWorldPropertyItemToKill, true);
        add_condition(planner, eWorldPropertyItemCanKill, true);
        add_condition(planner, eWorldPropertyEnemyWounded, false);
        add_condition(planner, eWorldPropertyPlayerOnThePath, false);
        add_condition(planner, eWorldPropertyInCover, true);
        add_condition(planner, eWorldPropertyLowCover, true);
        add_effect(planner, eWorldPropertyLowCover, false);
        add_operator(eWorldOperatorLowCover, planner);
    }

    action = new CStalkerCombatActionSmartCover(m_object, "smart_cover");
    add_condition(action, eWorldPropertyPureEnemy, true);
    add_condition(action, eWorldPropertyPanic, false);
    add_condition(action, eWorldPropertyCriticallyWounded, false);
    add_condition(action, eWorldPropertyDangerGrenade, false);
    add_condition(action, eWorldPropertyUseSuddenness, false);
    add_condition(action, eWorldPropertyItemToKill, true);
    add_condition(action, eWorldPropertyItemCanKill, true);
    add_condition(action, eWorldPropertyEnemyWounded, false);
    add_condition(action, eWorldPropertyInSmartCover, true);
    add_effect(action, eWorldPropertyInSmartCover, false);
    add_operator(eWorldOperatorInSmartCover, action);
}

void CStalkerCombatPlanner::save(NET_Packet& packet) { inherited::save(packet); }
void CStalkerCombatPlanner::load(IReader& packet) { inherited::load(packet); }
