#include "StdAfx.h"
#include "EliteDetector.h"
#include "player_hud.h"
#include "Include/xrRender/UIRender.h"
#include "ui/UIXmlInit.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "xrUICore/Static/UIStatic.h"
#include "ui/ArtefactDetectorUI.h"
#include "ui/UIHelper.h"

constexpr cpcstr AF_SIGN = "af_sign";

CEliteDetector::CEliteDetector() { m_artefacts.m_af_rank = 3; }

void CEliteDetector::CreateUI()
{
    R_ASSERT(NULL == m_ui);
    m_ui = xr_new<CUIArtefactDetectorElite>(this);
}

CUIArtefactDetectorElite& CEliteDetector::ui() const
{
    return *static_cast<CUIArtefactDetectorElite*>(m_ui);
}

void CEliteDetector::Scan()
{
    ui().Clear();

    Fvector detector_pos = Position();

    for (const auto& [artefact, _] : m_artefacts.m_ItemInfos)
    {
        if (artefact->H_Parent())
            continue;

        ui().RegisterItemToDraw(artefact->Position(), artefact->cNameSect());

        if (artefact->CanBeInvisible())
        {
            float d = detector_pos.distance_to(artefact->Position());
            if (d < m_fAfVisRadius)
                artefact->SwitchVisibility(true);
        }
    }

    for (const auto& [zone, _] : m_zones.m_ItemInfos)
    {
        ui().RegisterItemToDraw(zone->Position(), zone->cNameSect());
    }
}

bool CEliteDetector::render_item_3d_ui_query()
{
    return IsWorking();
}

void CEliteDetector::render_item_3d_ui()
{
    R_ASSERT(HudItemData());
    inherited::render_item_3d_ui();
    ui().Draw();
    //	Restore cull mode
    GEnv.UIRender->CacheSetCullMode(IUIRender::cmCCW);
}

void fix_ws_wnd_size(CUIWindow* w, const float kx)
{
    Fvector2 p = w->GetWndSize();
    p.x /= kx;
    w->SetWndSize(p);

    p = w->GetWndPos();
    p.x /= kx;
    w->SetWndPos(p);

    CUIWindow::WINDOW_LIST::iterator it = w->GetChildWndList().begin();
    CUIWindow::WINDOW_LIST::iterator it_e = w->GetChildWndList().end();

    for (; it != it_e; ++it)
    {
        CUIWindow* w2 = *it;
        fix_ws_wnd_size(w2, kx);
    }
}

CUIArtefactDetectorElite::CUIArtefactDetectorElite(CEliteDetector* p)
    : CUIWindow(CUIArtefactDetectorElite::GetDebugType()), m_parent(p)
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "ui_detector_artefact.xml");

    string512 buff;
    xr_strcpy(buff, p->ui_xml_tag());

    CUIXmlInit::InitWindow(uiXml, buff, 0, this);

    m_wrk_area = xr_new<CUIWindow>("Work area");

    xr_sprintf(buff, "%s:wrk_area", p->ui_xml_tag());

    CUIXmlInit::InitWindow(uiXml, buff, 0, m_wrk_area);
    m_wrk_area->SetAutoDelete(true);
    AttachChild(m_wrk_area);

    xr_sprintf(buff, "%s", p->ui_xml_tag());
    XML_NODE pStoredRoot = uiXml.GetLocalRoot();
    uiXml.SetLocalRoot(uiXml.NavigateToNode(buff, 0));

    // Always allow CS style icon
    if (auto* af_sign = UIHelper::CreateStatic(uiXml, AF_SIGN, m_wrk_area, false))
    {
        af_sign->SetCustomDraw(true);
        m_palette[AF_SIGN] = af_sign;
    }

    const size_t num = uiXml.GetNodesNum(buff, 0, "palette");
    for (size_t idx = 0; idx < num; ++idx)
    {
        shared_str name = uiXml.ReadAttrib("palette", idx, "id");
        if (name.empty())
        {
#ifndef MASTER_GOLD
            Msg("! CUIArtefactDetectorElite::construct(): palette with index[%zu] in section[%s] has empty id", idx, p->ui_xml_tag());
#endif
            continue;
        }
        auto& S = m_palette[name];
        if (S)
        {
            // prevent memory leak in cases of duplicated IDs
            // Will be automatically freed due to auto delete
            m_wrk_area->DetachChild(S);
        }

        S = UIHelper::CreateStatic(uiXml, "palette", idx, m_wrk_area);
        S->SetCustomDraw(true);
    }

    uiXml.SetLocalRoot(pStoredRoot);

    Fvector _map_attach_p = pSettings->r_fvector3(m_parent->cNameSect(), "ui_p");
    Fvector _map_attach_r = pSettings->r_fvector3(m_parent->cNameSect(), "ui_r");

    _map_attach_r.mul(PI / 180.f);
    m_map_attach_offset.setHPB(_map_attach_r.x, _map_attach_r.y, _map_attach_r.z);
    m_map_attach_offset.translate_over(_map_attach_p);
}

void CUIArtefactDetectorElite::update()
{
    inherited::update();
    CUIWindow::Update();
}

void CUIArtefactDetectorElite::Draw()
{
    const auto detect_radius = m_parent->GetAfDetectRadius();

    Fmatrix LM;
    GetUILocatorMatrix(LM);

    IUIRender::ePointType bk = UI().m_currentPointType;

    UI().m_currentPointType = IUIRender::pttLIT;

    GEnv.UIRender->CacheSetXformWorld(LM);
    GEnv.UIRender->CacheSetCullMode(IUIRender::cmNONE);

    CUIWindow::Draw();

    //.	Frect r						= m_wrk_area->GetWndRect();
    Fvector2 wrk_sz = m_wrk_area->GetWndSize();
    Fvector2 rp;
    m_wrk_area->GetAbsolutePos(rp);

    Fmatrix M, Mc;
    float h, p;
    Device.vCameraDirection.getHP(h, p);
    Mc.setHPB(h, 0, 0);
    Mc.c.set(Device.vCameraPosition);
    M.invert(Mc);

    UI().ScreenFrustumLIT().CreateFromRect(Frect().set(rp.x, rp.y, wrk_sz.x, wrk_sz.y));

    for (const auto& draw_item : m_items_to_draw)
    {
        Fvector p = draw_item.pos;
        Fvector pt3d;
        M.transform_tiny(pt3d, p);
        float kz = wrk_sz.y / detect_radius;
        pt3d.x *= kz;
        pt3d.z *= kz;

        pt3d.x += wrk_sz.x / 2.0f;
        pt3d.z -= wrk_sz.y;

        Fvector2 pos{ pt3d.x, -pt3d.z };
        pos.sub(rp);
        if (1 /* r.in(pos)*/)
        {
            draw_item.pStatic->SetWndPos(pos);
            draw_item.pStatic->Draw();
        }
    }

    UI().m_currentPointType = bk;
}

void CUIArtefactDetectorElite::GetUILocatorMatrix(Fmatrix& m) const
{
    const Fmatrix trans = m_parent->HudItemData()->m_item_transform;
    const u16 bid = m_parent->HudItemData()->m_model->LL_BoneID("cover");
    const Fmatrix cover_bone = m_parent->HudItemData()->m_model->LL_GetTransform(bid);
    m.mul(trans, cover_bone);
    m.mulB_43(m_map_attach_offset);
}

void CUIArtefactDetectorElite::Clear()
{
    m_items_to_draw.clear();
}

void CUIArtefactDetectorElite::RegisterItemToDraw(const Fvector& p, const shared_str& palette_idx)
{
    auto it = m_palette.find(palette_idx);
    if (it == m_palette.end())
        it = m_palette.find(AF_SIGN);

    if (it == m_palette.end())
    {
#ifndef MASTER_GOLD
        Msg("! CUIArtefactDetectorElite::RegisterItemToDraw(): static not found for [%s]", palette_idx.c_str());
#endif
        return;
    }

    CUIStatic* s = it->second;
    m_items_to_draw.emplace_back(s, p);
}
