#include "pch_script.h"
#include "alife_space.h"
#include "Car.h"
#include "CarWeapon.h"
#include "script_game_object.h"

using namespace luabind;

#pragma optimize("s",on)
void CCar::script_register(lua_State *L)
{
	module(L)
	[
		class_<CCar,bases<CGameObject,CHolderCustom> >("CCar")
			.enum_("wpn_action")
				[
					value("eWpnDesiredDir",							int(CCarWeapon::eWpnDesiredDir)),
					value("eWpnDesiredPos",							int(CCarWeapon::eWpnDesiredPos)),
					value("eWpnActivate",							int(CCarWeapon::eWpnActivate)),
					value("eWpnFire",								int(CCarWeapon::eWpnFire)),
					value("eWpnAutoFire",							int(CCarWeapon::eWpnAutoFire)),
					value("eWpnToDefaultDir",						int(CCarWeapon::eWpnToDefaultDir))
				]
		.def("Action",			&CCar::Action)
//		.def("SetParam",		(void (CCar::*)(int,Fvector2)) &CCar::SetParam)
		.def("SetParam",		(void (CCar::*)(int,Fvector)) &CCar::SetParam)
		.def("CanHit",			&CCar::WpnCanHit)
		.def("FireDirDiff",		&CCar::FireDirDiff)
		.def("IsObjectVisible",	&CCar::isObjectVisible)
		.def("HasWeapon",		&CCar::HasWeapon)
		.def("CurrentVel",		&CCar::CurrentVel)
		.def("GetfHealth",		&CCar::GetfHealth)
		.def("SetfHealth",		&CCar::SetfHealth)
		.def("SetExplodeTime",	&CCar::SetExplodeTime)
		.def("ExplodeTime",		&CCar::ExplodeTime)
		.def("CarExplode",		&CCar::CarExplode)
		/************************************************** added by Ray Twitty (aka Shadows) START **************************************************/
		.def("GetfFuel",		&CCar::GetfFuel)
		.def("SetfFuel",		&CCar::SetfFuel)
		.def("GetfFuelTank",	&CCar::GetfFuelTank)
		.def("SetfFuelTank",	&CCar::SetfFuelTank)
		.def("GetfFuelConsumption",		&CCar::GetfFuelConsumption)
		.def("SetfFuelConsumption",		&CCar::SetfFuelConsumption)
		.def("ChangefFuel",		&CCar::ChangefFuel)
		.def("ChangefHealth",	&CCar::ChangefHealth)
		.def("PlayDamageParticles",		&CCar::PlayDamageParticles)
		.def("StopDamageParticles",		&CCar::StopDamageParticles)
		.def("StartEngine",		&CCar::StartEngine)
		.def("StopEngine",		&CCar::StopEngine)
		.def("IsActiveEngine",	&CCar::isActiveEngine)
		/*************************************************** added by Ray Twitty (aka Shadows) END ***************************************************/
		.def(constructor<>())
	];
}