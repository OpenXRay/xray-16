#pragma once
#include "xrUICore/TabControl/UITabButton.h"

class CUITabButtonMP : public CUITabButton
{
    typedef CUITabButton inherited;

public:
    CUITabButtonMP();
    virtual void Draw();
    virtual void Update();
    virtual void UpdateTextAlign();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);

    virtual bool IsEnabled() { return true; }
    void SetOrientation(bool bVert) { m_orientationVertical = bVert; };
    void CreateHint();

    shared_str m_temp_id;

    Fvector2 m_text_ident_normal;
    Fvector2 m_text_ident_cursor_over;
    CUIStatic* m_hint;
    bool m_orientationVertical;
};
