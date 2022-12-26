#include "pch_script.h"
#include "snork.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CSnork, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CSnork, CGameObject>("CSnork")
        .def(constructor<>())
    ];
});
