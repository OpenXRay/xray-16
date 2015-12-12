////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Monsters_script3.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server monsters for ALife simulator, script export, the second part
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer_script_macroses.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;

SCRIPT_EXPORT(CSE_ALifeCreatureActor, (CSE_ALifeCreatureAbstract, CSE_ALifeTraderAbstract, CSE_PHSkeleton),
{
	module(luaState)
    [
		luabind_class_creature3(
			CSE_ALifeCreatureActor,
			"cse_alife_creature_actor",
			CSE_ALifeCreatureAbstract,
			CSE_ALifeTraderAbstract,
			CSE_PHSkeleton
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeTorridZone, (CSE_ALifeCustomZone, CSE_Motion),
{
	module(luaState)
    [
		luabind_class_dynamic_alife2(
			CSE_ALifeTorridZone,
			"cse_torrid_zone",
			CSE_ALifeCustomZone,
			CSE_Motion
			)
	];
});

SCRIPT_EXPORT(CSE_ALifeZoneVisual, (CSE_ALifeAnomalousZone, CSE_Visual),
{
	module(luaState)
    [
		luabind_class_dynamic_alife2(
			CSE_ALifeZoneVisual,
			"cse_zone_visual",
			CSE_ALifeAnomalousZone,
			CSE_Visual
			)
	];
});

SCRIPT_EXPORT(CSE_ALifeCreaturePhantom, (CSE_ALifeCreatureAbstract),
{
	module(luaState)
    [
		luabind_class_creature1(
			CSE_ALifeCreaturePhantom,
			"cse_alife_creature_phantom",
			CSE_ALifeCreatureAbstract
			)
	];
});

static SRotation* CSE_ALifeCreatureAbstract__o_torso	(CSE_ALifeCreatureAbstract* self)
{
	return	(&self->o_torso);
}

SCRIPT_EXPORT(CSE_ALifeCreatureAbstract, (CSE_ALifeDynamicObjectVisual),
{
	module(luaState)
    [
		luabind_class_creature1(
			CSE_ALifeCreatureAbstract,
			"cse_alife_creature_abstract",
			CSE_ALifeDynamicObjectVisual
		)
		.def("health",&CSE_ALifeCreatureAbstract::get_health)
		.def("alive",&CSE_ALifeCreatureAbstract::g_Alive)
		.def_readwrite("team",&CSE_ALifeCreatureAbstract::s_team)
		.def_readwrite("squad",&CSE_ALifeCreatureAbstract::s_squad)
		.def_readwrite("group",&CSE_ALifeCreatureAbstract::s_group)
		.def("o_torso",&CSE_ALifeCreatureAbstract__o_torso)
	];
});

static void CSE_ALifeOnlineOfflineGroup_Export(lua_State *luaState)
{
	module(luaState)
    [
        class_< CSE_ALifeOnlineOfflineGroup::MEMBERS::value_type >("MEMBERS__value_type")
			.def_readonly("id", 	&CSE_ALifeOnlineOfflineGroup::MEMBERS::value_type::first )
			.def_readonly("object",	&CSE_ALifeOnlineOfflineGroup::MEMBERS::value_type::second ),

		luabind_class_online_offline_group2(
			CSE_ALifeOnlineOfflineGroup,
			"cse_alife_online_offline_group",
			CSE_ALifeDynamicObject,
			CSE_ALifeSchedulable
		)
#ifdef XRGAME_EXPORTS
		.def("register_member",		&CSE_ALifeOnlineOfflineGroup::register_member)
		.def("unregister_member",	&CSE_ALifeOnlineOfflineGroup::unregister_member)
		.def("commander_id",		&CSE_ALifeOnlineOfflineGroup::commander_id)
		.def("squad_members",		&CSE_ALifeOnlineOfflineGroup::squad_members, return_stl_iterator())
		.def("npc_count",			&CSE_ALifeOnlineOfflineGroup::npc_count)
		.def("add_location_type",	&CSE_ALifeOnlineOfflineGroup::add_location_type)
		.def("clear_location_types",&CSE_ALifeOnlineOfflineGroup::clear_location_types)
		.def("force_change_position",&CSE_ALifeOnlineOfflineGroup::force_change_position)
#endif
	];
}

SCRIPT_EXPORT_FUNC(CSE_ALifeOnlineOfflineGroup, (CSE_ALifeDynamicObject, CSE_ALifeSchedulable),
    CSE_ALifeOnlineOfflineGroup_Export);
