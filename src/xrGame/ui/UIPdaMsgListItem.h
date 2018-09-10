#pragma once
#include "xrUICore/Static/UIStatic.h"

class CUIPdaMsgListItem : public CUIColorAnimConrollerContainer
{
    typedef CUIColorAnimConrollerContainer inherited;

public:
    void InitPdaMsgListItem(const Fvector2& size);
    virtual void SetFont(CGameFont* pFont);

    CUIStatic UIIcon;
    CUITextWnd UITimeText;
    CUITextWnd UICaptionText;
    CUITextWnd UIMsgText;
};
