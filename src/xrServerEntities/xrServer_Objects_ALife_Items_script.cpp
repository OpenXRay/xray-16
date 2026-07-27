////////////////////////////////////////////////////////////////////////////
//  Module      : xrServer_Objects_ALife_Items_script.cpp
//  Created     : 19.09.2002
//  Modified    : 04.06.2003
//  Author      : Dmitriy Iassenev
//  Description : Server items for ALife simulator, script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_script_macroses.h"

void CSE_ALifeInventoryItem::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CSE_ALifeInventoryItem>("cse_alife_inventory_item")
            //.def(constructor<pcstr>())
            .def("has_upgrade", +[](CSE_ALifeInventoryItem* ta, pcstr str)
            {
                return ta->has_upgrade(str);
            })
            .def("add_upgrade", +[](CSE_ALifeInventoryItem* ta, pcstr str)
            {
                ta->add_upgrade(str);
            })
    ];
}

void CSE_ALifeItem::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        luabind_class_item2(
        //      luabind_class_abstract2(
        CSE_ALifeItem, "cse_alife_item", CSE_ALifeDynamicObjectVisual, CSE_ALifeInventoryItem)];
}

void CSE_ALifeItemTorch::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemTorch, "cse_alife_item_torch", CSE_ALifeItem)
    ];
}

void CSE_ALifeItemAmmo::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemAmmo, "cse_alife_item_ammo", CSE_ALifeItem)
    ];
}

void CSE_ALifeItemWeapon::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemWeapon, "cse_alife_item_weapon", CSE_ALifeItem)
            .enum_("addon_flag")
            [
                value("eWeaponAddonGrenadeLauncher", int(CSE_ALifeItemWeapon::EWeaponAddonState::eWeaponAddonGrenadeLauncher)),
                value("eWeaponAddonScope", int(CSE_ALifeItemWeapon::EWeaponAddonState::eWeaponAddonScope)),
                value("eWeaponAddonSilencer", int(CSE_ALifeItemWeapon::EWeaponAddonState::eWeaponAddonSilencer)),
                value("eAddonAttachable", int(CSE_ALifeItemWeapon::EWeaponAddonStatus::eAddonAttachable)),
                value("eAddonDisabled", int(CSE_ALifeItemWeapon::EWeaponAddonStatus::eAddonDisabled)),
                value("eAddonPermanent", int(CSE_ALifeItemWeapon::EWeaponAddonStatus::eAddonPermanent))
            ]
            .def("clone_addons", &CSE_ALifeItemWeapon::clone_addons)
            .def("set_ammo_elapsed", &CSE_ALifeItemWeapon::set_ammo_elapsed)
            .def("get_ammo_elapsed", &CSE_ALifeItemWeapon::get_ammo_elapsed)
            .def("get_ammo_magsize", &CSE_ALifeItemWeapon::get_ammo_magsize)
            .def("get_addon_flags", &CSE_ALifeItemWeapon::get_addon_flags)
    ];
}

void CSE_ALifeItemWeaponShotGun::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemWeaponShotGun, "cse_alife_item_weapon_shotgun", CSE_ALifeItemWeapon)
    ];
}

void CSE_ALifeItemWeaponAutoShotGun::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        luabind_class_item1(
            CSE_ALifeItemWeaponAutoShotGun, "cse_alife_item_weapon_auto_shotgun", CSE_ALifeItemWeapon)
    ];
}

void CSE_ALifeItemDetector::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemDetector, "cse_alife_item_detector", CSE_ALifeItem)
    ];
}

void CSE_ALifeItemArtefact::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        luabind_class_item1(CSE_ALifeItemArtefact, "cse_alife_item_artefact", CSE_ALifeItem)
    ];
}
