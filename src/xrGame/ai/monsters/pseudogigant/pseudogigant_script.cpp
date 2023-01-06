#include "pch_script.h"
#include "pseudo_gigant.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CPseudoGigant, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CPseudoGigant, CGameObject>("CPseudoGigant")
            .def(constructor<>())
    ];
});
