#pragma once
#include "xrUICore/Static/UIStatic.h"

class XRUICORE_API UI_Arrow final : public CUIStatic
{
private:
    typedef CUIStatic inherited;

public:
    UI_Arrow();

    void init_from_xml(CUIXml& xml, LPCSTR path, CUIWindow* parent);
    void SetNewValue(float new_value);
    void SetPos(float pos);
    IC float GetPos() { return m_pos; }

    pcstr GetDebugType() override { return "UI_Arrow"; }

private:
    float m_angle_begin;
    float m_angle_end;
    float m_ang_velocity;
    float m_angle_range;

    float m_temp_pos;
    float m_pos;

}; // class UI_Arrow
