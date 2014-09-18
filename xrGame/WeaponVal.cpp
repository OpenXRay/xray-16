#include "pch_script.h"
#include "weaponval.h"

CWeaponVal::CWeaponVal(void) : CWeaponMagazined(SOUND_TYPE_WEAPON_SUBMACHINEGUN)
{
}

CWeaponVal::~CWeaponVal(void)
{
}

using namespace luabind;

#pragma optimize("s",on)
void CWeaponVal::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponVal,CGameObject>("CWeaponVal")
			.def(constructor<>())
	];
}
