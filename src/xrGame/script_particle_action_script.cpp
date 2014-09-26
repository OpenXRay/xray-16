////////////////////////////////////////////////////////////////////////////
//	Module 		: script_particle_action.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script particle action class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_particle_action.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptParticleAction::script_register(lua_State *L)
{
	module(L)
	[
		class_<CScriptParticleAction>("particle")
			.def(					constructor<>())
			.def(					constructor<LPCSTR,LPCSTR>())
			.def(					constructor<LPCSTR,LPCSTR,const CParticleParams &>())
			.def(					constructor<LPCSTR,LPCSTR,const CParticleParams &, bool>())
			.def(					constructor<LPCSTR,const CParticleParams &>())
			.def(					constructor<LPCSTR,const CParticleParams &, bool>())
			.def("set_particle",	&CScriptParticleAction::SetParticle)
			.def("set_bone",		&CScriptParticleAction::SetBone)
			.def("set_position",	&CScriptParticleAction::SetPosition)
			.def("set_angles",		&CScriptParticleAction::SetAngles)
			.def("set_velocity",	&CScriptParticleAction::SetVelocity)
			.def("completed",		(bool (CScriptParticleAction::*)())(&CScriptParticleAction::completed))
	];
}
