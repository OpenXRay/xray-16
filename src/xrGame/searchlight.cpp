#include "StdAfx.h"
#include "searchlight.h"
#include "xrEngine/LightAnimLibrary.h"
#include "script_entity_action.h"
#include "xrServer_Objects_ALife.h"
#include "Include/xrRender/Kinematics.h"
#include "game_object_space.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProjector::CProjector()
{
    light_render = GEnv.Render->light_create();
    light_render->set_type(IRender_Light::SPOT);
    light_render->set_shadow(true);
    glow_render = GEnv.Render->glow_create();
    lanim = 0;
    bone_x.id = BI_NONE;
    bone_y.id = BI_NONE;
}

CProjector::~CProjector()
{
    light_render.destroy();
    glow_render.destroy();
}

void CProjector::Load(LPCSTR section) { inherited::Load(section); }
void CProjector::BoneCallbackX(CBoneInstance* B)
{
    CProjector* P = static_cast<CProjector*>(B->callback_param());

    Fmatrix M;
    M.setHPB(0.0f, P->_current.pitch, 0.0f);
    B->mTransform.mulB_43(M);
}

void CProjector::BoneCallbackY(CBoneInstance* B)
{
    CProjector* P = static_cast<CProjector*>(B->callback_param());

    float delta_yaw = angle_difference(P->_start.yaw, P->_current.yaw);
    if (angle_normalize_signed(P->_start.yaw - P->_current.yaw) > 0)
        delta_yaw = -delta_yaw;

    Fmatrix M;
    M.setHPB(-delta_yaw, 0.0, 0.0f);
    B->mTransform.mulB_43(M);
}

BOOL CProjector::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeObjectProjector* slight = smart_cast<CSE_ALifeObjectProjector*>(e);
    R_ASSERT(slight);

    if (!inherited::net_Spawn(DC))
        return (FALSE);

    R_ASSERT(Visual() && smart_cast<IKinematics*>(Visual()));

    IKinematics* K = smart_cast<IKinematics*>(Visual());
    CInifile* pUserData = K->LL_UserData();
    R_ASSERT3(pUserData, "Empty Projector user data!", slight->get_visual());
    lanim = LALib.FindItem(pUserData->r_string("projector_definition", "color_animator"));
    guid_bone = K->LL_BoneID(pUserData->r_string("projector_definition", "guide_bone"));
    VERIFY(guid_bone != BI_NONE);
    bone_x.id = K->LL_BoneID(pUserData->r_string("projector_definition", "rotation_bone_x"));
    VERIFY(bone_x.id != BI_NONE);
    bone_y.id = K->LL_BoneID(pUserData->r_string("projector_definition", "rotation_bone_y"));
    VERIFY(bone_y.id != BI_NONE);
    Fcolor clr = pUserData->r_fcolor("projector_definition", "color");
    fBrightness = clr.intensity();
    light_render->set_color(clr);
    light_render->set_range(pUserData->r_float("projector_definition", "range"));
    light_render->set_cone(deg2rad(pUserData->r_float("projector_definition", "spot_angle")));
    light_render->set_texture(pUserData->r_string("projector_definition", "spot_texture"));

    glow_render->set_texture(pUserData->r_string("projector_definition", "glow_texture"));
    glow_render->set_color(clr);
    glow_render->set_radius(pUserData->r_float("projector_definition", "glow_radius"));

    setVisible(TRUE);
    setEnabled(TRUE);

    TurnOn();

    //////////////////////////////////////////////////////////////////////////
    CBoneInstance& b_x = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(bone_x.id);
    b_x.set_callback(bctCustom, BoneCallbackX, this);

    CBoneInstance& b_y = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(bone_y.id);
    b_y.set_callback(bctCustom, BoneCallbackY, this);

    Direction().getHP(_current.yaw, _current.pitch);
    _start = _target = _current;

    //////////////////////////////////////////////////////////////////////////

    return TRUE;
}

void CProjector::shedule_Update(u32 dt) { inherited::shedule_Update(dt); }
void CProjector::TurnOn()
{
    if (light_render->get_active())
        return;

    light_render->set_active(true);
    glow_render->set_active(true);

    IKinematics* visual = smart_cast<IKinematics*>(Visual());

    visual->LL_SetBoneVisible(guid_bone, TRUE, TRUE);
    visual->CalculateBones_Invalidate();
    visual->CalculateBones(TRUE);
}

void CProjector::TurnOff()
{
    if (!light_render->get_active())
        return;

    light_render->set_active(false);
    glow_render->set_active(false);

    smart_cast<IKinematics*>(Visual())->LL_SetBoneVisible(guid_bone, FALSE, TRUE);
}

void CProjector::UpdateCL()
{
    inherited::UpdateCL();

    // update light source
    if (light_render->get_active())
    {
        // calc color animator
        if (lanim)
        {
            int frame;
            // возвращает в формате BGR
            u32 clr = lanim->CalculateBGR(Device.fTimeGlobal, frame);

            Fcolor fclr;
            fclr.set((float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f);
            fclr.mul_rgb(fBrightness / 255.f);
            light_render->set_color(fclr);
            glow_render->set_color(fclr);
        }

        CBoneInstance& BI = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(guid_bone);
        Fmatrix M;

        M.mul(XFORM(), BI.mTransform);

        light_render->set_rotation(M.k, M.i);
        light_render->set_position(M.c);
        glow_render->set_position(M.c);
        glow_render->set_direction(M.k);
    }

    // Update searchlight
    angle_lerp(_current.yaw, _target.yaw, bone_x.velocity, Device.fTimeDelta);
    angle_lerp(_current.pitch, _target.pitch, bone_y.velocity, Device.fTimeDelta);
}

void CProjector::renderable_Render() { inherited::renderable_Render(); }
BOOL CProjector::UsedAI_Locations() { return (FALSE); }
bool CProjector::bfAssignWatch(CScriptEntityAction* tpEntityAction)
{
    if (!inherited::bfAssignWatch(tpEntityAction))
        return (false);

    CScriptWatchAction& l_tWatchAction = tpEntityAction->m_tWatchAction;

    (!l_tWatchAction.m_tpObjectToWatch) ? SetTarget(l_tWatchAction.m_tTargetPoint) :
                                          SetTarget(l_tWatchAction.m_tpObjectToWatch->Position());

    float delta_yaw = angle_difference(_current.yaw, _target.yaw);
    float delta_pitch = angle_difference(_current.pitch, _target.pitch);

    bone_x.velocity = l_tWatchAction.vel_bone_x;
    float time = delta_yaw / bone_x.velocity;
    bone_y.velocity = (fis_zero(time, EPS_L) ? l_tWatchAction.vel_bone_y : delta_pitch / time);

    return false == (l_tWatchAction.m_bCompleted = ((delta_yaw < EPS_L) && (delta_pitch < EPS_L)));
}

bool CProjector::bfAssignObject(CScriptEntityAction* tpEntityAction)
{
    if (!inherited::bfAssignObject(tpEntityAction))
        return (false);

    CScriptObjectAction& l_tObjectAction = tpEntityAction->m_tObjectAction;

    if (l_tObjectAction.m_tGoalType == MonsterSpace::eObjectActionTurnOn)
        TurnOn();
    else if (l_tObjectAction.m_tGoalType == MonsterSpace::eObjectActionTurnOff)
        TurnOff();

    return (true);
}

void CProjector::SetTarget(const Fvector& target_pos)
{
    float th, tp;
    Fvector().sub(target_pos, Position()).getHP(th, tp);

    float delta_h;
    delta_h = angle_difference(th, _start.yaw);

    if (angle_normalize_signed(th - _start.yaw) > 0)
        delta_h = -delta_h;
    clamp(delta_h, -PI_DIV_2, PI_DIV_2);

    _target.yaw = angle_normalize(_start.yaw + delta_h);

    clamp(tp, -PI_DIV_2, PI_DIV_2);
    _target.pitch = tp;
}

Fvector CProjector::GetCurrentDirection() { return (Fvector().setHP(_current.yaw, _current.pitch)); }
