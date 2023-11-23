////////////////////////////////////////////////////////////////////////////
//  Module      : alife_space.h
//  Created     : 08.01.2002
//  Modified    : 08.01.2003
//  Author      : Dmitriy Iassenev
//  Description : ALife space
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_map.h"

// ALife objects, events and tasks
#define ALIFE_VERSION 0x0007
#define ALIFE_CHUNK_DATA 0x0000
#define SPAWN_CHUNK_DATA 0x0001
#define OBJECT_CHUNK_DATA 0x0002
#define GAME_TIME_CHUNK_DATA 0x0005
#define REGISTRY_CHUNK_DATA 0x0009
#define SECTION_HEADER "location_"
#define SAVE_EXTENSION ".scop"
#define SAVE_EXTENSION_LEGACY ".sav"
#define SPAWN_NAME "game.spawn"
// inventory rukzak size
#define MAX_ITEM_VOLUME 100
#define INVALID_STORY_ID ALife::_STORY_ID(-1)
#define INVALID_SPAWN_STORY_ID ALife::_SPAWN_STORY_ID(-1)

class CSE_ALifeDynamicObject;
class CSE_ALifeMonsterAbstract;
class CSE_ALifeTrader;
class CSE_ALifeInventoryItem;
class CSE_ALifeItemWeapon;
class CSE_ALifeSchedulable;
class CGameGraph;

namespace ALife
{
typedef u64 _CLASS_ID; // Class ID
typedef u16 _OBJECT_ID; // Object ID
typedef u64 _TIME_ID; // Time  ID
typedef u32 _EVENT_ID; // Event ID
typedef u32 _TASK_ID; // Event ID
typedef u16 _SPAWN_ID; // Spawn ID
typedef u16 _TERRAIN_ID; // Terrain ID
typedef u32 _STORY_ID; // Story ID
typedef u32 _SPAWN_STORY_ID; // Spawn Story ID

struct SSumStackCell
{
    int i1;
    int i2;
    int iCurrentSum;
};

enum ECombatResult : u32
{
    eCombatResultRetreat1,
    eCombatResultRetreat2,
    eCombatResultRetreat12,
    eCombatResult1Kill2,
    eCombatResult2Kill1,
    eCombatResultBothKilled,
};

enum ECombatAction : u32
{
    eCombatActionAttack,
    eCombatActionRetreat,
};

enum EMeetActionType : u32
{
    eMeetActionTypeAttack,
    eMeetActionTypeInteract,
    eMeetActionTypeIgnore,
    eMeetActionSmartTerrain,
};

enum ERelationType : u32
{
    eRelationTypeFriend = u32(0),
    eRelationTypeNeutral,
    eRelationTypeEnemy,
    eRelationTypeWorstEnemy,
    eRelationTypeLast,
    eRelationTypeDummy = u32(-1),
};

enum EHitType : u32
{
    eHitTypeBurn = u32(0),
    eHitTypeShock,
    eHitTypeChemicalBurn,
    eHitTypeRadiation,
    eHitTypeTelepatic,
    eHitTypeWound,
    eHitTypeFireWound,
    eHitTypeStrike,
    eHitTypeExplosion,
    eHitTypeWound_2, // knife's alternative fire
    eHitTypeLightBurn,
    eHitTypePhysicStrike,
    eHitTypeMax,
};

enum EInfluenceType
{
    infl_rad = u32(0),
    infl_fire,
    infl_acid,
    infl_psi,
    infl_electra,
    infl_max_count
};

enum EConditionRestoreType
{
    eHealthRestoreSpeed = u32(0),
    eSatietyRestoreSpeed,
    ePowerRestoreSpeed,
    eBleedingRestoreSpeed,
    eRadiationRestoreSpeed,
    eRestoreTypeMax,
};

enum ETakeType
{
    eTakeTypeAll,
    eTakeTypeMin,
    eTakeTypeRest,
};

enum EWeaponPriorityType : u32
{
    eWeaponPriorityTypeKnife,
    eWeaponPriorityTypeSecondary,
    eWeaponPriorityTypePrimary,
    eWeaponPriorityTypeGrenade,
};

enum ECombatType : u32
{
    eCombatTypeMonsterMonster,
    eCombatTypeMonsterAnomaly,
    eCombatTypeAnomalyMonster,
    eCombatTypeSmartTerrain,
};

// ability to attach addons
enum EWeaponAddonStatus
{
    eAddonDisabled = 0,   // can't attach
    eAddonPermanent = 1,  // always attached, already
    eAddonAttachable = 2, // can attach/detach
};

EHitType g_tfString2HitType(LPCSTR caHitType);
pcstr g_cafHitType2String(EHitType tHitType);

using INT_VECTOR = xr_vector<int>;
using OBJECT_VECTOR = xr_vector<_OBJECT_ID>;
using OBJECT_IT = OBJECT_VECTOR::iterator;
using ITEM_P_VECTOR = xr_vector<CSE_ALifeInventoryItem*>;
using WEAPON_P_VECTOR = xr_vector<CSE_ALifeItemWeapon*>;
using SCHEDULE_P_VECTOR = xr_vector<CSE_ALifeSchedulable*>;

using D_OBJECT_P_MAP = xr_map<_OBJECT_ID, CSE_ALifeDynamicObject*>;
using STORY_P_MAP = xr_map<_STORY_ID, CSE_ALifeDynamicObject*>;
}
