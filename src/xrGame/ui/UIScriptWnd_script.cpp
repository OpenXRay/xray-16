#include "pch_script.h"

#include "UIScriptWnd.h"

#include "xrUICore/ListWnd/UIListWnd.h"
#include "xrUICore/TabControl/UITabControl.h"

#include "xrScriptEngine/ScriptExporter.hpp"

#if LUA_VERSION_NUM < 502
#   define lua_getuservalue lua_getfenv
#   define lua_setuservalue lua_setfenv
#endif

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
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
        if (debugState.selected == this)
            flags |= ImGuiTreeNodeFlags_Selected;

        inherited::FillDebugTree(debugState);

        const auto& wrap_ref = luabind::detail::wrap_access::ref(*this);

        lua_State* ls = wrap_ref.state();
        const int prev = lua_gettop(ls);
        {
            wrap_ref.get(ls);
            luabind::detail::stack_pop pop{ ls, 1 };

            const auto* obj = static_cast<luabind::detail::object_rep*>(lua_touserdata(ls, -1));

            ImGui::PushID(obj->crep()->name());
            const bool open = ImGui::TreeNodeEx(this, flags, "%s * LUA", obj->crep()->name());
            if (ImGui::IsItemClicked())
                debugState.select(this);

            if (open)
            {
                lua_getuservalue(ls, -1);
                luabind::detail::stack_pop pop2{ ls, 1 };

                luabind::table members(luabind::from_stack(ls, -1));
                for (luabind::iterator it(members), end; it != end; ++it)
                {
                    auto proxy = *it;
                    proxy.push(ls);
                    luabind::detail::stack_pop pop3{ ls, 1 };

                    if (luabind::type(proxy) == LUA_TUSERDATA)
                    {
                        const auto ptr = luabind::detail::get_instance(ls, -1);
                        if (!ptr)
                            continue;

                        const auto inst = ptr->get_instance(luabind::detail::registered_class<CUIWindow>::id).first;
                        if (!inst)
                            continue;

                        ((CUIWindow*)inst)->FillDebugTree(debugState);
                    }
                }

                ImGui::TreePop();
            } // if (open)
            ImGui::PopID();
        }
        R_ASSERT(lua_gettop(ls) == prev);

        return true;
#else
        UNUSED(debugState);
        return false;
#endif
    }

    void FillDebugInfo() override
    {
        inherited::FillDebugInfo();

        const auto& wrap_ref = luabind::detail::wrap_access::ref(*this);

        lua_State* ls = wrap_ref.state();
        const int prev = lua_gettop(ls);
        {
            wrap_ref.get(ls);
            luabind::detail::stack_pop pop{ ls, 1 };
            const auto* obj = static_cast<luabind::detail::object_rep*>(lua_touserdata(ls, -1));

            if (ImGui::CollapsingHeader(obj->crep()->name()))
            {
                ImGui::BeginDisabled();

                lua_getuservalue(ls, -1);
                luabind::detail::stack_pop pop2{ ls, 1 };

                luabind::table members(luabind::from_stack(ls, -1));
                for (luabind::iterator it(members), end; it != end; ++it)
                {
                    auto proxy = *it;
                    proxy.push(ls);
                    luabind::detail::stack_pop pop3{ ls, 1 };

                    auto str = luabind::detail::stack_content_by_name(ls, prev);
                    cpcstr name = lua_tostring(ls, -2);
                    if (!name)
                        continue;

                    switch (luabind::type(proxy))
                    {
                    case LUA_TBOOLEAN:
                    {
                        bool boolean = lua_toboolean(ls, -1);
                        ImGui::Checkbox(name, &boolean);
                        break;
                    }
                    case LUA_TNUMBER:
                    {
                        lua_Number number = lua_tonumber(ls, -1);
                        ImGui::DragScalar(name, ImGuiDataType_Double, &number);
                        break;
                    }
                    case LUA_TSTRING:
                    {
                        cpcstr string = lua_tostring(ls, -1);
                        ImGui::Text("%s", string);
                        break;
                    }
                    } // switch (luabind::type(proxy))
                }
                ImGui::EndDisabled();
            }
        }
        R_ASSERT(lua_gettop(ls) == prev);
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
