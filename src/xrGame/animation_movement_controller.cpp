#include "StdAfx.h"
#include "animation_movement_controller.h"

#include "Include/xrRender/Kinematics.h"
#include "game_object_space.h"
#include "xrPhysics/matrix_utils.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif
#include "Common/Noncopyable.hpp"
void DBG_DrawBones(const Fmatrix& xform, IKinematics* K);
#ifdef DEBUG
BOOL dbg_draw_animation_movement_controller = FALSE;
u16 dbg_frame_count = 0;
#endif

animation_movement_controller::animation_movement_controller(
    Fmatrix* _pObjXForm, const Fmatrix& inital_pose, IKinematics* _pKinematicsC, CBlend* b)
    : m_startObjXForm(inital_pose), m_pObjXForm(*_pObjXForm), m_pKinematicsC(_pKinematicsC),
      m_pKinematicsA(smart_cast<IKinematicsAnimated*>(_pKinematicsC)), inital_position_blending(true), stopped(false),
      blend_linear_speed(0), blend_angular_speed(0), m_control_blend(b), m_poses_blending(Fidentity, Fidentity, -1.f)
#ifdef DEBUG
      ,
      DBG_previous_position(*_pObjXForm)
#endif
{
    VERIFY(_pKinematicsC);
    VERIFY(m_pKinematicsA);
    VERIFY(_pObjXForm);
    VERIFY(b);

#ifdef DEBUG
    if (dbg_draw_animation_movement_controller)
    {
        m_pKinematicsC->CalculateBones_Invalidate();
        m_pKinematicsC->CalculateBones(TRUE);
        DBG_OpenCashedDraw();
        DBG_DrawBones(*_pObjXForm, _pKinematicsC);
        DBG_ClosedCashedDraw(50000);
    }
#endif
    CBoneInstance& B = m_pKinematicsC->LL_GetBoneInstance(m_pKinematicsC->LL_GetBoneRoot());
    VERIFY(!B.callback() && !B.callback_param());
    B.set_callback(bctCustom, RootBoneCallback, this, TRUE);
    B.mTransform = Fidentity;
    GetInitalPositionBlenSpeed();
    // m_pKinematicsC->LL_VisBoxInvalidate();
    m_pKinematicsA->SetBlendDestroyCallback(this);
    m_pKinematicsC->CalculateBones_Invalidate();
    m_pKinematicsC->CalculateBones(TRUE);
    SetPosesBlending();
#ifdef DEBUG
    if (dbg_draw_animation_movement_controller)
    {
        DBG_OpenCashedDraw();
        DBG_DrawMatrix(*_pObjXForm, 3, 100);
        DBG_DrawBones(*_pObjXForm, _pKinematicsC);
        DBG_ClosedCashedDraw(50000);
    }
#endif
}

animation_movement_controller::~animation_movement_controller()
{
    if (IsActive())
        deinitialize();
}

IC bool is_blending_in(CBlend& b) { return b.blend_state() == CBlend::eAccrue && b.blendPower - EPS > b.blendAmount; }
void animation_movement_controller::deinitialize()
{
#ifdef DEBUG
    if (dbg_draw_animation_movement_controller)
    {
        DBG_OpenCashedDraw();
        DBG_DrawMatrix(m_pObjXForm, 3, 100);
        DBG_DrawBones(m_pObjXForm, m_pKinematicsC);
        DBG_ClosedCashedDraw(50000);
    }
#endif

    CBoneInstance& B = m_pKinematicsC->LL_GetBoneInstance(m_pKinematicsC->LL_GetBoneRoot());
    VERIFY(B.callback() == RootBoneCallback);
    VERIFY(B.callback_param() == (void*)this);
    B.reset_callback();
    m_pKinematicsA->SetBlendDestroyCallback(0);
    m_control_blend = 0;

#ifdef DEBUG
    if (dbg_draw_animation_movement_controller)
    {
        DBG_OpenCashedDraw();
        DBG_DrawMatrix(m_pObjXForm, 3, 100);
        DBG_DrawBones(m_pObjXForm, m_pKinematicsC);
        DBG_ClosedCashedDraw(50000);
    }
#endif
}

void animation_movement_controller::GetInitalPositionBlenSpeed()
{
    float sv_blend_time = m_control_blend->timeCurrent;

    // u16 root = m_pKinematicsC->LL_GetBoneRoot();
    Fmatrix m1;
    // m_pKinematicsC->Bone_GetAnimPos( m1, root, u8(-1), true );
    animation_root_position(m1);
    m_control_blend->timeCurrent += Device.fTimeDelta;
    clamp(m_control_blend->timeCurrent, 0.f, m_control_blend->timeTotal);
    Fmatrix m0;
    // m_pKinematicsC->Bone_GetAnimPos( m0, root, u8(-1), true );
    animation_root_position(m0);
    float l, a;
    get_diff_value(m0, m1, l, a);
    blend_linear_speed = l / Device.fTimeDelta;
    blend_angular_speed = a / Device.fTimeDelta;
    m_control_blend->timeCurrent = sv_blend_time;
}

bool animation_movement_controller::IsBlending() const
{
    return is_blending_in(*m_control_blend); // inital_position_blending ||
}
float blend_linear_accel = 1.f;
float blend_angular_accel = 1.f;
void animation_movement_controller::InitalPositionBlending(const Fmatrix& to)
{
#ifdef DEBUG
    if (dbg_draw_animation_movement_controller)
    {
        DBG_DrawMatrix(m_pObjXForm, 1);
        // DBG_DrawMatrix( m_startObjXForm, 3 );
    }
#endif
// if( !inital_position_blending )
//{
//	if( m_control_blend->stop_at_end_callback && !IsBlending() )
//	m_pObjXForm.set( to );
//	return ;
//}

#ifdef DEBUG
    if (dbg_draw_animation_movement_controller)
        DBG_DrawMatrix(to, 2);
#endif
    /*
        Fmatrix res = to;
        blend_linear_speed  += blend_linear_accel *Device.fTimeDelta ;
        blend_angular_speed += blend_angular_accel *Device.fTimeDelta ;

        inital_position_blending = !clamp_change( res, m_pObjXForm, blend_linear_speed*Device.fTimeDelta,
       blend_angular_speed*Device.fTimeDelta, 0.00001, 0.000001 );
        m_pObjXForm.set( res );
    */
    if (!m_poses_blending.target_reached(m_control_blend->timeCurrent))
        m_poses_blending.pose(m_pObjXForm, m_control_blend->timeCurrent);
    else
        m_pObjXForm.set(to);

#ifdef DEBUG
    DBG_previous_position = m_pObjXForm;
#endif
}
static void get_animation_root_position(Fmatrix& pos, IKinematics* K, IKinematicsAnimated* KA, CBlend* control_blend)
{
    VERIFY(KA);
    VERIFY(K);
    VERIFY(smart_cast<IKinematics*>(KA) == K);

    SKeyTable keys;
    KA->LL_BuldBoneMatrixDequatize(&K->LL_GetData(0), u8(1 << 0), keys);

    // find
    CKey* key = 0;
    for (int i = 0; i < keys.chanel_blend_conts[0]; ++i)
    {
        if (keys.blends[0][i] == control_blend)
            key = &keys.keys[0][i];
    }
    VERIFY(key);

    float sv_amount = control_blend->blendAmount;
    control_blend->blendAmount = 1.f;
    keys.blends[0][0] = control_blend;
    keys.chanel_blend_conts[0] = 1;
    keys.keys[0][0] = *key;

    for (int j = 1; j < MAX_CHANNELS; ++j)
        keys.chanel_blend_conts[j] = 0;

    CBoneInstance BI = K->LL_GetBoneInstance(0);

    KA->LL_BoneMatrixBuild(BI, &Fidentity, keys);
    pos.set(BI.mTransform);
    control_blend->blendAmount = sv_amount;
}
void animation_movement_controller::animation_root_position(Fmatrix& pos)
{
    get_animation_root_position(pos, m_pKinematicsC, m_pKinematicsA, m_control_blend);
}

void animation_movement_controller::OnFrame()
{
    // if( !isActive() )
    //	return;
    VERIFY(IsActive());
    DBG_verify_position_not_chaged();
//		ka->CalculateBones_Invalidate( );
//	ka->CalculateBones( TRUE );

#ifdef DEBUG
    if (dbg_draw_animation_movement_controller && dbg_frame_count < 3)
    {
        DBG_OpenCashedDraw();
        DBG_DrawBones(m_pObjXForm, m_pKinematicsC);
        DBG_ClosedCashedDraw(50000);
    }
#endif

    // m_pKinematicsC->Bone_GetAnimPos( root_pos, m_pKinematicsC->LL_GetBoneRoot( ), u8(-1), true );
    Fmatrix root_pos;
    animation_root_position(root_pos);

    Fmatrix obj_pos = Fmatrix().mul_43(m_startObjXForm, root_pos);
    // Fvector prv_pos = m_pObjXForm.c;
    InitalPositionBlending(obj_pos);

#ifdef DEBUG
    DBG_previous_position = m_pObjXForm;
#endif

//	UpdateVisBox( Fvector().sub(m_pObjXForm.c,prv_pos).square_magnitude()  );

/*
    if( IsActive() && IsBlending() )
    {
        m_control_blend->timeCurrent = 0;

        struct scb : public IterateBlendsCallback, private Noncopyable
        {
            const CBlend &m_control_blend;
            scb( const CBlend &B ): m_control_blend( B ){}
            virtual	void	operator () ( CBlend &B )
            {
                if(B.motionID == m_control_blend.motionID )
                    B.timeCurrent  = m_control_blend.timeCurrent;
            }
        } cb( *m_control_blend );

        m_pKinematicsA->LL_IterateBlends(cb);
    }
*/
// m_pKinematicsC->CalculateBones( );
#ifdef DEBUG
    ++dbg_frame_count;
#endif
}

void animation_movement_controller::NewBlend(CBlend* B, const Fmatrix& new_matrix, bool local_animation)
{
    /*
#ifdef	DEBUG
    LPCSTR old_anim_name	= m_pKinematicsC->dcast_PKinematicsAnimated( )->LL_MotionDefName_dbg( ControlBlend(
)->motionID ).first;
    LPCSTR old_anim_set		= m_pKinematicsC->dcast_PKinematicsAnimated( )->LL_MotionDefName_dbg( ControlBlend(
)->motionID ).second;
    LPCSTR new_anim_name	= m_pKinematicsC->dcast_PKinematicsAnimated( )->LL_MotionDefName_dbg( B->motionID ).first;
    LPCSTR new_anim_set		= m_pKinematicsC->dcast_PKinematicsAnimated( )->LL_MotionDefName_dbg( B->motionID ).second;

    if( ControlBlend( )->playing )
        Msg( " ! obj movement anim not yet ended anim: %s anim set: %s \n and already another started anim: %s anim set:
%s",
            new_anim_name,new_anim_set,old_anim_name,old_anim_set
            );
    if( !ControlBlend( )->stop_at_end )
        Msg( " ! obj movement anim  : %s anim set: %s  is not stop-at-end but fallowed in chain by another obj movement
anim: %s anim set: %s",
            old_anim_name,old_anim_set,new_anim_name,new_anim_set
            );
    if( !B->stop_at_end )
        Msg( " ! obj movement anim  : %s anim set: %s  is not stop-at-end but fallowing after another obj movement anim:
%s anim set: %s",
            new_anim_name,new_anim_set,old_anim_name,old_anim_set
            );
#endif
    //	VERIFY(  );
        ControlBlend( )->blendAmount = 0;// B->blendPower;
        B->blendAmount = B->blendPower;
        m_control_blend = B;
    */
    // CMotion* m_curr = smart_cast<IKinematicsAnimated*>(m_pKinematicsC)->LL_GetRootMotion(m_control_blend->motionID);
    // CMotion* m_new = smart_cast<IKinematicsAnimated*>(m_pKinematicsC)->LL_GetRootMotion(B->motionID);
    VERIFY(IsActive());

// m_control_blend->timeCurrent = m_control_blend->timeTotal - SAMPLE_SPF;
// m_pKinematicsC->Bone_GetAnimPos( m_pObjXForm, 0, u8(-1), true );
// m_pObjXForm.mulA_43( m_startObjXForm );

#ifdef DEBUG
    DBG_previous_position = m_pObjXForm;
#endif

    bool set_blending = !m_poses_blending.target_reached(m_control_blend->timeCurrent);

    if (stopped)
    {
        m_control_blend = B;
        m_startObjXForm.set(new_matrix);
        GetInitalPositionBlenSpeed();
        inital_position_blending = true;
        set_blending = true;
        stopped = false;
    }
    else if (local_animation)
    {
        float blend_time = m_control_blend->timeCurrent;
        m_control_blend->timeCurrent = m_control_blend->timeTotal - SAMPLE_SPF; //(SAMPLE_SPF+EPS);
        Fmatrix root;
        animation_root_position(root);
        m_startObjXForm.mulB_43(root);
#ifdef DEBUG
        if (dbg_draw_animation_movement_controller)
        {
            DBG_OpenCashedDraw();
            DBG_DrawMatrix(m_startObjXForm, 1);
            DBG_ClosedCashedDraw(5000);
        }
#endif
        m_control_blend->timeCurrent = blend_time;
    }

    m_control_blend = B;
    if (set_blending)
        SetPosesBlending();
    else
        m_poses_blending = poses_blending(Fidentity, Fidentity, -1.f);
}
void animation_movement_controller::DBG_verify_position_not_chaged() const
{
#ifdef DEBUG
    VERIFY(!IsActive() || inital_position_blending || cmp_matrix(DBG_previous_position, m_pObjXForm, EPS, EPS));
#endif
}
void animation_movement_controller::RootBoneCallback(CBoneInstance* B)
{
    VERIFY(B);
    VERIFY(B->callback_param());

    animation_movement_controller* O = (animation_movement_controller*)(B->callback_param());

    O->DBG_verify_position_not_chaged();

    // if( O->m_control_blend->stop_at_end_callback && !O->IsBlending() )
    //{
    //	O->m_pObjXForm.mul_43( O->m_startObjXForm, B->mTransform );
    //}

    // else
    //	Msg("blending");
    B->mTransform.set(Fidentity);

#if 0
	VERIFY( cmp_matrix( O->DBG_previous_position, O->m_pObjXForm, 1.f, 1.f ) );
#endif
    R_ASSERT2(_valid(B->mTransform), "animation_movement_controller::RootBoneCallback");
}

bool animation_movement_controller::IsActive() const { return !!m_control_blend; }
void animation_movement_controller::BlendDestroy(CBlend& blend)
{
    VERIFY(m_control_blend);
    // Msg("deinit");
    if (m_control_blend == &blend)
        deinitialize();
}

void animation_movement_controller::stop() { stopped = true; }
const float percent_blending = 0.2f;
void animation_movement_controller::SetPosesBlending()
{
    VERIFY(IsActive());
    float blending_time = percent_blending * m_control_blend->timeTotal;

    float sv_time = m_control_blend->timeCurrent;
    m_control_blend->timeCurrent = blending_time;

    Fmatrix root;
    animation_root_position(root);

    poses_blending blending(m_pObjXForm, Fmatrix().mul_43(m_startObjXForm, root), blending_time);
    m_poses_blending = blending;
    m_control_blend->timeCurrent = sv_time;
}

float change_pos_delta = 0.02f;

// void	animation_movement_controller::UpdateVisBox	( float pos_sq_delta )
//{
//	VERIFY( m_pKinematicsC );
//	const Fbox &b = m_pKinematicsC->GetBox();
//	Fsphere		sphere; b.getsphere( sphere.P, sphere.R );
//	float sq_diff = Fvector().sub( m_pObjXForm.c,m_update_vis_pos).magnitude();
//
//	float change_pos_sq_delta = change_pos_delta * change_pos_delta * (( Device.fTimeDelta/0.01f )*(
// Device.fTimeDelta/0.01f ));
//
//	if(  pos_sq_delta > change_pos_sq_delta || sphere.P.square_magnitude() + change_pos_sq_delta + pos_sq_delta >
// sphere.R*sphere.R )
//	{
//		m_update_vis_pos = m_pObjXForm.c;
//		m_pKinematicsC->LL_VisBoxInvalidate();
//		m_pKinematicsC->CalculateBones_Invalidate( );
//		m_pKinematicsC->CalculateBones(TRUE);// TRUE
//	}
//}
