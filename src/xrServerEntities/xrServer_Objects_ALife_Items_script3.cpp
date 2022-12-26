////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Items_script3.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Dmitriy Iassenev
//	Description : Server items for ALife simulator, script export, the second part
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_script_macroses.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CSE_ALifeItemWeaponMagazinedWGL, (CSE_ALifeItemWeaponMagazined),
{
    using namespace luabind;

    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemWeaponMagazinedWGL, "cse_alife_item_weapon_magazined_w_gl", CSE_ALifeItemWeaponMagazined)
    ];
});
