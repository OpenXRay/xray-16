#include "pch_script.h"
#include "UIGameCustom.h"
#include "Level.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

CUIGameCustom* get_hud() { return CurrentGameUI(); }
SCRIPT_EXPORT(CUIGameCustom, (), {
    module(luaState)[class_<StaticDrawableWrapper>("StaticDrawableWrapper")
                         .def_readwrite("m_endTime", &StaticDrawableWrapper::m_endTime)
                         .def("wnd", &StaticDrawableWrapper::wnd),

        class_<CUIGameCustom>("CUIGameCustom")
            .def("AddDialogToRender", &CUIGameCustom::AddDialogToRender)
            .def("RemoveDialogToRender", &CUIGameCustom::RemoveDialogToRender)
            .def("AddCustomStatic", &CUIGameCustom::AddCustomStatic)
            .def("RemoveCustomStatic", &CUIGameCustom::RemoveCustomStatic)
            .def("HideActorMenu", &CUIGameCustom::HideActorMenu)
             //Alundaio
            .def("UpdateActorMenu", &CUIGameCustom::UpdateActorMenu)
            .def("CurrentItemAtCell", &CUIGameCustom::CurrentItemAtCell)
            //-Alundaio
            .def("HidePdaMenu", &CUIGameCustom::HidePdaMenu)
            .def("show_messages", &CUIGameCustom::ShowMessagesWindow)
            .def("hide_messages", &CUIGameCustom::HideMessagesWindow)
            .def("GetCustomStatic", &CUIGameCustom::GetCustomStatic)
            .def("update_fake_indicators", &CUIGameCustom::update_fake_indicators)
            .def("enable_fake_indicators", &CUIGameCustom::enable_fake_indicators),
        def("get_hud", &get_hud)];
});
