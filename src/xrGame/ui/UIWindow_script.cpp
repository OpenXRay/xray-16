#include "pch_script.h"
#include "UIDialogWnd.h"
#include "UIDialogHolder.h"
#include "GamePersistent.h"
#include "UILabel.h"
#include "UIMMShniaga.h"
#include "UISleepStatic.h"
#include "ScriptXMLInit.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;

// clang-format off
SCRIPT_EXPORT(CDialogHolder, (), {
    module(luaState)[class_<CDialogHolder>("CDialogHolder")
                         .def("AddDialogToRender", &CDialogHolder::AddDialogToRender)
                         .def("RemoveDialogToRender", &CDialogHolder::RemoveDialogToRender)];
});

SCRIPT_EXPORT(CUIDialogWnd, (CUIWindow), {
    module(luaState)[class_<CUIDialogWnd, CUIWindow>("CUIDialogWnd")
                         .def("ShowDialog", &CUIDialogWnd::ShowDialog)
                         .def("HideDialog", &CUIDialogWnd::HideDialog)
                         .def("GetHolder", &CUIDialogWnd::GetHolder)];
});

SCRIPT_EXPORT(CUIMMShniaga, (CUIWindow), {
    module(luaState)[class_<CUIMMShniaga, CUIWindow>("CUIMMShniaga")
                         .enum_("enum_page_id")[value("epi_main", CUIMMShniaga::epi_main),
                             value("epi_new_game", CUIMMShniaga::epi_new_game),
                             value("epi_new_network_game", CUIMMShniaga::epi_new_network_game)]
                         .def("SetVisibleMagnifier", &CUIMMShniaga::SetVisibleMagnifier)
                         .def("SetPage", &CUIMMShniaga::SetPage)
                         .def("ShowPage", &CUIMMShniaga::ShowPage)];
});

SCRIPT_EXPORT(CUISleepStatic, (CUIStatic),
    { module(luaState)[class_<CUISleepStatic, CUIStatic>("CUISleepStatic").def(constructor<>())]; });
// clang-format on
