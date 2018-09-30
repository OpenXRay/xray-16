#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Buttons/UIButton.h"

class XRUICORE_API CUIProgressBar : public CUIWindow
{
    friend class CUIXmlInitBase;
    typedef CUIWindow inherited;

protected:
    //	bool				m_bIsHorizontal;
    enum EOrientMode
    {
        om_horz = 0,
        om_vert = 1,
        om_back = 2,
        om_down = 3,
        om_fromcenter = 4,
        om_vfromcenter = 5,
        om_count
    } m_orient_mode;

    Fvector2 m_ProgressPos; // x-current y-dest
    float m_MinPos;
    float m_MaxPos;

    float m_CurrentLength;

    bool m_bBackgroundPresent;
    Fvector2 m_BackgroundOffset;
    u32 m_last_render_frame;
    void UpdateProgressBar();

public:
    bool m_bUseColor;
    bool m_bUseGradient; //Alundaio: if false then use only solid color with m_maxColor
    bool colorSmoothing;
    Fcolor m_minColor;
    Fcolor m_middleColor;
    Fcolor m_maxColor;
    float m_inertion; //

public:
    CUIStatic m_UIProgressItem;
    CUIStatic m_UIBackgroundItem;

    CUIProgressBar();
    virtual ~CUIProgressBar();

    void InitProgressBar(Fvector2 pos, Fvector2 size, EOrientMode mode);

    void SetRange(float _Min, float _Max)
    {
        m_MinPos = _Min;
        m_MaxPos = _Max;
        UpdateProgressBar();
    }
    float GetRange_min() { return m_MinPos; }
    float GetRange_max() { return m_MaxPos; }
    void SetProgressPos(float _Pos);
    float GetProgressPos() { return m_ProgressPos.y; }
    void ShowBackground(bool status) { m_bBackgroundPresent = status; }
    bool IsShownBackground() { return m_bBackgroundPresent; }
    virtual void Draw();
    virtual void Update();
};
