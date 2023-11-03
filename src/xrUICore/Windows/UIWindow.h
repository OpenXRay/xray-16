#pragma once

#define ui_list xr_vector

#define DEF_UILIST(N, T)  \
    typedef ui_list<T> N; \
    typedef N::iterator N##_it;

//////////////////////////////////////////////////////////////////////////

#include "xrUICore/UIMessages.h"
#include "xrUICore/uiabstract.h"
#include "xrUICore/ui_debug.h"

class XRUICORE_API CUIWindow : public CUISimpleWindow, public CUIDebuggable
{
public:
    CUIWindow(pcstr window_name);
    ~CUIWindow() override;

    ////////////////////////////////////
    //работа с дочерними и родительскими окнами
    virtual void AttachChild(CUIWindow* pChild);
    virtual void DetachChild(CUIWindow* pChild);
    virtual void DetachAll();

    [[nodiscard]]
    virtual bool IsChild(CUIWindow* pPossibleChild) const;

    [[nodiscard]]
    u32 GetChildNum() const { return (u32)m_ChildWndList.size(); }

    [[nodiscard]]
    CUIWindow* GetParent() const { return m_pParentWnd; }
    void SetParent(CUIWindow* pNewParent);

    //получить окно самого верхнего уровня
    [[nodiscard]]
    CUIWindow* GetTop()
    {
        if (m_pParentWnd == NULL)
            return this;
        else
            return m_pParentWnd->GetTop();
    }
    CUIWindow* GetCurrentMouseHandler();
    CUIWindow* GetChildMouseHandler();

    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual bool OnTextInput(pcstr text);

    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void OnMouseMove();
    virtual void OnMouseScroll(float iDirection);
    virtual bool OnDbClick();
    virtual bool OnMouseDown(int mouse_btn);
    virtual void OnMouseUp(int mouse_btn);

    virtual bool OnControllerAction(int axis, float x, float y, EUIMessages controller_action);

    virtual void OnFocusReceive();
    virtual void OnFocusLost();

    //захватить/освободить мышь окном
    //сообщение посылается дочерним окном родительскому
    void SetCapture(CUIWindow* pChildWindow, bool capture_status);
    CUIWindow* GetMouseCapturer() { return m_pMouseCapturer; }

    //окошко, которому пересылаются сообщения,
    //если NULL, то шлем на GetParent()
    void SetMessageTarget(CUIWindow* pWindow) { m_pMessageTarget = pWindow; }
    CUIWindow* GetMessageTarget();

    void SetKeyboardCapture(CUIWindow* pChildWindow, bool capture_status);

    //обработка сообщений не предусмотреных стандартными обработчиками
    //ф-ция должна переопределяться
    // pWnd - указатель на окно, которое послало сообщение
    // pData - указатель на дополнительные данные, которые могут понадобиться
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    virtual void Enable(bool status) { m_bIsEnabled = status; }

    [[nodiscard]]
    bool IsEnabled() const { return m_bIsEnabled; }

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
    void GetAbsoluteRect(Frect& r);
    IC void GetAbsolutePos(Fvector2& p)
    {
        Frect abs;
        GetAbsoluteRect(abs);
        p.set(abs.x1, abs.y1);
    }

    void SetWndRect_script(Frect rect) { CUISimpleWindow::SetWndRect(rect); }
    void SetWndPos_script(Fvector2 pos) { CUISimpleWindow::SetWndPos(pos); }
    void SetWndSize_script(Fvector2 size) { CUISimpleWindow::SetWndSize(size); }
    //прорисовка окна
    virtual void Draw();
    virtual void Draw(float x, float y);
    //обновление окна передпрорисовкой
    virtual void Update();

    //для перевода окна и потомков в исходное состояние
    virtual void Reset();
    void ResetAll();

    virtual void SetFont(CGameFont* pFont)
    {
        UNUSED(pFont);
    }

    virtual CGameFont* GetFont()
    {
        if (m_pParentWnd)
            return m_pParentWnd->GetFont();

        return nullptr;
    }

    using WINDOW_LIST = ui_list<CUIWindow*>;

    WINDOW_LIST& GetChildWndList() { return m_ChildWndList; }

    [[nodiscard]]
    IC bool IsAutoDelete() const { return m_bAutoDelete; }
    IC void SetAutoDelete(bool auto_delete) { m_bAutoDelete = auto_delete; }

    // Name of the window
    shared_str WindowName() const { return m_windowName; }
    void SetWindowName(pcstr wn) { m_windowName = wn; }

    CUIWindow* FindChild(const shared_str name);

    [[nodiscard]]
    IC bool CursorOverWindow() const { return m_bCursorOverWindow; }

    [[nodiscard]]
    IC u32 FocusReceiveTime() const { return m_dwFocusReceiveTime; }

    IC bool GetCustomDraw() const { return m_bCustomDraw; }
    IC void SetCustomDraw(bool b) { m_bCustomDraw = b; }

    pcstr GetDebugType() override { return "CUIWindow"; }
    bool FillDebugTree(const CUIDebugState& debugState) override;
    void FillDebugInfo() override;

protected:
    IC void SafeRemoveChild(CUIWindow* child)
    {
        auto it = std::find(m_ChildWndList.begin(), m_ChildWndList.end(), child);
        if (it != m_ChildWndList.end())
            m_ChildWndList.erase(it);
    };

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

    //флаг автоматического удаления во время вызова деструктора
    bool m_bAutoDelete{};

    // Is user input allowed
    bool m_bIsEnabled;

    // Если курсор над окном
    bool m_bCursorOverWindow{};
    bool m_bCustomDraw{};
};

XRUICORE_API bool fit_in_rect(CUIWindow* w, Frect const& vis_rect, float border = 0.0f, float dx16pos = 0.0f);
