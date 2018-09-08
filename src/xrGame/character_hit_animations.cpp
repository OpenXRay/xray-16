#include "StdAfx.h"

#include "character_hit_animations.h"

#include "character_hit_animations_params.h"
#include "entity_alive.h"

#include "Include/xrRender/Kinematics.h"
#include "animation_utils.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif

hit_animation_global_params ghit_anims_params;

int tune_hit_anims = 0;
static hit_animation_global_params g_params;

hit_animation_global_params::hit_animation_global_params()
{
    power_factor = 2.000000f;
    rotational_power_factor = 3.000000f;
    side_sensitivity_threshold = 0.200000f;
    anim_channel_factor = 3.000000f;

    block_blend = 0.500000f;
    reduce_blend = 0.800000f;
    reduce_power_factor = 0.500000f;
}

void character_hit_animation_controller::SetupHitMotions(IKinematicsAnimated& ca)
{
    // IKinematicsAnimated* ca = smart_cast<IKinematicsAnimated*>(m_EntityAlife.Visual());
    /*
    bkhit_motion= ca.LL_MotionID("hitback");	//hitback2.skl
    fvhit_motion= ca.LL_MotionID("hitfront");
    rthit_motion= ca.LL_MotionID("hitright");
    lthit_motion= ca.LL_MotionID("hitleft");
*/
    if (tune_hit_anims)
        g_params = ghit_anims_params;

    bkhit_motion = ca.LL_MotionID("hitback17"); // hitback2.skl
    fvhit_motion = ca.LL_MotionID("hitfront17");
    rthit_motion = ca.LL_MotionID("hitf_right17"); // hitright
    lthit_motion = ca.LL_MotionID("hitf_left17"); // hitleft

    turn_right = ca.LL_MotionID("hit_right_shoulder17");
    turn_left = ca.LL_MotionID("hit_left_shoulder17");

    all_shift_down = ca.LL_MotionID("hitf_down17");
    hit_downl = ca.LL_MotionID("hit_downl");
    hit_downr = ca.LL_MotionID("hit_downr");

    base_bone = smart_cast<IKinematics*>(&ca)->LL_BoneID("bip01_spine1"); // bip01_spine1
    for (u16 i = 0; num_anims > i; ++i)
        block_blends[i] = 0;
}
ICF int sign(float x) { return x < 0 ? -1 : 1; }
IC void set_blend_params(CBlend* B)
{
    if (!B)
        return;
    B->blendAmount = 1.0;
}

IC void play_cycle(IKinematicsAnimated* CA, const MotionID& m, u8 channel, CBlend*& blend_block, float base_power)
{
    const BOOL mixin = TRUE;
    float power = base_power;
    if (blend_block && blend_block->blend_state() != CBlend::eFREE_SLOT)
    {
        float blend_pecent = blend_block->timeCurrent / blend_block->timeTotal;
        if (blend_pecent < g_params.block_blend)
            return;
        if (blend_pecent < g_params.reduce_blend)
            power *= g_params.reduce_power_factor;
    }
    CBlend* B = (CA->PlayCycle(m, mixin, 0, 0, channel));
    B->blendAmount = power;
    B->blendPower = power;
    blend_block = B;
}

void character_hit_animation_controller::PlayHitMotion(
    const Fvector& dir, const Fvector& bone_pos, u16 bi, CEntityAlive& ea) const
{
    IRenderVisual* pV = ea.Visual();
    IKinematicsAnimated* CA = smart_cast<IKinematicsAnimated*>(pV);
    IKinematics* K = smart_cast<IKinematics*>(pV);

    // play_cycle(CA,all_shift_down,1,block_times[6],1) ;
    if (!(K->LL_BoneCount() > bi))
        return;

    Fvector dr = dir;
    Fmatrix m;
    GetBaseMatrix(m, ea);

#ifdef DEBUG
    if (ph_dbg_draw_mask1.test(phDbgHitAnims))
    {
        DBG_OpenCashedDraw();
        DBG_DrawLine(m.c, Fvector().sub(m.c, Fvector().mul(dir, 1.5)), color_xrgb(255, 0, 255));
        DBG_ClosedCashedDraw(1000);
    }
#endif

    m.invert();
    m.transform_dir(dr);
    //
    Fvector hit_point;
    K->LL_GetTransform(bi).transform_tiny(hit_point, bone_pos);
    ea.XFORM().transform_tiny(hit_point);
    m.transform_tiny(hit_point);
    Fvector torqu;
    torqu.crossproduct(dr, hit_point);
    hit_point.x = 0;

    float rotational_ammount =
        hit_point.magnitude() * g_params.power_factor * g_params.rotational_power_factor; //_abs(torqu.x)

    if (torqu.x < 0)
        play_cycle(CA, hit_downr, 3, block_blends[7], 1);
    else
        play_cycle(CA, hit_downl, 3, block_blends[6], 1);

    if (!IsEffected(bi, *K))
        return;
    if (torqu.x < 0)
        play_cycle(CA, turn_right, 2, block_blends[4], rotational_ammount);
    else
        play_cycle(CA, turn_left, 2, block_blends[5], rotational_ammount);

    // CA->LL_SetChannelFactor(3,rotational_ammount);

    dr.x = 0;
    dr.normalize_safe();

    dr.mul(g_params.power_factor);
    if (dr.y > g_params.side_sensitivity_threshold)
        play_cycle(CA, rthit_motion, 2, block_blends[0], _abs(dr.y));
    else if (dr.y < -g_params.side_sensitivity_threshold)
        play_cycle(CA, lthit_motion, 2, block_blends[1], _abs(dr.y));

    if (dr.z < 0.f)
        play_cycle(CA, fvhit_motion, 2, block_blends[2], _abs(dr.z));
    else
        play_cycle(CA, bkhit_motion, 2, block_blends[3], _abs(dr.z));

    CA->LL_SetChannelFactor(2, g_params.anim_channel_factor);
}

bool character_hit_animation_controller::IsEffected(u16 bi, IKinematics& ca) const
{
    return find_in_parents(base_bone, bi, ca);
    /*
    u16 root = ca.LL_GetBoneRoot();
    for( ; bi != root && bi != BI_NONE ; )
    {
        CBoneData &bd	= ca.LL_GetData(bi);
        if(bi == base_bone)
            return true;
        bi = bd.GetParentID();
    }
    return false;
    */
}

void character_hit_animation_controller::GetBaseMatrix(Fmatrix& m, CEntityAlive& ea) const
{
    IKinematics* CA = smart_cast<IKinematics*>(ea.Visual());
    m.mul_43(ea.XFORM(), CA->LL_GetTransform(base_bone));
}
