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

bool sortByOrder(CUITrackBar* A, CUITrackBar* B) { return A->GetOrder() < B->GetOrder(); }
bool sortByDisplayOrder(CUITrackBar* A, CUITrackBar* B) { return A->GetDisplayOrder() < B->GetDisplayOrder(); }

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

    m_default_display_modifier = 1.f;
}

bool CUIMultiTrackBar::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
    CUIWindow::OnMouseAction(x, y, mouse_action);

    bool didAction = false;

    for (int i = 0; i < childCount; i++)
    {
        auto slider = GetTrackBarAtIdx(i);
        if (slider->CursorOverWindow())
        {
            didAction = slider->OnMouseAction(x, y, mouse_action);
        }
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
    xr_vector<pcstr> mapping = { "x", "y", "z", "w" };

    if (dataType == SDT_Fvector3)
        initialData = { initialData2.x, initialData2.y, initialData2.z, 0.f};

    m_f_opt_backup_value = initialData;
    m_f_val = initialData;

    int titleRow = m_static->IsEnabled();
    int rowCount = childCount;
    if (titleRow)
        rowCount += 1;

    for (int i = 0; i < childCount; i++)
    {
        auto slider = xr_new<CUITrackBar>();
        AttachChild(slider);
        slider->SetAutoDelete(true);
        m_pSliders->emplace_back(slider);

        ChildTrackBarData sliderData = trackBarData[i];

        slider->SetType(true); // hardcoded to float for now
        slider->SetInvert(!!sliderData.isInverted);
        slider->SetStep(sliderData.step);
        slider->SetOptFBounds(sliderData.min, sliderData.max);
        slider->SetBoundReady(true);
        slider->SetOrder(i);
        slider->SetDisplayOrder(sliderData.displayOrder);
        slider->SetDisplayLabel(sliderData.displayLabel);
        slider->m_f_opt_backup_value = sliderData.defaultValue;

        auto value = m_f_val[i];
        slider->SetFValue(m_f_val[i]);
        slider->UpdatePos();
        slider->OnChangedOptValue();

        string128 childEntry;
        xr_sprintf(childEntry, "%s.%s", m_entry.c_str(), mapping[i]);
        slider->m_entry = childEntry;

        float childDisplayModifier = sliderData.displayScale;
        slider->SetDisplayModifier(childDisplayModifier);
    }

    std::sort(m_pSliders->begin(), m_pSliders->end(), sortByDisplayOrder);

    for (int i = 0; i < childCount; i++)
    {
        auto slider = GetTrackBarAtIdx(i);
        Fvector2 newPos;
        newPos.set(pos.x, (i + titleRow) * GetHeight());
        slider->InitTrackBar(newPos, { size.x / 2.f, size.y });
    }


#ifdef DEBUG
    Msg("UI DEBUG - final size of MultiTrackBar %s = %3.3f", m_entry.c_str(), m_wndSize.y * rowCount);
#endif

    m_wndPos = pos;
    m_wndSize = { m_wndSize.x, m_wndSize.y * rowCount };
    InitIB(m_wndPos, m_wndSize);
}

void CUIMultiTrackBar::Draw()
{
    CUI_IB_FrameLineWnd::Draw();
    for (auto m_pSlider : *m_pSliders)
    {
        m_pSlider->Draw();
        m_pSlider->m_static->Draw();
    }

    m_static->Draw();
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
    std::sort(m_pSliders->begin(), m_pSliders->end(), sortByOrder);
    
    for (int i = 0; i < childCount; i++)
    {
        auto slider = GetTrackBarAtIdx(i);
        m_f_val[i] = slider->GetFValue();
    }

    std::sort(m_pSliders->begin(), m_pSliders->end(), sortByDisplayOrder);
}

void CUIMultiTrackBar::SaveOptValue()
{
    std::sort(m_pSliders->begin(), m_pSliders->end(), sortByOrder);

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

    std::sort(m_pSliders->begin(), m_pSliders->end(), sortByDisplayOrder);
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
    CUIOptionsItem::UndoOptValue();
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

void CUIMultiTrackBar::OnMessage(LPCSTR message)
{
    if (0 == xr_strcmp(message, "set_default_value"))
    {
        for (int i = 0; i < childCount; i++)
        {
            auto slider = GetTrackBarAtIdx(i);
            slider->SetFValue(slider->m_f_opt_backup_value);
            slider->UpdatePos();
        }
    }
}

void CUIMultiTrackBar::UpdatePos()
{
    for (int i = 0; i < childCount; i++)
    {
        auto slider = GetTrackBarAtIdx(i);
        slider->UpdatePos();
    }
}
