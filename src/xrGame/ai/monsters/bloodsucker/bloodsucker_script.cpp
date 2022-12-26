#include "pch_script.h"
#include "bloodsucker.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CAI_Bloodsucker, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CAI_Bloodsucker, CGameObject>("CAI_Bloodsucker")
            .def(constructor<>())
            .def("force_visibility_state", &CAI_Bloodsucker::force_visibility_state)
    ];
});
