////////////////////////////////////////////////////////////////////////////
//	Module 		: particle_params.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Particle parameters class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "particle_params.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CParticleParams, (),
{
	module(luaState)
	[
		class_<CParticleParams>("particle_params")
			.def(								constructor<>())
			.def(								constructor<const Fvector &>())
			.def(								constructor<const Fvector &,const Fvector &>())
			.def(								constructor<const Fvector &,const Fvector &,const Fvector &>())
	];
});
