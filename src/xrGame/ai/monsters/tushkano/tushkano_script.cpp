#include "pch_script.h"
#include "tushkano.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CTushkano, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CTushkano, CGameObject>("CTushkano")
            .def(constructor<>())
    ];
});
