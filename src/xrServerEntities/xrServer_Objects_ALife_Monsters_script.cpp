////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Monsters_script.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server monsters for ALife simulator, script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer_script_macroses.h"

using namespace luabind;

LPCSTR profile_name_script (CSE_ALifeTraderAbstract* ta)
{
	return *ta->character_profile();
}

#pragma optimize("s",on)
void CSE_ALifeTraderAbstract::script_register(lua_State *L)
{
	module(L)[
		class_<CSE_ALifeTraderAbstract>
			("cse_alife_trader_abstract")
//			.def(		constructor<LPCSTR>())
#ifdef XRGAME_EXPORTS
			.def("community",		&CommunityName)
			.def("profile_name",	&profile_name_script)
			.def("rank",			&Rank)
			.def("reputation",		&Reputation)
#endif // XRGAME_EXPORTS
	];
}

void CSE_ALifeTrader::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife2(
			CSE_ALifeTrader,
			"cse_alife_trader",
			CSE_ALifeDynamicObjectVisual,
			CSE_ALifeTraderAbstract
		)
	];
}

void CSE_ALifeCustomZone::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife2(
			CSE_ALifeCustomZone,
			"cse_custom_zone",
			CSE_ALifeDynamicObject,
			CSE_Shape
		)
	];
}

void CSE_ALifeAnomalousZone::script_register(lua_State *L)
{
	module(L)[
		luabind_class_dynamic_alife1(
			CSE_ALifeAnomalousZone,
			"cse_anomalous_zone",
			CSE_ALifeCustomZone
		)
#ifdef XRGAME_EXPORTS
//.		.def("spawn_artefacts",	&CSE_ALifeAnomalousZone::spawn_artefacts)
#endif
	];
}

void CSE_ALifeMonsterRat::script_register(lua_State *L)
{
	module(L)[
		luabind_class_monster2(
			CSE_ALifeMonsterRat,
			"cse_alife_monster_rat",
			CSE_ALifeMonsterAbstract,
			CSE_ALifeInventoryItem
		)
	];
}