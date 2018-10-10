//=============================================================================
//  Красивый текстовый баннер с множеством стилей
//=============================================================================

#include "StdAfx.h"
#include "UITextBanner.h"

CUITextBanner::CUITextBanner()
    : m_bAnimate(true), m_Cl(0xffffffff), m_pFont(NULL), fontSize(-1.0f), aligment(CGameFont::alLeft)
{
}

////////////////////////////////////////////////////////////////////////////////

CUITextBanner::~CUITextBanner() {}
////////////////////////////////////////////////////////////////////////////////

EffectParams* CUITextBanner::SetStyleParams(const TextBannerStyles styleName)
{
    if (tbsNone == styleName)
    {
        if (!m_StyleParams.empty())
            m_StyleParams.clear();
        return NULL;
    }

    return &m_StyleParams[styleName];
}

////////////////////////////////////////////////////////////////////////////////

void CUITextBanner::Update()
{
    StyleParams_it it = m_StyleParams.begin();

    // Если анимация включена
    if (m_bAnimate)
    {
        for (; it != m_StyleParams.end(); ++it)
            if (it->second.bOn)
                it->second.fTimePassed += Device.fTimeDelta;
    }
}

////////////////////////////////////////////////////////////////////////////////

void CUITextBanner::Out(float x, float y, const char* fmt, ...)
{
    if (!fmt)
        return;

    StyleParams_it it = m_StyleParams.begin();

    // Применяем эффекты
    for (; it != m_StyleParams.end(); ++it)
    {
        // Fade effect
        if (it->first & tbsFade)
        {
            EffectFade();
        }
        // Flicker effect
        if (it->first & tbsFlicker)
        {
            EffectFlicker();
        }
    }

    va_list Print;
    string512 msg;
    xr_string buf;

    va_start(Print, fmt);
    vsprintf_s(msg, fmt, Print);
    va_end(Print);

    buf += msg;

    R_ASSERT(m_pFont);
    m_pFont->SetColor(m_Cl);
    m_pFont->SetAligment(aligment);
    //	if(fontSize>0.0f)
    //		m_pFont->SetHeight(fontSize);

    Fvector2 pos;
    UI()->ClientToScreenScaled(pos, x, y);
    m_pFont->Out(pos.x, pos.y, "%s", buf.c_str());
}

////////////////////////////////////////////////////////////////////////////////

void CUITextBanner::EffectFade()
{
    EffectParams& fade = m_StyleParams[tbsFade];

    // Проверям включена ли анимация
    if (!fade.bOn)
        return;

    // Если пришло время сменить направление фейда
    if (fade.fTimePassed > fade.fPeriod)
    {
        if (!fade.bCyclic)
        {
            fade.bOn = false;
            return;
        }

        if (0 == fade.iEffectStage)
            fade.iEffectStage = 1;
        else
            fade.iEffectStage = 0;
        fade.fTimePassed = 0;
    }

    if (fade.iEffectStage)
    {
        m_Cl = subst_alpha(GetTextColor(), u8(iFloor(255.f * (fade.fTimePassed / fade.fPeriod))));
    }
    else
    {
        m_Cl = subst_alpha(GetTextColor(), u8(iFloor(255.f * (1 - (fade.fTimePassed / fade.fPeriod)))));
    }
}

//////////////////////////////////////////////////////////////////////////

void CUITextBanner::EffectFlicker()
{
    EffectParams& flicker = m_StyleParams[tbsFlicker];

    // Проверям включена ли анимация
    if (!flicker.bOn)
        return;

    // Если пришло время, показать/спрятать надпись
    if (flicker.fTimePassed > flicker.fPeriod)
    {
        if (!flicker.bCyclic)
        {
            flicker.bOn = false;
            return;
        }

        if (0 == flicker.iEffectStage)
            flicker.iEffectStage = 1;
        else
            flicker.iEffectStage = 0;
        flicker.fTimePassed = 0;
    }

    if (flicker.iEffectStage)
    {
        m_Cl = subst_alpha(GetTextColor(), 0);
    }
    else
    {
        m_Cl = subst_alpha(GetTextColor(), 255);
    }
}

//////////////////////////////////////////////////////////////////////////

void CUITextBanner::SetTextColor(u32 cl) { m_Cl = cl; }
//////////////////////////////////////////////////////////////////////////

u32 CUITextBanner::GetTextColor()
{
    void SetTextColor(u32 cl);
    u32 GetTextColor();

    return m_Cl;
}

//////////////////////////////////////////////////////////////////////////

void CUITextBanner::ResetAnimation(const TextBannerStyles styleName) { m_StyleParams[styleName].fTimePassed = 0; }
