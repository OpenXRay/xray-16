#include "pch_script.h"

#include "alife_space.h"
#include "Car.h"
#include "CarWeapon.h"
#include "script_game_object.h"

void CCar::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CCar, bases<CGameObject, CHolderCustom>>("CCar")
            .enum_("wpn_action")
            [
                value("eWpnDesiredDir", int(CCarWeapon::eWpnDesiredDir)),
                value("eWpnDesiredPos", int(CCarWeapon::eWpnDesiredPos)),
                value("eWpnActivate", int(CCarWeapon::eWpnActivate)),
                value("eWpnFire", int(CCarWeapon::eWpnFire)),
                value("eWpnAutoFire", int(CCarWeapon::eWpnAutoFire)),
                value("eWpnToDefaultDir", int(CCarWeapon::eWpnToDefaultDir))
            ]
            .def("Action", &CCar::Action)
            //		.def("SetParam",		(void (CCar::*)(int,Fvector2)) &CCar::SetParam)
            .def("SetParam", (void (CCar::*)(int, Fvector)) & CCar::SetParam)
            .def("CanHit", &CCar::WpnCanHit)
            .def("FireDirDiff", &CCar::FireDirDiff)
            .def("IsObjectVisible", &CCar::isObjectVisible)
            .def("HasWeapon", &CCar::HasWeapon)
            .def("CurrentVel", &CCar::CurrentVel)
            .def("GetfHealth", &CCar::GetfHealth)
            .def("SetfHealth", &CCar::SetfHealth)
            .def("SetExplodeTime", &CCar::SetExplodeTime)
            .def("ExplodeTime", &CCar::ExplodeTime)
            .def("CarExplode", &CCar::CarExplode)
            // X-Ray Extensions:
            .def("get_fuel", &CCar::GetfFuel)
            .def("set_fuel", &CCar::SetfFuel)
            .def("get_fuel_tank", &CCar::GetfFuelTank)
            .def("set_fuel_tank", &CCar::SetfFuelTank)
            .def("get_fuel_consumption", &CCar::GetfFuelConsumption)
            .def("set_fuel_consumption", &CCar::SetfFuelConsumption)
            /***** added by Ray Twitty (aka Shadows) START *****/
            .def("GetfFuel", &CCar::GetfFuel)
            .def("SetfFuel", &CCar::SetfFuel)
            .def("GetfFuelTank", &CCar::GetfFuelTank)
            .def("SetfFuelTank", &CCar::SetfFuelTank)
            .def("GetfFuelConsumption", &CCar::GetfFuelConsumption)
            .def("SetfFuelConsumption", &CCar::SetfFuelConsumption)
            .def("ChangefFuel", &CCar::ChangefFuel)
            .def("ChangefHealth", &CCar::ChangefHealth)
            .def("PlayDamageParticles", &CCar::PlayDamageParticles)
            .def("StopDamageParticles", &CCar::StopDamageParticles)
            .def("StartEngine", &CCar::StartEngine)
            .def("StopEngine", &CCar::StopEngine)
            .def("IsActiveEngine", &CCar::isActiveEngine)
            .def("HandBreak", &CCar::HandBreak)
            .def("ReleaseHandBreak", &CCar::ReleaseHandBreak)
            .def("GetRPM", &CCar::GetRPM)
            .def("SetRPM", &CCar::SetRPM)
            /***** added by Ray Twitty (aka Shadows) END *****/
            .def(constructor<>())
    ];
}
