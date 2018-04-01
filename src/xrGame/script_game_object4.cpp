////////////////////////////////////////////////////////////////////////////
// script_game_object_trader.сpp :	функции для торговли и торговцев
//////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"

#include "script_zone.h"
#include "ai/trader/ai_trader.h"

#include "ai_space.h"
#include "alife_simulator.h"

#include "ai/stalker/ai_stalker.h"
#include "stalker_movement_manager_smart_cover.h"

#include "sight_manager_space.h"
#include "sight_control_action.h"
#include "sight_manager.h"
#include "inventoryBox.h"
#include "ZoneCampfire.h"
#include "physicobject.h"
#include "artefact.h"
#include "stalker_sound_data.h"
#include "level_changer.h"

#include "eatable_item.h"
#include "car.h"
#include "helicopter.h"
#include "actor.h"
#include "customoutfit.h"
#include "customzone.h"
#include "ai\monsters\basemonster\base_monster.h"
#include "medkit.h"
#include "antirad.h"
#include "scope.h"
#include "silencer.h"
#include "torch.h"
#include "GrenadeLauncher.h"
#include "searchlight.h"
#include "WeaponAmmo.h"
#include "grenade.h"
#include "BottleItem.h"
#include "WeaponMagazinedWGrenade.h"

class CWeapon;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

bool CScriptGameObject::is_body_turning() const
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CGameObject : cannot access class member is_turning!");
        return (false);
    }

    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(monster);
    if (!stalker)
        return (!fis_zero(angle_difference(
            monster->movement().body_orientation().target.yaw, monster->movement().body_orientation().current.yaw)));
    else
        return (!fis_zero(angle_difference(stalker->movement().head_orientation().target.yaw,
                    stalker->movement().head_orientation().current.yaw)) ||
            !fis_zero(angle_difference(monster->movement().body_orientation().target.yaw,
                monster->movement().body_orientation().current.yaw)));
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

u32 CScriptGameObject::add_sound(
    LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type, LPCSTR bone_name)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSoundPlayer : cannot access class member add!");
        return (0);
    }
    else
        return (monster->sound().add(prefix, max_count, type, priority, mask, internal_type, bone_name));
}

u32 CScriptGameObject::add_combat_sound(
    LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type, LPCSTR bone_name)
{
    CAI_Stalker* const stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSoundPlayer : cannot access class member add!");
        return (0);
    }
    else
        return (stalker->sound().add(
            prefix, max_count, type, priority, mask, internal_type, bone_name, new CStalkerSoundData(stalker)));
}

u32 CScriptGameObject::add_sound(
    LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type)
{
    return (add_sound(prefix, max_count, type, priority, mask, internal_type, "bip01_head"));
}

void CScriptGameObject::remove_sound(u32 internal_type)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSoundPlayer : cannot access class member add!");
    else
        monster->sound().remove(internal_type);
}

void CScriptGameObject::set_sound_mask(u32 sound_mask)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CSoundPlayer : cannot access class member set_sound_mask!");
    else
    {
        CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(monster);
        if (entity_alive)
        {
            VERIFY2(entity_alive->g_Alive(), "Stalkers talk after death??? Say why??");
        }
        monster->sound().set_sound_mask(sound_mask);
    }
}

void CScriptGameObject::play_sound(u32 internal_type)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSoundPlayer : cannot access class member play!");
    else
        monster->sound().play(internal_type);
}

void CScriptGameObject::play_sound(u32 internal_type, u32 max_start_time)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSoundPlayer : cannot access class member play!");
    else
        monster->sound().play(internal_type, max_start_time);
}

void CScriptGameObject::play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSoundPlayer : cannot access class member play!");
    else
        monster->sound().play(internal_type, max_start_time, min_start_time);
}

void CScriptGameObject::play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSoundPlayer : cannot access class member play!");
    else
        monster->sound().play(internal_type, max_start_time, min_start_time, max_stop_time);
}

void CScriptGameObject::play_sound(
    u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time, u32 min_stop_time)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSoundPlayer : cannot access class member play!");
    else
        monster->sound().play(internal_type, max_start_time, min_start_time, max_stop_time, min_stop_time);
}

void CScriptGameObject::play_sound(
    u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time, u32 min_stop_time, u32 id)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CSoundPlayer : cannot access class member play!");
    else
        monster->sound().play(internal_type, max_start_time, min_start_time, max_stop_time, min_stop_time, id);
}

int CScriptGameObject::active_sound_count(bool only_playing)
{
    CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
    if (!monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CGameObject : cannot access class member active_sound_count!");
        return (-1);
    }
    else
        return (monster->sound().active_sound_count(only_playing));
}

int CScriptGameObject::active_sound_count() { return (active_sound_count(false)); }
bool CScriptGameObject::wounded() const
{
    const CAI_Stalker* stalker = smart_cast<const CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member wounded!");
        return (false);
    }

    return (stalker->wounded());
}

void CScriptGameObject::wounded(bool value)
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(LuaMessageType::Error, "CAI_Stalker : cannot access class member wounded!");
        return;
    }

    stalker->wounded(value);
}

CSightParams CScriptGameObject::sight_params()
{
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(&object());
    if (!stalker)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CAI_Stalker : cannot access class member sight_params!");

        CSightParams result;
        result.m_object = 0;
        result.m_vector = Fvector().set(flt_max, flt_max, flt_max);
        result.m_sight_type = SightManager::eSightTypeDummy;
        return (result);
    }

    const CSightControlAction& action = stalker->sight().current_action();
    CSightParams result;
    result.m_sight_type = action.sight_type();
    result.m_object = action.object_to_look() ? action.object_to_look()->lua_game_object() : 0;
    result.m_vector = action.vector3d();
    return (result);
}

bool CScriptGameObject::critically_wounded()
{
    CCustomMonster* custom_monster = smart_cast<CCustomMonster*>(&object());
    if (!custom_monster)
    {
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "CCustomMonster : cannot access class member critically_wounded!");
        return (false);
    }

    return (custom_monster->critically_wounded());
}

bool CScriptGameObject::IsInvBoxEmpty()
{
    CInventoryBox* ib = smart_cast<CInventoryBox*>(&object());
    if (!ib)
        return (false);
    else
        return ib->IsEmpty();
}

bool CScriptGameObject::inv_box_closed(bool status, LPCSTR reason)
{
    CInventoryBox* ib = smart_cast<CInventoryBox*>(&object());
    if (!ib)
    {
        return false;
    }
    else
    {
        ib->set_closed(status, reason);
        return true;
    }
}

bool CScriptGameObject::inv_box_closed_status()
{
    CInventoryBox* ib = smart_cast<CInventoryBox*>(&object());
    if (!ib)
    {
        return false;
    }
    else
    {
        return ib->closed();
    }
}

bool CScriptGameObject::inv_box_can_take(bool status)
{
    CInventoryBox* ib = smart_cast<CInventoryBox*>(&object());
    if (!ib)
    {
        return false;
    }
    else
    {
        ib->set_can_take(status);
        return true;
    }
}

bool CScriptGameObject::inv_box_can_take_status()
{
    CInventoryBox* ib = smart_cast<CInventoryBox*>(&object());
    if (!ib)
    {
        return false;
    }
    else
    {
        return ib->can_take();
    }
}

CZoneCampfire* CScriptGameObject::get_campfire() { return smart_cast<CZoneCampfire*>(&object()); }
CArtefact* CScriptGameObject::get_artefact() { return smart_cast<CArtefact*>(&object()); }
CPhysicObject* CScriptGameObject::get_physics_object() { return smart_cast<CPhysicObject*>(&object()); }

void CScriptGameObject::enable_level_changer(bool b)
{
    CLevelChanger* lch = smart_cast<CLevelChanger*>(&object());
    if (lch)
        lch->EnableLevelChanger(b);
}
bool CScriptGameObject::is_level_changer_enabled()
{
    CLevelChanger* lch = smart_cast<CLevelChanger*>(&object());
    if (lch)
        return lch->IsLevelChangerEnabled();
    return false;
}

void CScriptGameObject::set_level_changer_invitation(LPCSTR str)
{
    CLevelChanger* lch = smart_cast<CLevelChanger*>(&object());
    if (lch)
        lch->SetLEvelChangerInvitationStr(str);
}

void CScriptGameObject::start_particles(LPCSTR pname, LPCSTR bone)
{
    CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(&object());
    if (!PP)
        return;

    IKinematics* K = smart_cast<IKinematics*>(object().Visual());
    if (!K)
        return;
    //R_ASSERT(K);

    u16 play_bone = K->LL_BoneID(bone);
    R_ASSERT(play_bone != BI_NONE);
    if (K->LL_GetBoneVisible(play_bone))
        PP->StartParticles(pname, play_bone, Fvector().set(0, 1, 0), 9999);
    else
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "Cant start particles, bone [%s] is not visible now", bone);
}

void CScriptGameObject::stop_particles(LPCSTR pname, LPCSTR bone)
{
    CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(&object());
    if (!PP)
        return;

    IKinematics* K = smart_cast<IKinematics*>(object().Visual());
    if (!K)
        return;
    //R_ASSERT(K);

    u16 play_bone = K->LL_BoneID(bone);
    R_ASSERT(play_bone != BI_NONE);

    if (K->LL_GetBoneVisible(play_bone))
        PP->StopParticles(9999, play_bone, true);
    else
        GEnv.ScriptEngine->script_log(
            LuaMessageType::Error, "Cant stop particles, bone [%s] is not visible now", bone);
}

//AVO: directly set entity health instead of going through normal health property which operates on delta
void CScriptGameObject::SetHealthEx(float hp)
{
    CEntity* obj = smart_cast<CEntity*>(&object());
    if (!obj) return;
    clamp(hp, -0.01f, 1.0f);
    obj->SetfHealth(hp);
}
//-AVO

#define SPECIFIC_CAST(A,B)\
B* A ()\
{\
    B				*l_tpEntity = smart_cast<B*>(&object());\
    if (!l_tpEntity)\
        return (0);\
                else\
        return l_tpEntity;\
};\

SPECIFIC_CAST(CScriptGameObject::cast_GameObject, CScriptGameObject);
SPECIFIC_CAST(CScriptGameObject::cast_Car, CCar);
SPECIFIC_CAST(CScriptGameObject::cast_Heli, CHelicopter);
SPECIFIC_CAST(CScriptGameObject::cast_HolderCustom, CHolderCustom);
SPECIFIC_CAST(CScriptGameObject::cast_EntityAlive, CEntityAlive);
SPECIFIC_CAST(CScriptGameObject::cast_InventoryItem, CInventoryItem);
SPECIFIC_CAST(CScriptGameObject::cast_InventoryOwner, CInventoryOwner);
SPECIFIC_CAST(CScriptGameObject::cast_Actor, CActor);
SPECIFIC_CAST(CScriptGameObject::cast_Weapon, CWeapon);
CMedkit* CScriptGameObject::cast_Medkit()
{
    CInventoryItem* ii = object().cast_inventory_item();
    return ii ? smart_cast<CMedkit*>(ii) : (0);
}
CEatableItem* CScriptGameObject::cast_EatableItem()
{
    CInventoryItem* ii = object().cast_inventory_item();
    return ii ? ii->cast_eatable_item() : (0);
}
CAntirad* CScriptGameObject::cast_Antirad()
{
    CInventoryItem* ii = object().cast_inventory_item();
    return ii ? smart_cast<CAntirad*>(ii) : (0);
}
SPECIFIC_CAST(CScriptGameObject::cast_CustomOutfit, CCustomOutfit);
SPECIFIC_CAST(CScriptGameObject::cast_Scope, CScope);
SPECIFIC_CAST(CScriptGameObject::cast_Silencer, CSilencer);
SPECIFIC_CAST(CScriptGameObject::cast_GrenadeLauncher, CGrenadeLauncher);
SPECIFIC_CAST(CScriptGameObject::cast_WeaponMagazined, CWeaponMagazined);
SPECIFIC_CAST(CScriptGameObject::cast_SpaceRestrictor, CSpaceRestrictor);
SPECIFIC_CAST(CScriptGameObject::cast_Stalker, CAI_Stalker);
SPECIFIC_CAST(CScriptGameObject::cast_CustomZone, CCustomZone);
SPECIFIC_CAST(CScriptGameObject::cast_Monster, CCustomMonster);
SPECIFIC_CAST(CScriptGameObject::cast_Explosive, CExplosive);
SPECIFIC_CAST(CScriptGameObject::cast_ScriptZone, CScriptZone);
//SPECIFIC_CAST(CScriptGameObject::cast_Projector, CProjector);
SPECIFIC_CAST(CScriptGameObject::cast_Trader, CAI_Trader);
CHudItem* CScriptGameObject::cast_HudItem()
{
    CInventoryItem* ii = object().cast_inventory_item();
    return ii ? smart_cast<CHudItem*>(ii) : (0);
}
CFoodItem* CScriptGameObject::cast_FoodItem()
{
    CInventoryItem* ii = object().cast_inventory_item();
    return ii ? ii->cast_food_item() : (0);
}
SPECIFIC_CAST(CScriptGameObject::cast_Artefact, CArtefact);
SPECIFIC_CAST(CScriptGameObject::cast_Ammo, CWeaponAmmo);
//SPECIFIC_CAST(CScriptGameObject::cast_Missile, CMissile);
SPECIFIC_CAST(CScriptGameObject::cast_PhysicsShellHolder, CPhysicsShellHolder);
//SPECIFIC_CAST(CScriptGameObject::cast_Grenade, CGrenade);
CBottleItem* CScriptGameObject::cast_BottleItem()
{
    CInventoryItem* ii = object().cast_inventory_item();
    return ii ? smart_cast<CBottleItem*>(ii) : (0);
}
CTorch* CScriptGameObject::cast_Torch()
{

    CInventoryItem* ii = object().cast_inventory_item();
    return ii ? smart_cast<CTorch*>(ii) : (0);
}
SPECIFIC_CAST(CScriptGameObject::cast_WeaponMagazinedWGrenade, CWeaponMagazinedWGrenade);
SPECIFIC_CAST(CScriptGameObject::cast_InventoryBox, CInventoryBox);
