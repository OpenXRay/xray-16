#include "pch_script.h"

#include "xrUICore/Static/UIStatic.h"

#include "UIGameCustom.h"
#include "Level.h"

void CUIGameCustom::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<StaticDrawableWrapper>("StaticDrawableWrapper")
            .def_readwrite("m_endTime", &StaticDrawableWrapper::m_endTime)
            .def("wnd", &StaticDrawableWrapper::wnd),

        class_<CUIGameCustom, CDialogHolder>("CUIGameCustom")
            .def("AddDialogToRender", &CUIGameCustom::AddDialogToRender)
            .def("RemoveDialogToRender", &CUIGameCustom::RemoveDialogToRender)
            .def("AddCustomStatic", +[](CUIGameCustom* self, pcstr id, bool singleInstance)
            {
                return self->AddCustomStatic(id, singleInstance);
            })
            .def("AddCustomStatic", &CUIGameCustom::AddCustomStatic)
            // Dead Air / CoC scripts call AddCustomStatic(id) with a single argument.
            .def("AddCustomStatic", +[](CUIGameCustom* self, pcstr id)
            {
                return self->AddCustomStatic(id, false);
            })
            .def("RemoveCustomStatic", &CUIGameCustom::RemoveCustomStatic)
            .def("HideActorMenu", &CUIGameCustom::HideActorMenu)
             //Alundaio
            .def("ShowActorMenu", &CUIGameCustom::ShowActorMenu)
            .def("UpdateActorMenu", &CUIGameCustom::UpdateActorMenu)
            .def("CurrentItemAtCell", &CUIGameCustom::CurrentItemAtCell)
            //-Alundaio
            .def("HidePdaMenu", &CUIGameCustom::HidePdaMenu)
            .def("show_messages", &CUIGameCustom::ShowMessagesWindow)
            .def("hide_messages", &CUIGameCustom::HideMessagesWindow)
            .def("GetCustomStatic", &CUIGameCustom::GetCustomStatic)
            .def("update_fake_indicators", &CUIGameCustom::update_fake_indicators)
            .def("enable_fake_indicators", &CUIGameCustom::enable_fake_indicators),

        def("get_hud", +[]() -> CUIGameCustom* { return CurrentGameUI(); })
    ];
}
