#include "pch_script.h"
#include "UIMapInfo.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CUIMapInfo, (CUIWindow),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIMapInfo, CUIWindow>("CUIMapInfo")
            .def(constructor<>())
            .def("Init", &CUIMapInfo::InitMapInfo)
            .def("InitMap", &CUIMapInfo::InitMap)
    ];
});
