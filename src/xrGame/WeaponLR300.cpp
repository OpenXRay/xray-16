#include "pch_script.h"
#include "WeaponLR300.h"

CWeaponLR300::CWeaponLR300		() : CWeaponMagazined(SOUND_TYPE_WEAPON_SUBMACHINEGUN)
{}

CWeaponLR300::~CWeaponLR300		()
{}

using namespace luabind;

#pragma optimize("s",on)
void CWeaponLR300::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponLR300,CGameObject>("CWeaponLR300")
			.def(constructor<>())
	];
}
