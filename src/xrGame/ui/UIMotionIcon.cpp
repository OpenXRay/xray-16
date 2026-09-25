#include "StdAfx.h"

#include "UIMotionIcon.h"

#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/ProgressBar/UIProgressShape.h"

#include "UIXmlInit.h"
#include "UIHelper.h"

constexpr pcstr MOTION_ICON_XML = "motion_icon.xml";

CUIMotionIcon::CUIMotionIcon() : CUIStatic("Motion Icon") {}

bool CUIMotionIcon::Init()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, MOTION_ICON_XML);

    const bool attachedToMinimap = CUIXmlInit::InitWindow(uiXml, "window", 0, this, false);
    if (attachedToMinimap)
        m_relative_size = uiXml.ReadAttribFlt("window", 0, "rel_size", 1.0f);
    else
    {
        CUIXmlInit::InitStatic(uiXml, "background", 0, this, false);
    }

    m_power_progress = UIHelper::CreateProgressBar(uiXml, "power_progress", this, false);

    // Initialization order matters, we should try progress bars first!!!
    m_luminosity_progress_bar = UIHelper::CreateProgressBar(uiXml, "luminosity_progress", this, false);
    m_noise_progress_bar = UIHelper::CreateProgressBar(uiXml, "noise_progress", this, false);

    // Allow only shape or bar, not both
    if (!m_luminosity_progress_bar)
        m_luminosity_progress_shape = UIHelper::CreateProgressShape(uiXml, "luminosity_progress", this, false);

    if (!m_noise_progress_bar)
        m_noise_progress_shape = UIHelper::CreateProgressShape(uiXml, "noise_progress", this, false);

    const auto create_static = [&uiXml, this](pcstr ui_path, EState state)
    {
        if (const auto ui_static = UIHelper::CreateStatic(uiXml, ui_path, this, false))
        {
            m_states[state] = ui_static;
            ui_static->Show(false);
        }
    };

    create_static("state_normal", stNormal);
    create_static("state_crouch", stCrouch);
    create_static("state_creep",  stCreep);
    create_static("state_climb",  stClimb);
    create_static("state_run",    stRun);
    create_static("state_sprint", stSprint);

    ShowState(stNormal);

    return attachedToMinimap;
}

void CUIMotionIcon::AttachToMinimap(const Frect& rect)
{
    Fvector2 sz{}, pos{};

    rect.getsize(sz);
    pos.set(sz.x / 2.0f, sz.y / 2.0f);

    SetWndSize(sz);
    SetWndPos(pos);

    const float k = UICore::get_current_kx();
    sz.mul(m_relative_size * k);

    if (m_luminosity_progress_shape)
    {
        m_luminosity_progress_shape->SetWndSize(sz);
        m_luminosity_progress_shape->SetWndPos(pos);
    }
    if (m_noise_progress_shape)
    {
        m_noise_progress_shape->SetWndSize(sz);
        m_noise_progress_shape->SetWndPos(pos);
    }
}

void CUIMotionIcon::ShowState(EState state)
{
    if (m_current_state == state)
        return;

    if (m_current_state != stLast)
    {
        if (CUIStatic* curState = m_states[m_current_state])
        {
            curState->Show(false);
            curState->Enable(false);
        }
    }

    if (CUIStatic* newState = m_states[state])
    {
        newState->Show(true);
        newState->Enable(true);
    }

    m_current_state = state;
}

void CUIMotionIcon::SetPower(float newPos)
{
    if (m_power_progress)
        m_power_progress->SetProgressPos(newPos);
}

void CUIMotionIcon::SetNoise(float newPos)
{
    if (!IsGameTypeSingle())
        return;

    if (m_noise_progress_shape)
    {
        float pos = newPos;
        pos = clampr(pos, 0.f, 100.f);
        m_noise_progress_shape->SetPos(pos / 100.f);
    }
    else if (m_noise_progress_bar)
    {
        float pos = newPos;
        pos = clampr(pos, m_noise_progress_bar->GetRange_min(), m_noise_progress_bar->GetRange_max());
        m_noise_progress_bar->SetProgressPos(pos);
    }
}

void CUIMotionIcon::SetLuminosity(float value, const bool absolute /*= true*/)
{
    if (!IsGameTypeSingle())
        return;

    if (!absolute)
    {
        if (m_luminosity_progress_shape)
        {
            clamp(value, 0.f, 1.f);
            value *= 100.f;
        }
        else if (m_luminosity_progress_bar)
        {
            const float v = float(m_luminosity_progress_bar->GetRange_max() - m_luminosity_progress_bar->GetRange_min());
            value *= v;
            value += m_luminosity_progress_bar->GetRange_min();
        }
    }

    if (m_luminosity_progress_shape)
        m_luminosity = value;
    else if (m_luminosity_progress_bar)
    {
        value = clampr(value, m_luminosity_progress_bar->GetRange_min(), m_luminosity_progress_bar->GetRange_max());
        m_luminosity = value;
    }
}

void CUIMotionIcon::Draw()
{
    if (!ClearSkyMode)
        inherited::Draw();
}

void CUIMotionIcon::Update()
{
    inherited::Update();

    if (m_luminosity_progress_shape)
    {
        if (m_cur_pos != m_luminosity)
        {
            const float _diff = _abs(m_luminosity - m_cur_pos);
            if (m_luminosity > m_cur_pos)
            {
                m_cur_pos += _diff * Device.fTimeDelta;
            }
            else
            {
                m_cur_pos -= _diff * Device.fTimeDelta;
            }
            clamp(m_cur_pos, 0.f, 100.f);
            // XXX: make it like progress bar so we can remove m_cur_pos
            m_luminosity_progress_shape->SetPos(m_cur_pos / 100.f);
        }
    }
    else if (m_luminosity_progress_bar)
    {
        const float len = m_luminosity_progress_bar->GetRange_max() - m_luminosity_progress_bar->GetRange_min();
        m_cur_pos = m_luminosity_progress_bar->GetProgressPos();
        if (m_cur_pos != m_luminosity)
        {
            const float _diff = _abs(m_luminosity - m_cur_pos);
            if (m_luminosity > m_cur_pos)
            {
                m_cur_pos += _min(len * Device.fTimeDelta, _diff);
            }
            else
            {
                m_cur_pos -= _min(len * Device.fTimeDelta, _diff);
            }
            clamp(m_cur_pos, m_luminosity_progress_bar->GetRange_min(), m_luminosity_progress_bar->GetRange_max());
            m_luminosity_progress_bar->SetProgressPos(m_cur_pos);
        }
    }
}
