////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Items_script2.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server items for ALife simulator, script export, the second part
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_script_macroses.h"

using namespace luabind;

#pragma optimize("s",on)
void CSE_ALifeItemPDA::script_register(lua_State *L)
{
	module(L)[
		luabind_class_item1(
			CSE_ALifeItemPDA,
			"cse_alife_item_pda",
			CSE_ALifeItem
		)
	];
}

void CSE_ALifeItemDocument::script_register(lua_State *L)
{
	module(L)[
		luabind_class_item1(
			CSE_ALifeItemDocument,
			"cse_alife_item_document",
			CSE_ALifeItem
		)
	];
}

void CSE_ALifeItemGrenade::script_register(lua_State *L)
{
	module(L)[
		luabind_class_item1(
			CSE_ALifeItemGrenade,
			"cse_alife_item_grenade",
			CSE_ALifeItem
		)
	];
}

void CSE_ALifeItemExplosive::script_register(lua_State *L)
{
	module(L)[
		luabind_class_item1(
			CSE_ALifeItemExplosive,
			"cse_alife_item_explosive",
			CSE_ALifeItem
		)
	];
}

void CSE_ALifeItemBolt::script_register(lua_State *L)
{
	module(L)[
		luabind_class_item1(
			CSE_ALifeItemBolt,
			"cse_alife_item_bolt",
			CSE_ALifeItem
		)
	];
}

void CSE_ALifeItemCustomOutfit::script_register(lua_State *L)
{
	module(L)[
		luabind_class_item1(
			CSE_ALifeItemCustomOutfit,
			"cse_alife_item_custom_outfit",
			CSE_ALifeItem
		)
	];
}

void CSE_ALifeItemHelmet::script_register(lua_State *L)
{
	module(L)[
		luabind_class_item1(
			CSE_ALifeItemHelmet,
			"cse_alife_item_helmet",
			CSE_ALifeItem
		)
	];
}

void CSE_ALifeItemWeaponMagazined::script_register(lua_State *L)
{
	module(L)[
		luabind_class_item1(
			CSE_ALifeItemWeaponMagazined,
			"cse_alife_item_weapon_magazined",
			CSE_ALifeItemWeapon
			)
	];
}
