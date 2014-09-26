#include "pch_script.h"
#include "weaponsvu.h"

CWeaponSVU::CWeaponSVU(void)
{}

CWeaponSVU::~CWeaponSVU(void)
{}

using namespace luabind;

#pragma optimize("s",on)
void CWeaponSVU::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponSVU,CGameObject>("CWeaponSVU")
			.def(constructor<>())
	];
}
