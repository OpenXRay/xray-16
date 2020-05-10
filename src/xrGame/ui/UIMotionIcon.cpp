#include "StdAfx.h"
#include "UIMainIngameWnd.h"
#include "UIMotionIcon.h"
#include "UIXmlInit.h"
#include "UIHelper.h"

constexpr pcstr MOTION_ICON_XML = "motion_icon.xml";

CUIMotionIcon* g_pMotionIcon = nullptr;

CUIMotionIcon::CUIMotionIcon()
{
    m_current_state = stLast;
    g_pMotionIcon = this;
    m_bchanged = true;
    m_luminosity = 0.0f;
    m_cur_pos = 0.f;

    m_power_progress = nullptr;
    m_luminosity_progress_bar = nullptr;
    m_noise_progress_bar = nullptr;
    m_luminosity_progress_shape = nullptr;
    m_noise_progress_shape = nullptr;
}

CUIMotionIcon::~CUIMotionIcon() { g_pMotionIcon = nullptr; }
void CUIMotionIcon::ResetVisibility()
{
    m_npc_visibility.clear();
    m_bchanged = true;
}

bool CUIMotionIcon::Init(Frect const& zonemap_rect)
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, MOTION_ICON_XML);

    bool independent = false; // Not bound to minimap
    if (!CUIXmlInit::InitWindow(uiXml, "window", 0, this, false))
    {
        independent = CUIXmlInit::InitStatic(uiXml, "background", 0, this, false);
    }

    Fvector2 sz;
    Fvector2 pos;

    if (!independent)
    {
        const float rel_sz = uiXml.ReadAttribFlt("window", 0, "rel_size", 1.0f);
        
        zonemap_rect.getsize(sz);
        pos.set(sz.x / 2.0f, sz.y / 2.0f);
        
        SetWndSize(sz);
        SetWndPos(pos);

        const float k = UICore::get_current_kx();
        sz.mul(rel_sz * k);
    }

    m_power_progress = UIHelper::CreateProgressBar(uiXml, "power_progress", this, false);

    // Initialization order matters, we should try progress bars first!!!
    m_luminosity_progress_bar = UIHelper::CreateProgressBar(uiXml, "luminosity_progress", this, false);
    m_noise_progress_bar = UIHelper::CreateProgressBar(uiXml, "noise_progress", this, false);

    // Allow only shape or bar, not both
    if (!m_luminosity_progress_bar)
    {
        m_luminosity_progress_shape = UIHelper::CreateProgressShape(uiXml, "luminosity_progress", this, false);
        if (m_luminosity_progress_shape && !independent)
        {
            m_luminosity_progress_shape->SetWndSize(sz);
            m_luminosity_progress_shape->SetWndPos(pos);
        }
    }
    
    if (!m_noise_progress_bar)
    {
        m_noise_progress_shape = UIHelper::CreateProgressShape(uiXml, "noise_progress", this, false);
        if (m_noise_progress_shape && !independent)
        {
            m_noise_progress_shape->SetWndSize(sz);
            m_noise_progress_shape->SetWndPos(pos);
        }
    }

    CUIStatic* state;

    if ((state = UIHelper::CreateStatic(uiXml, "state_normal", this, false)))
    {
        m_states[stNormal] = state;
        state->Show(false);
    }

    if ((state = UIHelper::CreateStatic(uiXml, "state_crouch", this, false)))
    {
        m_states[stCrouch] = state;
        state->Show(false);
    }

    if ((state = UIHelper::CreateStatic(uiXml, "state_creep", this, false)))
    {
        m_states[stCreep] = state;
        state->Show(false);
    }

    if ((state = UIHelper::CreateStatic(uiXml, "state_climb", this, false)))
    {
        m_states[stClimb] = state;
        state->Show(false);
    }

    if ((state = UIHelper::CreateStatic(uiXml, "state_run", this, false)))
    {
        m_states[stRun] = state;
        state->Show(false);
    }

    if ((state = UIHelper::CreateStatic(uiXml, "state_sprint", this, false)))
    {
        m_states[stSprint] = state;
        state->Show(false);
    }

    ShowState(stNormal);

    return independent;
}

void CUIMotionIcon::ShowState(EState state)
{
    if (m_current_state == state)
        return;

    if (m_current_state != stLast)
    {
        CUIStatic* curState = m_states[m_current_state];
        if (curState)
        {
            curState->Show(false);
            curState->Enable(false);
        }
    }

    CUIStatic* newState = m_states[state];
    if (newState)
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

void CUIMotionIcon::SetLuminosity(float newPos)
{
    if (!IsGameTypeSingle())
        return;

    if (m_luminosity_progress_shape)
        m_luminosity = newPos;
    else if (m_luminosity_progress_bar)
    {
        newPos = clampr(newPos, m_luminosity_progress_bar->GetRange_min(), m_luminosity_progress_bar->GetRange_max());
        m_luminosity = newPos;
    }
}

void CUIMotionIcon::Draw()
{
    if (!ClearSkyMode)
        inherited::Draw();
}

void CUIMotionIcon::Update()
{
    if (!IsGameTypeSingle())
    {
        inherited::Update();
        return;
    }
    if (m_bchanged)
    {
        m_bchanged = false;
        if (!m_npc_visibility.empty())
        {
            std::sort(m_npc_visibility.begin(), m_npc_visibility.end());
            SetLuminosity(m_npc_visibility.back().value);
        }
        else
            SetLuminosity(0.f);
    }
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

void SetActorVisibility(u16 who_id, float value)
{
    if (!IsGameTypeSingle())
        return;

    if (g_pMotionIcon)
        g_pMotionIcon->SetActorVisibility(who_id, value);
}

void CUIMotionIcon::SetActorVisibility(u16 who_id, float value)
{
    if (m_luminosity_progress_shape)
    {
        clamp(value, 0.f, 1.f);
        value *= 100.f;
    }
    else if (m_luminosity_progress_bar)
    {
        float v = float(m_luminosity_progress_bar->GetRange_max() - m_luminosity_progress_bar->GetRange_min());
        value *= v;
        value += m_luminosity_progress_bar->GetRange_min();
    }

    auto it = std::find(m_npc_visibility.begin(), m_npc_visibility.end(), who_id);

    if (it == m_npc_visibility.end() && value != 0)
    {
        m_npc_visibility.resize(m_npc_visibility.size() + 1);
        _npc_visibility& v = m_npc_visibility.back();
        v.id = who_id;
        v.value = value;
    }
    else if (fis_zero(value))
    {
        if (it != m_npc_visibility.end())
            m_npc_visibility.erase(it);
    }
    else
    {
        (*it).value = value;
    }

    m_bchanged = true;
}
