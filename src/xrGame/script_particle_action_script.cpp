////////////////////////////////////////////////////////////////////////////
//	Module 		: script_particle_action.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script particle action class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_particle_action.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CScriptParticleAction, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CScriptParticleAction>("particle")
            .def(constructor<>())
            .def(constructor<pcstr, pcstr>())
            .def(constructor<pcstr, pcstr, const CParticleParams&>())
            .def(constructor<pcstr, pcstr, const CParticleParams&, bool>())
            .def(constructor<pcstr, const CParticleParams&>())
            .def(constructor<pcstr, const CParticleParams&, bool>())
            .def("set_particle", &CScriptParticleAction::SetParticle)
            .def("set_bone", &CScriptParticleAction::SetBone)
            .def("set_position", &CScriptParticleAction::SetPosition)
            .def("set_angles", &CScriptParticleAction::SetAngles)
            .def("set_velocity", &CScriptParticleAction::SetVelocity)
            .def("completed", &CScriptAbstractAction::completed)
    ];
});
