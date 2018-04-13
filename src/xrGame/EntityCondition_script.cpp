#include "pch_script.h"
#include "ActorCondition.h"
#include "EntityCondition.h"

using namespace luabind;

IC static void CEntityCondition_Export(lua_State* luaState)
{
    module(luaState)
    [
        class_<CEntityCondition>("CEntityCondition")
            //.def(constructor<>())
            .def("GetWhoHitLastTimeID", &CEntityCondition::GetWhoHitLastTimeID)
            .def("GetPower", &CEntityCondition::GetPower)
            .def("GetRadiation", &CEntityCondition::GetRadiation)
            .def("GetPsyHealth", &CEntityCondition::GetPsyHealth)
            .def("GetSatiety", &CEntityCondition::GetSatiety)
            .def("GetEntityMorale", &CEntityCondition::GetEntityMorale)
            .def("GetHealthLost", &CEntityCondition::GetHealthLost)
            .def("IsLimping", &CEntityCondition::IsLimping)
            .def("ChangeSatiety", &CEntityCondition::ChangeSatiety)
            .def("ChangeHealth", &CEntityCondition::ChangeHealth)
            .def("ChangePower", &CEntityCondition::ChangePower)
            .def("ChangeRadiation", &CEntityCondition::ChangeRadiation)
            .def("ChangePsyHealth", &CEntityCondition::ChangePsyHealth)
            .def("ChangeAlcohol", &CEntityCondition::ChangeAlcohol)
            .def("SetMaxPower", &CEntityCondition::SetMaxPower)
            .def("GetMaxPower", &CEntityCondition::GetMaxPower)
            .def("ChangeEntityMorale", &CEntityCondition::ChangeEntityMorale)
            .def("ChangeBleeding", &CEntityCondition::ChangeBleeding)
            .def("BleedingSpeed", &CEntityCondition::BleedingSpeed)
            .def("ChangeBleeding", &CEntityCondition::ChangeBleeding)
            .def("ChangeBleeding", &CEntityCondition::ChangeBleeding)
            .def("ChangeBleeding", &CEntityCondition::ChangeBleeding)
    ];
}

IC static void CActorCondition_Export(lua_State* luaState)
{
    module(luaState)
    [
        class_<CActorCondition, CEntityCondition>("CActorCondition")
            //.def(constructor<>())
            .def("BoostMaxWeight", &CActorCondition::BoostMaxWeight)
            .def("BoostHpRestore", &CActorCondition::BoostHpRestore)
            .def("BoostPowerRestore", &CActorCondition::BoostPowerRestore)
            .def("BoostRadiationRestore", &CActorCondition::BoostRadiationRestore)
            .def("BoostBleedingRestore", &CActorCondition::BoostBleedingRestore)
            .def("BoostBurnImmunity", &CActorCondition::BoostBurnImmunity)
            .def("BoostShockImmunity", &CActorCondition::BoostShockImmunity)
            .def("BoostRadiationImmunity", &CActorCondition::BoostRadiationImmunity)
            .def("BoostTelepaticImmunity", &CActorCondition::BoostTelepaticImmunity)
            .def("BoostChemicalBurnImmunity", &CActorCondition::BoostChemicalBurnImmunity)
            .def("BoostExplImmunity", &CActorCondition::BoostExplImmunity)
            .def("BoostStrikeImmunity", &CActorCondition::BoostStrikeImmunity)
            .def("BoostFireWoundImmunity", &CActorCondition::BoostFireWoundImmunity)
            .def("BoostWoundImmunity", &CActorCondition::BoostWoundImmunity)
            .def("BoostRadiationProtection", &CActorCondition::BoostRadiationProtection)
            .def("BoostTelepaticProtection", &CActorCondition::BoostTelepaticProtection)
            .def("BoostChemicalBurnProtection", &CActorCondition::BoostChemicalBurnProtection)
            .def("IsLimping", &CActorCondition::IsLimping)
            .def("IsCantWalk", &CActorCondition::IsCantWalk)
            .def("IsCantWalkWeight", &CActorCondition::IsCantWalkWeight)
            .def("IsCantSprint", &CActorCondition::IsCantSprint)
            .def_readwrite("m_MaxWalkWeight", &CActorCondition::m_MaxWalkWeight)
    ];
}

SCRIPT_EXPORT_FUNC(CEntityCondition, (), CEntityCondition_Export);
SCRIPT_EXPORT_FUNC(CActorCondition, (CEntityCondition), CActorCondition_Export);
