////////////////////////////////////////////////////////////////////////////
//	Module 		: script_sound_type_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script sound type script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_sound_type.h"
#include "ai_sounds.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptSoundType::script_register(lua_State *L)
{
	module(L)
	[
		class_<enum_exporter<ESoundTypes> >("snd_type")
			.enum_("sound_types")
			[
				value("no_sound",				int(SOUND_TYPE_NO_SOUND					)),
				value("weapon",					int(SOUND_TYPE_WEAPON					)),
				value("item",					int(SOUND_TYPE_ITEM						)),
				value("monster",				int(SOUND_TYPE_MONSTER					)),
				value("anomaly",				int(SOUND_TYPE_ANOMALY					)),
				value("world",					int(SOUND_TYPE_WORLD					)),
				value("pick_up",				int(SOUND_TYPE_PICKING_UP				)),
				value("drop",					int(SOUND_TYPE_DROPPING					)),
				value("hide",					int(SOUND_TYPE_HIDING					)),
				value("take",					int(SOUND_TYPE_TAKING					)),
				value("use",					int(SOUND_TYPE_USING					)),
				value("shoot",					int(SOUND_TYPE_SHOOTING					)),
				value("empty",					int(SOUND_TYPE_EMPTY_CLICKING			)),
				value("bullet_hit",				int(SOUND_TYPE_BULLET_HIT				)),
				value("reload",					int(SOUND_TYPE_RECHARGING				)),
				value("die",					int(SOUND_TYPE_DYING					)),
				value("injure",					int(SOUND_TYPE_INJURING					)),
				value("step",					int(SOUND_TYPE_STEP						)),
				value("talk",					int(SOUND_TYPE_TALKING					)),
				value("attack",					int(SOUND_TYPE_ATTACKING				)),
				value("eat",					int(SOUND_TYPE_EATING					)),
				value("idle",					int(SOUND_TYPE_IDLE						)),
				value("object_break",			int(SOUND_TYPE_OBJECT_BREAKING			)),
				value("object_collide",			int(SOUND_TYPE_OBJECT_COLLIDING			)),
				value("object_explode",			int(SOUND_TYPE_OBJECT_EXPLODING			)),
				value("ambient",				int(SOUND_TYPE_AMBIENT					)),
				value("item_pick_up",			int(SOUND_TYPE_ITEM_PICKING_UP			)),
				value("item_drop",				int(SOUND_TYPE_ITEM_DROPPING			)),
				value("item_hide",				int(SOUND_TYPE_ITEM_HIDING				)),
				value("item_take",				int(SOUND_TYPE_ITEM_TAKING				)),
				value("item_use",				int(SOUND_TYPE_ITEM_USING				)),
				value("weapon_shoot",			int(SOUND_TYPE_WEAPON_SHOOTING			)),
				value("weapon_empty",			int(SOUND_TYPE_WEAPON_EMPTY_CLICKING	)),
				value("weapon_bullet_hit",		int(SOUND_TYPE_WEAPON_BULLET_HIT		)),
				value("weapon_reload",			int(SOUND_TYPE_WEAPON_RECHARGING		)),
				value("monster_die",			int(SOUND_TYPE_MONSTER_DYING			)),
				value("monster_injure",			int(SOUND_TYPE_MONSTER_INJURING			)),
				value("monster_step",			int(SOUND_TYPE_MONSTER_STEP				)),
				value("monster_talk",			int(SOUND_TYPE_MONSTER_TALKING			)),
				value("monster_attack",			int(SOUND_TYPE_MONSTER_ATTACKING		)),
				value("monster_eat",			int(SOUND_TYPE_MONSTER_EATING			)),
				value("anomaly_idle",			int(SOUND_TYPE_ANOMALY_IDLE				)),
				value("world_object_break",		int(SOUND_TYPE_WORLD_OBJECT_BREAKING	)),
				value("world_object_collide",	int(SOUND_TYPE_WORLD_OBJECT_COLLIDING	)),
				value("world_object_explode",	int(SOUND_TYPE_WORLD_OBJECT_EXPLODING	)),
				value("world_ambient",			int(SOUND_TYPE_WORLD_AMBIENT			))
			]
	];
}