#include "pch.hpp"
#include "UIProgressBar.h"

CUIProgressBar::CUIProgressBar(void)
{
    m_MinPos = 1.0f;
    m_MaxPos = 1.0f + EPS;

    Enable(false);

    m_bBackgroundPresent = false;
    m_bUseColor = false;
    m_bUseGradient = true;

    AttachChild(&m_UIBackgroundItem);
    AttachChild(&m_UIProgressItem);
    m_ProgressPos.x = 0.0f;
    m_ProgressPos.y = 0.0f;
    m_inertion = 0.0f;
    m_last_render_frame = u32(-1);
    m_orient_mode = om_horz;
}

CUIProgressBar::~CUIProgressBar(void) {}
void CUIProgressBar::InitProgressBar(Fvector2 pos, Fvector2 size, EOrientMode mode)
{
    m_orient_mode = mode;
    SetWndPos(pos);
    SetWndSize(size);
    UpdateProgressBar();
}

void CUIProgressBar::UpdateProgressBar()
{
    if (fsimilar(m_MaxPos, m_MinPos))
        m_MaxPos += EPS;

    float progressbar_unit = 1 / (m_MaxPos - m_MinPos);

    float fCurrentLength = m_ProgressPos.x * progressbar_unit;

    switch (m_orient_mode)
    {
    case om_horz:
    case om_back:
    case om_fromcenter:
        m_CurrentLength = GetWidth() * fCurrentLength;
        break;
    case om_vert:
    case om_down:
    case om_vfromcenter:
        m_CurrentLength = GetHeight() * fCurrentLength;
        break;
    default:
        m_CurrentLength = 0.0f;
    }

    if (m_bUseColor)
    {
        if ( m_bUseGradient )
        {
            Fcolor curr;
            curr.lerp(m_minColor, m_middleColor, m_maxColor, fCurrentLength);
            m_UIProgressItem.SetTextureColor(curr.get());
        }
        else
            m_UIProgressItem.SetTextureColor(m_maxColor.get());

        // XXX: Implement color smoothing
        if (colorSmoothing)
            R_ASSERT2(false, "color smoothing is not implemented.");
    }
}

void CUIProgressBar::SetProgressPos(float _Pos)
{
    m_ProgressPos.y = _Pos;
    clamp(m_ProgressPos.y, m_MinPos, m_MaxPos);
    UpdateProgressBar();
}

float _sign(const float& v) { return (v > 0.0f) ? +1.0f : -1.0f; }
void CUIProgressBar::Update()
{
    inherited::Update();
    if (!fsimilar(m_ProgressPos.x, m_ProgressPos.y))
    {
        if (fsimilar(m_MaxPos, m_MinPos))
            m_MaxPos += EPS; // hack ^(
        float _diff = m_ProgressPos.y - m_ProgressPos.x;

        float _length = (m_MaxPos - m_MinPos);
        float _val = _length * (1.0f - m_inertion) * Device.fTimeDelta;

        _val = _min(_abs(_val), _abs(_diff));
        _val *= _sign(_diff);
        m_ProgressPos.x += _val;
        UpdateProgressBar();
    }
}

void CUIProgressBar::Draw()
{
    Frect rect;
    GetAbsoluteRect(rect);

    if (m_bBackgroundPresent)
    {
        UI().PushScissor(rect);
        m_UIBackgroundItem.Draw();
        UI().PopScissor();
    }

    Frect progress_rect;

    switch (m_orient_mode)
    {
    case om_horz: progress_rect.set(0, 0, m_CurrentLength, GetHeight()); break;
    case om_vert: progress_rect.set(0, GetHeight() - m_CurrentLength, GetWidth(), GetHeight()); break;
    case om_back: progress_rect.set(GetWidth() - m_CurrentLength * 1.01f, 0, GetWidth(), GetHeight()); break;
    case om_down: progress_rect.set(0, 0, GetWidth(), m_CurrentLength); break;
    case om_fromcenter:
    {
        const float center = GetWidth() / 2.f;
        progress_rect.set(center - m_CurrentLength, 0, center + m_CurrentLength, GetHeight());
        break;
    }
    case om_vfromcenter:
    {
        const float center = GetHeight() / 2.f;
        progress_rect.set(0, center - m_CurrentLength, GetWidth(), center + m_CurrentLength);
        break;
    }
    default: NODEFAULT; break;
    }

    if (m_CurrentLength > 0)
    {
        Fvector2 pos = m_UIProgressItem.GetWndPos();
        progress_rect.add(rect.left + pos.x, rect.top + pos.y);

        UI().PushScissor(progress_rect);
        m_UIProgressItem.Draw();
        UI().PopScissor();
    }
    m_last_render_frame = Device.dwFrame;
}
