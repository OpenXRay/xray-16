////////////////////////////////////////////////////////////////////////////
//	Module 		: script_effector.cpp
//	Created 	: 06.02.2004
//  Modified 	: 06.02.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script effector class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_effector.h"
#include "script_effector_wrapper.h"

using namespace luabind;

void SPPInfo_assign(SPPInfo *self, SPPInfo *obj)
{
	*self	= *obj;
}

void add_effector(CScriptEffector *self)
{
	self->Add		();
}

void remove_effector(CScriptEffector *self)
{
	self->Remove	();
}

#pragma optimize("s",on)
void CScriptEffector::script_register(lua_State *L)
{
	module(L)
	[
		class_<SPPInfo::SDuality>("duality")
			.def_readwrite("h",					&SPPInfo::SDuality::h)
			.def_readwrite("v",					&SPPInfo::SDuality::v)
			.def(								constructor<>())
			.def(								constructor<float,float>())
			.def("set",							&SPPInfo::SDuality::set),

		class_<SPPInfo::SColor>	("color")
			.def_readwrite("r",					&SPPInfo::SColor::r)
			.def_readwrite("g",					&SPPInfo::SColor::g)
			.def_readwrite("b",					&SPPInfo::SColor::b)
			.def(								constructor<>())
			.def(								constructor<float,float,float>())
			.def("set",							&SPPInfo::SColor::set),

		class_<SPPInfo::SNoise>("noise")
			.def_readwrite("intensity",			&SPPInfo::SNoise::intensity)
			.def_readwrite("grain",				&SPPInfo::SNoise::grain)
			.def_readwrite("fps",				&SPPInfo::SNoise::fps)
			.def(								constructor<>())
			.def(								constructor<float,float,float>())
			.def("set",							&SPPInfo::SNoise::set),

		class_<SPPInfo>("effector_params")
			.def_readwrite("blur",				&SPPInfo::blur)
			.def_readwrite("gray",				&SPPInfo::gray)
			.def_readwrite("dual",				&SPPInfo::duality)
			.def_readwrite("noise",				&SPPInfo::noise)
			.def_readwrite("color_base",		&SPPInfo::color_base)
			.def_readwrite("color_gray",		&SPPInfo::color_gray)
			.def_readwrite("color_add",			&SPPInfo::color_add)
			.def(								constructor<>())
			.def("assign",						&SPPInfo_assign),

		class_<CScriptEffector, CScriptEffectorWrapper>("effector")
			.def(								constructor<int,float>())
			.def("start",						&add_effector,		adopt(_1))
			.def("finish",						&remove_effector,	adopt(_1))
			.def("process",	 					&CScriptEffector::process,	&CScriptEffectorWrapper::process_static)
	];
}
