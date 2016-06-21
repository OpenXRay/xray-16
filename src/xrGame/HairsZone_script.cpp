#include "pch_script.h"
#include "HairsZone.h"
#include "AmebaZone.h"
#include "NoGravityZone.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CHairsZone, (CGameObject),
{
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
