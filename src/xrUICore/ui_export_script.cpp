#include "pch.hpp"

#include "xrCore/XML/XMLDocument.hpp"

#include "ui_styles.h"
#include "ui_focus.h"
#include "Cursor/UICursor.h"
#include "XML/UITextureMaster.h"

#include "Windows/UIWindow.h"
#include "Windows/UIFrameWindow.h"
#include "Windows/UIFrameLineWnd.h"
#include "ScrollView/UIScrollView.h"
#include "Hint/UIHint.h"

#include "ListBox/UIListBox.h"
#include "ListBox/UIListBoxItem.h"
#include "ListBox/UIListBoxItemMsgChain.h"

#include "ListWnd/UIListWnd.h"
#include "ListWnd/UIListItemEx.h"

#include "Static/UIStatic.h"

#include "Buttons/UIButton.h"
#include "Buttons/UI3tButton.h"
#include "Buttons/UICheckButton.h"
#include "SpinBox/UISpinNum.h"
#include "SpinBox/UISpinText.h"

#include "TrackBar/UITrackBar.h"

#include "TabControl/UITabControl.h"
#include "TabControl/UITabButton.h"

#include "ComboBox/UIComboBox.h"

#include "ProgressBar/UIProgressBar.h"

#include "PropertiesBox/UIPropertiesBox.h"

#include "EditBox/UIEditBox.h"

#include "MessageBox/UIMessageBox.h"

#include "xrScriptEngine/script_space.hpp"

void UIStyleManager::script_register(lua_State* luaState)
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
}

void CUIFocusSystem::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<FocusDirection>("FocusDirection")
            .enum_("direction")
            [
                value("Same",       (int)FocusDirection::Same),
                value("Up",         (int)FocusDirection::Up),
                value("Down",       (int)FocusDirection::Down),
                value("Left",       (int)FocusDirection::Left),
                value("Right",      (int)FocusDirection::Right),
                value("UpperLeft",  (int)FocusDirection::UpperLeft),
                value("UpperRight", (int)FocusDirection::UpperRight),
                value("LowerLeft",  (int)FocusDirection::LowerLeft),
                value("LowerRight", (int)FocusDirection::LowerRight)
            ],

        class_<CUIFocusSystem>("CUIFocusSystem")
            .def("RegisterFocusable", &CUIFocusSystem::RegisterFocusable)
            .def("UnregisterFocusable", &CUIFocusSystem::UnregisterFocusable)
            .def("IsRegistered", &CUIFocusSystem::IsRegistered)
            .def("IsValuable", &CUIFocusSystem::IsValuable)
            .def("IsNonValuable", &CUIFocusSystem::IsNonValuable)
            .def("Update", &CUIFocusSystem::Update)
            .def("LockToWindow", &CUIFocusSystem::LockToWindow)
            .def("Unlock", &CUIFocusSystem::Unlock)
            .def("GetLocker", &CUIFocusSystem::GetLocker)
            .def("GetFocused", &CUIFocusSystem::GetFocused)
            .def("SetFocused", &CUIFocusSystem::SetFocused)
            .def("FindClosestFocusable", &CUIFocusSystem::FindClosestFocusable),

        def("GetUIFocusSystem", +[] { return UI().Focus(); })
    ];
}

void CUITextureMaster::script_register(lua_State* luaState)
{
    using namespace luabind;

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
}

void CUIWindow::script_register(lua_State* luaState)
{
    // We don't change game assets.
    // This class allowes original game scripts to not specify the window name.
    class CUIWindowScript final : public CUIWindow
    {
    public:
        CUIWindowScript() : CUIWindow(CUIWindowScript::GetDebugType()) {}

        pcstr GetDebugType() override
        {
            return "CUIWindowScript";
        }
    };

    class EnumUIMessages
    {
    };

    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        def("GetARGB", +[](u16 a, u16 r, u16 g, u16 b) { return color_argb(a, r, g, b); }),

        // HUD fonts
        def("GetFontSmall",  +[] { return UI().Font().pFontStat; }),
        def("GetFontMedium", +[] { return UI().Font().pFontMedium; }),
        def("GetFontDI",     +[] { return UI().Font().pFontDI; }),

        // UI fonts
        def("GetFontGraffiti19Russian",  +[] { return UI().Font().pFontGraffiti19Russian; }),
        def("GetFontGraffiti22Russian",  +[] { return UI().Font().pFontGraffiti22Russian; }),
        def("GetFontLetterica16Russian", +[] { return UI().Font().pFontLetterica16Russian; }),
        def("GetFontLetterica18Russian", +[] { return UI().Font().pFontLetterica18Russian; }),
        def("GetFontGraffiti32Russian",  +[] { return UI().Font().pFontGraffiti32Russian; }),
        def("GetFontGraffiti50Russian",  +[] { return UI().Font().pFontGraffiti50Russian; }),
        def("GetFontLetterica25",        +[] { return UI().Font().pFontLetterica25; }),

        def("GetCursorPosition", +[] { return GetUICursor().GetCursorPosition(); }),
        def("SetCursorPosition", +[](const Fvector2& pos) { GetUICursor().SetUICursorPosition(pos); }),

        def("FitInRect", &fit_in_rect),

        class_<CUIWindow>("CUIWindowBase")
            .def(constructor<pcstr>())
            .def("AttachChild", &CUIWindow::AttachChild, adopt<2>())
            .def("DetachChild", &CUIWindow::DetachChild)
            .def("SetAutoDelete", &CUIWindow::SetAutoDelete)
            .def("IsAutoDelete", &CUIWindow::IsAutoDelete)

            .def("IsCursorOverWindow", &CUIWindow::CursorOverWindow)
            .def("IsUsingCursorRightNow", &CUIWindow::IsUsingCursorRightNow)
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
            .def(constructor<>()),

        class_<UIHint, CUIWindow>("UIHint")
            .def(constructor<>())
            .def("SetWidth", &UIHint::SetWidth)
            .def("SetHeight", &UIHint::SetHeight)
            .def("SetHintText", &UIHint::set_text)
            .def("GetHintText", &UIHint::get_text),

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
            value("WINDOW_MOUSE_CAPTURE_LOST", int(WINDOW_MOUSE_CAPTURE_LOST)),
            value("WINDOW_KEYBOARD_CAPTURE_LOST", int(WINDOW_KEYBOARD_CAPTURE_LOST)),

            // Legacy SOC/CS events
            value("STATIC_FOCUS_RECEIVED", int(WINDOW_FOCUS_RECEIVED)),
            value("STATIC_FOCUS_LOST", int(WINDOW_FOCUS_LOST)),

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
}

void CUIFrameWindow::script_register(lua_State* luaState)
{
    // We don't change game assets.
    // This class allowes original game scripts to not specify the window name.
    class CUIFrameWindowScript final : public CUIFrameWindow
    {
    public:
        CUIFrameWindowScript() : CUIFrameWindow(CUIFrameWindowScript::GetDebugType()) {}

        pcstr GetDebugType() override
        {
            return "CUIFrameWindowScript";
        }
    };

    using namespace luabind;

    module(luaState)
    [
        class_<CUIFrameWindow, CUIWindow>("CUIFrameWindowBase")
            .def(constructor<pcstr>())
            .def("SetWidth", &CUIFrameWindow::SetWidth)
            .def("SetHeight", &CUIFrameWindow::SetHeight)
            .def("SetColor", &CUIFrameWindow::SetTextureColor)
            .def("Init", +[](CUIFrameWindow* self, pcstr texture, float x, float y, float width, float height)
            {
                self->SetWndRect({ x, y, width, height });
                self->InitTexture(texture);
            }),

        class_<CUIFrameWindowScript, CUIFrameWindow>("CUIFrameWindow")
            .def(constructor<>())
    ];
}

void CUIFrameLineWnd::script_register(lua_State* luaState)
{
    // We don't change game assets.
    // This class allowes original game scripts to not specify the window name.
    class CUIFrameLineWndScript final : public CUIFrameLineWnd
    {
    public:
        CUIFrameLineWndScript() : CUIFrameLineWnd(CUIFrameLineWndScript::GetDebugType()) {}

        pcstr GetDebugType() override
        {
            return "CUIFrameLineWndScript";
        }
    };

    using namespace luabind;

    module(luaState)
    [
        class_<CUIFrameLineWnd, CUIWindow>("CUIFrameLineWndBase")
            .def(constructor<pcstr>())
            .def("SetWidth", &CUIFrameLineWnd::SetWidth)
            .def("SetHeight", &CUIFrameLineWnd::SetHeight)
            .def("SetColor", &CUIFrameLineWnd::SetTextureColor)
            .def("Init", +[](CUIFrameLineWnd* self, cpcstr texture, float x, float y, float width, float height, bool horizontal)
            {
                self->SetWndPos({ x, y });
                self->SetWndSize({ width, height });
                self->SetHorizontal(horizontal);
                self->InitTexture(texture);
            }),

        class_<CUIFrameLineWndScript, CUIFrameLineWnd>("CUIFrameLineWnd")
            .def(constructor<>())
    ];
}

void CUIScrollView::script_register(lua_State* luaState)
{
    using namespace luabind;

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
}

void CUIListBox::script_register(lua_State* luaState)
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<CUIListBox, CUIScrollView>("CUIListBox")
            .def(constructor<>())
            .def("ShowSelectedItem", &CUIListBox::Show)
            .def("RemoveAll", &CUIListBox::Clear)
            .def("GetSize", &CUIListBox::GetSize)
            .def("GetSelectedItem", &CUIListBox::GetSelectedItem)
            .def("GetSelectedIndex", &CUIListBox::GetSelectedIDX)
            .def("SetSelectedIndex", &CUIListBox::SetSelectedIDX)
            .def("SetItemHeight", &CUIListBox::SetItemHeight)
            .def("GetItemHeight", &CUIListBox::GetItemHeight)
            .def("GetItemByIndex", &CUIListBox::GetItemByIDX)
            .def("GetItem", &CUIListBox::GetItem)
            .def("RemoveItem", &CUIListBox::RemoveWindow)
            .def("AddTextItem", &CUIListBox::AddTextItem)
            .def("AddExistingItem", &CUIListBox::AddExistingItem, adopt<2>())
    ];
}

void CUIListBoxItem::script_register(lua_State* luaState)
{
    struct CUIListBoxItemWrapper : public CUIListBoxItem, public luabind::wrap_base
    {
        CUIListBoxItemWrapper(float h) : CUIListBoxItem(h) {}
        pcstr GetDebugType() override { return "CUIListBoxItemScript"; }
    };

    struct CUIListBoxItemMsgChainWrapper : public CUIListBoxItemMsgChain, public luabind::wrap_base
    {
        CUIListBoxItemMsgChainWrapper(float h) : CUIListBoxItemMsgChain(h) {}
        pcstr GetDebugType() override { return "CUIListBoxItemMsgChainScript"; }
    };

    using namespace luabind;

    module(luaState)
    [
        class_<CUIListBoxItem, CUIFrameLineWnd, default_holder, CUIListBoxItemWrapper>("CUIListBoxItem")
            .def(constructor<float>())
            .def("GetTextItem", &CUIListBoxItem::GetTextItem)
            .def("AddTextField", &CUIListBoxItem::AddTextField)
            .def("AddIconField", &CUIListBoxItem::AddIconField)
            .def("SetTextColor", &CUIListBoxItem::SetTextColor),

        class_<CUIListBoxItemMsgChain, CUIListBoxItem, default_holder, CUIListBoxItemMsgChainWrapper>("CUIListBoxItemMsgChain")
            .def(constructor<float>())
    ];
}

void CUIListWnd::script_register(lua_State* luaState)
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<CUIListWnd, CUIWindow>("CUIListWnd")
            .def(constructor<>())
            //.def("AddText", &CUIListWnd::AddText_script)
            .def("AddItem", +[](CUIListWnd* self, CUIListItem* item)
            {
                return self->AddItem(item, -1);
            }, adopt<2>())
            .def("RemoveItem", &CUIListWnd::RemoveItem)
            .def("RemoveAll", &CUIListWnd::RemoveAll)
            .def("EnableScrollBar", &CUIListWnd::EnableScrollBar)
            .def("IsScrollBarEnabled", &CUIListWnd::IsScrollBarEnabled)
            .def("ScrollToBegin", &CUIListWnd::ScrollToBegin)
            .def("ScrollToEnd", &CUIListWnd::ScrollToEnd)
            .def("SetItemHeight", &CUIListWnd::SetItemHeight)
            .def("GetItem", &CUIListWnd::GetItem)
            .def("GetItemPos", &CUIListWnd::GetItemPos)
            .def("GetSize", &CUIListWnd::GetItemsCount)
            .def("ScrollToBegin", &CUIListWnd::ScrollToBegin)
            .def("ScrollToEnd", &CUIListWnd::ScrollToEnd)
            .def("ScrollToPos", +[](CUIListWnd* self, int position)
            {
                self->ScrollToPos(position, 0.0f);
            })
            .def("ScrollToPos", &CUIListWnd::ScrollToPos)
            .def("SetWidth", &CUIListWnd::SetWidth)
            .def("SetTextColor", &CUIListWnd::SetTextColor)
            .def("ActivateList", &CUIListWnd::ActivateList)
            .def("IsListActive", &CUIListWnd::IsListActive)
            .def("SetVertFlip", &CUIListWnd::SetVertFlip)
            .def("GetVertFlip", &CUIListWnd::GetVertFlip)
            .def("SetFocusedItem", &CUIListWnd::SetFocusedItem)
            .def("GetFocusedItem", &CUIListWnd::GetFocusedItem)
            .def("ShowSelectedItem", &CUIListWnd::ShowSelectedItem)

            .def("GetSelectedItem", &CUIListWnd::GetSelectedItem)
            .def("ResetFocusCapture", &CUIListWnd::ResetFocusCapture)
    ];
}

void CUIListItem::script_register(lua_State* luaState)
{
    struct CUIListItemWrapper final : public CUIListItem, public luabind::wrap_base
    {
        pcstr GetDebugType() override { return "CUIListItemScript"; }
    };

    struct CUIListItemExWrapper final : public CUIListItemEx, public luabind::wrap_base
    {
        pcstr GetDebugType() override { return "CUIListItemExScript"; }
    };

    using namespace luabind;

    module(luaState)
    [
        class_<CUIListItem, CUIButton, default_holder, CUIListItemWrapper>("CUIListItem")
            .def(constructor<>()),
        class_<CUIListItemEx, CUIListItem, default_holder, CUIListItemExWrapper>("CUIListItemEx")
            .def(constructor<>())
            .def("SetSelectionColor", &CUIListItemEx::SetSelectionColor)
    ];
}

void CUIStatic::script_register(lua_State* luaState)
{
    // We don't change game assets.
    // This class allowes original game scripts to not specify the window name.
    class CUIStaticScript final : public CUIStatic
    {
    public:
        CUIStaticScript() : CUIStatic("CUIStaticScript") {}
        pcstr GetDebugType() override { return "CUIStaticScript"; }
    };

    using namespace luabind;

    module(luaState)
    [
        class_<CUILines>("CUILines")
            .def("SetFont", &CUILines::SetFont)
            .def("SetText", &CUILines::SetText)
            .def("SetTextST", &CUILines::SetTextST)
            .def("GetText", &CUILines::GetText)
            .def("SetElipsis", &CUILines::SetEllipsis)
            .def("SetTextColor", &CUILines::SetTextColor),

        class_<CUIStatic, CUIWindow>("CUIStaticBase")
            .def(constructor<pcstr>())

            .def("TextControl", &CUIStatic::TextItemControl)

            .def("SetText",   &CUIStatic::SetText)
            .def("SetTextST", &CUIStatic::SetTextST)

            .def("GetText", &CUIStatic::GetText)

            .def("SetTextOffset", &CUIStatic::SetTextOffset)
            .def("SetTextX", +[](CUIStatic* self, float x) { self->TextItemControl()->m_TextOffset.x = x; })
            .def("SetTextY", +[](CUIStatic* self, float y) { self->TextItemControl()->m_TextOffset.y = y; })
            .def("GetTextX", +[](CUIStatic* self) { return self->TextItemControl()->m_TextOffset.x; })
            .def("GetTextY", +[](CUIStatic* self) { return self->TextItemControl()->m_TextOffset.y; })

            .def("SetColor", &CUIStatic::SetTextureColor)
            .def("GetColor", &CUIStatic::GetTextureColor)

            .def("SetFont", &CUIStatic::SetFont)
            .def("GetFont", &CUIStatic::GetFont)

            .def("SetTextColor", +[](CUIStatic* self, int a, int r, int g, int b)
            {
                self->SetTextColor(color_argb(a, r, g, b));
            })
            .def("GetTextColor", &CUIStatic::GetTextColor)
            .def("SetTextComplexMode", &CUIStatic::SetTextComplexMode)
            .def("SetTextAlignment", &CUIStatic::SetTextAlignment)
            .def("SetVTextAlignment", &CUIStatic::SetVTextAlignment)

            .def("AdjustHeightToText", &CUIStatic::AdjustHeightToText)
            .def("AdjustWidthToText", &CUIStatic::AdjustWidthToText)

            .def("Init", +[](CUIStatic* self, float x, float y, float width, float height)
            {
                self->SetWndRect({ x, y, width, height });
            })
            .def("Init", +[](CUIStatic* self, cpcstr texture, float x, float y, float width, float height)
            {
                self->SetWndRect({ x, y, width, height });
                self->InitTexture(texture);
            })

            .def("InitTexture", &CUIStatic::InitTexture)
            .def("InitTexture", +[](CUIStatic* self, pcstr texture) { self->InitTexture(texture); })
            .def("InitTextureEx", &CUIStatic::InitTextureEx)
            .def("InitTextureEx", +[](CUIStatic* self, pcstr texture, pcstr shader) { self->InitTextureEx(texture, shader); })

            .def("SetTextureColor", &CUIStatic::SetTextureColor)
            .def("GetTextureColor", &CUIStatic::GetTextureColor)

            .def("SetTextureOffset", &CUIStatic::SetTextureOffset)

            .def("SetTextureRect", &CUIStatic::SetTextureRect_script)
            .def("GetTextureRect", &CUIStatic::GetTextureRect_script)

            .def("SetOriginalRect", &CUIStatic::SetTextureRect_script)
            .def("GetOriginalRect", &CUIStatic::GetTextureRect_script)

            .def("SetStretchTexture", &CUIStatic::SetStretchTexture)
            .def("GetStretchTexture", &CUIStatic::GetStretchTexture)

            .def("SetTextAlign", +[](CUIStatic* self, u32 align)
            {
                self->TextItemControl()->SetTextAlignment(static_cast<ETextAlignment>(align));
                self->TextItemControl()->GetFont()->SetAligment(static_cast<CGameFont::EAligment>(align));
            })
            .def("GetTextAlign", +[](CUIStatic* self) -> u32
            {
                return self->TextItemControl()->GetTextAlignment();
            })

            .def("SetHeading", &CUIStatic::SetHeading)
            .def("GetHeading", &CUIStatic::GetHeading)

            //.def("ClipperOn", &CUIStatic::ClipperOn)
            //.def("ClipperOff", (void(CUIStatic::*)(void))&CUIStatic::ClipperOff)

            //.def("GetClipperState", &CUIStatic::GetClipperState)

            .def("SetEllipsis", &CUIStatic::SetEllipsis)
            .def("SetElipsis",  &CUIStatic::SetEllipsis)
            .def("SetElipsis", +[](CUIStatic* self, int mode, int indent)
            {
                self->SetEllipsis(mode != 0);
            }),

        class_<CUIStaticScript, CUIStatic>("CUIStatic")
            .def(constructor<>())
    ];
}

void CUIButton::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIButton, CUIStatic>("CUIButton")
            .def("Init", +[](CUIButton* self, float x, float y, float width, float height)
            {
                self->SetWndRect({ x, y, width, height });
            })
            .def("Init", +[](CUIButton* self, cpcstr texture, float x, float y, float width, float height)
            {
                self->SetWndRect({ x, y, width, height });
                self->InitTexture(texture);
            })
            .def(constructor<>()),

        class_<CUI3tButton, CUIButton>("CUI3tButton")
            .def(constructor<>()),

        class_<CUICheckButton, CUI3tButton>("CUICheckButton")
            .def(constructor<>())
            .def("GetCheck", &CUICheckButton::GetCheck)
            .def("SetCheck", &CUICheckButton::SetCheck)
            .def("SetDependControl", &CUICheckButton::SetDependControl),

        class_<CUITabButton, CUIButton>("CUITabButton")
            .def(constructor<>()),

        class_<CUICustomSpin, CUIWindow>("CUICustomSpin")
            .def("Init", +[](CUICustomSpin* self, float x, float y, float width, float height)
            {
                const Fvector2 pos { x, y };
                const Fvector2 size { width, height };

                self->InitSpin(pos, size);
            })
            .def("GetText", &CUICustomSpin::GetText),

        class_<CUISpinNum, CUICustomSpin>("CUISpinNum")
            .def(constructor<>()),

        class_<CUISpinFlt, CUICustomSpin>("CUISpinFlt")
            .def(constructor<>()),

        class_<CUISpinText, CUICustomSpin>("CUISpinText")
            .def(constructor<>()),

        class_<CUITrackBar, CUIWindow>("CUITrackBar")
            .def(constructor<>())
            .def("GetCheck", &CUITrackBar::GetCheck)
            .def("SetCheck", &CUITrackBar::SetCheck)
            .def("GetIValue", &CUITrackBar::GetIValue)
            .def("GetFValue", &CUITrackBar::GetFValue)
            .def("SetOptIBounds", &CUITrackBar::SetOptIBounds)
            .def("SetOptFBounds", &CUITrackBar::SetOptFBounds)
            .def("SetCurrentValue", &CUITrackBar::SetCurrentOptValue)
    ];
}

void CUITabControl::script_register(lua_State* luaState)
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<CUITabControl, CUIWindow>("CUITabControl")
            .def(constructor<>())
            .def("AddItem", (bool (CUITabControl::*)(CUITabButton*))(&CUITabControl::AddItem), adopt<2>())
            .def("AddItem", (bool (CUITabControl::*)(pcstr, pcstr, Fvector2, Fvector2)) &CUITabControl::AddItem)
            .def("AddItem", +[](CUITabControl* self, pcstr pItemName, pcstr pTexName, float x, float y, float width, float height)
            {
                self->AddItem(pItemName, pTexName, { x, y }, { width, height });
            })
            .def("RemoveItem", &CUITabControl::RemoveItemByIndex)
            .def("RemoveItemById", &CUITabControl::RemoveItemById_script)
            .def("RemoveAll", &CUITabControl::RemoveAll)
            .def("GetActiveId", &CUITabControl::GetActiveId_script)
            .def("GetActiveIndex", &CUITabControl::GetActiveIndex)
            .def("GetTabsCount", &CUITabControl::GetTabsCount)
            .def("SetActiveTab", &CUITabControl::SetActiveTab_script)
            .def("SetNewActiveTab", &CUITabControl::SetActiveTabByIndex)
            .def("GetButtonById", &CUITabControl::GetButtonById_script)
            .def("GetButtonByIndex", &CUITabControl::GetButtonByIndex)
    ];
}

void CUIComboBox::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIComboBox, CUIWindow>("CUIComboBox")
            .def(constructor<>())
            .def("Init", +[](CUIComboBox* self, float x, float y, float width)
            {
                self->InitComboBox({ x, y }, width);
            })
            .def("Init", +[](CUIComboBox* self, float x, float y, float width, float /*height*/)
            {
                self->InitComboBox({ x, y }, width);
            })
            .def("SetVertScroll", &CUIComboBox::SetVertScroll)
            .def("SetListLength", &CUIComboBox::SetListLength)
            .def("CurrentID", &CUIComboBox::CurrentID)
            .def("SetCurrentID", &CUIComboBox::SetItemIDX)
            .def("disable_id", &CUIComboBox::disable_id)
            .def("enable_id", &CUIComboBox::enable_id)
            .def("AddItem", &CUIComboBox::AddItem_)
            .def("GetText", &CUIComboBox::GetText)
            .def("GetTextOf", &CUIComboBox::GetTextOf)
            .def("SetText", &CUIComboBox::SetText)
            .def("ClearList", &CUIComboBox::ClearList)
            .def("SetCurrentValue", &CUIComboBox::SetCurrentOptValue)
            .def("SetCurrentOptValue", &CUIComboBox::SetCurrentOptValue)
            .def("SetCurrentIdx", &CUIComboBox::SetSelectedIDX)
            .def("GetCurrentIdx", &CUIComboBox::GetSelectedIDX)
    ];
}

void CUIProgressBar::script_register(lua_State* luaState)
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
}

void CUIPropertiesBox::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIPropertiesBox, CUIFrameWindow>("CUIPropertiesBox")
            .def(constructor<>())
            .def("RemoveItem", &CUIPropertiesBox::RemoveItemByTAG)
            .def("RemoveAll", &CUIPropertiesBox::RemoveAll)
            .def("Show", (void (CUIPropertiesBox::*)(int, int)) &CUIPropertiesBox::Show)
            .def("Hide", &CUIPropertiesBox::Hide)
            .def("GetSelectedItem", &CUIPropertiesBox::GetClickedItem)
            .def("AutoUpdateSize", &CUIPropertiesBox::AutoUpdateSize)
            .def("AddItem", &CUIPropertiesBox::AddItem_script)
            .def("InitPropertiesBox", &CUIPropertiesBox::InitPropertiesBox)
    ];
}

void CUIEditBox::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUICustomEdit, CUIWindow>("CUICustomEdit")
            .def("SetText", &CUICustomEdit::SetText)
            .def("GetText", &CUICustomEdit::GetText)
            .def("CaptureFocus", &CUICustomEdit::CaptureFocus)
            .def("SetNextFocusCapturer", &CUICustomEdit::SetNextFocusCapturer),

        class_<CUIEditBox, CUICustomEdit>("CUIEditBox")
            .def(constructor<>())
            .def("InitTexture", &CUIEditBox::InitTexture)
            .def("InitTexture", +[](CUIEditBox* self, pcstr texture) { self->InitTexture(texture); })
    ];
}

void CUIMessageBox::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        class_<CUIMessageBox, CUIStatic>("CUIMessageBox")
            .def(constructor<>())
            .def("Init", &CUIMessageBox::InitMessageBox)
            .def("InitMessageBox", &CUIMessageBox::InitMessageBox)
            .def("SetText", &CUIMessageBox::SetText)
            .def("GetHost", &CUIMessageBox::GetHost)
            .def("GetPassword", &CUIMessageBox::GetPassword)
    ];
}

void CUIOptionsManager::script_register(lua_State* luaState)
{
    using namespace luabind;

    class CUIOptionsManagerScript
    {
    public:
        void SaveBackupValues(pcstr group)
        {
            CUIOptionsItem::GetOptionsManager()->SaveBackupValues(group);
        }
        void SetCurrentValues(pcstr group)
        {
            CUIOptionsItem::GetOptionsManager()->SetCurrentValues(group);
        }
        void SaveValues(pcstr group)
        {
            CUIOptionsItem::GetOptionsManager()->SaveValues(group);
        }
        void UndoGroup(pcstr group)
        {
            CUIOptionsItem::GetOptionsManager()->UndoGroup(group);
        }
        void OptionsPostAccept()
        {
            CUIOptionsItem::GetOptionsManager()->OptionsPostAccept();
        }
        void SendMessage2Group(pcstr group, pcstr message)
        {
            CUIOptionsItem::GetOptionsManager()->SendMessage2Group(group, message);
        }
        bool NeedSystemRestart()
        {
            return CUIOptionsItem::GetOptionsManager()->NeedSystemRestart();
        }
        bool NeedVidRestart()
        {
            return CUIOptionsItem::GetOptionsManager()->NeedVidRestart();
        }
    };

    module(luaState)
    [
        class_<CUIOptionsManagerScript>("COptionsManager")
            .def(constructor<>())
            .def("SaveBackupValues", &CUIOptionsManagerScript::SaveBackupValues)
            .def("SetCurrentValues", &CUIOptionsManagerScript::SetCurrentValues)
            .def("SaveValues", &CUIOptionsManagerScript::SaveValues)
            .def("UndoGroup", &CUIOptionsManagerScript::UndoGroup)
            .def("OptionsPostAccept", &CUIOptionsManagerScript::OptionsPostAccept)
            .def("SendMessage2Group", &CUIOptionsManagerScript::SendMessage2Group)
            .def("NeedSystemRestart", &CUIOptionsManagerScript::NeedSystemRestart)
            .def("NeedVidRestart", &CUIOptionsManagerScript::NeedVidRestart)
    ];
}
