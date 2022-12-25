#include "pch_script.h"
#include "zombie.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CZombie, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CZombie, CGameObject>("CZombie")
            .def(constructor<>())
    ];
});
