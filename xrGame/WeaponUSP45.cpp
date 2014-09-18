#include "pch_script.h"
#include "weaponusp45.h"

CWeaponUSP45::CWeaponUSP45()
{}

CWeaponUSP45::~CWeaponUSP45()
{}

using namespace luabind;

#pragma optimize("s",on)
void CWeaponUSP45::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponUSP45,CGameObject>("CWeaponUSP45")
			.def(constructor<>())
	];
}
