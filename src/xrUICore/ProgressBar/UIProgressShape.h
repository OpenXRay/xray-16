#pragma once

#include "xrUICore/Static/UIStatic.h"

class CUIStatic;

// XXX: make it like CUIProgressBar
// and add following functions:
/*
    SetRange
    GetProgressPos
    SetProgressPos
    GetRange_min
    GetRange_max
*/
class XRUICORE_API CUIProgressShape final : public CUIStatic
{
    friend class CUIXmlInitBase;

public:
    CUIProgressShape();

    void SetPos(int pos, int max);
    void SetPos(float pos);
    void SetTextVisible(bool b);

    virtual void Draw();

    pcstr GetDebugType() override { return "CUIProgressShape"; }

protected:
    bool m_bClockwise;
    u32 m_sectorCount;
    float m_stage;
    CUIStatic* m_pTexture;
    CUIStatic* m_pBackground;
    bool m_bText;
    bool m_blend;

    float m_angle_begin;
    float m_angle_end;
};
