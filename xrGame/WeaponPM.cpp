#include "pch_script.h"
#include "WeaponPM.h"

CWeaponPM::CWeaponPM() : CWeaponPistol()
{}

CWeaponPM::~CWeaponPM()
{}

using namespace luabind;

#pragma optimize("s",on)
void CWeaponPM::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponPM,CGameObject>("CWeaponPM")
			.def(constructor<>())
	];
}
