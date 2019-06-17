#pragma once

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
struct CWrapperBase : public T, public luabind::wrap_base
{
    typedef T inherited;
    typedef CWrapperBase<T> self_type;

    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action)
    {
        return luabind::call_member<bool>(this, "OnKeyboard", dik, keyboard_action);
    }
    static bool OnKeyboard_static(inherited* ptr, int dik, EUIMessages keyboard_action)
    {
        return ptr->self_type::inherited::OnKeyboardAction(dik, keyboard_action);
    }

    virtual void Update() { luabind::call_member<void>(this, "Update"); }
    static void Update_static(inherited* ptr) { ptr->self_type::inherited::Update(); }
    virtual bool Dispatch(int cmd, int param) { return luabind::call_member<bool>(this, "Dispatch", cmd, param); }
    static bool Dispatch_static(inherited* ptr, int cmd, int param)
    {
        return ptr->self_type::inherited::Dispatch(cmd, param);
    }
};

typedef CWrapperBase<CUIDialogWndEx> WrapType;
typedef CUIDialogWndEx BaseType;

template <typename T>
T* CUIDialogWndEx::GetControl(pcstr name)
{
    shared_str n = name;
    CUIWindow* pWnd = FindChild(n);
    if (pWnd == nullptr)
        return nullptr;

    return smart_cast<T*>(pWnd);
}
