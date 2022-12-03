#include "pch_script.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "xrScriptEngine/ScriptExporter.hpp"

void CCustomOutfit_Export(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CCustomOutfit, CGameObject>("CCustomOutfit")
            .def(constructor<>())
            .def_readwrite("m_fPowerLoss", &CCustomOutfit::m_fPowerLoss)
            .def_readwrite("m_additional_weight", &CCustomOutfit::m_additional_weight)
            .def_readwrite("m_additional_weight2", &CCustomOutfit::m_additional_weight2)
            .def_readwrite("m_fHealthRestoreSpeed", &CCustomOutfit::m_fHealthRestoreSpeed)
            .def_readwrite("m_fRadiationRestoreSpeed", &CCustomOutfit::m_fRadiationRestoreSpeed)
            .def_readwrite("m_fSatietyRestoreSpeed", &CCustomOutfit::m_fSatietyRestoreSpeed)
            .def_readwrite("m_fPowerRestoreSpeed", &CCustomOutfit::m_fPowerRestoreSpeed)
            .def_readwrite("m_fBleedingRestoreSpeed", &CCustomOutfit::m_fBleedingRestoreSpeed)
            .def_readonly("bIsHelmetAvaliable", &CCustomOutfit::bIsHelmetAvaliable)
            .def("BonePassBullet", &CCustomOutfit::BonePassBullet)
            .def("get_artefact_count", &CCustomOutfit::get_artefact_count)
            .def("GetDefHitTypeProtection", +[](CCustomOutfit* self, int hit_type)
            {
                return self->GetDefHitTypeProtection(ALife::EHitType(hit_type));
            })
            .def("GetHitTypeProtection", +[](CCustomOutfit* self, int hit_type, pcstr element)
            {
                const u16 elem = u16(size_t(element));
                return self->GetHitTypeProtection(ALife::EHitType(hit_type), elem);
            })
            .def("GetBoneArmor", &CCustomOutfit::GetBoneArmor)
    ];
}

SCRIPT_EXPORT_FUNC(CCustomOutfit, (CGameObject), CCustomOutfit_Export);

void CHelmet_Export(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CHelmet, CGameObject>("CHelmet")
            .def(constructor<>())
            .def_readwrite("m_fPowerLoss", &CHelmet::m_fPowerLoss)
            .def_readwrite("m_fHealthRestoreSpeed", &CHelmet::m_fHealthRestoreSpeed)
            .def_readwrite("m_fRadiationRestoreSpeed", &CHelmet::m_fRadiationRestoreSpeed)
            .def_readwrite("m_fSatietyRestoreSpeed", &CHelmet::m_fSatietyRestoreSpeed)
            .def_readwrite("m_fPowerRestoreSpeed", &CHelmet::m_fPowerRestoreSpeed)
            .def_readwrite("m_fBleedingRestoreSpeed", &CHelmet::m_fBleedingRestoreSpeed)
            .def("GetDefHitTypeProtection", +[](CHelmet* self, int hit_type)
            {
                return self->GetDefHitTypeProtection(ALife::EHitType(hit_type));
            })
            .def("GetHitTypeProtection", +[](CHelmet* self, int hit_type, pcstr element)
            {
                const u16 elem = u16(size_t(element));
                return self->GetHitTypeProtection(ALife::EHitType(hit_type), elem);
            })
            .def("GetBoneArmor", &CHelmet::GetBoneArmor)
    ];
}

SCRIPT_EXPORT_FUNC(CHelmet, (CGameObject), CHelmet_Export);
