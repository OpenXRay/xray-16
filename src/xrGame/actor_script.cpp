////////////////////////////////////////////////////////////////////////////
//	Module 		: actor_script.cpp
//	Created 	: 17.01.2008
//  Modified 	: 17.01.2008
//	Author		: Dmitriy Iassenev
//	Description : actor script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "actor.h"
#include "ActorCondition.h"
#include "level_changer.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "holder_custom.h"
#include "script_game_object.h"

using namespace luabind;

IC static void CActor_Export(lua_State* luaState)
{
    module(luaState)
    [class_<CActor, bases<CEntityAlive, CInventoryOwner>>("CActor")
            .def(constructor<>())
            .def("conditions", &CActor::conditions)
            .def("ObjectWeLookingAt", &CActor::ObjectWeLookingAt_script)
            .def("GetDefaultActionForObject", &CActor::GetDefaultActionForObject)
            .def("detach_Vehicle", &CActor::detach_Vehicle)
            .def("attach_Vehicle", &CActor::attach_Vehicle)
            .def("GetProtection_ArtefactsOnBelt", &CActor::GetProtection_ArtefactsOnBelt)
            .def("MoveActor", &CActor::MoveActor)
            .def("get_state", &CActor::get_state)
            .def("set_state", &CActor::set_state)
            .def("get_state_wishful", &CActor::get_state_wishful)
            .def("set_state_wishful", &CActor::set_state_wishful)
            .def("GetNightVisionStatus", &CActor::GetNightVisionStatus)
            .def("SetNightVisionAllowed", &CActor::SetNightVisionAllowed)
            .def("MaxCarryWeight", &CActor::MaxCarryWeight)
            .def("MaxWalkWeight", &CActor::MaxWalkWeight)
            .def("get_additional_weight", &CActor::get_additional_weight)
            .def("IsZoomAimingMode", &CActor::IsZoomAimingMode)
            .def("GetFireDispersion", &CActor::GetFireDispertion)
            .def("GetWeaponAccuracy", &CActor::GetWeaponAccuracy)
            .def("use_HolderEx", &CActor::use_HolderEx)
            .def_readwrite("m_fWalkAccel", &CActor::m_fWalkAccel)
            .def_readwrite("m_fJumpSpeed", &CActor::m_fJumpSpeed)
            .def_readwrite("m_fRunFactor", &CActor::m_fRunFactor)
            .def_readwrite("m_fRunBackFactor", &CActor::m_fRunBackFactor)
            .def_readwrite("m_fWalkBackFactor", &CActor::m_fWalkBackFactor)
            .def_readwrite("m_fCrouchFactor", &CActor::m_fCrouchFactor)
            .def_readwrite("m_fClimbFactor", &CActor::m_fClimbFactor)
            .def_readwrite("m_fSprintFactor", &CActor::m_fSprintFactor)
            .def_readwrite("m_fWalk_StrafeFactor", &CActor::m_fWalk_StrafeFactor)
            .def_readwrite("m_fRun_StrafeFactor", &CActor::m_fRun_StrafeFactor)
            .def_readonly("CurrentHeight", &CActor::CurrentHeight)
            .def("CanSprint", &CActor::CanSprint)
            .def("CanRun", &CActor::CanRun)
            .def("is_jump", &CActor::is_jump)
            .def("CameraHeight", &CActor::CameraHeight)


#ifndef BENCHMARK_BUILD
                         ,
        class_<CLevelChanger, CGameObject>("CLevelChanger").def(constructor<>())
#endif
    ];
};

SCRIPT_EXPORT_FUNC(CActor, (CGameObject), CActor_Export);
