#include "StdAfx.h"

#include "SimpleDetector.h"
#include "ui/ArtefactDetectorUI.h"

#include "Include/xrRender/Kinematics.h"
#include "xrEngine/LightAnimLibrary.h"
#include "player_hud.h"

CSimpleDetector::CSimpleDetector() { m_artefacts.m_af_rank = 1; }

void CSimpleDetector::CreateUI()
{
    R_ASSERT(NULL == m_ui);
    m_ui = xr_new<CUIArtefactDetectorSimple>(this);
}

CUIArtefactDetectorSimple& CSimpleDetector::ui() const
{
    return *static_cast<CUIArtefactDetectorSimple*>(m_ui);
}

void CSimpleDetector::Scan()
{
    const Fvector& pos = Position();

    m_zones.scan(pos, this);

    const auto& [artefact, fRelPow] = m_artefacts.scan(pos, this, m_fAfVisRadius);

    if (!artefact)
        return;

    const auto& af_info = m_artefacts.m_ItemInfos[artefact];

    if (af_info.snd_time > af_info.cur_period)
        ui().Flash(true, fRelPow);
}

CUIArtefactDetectorSimple::CUIArtefactDetectorSimple(CSimpleDetector* parent)
    : m_parent(parent)
{
    Flash(false, 0.0f);
}

CUIArtefactDetectorSimple::~CUIArtefactDetectorSimple()
{
    m_flash_light.destroy();
    m_on_off_light.destroy();
}

void CUIArtefactDetectorSimple::Flash(bool bOn, float fRelPower)
{
    if (!m_parent->HudItemData())
        return;

    IKinematics* K = m_parent->HudItemData()->m_model;
    R_ASSERT(K);
    if (bOn)
    {
        K->LL_SetBoneVisible(m_flash_bone, TRUE, TRUE);
        m_turn_off_flash_time = Device.dwTimeGlobal + iFloor(fRelPower * 1000.0f);
    }
    else
    {
        K->LL_SetBoneVisible(m_flash_bone, FALSE, TRUE);
        m_turn_off_flash_time = 0;
    }
    if (bOn != m_flash_light->get_active())
        m_flash_light->set_active(bOn);
}

void CUIArtefactDetectorSimple::setup_internals()
{
    R_ASSERT(!m_flash_light);
    m_flash_light = GEnv.Render->light_create();
    m_flash_light->set_shadow(false);
    m_flash_light->set_type(IRender_Light::POINT);
    m_flash_light->set_range(pSettings->r_float(m_parent->HudItemData()->m_sect_name, "flash_light_range"));
    m_flash_light->set_hud_mode(true);

    R_ASSERT(!m_on_off_light);
    m_on_off_light = GEnv.Render->light_create();
    m_on_off_light->set_shadow(false);
    m_on_off_light->set_type(IRender_Light::POINT);
    m_on_off_light->set_range(pSettings->r_float(m_parent->HudItemData()->m_sect_name, "onoff_light_range"));
    m_on_off_light->set_hud_mode(true);

    IKinematics* K = m_parent->HudItemData()->m_model;
    R_ASSERT(K);

    R_ASSERT(m_flash_bone == BI_NONE);
    R_ASSERT(m_on_off_bone == BI_NONE);

    m_flash_bone = K->LL_BoneID("light_bone_2");
    m_on_off_bone = K->LL_BoneID("light_bone_1");

    K->LL_SetBoneVisible(m_flash_bone, FALSE, TRUE);
    K->LL_SetBoneVisible(m_on_off_bone, TRUE, TRUE);

    m_pOnOfLAnim = LALib.FindItem("det_on_off");
    m_pFlashLAnim = LALib.FindItem("det_flash");
}

void CUIArtefactDetectorSimple::update()
{
    inherited::update();

    if (m_parent->HudItemData())
    {
        if (m_flash_bone == BI_NONE)
            setup_internals();

        if (m_turn_off_flash_time && m_turn_off_flash_time < Device.dwTimeGlobal)
            Flash(false, 0.0f);

        firedeps fd;
        m_parent->HudItemData()->setup_firedeps(fd);
        if (m_flash_light->get_active())
            m_flash_light->set_position(fd.vLastFP);

        m_on_off_light->set_position(fd.vLastFP2);
        if (!m_on_off_light->get_active())
            m_on_off_light->set_active(true);

        int frame = 0;
        const u32 clr = m_pOnOfLAnim->CalculateRGB(Device.fTimeGlobal, frame);
        m_on_off_light->set_color(Fcolor(clr));
    }
}
