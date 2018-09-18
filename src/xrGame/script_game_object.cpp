////////////////////////////////////////////////////////////////////////////
//	Module 		: script_game_object.cpp
//	Created 	: 25.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script game object class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "script_entity_action.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "script_entity.h"
#include "PhysicsShellHolder.h"
#include "helicopter.h"
#include "holder_custom.h"
#include "InventoryOwner.h"
#include "movement_manager.h"
#include "entity_alive.h"
#include "WeaponMagazined.h"
#include "xrMessages.h"
#include "Inventory.h"
#include "script_ini_file.h"
#include "Include/xrRender/Kinematics.h"
#include "HangingLamp.h"
#include "patrol_path_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "CustomMonster.h"
#include "EntityCondition.h"
#include "space_restrictor.h"
#include "detail_path_manager.h"
#include "xrAICore/Navigation/level_graph.h"
#include "Actor.h"
#include "actor_memory.h"
#include "visual_memory_manager.h"
#include "smart_cover_object.h"
#include "smart_cover.h"
#include "smart_cover_description.h"
#include "physics_shell_scripted.h"
#include "CharacterPhysicsSupport.h" //Alundaio: For set_visual
#include "damage_manager.h" //Alundaio: For set_visual
#include "ai/phantom/phantom.h"

#include "UIGameCustom.h"
#include "ui/UIActorMenu.h"
#include "InventoryBox.h"

class CScriptBinderObject;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
Fvector CScriptGameObject::Center()
{
    Fvector c;
    m_game_object->Center(c);
    return c;
}

BIND_FUNCTION10(&object(), CScriptGameObject::Position, CGameObject, Position, Fvector, Fvector());
BIND_FUNCTION10(&object(), CScriptGameObject::Direction, CGameObject, Direction, Fvector, Fvector());
BIND_FUNCTION10(&object(), CScriptGameObject::Mass, CPhysicsShellHolder, GetMass, float, float(-1));
BIND_FUNCTION10(&object(), CScriptGameObject::ID, CGameObject, ID, u16, u16(-1));
BIND_FUNCTION10(&object(), CScriptGameObject::getVisible, CGameObject, getVisible, BOOL, FALSE);
// BIND_FUNCTION01	(&object(),	CScriptGameObject::setVisible,			CGameObject,	setVisible,			BOOL,
// BOOL);
BIND_FUNCTION10(&object(), CScriptGameObject::getEnabled, CGameObject, getEnabled, BOOL, FALSE);
// BIND_FUNCTION01	(&object(),	CScriptGameObject::setEnabled,			CGameObject,	setEnabled,			BOOL,
// BOOL);
BIND_FUNCTION10(&object(), CScriptGameObject::story_id, CGameObject, story_id, ALife::_STORY_ID, ALife::_STORY_ID(-1));
BIND_FUNCTION10(&object(), CScriptGameObject::DeathTime, CEntity, GetLevelDeathTime, u32, 0);
BIND_FUNCTION10(&object(), CScriptGameObject::MaxHealth, CEntity, GetMaxHealth, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::Accuracy, CInventoryOwner, GetWeaponAccuracy, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::Team, CEntity, g_Team, int, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::Squad, CEntity, g_Squad, int, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::Group, CEntity, g_Group, int, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetFOV, CEntityAlive, ffGetFov, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetRange, CEntityAlive, ffGetRange, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetHealth, CEntityAlive, conditions().GetHealth, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetPsyHealth, CEntityAlive, conditions().GetPsyHealth, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetPower, CEntityAlive, conditions().GetPower, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetSatiety, CEntityAlive, conditions().GetSatiety, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetRadiation, CEntityAlive, conditions().GetRadiation, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetBleeding, CEntityAlive, conditions().BleedingSpeed, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetMorale, CEntityAlive, conditions().GetEntityMorale, float, -1);
BIND_FUNCTION01(&object(), CScriptGameObject::SetHealth, CEntityAlive, conditions().ChangeHealth, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::SetPsyHealth, CEntityAlive, conditions().ChangePsyHealth, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::SetPower, CEntityAlive, conditions().ChangePower, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::ChangeSatiety, CEntityAlive, conditions().ChangeSatiety, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::SetRadiation, CEntityAlive, conditions().ChangeRadiation, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::SetBleeding, CEntityAlive, conditions().ChangeBleeding, float, float);
BIND_FUNCTION01(
    &object(), CScriptGameObject::SetCircumspection, CEntityAlive, conditions().ChangeCircumspection, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::SetMorale, CEntityAlive, conditions().ChangeEntityMorale, float, float);
BIND_FUNCTION02(
    &object(), CScriptGameObject::SetScriptControl, CScriptEntity, SetScriptControl, bool, LPCSTR, bool, shared_str);
BIND_FUNCTION10(&object(), CScriptGameObject::GetScriptControl, CScriptEntity, GetScriptControl, bool, false);
BIND_FUNCTION10(&object(), CScriptGameObject::GetScriptControlName, CScriptEntity, GetScriptControlName, LPCSTR, "");
BIND_FUNCTION10(&object(), CScriptGameObject::GetEnemyStrength, CScriptEntity, get_enemy_strength, int, 0);
BIND_FUNCTION10(&object(), CScriptGameObject::GetActionCount, CScriptEntity, GetActionCount, u32, 0);
BIND_FUNCTION10(&object(), CScriptGameObject::can_script_capture, CScriptEntity, can_script_capture, bool, 0);

u32 CScriptGameObject::level_vertex_id() const { return (object().ai_location().level_vertex_id()); }
u32 CScriptGameObject::game_vertex_id() const { return (object().ai_location().game_vertex_id()); }
CScriptIniFile* CScriptGameObject::spawn_ini() const { return ((CScriptIniFile*)object().spawn_ini()); }
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CScriptGameObject::ResetActionQueue()
{
    CScriptEntity* l_tpScriptMonster = smart_cast<CScriptEntity*>(&object());
    if (!l_tpScriptMonster)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CSciptEntity : cannot access class member ResetActionQueue!");
    else
        l_tpScriptMonster->ClearActionQueue();
}

CScriptEntityAction* CScriptGameObject::GetCurrentAction() const
{
    CScriptEntity* l_tpScriptMonster = smart_cast<CScriptEntity*>(&object());
    if (!l_tpScriptMonster)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CSciptEntity : cannot access class member GetCurrentAction!");
    else if (l_tpScriptMonster->GetCurrentAction())
        return (new CScriptEntityAction(l_tpScriptMonster->GetCurrentAction()));
    return (0);
}

void CScriptGameObject::AddAction(const CScriptEntityAction* tpEntityAction, bool bHighPriority)
{
    CScriptEntity* l_tpScriptMonster = smart_cast<CScriptEntity*>(&object());
    if (!l_tpScriptMonster)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSciptEntity : cannot access class member AddAction!");
    else
        l_tpScriptMonster->AddAction(tpEntityAction, bHighPriority);
}

const CScriptEntityAction* CScriptGameObject::GetActionByIndex(u32 action_index)
{
    CScriptEntity* l_tpScriptMonster = smart_cast<CScriptEntity*>(&object());
    if (!l_tpScriptMonster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptEntity : cannot access class member GetActionByIndex!");
        return (0);
    }
    else
        return (l_tpScriptMonster->GetActionByIndex(action_index));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

u16 CScriptGameObject::get_bone_id(LPCSTR bone_name) const
{
    return object().Visual()->dcast_PKinematics()->LL_BoneID(bone_name);
}

cphysics_shell_scripted* CScriptGameObject::get_physics_shell() const
{
    CPhysicsShellHolder* ph_shell_holder = smart_cast<CPhysicsShellHolder*>(&object());
    if (!ph_shell_holder)
        return NULL;
    if (!ph_shell_holder->PPhysicsShell())
        return NULL;
    return get_script_wrapper<cphysics_shell_scripted>(*ph_shell_holder->PPhysicsShell());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CHelicopter* CScriptGameObject::get_helicopter()
{
    CHelicopter* helicopter = smart_cast<CHelicopter*>(&object());
    if (!helicopter)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member get_helicopter!");
        NODEFAULT;
    }
    return helicopter;
}

CHangingLamp* CScriptGameObject::get_hanging_lamp()
{
    CHangingLamp* lamp = smart_cast<CHangingLamp*>(&object());
    if (!lamp)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CGameObject : it is not a lamp!");
        NODEFAULT;
    }
    return lamp;
}

CHolderCustom* CScriptGameObject::get_custom_holder()
{
    CHolderCustom* holder = smart_cast<CHolderCustom*>(&object());
    if (!holder)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CGameObject : it is not a holder!");
    }
    return holder;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LPCSTR CScriptGameObject::WhoHitName()
{
    CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(&object());
    if (entity_alive)
        return entity_alive->conditions().GetWhoHitLastTime() ?
            (*entity_alive->conditions().GetWhoHitLastTime()->cName()) :
            NULL;
    else
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member  WhoHitName()");
        return NULL;
    }
}

LPCSTR CScriptGameObject::WhoHitSectionName()
{
    CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(&object());
    if (entity_alive)
        return entity_alive->conditions().GetWhoHitLastTime() ?
            (*entity_alive->conditions().GetWhoHitLastTime()->cNameSect()) :
            NULL;
    else
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot access class member  WhoHitName()");
        return NULL;
    }
}

bool CScriptGameObject::CheckObjectVisibility(const CScriptGameObject* tpLuaGameObject)
{
    CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(&object());
    if (entity_alive && !entity_alive->g_Alive())
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CScriptGameObject : cannot check visibility of dead object!");
        return (false);
    }

    CScriptEntity* script_entity = smart_cast<CScriptEntity*>(&object());
    if (!script_entity)
    {
        CActor* actor = smart_cast<CActor*>(&object());
        if (!actor)
        {
            GEnv.ScriptEngine->script_log(
                LuaMessageType::Error, "CScriptGameObject : cannot access class member CheckObjectVisibility!");
            return (false);
        }
        return (actor->memory().visual().visible_now(&tpLuaGameObject->object()));
    }

    return (script_entity->CheckObjectVisibility(&tpLuaGameObject->object()));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CScriptBinderObject* CScriptGameObject::binded_object() { return (object().GetScriptBinderObject()); }
void CScriptGameObject::bind_object(CScriptBinderObject* game_object) { object().SetScriptBinderObject(game_object); }
void CScriptGameObject::set_previous_point(int point_index)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member set_previous_point!");
    else
        monster->movement().patrol().set_previous_point(point_index);
}

void CScriptGameObject::set_start_point(int point_index)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member set_start_point!");
    else
        monster->movement().patrol().set_start_point(point_index);
}

u32 CScriptGameObject::get_current_patrol_point_index()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot call [get_current_patrol_point_index()]!");
        return (u32(-1));
    }
    return (monster->movement().patrol().get_current_point_index());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

Fvector CScriptGameObject::bone_position(LPCSTR bone_name) const
{
    u16 bone_id;
    if (xr_strlen(bone_name))
        bone_id = smart_cast<IKinematics*>(object().Visual())->LL_BoneID(bone_name);
    else
        bone_id = smart_cast<IKinematics*>(object().Visual())->LL_GetBoneRoot();

    Fmatrix matrix;
    matrix.mul_43(
        object().XFORM(), smart_cast<IKinematics*>(object().Visual())->LL_GetBoneInstance(bone_id).mTransform);
    return (matrix.c);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

u32 CScriptGameObject::GetAmmoElapsed()
{
    const CWeapon* weapon = smart_cast<const CWeapon*>(&object());
    if (!weapon)
        return (0);
    return (weapon->GetAmmoElapsed());
}

void CScriptGameObject::SetAmmoElapsed(int ammo_elapsed)
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon)
        return;
    weapon->SetAmmoElapsed(ammo_elapsed);
}

//Alundaio
int CScriptGameObject::GetAmmoCount(u8 type)
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon) return 0;

    if (type < weapon->m_ammoTypes.size())
        return weapon->GetAmmoCount_forType(weapon->m_ammoTypes[type]);

    return 0;
}

void CScriptGameObject::SetAmmoType(u8 type)
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon) return;

    weapon->SetAmmoType(type);
}

u8 CScriptGameObject::GetAmmoType()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon) return 255;

    return weapon->GetAmmoType();
}

void CScriptGameObject::SetMainWeaponType(u32 type)
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon) return;

    weapon->set_ef_main_weapon_type(type);
}

void CScriptGameObject::SetWeaponType(u32 type)
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon) return;

    weapon->set_ef_weapon_type(type);
}

u32 CScriptGameObject::GetMainWeaponType()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon) return 255;

    return weapon->ef_main_weapon_type();
}

u32 CScriptGameObject::GetWeaponType()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon) return 255;

    return weapon->ef_weapon_type();
}

bool CScriptGameObject::HasAmmoType(u8 type)
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon) return false;

    return type < weapon->m_ammoTypes.size();
}

u8 CScriptGameObject::GetWeaponSubstate()
{
    CWeapon* weapon = smart_cast<CWeapon*>(&object());
    if (!weapon) return 255;

    return weapon->m_sub_state;
}

//-Alundaio

u32 CScriptGameObject::GetSuitableAmmoTotal() const
{
    const CWeapon* weapon = smart_cast<const CWeapon*>(&object());
    if (!weapon)
        return 0;
    return weapon->GetSuitableAmmoTotal(true);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CScriptGameObject::SetQueueSize(u32 queue_size)
{
    CWeaponMagazined* weapon = smart_cast<CWeaponMagazined*>(&object());
    if (!weapon)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CWeaponMagazined : cannot access class member SetQueueSize!");
        return;
    }
    weapon->SetQueueSize(queue_size);
}

////////////////////////////////////////////////////////////////////////////
//// Inventory Owner
////////////////////////////////////////////////////////////////////////////

u32 CScriptGameObject::Cost() const
{
    CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&object());
    if (!inventory_item)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSciptEntity : cannot access class member Cost!");
        return (false);
    }
    return (inventory_item->Cost());
}

float CScriptGameObject::GetCondition() const
{
    CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&object());
    if (!inventory_item)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CSciptEntity : cannot access class member GetCondition!");
        return (false);
    }
    return (inventory_item->GetCondition());
}

void CScriptGameObject::SetCondition(float val)
{
    CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&object());
    if (!inventory_item)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CSciptEntity : cannot access class member SetCondition!");
        return;
    }
    val -= inventory_item->GetCondition();
    inventory_item->ChangeCondition(val);
}

void CScriptGameObject::eat(CScriptGameObject* item)
{
    if (!item)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSciptEntity : cannot access class member eat!");
        return;
    }

    CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&item->object());
    if (!inventory_item)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSciptEntity : cannot access class member eat!");
        return;
    }

    CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
    if (!inventory_owner)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSciptEntity : cannot access class member eat!");
        return;
    }

    inventory_owner->inventory().Eat(inventory_item);
}

bool CScriptGameObject::inside(const Fvector& position, float epsilon) const
{
    CSpaceRestrictor* space_restrictor = smart_cast<CSpaceRestrictor*>(&object());
    if (!space_restrictor)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSpaceRestrictor : cannot access class member inside!");
        return (false);
    }
    Fsphere sphere;
    sphere.P = position;
    sphere.R = epsilon;
    return (space_restrictor->inside(sphere));
}

bool CScriptGameObject::inside(const Fvector& position) const { return (inside(position, EPS_L)); }
void CScriptGameObject::set_patrol_extrapolate_callback(const luabind::functor<bool>& functor)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member set_patrol_extrapolate_callback!");
        return;
    }
    monster->movement().patrol().extrapolate_callback().set(functor);
}

void CScriptGameObject::set_patrol_extrapolate_callback(
    const luabind::functor<bool>& functor, const luabind::object& object)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member set_patrol_extrapolate_callback!");
        return;
    }
    monster->movement().patrol().extrapolate_callback().set(functor, object);
}

void CScriptGameObject::set_patrol_extrapolate_callback()
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member set_patrol_extrapolate_callback!");
        return;
    }
    monster->movement().patrol().extrapolate_callback().clear();
}

void CScriptGameObject::extrapolate_length(float extrapolate_length)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member extrapolate_length!");
        return;
    }
    monster->movement().detail().extrapolate_length(extrapolate_length);
}

float CScriptGameObject::extrapolate_length() const
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member extrapolate_length!");
        return (0.f);
    }
    return (monster->movement().detail().extrapolate_length());
}

void CScriptGameObject::set_fov(float new_fov)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CCustomMonster : cannot access class member set_fov!");
        return;
    }
    monster->set_fov(new_fov);
}

void CScriptGameObject::set_range(float new_range)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member set_range!");
        return;
    }
    monster->set_range(new_range);
}

u32 CScriptGameObject::vertex_in_direction(u32 level_vertex_id, Fvector direction, float max_distance) const
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member vertex_in_direction!");
        return (u32(-1));
    }

    if (!monster->movement().restrictions().accessible(level_vertex_id))
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster::vertex_in_direction - start vertex id is not accessible!");
        return (u32(-1));
    }

    direction.normalize_safe();
    direction.mul(max_distance);
    Fvector start_position = ai().level_graph().vertex_position(level_vertex_id);
    Fvector finish_position = Fvector(start_position).add(direction);
    u32 result = u32(-1);
    monster->movement().restrictions().add_border(level_vertex_id, max_distance);
    ai().level_graph().farthest_vertex_in_direction(level_vertex_id, start_position, finish_position, result, 0, true);
    monster->movement().restrictions().remove_border();
    return (ai().level_graph().valid_vertex_id(result) ? result : level_vertex_id);
}

bool CScriptGameObject::invulnerable() const
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member invulnerable!");
        return (false);
    }

    return monster->invulnerable();
}

void CScriptGameObject::invulnerable(bool invulnerable)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member invulnerable!");
        return;
    }

    monster->invulnerable(invulnerable);
}

pcstr CScriptGameObject::get_smart_cover_description() const
{
    smart_cover::object* smart_cover_object = smart_cast<smart_cover::object*>(&object());
    if (!smart_cover_object)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "smart_cover::object : cannot access class member get_smart_cover_description!");
        return nullptr;
    }
    return smart_cover_object->get_cover().get_description()->table_id().c_str();
}

void CScriptGameObject::set_visual_name(LPCSTR visual) { object().cNameVisual_set(visual); }
LPCSTR CScriptGameObject::get_visual_name() const { return object().cNameVisual().c_str(); }

void CScriptGameObject::PhantomSetEnemy(CScriptGameObject* enemy)
{
    CPhantom* phant = smart_cast<CPhantom*>(&object());
    if (!phant)
        return;

    phant->SetEnemy(&enemy->object());
}

//Allows to force use an object if passed obj is the actor
bool CScriptGameObject::Use(CScriptGameObject* obj)
{
    bool ret = object().use(&obj->object());

    CActor* actor = smart_cast<CActor*>(&obj->object());
    if (!actor)
        return ret;

    CInventoryOwner* pActorInv = smart_cast<CInventoryOwner*>(actor);
    if (!pActorInv)
        return ret;

    CUIActorMenu& ActorMenu = CurrentGameUI()->GetActorMenu();

    CInventoryBox* pBox = smart_cast<CInventoryBox*>(&object());
    if (pBox)
    {
        ActorMenu.SetActor(pActorInv);
        ActorMenu.SetInvBox(pBox);

        ActorMenu.SetMenuMode(mmDeadBodySearch);
        ActorMenu.ShowDialog(true);

        return true;
    }
    else
    {
        CInventoryOwner* pOtherOwner = smart_cast<CInventoryOwner*>(&object());
        if (!pOtherOwner)
            return ret;

        /*
        CEntityAlive* e = smart_cast<CEntityAlive*>(pOtherOwner);
        if (e && e->g_Alive())
        {
            actor->RunTalkDialog(pOtherOwner, false);
            return true;
        }
        */

        ActorMenu.SetActor(pActorInv);
        ActorMenu.SetPartner(pOtherOwner);

        ActorMenu.SetMenuMode(mmDeadBodySearch);
        ActorMenu.ShowDialog(true);

        return true;
    }

    return false;
}

void CScriptGameObject::StartTrade(CScriptGameObject* obj)
{
    CActor* actor = smart_cast<CActor*>(&obj->object());
    if (!actor)
        return;

    CInventoryOwner* pActorInv = smart_cast<CInventoryOwner*>(actor);
    if (!pActorInv)
        return;

    CInventoryOwner* pOtherOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pOtherOwner)
        return;

    CUIActorMenu& ActorMenu = CurrentGameUI()->GetActorMenu();

    ActorMenu.SetActor(pActorInv);
    ActorMenu.SetPartner(pOtherOwner);

    ActorMenu.SetMenuMode(mmTrade);
    ActorMenu.ShowDialog(true);
}

void CScriptGameObject::StartUpgrade(CScriptGameObject* obj)
{
    CActor* actor = smart_cast<CActor*>(&obj->object());
    if (!actor)
        return;

    CInventoryOwner* pActorInv = smart_cast<CInventoryOwner*>(actor);
    if (!pActorInv)
        return;

    CInventoryOwner* pOtherOwner = smart_cast<CInventoryOwner*>(&object());
    if (!pOtherOwner)
        return;

    CUIActorMenu& ActorMenu = CurrentGameUI()->GetActorMenu();

    ActorMenu.SetActor(pActorInv);
    ActorMenu.SetPartner(pOtherOwner);

    ActorMenu.SetMenuMode(mmUpgrade);
    ActorMenu.ShowDialog(true);
}
