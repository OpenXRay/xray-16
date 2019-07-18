#pragma once
#include "xrUICore/TabControl/UITabButton.h"

class CUIRadioButton : public CUITabButton
{
    typedef CUITabButton inherited;

public:
    virtual void InitButton(Fvector2 pos, Fvector2 size);
    virtual bool InitTexture(pcstr texture, bool fatal = true);
    virtual void SetTextX(float x) { /*do nothing*/}
    bool OnMouseDown(int mouse_btn) override;
};
