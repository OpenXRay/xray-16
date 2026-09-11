#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Buttons/UIButton.h"

class XRUICORE_API CUIProgressBar final : public CUIWindow
{
    friend class CUIXmlInitBase;
    friend class CUIDoubleProgressBar;

protected:
    using inherited = CUIWindow;

    enum EOrientMode : u8
    {
        om_horz = 0,
        om_vert = 1,
        om_back = 2,
        om_down = 3,
        om_fromcenter = 4,
        om_vfromcenter = 5,
        om_count
    } m_orient_mode{ om_horz };

    bool m_bUseColor          : 1 { false };
    bool m_bUseMiddleColor    : 1 { false }; // Hrust: optional middle color for CS/SoC compatibility, without middle color it doesn't looks correctly
    bool m_bUseGradient       : 1 { true }; //Alundaio: if false then use only solid color with m_maxColor

    Fvector2 m_ProgressPos{}; // x-current y-dest
    float m_MinPos{ 1.0f };
    float m_MaxPos{ 1.0f + EPS };

    float m_CurrentLength{};

    Fcolor m_minColor{};
    Fcolor m_middleColor{};
    Fcolor m_maxColor{};
    float m_inertion{};

protected:
    void UpdateProgressBar();

public:
    CUIStatic m_UIProgressItem;
    CUIStatic m_UIBackgroundItem;

    CUIProgressBar();

    void InitProgressBar(Fvector2 pos, Fvector2 size, EOrientMode mode);

    void Draw() override;
    void Update() override;

    void SetRange(float min, float max)
    {
        m_MinPos = min;
        m_MaxPos = max;
        UpdateProgressBar();
    }

    [[nodiscard]] float GetRange_min() const { return m_MinPos; }
    [[nodiscard]] float GetRange_max() const { return m_MaxPos; }

    [[nodiscard]]
    float GetProgressPos() const { return m_ProgressPos.y; }
    void SetProgressPos(float pos);
    void ForceSetProgressPos(float pos);

    [[nodiscard]]
    bool IsShownBackground() const { return m_UIBackgroundItem.GetVisible(); }
    void ShowBackground(bool status) { m_UIBackgroundItem.SetVisible(status); }

    void UseGradient(bool status) { m_bUseGradient = status; }

    pcstr GetDebugType() override { return "CUIProgressBar"; }
    void FillDebugInfo() override;

private:
    DECLARE_SCRIPT_REGISTER_FUNCTION(CUIWindow);
};
