#include "pch.hpp"
#include "UIProgressBar.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CUIProgressBar, (CUIWindow),
{
    module(luaState)
    [
        class_<CUIProgressBar, CUIWindow>("CUIProgressBar")
            .def(constructor<>())
            .def("SetProgressPos", &CUIProgressBar::SetProgressPos)
            .def("GetProgressPos", &CUIProgressBar::GetProgressPos)

            .def("GetRange_min", &CUIProgressBar::GetRange_min)
            .def("GetRange_max", &CUIProgressBar::GetRange_max)
    ];
});
