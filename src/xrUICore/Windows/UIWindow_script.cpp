#include "pch.hpp"
#include "UIWindow.h"
#include "UIFrameWindow.h"
#include "UIFrameLineWnd.h"
#include "XML/UITextureMaster.h"
#include "ScrollView/UIScrollView.h"
#include "Hint/UIHint.h"
#include "Cursor/UICursor.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CFontManager& mngr() { return UI().Font(); }
// hud font
CGameFont* GetFontSmall() { return mngr().pFontStat; }
CGameFont* GetFontMedium() { return mngr().pFontMedium; }
CGameFont* GetFontDI() { return mngr().pFontDI; }
//шрифты для интерфейса
CGameFont* GetFontGraffiti19Russian() { return mngr().pFontGraffiti19Russian; }
CGameFont* GetFontGraffiti22Russian() { return mngr().pFontGraffiti22Russian; }
CGameFont* GetFontLetterica16Russian() { return mngr().pFontLetterica16Russian; }
CGameFont* GetFontLetterica18Russian() { return mngr().pFontLetterica18Russian; }
CGameFont* GetFontGraffiti32Russian() { return mngr().pFontGraffiti32Russian; }
CGameFont* GetFontGraffiti50Russian() { return mngr().pFontGraffiti50Russian; }
CGameFont* GetFontLetterica25() { return mngr().pFontLetterica25; }
int GetARGB(u16 a, u16 r, u16 g, u16 b) { return color_argb(a, r, g, b); }
const Fvector2 get_wnd_pos(CUIWindow* w) { return w->GetWndPos(); }

Fvector2 GetCursorPosition_script() { return GetUICursor().GetCursorPosition(); }

void SetCursorPosition_script(Fvector2& pos) { GetUICursor().SetUICursorPosition(pos); }

using namespace luabind;
using namespace luabind::policy;
Frect	get_texture_rect(LPCSTR icon_name)
{
    return CUITextureMaster::GetTextureRect(icon_name);
}
// clang-format off
SCRIPT_EXPORT(CUIWindow, (), {
    module(luaState)
    [
        def("GetARGB", &GetARGB), def("GetFontSmall", &GetFontSmall), def("GetFontMedium", &GetFontMedium),
        def("GetFontDI", &GetFontDI), def("GetFontGraffiti19Russian", &GetFontGraffiti19Russian),
        def("GetFontGraffiti22Russian", &GetFontGraffiti22Russian),
        def("GetFontLetterica16Russian", &GetFontLetterica16Russian),
        def("GetFontLetterica18Russian", &GetFontLetterica18Russian),
        def("GetFontGraffiti32Russian", &GetFontGraffiti32Russian),
        def("GetFontGraffiti50Russian", &GetFontGraffiti50Russian),
        def("GetFontLetterica25", &GetFontLetterica25),
        def("GetCursorPosition", &GetCursorPosition_script),
        def("SetCursorPosition", &SetCursorPosition_script),
        def("FitInRect", &fit_in_rect),

        class_<TEX_INFO>("TEX_INFO")
            .def("get_file_name", &TEX_INFO::get_file_name)
            .def("get_rect", &TEX_INFO::get_rect),

        def("GetTextureName", +[](pcstr iconName)
        {
            return CUITextureMaster::GetTextureFileName(iconName);
        }),

        def("GetTextureRect", +[](pcstr iconName)
        {
            return CUITextureMaster::GetTextureRect(iconName);
        }),

        def("GetTextureInfo", +[](pcstr name)
        {
            return CUITextureMaster::FindItem(name);
        }),

        def("GetTextureInfo", +[](pcstr name, pcstr defaultName)
        {
            return CUITextureMaster::FindItem(name, defaultName);
        }),

        class_<CUIWindow>("CUIWindow")
            .def(constructor<>())
            .def("AttachChild", &CUIWindow::AttachChild, adopt<2>())
            .def("DetachChild", &CUIWindow::DetachChild)
            .def("SetAutoDelete", &CUIWindow::SetAutoDelete)
            .def("IsAutoDelete", &CUIWindow::IsAutoDelete)

            .def("IsCursorOverWindow", &CUIWindow::CursorOverWindow)
            .def("FocusReceiveTime", &CUIWindow::FocusReceiveTime)
            .def("GetAbsoluteRect", &CUIWindow::GetAbsoluteRect)

            .def("SetWndRect", (void (CUIWindow::*)(Frect)) & CUIWindow::SetWndRect_script)
            .def("SetWndSize", (void (CUIWindow::*)(Fvector2)) & CUIWindow::SetWndSize_script)

            .def("GetWndPos", &get_wnd_pos)
            .def("SetWndPos", (void (CUIWindow::*)(Fvector2)) & CUIWindow::SetWndPos_script)

            .def("GetWidth", &CUIWindow::GetWidth)
            .def("SetWidth", &CUIWindow::SetWidth)

            .def("GetHeight", &CUIWindow::GetHeight)
            .def("SetHeight", &CUIWindow::SetHeight)

            .def("Enable", &CUIWindow::Enable)
            .def("IsEnabled", &CUIWindow::IsEnabled)

            .def("Show", &CUIWindow::Show)
            .def("IsShown", &CUIWindow::IsShown)

            .def("SetFont", &CUIWindow::SetFont)
            .def("GetFont", &CUIWindow::GetFont)

            .def("WindowName", &CUIWindow::WindowName_script)
            .def("SetWindowName", &CUIWindow::SetWindowName)
    ];
});

SCRIPT_EXPORT(CUIFrameWindow, (CUIWindow), {
    module(luaState)[class_<CUIFrameWindow, CUIWindow>("CUIFrameWindow")
                         .def(constructor<>())
                         .def("SetWidth", &CUIFrameWindow::SetWidth)
                         .def("SetHeight", &CUIFrameWindow::SetHeight)
                         .def("SetColor", &CUIFrameWindow::SetTextureColor)];
});

SCRIPT_EXPORT(CUIFrameLineWnd, (CUIWindow), {
    module(luaState)[class_<CUIFrameLineWnd, CUIWindow>("CUIFrameLineWnd")
                         .def(constructor<>())
                         .def("SetWidth", &CUIFrameLineWnd::SetWidth)
                         .def("SetHeight", &CUIFrameLineWnd::SetHeight)
                         .def("SetColor", &CUIFrameLineWnd::SetTextureColor)];
});

SCRIPT_EXPORT(UIHint, (CUIWindow), {
    module(luaState)
    [
        class_<UIHint, CUIWindow>("UIHint")
            .def(constructor<>())
            .def("SetWidth", &UIHint::SetWidth)
            .def("SetHeight", &UIHint::SetHeight)
            .def("SetHintText", &UIHint::set_text)
            .def("GetHintText", &UIHint::get_text)
    ];
});

SCRIPT_EXPORT(CUIScrollView, (CUIWindow), {
    module(luaState)
    [
        class_<CUIScrollView, CUIWindow>("CUIScrollView")
            .def(constructor<>())
            .def("AddWindow", &CUIScrollView::AddWindow)
            .def("RemoveWindow", &CUIScrollView::RemoveWindow)
            .def("Clear", &CUIScrollView::Clear)
            .def("ScrollToBegin", &CUIScrollView::ScrollToBegin)
            .def("ScrollToEnd", &CUIScrollView::ScrollToEnd)
            .def("GetMinScrollPos", &CUIScrollView::GetMinScrollPos)
            .def("GetMaxScrollPos", &CUIScrollView::GetMaxScrollPos)
            .def("GetCurrentScrollPos", &CUIScrollView::GetCurrentScrollPos)
            .def("SetFixedScrollBar", &CUIScrollView::SetFixedScrollBar)
            .def("SetScrollPos", &CUIScrollView::SetScrollPos)
    ];
});

SCRIPT_EXPORT(EnumUIMessages, (), {
    class EnumUIMessages
    {
    };
    module(luaState)
    [
        class_<EnumUIMessages>("ui_events")
        .enum_("events")
        [
                             // CUIWindow
                             value("WINDOW_LBUTTON_DOWN", int(WINDOW_LBUTTON_DOWN)),
                             value("WINDOW_RBUTTON_DOWN", int(WINDOW_RBUTTON_DOWN)),
                             value("WINDOW_LBUTTON_UP", int(WINDOW_LBUTTON_UP)),
                             value("WINDOW_RBUTTON_UP", int(WINDOW_RBUTTON_UP)),
                             value("WINDOW_MOUSE_MOVE", int(WINDOW_MOUSE_MOVE)),
                             value("WINDOW_LBUTTON_DB_CLICK", int(WINDOW_LBUTTON_DB_CLICK)),
                             value("WINDOW_KEY_PRESSED", int(WINDOW_KEY_PRESSED)),
                             value("WINDOW_KEY_RELEASED", int(WINDOW_KEY_RELEASED)),
                             value("WINDOW_KEYBOARD_CAPTURE_LOST", int(WINDOW_KEYBOARD_CAPTURE_LOST)),

                             // CUIButton
                             value("BUTTON_CLICKED", int(BUTTON_CLICKED)), value("BUTTON_DOWN", int(BUTTON_DOWN)),

                             // CUITabControl
                             value("TAB_CHANGED", int(TAB_CHANGED)),

                             // CUICheckButton
                             value("CHECK_BUTTON_SET", int(CHECK_BUTTON_SET)),
                             value("CHECK_BUTTON_RESET", int(CHECK_BUTTON_RESET)),

                             // CUIRadioButton
                             value("RADIOBUTTON_SET", int(RADIOBUTTON_SET)),

                             // CUIScrollBox
                             value("SCROLLBOX_MOVE", int(SCROLLBOX_MOVE)),

                             // CUIScrollBar
                             value("SCROLLBAR_VSCROLL", int(SCROLLBAR_VSCROLL)),
                             value("SCROLLBAR_HSCROLL", int(SCROLLBAR_HSCROLL)),

                             // CUIListWnd
                             value("LIST_ITEM_CLICKED", int(LIST_ITEM_CLICKED)),
                             value("LIST_ITEM_SELECT", int(LIST_ITEM_SELECT)),

                             // UIPropertiesBox
                             value("PROPERTY_CLICKED", int(PROPERTY_CLICKED)),

                             // CUIMessageBox
                             value("MESSAGE_BOX_OK_CLICKED", int(MESSAGE_BOX_OK_CLICKED)),
                             value("MESSAGE_BOX_YES_CLICKED", int(MESSAGE_BOX_YES_CLICKED)),
                             value("MESSAGE_BOX_NO_CLICKED", int(MESSAGE_BOX_NO_CLICKED)),
                             value("MESSAGE_BOX_CANCEL_CLICKED", int(MESSAGE_BOX_CANCEL_CLICKED)),
                             value("MESSAGE_BOX_COPY_CLICKED", int(MESSAGE_BOX_COPY_CLICKED)),
                             value("MESSAGE_BOX_QUIT_GAME_CLICKED", int(MESSAGE_BOX_QUIT_GAME_CLICKED)),
                             value("MESSAGE_BOX_QUIT_WIN_CLICKED", int(MESSAGE_BOX_QUIT_WIN_CLICKED)),

                             value("EDIT_TEXT_COMMIT", int(EDIT_TEXT_COMMIT)),
                             // CMainMenu
                             value("MAIN_MENU_RELOADED", int(MAIN_MENU_RELOADED))
                         ]
    ];
});
// clang-format on
