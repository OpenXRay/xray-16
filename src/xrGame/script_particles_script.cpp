////////////////////////////////////////////////////////////////////////////
//	Module 		: script_sound_script.cpp
//	Created 	: 06.02.2004
//  Modified 	: 06.02.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script sound class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_particles.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CScriptParticles, (), {
    module(luaState)[class_<CScriptParticles>("particles_object")
                         .def(constructor<LPCSTR>())
                         .def("play", &CScriptParticles::Play)
                         .def("play_at_pos", &CScriptParticles::PlayAtPos)
                         .def("stop", &CScriptParticles::Stop)
                         // preserved for backwards compatibility
                         // XXX: review
                         .def("stop_deffered", &CScriptParticles::StopDeferred)
                         .def("stop_deferred", &CScriptParticles::StopDeferred)

                         .def("playing", &CScriptParticles::IsPlaying)
                         .def("looped", &CScriptParticles::IsLooped)

                         .def("move_to", &CScriptParticles::MoveTo)

                         .def("load_path", &CScriptParticles::LoadPath)
                         .def("start_path", &CScriptParticles::StartPath)
                         .def("stop_path", &CScriptParticles::StopPath)
                         .def("pause_path", &CScriptParticles::PausePath)];
});
