////////////////////////////////////////////////////////////////////////////
//	Module 		: saved_game_wrapper_script.cpp
//	Created 	: 21.02.2006
//  Modified 	: 21.02.2006
//	Author		: Dmitriy Iassenev
//	Description : saved game wrapper class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#include "saved_game_wrapper.h"
#include "ai_space.h"
#include "xr_time.h"

void CSavedGameWrapper::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CSavedGameWrapper>("CSavedGameWrapper")
            .def(constructor<pcstr>())
            .def("game_time", +[](const CSavedGameWrapper* self)
            {
                return (xrTime(self->game_time()));
            })
            .def("level_id", &CSavedGameWrapper::level_id)
            .def("level_name", &CSavedGameWrapper::level_name)
            .def("actor_health", &CSavedGameWrapper::actor_health),

        def("valid_saved_game", (bool (*)(pcstr))(&CSavedGameWrapper::valid_saved_game)),
        def("is_compatible_saved_game", (bool (*)(pcstr))(&CSavedGameWrapper::is_compatible_saved_game))
    ];
}
