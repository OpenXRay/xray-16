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
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

void set_yaw (CSE_ALifeObjectPhysic *obj, const float yaw)
{ obj->o_Angle.y = yaw; }


SCRIPT_EXPORT(CSE_ALifeObjectHangingLamp, (CSE_ALifeDynamicObjectVisual, CSE_PHSkeleton),
{
	module(luaState)
    [
		luabind_class_dynamic_alife2(
			CSE_ALifeObjectHangingLamp,
			"cse_alife_object_hanging_lamp",
			CSE_ALifeDynamicObjectVisual,
			CSE_PHSkeleton
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeObjectPhysic, (CSE_ALifeDynamicObjectVisual, CSE_PHSkeleton),
{
	module(luaState)
    [
		luabind_class_dynamic_alife2(
			CSE_ALifeObjectPhysic,
			"cse_alife_object_physic",
			CSE_ALifeDynamicObjectVisual,
			CSE_PHSkeleton
		)
		.def("set_yaw",				&set_yaw)
	];
});

SCRIPT_EXPORT(CSE_ALifeSmartZone, (CSE_ALifeSpaceRestrictor, CSE_ALifeSchedulable),
{
	module(luaState)
    [
		luabind_class_zone2(
			CSE_ALifeSmartZone,
			"cse_alife_smart_zone",
			CSE_ALifeSpaceRestrictor,
			CSE_ALifeSchedulable
		)
	];
});
