#pragma once
#include "xrUICore/Static/UIStatic.h"

class CUISleepStatic final : public CUIStatic
{
private:
    typedef CUIStatic inherited;

    int m_cur_time{};
    CUIStaticItem m_UIStaticItem2;

public:
    CUISleepStatic();
    virtual void Draw();
    virtual void Update();
    virtual void InitTextureEx(LPCSTR tex_name, LPCSTR sh_name = "hud" DELIMITER "default");
    pcstr GetDebugType() override { return "CUISleepStatic"; }
};
