#include "pch_script.h"
#include "controller.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CController, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CController, CGameObject>("CController")
            .def(constructor<>())
    ];
});
