#include "pch_script.h"
#include "chimera.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CChimera, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CChimera, CGameObject>("CChimera")
            .def(constructor<>())
    ];
});
