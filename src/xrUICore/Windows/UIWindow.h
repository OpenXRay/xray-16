#pragma once

#define ui_list xr_vector

//////////////////////////////////////////////////////////////////////////

#include "xrUICore/UIMessages.h"
#include "xrUICore/uiabstract.h"
#include "xrUICore/ui_debug.h"

#include "xrScriptEngine/ScriptExporter.hpp"

class XRUICORE_API CUIWindow : public CUIDebuggable
{
public:
    CUIWindow(pcstr window_name);
    ~CUIWindow() override;

    virtual void SetWndPos(const Fvector2& pos)
    {
        m_wndPos.set(pos.x, pos.y);
    }
    [[nodiscard]]
    const Fvector2& GetWndPos() const
    {
        return m_wndPos;
    }

    [[nodiscard]]
    Fvector2 GetWndCenterPos() const
    {
        return { m_wndPos.x + m_wndSize.x / 2.0f, m_wndPos.y + m_wndSize.y / 2.0f, };
    }

    virtual void SetWndSize(const Fvector2& size)
    {
        m_wndSize = size;
    }
    [[nodiscard]]
    const Fvector2& GetWndSize() const
    {
        return m_wndSize;
    }

    virtual void SetWndRect(const Frect& rect)
    {
        m_wndPos.set(rect.lt);
        rect.getsize(m_wndSize);
    }

    virtual void SetHeight(float height)
    {
        m_wndSize.y = height;
    }
    [[nodiscard]]
    float GetHeight() const
    {
        return m_wndSize.y;
    }
    virtual void SetWidth(float width)
    {
        m_wndSize.x = width;
    }
    [[nodiscard]]
    float GetWidth() const
    {
        return m_wndSize.x;
    }

    void SetVisible(bool vis)
    {
        m_bShowMe = vis;
    }
    [[nodiscard]]
    bool GetVisible() const
    {
        return m_bShowMe;
    }
    void SetAlignment(EWindowAlignment al)
    {
        m_alignment = al;
    }
    [[nodiscard]]
    EWindowAlignment GetAlignment() const
    {
        return m_alignment;
    }
    [[nodiscard]]
    Frect GetWndRect() const
    {
        Frect r;
        GetWndRect(r);
        return r;
    }
    void GetWndRect(Frect& res) const
    {
        const float width = (float)Device.dwWidth * (UI_BASE_WIDTH / (float)Device.dwWidth);
        const float height = (float)Device.dwHeight * (UI_BASE_HEIGHT / (float)Device.dwHeight);

        switch (m_alignment)
        {
        case waNone:
        case waLeft:
        {
            res.set(m_wndPos.x, m_wndPos.y, m_wndPos.x + m_wndSize.x, m_wndPos.y + m_wndSize.y);
            break;
        }
        case waCenter:
        {
            float half_w = m_wndSize.x / 2.0f;
            float half_h = m_wndSize.y / 2.0f;
            res.set(m_wndPos.x - half_w, m_wndPos.y - half_h, m_wndPos.x + half_w, m_wndPos.y + half_h);
            break;
        }
        case waRight:
        {
            res.set(width - m_wndSize.x, m_wndPos.y, width, m_wndPos.y + m_wndSize.y);
            break;
        }
        case waTop:
        {
            res.set(m_wndPos.x, 0.f, m_wndPos.x + m_wndSize.x, m_wndSize.y);
            break;
        }
        case waBottom:
            res.set(m_wndPos.x, height - m_wndSize.y, m_wndPos.x + m_wndSize.x, height);
            break;
        default: NODEFAULT;
        }
    }

    void MoveWndDelta(float dx, float dy)
    {
        m_wndPos.x += dx;
        m_wndPos.y += dy;
    }

    void MoveWndDelta(const Fvector2& d)
    {
        MoveWndDelta(d.x, d.y);
    }

    ////////////////////////////////////
    //работа с дочерними и родительскими окнами
    virtual void AttachChild(CUIWindow* pChild);
    virtual void DetachChild(CUIWindow* pChild);
    virtual void DetachAll();

    [[nodiscard]]
    virtual bool IsChild(CUIWindow* pPossibleChild) const;

    [[nodiscard]]
    auto GetChildNum() const { return m_ChildWndList.size(); }

    [[nodiscard]]
    CUIWindow* GetParent() const { return m_pParentWnd; }
    void SetParent(CUIWindow* pNewParent);

    [[nodiscard]]
    CUIWindow* GetWindowBeforeParent(const CUIWindow* parent)
    {
        CUIWindow* result = this;
        for (CUIWindow* it = GetParent(); it; it = it->GetParent())
        {
            if (it == parent)
                return result;
            result = it;
        }
        return nullptr;
    }

    //получить окно самого верхнего уровня
    [[nodiscard]]
    CUIWindow* GetTop()
    {
        if (m_pParentWnd == nullptr)
            return this;
        return m_pParentWnd->GetTop();
    }
    //получить окно самого верхнего уровня
    [[nodiscard]]
    const CUIWindow* GetTop() const
    {
        if (m_pParentWnd == nullptr)
            return this;
        return m_pParentWnd->GetTop();
    }

    [[nodiscard]]
    CUIWindow* GetCurrentMouseHandler();

    [[nodiscard]]
    CUIWindow* GetChildMouseHandler();

    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual bool OnTextInput(pcstr text);

    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void OnMouseMove();
    virtual void OnMouseScroll(float iDirection);
    virtual bool OnDbClick();
    virtual bool OnMouseDown(int mouse_btn);
    virtual void OnMouseUp(int mouse_btn);

    virtual bool OnControllerAction(int axis, const ControllerAxisState& state, EUIMessages controller_action);

    virtual void OnFocusReceive();
    virtual void OnFocusLost();

    //захватить/освободить мышь окном
    //сообщение посылается дочерним окном родительскому
    void SetCapture(CUIWindow* pChildWindow, bool capture_status);

    [[nodiscard]]
    CUIWindow* GetMouseCapturer() const { return m_pMouseCapturer; }

    //окошко, которому пересылаются сообщения,
    //если NULL, то шлем на GetParent()
    void SetMessageTarget(CUIWindow* pWindow) { m_pMessageTarget = pWindow; }

    [[nodiscard]]
    CUIWindow* GetMessageTarget();

    void SetKeyboardCapture(CUIWindow* pChildWindow, bool capture_status);

    [[nodiscard]]
    CUIWindow* GetKeyboardCapturer() const { return m_pKeyboardCapturer; }

    //обработка сообщений не предусмотреных стандартными обработчиками
    //ф-ция должна переопределяться
    // pWnd - указатель на окно, которое послало сообщение
    // pData - указатель на дополнительные данные, которые могут понадобиться
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = nullptr);

    virtual void Enable(bool status) { m_bIsEnabled = status; }

    [[nodiscard]]
    bool IsEnabled() const { return m_bIsEnabled; }

    [[nodiscard]]
    bool IsFocusValuable(const CUIWindow* top_parent, const CUIWindow* locker) const
    {
        bool valuable;
        bool child_of_locker{};
        const CUIWindow* it = this;

        for (;; it = it->GetParent())
        {
            valuable = it->IsShown() && it->IsEnabled();
            if (it == locker)
                child_of_locker = true;
            if (!valuable || !it->GetParent())
                break;
        }

        if (locker && !child_of_locker)
            return false;
        if (top_parent && top_parent != it)
            return false;
        return valuable;
    }

    //убрать/показать окно и его дочерние окна
    virtual void Show(bool status)
    {
        SetVisible(status);
        Enable(status);
    }

    [[nodiscard]]
    virtual bool IsShown() const { return GetVisible(); }

    void ShowChildren(bool show);

    //абсолютные координаты
    [[nodiscard]]
    void GetAbsoluteRect(Frect& r) const;

    void GetAbsolutePos(Fvector2& p) const
    {
        Frect abs;
        GetAbsoluteRect(abs);
        p.set(abs.x1, abs.y1);
    }

    [[nodiscard]]
    Fvector2 GetAbsoluteCenterPos() const
    {
        Frect abs;
        GetAbsoluteRect(abs);
        auto size = GetWndSize();
        size.div(2.0f);
        return { abs.x1 + size.x, abs.y1 + size.y };
    }

    // прорисовка окна
    virtual void Draw();
    virtual void Draw(float x, float y);

    // обновление окна перед прорисовкой
    virtual void Update();

    // для перевода окна и потомков в исходное состояние
    virtual void Reset();
    void ResetAll();

    virtual void SetFont(CGameFont* /*pFont*/) {}

    [[nodiscard]]
    virtual CGameFont* GetFont()
    {
        if (m_pParentWnd)
            return m_pParentWnd->GetFont();

        return nullptr;
    }

    using WINDOW_LIST = ui_list<CUIWindow*>;

    [[nodiscard]]
    WINDOW_LIST& GetChildWndList() { return m_ChildWndList; }
    [[nodiscard]]
    const WINDOW_LIST& GetChildWndList() const { return m_ChildWndList; }

    [[nodiscard]]
    bool IsAutoDelete() const { return m_bAutoDelete; }
    void SetAutoDelete(bool auto_delete) { m_bAutoDelete = auto_delete; }

    // Name of the window
    [[nodiscard]]
    shared_str WindowName() const { return m_windowName; }
    void SetWindowName(pcstr wn) { m_windowName = wn; }

    CUIWindow* FindChild(const shared_str name);

    [[nodiscard]]
    virtual bool IsUsingCursorRightNow() const { return false; }

    [[nodiscard]]
    bool CursorOverWindow() const { return m_bCursorOverWindow; }

    [[nodiscard]]
    u32 FocusReceiveTime() const { return m_dwFocusReceiveTime; }

    [[nodiscard]]
    bool GetCustomDraw() const { return m_bCustomDraw; }
    void SetCustomDraw(bool b) { m_bCustomDraw = b; }

    [[nodiscard]]
    pcstr GetDebugType() override { return "CUIWindow"; }
    bool FillDebugTree(const CUIDebugState& debugState) override;
    void FillDebugInfo() override;

protected:
    void SafeRemoveChild(CUIWindow* child)
    {
        auto it = std::find(m_ChildWndList.begin(), m_ChildWndList.end(), child);
        if (it != m_ChildWndList.end())
            m_ChildWndList.erase(it);
    }

    bool m_bShowMe : 1 {};

    //флаг автоматического удаления во время вызова деструктора
    bool m_bAutoDelete : 1 {};

    // Is user input allowed
    bool m_bIsEnabled : 1 { true };

    // Если курсор над окном
    bool m_bCursorOverWindow : 1 {};
    bool m_bCustomDraw : 1 {};

    EWindowAlignment m_alignment{};
    Fvector2 m_wndPos{};
    Fvector2 m_wndSize{};

    shared_str m_windowName;

    //список дочерних окон
    WINDOW_LIST m_ChildWndList;

    //указатель на родительское окно
    CUIWindow* m_pParentWnd{};

    //дочернее окно которое, захватило ввод мыши
    CUIWindow* m_pMouseCapturer{};

    //дочернее окно которое, захватило ввод клавиатуры
    CUIWindow* m_pKeyboardCapturer{};

    //кому шлем сообщения
    CUIWindow* m_pMessageTarget{};

    // Последняя позиция мышки
    Fvector2 cursor_pos;

    //время прошлого клика мышки
    //для определения DoubleClick
    u32 m_dwLastClickTime;
    u32 m_dwFocusReceiveTime{};

private:
    DECLARE_SCRIPT_REGISTER_FUNCTION();
};

XRUICORE_API bool fit_in_rect(CUIWindow* w, Frect const& vis_rect, float border = 0.0f, float dx16pos = 0.0f);
