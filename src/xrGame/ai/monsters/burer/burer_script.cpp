#include "pch_script.h"
#include "burer.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CBurer, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CBurer, CGameObject>("CBurer")
            .def(constructor<>())
    ];
});
