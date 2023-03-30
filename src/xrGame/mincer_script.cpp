#include "pch_script.h"
#include "Mincer.h"
#include "RadioactiveZone.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CMincer, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CMincer, CGameObject>("CMincer")
            .def(constructor<>()),
        class_<CRadioactiveZone, CGameObject>("CRadioactiveZone")
            .def(constructor<>())
    ];
});
