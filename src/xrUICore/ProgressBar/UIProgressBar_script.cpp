#include "pch.hpp"
#include "UIProgressBar.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CUIProgressBar, (CUIWindow),
{
    using namespace luabind;

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
