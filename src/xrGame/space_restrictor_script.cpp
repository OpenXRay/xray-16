#include "pch_script.h"
#include "space_restrictor.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CSpaceRestrictor, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CSpaceRestrictor, CGameObject>("CSpaceRestrictor")
            .def(constructor<>())
    ];
});
