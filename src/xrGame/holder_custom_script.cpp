#include "pch_script.h"
#include "holder_custom.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CHolderCustom, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CHolderCustom>("holder")
            .def("engaged", &CHolderCustom::Engaged)
            .def("Action", &CHolderCustom::Action)
            // .def("SetParam", (void (CHolderCustom::*)(int,Fvector2))&CHolderCustom::SetParam)
            .def("SetParam", (void (CHolderCustom::*)(int, Fvector)) & CHolderCustom::SetParam)
            .def("SetEnterLocked", &CHolderCustom::SetEnterLocked)
            .def("SetExitLocked", &CHolderCustom::SetExitLocked)
    ];
});
