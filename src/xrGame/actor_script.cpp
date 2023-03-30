////////////////////////////////////////////////////////////////////////////
//	Module 		: actor_script.cpp
//	Created 	: 17.01.2008
//  Modified 	: 17.01.2008
//	Author		: Dmitriy Iassenev
//	Description : actor script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "Actor.h"
#include "ActorCondition.h"
#include "level_changer.h"
#include "xrScriptEngine/ScriptExporter.hpp"

IC static void CActor_Export(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CActor, CGameObject>("CActor")
            .def(constructor<>())
            .def("conditions", &CActor::conditions),
        class_<CLevelChanger, CGameObject>("CLevelChanger")
            .def(constructor<>())
    ];
};

SCRIPT_EXPORT_FUNC(CActor, (CGameObject), CActor_Export);
