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
#include "specific_character.h"

using namespace luabind;

LPCSTR profile_name_script (CSE_ALifeTraderAbstract* ta)
{
	return *ta->character_profile();
}

void profile_name_set_script(CSE_ALifeTraderAbstract* ta, LPCSTR str)
{
	ta->set_character_profile(str);
}

void set_character_name_script(CSE_ALifeTraderAbstract* ta, LPCSTR str)
{
	ta->m_character_name = str;
}

LPCSTR character_name_script(CSE_ALifeTraderAbstract* ta)
{
	return ta->m_character_name.c_str();
}

LPCSTR icon_name_script(CSE_ALifeTraderAbstract* ta)
{
	ta->specific_character();
	if (!ta->m_icon_name.size())
	{
		CSpecificCharacter selected_char;
		selected_char.Load(ta->m_SpecificCharacter);
		ta->m_icon_name = selected_char.IconName();
	}
	return *ta->m_icon_name;
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
			.def("set_profile_name", &profile_name_set_script)
			.def("character_name", &character_name_script)
			.def("set_character_name", &set_character_name_script)
			.def("rank",			&Rank)
			.def("set_rank",		&SetRank)
			.def("reputation",		&Reputation)
			.def("character_icon", &icon_name_script)
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