////////////////////////////////////////////////////////////////////////////
//	Module 		: script_sound_script.cpp
//	Created 	: 06.02.2004
//  Modified 	: 06.02.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script sound class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_particles.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptParticles::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptParticles>("particles_object")
			.def(								constructor<LPCSTR>())
			.def("play",						&CScriptParticles::Play)
			.def("play_at_pos",					&CScriptParticles::PlayAtPos)
			.def("stop",						&CScriptParticles::Stop)
			.def("stop_deffered",				&CScriptParticles::StopDeffered)

			.def("playing",						&CScriptParticles::IsPlaying)
			.def("looped",						&CScriptParticles::IsLooped)

			.def("move_to",						&CScriptParticles::MoveTo)

			.def("load_path",					&CScriptParticles::LoadPath)
			.def("start_path",					&CScriptParticles::StartPath)
			.def("stop_path",					&CScriptParticles::StopPath)
			.def("pause_path",					&CScriptParticles::PausePath)
	];
}
