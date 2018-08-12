////////////////////////////////////////////////////////////////////////////
//	Module 		: actor_script.cpp
//	Created 	: 17.01.2008
//  Modified 	: 17.01.2008
//	Author		: Dmitriy Iassenev
//	Description : actor script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "Actor.h"
#include "level_changer.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

IC static void CActor_Export(lua_State* luaState)
{
    module(luaState)[class_<CActor, CGameObject>("CActor").def(constructor<>())

#ifndef BENCHMARK_BUILD
                         ,
        class_<CLevelChanger, CGameObject>("CLevelChanger").def(constructor<>())
#endif
    ];
};

SCRIPT_EXPORT_FUNC(CActor, (CGameObject), CActor_Export);
