#include "StdAfx.h"

#include "AdvancedDetector.h"
#include "ui/ArtefactDetectorUI.h"

#include "Include/xrRender/Kinematics.h"
#include "player_hud.h"

CAdvancedDetector::CAdvancedDetector() { m_artefacts.m_af_rank = 2; }

void CAdvancedDetector::CreateUI()
{
    R_ASSERT(NULL == m_ui);
    m_ui = xr_new<CUIArtefactDetectorAdv>(this);
}

CUIArtefactDetectorAdv& CAdvancedDetector::ui() const
{
    return *static_cast<CUIArtefactDetectorAdv*>(m_ui);
}

void CAdvancedDetector::Scan()
{
    const Fvector& pos = Position();

    m_zones.scan(pos, this);

    const auto [artefact, _] = m_artefacts.scan(pos, this, m_fAfVisRadius);

    if (artefact)
    {
        Fvector dir_to_artefact;
        dir_to_artefact.sub(artefact->Position(), Device.vCameraPosition);
        dir_to_artefact.normalize();

        ui().SetValue(dir_to_artefact);
    }
    else
    {
        ui().SetValue({});
    }
}

void CUIArtefactDetectorAdv::update()
{
    if (NULL == m_parent->HudItemData() || m_bid == u16(-1))
        return;
    inherited::update();
    attachable_hud_item* itm = m_parent->HudItemData();
    R_ASSERT(itm);

    BOOL b_visible = !fis_zero(m_target_dir.magnitude());
    if (b_visible != itm->m_model->LL_GetBoneVisible(m_bid))
        itm->m_model->LL_SetBoneVisible(m_bid, b_visible, TRUE);

    if (!b_visible)
        return;

    Fvector dest;
    Fmatrix Mi;
    Mi.invert(itm->m_item_transform);
    Mi.transform_dir(dest, m_target_dir);

    float dest_y_rot = -dest.getH();

    /*
        m_cur_y_rot						= angle_normalize_signed(m_cur_y_rot);
        float diff						= angle_difference_signed(m_cur_y_rot, dest_y_rot);
        float a							= (diff>0.0f)?-1.0f:1.0f;

        a								*= 2.0f;

        m_curr_ang_speed				= m_curr_ang_speed + a*Device.fTimeDelta;
        clamp							(m_curr_ang_speed,-2.0f,2.0f);
        float _add						= m_curr_ang_speed*Device.fTimeDelta;

        m_cur_y_rot						+= _add;
    */
    m_cur_y_rot = angle_inertion_var(m_cur_y_rot, dest_y_rot, PI_DIV_4, PI_MUL_4, PI_MUL_2, Device.fTimeDelta);
}
void CAdvancedDetector::on_a_hud_attach()
{
    inherited::on_a_hud_attach();
    ui().SetBoneCallbacks();
}

void CAdvancedDetector::on_b_hud_detach()
{
    inherited::on_b_hud_detach();
    ui().ResetBoneCallbacks();
}

void CUIArtefactDetectorAdv::BoneCallback(CBoneInstance* B)
{
    CUIArtefactDetectorAdv* P = static_cast<CUIArtefactDetectorAdv*>(B->callback_param());
    Fmatrix rY;
    rY.rotateY(P->CurrentYRotation());
    B->mTransform.mulB_43(rY);
}

void CUIArtefactDetectorAdv::SetBoneCallbacks()
{
    attachable_hud_item* itm = m_parent->HudItemData();
    R_ASSERT(itm);
    m_bid = itm->m_model->LL_BoneID("screen_bone");

    CBoneInstance& bi = itm->m_model->LL_GetBoneInstance(m_bid);
    bi.set_callback(bctCustom, BoneCallback, this);

    float p, b;
    bi.mTransform.getHPB(m_cur_y_rot, p, b);
}

void CUIArtefactDetectorAdv::ResetBoneCallbacks()
{
    attachable_hud_item* itm = m_parent->HudItemData();
    R_ASSERT(itm);
    u16 bid = itm->m_model->LL_BoneID("screen_bone");

    CBoneInstance& bi = itm->m_model->LL_GetBoneInstance(bid);
    bi.reset_callback();
}

float CUIArtefactDetectorAdv::CurrentYRotation() const
{
    const float one = PI_MUL_2 / 24.0f;
    const float ret = fmod(m_cur_y_rot, one);
    return (m_cur_y_rot - ret);
}
