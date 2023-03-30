#include "pch_script.h"
#include "cat.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CCat, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CCat, CGameObject>("CCat")
            .def(constructor<>())
    ];
});
