////////////////////////////////////////////////////////////////////////////
//	Module 		: UIHint.h
//	Created 	: 16.04.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Hint (for UIHintWindow) window class
////////////////////////////////////////////////////////////////////////////

#ifndef UI_HINT_H_INCLUDED
#define UI_HINT_H_INCLUDED

#include "xrUICore/Windows/UIWindow.h"

class CUIStatic;
class CUITextWnd;
class CUIFrameWindow;
class CUIXml;

class XRUICORE_API UIHint : public CUIWindow
{
private:
    typedef CUIWindow inherited;

public:
    UIHint();
    virtual ~UIHint(){};

    IC void set_visible(bool status = true) { m_visible = status; }
    IC bool is_visible() const { return m_visible; }
    IC void set_rect(Frect const& rect) { m_rect.set(rect); }
    IC Frect const& get_rect() const { return m_rect; }
    void init_from_xml(CUIXml& xml, LPCSTR path);

    void set_text(LPCSTR text);
    LPCSTR get_text() const;

    //	virtual void	Update		();
    virtual void Draw();

protected:
    CUIFrameWindow* m_background;
    CUITextWnd* m_text;

    bool m_visible;
    float m_border;
    Frect m_rect;
}; // class UIHint

// =================================================================================================
// UIHintWindow = Owner of UIHint
class XRUICORE_API UIHintWindow : public CUIWindow
{
    typedef CUIWindow inherited;

public:
    UIHintWindow();
    virtual ~UIHintWindow(){};

    virtual void Update();
    virtual void OnFocusReceive();
    virtual void OnFocusLost();
    virtual void Show(bool status);

    IC void set_hint_wnd(UIHint* hint_wnd) { m_hint_wnd = hint_wnd; }
    IC UIHint* get_hint_wnd() const { return m_hint_wnd; }
    IC void set_hint_delay(u32 delay) { m_hint_delay = delay; }
    IC u32 get_hint_delay() const { return m_hint_delay; }
    void disable_hint();
    void set_hint_text(shared_str const& text);
    void set_hint_text_ST(shared_str const& text);
    void update_hint_text();

private:
    UIHint* m_hint_wnd;
    u32 m_hint_delay;
    shared_str m_hint_text;
    bool m_enable;
}; // class UIHintWindow

#endif // UI_HINT_H_INCLUDED
