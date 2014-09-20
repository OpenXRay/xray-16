#include "pch_script.h"
#include "WeaponFN2000.h"

CWeaponFN2000::CWeaponFN2000() : CWeaponMagazined(SOUND_TYPE_WEAPON_SNIPERRIFLE)
{
}

CWeaponFN2000::~CWeaponFN2000()
{
}

using namespace luabind;

#pragma optimize("s",on)
void CWeaponFN2000::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponFN2000,CGameObject>("CWeaponFN2000")
			.def(constructor<>())
	];
}
