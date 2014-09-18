////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_script3.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server objects for ALife simulator, script export, the third part
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer_script_macroses.h"

//#ifdef XRGAME_EXPORTS
//#	include "alife_smart_terrain_task.h"
//#endif

using namespace luabind;

#pragma optimize("s",on)


void set_yaw (CSE_ALifeObjectPhysic *obj, const float yaw)
{
	obj->o_Angle.y = yaw;
}


void CSE_ALifeObjectHangingLamp::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife2(
			CSE_ALifeObjectHangingLamp,
			"cse_alife_object_hanging_lamp",
			CSE_ALifeDynamicObjectVisual,
			CSE_PHSkeleton
		)
	];
}
void CSE_ALifeObjectPhysic::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife2(
			CSE_ALifeObjectPhysic,
			"cse_alife_object_physic",
			CSE_ALifeDynamicObjectVisual,
			CSE_PHSkeleton
		)
		.def("set_yaw",				&set_yaw)
	];
}

void CSE_ALifeSmartZone::script_register(lua_State *L)
{
	module(L)[
		luabind_class_zone2(
			CSE_ALifeSmartZone,
			"cse_alife_smart_zone",
			CSE_ALifeSpaceRestrictor,
			CSE_ALifeSchedulable
		)
	];
}

