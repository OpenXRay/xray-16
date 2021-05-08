#pragma once
#include "xrUICore/Buttons/UI3tButton.h"

class XRUICORE_API CUITabButton : public CUI3tButton
{
    friend class CUIXmlInitBase;
    typedef CUI3tButton inherited;

    bool m_btn_id_default_assigned{};

public:
    shared_str m_btn_id;

    CUITabButton();
    virtual ~CUITabButton();

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = 0);
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual bool OnMouseDown(int mouse_btn);

    bool IsIdDefaultAssigned() const { return m_btn_id_default_assigned; }
};
