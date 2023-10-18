#pragma once
#include "Windows/UIFrameLineWnd.h"

class CUIScrollBox final : public CUIFrameLineWnd
{
    typedef CUIFrameLineWnd inherited;

public:
    CUIScrollBox() : CUIFrameLineWnd(GetDebugType()) {}

    bool OnMouseAction(float x, float y, EUIMessages mouse_action) override;

    pcstr GetDebugType() override { return "CUIScrollBox"; }
};
