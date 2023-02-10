#include "pch_script.h"
#include "Explosive.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CExplosive, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CExplosive>("explosive")
            .def("explode", &CExplosive::Explode)
    ];
});
