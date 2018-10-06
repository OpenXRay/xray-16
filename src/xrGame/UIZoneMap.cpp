#include "StdAfx.h"
#include "UIZoneMap.h"

#include "InfoPortion.h"
#include "PDA.h"

#include "Grenade.h"
#include "Level.h"
#include "game_cl_base.h"

#include "Actor.h"
#include "ai_space.h"
#include "xrAICore/Navigation/game_graph.h"

#include "ui/UIMap.h"
#include "ui/UIXmlInit.h"
#include "ui/UIHelper.h"
#include "ui/UIInventoryUtilities.h"
//////////////////////////////////////////////////////////////////////////

CUIZoneMap::CUIZoneMap() : m_current_map_idx(u8(-1)), visible(true) {}
CUIZoneMap::~CUIZoneMap() {}
void CUIZoneMap::Init()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "zone_map.xml");

    CUIXmlInit xml_init;
    xml_init.InitStatic(uiXml, "minimap:background", 0, &m_background);
    xml_init.InitWindow(uiXml, "minimap:level_frame", 0, &m_clipFrame);
    xml_init.InitStatic(uiXml, "minimap:center", 0, &m_center);

    m_clock_wnd = UIHelper::CreateStatic(uiXml, "minimap:clock_wnd", &m_background);

    m_activeMap = new CUIMiniMap();
    m_clipFrame.AttachChild(m_activeMap);
    m_activeMap->SetAutoDelete(true);

    m_activeMap->EnableHeading(true);
    xml_init.InitStatic(uiXml, "minimap:compass", 0, &m_compass);
    m_background.AttachChild(&m_compass);

    m_clipFrame.AttachChild(&m_center);

    m_Counter_text.SetText("");
    visible = true;

    Fvector2 sz_k = m_clipFrame.GetWndSize();
    Fvector2 sz = sz_k;
    {
        float k = UI().get_current_kx();

        sz.y *= UI_BASE_HEIGHT * k;
        sz.x = sz.y / k;

        m_clipFrame.SetWndSize(sz);

        Fvector2 p = m_clipFrame.GetWndPos();
        p.mul(UI_BASE_HEIGHT);
        m_clipFrame.SetWndPos(p);

        m_background.SetHeight(m_background.GetHeight() * UI_BASE_HEIGHT);
        m_background.SetWidth(m_background.GetHeight() * k);
    }

    Fvector2 map_center;
    m_clipFrame.GetWndRect().getcenter(map_center);
    m_background.SetWndPos(map_center);

    Fvector2 cp;
    cp.x = m_clipFrame.GetWidth() / 2.0f;
    cp.y = m_clipFrame.GetHeight() / 2.0f;
    m_center.SetWndPos(cp);

    Fvector2 rel_pos = m_compass.GetWndPos();
    rel_pos.mul(m_background.GetWndSize());
    m_compass.SetWndPos(rel_pos);

    rel_pos = m_clock_wnd->GetWndPos();
    rel_pos.mul(m_background.GetWndSize());
    m_clock_wnd->SetWndPos(rel_pos);

    if (IsGameTypeSingle())
    {
        xml_init.InitStatic(uiXml, "minimap:static_counter", 0, &m_Counter);
        m_background.AttachChild(&m_Counter);
        xml_init.InitTextWnd(uiXml, "minimap:static_counter:text_static", 0, &m_Counter_text);
        m_Counter.AttachChild(&m_Counter_text);

        rel_pos = m_Counter.GetWndPos();
        rel_pos.mul(m_background.GetWndSize());
        m_Counter.SetWndPos(rel_pos);
    }
}

void CUIZoneMap::Render()
{
    if (!visible)
        return;

    m_clipFrame.Draw();
    m_background.Draw();
}

void CUIZoneMap::Update()
{
    CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (!pActor)
        return;

    if (!(Device.dwFrame % 20) && IsGameTypeSingle())
    {
        string16 text_str;
        xr_strcpy(text_str, sizeof(text_str), "");

        CPda* pda = pActor->GetPDA();
        if (pda)
        {
            u32 cn = pda->ActiveContactsNum();
            if (cn > 0)
            {
                xr_sprintf(text_str, sizeof(text_str), "%d", cn);
            }
        }
        m_Counter_text.SetText(text_str);
    }

    UpdateRadar(Device.vCameraPosition);
    float h, p;
    Device.vCameraDirection.getHP(h, p);
    SetHeading(-h);

    m_clock_wnd->TextItemControl()->SetText(
        InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes).c_str());
}

void CUIZoneMap::SetHeading(float angle)
{
    m_activeMap->SetHeading(angle);
    m_compass.SetHeading(angle);
};

void CUIZoneMap::UpdateRadar(Fvector pos)
{
    m_clipFrame.Update();
    m_background.Update();
    m_activeMap->SetActivePoint(pos);
}

bool CUIZoneMap::ZoomIn() { return true; }
bool CUIZoneMap::ZoomOut() { return true; }
void CUIZoneMap::SetupCurrentMap()
{
    m_activeMap->Initialize(Level().name(), "hud" DELIMITER "default");

    Frect r;
    m_clipFrame.GetAbsoluteRect(r);
    m_activeMap->WorkingArea().set(r);

    Fvector2 wnd_size;
    float zoom_factor = float(m_clipFrame.GetWidth()) / 100.0f;

    LPCSTR ln = Level().name().c_str();
    if (pGameIni->section_exist(ln))
    {
        if (pGameIni->line_exist(ln, "minimap_zoom"))
            zoom_factor *= pGameIni->r_float(ln, "minimap_zoom");
    }
    else if (g_pGameLevel->pLevel->section_exist("minimap_zoom"))
    {
        zoom_factor *= g_pGameLevel->pLevel->r_float("minimap_zoom", "value");
    }
    wnd_size.x = m_activeMap->BoundRect().width() * zoom_factor;
    wnd_size.y = m_activeMap->BoundRect().height() * zoom_factor;
    m_activeMap->SetWndSize(wnd_size);
}

void CUIZoneMap::OnSectorChanged(int sector)
{
    if (!g_pGameLevel->pLevel->section_exist("sub_level_map"))
        return;
    u8 map_idx = u8(-1);
    string64 s_sector;
    xr_sprintf(s_sector, "%d", sector);

    if (!g_pGameLevel->pLevel->line_exist("sub_level_map", s_sector))
        return;

    map_idx = g_pGameLevel->pLevel->r_u8("sub_level_map", s_sector);
    if (m_current_map_idx == map_idx)
        return;

    m_current_map_idx = map_idx;

    string_path sub_texture;
    xr_sprintf(sub_texture, "%s#%d", m_activeMap->m_texture.c_str(), m_current_map_idx);

    if (map_idx == u8(-1))
        xr_sprintf(sub_texture, "%s", m_activeMap->m_texture.c_str());

    m_activeMap->InitTextureEx(sub_texture, m_activeMap->m_shader_name.c_str());
}

void CUIZoneMap::Counter_ResetClrAnimation() { m_Counter_text.ResetColorAnimation(); }
