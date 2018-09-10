#pragma once
#include "UIScrollBar.h"

class CUI3tButton;
class CUIScrollBox;
class CUIFrameLineWnd;

class XRUICORE_API CUIFixedScrollBar : public CUIScrollBar
{
private:
    typedef CUIWindow inherited;

protected:
    CUI3tButton* m_ScrollBox;
    Ivector2 m_ScrollBoxOffset;

    virtual void UpdateScrollBar();
    virtual void ClampByViewRect();
    virtual void SetPosScrollFromView(float view_pos, float view_width, float view_offs);

public:
    CUIFixedScrollBar();
    virtual ~CUIFixedScrollBar();
    virtual void InitScrollBar(Fvector2 pos, bool horizontal, LPCSTR profile = "pda");
    virtual void SetWidth(float width){};
    virtual void SetHeight(float height){};
    virtual void Draw() { inherited::Draw(); };
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual bool OnMouseDown(int mouse_btn);
    virtual bool OnMouseDownEx();
    virtual void OnMouseUp(int mouse_btn);
    virtual bool OnKeyboardHold(int dik);
};
