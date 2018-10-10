#include "pch.hpp"
#include "UITabButton.h"
#include "xrEngine/xr_input_xinput.h"

CUITabButton::CUITabButton() {}
CUITabButton::~CUITabButton() {}
bool CUITabButton::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    return CUIWindow::OnMouseAction(x, y, mouse_action);
}

bool CUITabButton::OnMouseDown(int mouse_btn)
{
    if (mouse_btn == MOUSE_1)
    {
        GetMessageTarget()->SendMessage(this, TAB_CHANGED, NULL);
        return true;
    }
    else
        return false;
}

void CUITabButton::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (!IsEnabled())
        return;

    switch (msg)
    {
    case TAB_CHANGED:
        if (this == pWnd)
        {
            SetButtonState(BUTTON_PUSHED);
            OnClick();
        }
        else
        {
            SetButtonState(BUTTON_NORMAL);
        }
        break;
    default:;
    }
}
