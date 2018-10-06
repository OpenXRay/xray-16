////////////////////////////////////////////////////////////////////////////
//	Module 		: script_game_object_script3.cpp
//	Created 	: 17.11.2004
//  Modified 	: 17.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Script game object class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "cover_evaluators.h"
#include "cover_point.h"
#include "cover_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_animation_manager.h"
#include "stalker_planner.h"
#include "Weapon.h"
#include "Inventory.h"
#include "CustomZone.h"
#include "patrol_path_manager.h"
#include "object_handler_planner.h"
#include "object_handler_space.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "sound_memory_manager.h"
#include "hit_memory_manager.h"
#include "sight_manager.h"
#include "stalker_movement_manager_smart_cover.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "level_debug.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "trade_parameters.h"
#include "script_ini_file.h"
#include "sound_player.h"
#include "stalker_decision_space.h"
#include "space_restriction_manager.h"
//Alundaio
#ifdef GAME_OBJECT_EXTENDED_EXPORTS
#include "Artefact.h"
#include "holder_custom.h"
#include "Actor.h"
#include "CharacterPhysicsSupport.h"
#include "player_hud.h"
#include "eatable_item.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "xrEngine/Feel_Touch.h"
#endif
//-Alundaio

namespace MemorySpace
{
struct CVisibleObject;
struct CSoundObject;
struct CHitObject;
};

const CCoverPoint* CScriptGameObject::best_cover(const Fvector& position, const Fvector& enemy_position, float radius,
    float min_enemy_distance, float max_enemy_distance)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CGameObject : cannot access class member best_cover!");
        return (0);
    }
    stalker->m_ce_best->setup(enemy_position, min_enemy_distance, max_enemy_distance, 0.f);
    const CCoverPoint* point = ai().cover_manager().best_cover(position, radius, *stalker->m_ce_best);
    return (point);
}

const CCoverPoint* CScriptGameObject::safe_cover(const Fvector& position, float radius, float min_distance)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CGameObject : cannot access class member best_cover!");
        return (0);
    }
    stalker->m_ce_safe->setup(min_distance);
    const CCoverPoint* point = ai().cover_manager().best_cover(position, radius, *stalker->m_ce_safe);
    return (point);
}

const xr_vector<MemorySpace::CVisibleObject>& CScriptGameObject::memory_visible_objects() const
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member memory_visible_objects!");
        NODEFAULT;
    }
    return (monster->memory().visual().objects());
}

const xr_vector<MemorySpace::CSoundObject>& CScriptGameObject::memory_sound_objects() const
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member memory_sound_objects!");
        NODEFAULT;
    }
    return (monster->memory().sound().objects());
}

const xr_vector<MemorySpace::CHitObject>& CScriptGameObject::memory_hit_objects() const
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member memory_hit_objects!");
        NODEFAULT;
    }
    return (monster->memory().hit().objects());
}

void CScriptGameObject::ChangeTeam(u8 team, u8 squad, u8 group)
{
    CCustomMonster* custom_monster = smart_cast<CCustomMonster*>(&object());
    if (!custom_monster)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster: cannot access class member ChangeTeam!");
    else
        custom_monster->ChangeTeam(team, squad, group);
}

void CScriptGameObject::SetVisualMemoryEnabled(bool enabled)
{
    CCustomMonster* custom_monster = smart_cast<CCustomMonster*>(&object());
    if (!custom_monster)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster: cannot access class member ChangeTeam!");
    else
        custom_monster->memory().visual().enable(enabled);
}

CScriptGameObject* CScriptGameObject::GetEnemy() const
{
    CCustomMonster* l_tpCustomMonster = smart_cast<CCustomMonster*>(&object());
    if (l_tpCustomMonster && l_tpCustomMonster->g_Alive())
    {
        if (l_tpCustomMonster->GetCurrentEnemy() && !l_tpCustomMonster->GetCurrentEnemy()->getDestroy())
            return (l_tpCustomMonster->GetCurrentEnemy()->lua_game_object());
        else
            return (0);
    }
    else
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member GetEnemy!");
        return (0);
    }
}

CScriptGameObject* CScriptGameObject::GetCorpse() const
{
    CCustomMonster* l_tpCustomMonster = smart_cast<CCustomMonster*>(&object());
    if (l_tpCustomMonster)
        if (l_tpCustomMonster->GetCurrentCorpse() && !l_tpCustomMonster->GetCurrentCorpse()->getDestroy())
            return (l_tpCustomMonster->GetCurrentCorpse()->lua_game_object());
        else
            return (0);
    else
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member GetCorpse!");
        return (0);
    }
}

bool CScriptGameObject::CheckTypeVisibility(const char* section_name)
{
    CCustomMonster* l_tpCustomMonster = smart_cast<CCustomMonster*>(&object());
    if (l_tpCustomMonster)
        return (l_tpCustomMonster->CheckTypeVisibility(section_name));
    else
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member CheckTypeVisibility!");
        return (false);
    }
}

CScriptGameObject* CScriptGameObject::GetCurrentWeapon() const
{
    CAI_Stalker* l_tpStalker = smart_cast<CAI_Stalker*>(&object());
    if (!l_tpStalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member GetCurrentWeapon!");
        return (0);
    }
    CGameObject* current_weapon = l_tpStalker->GetCurrentWeapon();
    return (current_weapon ? current_weapon->lua_game_object() : 0);
}

void CScriptGameObject::deadbody_closed(bool status)
{
    CInventoryOwner* inventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!inventoryOwner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member deadbody_closed!");
        return;
    }
    inventoryOwner->deadbody_closed(status);
}

bool CScriptGameObject::deadbody_closed_status()
{
    CInventoryOwner* inventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!inventoryOwner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member deadbody_closed_status!");
        return (0);
    }
    return inventoryOwner->deadbody_closed_status();
}

void CScriptGameObject::can_select_weapon(bool status)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member can_select_weapon!");
        return;
    }
    stalker->can_select_weapon(status);
}

bool CScriptGameObject::can_select_weapon() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member can_select_weapon!");
        return (0);
    }
    return stalker->can_select_weapon();
}

void CScriptGameObject::deadbody_can_take(bool status)
{
    CInventoryOwner* inventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!inventoryOwner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member deadbody_can_take!");
        return;
    }
    inventoryOwner->deadbody_can_take(status);
}

bool CScriptGameObject::deadbody_can_take_status()
{
    CInventoryOwner* inventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!inventoryOwner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member deadbody_can_take_status!");
        return (0);
    }
    return inventoryOwner->deadbody_can_take_status();
}
#include "CustomOutfit.h"

CScriptGameObject* CScriptGameObject::GetCurrentOutfit() const
{
    CInventoryOwner* inventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!inventoryOwner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member GetCurrentOutfit!");
        return (0);
    }
    CGameObject* current_equipment = inventoryOwner->GetOutfit();
    return (current_equipment ? current_equipment->lua_game_object() : 0);
}

float CScriptGameObject::GetCurrentOutfitProtection(int hit_type)
{
    CInventoryOwner* inventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (!inventoryOwner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member GetCurrentOutfitProtection!");
        return (0);
    }
    CGameObject* current_equipment = inventoryOwner->GetOutfit();
    CCustomOutfit* o = smart_cast<CCustomOutfit*>(current_equipment);
    if (!o)
        return 0.0f;

    return o->GetDefHitTypeProtection(ALife::EHitType(hit_type));
}

CScriptGameObject* CScriptGameObject::GetFood() const
{
    CAI_Stalker* l_tpStalker = smart_cast<CAI_Stalker*>(&object());
    if (!l_tpStalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member GetFood!");
        return (0);
    }
    CGameObject* food = l_tpStalker->GetFood() ? &l_tpStalker->GetFood()->object() : 0;
    return (food ? food->lua_game_object() : 0);
}

CScriptGameObject* CScriptGameObject::GetMedikit() const
{
    CAI_Stalker* l_tpStalker = smart_cast<CAI_Stalker*>(&object());
    if (!l_tpStalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member GetCurrentWeapon!");
        return (0);
    }
    CGameObject* medikit = l_tpStalker->GetMedikit() ? &l_tpStalker->GetMedikit()->object() : 0;
    return (medikit ? medikit->lua_game_object() : 0);
}

LPCSTR CScriptGameObject::GetPatrolPathName()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        CScriptEntity* script_monster = smart_cast<CScriptEntity*>(&object());
        if (!script_monster)
        {
            GEnv.ScriptEngine->script_log(
                LuaMessageType::Error, "CGameObject : cannot access class member GetPatrolPathName!");
            return ("");
        }
        else
            return (script_monster->GetPatrolPathName());
    }
    else
        return (*stalker->movement().patrol().path_name());
}

void CScriptGameObject::add_animation(LPCSTR animation, bool hand_usage, bool use_movement_controller)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member add_animation!");
        return;
    }

    if (stalker->movement().current_params().cover())
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "Cannot add animation [%s]: object [%s] is in smart_cover!", animation, stalker->cName().c_str());
    }

    if (stalker->animation().global_selector())
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "Cannot add animation [%s]: global selector is set for object [%s], in_smart_cover returned [%s]!",
            animation, stalker->cName().c_str(), in_smart_cover() ? "true" : "false");
        return;
    }

    stalker->animation().add_script_animation(animation, hand_usage, use_movement_controller);
}

void CScriptGameObject::add_animation(
    LPCSTR animation, bool hand_usage, Fvector position, Fvector rotation, bool local_animation)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member add_animation!");
        return;
    }

    if (stalker->movement().current_params().cover())
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "Cannot add animation [%s]: object [%s] is in smart_cover!", animation, stalker->cName().c_str());
    }

    if (stalker->animation().global_selector())
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "Cannot add animation [%s]: global selector is set for object [%s], in_smart_cover returned [%s]!",
            animation, stalker->cName().c_str(), in_smart_cover() ? "true" : "false");
        return;
    }

    stalker->animation().add_script_animation(animation, hand_usage, position, rotation, local_animation);
}

void CScriptGameObject::clear_animations()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member clear_animations!");
        return;
    }
    stalker->animation().clear_script_animations();
}

int CScriptGameObject::animation_count() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member clear_animations!");
        return (-1);
    }
    return ((int)stalker->animation().script_animations().size());
}

Flags32 CScriptGameObject::get_actor_relation_flags() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    THROW(stalker);

    return stalker->m_actor_relation_flags;
}

void CScriptGameObject::set_actor_relation_flags(Flags32 flags)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    THROW(stalker);
    stalker->m_actor_relation_flags = flags;
}

void CScriptGameObject::set_patrol_path(
    LPCSTR path_name, const EPatrolStartType patrol_start_type, const EPatrolRouteType patrol_route_type, bool random)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
        stalker->movement().patrol().set_path(path_name, patrol_start_type, patrol_route_type, random);
}

void CScriptGameObject::inactualize_patrol_path()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
        stalker->movement().patrol().make_inactual();
}

void CScriptGameObject::set_dest_level_vertex_id(u32 level_vertex_id)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_dest_level_vertex_id!");
    else
    {
        if (!ai().level_graph().valid_vertex_id(level_vertex_id))
        {
#ifdef DEBUG
            GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                "CAI_Stalker : invalid vertex id being setup by action %s!",
                stalker->brain().CStalkerPlanner::current_action().m_action_name);
#endif
            return;
        }
        if (!stalker->movement().restrictions().accessible(level_vertex_id))
        {
            GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                "! you are trying to setup destination for the stalker %s, which is not accessible by its restrictors "
                "in[%s] out[%s]",
                stalker->cName().c_str(), Level().space_restriction_manager().in_restrictions(stalker->ID()).c_str(),
                Level().space_restriction_manager().out_restrictions(stalker->ID()).c_str());
            return;
        }
        stalker->movement().set_level_dest_vertex(level_vertex_id);
    }
}

void CScriptGameObject::set_dest_game_vertex_id(GameGraph::_GRAPH_ID game_vertex_id)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_dest_game_vertex_id!");
    else
    {
        if (!ai().game_graph().valid_vertex_id(game_vertex_id))
        {
#ifdef DEBUG
            GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                "CAI_Stalker : invalid vertex id being setup by action %s!",
                stalker->brain().CStalkerPlanner::current_action().m_action_name);
#endif
            return;
        }
        stalker->movement().set_game_dest_vertex(game_vertex_id);
    }
}
void CScriptGameObject::set_movement_selection_type(ESelectionType selection_type)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member set_movement_selection_type!");
    stalker->movement().game_selector().set_selection_type(selection_type);
}

CHARACTER_RANK_VALUE CScriptGameObject::GetRank()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member GetRank!");
        return (CHARACTER_RANK_VALUE(0));
    }
    else
        return (stalker->Rank());
}

void CScriptGameObject::set_desired_position()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
        stalker->movement().set_desired_position(0);
}

void CScriptGameObject::set_desired_position(const Fvector* desired_position)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
    {
        THROW2(desired_position || stalker->movement().restrictions().accessible(*desired_position), *stalker->cName());
        stalker->movement().set_desired_position(desired_position);
    }
}

void CScriptGameObject::set_desired_direction()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
        stalker->movement().set_desired_direction(0);
}

void CScriptGameObject::set_desired_direction(const Fvector* desired_direction)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
    {
        if (fsimilar(desired_direction->magnitude(), 0.f))
            GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                "CAI_Stalker : [%s] set_desired_direction - you passed zero direction!", stalker->cName().c_str());
        else
        {
            if (!fsimilar(desired_direction->magnitude(), 1.f))
                GEnv.ScriptEngine->script_log(LuaMessageType::Error,
                    "CAI_Stalker : [%s] set_desired_direction - you passed non-normalized direction!",
                    stalker->cName().c_str());
        }

        Fvector direction = *desired_direction;
        direction.normalize_safe();
        stalker->movement().set_desired_direction(&direction);
    }
}

void CScriptGameObject::set_body_state(EBodyState body_state)
{
    THROW((body_state == eBodyStateStand) || (body_state == eBodyStateCrouch));
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
        stalker->movement().set_body_state(body_state);
}

void CScriptGameObject::set_movement_type(EMovementType movement_type)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
        stalker->movement().set_movement_type(movement_type);
}

void CScriptGameObject::set_mental_state(EMentalState mental_state)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
    {
#if 0 // def DEBUG
		if (mental_state != eMentalStateDanger) {
			if (stalker->brain().initialized()) {
				if (stalker->brain().current_action_id() == StalkerDecisionSpace::eWorldOperatorCombatPlanner) {
					GEnv.ScriptEngine->script_log	(LuaMessageType::Error,"CAI_Stalker : set_mental_state is used during universal combat!, object[%s]", stalker->cName().c_str());
//					return;
				}
			}
		}
#endif // DEBUG
        stalker->movement().set_mental_state(mental_state);
    }
}

void CScriptGameObject::set_path_type(MovementManager::EPathType path_type)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
        stalker->movement().set_path_type(path_type);
}

void CScriptGameObject::set_detail_path_type(DetailPathManager::EDetailPathType detail_path_type)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member movement!");
    else
        stalker->movement().set_detail_path_type(detail_path_type);
}

MonsterSpace::EBodyState CScriptGameObject::body_state() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member body_state!");
        return (MonsterSpace::eBodyStateStand);
    }
    return (stalker->movement().body_state());
}

MonsterSpace::EBodyState CScriptGameObject::target_body_state() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member body_state!");
        return (MonsterSpace::eBodyStateStand);
    }
    return (stalker->movement().target_body_state());
}

MonsterSpace::EMovementType CScriptGameObject::movement_type() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member movement_type!");
        return (MonsterSpace::eMovementTypeStand);
    }
    return (stalker->movement().movement_type());
}

MonsterSpace::EMovementType CScriptGameObject::target_movement_type() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member target_movement_type!");
        return (MonsterSpace::eMovementTypeStand);
    }
    return (stalker->movement().target_movement_type());
}

MonsterSpace::EMentalState CScriptGameObject::mental_state() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member mental_state!");
        return (MonsterSpace::eMentalStateDanger);
    }
    return (stalker->movement().mental_state());
}

MonsterSpace::EMentalState CScriptGameObject::target_mental_state() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member mental_state!");
        return (MonsterSpace::eMentalStateDanger);
    }
    return (stalker->movement().target_mental_state());
}

MovementManager::EPathType CScriptGameObject::path_type() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member path_type!");
        return (MovementManager::ePathTypeNoPath);
    }
    return (stalker->movement().path_type());
}

DetailPathManager::EDetailPathType CScriptGameObject::detail_path_type() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member detail_path_type!");
        return (DetailPathManager::eDetailPathTypeSmooth);
    }
    return (DetailPathManager::eDetailPathTypeSmooth);
}

void CScriptGameObject::set_sight(SightManager::ESightType sight_type, Fvector* vector3d, u32 dwLookOverDelay)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSightManager : cannot access class member set_sight!");
    else
    {
        if ((sight_type == SightManager::eSightTypeDirection) && vector3d && (_abs(vector3d->magnitude() - 1.f) > .01f))
        {
            VERIFY2(false, make_string("non-normalized direction passed [%f][%f][%f]", VPUSH(*vector3d)));
            vector3d->normalize();
        }

        stalker->sight().setup(sight_type, vector3d);
    }
}

void CScriptGameObject::set_sight(SightManager::ESightType sight_type, bool torso_look, bool path)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSightManager : cannot access class member set_sight!");
    else
        stalker->sight().setup(sight_type, torso_look, path);
}

void CScriptGameObject::set_sight(SightManager::ESightType sight_type, Fvector& vector3d, bool torso_look = false)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());

    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSightManager : cannot access class member set_sight!");
    else
    {
        //morrey
        // не работала чн-кая анимация
        if ((sight_type == SightManager::eSightTypeDirection) && (_abs(vector3d.magnitude() - 1.f) > .01f))
        {
            // Xottab_DUTY to Xottab_DUTY: здесь закомментировано две строчки.
            //VERIFY2(false, make_string("non-normalized direction passed [%f][%f][%f]", VPUSH(vector3d)));
            Msg("- non-normalized direction passed [%f][%f][%f]", VPUSH(vector3d));
            //vector3d.normalize();
        }
        //-morrey

        stalker->sight().setup(sight_type, vector3d, torso_look);
    }
}

void CScriptGameObject::set_sight(SightManager::ESightType sight_type, Fvector* vector3d)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSightManager : cannot access class member set_sight!");
    else
    {
        if ((sight_type == SightManager::eSightTypeDirection) && vector3d && (_abs(vector3d->magnitude() - 1.f) > .01f))
        {
            VERIFY2(false, make_string("non-normalized direction passed [%f][%f][%f]", VPUSH(*vector3d)));
            vector3d->normalize();
        }

        stalker->sight().setup(sight_type, vector3d);
    }
}

void CScriptGameObject::set_sight(CScriptGameObject* object_to_look)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSightManager : cannot access class member set_sight!");
    else
        stalker->sight().setup(&object_to_look->object());
}

void CScriptGameObject::set_sight(CScriptGameObject* object_to_look, bool torso_look)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSightManager : cannot access class member set_sight!");
    else
        stalker->sight().setup(&object_to_look->object(), torso_look);
}

void CScriptGameObject::set_sight(CScriptGameObject* object_to_look, bool torso_look, bool fire_object)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSightManager : cannot access class member set_sight!");
    else
        stalker->sight().setup(&object_to_look->object(), torso_look, fire_object);
}

void CScriptGameObject::set_sight(CScriptGameObject* object_to_look, bool torso_look, bool fire_object, bool no_pitch)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSightManager : cannot access class member set_sight!");
    else
        stalker->sight().setup(CSightAction(&object_to_look->object(), torso_look, fire_object, no_pitch));
}

void CScriptGameObject::set_sight(const CMemoryInfo* memory_object, bool torso_look)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSightManager : cannot access class member set_sight!");
    else
        stalker->sight().setup(memory_object, torso_look);
}

// CAI_Stalker
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

u32 CScriptGameObject::GetInventoryObjectCount() const
{
    CInventoryOwner* l_tpInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (l_tpInventoryOwner)
        return (l_tpInventoryOwner->inventory().dwfGetObjectCount());
    else
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member obj_count!");
        return (0);
    }
}

CScriptGameObject* CScriptGameObject::GetActiveItem()
{
    CInventoryOwner* l_tpInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (l_tpInventoryOwner)
        if (l_tpInventoryOwner->inventory().ActiveItem())
            return (l_tpInventoryOwner->inventory().ActiveItem()->object().lua_game_object());
        else
            return (0);
    else
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member activge_item!");
        return (0);
    }
}

CScriptGameObject* CScriptGameObject::GetObjectByName(LPCSTR caObjectName) const
{
    CInventoryOwner* l_tpInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (l_tpInventoryOwner)
    {
        CInventoryItem* l_tpInventoryItem = l_tpInventoryOwner->inventory().GetItemFromInventory(caObjectName);
        CGameObject* l_tpGameObject = smart_cast<CGameObject*>(l_tpInventoryItem);
        if (!l_tpGameObject)
            return (0);
        else
            return (l_tpGameObject->lua_game_object());
    }
    else
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member object!");
        return (0);
    }
}

CScriptGameObject* CScriptGameObject::GetObjectByIndex(int iIndex) const
{
    CInventoryOwner* l_tpInventoryOwner = smart_cast<CInventoryOwner*>(&object());
    if (l_tpInventoryOwner)
    {
        CInventoryItem* l_tpInventoryItem = l_tpInventoryOwner->inventory().tpfGetObjectByIndex(iIndex);
        CGameObject* l_tpGameObject = smart_cast<CGameObject*>(l_tpInventoryItem);
        if (!l_tpGameObject)
            return (0);
        else
            return (l_tpGameObject->lua_game_object());
    }
    else
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member object!");
        return (0);
    }
}

void CScriptGameObject::EnableAnomaly()
{
    CCustomZone* zone = smart_cast<CCustomZone*>(&object());
    THROW(zone);
    zone->ZoneEnable();
}

void CScriptGameObject::DisableAnomaly()
{
    CCustomZone* zone = smart_cast<CCustomZone*>(&object());
    THROW(zone);
    zone->ZoneDisable();
}

float CScriptGameObject::GetAnomalyPower()
{
    CCustomZone* zone = smart_cast<CCustomZone*>(&object());
    THROW(zone);
    return zone->GetMaxPower();
}

void CScriptGameObject::SetAnomalyPower(float p)
{
    CCustomZone* zone = smart_cast<CCustomZone*>(&object());
    THROW(zone);
    zone->SetMaxPower(p);
}

bool CScriptGameObject::weapon_strapped() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member weapon_strapped!");
        return (false);
    }

    bool const result = stalker->weapon_strapped();
    //	Msg					( "[%6d][%s] weapon_strapped = %s", Device.dwTimeGlobal, stalker->cName().c_str(), result ?
    //"true"
    //: "false" );
    return (result);
}

bool CScriptGameObject::weapon_unstrapped() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member weapon_unstrapped!");
        return (false);
    }
    bool const result = stalker->weapon_unstrapped();
    //	Msg					( "[%6d][%s] weapon_unstrapped = %s", Device.dwTimeGlobal, stalker->cName().c_str(), result
    //?
    //"true" : "false" );
    return (result);
}

bool CScriptGameObject::path_completed() const
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member path_completed!");
        return (false);
    }
    return (monster->movement().path_completed());
}

void CScriptGameObject::patrol_path_make_inactual()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member patrol_path_make_inactual!");
        return;
    }
    monster->movement().patrol().make_inactual();
}

Fvector CScriptGameObject::head_orientation() const
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member head_orientation!");
        return (Fvector().set(flt_max, flt_max, flt_max));
    }
    const SRotation& r = stalker->movement().head_orientation().current;
    return (Fvector().setHP(-r.yaw, -r.pitch));
}

void CScriptGameObject::info_add(LPCSTR text)
{
#ifdef DEBUG
    DBG().object_info(&object(), this).add_item(text, color_xrgb(255, 0, 0), 0);
#endif
}

void CScriptGameObject::info_clear()
{
#ifdef DEBUG
    DBG().object_info(&object(), this).clear();
#endif
}

void CScriptGameObject::jump(const Fvector& position, float factor)
{
    CBaseMonster* monster = smart_cast<CBaseMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot process jump for not a monster!");
        return;
    }

    monster->jump(position, factor);
}

void CScriptGameObject::make_object_visible_somewhen(CScriptGameObject* object)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&this->object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member make_object_visible_somewhen!");
        return;
    }

    CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(&object->object());
    if (!entity_alive)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CEntityAlive : cannot access class member make_object_visible_somewhen!");
        return;
    }

    stalker->memory().make_object_visible_somewhen(entity_alive);
}

void CScriptGameObject::sell_condition(CScriptIniFile* ini_file, LPCSTR section)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member sell_condition!");
        return;
    }

    inventory_owner->trade_parameters().process(CTradeParameters::action_sell(0), *ini_file, section);
}

void CScriptGameObject::sell_condition(float friend_factor, float enemy_factor)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member sell_condition!");
        return;
    }

    inventory_owner->trade_parameters().default_factors(
        CTradeParameters::action_sell(0), CTradeFactors(friend_factor, enemy_factor));
}

void CScriptGameObject::buy_condition(CScriptIniFile* ini_file, LPCSTR section)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member buy_condition!");
        return;
    }

    inventory_owner->trade_parameters().process(CTradeParameters::action_buy(0), *ini_file, section);
}

void CScriptGameObject::buy_condition(float friend_factor, float enemy_factor)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member buy_condition!");
        return;
    }

    inventory_owner->trade_parameters().default_factors(
        CTradeParameters::action_buy(0), CTradeFactors(friend_factor, enemy_factor));
}

void CScriptGameObject::show_condition(CScriptIniFile* ini_file, LPCSTR section)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member show_condition!");
        return;
    }

    inventory_owner->trade_parameters().process(CTradeParameters::action_show(0), *ini_file, section);
}

void CScriptGameObject::buy_supplies(CScriptIniFile* ini_file, LPCSTR section)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member buy_condition!");
        return;
    }

    inventory_owner->buy_supplies(*ini_file, section);
}

void CScriptGameObject::buy_item_condition_factor(float factor)
{
    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CInventoryOwner : cannot access class member buy_item_condition_factor!");
        return;
    }

    inventory_owner->trade_parameters().buy_item_condition_factor = factor;
}

void sell_condition(CScriptIniFile* ini_file, LPCSTR section)
{
    default_trade_parameters().process(CTradeParameters::action_sell(0), *ini_file, section);
}

void sell_condition(float friend_factor, float enemy_factor)
{
    default_trade_parameters().default_factors(
        CTradeParameters::action_sell(0), CTradeFactors(friend_factor, enemy_factor));
}

void buy_condition(CScriptIniFile* ini_file, LPCSTR section)
{
    default_trade_parameters().process(CTradeParameters::action_buy(0), *ini_file, section);
}

void buy_condition(float friend_factor, float enemy_factor)
{
    default_trade_parameters().default_factors(
        CTradeParameters::action_buy(0), CTradeFactors(friend_factor, enemy_factor));
}

void show_condition(CScriptIniFile* ini_file, LPCSTR section)
{
    default_trade_parameters().process(CTradeParameters::action_show(0), *ini_file, section);
}

LPCSTR CScriptGameObject::sound_prefix() const
{
    CCustomMonster* custom_monster = smart_cast<CCustomMonster*>(&object());
    if (!custom_monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member sound_prefix!");
        return (0);
    }

    return (*custom_monster->sound().sound_prefix());
}

void CScriptGameObject::sound_prefix(LPCSTR sound_prefix)
{
    CCustomMonster* custom_monster = smart_cast<CCustomMonster*>(&object());
    if (!custom_monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member sound_prefix!");
        return;
    }

    custom_monster->sound().sound_prefix(sound_prefix);
}

bool CScriptGameObject::is_weapon_going_to_be_strapped(CScriptGameObject const* object) const
{
    if (!object)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error,
            "CGameObject : cannot access class member is_weapon_going_to_be_strapped (object passed is null)!");
        return false;
    }

    CAI_Stalker const* stalker = smart_cast<CAI_Stalker const*>(&this->object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member is_weapon_going_to_be_strapped!");
        return false;
    }

    return stalker->is_weapon_going_to_be_strapped(&object->object());
}

//Alundaio: Taken from Radium
#ifdef GAME_OBJECT_EXTENDED_EXPORTS
float CScriptGameObject::GetArtefactHealthRestoreSpeed()
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    return artefact->GetHealthPower();
}

float CScriptGameObject::GetArtefactRadiationRestoreSpeed()
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    return artefact->GetRadiationPower();
}

float CScriptGameObject::GetArtefactSatietyRestoreSpeed()
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    return artefact->GetSatietyPower();
}

float CScriptGameObject::GetArtefactPowerRestoreSpeed()
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    return artefact->GetPowerPower();
}

float CScriptGameObject::GetArtefactBleedingRestoreSpeed()
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    return artefact->GetBleedingPower();
}

void CScriptGameObject::SetArtefactHealthRestoreSpeed(float value)
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    artefact->SetHealthPower(value);
}

void CScriptGameObject::SetArtefactRadiationRestoreSpeed(float value)
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    artefact->SetRadiationPower(value);
}

void CScriptGameObject::SetArtefactSatietyRestoreSpeed(float value)
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    artefact->SetSatietyPower(value);
}

void CScriptGameObject::SetArtefactPowerRestoreSpeed(float value)
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    artefact->SetPowerPower(value);
}

void CScriptGameObject::SetArtefactBleedingRestoreSpeed(float value)
{
    CArtefact* artefact = smart_cast<CArtefact*>(&object());
    THROW(artefact);

    artefact->SetBleedingPower(value);
}

void CScriptGameObject::AttachVehicle(CScriptGameObject* veh)
{
    CActor* actor = smart_cast<CActor*>(&object());
    if (actor)
    {
        CHolderCustom* vehicle = veh->object().cast_holder_custom();//smart_cast<CHolderCustom*>(veh->object());
        if (vehicle)
            actor->attach_Vehicle(vehicle);
    }
}

void CScriptGameObject::DetachVehicle()
{
    CActor* actor = smart_cast<CActor*>(&object());
    if (actor)
        actor->detach_Vehicle();
}

CScriptGameObject* CScriptGameObject::GetAttachedVehicle()
{
    CActor* actor = smart_cast<CActor*>(&object());
    if (!actor)
        return nullptr;

    CHolderCustom* H = actor->Holder();
    if (!H)
        return nullptr;

    CGameObject* GO = smart_cast<CGameObject*>(H);
    if (!GO)
        return nullptr;

    return GO->lua_game_object();
}

u32 CScriptGameObject::PlayHudMotion(pcstr M, bool mixIn, u32 state)
{
    CWeapon* Weapon = object().cast_weapon();
    if (Weapon)
    {
        if (!Weapon->isHUDAnimationExist(M))
            return 0;

        return Weapon->PlayHUDMotion(M, mixIn, Weapon, state);
    }

    CHudItem* itm = object().cast_inventory_item()->cast_hud_item();
    if (!itm)
        return 0;

    if (!itm->isHUDAnimationExist(M))
        return 0;

    return itm->PlayHUDMotion(M, mixIn, itm, state);
}

void CScriptGameObject::SwitchState(u32 state)
{
    CWeapon* Weapon = object().cast_weapon();
    if (Weapon)
    {
        Weapon->SwitchState(state);
        return;
    }

    CInventoryItem* IItem = object().cast_inventory_item();
    if (IItem)
    {
        CHudItem* itm = IItem->cast_hud_item();
        if (itm)
            itm->SwitchState(state);
    }
}

u32 CScriptGameObject::GetState()
{
    CWeapon* Weapon = object().cast_weapon();
    if (Weapon)
        return Weapon->GetState();

    CInventoryItem* IItem = object().cast_inventory_item();
    if (IItem)
    {
        CHudItem* itm = IItem->cast_hud_item();
        if (itm)
            return itm->GetState();
    }

    return 65535;
}

void CScriptGameObject::SetBoneVisible(pcstr bone_name, bool bVisibility, bool bRecursive)
{
    IKinematics* k = object().Visual()->dcast_PKinematics();

    if (!k)
        return;

    u16 bone_id = k->LL_BoneID(bone_name);
    if (bone_id == BI_NONE)
        return;

    if (bVisibility == !k->LL_GetBoneVisible(bone_id))
        k->LL_SetBoneVisible(bone_id, bVisibility, bRecursive);
}

bool CScriptGameObject::IsBoneVisible(pcstr bone_name)
{
    IKinematics* k = object().Visual()->dcast_PKinematics();

    if (!k)
        return false;

    u16 bone_id = k->LL_BoneID(bone_name);
    if (bone_id == BI_NONE)
        return false;

    return k->LL_GetBoneVisible(bone_id);
}

float CScriptGameObject::GetLuminocityHemi()
{
    CGameObject* e = smart_cast<CGameObject*>(&object());
    if (!e || !e->renderable_ROS())
        return 0;
    return e->renderable_ROS()->get_luminocity_hemi();
}

float CScriptGameObject::GetLuminocity()
{
    CGameObject* e = smart_cast<CGameObject*>(&object());
    if (!e || !e->renderable_ROS())
        return 0;
    return e->renderable_ROS()->get_luminocity();
}

void CScriptGameObject::ForceSetPosition(Fvector pos, bool bActivate)
{
    CPhysicsShellHolder* sh = object().cast_physics_shell_holder();
    if (!sh)
        return;

    CPhysicsShell* shell = sh->PPhysicsShell();
    if (shell)
    {
        if (bActivate)
            sh->activate_physic_shell();

        Fmatrix M = object().XFORM();
        M.c = pos;
        M.set(M);

        shell->SetGlTransformDynamic(M);
        if (sh->character_physics_support())
            sh->character_physics_support()->ForceTransform(M);
    }
    else
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "force_set_position: object %s has no physics shell!",
                                        *object().cName());
}

void CScriptGameObject::SetRemainingUses(u8 value)
{
    CInventoryItem* IItm = object().cast_inventory_item();
    if (!IItm)
        return;

    CEatableItem* eItm = IItm->cast_eatable_item();
    if (!eItm)
        return;

    eItm->SetRemainingUses(value);
}

u8 CScriptGameObject::GetRemainingUses()
{
    CInventoryItem* IItm = object().cast_inventory_item();
    if (!IItm)
        return 0;

    CEatableItem* eItm = IItm->cast_eatable_item();
    if (!eItm)
        return 0;

    return eItm->GetRemainingUses();
}

u8 CScriptGameObject::GetMaxUses()
{
    CInventoryItem* IItm = object().cast_inventory_item();
    if (!IItm)
        return 0;

    CEatableItem* eItm = IItm->cast_eatable_item();
    if (!eItm)
        return 0;

    return eItm->GetMaxUses();
}

void CScriptGameObject::IterateFeelTouch(luabind::functor<void> functor)
{
    Feel::Touch* touch = smart_cast<Feel::Touch*>(&object());
    if (touch)
    {
        for (const auto& game_object : touch->feel_touch)
        {
            // Xottab_DUTY: Do we need this cast from IGameObject* to IGameObject* ?
            IGameObject* o = smart_cast<IGameObject*>(game_object);
            if (o)
                functor(game_object->ID());
        }
    }
}

void CScriptGameObject::SetSpatialType(u32 sptype)
{
    object().spatial.type = sptype;
}

u32 CScriptGameObject::GetSpatialType()
{
    return object().spatial.type;
}

u8 CScriptGameObject::GetRestrictionType()
{
    CSpaceRestrictor* restr = smart_cast<CSpaceRestrictor*>(&object());
    if (restr)
        return restr->m_space_restrictor_type;

    return -1;
}

void CScriptGameObject::SetRestrictionType(u8 type)
{
    CSpaceRestrictor* restr = smart_cast<CSpaceRestrictor*>(&object());
    if (restr)
    {
        restr->m_space_restrictor_type = type;
        if (type != RestrictionSpace::eRestrictorTypeNone)
            Level().space_restriction_manager().register_restrictor(restr, RestrictionSpace::ERestrictorTypes(type));
    }
}
#endif
//-Alundaio
