#include "pch_script.h"
#include "fracture.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CFracture, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CFracture, CGameObject>("CFracture")
            .def(constructor<>())
    ];
});
