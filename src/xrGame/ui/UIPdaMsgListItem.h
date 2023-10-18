//////////////////////////////////////////////////////////////////////
// UIPdaMsgListItem.h: элемент окна списка в основном
// экране для сообщений PDA
//////////////////////////////////////////////////////////////////////
#pragma once

#include "xrUICore/Static/UIStatic.h"

class CUIPdaMsgListItem final : public CUIColorAnimConrollerContainer
{
    using inherited = CUIColorAnimConrollerContainer;

public:
    CUIPdaMsgListItem() : CUIColorAnimConrollerContainer("CUIPdaMsgListItem") {}

    void InitPdaMsgListItem(const Fvector2& size);
    void SetFont(CGameFont* pFont) override;

    pcstr GetDebugType() override { return "CUIPdaMsgListItem"; }

    CUIStatic UIIcon{ "Icon" };
    CUITextWnd UITimeText;
    CUITextWnd UICaptionText;
    CUITextWnd UIMsgText;
};
