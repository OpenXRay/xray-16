#pragma once
#include "xrUICore/Windows/UIWindow.h"

class CUI3tButton;
class CUIScrollBox;
class CUIFrameLineWnd;

class XRUICORE_API CUIScrollBar : public CUIWindow
{
private:
    typedef CUIWindow inherited;

protected:
    CUI3tButton* m_DecButton;
    CUI3tButton* m_IncButton;
    CUIScrollBox* m_ScrollBox;
    CUIFrameLineWnd* m_FrameBackground;

    float m_hold_delay{ 50.0f };
    int m_iScrollPos{};

    int m_iStepSize{ 1 };

    int m_iMinPos{ 1 };
    int m_iMaxPos{ 1 };

    int m_iPageSize{ 1 };

    int m_ScrollWorkArea;
    bool m_b_enabled{ true };
    bool m_bIsHorizontal;

    int m_mouse_state{};

    bool ScrollInc(bool by_scrollbox = false);
    bool ScrollDec(bool by_scrollbox = false);
    virtual void UpdateScrollBar();

    u32 ScrollSize() const { return _max(1, m_iMaxPos - m_iMinPos - m_iPageSize + 1); }
    virtual void ClampByViewRect();
    virtual void SetPosScrollFromView(float view_pos, float view_size, float view_offs);
    int PosViewFromScroll(int view_size, int view_offs) const;
    void SetScrollPosClamped(int iPos);

    bool IsRelevant() const;

public:
    CUIScrollBar();

    void SetEnabled(bool b)
    {
        m_b_enabled = b;
        if (!m_b_enabled)
            Show(m_b_enabled);
    }
    bool GetEnabled() const { return m_b_enabled; }
    virtual void Show(bool b);
    virtual void Enable(bool b);
    virtual bool InitScrollBar(Fvector2 pos, float length, bool bIsHorizontal, cpcstr profile = "default");

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual bool OnMouseDown(int mouse_btn);
    virtual bool OnMouseDownEx();
    virtual void OnMouseUp(int mouse_btn);
    bool OnKeyboardAction(int dik, EUIMessages keyboard_action) override;

    virtual void Draw();

    virtual void SetWidth(float width);
    virtual void SetHeight(float height);

    virtual void Reset();
    void Refresh();
    void SetStepSize(int step);
    IC int GetStepSize() const { return m_iStepSize; }
    void SetRange(int iMin, int iMax);
    void GetRange(int& iMin, int& iMax) const
    {
        iMin = m_iMinPos;
        iMax = m_iMaxPos;
    }
    int GetMaxRange() const { return m_iMaxPos; }
    int GetMinRange() const { return m_iMinPos; }
    void SetPageSize(int iPage)
    {
        m_iPageSize = _max(0, iPage);
        UpdateScrollBar();
    }
    int GetPageSize() const { return m_iPageSize; }
    void SetScrollPos(int iPos)
    {
        SetScrollPosClamped(iPos);
        UpdateScrollBar();
    }
    int GetScrollPos() const { return _max(m_iMinPos, m_iScrollPos); }
    void TryScrollInc(bool by_scrollbox = false);
    void TryScrollDec(bool by_scrollbox = false);

    pcstr GetDebugType() override { return "CUIScrollBar"; }
    void FillDebugInfo() override;
};
