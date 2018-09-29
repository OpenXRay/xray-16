#include "pch.hpp"
#include "UIScrollBox.h"
#include "Cursor/UICursor.h"

CUIScrollBox::CUIScrollBox() {}
bool CUIScrollBox::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    Fvector2 border;
    border.x = 512.0f; // :)
    border.y = 512.0f;

    bool over_x = (x >= -border.x && x < (GetWidth() + border.x));
    bool over_y = (y >= -border.y && y < (GetHeight() + border.y));

    bool cursor_over = false;
    if (over_x && over_y)
    {
        cursor_over = true;
    }

    bool im_capturer = (GetParent()->GetMouseCapturer() == this);

    if (mouse_action == WINDOW_LBUTTON_DOWN || mouse_action == WINDOW_LBUTTON_DB_CLICK)
    {
        GetParent()->SetCapture(this, true);
        return true;
    }
    if (mouse_action == WINDOW_LBUTTON_UP)
    {
        GetParent()->SetCapture(this, false);
        return true;
    }

    if (im_capturer && mouse_action == WINDOW_MOUSE_MOVE && cursor_over)
    {
        Fvector2 pos = GetWndPos();
        Fvector2 delta = GetUICursor().GetCursorPositionDelta();

        if (IsHorizontal())
            pos.x += delta.x;
        else
            pos.y += delta.y;

        SetWndPos(pos);

        GetMessageTarget()->SendMessage(this, SCROLLBOX_MOVE);
    }

    if (!cursor_over)
    {
        GetParent()->SetCapture(this, false);
    }
    return true;
}
