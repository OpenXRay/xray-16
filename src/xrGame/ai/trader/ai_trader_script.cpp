#include "pch_script.h"
#include "ai_trader.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CAI_Trader, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CAI_Trader, CGameObject>("CAI_Trader")
            .def(constructor<>())
    ];
});
