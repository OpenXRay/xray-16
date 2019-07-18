#include "pch.hpp"
#include "UIWindow.h"
#include "Cursor/UICursor.h"
#include "Include/xrRender/DebugRender.h"
#include "Include/xrRender/UIRender.h"
#include "xrEngine/xr_input_xinput.h"

xr_vector<Frect> g_wnds_rects;
BOOL g_show_wnd_rect2 = FALSE;

XRUICORE_API void clean_wnd_rects()
{
#ifdef DEBUG
    GEnv.DRender->DestroyDebugShader(IDebugRender::dbgShaderWindow);
#endif // DEBUG
}

void add_rect_to_draw(Frect r) { g_wnds_rects.push_back(r); }
void draw_rect(Frect& r, u32 color)
{
#ifdef DEBUG

    GEnv.DRender->SetDebugShader(IDebugRender::dbgShaderWindow);

    //.	GlobalEnv.UIRender->StartLineStrip	(5);
    GEnv.UIRender->StartPrimitive(5, IUIRender::ptLineStrip, UI().m_currentPointType);

    GEnv.UIRender->PushPoint(r.lt.x, r.lt.y, 0, color, 0, 0);
    GEnv.UIRender->PushPoint(r.rb.x, r.lt.y, 0, color, 0, 0);
    GEnv.UIRender->PushPoint(r.rb.x, r.rb.y, 0, color, 0, 0);
    GEnv.UIRender->PushPoint(r.lt.x, r.rb.y, 0, color, 0, 0);
    GEnv.UIRender->PushPoint(r.lt.x, r.lt.y, 0, color, 0, 0);

    //.	GlobalEnv.UIRender->FlushLineStrip();
    GEnv.UIRender->FlushPrimitive();

#endif // DEBUG
}

XRUICORE_API void draw_wnds_rects()
{
    if (0 == g_wnds_rects.size())
        return;

    xr_vector<Frect>::iterator it = g_wnds_rects.begin();
    xr_vector<Frect>::iterator it_e = g_wnds_rects.end();

    for (; it != it_e; ++it)
    {
        Frect& r = *it;
        UI().ClientToScreenScaled(r.lt, r.lt.x, r.lt.y);
        UI().ClientToScreenScaled(r.rb, r.rb.x, r.rb.y);
        draw_rect(r, color_rgba(255, 0, 0, 255));
    };

    g_wnds_rects.clear();
}

CUIWindow::CUIWindow()
    : m_pParentWnd(NULL), m_pMouseCapturer(NULL), m_pMessageTarget(NULL), m_pKeyboardCapturer(NULL),
      m_bAutoDelete(false), m_bCursorOverWindow(false), m_dwFocusReceiveTime(0), m_bCustomDraw(false)
{
    Show(true);
    Enable(true);
}

CUIWindow::~CUIWindow()
{
    VERIFY(!(GetParent() && IsAutoDelete()));

    CUIWindow* parent = GetParent();
    bool ad = IsAutoDelete();
    if (parent && !ad)
        parent->CUIWindow::DetachChild(this);

    DetachAll();
}

void CUIWindow::Draw()
{
    for (auto it = m_ChildWndList.begin(); m_ChildWndList.end() != it; ++it)
    {
        if (!(*it)->IsShown())
            continue;
        if ((*it)->GetCustomDraw())
            continue;
        (*it)->Draw();
    }
#ifdef DEBUG
    if (g_show_wnd_rect2)
    {
        Frect r;
        GetAbsoluteRect(r);
        add_rect_to_draw(r);
    }
#endif
}

void CUIWindow::Draw(float x, float y)
{
    SetWndPos(Fvector2().set(x, y));
    Draw();
}

void CUIWindow::Update()
{
    if (GetUICursor().IsVisible())
    {
        bool cursor_on_window;

        Fvector2 temp = GetUICursor().GetCursorPosition();
        Frect r;
        GetAbsoluteRect(r);
        cursor_on_window = !!r.in(temp);
        // RECEIVE and LOST focus
        if (m_bCursorOverWindow != cursor_on_window)
        {
            if (cursor_on_window)
                OnFocusReceive();
            else
                OnFocusLost();
        }
    }

    for (auto it = m_ChildWndList.begin(); m_ChildWndList.end() != it; ++it)
    {
        if (!(*it)->IsShown())
            continue;
        (*it)->Update();
    }
}

void CUIWindow::AttachChild(CUIWindow* pChild)
{
    R_ASSERT(pChild);
    if (!pChild)
        return;

    R_ASSERT(!IsChild(pChild));
    pChild->SetParent(this);
    m_ChildWndList.push_back(pChild);
}

void CUIWindow::DetachChild(CUIWindow* pChild)
{
    R_ASSERT(pChild);
    if (NULL == pChild)
        return;

    if (m_pMouseCapturer == pChild)
        SetCapture(pChild, false);

    //.	SafeRemoveChild			(pChild);
    auto it = std::find(m_ChildWndList.begin(), m_ChildWndList.end(), pChild);
    R_ASSERT(it != m_ChildWndList.end());
    m_ChildWndList.erase(it);

    pChild->SetParent(NULL);

    if (pChild->IsAutoDelete())
        xr_delete(pChild);
}

void CUIWindow::DetachAll()
{
    while (!m_ChildWndList.empty())
    {
        DetachChild(m_ChildWndList.back());
    }
}

void CUIWindow::GetAbsoluteRect(Frect& r)
{
    if (GetParent() == NULL)
    {
        GetWndRect(r);
        return;
    }
    GetParent()->GetAbsoluteRect(r);

    Frect rr;
    GetWndRect(rr);
    r.left += rr.left;
    r.top += rr.top;
    r.right = r.left + GetWidth();
    r.bottom = r.top + GetHeight();
}

//реакция на мышь
//координаты курсора всегда, кроме начального вызова
//задаются относительно текущего окна

#define DOUBLE_CLICK_TIME 250

bool CUIWindow::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    cursor_pos.x = x;
    cursor_pos.y = y;

    if (WINDOW_LBUTTON_DOWN == mouse_action)
    {
        static u32 _last_db_click_frame = 0;
        const u32 dwCurTime = Device.dwTimeContinual;

        if ((_last_db_click_frame != Device.dwFrame) && (dwCurTime - m_dwLastClickTime < DOUBLE_CLICK_TIME))
        {
            mouse_action = WINDOW_LBUTTON_DB_CLICK;
            _last_db_click_frame = Device.dwFrame;
        }

        m_dwLastClickTime = dwCurTime;
    }

    if (GetParent() == NULL)
    {
        const Frect& wndRect = GetWndRect();
        if (!wndRect.in(cursor_pos))
            return false;
        //получить координаты относительно окна
        cursor_pos.x -= wndRect.left;
        cursor_pos.y -= wndRect.top;
    }

    //если есть дочернее окно,захватившее мышь, то
    //сообщение направляем ему сразу
    if (m_pMouseCapturer)
    {
        m_pMouseCapturer->OnMouseAction(cursor_pos.x - m_pMouseCapturer->GetWndRect().left,
            cursor_pos.y - m_pMouseCapturer->GetWndRect().top, mouse_action);
        return true;
    }

    // handle any action
    switch (mouse_action)
    {
    case WINDOW_MOUSE_MOVE: OnMouseMove(); break;
    case WINDOW_MOUSE_WHEEL_DOWN: OnMouseScroll(WINDOW_MOUSE_WHEEL_DOWN); break;
    case WINDOW_MOUSE_WHEEL_UP: OnMouseScroll(WINDOW_MOUSE_WHEEL_UP); break;
    case WINDOW_MOUSE_WHEEL_LEFT: OnMouseScroll(WINDOW_MOUSE_WHEEL_LEFT); break;
    case WINDOW_MOUSE_WHEEL_RIGHT: OnMouseScroll(WINDOW_MOUSE_WHEEL_RIGHT); break;
    case WINDOW_LBUTTON_DOWN:
        if (OnMouseDown(MOUSE_1))
            return true;
        break;
    case WINDOW_RBUTTON_DOWN:
        if (OnMouseDown(MOUSE_2))
            return true;
        break;
    case WINDOW_CBUTTON_DOWN:
        if (OnMouseDown(MOUSE_3))
            return true;
        break;
    case WINDOW_LBUTTON_DB_CLICK:
        if (OnDbClick())
            return true;
        break;
    default: break;
    }

    //Проверка на попадание мыши в окно,
    //происходит в обратном порядке, чем рисование окон
    //(последние в списке имеют высший приоритет)
    WINDOW_LIST::reverse_iterator it = m_ChildWndList.rbegin();

    for (; it != m_ChildWndList.rend(); ++it)
    {
        CUIWindow* w = (*it);
        const Frect& wndRect = w->GetWndRect();
        if (wndRect.in(cursor_pos))
        {
            if (w->IsEnabled())
            {
                if (w->OnMouseAction(
                        cursor_pos.x - w->GetWndRect().left, cursor_pos.y - w->GetWndRect().top, mouse_action))
                    return true;
            }
        }
        else if (w->IsEnabled() && w->CursorOverWindow())
        {
            if (w->OnMouseAction(cursor_pos.x - w->GetWndRect().left, cursor_pos.y - w->GetWndRect().top, mouse_action))
                return true;
        }
    }

    return false;
}

void CUIWindow::OnMouseMove() {}
void CUIWindow::OnMouseScroll(float iDirection) {}
bool CUIWindow::OnDbClick()
{
    if (GetMessageTarget())
        GetMessageTarget()->SendMessage(this, WINDOW_LBUTTON_DB_CLICK);
    return false;
}

bool CUIWindow::OnMouseDown(int mouse_btn) { return false; }
void CUIWindow::OnMouseUp(int mouse_btn) {}
void CUIWindow::OnFocusReceive()
{
    m_dwFocusReceiveTime = Device.dwTimeGlobal;
    m_bCursorOverWindow = true;

    if (GetMessageTarget())
        GetMessageTarget()->SendMessage(this, WINDOW_FOCUS_RECEIVED, NULL);
}

void CUIWindow::OnFocusLost()
{
    m_dwFocusReceiveTime = 0;
    m_bCursorOverWindow = false;

    if (GetMessageTarget())
        GetMessageTarget()->SendMessage(this, WINDOW_FOCUS_LOST, NULL);
}

//Сообщение, посылаемое дочерним окном,
//о том, что окно хочет захватить мышь,
//все сообщения от нее будут направляться только
//ему в независимости от того где мышь
void CUIWindow::SetCapture(CUIWindow* pChildWindow, bool capture_status)
{
    if (GetParent())
    {
        GetParent()->SetCapture(this, capture_status);
    }

    if (capture_status)
    {
        //оповестить дочернее окно о потере фокуса мыши
        if (NULL != m_pMouseCapturer)
            m_pMouseCapturer->SendMessage(this, WINDOW_MOUSE_CAPTURE_LOST);

        m_pMouseCapturer = pChildWindow;
    }
    else
    {
        m_pMouseCapturer = NULL;
    }
}

//реакция на клавиатуру
bool CUIWindow::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    bool result;

    //если есть дочернее окно,захватившее клавиатуру, то
    //сообщение направляем ему сразу
    if (NULL != m_pKeyboardCapturer)
    {
        result = m_pKeyboardCapturer->OnKeyboardAction(dik, keyboard_action);

        if (result)
            return true;
    }

    WINDOW_LIST::reverse_iterator it = m_ChildWndList.rbegin();

    for (; it != m_ChildWndList.rend(); ++it)
    {
        if ((*it)->IsEnabled())
        {
            result = (*it)->OnKeyboardAction(dik, keyboard_action);

            if (result)
                return true;
        }
    }
    return false;
}

bool CUIWindow::OnKeyboardHold(int dik)
{
    bool result;

    if (NULL != m_pKeyboardCapturer)
    {
        result = m_pKeyboardCapturer->OnKeyboardHold(dik);

        if (result)
            return true;
    }

    WINDOW_LIST::reverse_iterator it = m_ChildWndList.rbegin();

    for (; it != m_ChildWndList.rend(); ++it)
    {
        if ((*it)->IsEnabled())
        {
            result = (*it)->OnKeyboardHold(dik);

            if (result)
                return true;
        }
    }

    return false;
}

bool CUIWindow::OnTextInput(pcstr text)
{
    bool result;

    // если есть дочернее окно,захватившее клавиатуру,
    // то сообщение направляем сразу ему
    if (nullptr != m_pKeyboardCapturer)
    {
        result = m_pKeyboardCapturer->OnTextInput(text);

        if (result)
            return true;
    }

    WINDOW_LIST::reverse_iterator it = m_ChildWndList.rbegin();

    for (; it != m_ChildWndList.rend(); ++it)
    {
        if ((*it)->IsEnabled())
        {
            result = (*it)->OnTextInput(text);

            if (result)
                return true;
        }
    }
    return false;
}

void CUIWindow::SetKeyboardCapture(CUIWindow* pChildWindow, bool capture_status)
{
    if (NULL != GetParent())
        GetParent()->SetKeyboardCapture(this, capture_status);

    if (capture_status)
    {
        //оповестить дочернее окно о потере фокуса клавиатуры
        if (NULL != m_pKeyboardCapturer)
            m_pKeyboardCapturer->SendMessage(this, WINDOW_KEYBOARD_CAPTURE_LOST);

        m_pKeyboardCapturer = pChildWindow;
    }
    else
        m_pKeyboardCapturer = NULL;
}

//обработка сообщений
void CUIWindow::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    //оповестить дочерние окна
    for (auto it = m_ChildWndList.begin(); m_ChildWndList.end() != it; ++it)
    {
        if ((*it)->IsEnabled())
            (*it)->SendMessage(pWnd, msg, pData);
    }
}

CUIWindow* CUIWindow::GetCurrentMouseHandler() { return GetTop()->GetChildMouseHandler(); }
CUIWindow* CUIWindow::GetChildMouseHandler()
{
    CUIWindow* pWndResult;
    WINDOW_LIST::reverse_iterator it = m_ChildWndList.rbegin();

    for (; it != m_ChildWndList.rend(); ++it)
    {
        Frect wndRect = (*it)->GetWndRect();
        // very strange code.... i can't understand difference between
        // first and second condition. I Got It from OnMouseAction() method;
        if (wndRect.in(cursor_pos))
        {
            if ((*it)->IsEnabled())
            {
                return pWndResult = (*it)->GetChildMouseHandler();
            }
        }
        else if ((*it)->IsEnabled() && (*it)->CursorOverWindow())
        {
            return pWndResult = (*it)->GetChildMouseHandler();
        }
    }

    return this;
}

//для перевода окна и потомков в исходное состояние
void CUIWindow::Reset() { m_pMouseCapturer = NULL; }
void CUIWindow::ResetAll()
{
    for (auto it = m_ChildWndList.begin(); m_ChildWndList.end() != it; ++it)
    {
        (*it)->Reset();
    }
}

CUIWindow* CUIWindow::GetMessageTarget() { return m_pMessageTarget ? m_pMessageTarget : GetParent(); }
bool CUIWindow::IsChild(CUIWindow* pPossibleChild) const
{
    WINDOW_LIST::const_iterator it = std::find(m_ChildWndList.begin(), m_ChildWndList.end(), pPossibleChild);
    return it != m_ChildWndList.end();
}

CUIWindow* CUIWindow::FindChild(const shared_str name)
{
    if (WindowName() == name)
        return this;

    WINDOW_LIST::const_iterator it = m_ChildWndList.begin();
    WINDOW_LIST::const_iterator it_e = m_ChildWndList.end();
    for (; it != it_e; ++it)
    {
        CUIWindow* pRes = (*it)->FindChild(name);
        if (pRes != NULL)
            return pRes;
    }
    return NULL;
}

void CUIWindow::SetParent(CUIWindow* pNewParent)
{
    R_ASSERT(!(m_pParentWnd && m_pParentWnd->IsChild(this)));

    m_pParentWnd = pNewParent;
}

void CUIWindow::ShowChildren(bool show)
{
    for (auto it = m_ChildWndList.begin(); m_ChildWndList.end() != it; ++it)
        (*it)->Show(show);
}

static bool is_in(Frect const& a, Frect const& b) // b in a
{
    return (a.x1 < b.x1) && (a.x2 > b.x2) && (a.y1 < b.y1) && (a.y2 > b.y2);
}

bool fit_in_rect(CUIWindow* w, Frect const& vis_rect, float border, float dx16pos) // this = hint wnd
{
    float const cursor_height = 43.0f;
    Fvector2 cursor_pos = GetUICursor().GetCursorPosition();
    if (UI().is_widescreen())
    {
        cursor_pos.x -= dx16pos;
    }

    if (!vis_rect.in(cursor_pos))
    {
        return false;
    }

    Frect rect;
    rect.set(-border, -border, w->GetWidth() - 2.0f * border, w->GetHeight() - 2.0f * border);
    rect.add(cursor_pos.x, cursor_pos.y);

    rect.sub(0.0f, rect.height() - border);
    if (!is_in(vis_rect, rect))
    {
        rect.sub(rect.width() - border, 0.0f);
    }
    if (!is_in(vis_rect, rect))
    {
        rect.add(0.0f, rect.height() - border);
    }
    if (!is_in(vis_rect, rect))
    {
        rect.add(rect.width() - border, cursor_height);
    }

    float yn = rect.top - vis_rect.height() + rect.height() - border + cursor_height;
    if (!is_in(vis_rect, rect))
    {
        rect.sub(0.0f, yn);
    }
    if (!is_in(vis_rect, rect))
    {
        rect.sub(rect.width() - border, 0.0f);
    }

    w->SetWndPos(rect.lt);
    return true;
}
