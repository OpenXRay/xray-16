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
    : m_b_invert(false), m_b_is_float(true), m_b_bound_already_set(false)
{
    Fvector4 zero{ 0.f, 0.f, 0.f, 0.f };
    m_f_val = zero;
    m_f_opt_backup_value = zero;
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

    for (int i = 0; i < childCount; i++)
    {
        auto slider = GetTrackBarAtIdx(i);
        didAction = slider->OnMouseAction(x, y, mouse_action);
    }

    if (didAction)
    {
        SetCurrentOptValue();
    }

    return didAction;
}

void CUIMultiTrackBar::InitTrackBars(Fvector2 pos, Fvector2 size, xr_vector<CUIMultiTrackBar::ChildTrackBarData>& trackBarData)
{
    auto dataType = GetSaveDataType();
    Fvector4 initialData = GetOptVector4Value();
    Fvector3 initialData2 = GetOptVector3Value();

    Msg("Yohji debug - checking data 1 %s %3.3f %3.3f %3.3f %3.3f", m_entry.c_str(), initialData.x, initialData.y, initialData.z, initialData.w);
    Msg("Yohji debug - checking data 1 %s %3.3f %3.3f %3.3f", m_entry.c_str(), initialData2.x, initialData2.y, initialData2.z);


    if (dataType == SDT_Fvector3)
        initialData = { initialData2.x, initialData2.y, initialData2.z, 0.f};

    m_f_opt_backup_value = initialData;
    m_f_val = initialData;

    for (int i = 0; i < childCount; i++)
    {
        auto slider = xr_new<CUITrackBar>();
        AttachChild(slider);
        slider->SetAutoDelete(true);
        m_pSliders->emplace_back(slider);

        auto sliderData = trackBarData[i];

        slider->SetType(true); // hardcoded to float for now
        slider->SetInvert(!!sliderData.isInverted);
        slider->SetStep(sliderData.step);
        slider->SetOptFBounds(sliderData.min, sliderData.max);
        slider->SetBoundReady(true);

        Fvector2 newPos;
        newPos.set(0.f, i * GetHeight());
        slider->InitTrackBar(newPos, size);

        auto value = m_f_val[i];
        slider->SetFValue(m_f_val[i]);
        slider->UpdatePos();
        slider->OnChangedOptValue();

        Msg("Yohji debug - checking data 3 in slider %s %3.3f", m_entry.c_str(), slider->GetFValue());
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
    for (int i = 0; i < childCount; i++)
    {
        auto slider = GetTrackBarAtIdx(i);
        m_f_val[i] = slider->GetFValue();
    }

    Msg("Yohji debug - Set entry %s to value %3.3f %3.3f %3.3f %3.3f", m_entry.c_str(), m_f_val.x, m_f_val.y, m_f_val.z, m_f_val.w);

    UpdatePos();
}

void CUIMultiTrackBar::SaveOptValue()
{
    auto dataType = GetSaveDataType();
    switch (dataType)
    {
    case SDT_Fvector3:
        SaveOptVector3Value(Fvector3{ m_f_val.x, m_f_val.y, m_f_val.z });
        break;
    case SDT_Fvector4:
        SaveOptVector4Value(m_f_val);
        break;
    }

    Msg("Yohji debug - Save entry %s to value %3.3f %3.3f %3.3f %3.3f", m_entry.c_str(), m_f_val.x, m_f_val.y, m_f_val.z, m_f_val.w);
}

bool CUIMultiTrackBar::IsChangedOptValue() const
{
    bool isChanged = !fsimilar(m_f_opt_backup_value.x, m_f_val.x) || !fsimilar(m_f_opt_backup_value.y, m_f_val.y) || !fsimilar(m_f_opt_backup_value.z, m_f_val.z);
    return isChanged;
}

void CUIMultiTrackBar::SaveBackUpOptValue()
{
    m_f_opt_backup_value = m_f_val;
}

void CUIMultiTrackBar::UndoOptValue()
{
    m_f_val = m_f_opt_backup_value;

    UpdatePos();
    CUIOptionsItem::UndoOptValue();
}

void CUIMultiTrackBar::SetStep(float step)
{
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

    UpdatePos();
    OnChangedOptValue();
}

void CUIMultiTrackBar::UpdatePos()
{
    if (m_pSliders->empty())
        return;

}

void CUIMultiTrackBar::OnMessage(LPCSTR message)
{
    if (0 == xr_strcmp(message, "set_default_value"))
    {
        for (int i = 0; i < childCount; i++)
        {
            auto slider = GetTrackBarAtIdx(i);
            slider->SetFValue(m_f_val[i]);
            slider->UpdatePos();
        }
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
}

void CUIMultiTrackBar::SetOptFBounds(float fmin, float fmax)
{
}
