#include "StdAfx.h"

#include "character_shell_control.h"

#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"

#include "xrPhysics/ExtendedGeom.h"
#include "xrPhysics/PhysicsShell.h"

#include "Level.h"
#include "CustomZone.h"

#ifdef DEBUG
extern BOOL death_anim_debug;
#endif // DEBUG

character_shell_control::character_shell_control()
    : m_shot_up_factor(0.f), m_after_death_velocity_factor(1.f), m_was_wounded(false), m_Pred_Time(0.0)
{
}

void character_shell_control::Load(LPCSTR section)
{
    skel_airr_ang_factor = pSettings->r_float(section, "ph_skeleton_airr_ang_factor");
    skel_airr_lin_factor = pSettings->r_float(section, "ph_skeleton_airr_lin_factor");
    hinge_force_factor1 = pSettings->r_float(section, "ph_skeleton_hinger_factor1");
    skel_ddelay = pSettings->r_float(section, "ph_skeleton_ddelay");
    skel_remain_time = skel_ddelay;
    skel_fatal_impulse_factor = pSettings->r_float(section, "ph_skel_fatal_impulse_factor");
    skeleton_skin_ddelay = pSettings->r_float(section, "ph_skeleton_skin_ddelay");
    skeleton_skin_remain_time = skeleton_skin_ddelay;
    // gray_wolf>Читаем из ltx параметры для поддержки изменяющегося трения у персонажей во время смерти
    // gray_wolf<
    skeleton_skin_friction_start = pSettings->r_float(section, "ph_skeleton_skin_friction_start");
    skeleton_skin_friction_end = pSettings->r_float(section, "ph_skeleton_skin_friction_end");
    character_have_wounded_state = pSettings->r_bool(section, "ph_character_have_wounded_state");
    skeleton_skin_ddelay_after_wound = pSettings->r_float(section, "ph_skeleton_skin_ddelay_after_wound");
    skeleton_skin_remain_time_after_wound = skeleton_skin_ddelay_after_wound;
    pelvis_factor_low_pose_detect = pSettings->r_float(section, "ph_pelvis_factor_low_pose_detect");
    if (pSettings->line_exist(section, "ph_skel_shot_up_factor"))
        m_shot_up_factor = pSettings->r_float(section, "ph_skel_shot_up_factor");
    if (pSettings->line_exist(section, "ph_after_death_velocity_factor"))
        m_after_death_velocity_factor = pSettings->r_float(section, "ph_after_death_velocity_factor");
}

void character_shell_control::set_kill_hit(SHit& H) const
{
    Fvector& dir = H.dir;
    if (!fis_zero(m_shot_up_factor) && H.type() != ALife::eHitTypeExplosion)
    {
        dir.y += m_shot_up_factor;
        dir.normalize();
    }
}

void character_shell_control::set_fatal_impulse(SHit& H) const
{
    if (!m_was_wounded)
    {
        H.impulse *= (H.type() == ALife::eHitTypeExplosion ? 1.f : skel_fatal_impulse_factor);
    }
}
void OnCharacterContactInDeath(
    bool& do_colide, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/)
{
    dSurfaceParameters& surface = c.surface;
    character_shell_control* l_character_physic_support = 0;
    if (bo1)
    {
        l_character_physic_support = (character_shell_control*)PHRetrieveGeomUserData(c.geom.g1)->callback_data;
    }
    else
    {
        l_character_physic_support = (character_shell_control*)PHRetrieveGeomUserData(c.geom.g2)->callback_data;
    }

    surface.mu = l_character_physic_support->curr_skin_friction_in_death();
}
void character_shell_control::set_start_shell_params(CPhysicsShell* sh) const
{
    sh->SetAirResistance(skel_airr_lin_factor, skel_airr_ang_factor);
    sh->add_ObjectContactCallback(OnCharacterContactInDeath);
    sh->set_CallbackData((void*)this);
}

void character_shell_control::apply_start_velocity_factor(IGameObject* who, Fvector& velocity) const
{
    velocity.mul(1.3f);
    velocity.mul(1.25f * m_after_death_velocity_factor);
    // set shell params
    if (!smart_cast<CCustomZone*>(who))
    {
        velocity.mul(1.25f * m_after_death_velocity_factor);
    }
}

void character_shell_control::TestForWounded(const Fmatrix& xform, IKinematics* CKA)
{
    m_was_wounded = false;
    if (!character_have_wounded_state)
    {
        return;
    }

    // IKinematics* CKA=smart_cast<IKinematics*>(m_EntityAlife.Visual());
    CKA->CalculateBones();
    CBoneInstance CBI = CKA->LL_GetBoneInstance(CKA->LL_BoneID("bip01_pelvis"));
    Fmatrix position_matrix;
    position_matrix.mul(xform, CBI.mTransform);

    xrXRC xrc;
    xrc.ray_options(0);
    xrc.ray_query(Level().ObjectSpace.GetStaticModel(), position_matrix.c, Fvector().set(0.0f, -1.0f, 0.0f),
        pelvis_factor_low_pose_detect);

    if (xrc.r_count())
    {
        m_was_wounded = true;
    }
#ifdef DEBUG
    if (death_anim_debug)
    {
        Msg("death anim: test for wounded %s ", m_was_wounded ? "true" : "false");
    }
#endif
};

void character_shell_control::CalculateTimeDelta()
{
    if (m_Pred_Time == 0.0)
    {
        m_time_delta = 0;
    }
    else
    {
        m_time_delta = Device.fTimeGlobal - m_Pred_Time;
    }
    m_Pred_Time = Device.fTimeGlobal;
};

void character_shell_control::UpdateFrictionAndJointResistanse(CPhysicsShell* sh)
{
    //Преобразование skel_ddelay из кадров в секунды и линейное нарастание сопротивления в джоинтах со временем от
    //момента смерти

    if (skel_remain_time != 0)
    {
        skel_remain_time -= m_time_delta;
    };
    if (skel_remain_time < 0)
    {
        skel_remain_time = 0;
    };

    float curr_joint_resistance = hinge_force_factor1 - (skel_remain_time * hinge_force_factor1) / skel_ddelay;
    sh->set_JointResistance(curr_joint_resistance);

    if (skeleton_skin_remain_time != 0)
    {
        skeleton_skin_remain_time -= m_time_delta;
    }
    if (skeleton_skin_remain_time < 0)
    {
        skeleton_skin_remain_time = 0;
    }

    if (skeleton_skin_remain_time_after_wound != 0)
    {
        skeleton_skin_remain_time_after_wound -= m_time_delta;
    };
    if (skeleton_skin_remain_time_after_wound < 0)
    {
        skeleton_skin_remain_time_after_wound = 0;
    };

    float ddelay, remain;
    if (m_was_wounded)
    {
        ddelay = skeleton_skin_ddelay_after_wound;
        remain = skeleton_skin_remain_time_after_wound;
    }
    else
    {
        ddelay = skeleton_skin_ddelay;
        remain = skeleton_skin_remain_time;
    }

    m_curr_skin_friction_in_death =
        skeleton_skin_friction_end + (remain / ddelay) * (skeleton_skin_friction_start - skeleton_skin_friction_end);
};
