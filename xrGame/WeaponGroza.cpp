#include "pch_script.h"
#include "weapongroza.h"

CWeaponGroza::CWeaponGroza() :CWeaponMagazinedWGrenade(SOUND_TYPE_WEAPON_SUBMACHINEGUN) 
{}

CWeaponGroza::~CWeaponGroza() 
{}

using namespace luabind;

#pragma optimize("s",on)
void CWeaponGroza::script_register	(lua_State *L)
{
	module(L)
	[
		class_<CWeaponGroza,CGameObject>("CWeaponGroza")
			.def(constructor<>())
	];
}
