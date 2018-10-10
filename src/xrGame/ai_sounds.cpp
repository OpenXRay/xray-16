#include "StdAfx.h"
#pragma hdrstop

#include "ai_sounds.h"
#include "xrCore/xr_token.h"

const xr_token anomaly_type_token[] = {
    {"undefined", int(sg_Undefined)}, {"Item picking up", int(SOUND_TYPE_ITEM_PICKING_UP)},
    {"Item dropping", int(SOUND_TYPE_ITEM_DROPPING)}, {"Item taking", int(SOUND_TYPE_ITEM_TAKING)},
    {"Item hiding", int(SOUND_TYPE_ITEM_HIDING)}, {"Item using", int(SOUND_TYPE_ITEM_USING)},
    {"Weapon shooting", int(SOUND_TYPE_WEAPON_SHOOTING)}, {"Weapon empty clicking", int(SOUND_TYPE_WEAPON_EMPTY_CLICKING)},
    {"Weapon bullet hit", int(SOUND_TYPE_WEAPON_BULLET_HIT)}, {"Weapon recharging", int(SOUND_TYPE_WEAPON_RECHARGING)},
    {"NPC dying", int(SOUND_TYPE_MONSTER_DYING)}, {"NPC injuring", int(SOUND_TYPE_MONSTER_INJURING)},
    {"NPC step", int(SOUND_TYPE_MONSTER_STEP)}, {"NPC talking", int(SOUND_TYPE_MONSTER_TALKING)},
    {"NPC attacking", int(SOUND_TYPE_MONSTER_ATTACKING)}, {"NPC eating", int(SOUND_TYPE_MONSTER_EATING)},
    {"Anomaly idle", int(SOUND_TYPE_ANOMALY_IDLE)}, {"Object breaking", int(SOUND_TYPE_WORLD_OBJECT_BREAKING)},
    {"Object colliding", int(SOUND_TYPE_WORLD_OBJECT_COLLIDING)}, {"Object exploding", int(SOUND_TYPE_WORLD_OBJECT_EXPLODING)},
    {"World ambient", int(SOUND_TYPE_WORLD_AMBIENT)}, {0, 0}};
