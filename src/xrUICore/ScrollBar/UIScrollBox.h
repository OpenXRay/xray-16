#pragma once
#include "Windows/UIFrameLineWnd.h"

class CUIScrollBox : public CUIFrameLineWnd
{
    typedef CUIFrameLineWnd inherited;

public:
    CUIScrollBox();

    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
};
