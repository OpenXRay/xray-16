#include "pch_script.h"

#include "UIScriptWnd.h"

#include "xrUICore/ListWnd/UIListWnd.h"
#include "xrUICore/TabControl/UITabControl.h"

#include "xrScriptEngine/ScriptExporter.hpp"

//UI-controls
class CUIStatic;
class CUIEditBox;
class CUIDialogWnd;
class CUIFrameWindow;
class CUIFrameLineWnd;
class CUIProgressBar;
class CUITabControl;
class CUIListWnd;
class CUIListBox;

template <typename T>
T* CUIDialogWndEx::GetControl(pcstr name)
{
    shared_str n = name;
    CUIWindow* pWnd = FindChild(n);
    if (pWnd == nullptr)
        return nullptr;

    return smart_cast<T*>(pWnd);
}

struct CUIDialogWndExWrapperBase final : public CUIDialogWndEx, public luabind::wrap_base
{
    typedef CUIDialogWndEx inherited;
    typedef CUIDialogWndExWrapperBase self_type;

    CUIDialogWndExWrapperBase()
    {
        SetWindowName(GetDebugType());
    }

    bool OnKeyboardAction(int dik, EUIMessages keyboard_action) override
    {
        return luabind::call_member<bool>(this, "OnKeyboard", dik, keyboard_action);
    }

    static bool OnKeyboard_static(inherited* ptr, int dik, EUIMessages keyboard_action)
    {
        return ptr->self_type::inherited::OnKeyboardAction(dik, keyboard_action);
    }

    void Update() override
    {
        luabind::call_member<void>(this, "Update");
    }

    static void Update_static(inherited* ptr)
    {
        ptr->self_type::inherited::Update();
    }

    bool Dispatch(int cmd, int param) override
    {
        return luabind::call_member<bool>(this, "Dispatch", cmd, param);
    }

    static bool Dispatch_static(inherited* ptr, int cmd, int param)
    {
        return ptr->self_type::inherited::Dispatch(cmd, param);
    }

    pcstr GetDebugType() override { return "CUIScriptWnd"; }

    bool FillDebugTree(const CUIDebugState& debugState) override
    {
#ifndef MASTER_GOLD
        if (!inherited::FillDebugTree(debugState))
            return false;

        return true;
#else
        return false;
#endif
    }
};

// clang-format off
SCRIPT_EXPORT(CUIDialogWndEx, (CUIDialogWnd, IFactoryObject),
{
    using namespace luabind;
    using BaseType = CUIDialogWndEx;
    using WrapType = CUIDialogWndExWrapperBase;

    module(luaState)
    [
        class_<CUIDialogWndEx, luabind::bases<CUIDialogWnd, IFactoryObject>, luabind::default_holder, WrapType>("CUIScriptWnd")
            .def(constructor<>())
            .def("AddCallback", &BaseType::AddCallback)
            .def("Register", (void (BaseType::*)(CUIWindow*, pcstr)) & BaseType::Register)
            .def("OnKeyboard", &BaseType::OnKeyboardAction, &WrapType::OnKeyboard_static)
            .def("Update", &BaseType::Update, &WrapType::Update_static)
            .def("Dispatch", &BaseType::Dispatch, &WrapType::Dispatch_static)
            .def("Load", &BaseType::Load)

            .def("GetStatic", (CUIStatic* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIStatic>)
            .def("GetEditBox", (CUIEditBox* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIEditBox>)
            .def("GetDialogWnd", (CUIDialogWnd* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIDialogWnd>)
            .def("GetFrameWindow", (CUIFrameWindow* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIFrameWindow>)
            .def("GetFrameLineWnd", (CUIFrameLineWnd* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIFrameLineWnd>)
            .def("GetProgressBar", (CUIProgressBar* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIProgressBar>)
            .def("GetTabControl", (CUITabControl* (BaseType::*)(pcstr)) &BaseType::GetControl<CUITabControl>)
            .def("GetListBox", (CUIListBox* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIListBox>)
            .def("GetListWnd", (CUIListWnd* (BaseType::*)(pcstr)) &BaseType::GetControl<CUIListWnd>)
    ];
});
// clang-format on
