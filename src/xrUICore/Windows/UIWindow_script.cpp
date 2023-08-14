#include "pch.hpp"

#include "xrCore/XML/XMLDocument.hpp"

#include "UIWindow.h"
#include "UIFrameWindow.h"
#include "UIFrameLineWnd.h"
#include "XML/UITextureMaster.h"
#include "ScrollView/UIScrollView.h"
#include "Hint/UIHint.h"
#include "Cursor/UICursor.h"
#include "ui_styles.h"

#include "xrScriptEngine/ScriptExporter.hpp"

// clang-format off
SCRIPT_EXPORT(UICore, (),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        def("GetARGB", +[](u16 a, u16 r, u16 g, u16 b) { return color_argb(a, r, g, b); }),

        // hud font
        def("GetFontSmall",  +[] { return UI().Font().pFontStat; }),
        def("GetFontMedium", +[] { return UI().Font().pFontMedium; }),
        def("GetFontDI",     +[] { return UI().Font().pFontDI; }),

        //шрифты для интерфейса
        def("GetFontGraffiti19Russian",  +[] { return UI().Font().pFontGraffiti19Russian; }),
        def("GetFontGraffiti22Russian",  +[] { return UI().Font().pFontGraffiti22Russian; }),
        def("GetFontLetterica16Russian", +[] { return UI().Font().pFontLetterica16Russian; }),
        def("GetFontLetterica18Russian", +[] { return UI().Font().pFontLetterica18Russian; }),
        def("GetFontGraffiti32Russian",  +[] { return UI().Font().pFontGraffiti32Russian; }),
        def("GetFontGraffiti50Russian",  +[] { return UI().Font().pFontGraffiti50Russian; }),
        def("GetFontLetterica25",        +[] { return UI().Font().pFontLetterica25; }),

        def("GetCursorPosition", +[] { return GetUICursor().GetCursorPosition(); }),
        def("SetCursorPosition", +[](Fvector2& pos) { GetUICursor().SetUICursorPosition(pos); }),

        def("FitInRect", &fit_in_rect)
    ];
});

SCRIPT_EXPORT(UIStyleManager, (),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        def("GetDefaultUIPath",              +[] { return UI_PATH_DEFAULT; }),
        def("GetDefaultUIPathWithDelimiter", +[] { return UI_PATH_DEFAULT_WITH_DELIMITER; }),
        def("GetUIPath",                     +[] { return UI_PATH; }),
        def("GetUIPathWithDelimiter",        +[] { return UI_PATH_WITH_DELIMITER; }),

        class_<UIStyleManager>("UIStyleManager")
            .def("GetAllStyles", &UIStyleManager::GetToken, return_stl_iterator())
            .def("DefaultStyleIsSet", &UIStyleManager::DefaultStyleIsSet)
            .def("GetCurrentStyleId", &UIStyleManager::GetCurrentStyleId)
            .def("GetCurrentStyleName", &UIStyleManager::GetCurrentStyleName)
            .def("SetStyle", &UIStyleManager::SetStyle)
            .def("SetupStyle", &UIStyleManager::SetupStyle)
            .def("ResetUI", &UIStyleManager::Reset),

        def("GetUIStyleManager", +[] { return UIStyles; })
    ];
});

SCRIPT_EXPORT(CUITextureMaster, (),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
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

        def("GetTextureInfo", +[](pcstr name, TEX_INFO& outValue)
        {
            return CUITextureMaster::FindItem(name, outValue);
        }),

        def("GetTextureInfo", +[](pcstr name, pcstr defaultName, TEX_INFO& outValue)
        {
            return CUITextureMaster::FindItem(name, defaultName, outValue);
        })
    ];
});

// We don't change game assets.
// This class allowes original game scripts to not specify the window name.
class CUIWindowScript final : public CUIWindow
{
public:
    CUIWindowScript() : CUIWindow(CUIWindowScript::GetDebugType()) {}
    pcstr GetDebugType() override { return "CUIWindowScript"; }
};

SCRIPT_EXPORT(CUIWindow, (),
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<CUIWindow>("CUIWindowBase")
            .def(constructor<pcstr>())
            .def("AttachChild", &CUIWindow::AttachChild, adopt<2>())
            .def("DetachChild", &CUIWindow::DetachChild)
            .def("SetAutoDelete", &CUIWindow::SetAutoDelete)
            .def("IsAutoDelete", &CUIWindow::IsAutoDelete)

            .def("IsCursorOverWindow", &CUIWindow::CursorOverWindow)
            .def("FocusReceiveTime", &CUIWindow::FocusReceiveTime)
            .def("GetAbsoluteRect", &CUIWindow::GetAbsoluteRect)

            .def("Init", +[](CUIWindow* self, float x, float y, float width, float height)
            {
                const Frect rect { x, y, width, height };
                self->SetWndRect(rect);
            })
            .def("Init", (void (CUIWindow::*)(Frect))& CUIWindow::SetWndRect_script)

            .def("SetWndRect", (void (CUIWindow::*)(Frect)) & CUIWindow::SetWndRect_script)
            .def("SetWndRect", +[](CUIWindow* self, float x, float y, float width, float height)
            {
                const Frect rect { x, y, width, height };
                self->SetWndRect(rect);
            })

            .def("SetWndSize", (void (CUIWindow::*)(Fvector2)) & CUIWindow::SetWndSize_script)

            .def("GetWndPos", +[](CUIWindow* w) -> Fvector2 { return w->GetWndPos(); })
            .def("SetWndPos", (void (CUIWindow::*)(Fvector2)) & CUIWindow::SetWndPos_script)

            .def("SetWndPos", +[](CUIWindow* self, float x, float y)
            {
                const Fvector2 pos { x, y };
                self->SetWndPos(pos);
            })
            .def("SetWndSize", +[](CUIWindow* self, float width, float height)
            {
                const Fvector2 size { width, height };
                self->SetWndSize(size);
            })

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

            .def("WindowName", +[](CUIWindow* self) -> pcstr { return self->WindowName().c_str(); })
            .def("SetWindowName", &CUIWindow::SetWindowName),

        class_<CUIWindowScript, CUIWindow>("CUIWindow")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CUIFrameWindow, (CUIWindow),
{
    using namespace luabind;
    using namespace luabind::policy;

    // We don't change game assets.
    // This class allowes original game scripts to not specify the window name.
    class CUIFrameWindowScript final : public CUIFrameWindow
    {
    public:
        CUIFrameWindowScript() : CUIFrameWindow(CUIFrameWindowScript::GetDebugType()) {}
        pcstr GetDebugType() override { return "CUIFrameWindowScript"; }
    };

    module(luaState)
    [
        class_<CUIFrameWindow, CUIWindow>("CUIFrameWindowBase")
            .def(constructor<pcstr>())
            .def("SetWidth", &CUIFrameWindow::SetWidth)
            .def("SetHeight", &CUIFrameWindow::SetHeight)
            .def("SetColor", &CUIFrameWindow::SetTextureColor)
            .def("Init", +[](CUIFrameWindow* self, pcstr texture, float x, float y, float width, float height)
            {
                const Frect rect { x, y, width, height };
                self->SetWndRect(rect);
                self->InitTexture(texture);
            }),
        class_<CUIFrameWindowScript, CUIFrameWindow>("CUIFrameWindow")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(CUIFrameLineWnd, (CUIWindow),
{
    using namespace luabind;
    using namespace luabind::policy;

    // We don't change game assets.
    // This class allowes original game scripts to not specify the window name.
    class CUIFrameLineWndScript final : public CUIFrameLineWnd
    {
    public:
        CUIFrameLineWndScript() : CUIFrameLineWnd(CUIFrameLineWndScript::GetDebugType()) {}
        pcstr GetDebugType() override { return "CUIFrameLineWndScript"; }
    };

    module(luaState)
    [
        class_<CUIFrameLineWnd, CUIWindow>("CUIFrameLineWndBase")
            .def(constructor<pcstr>())
            .def("SetWidth", &CUIFrameLineWnd::SetWidth)
            .def("SetHeight", &CUIFrameLineWnd::SetHeight)
            .def("SetColor", &CUIFrameLineWnd::SetTextureColor)
            .def("Init", +[](CUIFrameLineWnd* self, cpcstr texture, float x, float y, float width, float height, bool horizontal)
            {
                const Fvector2 pos { x, y };
                const Fvector2 size { width, height };
                self->InitFrameLineWnd(texture, pos, size, horizontal);
            }),
        class_<CUIFrameLineWndScript, CUIFrameLineWnd>("CUIFrameLineWnd")
            .def(constructor<>())
    ];
});

SCRIPT_EXPORT(UIHint, (CUIWindow),
{
    using namespace luabind;
    using namespace luabind::policy;

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

SCRIPT_EXPORT(CUIScrollView, (CUIWindow),
{
    using namespace luabind;
    using namespace luabind::policy;

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

SCRIPT_EXPORT(EnumUIMessages, (),
{
    using namespace luabind;
    using namespace luabind::policy;

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
            value("BUTTON_CLICKED", int(BUTTON_CLICKED)),
            value("BUTTON_DOWN", int(BUTTON_DOWN)),

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
            value("LIST_ITEM_UNSELECT", int(LIST_ITEM_UNSELECT)),

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
