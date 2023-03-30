#include "pch_script.h"
#include "poltergeist.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CPoltergeist, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CPoltergeist, CGameObject>("CPoltergeist")
            .def(constructor<>())
    ];
});
