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
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CSE_ALifeItemPDA, (CSE_ALifeItem),
{
	module(luaState)
    [
		luabind_class_item1(
			CSE_ALifeItemPDA,
			"cse_alife_item_pda",
			CSE_ALifeItem
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeItemDocument, (CSE_ALifeItem),
{
	module(luaState)
    [
		luabind_class_item1(
			CSE_ALifeItemDocument,
			"cse_alife_item_document",
			CSE_ALifeItem
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeItemGrenade, (CSE_ALifeItem),
{
	module(luaState)
    [
		luabind_class_item1(
			CSE_ALifeItemGrenade,
			"cse_alife_item_grenade",
			CSE_ALifeItem
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeItemExplosive, (CSE_ALifeItem),
{
	module(luaState)
    [
		luabind_class_item1(
			CSE_ALifeItemExplosive,
			"cse_alife_item_explosive",
			CSE_ALifeItem
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeItemBolt, (CSE_ALifeItem),
{
	module(luaState)
    [
		luabind_class_item1(
			CSE_ALifeItemBolt,
			"cse_alife_item_bolt",
			CSE_ALifeItem
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeItemCustomOutfit, (CSE_ALifeItem),
{
	module(luaState)
    [
		luabind_class_item1(
			CSE_ALifeItemCustomOutfit,
			"cse_alife_item_custom_outfit",
			CSE_ALifeItem
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeItemHelmet, (CSE_ALifeItem),
{
	module(luaState)
    [
		luabind_class_item1(
			CSE_ALifeItemHelmet,
			"cse_alife_item_helmet",
			CSE_ALifeItem
		)
	];
});

SCRIPT_EXPORT(CSE_ALifeItemWeaponMagazined, (CSE_ALifeItemWeapon),
{
	module(luaState)
    [
		luabind_class_item1(
			CSE_ALifeItemWeaponMagazined,
			"cse_alife_item_weapon_magazined",
			CSE_ALifeItemWeapon
			)
	];
});
