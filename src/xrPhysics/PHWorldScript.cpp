#include "StdAfx.h"

#include "Physics.h"
#include "PHWorld.h"
#include "PHCommander.h"

#include "xrScriptEngine/script_space.hpp"

void CPHWorld::script_register(lua_State* luaState)
{
    using namespace luabind;

	module(luaState)
	[
		class_<CPHWorld>("physics_world")
		    .def("set_gravity",					&CPHWorld::SetGravity)
		    .def("gravity",						&CPHWorld::Gravity)
		    .def("add_call",					&CPHWorld::AddCall)
	];

    module(luaState, "level")
    [
        def("physics_world", +[]
        {
            return ph_world;
        }),
        // X-Ray Extensions:
        def("get_ph_time_factor", +[]
        {
            return phTimefactor;
        }),
        def("set_ph_time_factor", +[](float time_factor)
        {
            return phTimefactor = time_factor;
        })
    ];
}
