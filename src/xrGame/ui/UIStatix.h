#pragma once

#include "xrUICore/Static/UIStatic.h"

class CUIStatix final : public CUIStatic
{
public:
    CUIStatix();

    virtual void Update();
    virtual void OnFocusReceive();
    virtual void OnFocusLost();
    virtual bool OnMouseDown(int mouse_btn);
    void SetSelectedState(bool state);
    bool GetSelectedState();

    pcstr GetDebugType() override { return "CUIStatix"; }

private:
    void start_anim();
    void stop_anim();

    bool m_bSelected{};
};
