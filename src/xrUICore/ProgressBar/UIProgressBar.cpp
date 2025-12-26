#include "pch.hpp"
#include "UIProgressBar.h"

CUIProgressBar::CUIProgressBar()
    : CUIWindow("CUIProgressBar"), m_UIProgressItem("Progress"), m_UIBackgroundItem("Background")
{
    Enable(false);

    AttachChild(&m_UIBackgroundItem);
    AttachChild(&m_UIProgressItem);
}

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
        if (m_bUseGradient)
        {
            Fcolor curr;
            if (m_bUseMiddleColor)
                curr.lerp(m_minColor, m_middleColor, m_maxColor, fCurrentLength);
            else
                curr.lerp(m_minColor, m_maxColor, fCurrentLength);
            m_UIProgressItem.SetTextureColor(curr.get());
        }
        else
            m_UIProgressItem.SetTextureColor(m_maxColor.get());
    }
}

void CUIProgressBar::SetProgressPos(float pos)
{
    m_ProgressPos.y = pos;
    clamp(m_ProgressPos.y, m_MinPos, m_MaxPos);
    UpdateProgressBar();
}

void CUIProgressBar::ForceSetProgressPos(float pos)
{
    clamp(pos, m_MinPos, m_MaxPos);
    m_ProgressPos = { pos, pos };
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
        float _val = _length * (1.0f - m_inertion) * Device.fTimeDelta / Device.time_factor();

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

    if (IsShownBackground())
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
}

void CUIProgressBar::FillDebugInfo()
{
#ifndef MASTER_GOLD
    CUIWindow::FillDebugInfo();

    if (!ImGui::CollapsingHeader(CUIProgressBar::GetDebugType()))
        return;

    ImGui::DragFloat2("Position (current vs destination)", reinterpret_cast<float*>(&m_ProgressPos));
    ImGui::DragFloat("Current length", &m_CurrentLength);

    bool update{};
    update |= ImGui::DragFloat("Min position", &m_MinPos);
    update |= ImGui::DragFloat("Max position", &m_MaxPos);
    update |= ImGui::DragFloat("Inertion", &m_inertion);
    ImGui::Separator();

    constexpr pcstr styles[om_count] =
    {
        "Horizontal (left to right)",
        "Vertical (bottom to top)",
        "Horizontal (right to left)",
        "Vertical (top to bottom)",
        "Horizontal (from center to sides)",
        "Vertical (from center to sides)",
    };
    int mode = m_orient_mode;
    if (ImGui::SliderInt("Mode", &mode, 0, om_count - 1, styles[m_orient_mode]))
    {
        m_orient_mode = (EOrientMode)mode;
        update |= true;
    }

    update |= ImGui::Button("Update");

    bool v = IsShownBackground();
    if (ImGui::Checkbox("Background", &v))
        ShowBackground(v);

    v = m_bUseColor;
    if (ImGui::Checkbox("Color", &v))
    {
        m_bUseColor = v;
        update |= true;
    }

    v = m_bUseMiddleColor;
    if (ImGui::Checkbox("Middle color", &v))
    {
        m_bUseMiddleColor = v;
        update |= true;
    }

    v = m_bUseGradient;
    if (ImGui::Checkbox("Gradient", &v))
    {
        m_bUseGradient = v;
        update |= true;
    }

    ImGui::Separator();

    update |= ImGui::ColorEdit4("Min color", reinterpret_cast<float*>(&m_minColor));
    update |= ImGui::ColorEdit4("Middle color", reinterpret_cast<float*>(&m_middleColor));
    update |= ImGui::ColorEdit4("Max color", reinterpret_cast<float*>(&m_maxColor));

    if (update)
        UpdateProgressBar();
#endif
}
