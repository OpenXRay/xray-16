////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_script2.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server objects for ALife simulator, script export, the second part
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer_script_macroses.h"

using namespace luabind;

#pragma optimize("s",on)
void CSE_ALifeObjectProjector::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife1(
			CSE_ALifeObjectProjector,
			"cse_alife_object_projector",
			CSE_ALifeDynamicObjectVisual
		)
	];
}

void CSE_ALifeHelicopter::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife3(
			CSE_ALifeHelicopter,
			"cse_alife_helicopter",
			CSE_ALifeDynamicObjectVisual,
			CSE_Motion,
			CSE_PHSkeleton
		)
	];
}

void CSE_ALifeCar::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife2(
			CSE_ALifeCar,
			"cse_alife_car",
			CSE_ALifeDynamicObjectVisual,
			CSE_PHSkeleton
		)
	];
}

void CSE_ALifeObjectBreakable::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife1(
			CSE_ALifeObjectBreakable,
			"cse_alife_object_breakable",
			CSE_ALifeDynamicObjectVisual
		)
	];
}

void CSE_ALifeObjectClimable::script_register(lua_State *L)
{
	module(L)[
		luabind_class_abstract2(
			CSE_ALifeObjectClimable,
			"cse_alife_object_climable",
			CSE_Shape,
			CSE_Abstract
			)
	];
}

void CSE_ALifeMountedWeapon::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife1(
			CSE_ALifeMountedWeapon,
			"cse_alife_mounted_weapon",
			CSE_ALifeDynamicObjectVisual
		)
	];
}

void CSE_ALifeTeamBaseZone::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife1(
			CSE_ALifeTeamBaseZone,
			"cse_alife_team_base_zone",
			CSE_ALifeSpaceRestrictor
		)
	];
}

