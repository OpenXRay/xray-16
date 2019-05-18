#include "StdAfx.h"
#include "UIMainIngameWnd.h"
#include "UIMotionIcon.h"
#include "UIXmlInit.h"
const LPCSTR MOTION_ICON_XML = "motion_icon.xml";

CUIMotionIcon* g_pMotionIcon = NULL;

CUIMotionIcon::CUIMotionIcon()
{
    g_pMotionIcon = this;
    m_bchanged = true;
    m_luminosity = 0.0f;
    cur_pos = 0.f;
}

CUIMotionIcon::~CUIMotionIcon() { g_pMotionIcon = NULL; }
void CUIMotionIcon::ResetVisibility()
{
    m_npc_visibility.clear();
    m_bchanged = true;
}

void CUIMotionIcon::Init(Frect const& zonemap_rect)
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, MOTION_ICON_XML);

    CUIXmlInit::InitWindow(uiXml, "window", 0, this);
    float rel_sz = uiXml.ReadAttribFlt("window", 0, "rel_size", 1.0f);
    Fvector2 sz;
    Fvector2 pos;
    zonemap_rect.getsize(sz);

    pos.set(sz.x / 2.0f, sz.y / 2.0f);
    SetWndSize(sz);
    SetWndPos(pos);

    float k = UI().get_current_kx();
    sz.mul(rel_sz * k);

    // float h = Device.dwHeight;
    // float w = Device.dwWidth;
    AttachChild(&m_luminosity_progress);
    CUIXmlInit::InitProgressShape(uiXml, "luminosity_progress", 0, &m_luminosity_progress);
    m_luminosity_progress.SetWndSize(sz);
    m_luminosity_progress.SetWndPos(pos);

    AttachChild(&m_noise_progress);
    CUIXmlInit::InitProgressShape(uiXml, "noise_progress", 0, &m_noise_progress);
    m_noise_progress.SetWndSize(sz);
    m_noise_progress.SetWndPos(pos);
}

void CUIMotionIcon::SetNoise(float Pos)
{
    if (!IsGameTypeSingle())
        return;

    Pos = clampr(Pos, 0.f, 100.f);
    m_noise_progress.SetPos(Pos / 100.f);
}

void CUIMotionIcon::SetLuminosity(float Pos)
{
    if (!IsGameTypeSingle())
        return;

    m_luminosity = Pos;
}

void CUIMotionIcon::Draw() { inherited::Draw(); }
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
        if (m_npc_visibility.size())
        {
            std::sort(m_npc_visibility.begin(), m_npc_visibility.end());
            SetLuminosity(m_npc_visibility.back().value);
        }
        else
            SetLuminosity(0.f);
    }
    inherited::Update();

    // m_luminosity_progress
    if (cur_pos != m_luminosity)
    {
        float _diff = _abs(m_luminosity - cur_pos);
        if (m_luminosity > cur_pos)
        {
            cur_pos += _diff * Device.fTimeDelta;
        }
        else
        {
            cur_pos -= _diff * Device.fTimeDelta;
        }
        clamp(cur_pos, 0.f, 100.f);
        m_luminosity_progress.SetPos(cur_pos / 100.f);
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
    clamp(value, 0.f, 1.f);
    value *= 100.f;

    xr_vector<_npc_visibility>::iterator it = std::find(m_npc_visibility.begin(), m_npc_visibility.end(), who_id);

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
