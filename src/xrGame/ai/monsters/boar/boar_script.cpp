#include "pch_script.h"
#include "boar.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CAI_Boar, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CAI_Boar, CGameObject>("CAI_Boar")
        .def(constructor<>())
    ];
});
