#include "pch_script.h"
#include "HairsZone.h"
#include "AmebaZone.h"
#include "NoGravityZone.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CHairsZone, (CGameObject),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CHairsZone, CGameObject>("CHairsZone")
            .def(constructor<>()),
        class_<CAmebaZone, CGameObject>("CAmebaZone")
            .def(constructor<>()),
        class_<CNoGravityZone,CGameObject>("CNoGravityZone")
            .def(constructor<>())
    ];
});
