#include "pch_script.h"
#include "dog.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CAI_Dog, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CAI_Dog, CGameObject>("CAI_Dog")
            .def(constructor<>())
    ];
});
