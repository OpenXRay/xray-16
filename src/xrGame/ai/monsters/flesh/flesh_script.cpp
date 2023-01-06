#include "pch_script.h"
#include "flesh.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CAI_Flesh, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CAI_Flesh, CGameObject>("CAI_Flesh")
            .def(constructor<>())
    ];
});
