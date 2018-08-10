#pragma once

#include "xrUICore/Windows/UIWindow.h"
#include "xrEngine/LightAnimLibrary.h"
#include "xrCore/_color.h"

class CLAItem;

#define LA_CYCLIC (1 << 0)
#define LA_ONLYALPHA (1 << 1)
#define LA_TEXTCOLOR (1 << 2)
#define LA_TEXTURECOLOR (1 << 3)

struct color_animation
{
    color_animation();
    CLAItem* m_lanim;
    float m_lanim_start_time;
    float m_lanim_delay_time;
    Flags8 m_lanimFlags;
};

struct xform_animation : public color_animation
{
    xform_animation();
    Fvector2 m_origSize;
    void set_defaults();
};

class CUILightAnimColorConroller
{
public:
    virtual bool IsColorAnimationPresent() = 0;
    virtual void ResetColorAnimation() = 0;
    virtual void SetColorAnimation(LPCSTR lanim, u8 const& flags, float delay = 0.0f) = 0;
    virtual void ColorAnimationSetTextureColor(u32 color, bool only_alpha){};
    virtual void ColorAnimationSetTextColor(u32 color, bool only_alpha){};
};

class CUILightAnimColorConrollerImpl : public CUILightAnimColorConroller
{
    color_animation m_lanim_clr;

public:
    void SetColorAnimation(LPCSTR lanim, u8 const& flags, float delay = 0.0f)
    {
        if (lanim && lanim[0] != 0)
            m_lanim_clr.m_lanim = LALib.FindItem(lanim);
        else
        {
            m_lanim_clr.m_lanim = NULL;
            return;
        }

        m_lanim_clr.m_lanim_delay_time = delay;
        m_lanim_clr.m_lanimFlags.assign(flags);
        R_ASSERT((m_lanim_clr.m_lanim == NULL) || m_lanim_clr.m_lanimFlags.test(LA_TEXTCOLOR | LA_TEXTURECOLOR));
    }
    virtual void ResetColorAnimation()
    {
        m_lanim_clr.m_lanim_start_time = Device.dwTimeContinual / 1000.0f + m_lanim_clr.m_lanim_delay_time / 1000.0f;
    }
    virtual bool IsColorAnimationPresent()
    {
        if (m_lanim_clr.m_lanim == NULL)
            return false;

        if (m_lanim_clr.m_lanimFlags.test(LA_CYCLIC) || m_lanim_clr.m_lanim_start_time < 0.0f)
            return true;

        float t = Device.dwTimeContinual / 1000.0f;
        if (t - m_lanim_clr.m_lanim_start_time < m_lanim_clr.m_lanim->Length_sec())
            return true;
        else
            return false;
    }

    void UpdateColorAnimation()
    {
        if (m_lanim_clr.m_lanim == NULL)
            return;
        if (m_lanim_clr.m_lanim_start_time < 0.0f)
            ResetColorAnimation();

        float t = Device.dwTimeContinual / 1000.0f;

        if (t < m_lanim_clr.m_lanim_start_time) // consider animation delay
            return;

        if (m_lanim_clr.m_lanimFlags.test(LA_CYCLIC) ||
            t - m_lanim_clr.m_lanim_start_time < m_lanim_clr.m_lanim->Length_sec())
        {
            int frame;
            u32 clr = m_lanim_clr.m_lanim->CalculateRGB(t - m_lanim_clr.m_lanim_start_time, frame);

            if (m_lanim_clr.m_lanimFlags.test(LA_TEXTURECOLOR))
            {
                if (m_lanim_clr.m_lanimFlags.test(LA_ONLYALPHA))
                    ColorAnimationSetTextureColor(color_get_A(clr), true);
                else
                    ColorAnimationSetTextureColor(clr, false);
            }
            if (m_lanim_clr.m_lanimFlags.test(LA_TEXTCOLOR))
            {
                if (m_lanim_clr.m_lanimFlags.test(LA_ONLYALPHA))
                    ColorAnimationSetTextColor(color_get_A(clr), true);
                else
                    ColorAnimationSetTextColor(clr, false);
            }
        }
    }
};

class XRUICORE_API CUIColorAnimConrollerContainer : public CUIWindow, public CUILightAnimColorConrollerImpl
{
    typedef CUIWindow inherited;

public:
    virtual void Update();
    virtual void ColorAnimationSetTextureColor(u32 color, bool only_alpha);
    virtual void ColorAnimationSetTextColor(u32 color, bool only_alpha);
};
