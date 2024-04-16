#include "pch.hpp"
#include "UIMultiTrackBar.h"
#include "Buttons/UI3tButton.h"
#include "XML/UITextureMaster.h"
#include "xrEngine/xr_input.h"

#include "Common/object_broker.h"

#include "TrackBar/UITrackBar.h"

constexpr pcstr UI_SLIDER_BAR = "ui_inGame2_opt_slider_bar";

constexpr pcstr UI_SLIDER_ENABLED = "ui_slider_e";
constexpr pcstr UI_SLIDER_DISABLED = "ui_slider_d";

constexpr pcstr SLIDER_BOX_TEXTURE = "ui_inGame2_opt_slider_box";
constexpr pcstr SLIDER_BOX_TEXTURE_E = "ui_inGame2_opt_slider_box_e";

constexpr pcstr SLIDER_BUTTON_TEXTURE = "ui_slider_button";
constexpr pcstr SLIDER_BUTTON_TEXTURE_E = "ui_slider_button_e";

CUIMultiTrackBar::CUIMultiTrackBar()
    : m_b_invert(false), m_b_is_float(true), m_b_bound_already_set(false), m_f_val(0), m_f_max(1), m_f_min(0),
    m_f_step(0.01f), m_f_opt_backup_value(0)
{
    m_pSliders = xr_new<xr_vector<CUITrackBar*>>();

    m_static = xr_new<CUIStatic>("Value as text");
    m_static->Enable(false);
    AttachChild(m_static);
    m_static->SetAutoDelete(true);

    m_b_mouse_capturer = false;
}

bool CUIMultiTrackBar::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    CUIWindow::OnMouseAction(x, y, mouse_action);

    bool didAction = false;

    for (auto slider : *m_pSliders)
    {
        didAction = slider->OnMouseAction(x, y, mouse_action);
    }

    return didAction;
}

void CUIMultiTrackBar::InitTrackBar(Fvector2 pos, Fvector2 size)
{
    for (int i = 0; i < childCount; i++)
    {
        auto slider = xr_new<CUITrackBar>();
        AttachChild(slider);
        slider->SetAutoDelete(true);
        m_pSliders->emplace_back(slider);

        Fvector2 newPos;
        newPos.set(0.f, i * GetHeight());
        slider->InitTrackBar(newPos, size);
    }

    m_wndSize = { m_wndSize.x, m_wndSize.y * childCount };
}

void CUIMultiTrackBar::Draw()
{
    CUI_IB_FrameLineWnd::Draw();
    for (auto m_pSlider : *m_pSliders)
    {
        m_pSlider->Draw();
        m_pSlider->m_static->Draw();
    }
}

void CUIMultiTrackBar::Update()
{
    CUIWindow::Update();

    if (m_b_mouse_capturer)
    {
        if (!pInput->iGetAsyncKeyState(MOUSE_1))
            m_b_mouse_capturer = false;
    }
}

void CUIMultiTrackBar::SetCurrentOptValue()
{
    if (m_b_is_float)
    {
        float fake_min, fake_max;
        if (!m_b_bound_already_set)
            GetOptFloatValue(m_f_val, m_f_min, m_f_max);
        else
            GetOptFloatValue(m_f_val, fake_min, fake_max);
    }
    else
    {
        int fake_min, fake_max;
        if (!m_b_bound_already_set)
            GetOptIntegerValue(m_i_val, m_i_min, m_i_max);
        else
            GetOptIntegerValue(m_i_val, fake_min, fake_max);
    }

    UpdatePos();
}

void CUIMultiTrackBar::SaveOptValue()
{
    CUIOptionsItem::SaveOptValue();
    if (m_b_is_float)
        SaveOptFloatValue(m_f_val);
    else
        SaveOptIntegerValue(m_i_val);
}

bool CUIMultiTrackBar::IsChangedOptValue() const
{
    if (m_b_is_float)
    {
        return !fsimilar(m_f_opt_backup_value, m_f_val);
    }
    else
    {
        return (m_i_opt_backup_value != m_i_val);
    }
}

void CUIMultiTrackBar::SaveBackUpOptValue()
{
    if (m_b_is_float)
        m_f_opt_backup_value = m_f_val;
    else
        m_i_opt_backup_value = m_i_val;
}

void CUIMultiTrackBar::UndoOptValue()
{
    if (m_b_is_float)
        m_f_val = m_f_opt_backup_value;
    else
        m_i_val = m_i_opt_backup_value;

    UpdatePos();
    CUIOptionsItem::UndoOptValue();
}

void CUIMultiTrackBar::SetStep(float step)
{
    if (m_b_is_float)
        m_f_step = step;
    else
        m_i_step = iFloor(step);
}

void CUIMultiTrackBar::Enable(bool status)
{
    m_bIsEnabled = status;
    SetCurrentState(m_bIsEnabled ? S_Enabled : S_Disabled);

    for (auto m_pSlider : *m_pSliders)
    {
        m_pSlider->Enable(m_bIsEnabled);
    }
}

void CUIMultiTrackBar::UpdatePosRelativeToMouse()
{
    if (m_pSliders->empty())
        return;

    float _bkf = 0.0f;
    int _bki = 0;
    if (m_b_is_float)
    {
        _bkf = m_f_val;
    }
    else
    {
        _bki = m_i_val;
    }

    float btn_width = m_pSliders->front()->GetWidth();
    float window_width = GetWidth();
    float fpos = cursor_pos.x;

    if (GetInvert())
        fpos = window_width - fpos;

    if (fpos < btn_width / 2)
        fpos = btn_width / 2;
    else if (fpos > window_width - btn_width / 2)
        fpos = window_width - btn_width / 2;

    const float fmax = (m_b_is_float) ? m_f_max : (float)m_i_max;
    const float fmin = (m_b_is_float) ? m_f_min : (float)m_i_min;
    const float fstep = (m_b_is_float) ? m_f_step : (float)m_i_step;

    float fval = (fmax - fmin) * (fpos - btn_width / 2) / (window_width - btn_width) + fmin;

    const float d = (fval - fmin);

    const float val = d / fstep;
    const int vi = iFloor(val);
    float vf = fstep * vi;

    if (d - vf > fstep / 2.0f)
        vf += fstep;

    fval = fmin + vf;

    clamp(fval, fmin, fmax);

    if (m_b_is_float)
        m_f_val = fval;
    else
        m_i_val = iFloor(fval);

    bool b_ch = false;
    if (m_b_is_float)
    {
        b_ch = !fsimilar(_bkf, m_f_val);
    }
    else
    {
        b_ch = (_bki != m_i_val);
    }

    if (b_ch)
        GetMessageTarget()->SendMessage(this, BUTTON_CLICKED, NULL);

    UpdatePos();
    OnChangedOptValue();
}

void CUIMultiTrackBar::UpdatePos()
{
    if (m_pSliders->empty())
        return;

#ifdef DEBUG

    if (m_b_is_float)
        R_ASSERT2(
            m_f_val >= m_f_min && m_f_val <= m_f_max, "CUIMultiTrackBar::UpdatePos() - m_val >= m_min && m_val <= m_max");
    else
        R_ASSERT2(
            m_i_val >= m_i_min && m_i_val <= m_i_max, "CUIMultiTrackBar::UpdatePos() - m_val >= m_min && m_val <= m_max");

#endif

    float btn_width = m_pSliders->front()->GetWidth();
    float window_width = GetWidth();
    float free_space = window_width - btn_width;
    Fvector2 pos = m_pSliders->front()->GetWndPos();

    const float fval = (m_b_is_float) ? m_f_val : (float)m_i_val;
    const float fmax = (m_b_is_float) ? m_f_max : (float)m_i_max;
    const float fmin = (m_b_is_float) ? m_f_min : (float)m_i_min;

    pos.x = (fval - fmin) * free_space / (fmax - fmin);
    if (GetInvert())
        pos.x = free_space - pos.x;

    m_pSliders->front()->SetWndPos(pos);

    if (m_static->IsEnabled())
    {
        string256 buff;
        if (m_b_is_float)
        {
            xr_sprintf(buff, (m_static_format == nullptr ? "%.1f" : m_static_format.c_str()), m_f_val);
        }
        else
        {
            xr_sprintf(buff, (m_static_format == nullptr ? "%d" : m_static_format.c_str()), m_i_val);
        }
        m_static->TextItemControl()->SetTextST(buff);
    }
}

void CUIMultiTrackBar::OnMessage(LPCSTR message)
{
    if (0 == xr_strcmp(message, "set_default_value"))
    {
        if (m_b_is_float)
            m_f_val = m_f_min + (m_f_max - m_f_min) / 2.0f;
        else
            m_i_val = m_i_min + iFloor((m_i_max - m_i_min) / 2.0f);

        UpdatePos();
    }
}

bool CUIMultiTrackBar::GetCheck()
{
    VERIFY(!m_b_is_float);
    return !!m_i_val;
}

void CUIMultiTrackBar::SetCheck(bool b)
{
    VERIFY(!m_b_is_float);
    m_i_val = (b) ? m_i_max : m_i_min;
}

void CUIMultiTrackBar::SetOptIBounds(int imin, int imax)
{
    m_i_min = imin;
    m_i_max = imax;
    if (m_i_val < m_i_min || m_i_val > m_i_max)
    {
        clamp(m_i_val, m_i_min, m_i_max);
        OnChangedOptValue();
    }
}

void CUIMultiTrackBar::SetOptFBounds(float fmin, float fmax)
{
    m_f_min = fmin;
    m_f_max = fmax;
    if (m_f_val < m_f_min || m_f_val > m_f_max)
    {
        clamp(m_f_val, m_f_min, m_f_max);
        OnChangedOptValue();
    }
}
