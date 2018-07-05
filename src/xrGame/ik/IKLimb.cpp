#include "stdafx.h"
#include "IKLimb.h"
#include "Common/Noncopyable.hpp"
#include "Include/xrRender/Kinematics.h"
#include "GameObject.h"
#include "game_object_space.h"
#include "ik_anim_state.h"
#include "xrPhysics/MathUtils.h"
#include "xrPhysics/matrix_utils.h"
#include "pose_extrapolation.h"
#include "xrCore/buffer_vector.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif

extern int ik_allign_free_foot;

int ik_blend_free_foot = 1;
int ik_local_blending = 0;
int ik_collide_blend = 0;

const Matrix Midentity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}; //. in XGlobal

const Matrix IKLocalJoint = {0, 0, 1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 1}; //. in XGlobal
const Fmatrix XLocalJoint = {0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

const Fmatrix xm2im = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1};

const Fvector xgproj_axis = {0, 1, 0};
const Fvector xgpos_axis = {0, 0, 1};

const Fvector xlproj_axis = {1, 0, 0};
const Fvector xlpos_axis = {0, 0, 1};
typedef float IVektor[3];

const IVektor lproj_vector = {0, 0, 1};
const IVektor lpos_vector = {-1, 0, 0};

const IVektor gproj_vector = {0, 0, 1}; //. in XGlobal
const IVektor gpos_vector = {1, 0, 0};

// const float		ik_timedelta_eps = EPS;

IC bool null_frame() { return !!Device.Paused(); }
IC const Fmatrix& cvm(const Matrix& IM) { return *((Fmatrix*)(&IM)); }
string256 ik_bones[4] = {"bip01_l_thigh,bip01_l_calf,bip01_l_foot,bip01_l_toe0",
    "bip01_r_thigh,bip01_r_calf,bip01_r_foot,bip01_r_toe0",
    "bip01_l_upperarm,bip01_l_forearm,bip01_l_hand,bip01_l_finger0",
    "bip01_r_upperarm,bip01_r_forearm,bip01_r_hand,bip01_r_finger0"};

IC Fmatrix& SCalculateData::goal(Fmatrix& g) const
{
    g.set(Fmatrix().mul_43(Fmatrix().invert(*m_obj), state.goal.get()));
    m_limb->ref_bone_to_foot(g);
    return g;
}

CIKLimb::CIKLimb() { Invalidate(); }
CIKLimb::CIKLimb(const CIKLimb& l)
    : m_limb(l.m_limb), m_K(l.m_K), m_foot(l.m_foot),

      m_id(l.m_id), m_collide(l.m_collide), collide_data(l.collide_data), anim_state(l.anim_state),
      collider(l.collider), state_predict(l.state_predict)
{
    m_bones[0] = l.m_bones[0];
    m_bones[1] = l.m_bones[1];
    m_bones[2] = l.m_bones[2];
    m_bones[3] = l.m_bones[3];
    sv_state = ik_limb_state(this, l.sv_state);

#ifdef IK_DBG_STATE_SEQUENCE
    m_dbg_matrises = l.m_dbg_matrises;
#endif
#ifdef DEBUG
    dbg_disabled = l.dbg_disabled;
#endif
}

CIKLimb& CIKLimb::operator=(const CIKLimb& l)
{
    R_ASSERT(false);
    // CIKLimb temp ( l );
    // std::swap( *this, temp );
    // sv_state	= ik_limb_state( this, l.sv_state );
    return *this;
}

void CIKLimb::Invalidate()
{
    m_id = u16(-1);
    m_bones[0] = BI_NONE;
    m_bones[1] = BI_NONE;
    m_bones[2] = BI_NONE;
    m_bones[3] = BI_NONE;
    m_K = 0;
    m_collide = false;
#ifdef DEBUG
    dbg_disabled = false;
#endif
}
void XM_IM(const Fmatrix& XM, Fmatrix& IM) { IM.mul_43(xm2im, XM); }
void XM_IM(const Fmatrix& XM, Matrix& IM)
{
    //((Fmatrix*)(&IM))->mul_43(xm2im,XM);
    XM_IM(XM, *((Fmatrix*)(&IM)));
}
void IM_XM(const Matrix& IM, Fmatrix& XM) { XM.mul_43(xm2im, *((Fmatrix*)(&IM))); }
void XM2IM(const Fmatrix& XM, Fmatrix& IM)
{
    // IM=xm2im*XM*xm2im^-1
    Fmatrix tmp;
    tmp.mul_43(xm2im, XM);
    IM.mul_43(tmp, xm2im);
}
void XM2IM(const Fmatrix& XM, Matrix& IM) { XM2IM(XM, *((Fmatrix*)(&IM))); }
void IM2XM(const Matrix& IM, Fmatrix& XM) { XM2IM(*((Fmatrix*)(&IM)), XM); }
void XV2IV(const Fvector& XV, IVektor& IV) { xm2im.transform_dir(cast_fv(IV), XV); }
void IV2XV(const IVektor& IV, Fvector& XV) { xm2im.transform_dir(XV), cast_fv(IV); }
IC Fmatrix& CIKLimb::ref_bone_to_foot(Fmatrix& ref_bone) const { return m_foot.ref_bone_to_foot(ref_bone); }
void CIKLimb::ApplyState(SCalculateData& cd)
{
    m_foot.set_ref_bone();
    cd.state.ref_bone = ref_bone();
    cd.do_collide = m_collide && cd.do_collide;
    cd.state.foot_step = anim_state.step() && collide_data.collided;
}
void CIKLimb::SetGoal(SCalculateData& cd)
{
    SetAnimGoal(cd);
    SIKCollideData cld;
    if (cd.do_collide)
    {
        GetPickDir(collide_data.m_pick_dir, cd);
        cld = collide_data;
    }
    transform(cd.state.b2tob3, 2, 3);
#if 0
	if(!state_valide(sv_state))
	{
		Msg( "st ! valide:-: time: %d ;time delta: %d ; sv_state.calc_time: %d", Device.dwTimeGlobal, Device.dwTimeDelta,  sv_state.calc_time );
	}
#endif
    SetNewGoal(cld, cd);
}
void CIKLimb::SolveBones(SCalculateData& cd)
{
    if (cd.apply)
        Solve(cd);
}

Fmatrix& CIKLimb::transform(Fmatrix& m, u16 bone0, u16 bone1) const
{
    VERIFY(bone0 < 4);
    VERIFY(bone1 < 4);
    m.mul_43(
        Fmatrix().invert(Kinematics()->LL_GetTransform(m_bones[bone0])), Kinematics()->LL_GetTransform(m_bones[bone1]));
    return m;
}

float CIKLimb::SwivelAngle(const Fmatrix& ihip, const SCalculateData& cd)
{
    Fvector foot;
    foot.set(Kinematics()->LL_GetTransform(m_bones[2]).c); // use "0" channal only?
    ihip.transform_tiny(foot);
    xm2im.transform_tiny(foot);

    Fvector knee;
    knee.set(Kinematics()->LL_GetTransform(m_bones[1]).c);

    Fmatrix ih;
    CBoneData& BD = Kinematics()->LL_GetData(m_bones[0]);
    ih.mul_43(Kinematics()->LL_GetTransform(BD.GetParentID()), BD.bind_transform);
    ih.invert();

    ih.transform_tiny(knee);
    xm2im.transform_tiny(knee);

    return m_limb.KneeAngle(cast_fp(foot), cast_fp(knee));
}

void CIKLimb::GetKnee(Fvector& knee, const SCalculateData& cd) const
{
    const Fvector hip = Kinematics()->LL_GetTransform(m_bones[0]).c;
    knee = Kinematics()->LL_GetTransform(m_bones[1]).c;
    const Fvector foot = Kinematics()->LL_GetTransform(m_bones[2]).c;
    Fmatrix goal;
    Fvector p0;
    p0.sub(foot, hip);
    Fvector p1;
    p1.sub(cd.goal(goal).c, hip);
    float mp0 = p0.magnitude();
    if (fis_zero(mp0))
        return;
    p0.mul(1.f / mp0);
    knee.sub(hip);
    float dot = p0.dotproduct(knee);
    Fvector b1;
    b1.mul(p0, dot);
    Fvector b2;
    b2.sub(knee, b1);
    Fvector bb1;
    bb1.mul(p1, 1.f / mp0 * dot); // mp1
    knee.add(bb1, b2);
    knee.add(hip);
}

bool CIKLimb::SetGoalToLimb(const SCalculateData& cd)
{
#ifdef IK_DBG_STATE_SEQUENCE
    m_dbg_matrises.next_goal(cd);
#endif

    Matrix gl;
    Fmatrix goal;
#ifdef DEBUG
    return !!m_limb.SetGoal(Goal(gl, cd.goal(goal), cd), ph_dbg_draw_mask.test(phDbgIKLimits));
#else
    return !!m_limb.SetGoal(Goal(gl, cd.goal(goal), cd), FALSE);
#endif
}

void CIKLimb::Solve(SCalculateData& cd)
{
    if (!SetGoalToLimb(cd))
        return;

    float x[7];

    Fvector pos;
    GetKnee(pos, cd);

#ifdef DEBUG
    if (ph_dbg_draw_mask.test(phDbgDrawIKGoal))
    {
        Fvector dbg_pos;
        cd.m_obj->transform_tiny(dbg_pos, pos);
        DBG_DrawPoint(dbg_pos, 0.02f, color_xrgb(255, 255, 255));
    }
#endif
    Fmatrix ihip;
    GetHipInvert(ihip, cd);
    ihip.transform_tiny(pos);
    xm2im.transform_tiny(pos);

    if (m_limb.SolveByPos(cast_fp(pos), x))
    {
        cd.m_angles = x;
        CalculateBones(cd);
    }
#ifdef DEBUG
    if (ph_dbg_draw_mask.test(phDbgDrawIKGoal))
    {
        Fvector dbg_pos;
        m_foot.ToePosition(dbg_pos);
        Kinematics()->LL_GetBoneInstance(m_bones[m_foot.ref_bone()]).mTransform.transform_tiny(dbg_pos);
        cd.m_obj->transform_tiny(dbg_pos);
        DBG_DrawPoint(dbg_pos, 0.02f, color_xrgb(255, 255, 0));
    }
#endif
}

IC void set_limits(float& min, float& max, SJointLimit& l)
{
    min = -l.limit.y;
    max = -l.limit.x;
    min += M_PI;
    max += M_PI;
    clamp<float>(min, 0.f, 2 * M_PI);
    clamp<float>(max, 0.f, 2 * M_PI);
}

IC void free_limits(float& min, float& max)
{
    min = 0;
    max = 2 * M_PI;
}

u16 get_ik_bone(IKinematics* K, LPCSTR S, u16 i)
{
    string32 sbone;
    _GetItem(S, i, sbone);
    u16 bone = K->LL_BoneID(sbone);
#ifdef DEBUG
    if (BI_NONE == bone)
    {
        Msg("ik bone: %s does not found in visual: %s", sbone, *smart_cast<IRenderVisual*>(K)->getDebugName());
        VERIFY(false);
    }
#endif
    return bone;
}

void parse_bones_string(IKinematics* K, LPCSTR S, u16 bones[4])
{
    for (u16 i = 0; 4 > i; ++i)
        bones[i] = get_ik_bone(K, S, i);
}

bool has_ik_settings(IKinematics* K) { return K->LL_UserData() && K->LL_UserData()->section_exist("ik"); }
void CIKLimb::Create(u16 id, IKinematicsAnimated* K, bool collide_)
{
    VERIFY(K);
    m_id = id;
    m_K = K;

    IKinematics* CK = smart_cast<IKinematics*>(K);
    parse_bones_string(CK, ik_bones[get_id()], m_bones);

    if (has_ik_settings(CK))
    {
        string32 section;
        string32 buff;
        strconcat(sizeof(section), section, "ik_limb", xr_itoa(id, buff, 10));
        parse_bones_string(CK, CK->LL_UserData()->r_string(section, "bones"), m_bones);
        m_foot.Create(CK, section, m_bones);
    }
    else
        m_foot.Create(CK, 0, m_bones);
    ////////////////////////////////////////////////////////////////////
    sv_state.set_limb(this);
    m_collide = collide_;
    //////////////////////////////////////////////////////////////////////
    xr_vector<Fmatrix> binds;
    CK->LL_GetBindTransform(binds);
    Fmatrix XT, XS;
    XT.set(binds[m_bones[0]]);
    XT.invert();
    XT.mulB_43(binds[m_bones[1]]);
    XS.set(binds[m_bones[1]]);
    XS.invert();
    XS.mulB_43(binds[m_bones[2]]);
    Matrix T, S;
    XM2IM(XT, T);
    XM2IM(XS, S);
    /////////////////////////////////////////////////////////////////////
    float lmin[7], lmax[7];
    SJointLimit* limits = CK->LL_GetData(m_bones[0]).IK_data.limits;
    set_limits(lmin[0], lmax[0], limits[0]);
    set_limits(lmin[1], lmax[1], limits[1]);
    set_limits(lmin[2], lmax[2], limits[1]);
    // free_limits( lmin[0], lmax[0] );
    // lmin[0] = M_PI * 3.f/4.f;
    lmin[1] += 1.0f;
    lmax[1] -= 0.f;
    lmin[2] += 1.0f;
    lmax[2] -= 0.f;
    lmax[0] = 2 * M_PI - M_PI * 2.f / 3.f;

    //  lmin[2]=-1.f;lmax[2]=1.f;

    limits = CK->LL_GetData(m_bones[1]).IK_data.limits;
    set_limits(lmin[3], lmax[3], limits[1]);
    free_limits(lmin[3], lmax[3]);

    limits = CK->LL_GetData(m_bones[2]).IK_data.limits;
    set_limits(lmin[4], lmax[4], limits[0]);
    set_limits(lmin[5], lmax[5], limits[1]);
    set_limits(lmin[6], lmax[6], limits[2]);
    // free_limits( lmin[4], lmax[4] );
    // free_limits( lmin[5], lmax[5] );
    // free_limits( lmin[6], lmax[6] );
    // lmin[6]=-1.f;lmax[6]=1.f;
    lmin[4] -= 1.0f;
    lmax[4] += 1.f;
    lmin[5] -= 1.0f;
    lmax[5] += 1.f;
    lmin[6] -= 1.0f;
    lmax[6] += 1.f;
    // swap(lmin[4],lmax[4]);
    // swap(lmin[5],lmax[5]);
    // swap(lmin[6],lmax[6]);
    m_limb.init(T, S, ZXY, ZXY, gproj_vector, gpos_vector, lmin, lmax);
}

void CIKLimb::Destroy() {}
#ifdef DEBUG
bool dbg_always_valide = false;
#endif

/*

IC float clamp_rotation( Fquaternion &q, float v )
{
    float angl;Fvector ax;
    q.get_axis_angle( ax, angl );
    float abs_angl = _abs( angl );
    if( abs_angl > v )
    {
        if( angl <  0.f ) v = -v;
        q.rotation( ax, v );
        q.normalize( );
    }
    return abs_angl;
}

IC float  clamp_rotation( Fmatrix &m, float v )
{
    Fquaternion q;
    q.set(m);
    float r = clamp_rotation( q, v );
    Fvector c = m.c;
    m.rotation( q );
    m.c = c;
    return r;
}

IC bool get_axis_angle( const Fmatrix &m, Fvector &ax, float &angl )
{
    Fquaternion q;
    q.set( m );
    return !!q.get_axis_angle( ax, angl );
}

IC bool clamp_change( Fmatrix& m, const Fmatrix &start, float ml, float ma, float tl, float ta )
{
    Fmatrix diff; diff.mul_43( Fmatrix( ).invert( start ), m );
    float linear_ch	 = diff.c.magnitude( );
    bool ret = linear_ch < tl;

    if( linear_ch > ml )
        diff.c.mul( ml/linear_ch );

    if( clamp_rotation( diff, ma ) > ta )
        ret = false;

    if(!ret)
        m.mul_43( start, diff );
    return ret;
}

void get_diff_value( const Fmatrix & m0, const Fmatrix &m1, float &l, float &a )
{
    Fmatrix diff; diff.mul_43( Fmatrix( ).invert( m1 ), m0 );
    l = diff.c.magnitude( );
    Fvector ax;
    get_axis_angle( diff, ax, a );
    a = _abs( a );
}
*/
IC void get_blend_speed_limits(float& l, float& a, const SCalculateData& cd, const ik_limb_state& sv_state)
{
    Fmatrix m;
    get_diff_value(sv_state.anim_pos(m), cd.state.anim_pos, l, a);
    l *= 1.5f; // a*=1.5;
    // l*=0.3f;a*=0.3f;
    // clamp(l,0.f,0.1f);
    // clamp(a,0.f,0.03f);
    // l = 0.01f; a = 0.01f;
}

#ifdef DEBUG
static Fmatrix* dm = 0;
void print_det()
{
    if (dm)
        Msg("det : %f", DET(*dm));
}
float det_tolerance = 0.2f;
#endif

IC void reset_blend_speed(SCalculateData& cd)
{
    if (null_frame())
    {
        cd.state.speed_blend_l = 0.f;
        cd.state.speed_blend_a = 0.f;
        return;
    }
    VERIFY(Device.fTimeDelta > 0.f);
    cd.state.speed_blend_l = cd.l / Device.fTimeDelta;
    cd.state.speed_blend_a = cd.a / Device.fTimeDelta;
}

static const float blend_accel_l = 10.f, blend_accel_a = 40.f;

IC void blend_speed_accel(SCalculateData& cd)
{
    if (!cd.state.blending)
    {
        reset_blend_speed(cd);
        return;
    }

    cd.state.speed_blend_l += blend_accel_l * Device.fTimeDelta; // / Device.fTimeDelta;
    cd.state.speed_blend_a += blend_accel_a * Device.fTimeDelta; // / Device.fTimeDelta;
    cd.l = cd.state.speed_blend_l * Device.fTimeDelta;
    cd.a = cd.state.speed_blend_a * Device.fTimeDelta;
}

void CIKLimb::SetNewGoal(const SIKCollideData& cld, SCalculateData& cd)
{
    if (!cd.do_collide)
        return;
    get_blend_speed_limits(cd.l, cd.a, cd, sv_state);
    cd.state.foot_step =
        m_foot.GetFootStepMatrix(cd.state.goal, cd, cld, true, !!ik_allign_free_foot) && cd.state.foot_step;

    VERIFY2(fsimilar(1.f, DET(cd.state.goal.get()), det_tolerance),
        dump_string("cd.state.goal", cd.state.goal.get()).c_str());

    cd.state.blend_to = cd.state.goal;
    sv_state.get_calculate_state(cd.state);
    if (cd.state.foot_step) // the foot in animation on ground
        SetNewStepGoal(cld, cd);
    else if (ik_blend_free_foot)
    {
        cd.state.blending = sv_state.valide();
        // cd.l = 0; cd.a = 0;^
    }
    cd.cl_shift.sub(cd.state.goal.get().c, cd.state.anim_pos.c);

#ifdef DEBUG
    DBGDrawSetNewGoal(cd, cld);
#endif

    if (cd.state.blending)
        Blending(cd);

#ifdef IK_DBG_STATE_SEQUENCE
    m_dbg_matrises.next_state(cd);
#endif

    sv_state.save_new_state(cd.state);
}

static const float linear_tolerance = 0.0000001f, angualar_tolerance = 0.00005f;

IC bool clamp_change(Fmatrix& m, const Fmatrix& start, float ml, float ma)
{
    return clamp_change(m, start, ml, ma, linear_tolerance, angualar_tolerance);
}

void cmp_matrix(bool& eq_linear, bool& eq_angular, const Fmatrix& m0, const Fmatrix& m1)
{
    cmp_matrix(eq_linear, eq_angular, m0, m1, linear_tolerance, angualar_tolerance);
}

bool CIKLimb::blend_collide(
    ik_goal_matrix& m, const SCalculateData& cd, const ik_goal_matrix& m0, const ik_goal_matrix& m1) const
{
    Fmatrix fm = m1.get();
    bool ret = clamp_change(fm, m0.get(), cd.l, cd.a);
    m.set(fm, m0.collide_state());
    // if( collide_data.m_collide_point == ik_foot_geom::toe )
    m_foot.GetFootStepMatrix(m, fm, collide_data, true, true, true);
#ifdef DEBUG
    if (ph_dbg_draw_mask1.test(phDbgDrawIKBlending))
    {
        Fvector l_toe;
        m_foot.ToePosition(l_toe);
        Fvector v;
        fm.transform_tiny(v, l_toe);
        DBG_DrawPoint(v, 0.1, color_xrgb(0, (ik_goal_matrix::cl_free == m0.collide_state()) * 255,
                                  (ik_goal_matrix::cl_aligned == m0.collide_state()) * 255));
    }
#endif
    return ret;
}
/*
bool	CIKLimb::blend_collide( ik_goal_matrix &m, const SCalculateData& cd,  const ik_goal_matrix &m0, const
ik_goal_matrix &m1 )
{
    bool ret = false;
    VERIFY( m0.collide_state() != ik_goal_matrix::cl_undefined );
    VERIFY( m1.collide_state() != ik_goal_matrix::cl_undefined );

#ifdef DEBUG
    Fvector l_toe; m_foot.ToePosition( l_toe );
#endif

    if(	m0.collide_state() == m1.collide_state() &&
        (ik_goal_matrix::cl_free == m0.collide_state() ||
         ik_goal_matrix::cl_aligned == m0.collide_state()
        )
    )
    {
        Fmatrix fm =  m1.get() ;
        ret = clamp_change( fm, m0.get(), cd.l, cd.a );
        m.set( fm, m0.collide_state() );
        m_foot.GetFootStepMatrix( m, fm, collide_data, true, true );
#ifdef IK_DBG_DRAW_BLEND_COLLIDE
        Fvector	v; fm.transform_tiny( v, l_toe );
        DBG_DrawPoint( v, 0.1, color_xrgb( 0, (ik_goal_matrix::cl_free == m0.collide_state()) * 255
,(ik_goal_matrix::cl_aligned == m0.collide_state()) * 255 ) );
#endif
        return ret;
    }

    if( ik_goal_matrix::cl_free == m0.collide_state() )
    {
        Fmatrix fm =  m1.get() ;
        ret = clamp_change( fm, m0.get(), cd.l, cd.a );
        ik_goal_matrix r;
        //r.set( fm, m0.collide_state( ) );
        bool collided = m_foot.GetFootStepMatrix( r, fm, collide_data, true, true );
        if( r.collide_state() ==  ik_goal_matrix::cl_free )
        {
            m = r;
#ifdef IK_DBG_DRAW_BLEND_COLLIDE
            Fvector	v; r.get().transform_tiny( v, l_toe );
            DBG_DrawPoint( v, 0.1, color_xrgb( (!collided) * 255, 255 , 255 ) );
#endif
            return ret;
        }
        else
        {
            //NR
#ifdef IK_DBG_DRAW_BLEND_COLLIDE
        Fvector	v; r.get().transform_tiny( v, l_toe );
        DBG_DrawPoint( v, 0.1, color_xrgb( 255, 0 , 0 ) );
#endif
            m = r;
            return false;
            //bool bl = true, ba = true;
            //cmp_matrix( bl, ba, m0.get(), m1.get(), linear_tolerance, angular_tolerance );

        }
    } else if( ik_goal_matrix::cl_free == m1.collide_state() )
    {
        Fmatrix fm =  m1.get() ;//m0
        ret = clamp_change( fm, m0.get(), cd.l, cd.a );//m1
        ik_goal_matrix r;
        //r.set( fm, m0.collide_state( ) );
        m_foot.GetFootStepMatrix( r, fm, collide_data, true, true );
        if( r.collide_state() == ik_goal_matrix::cl_free )
        {
#ifdef DEBUG
        Fvector	v; r.get().transform_tiny( v, l_toe );
        DBG_DrawPoint( v, 0.1, color_xrgb( 255, 255 , 0 ) );
#endif
            m = r;
            return ret;
        }
        else
        {
#ifdef DEBUG
        Fvector	v; r.get().transform_tiny( v, l_toe );
        DBG_DrawPoint( v, 0.1, color_xrgb( 255, 0 , 0 ) );
#endif
            //NR
            m = r;
            return false;
        }

    } else
    {
        Fmatrix fm =  m1.get() ;
        ret = clamp_change( fm, m0.get(), cd.l, cd.a );

        ik_goal_matrix r;
        r.set( fm, m0.collide_state( ) );
        m_foot.GetFootStepMatrix( r, fm, collide_data, true, true );

#ifdef IK_DBG_DRAW_BLEND_COLLIDE
        Fvector	v; r.get().transform_tiny( v, l_toe );
        DBG_DrawPoint( v, 0.1, color_xrgb( 255, 0 , 255 ) );
#endif

        m = r;
        return ret;
    }
}
*/

void CIKLimb::Blending(SCalculateData& cd)
{
    VERIFY(state_valide(sv_state));

    if (sv_state.foot_step() != cd.state.foot_step)
        reset_blend_speed(cd);
    if (ik_local_blending && sv_state.blending() && !sv_state.foot_step() && !cd.state.foot_step) //.
    {
        blend_speed_accel(cd);
        ik_goal_matrix m;
        VERIFY(fsimilar(1.f, DET(sv_state.goal(m).get()), det_tolerance));
        VERIFY(fsimilar(1.f, DET(sv_state.blend_to(m).get()), det_tolerance));

        Fmatrix diff;
        diff.mul_43(Fmatrix().invert(sv_state.blend_to(m).get()), Fmatrix(sv_state.goal(m).get()));

        VERIFY(fsimilar(1.f, DET(diff), det_tolerance));

        Fmatrix blend = Fidentity; // cd.state.blend_to;
        cd.state.blending =
            !clamp_change(blend, diff, cd.l, cd.a, linear_tolerance, angualar_tolerance); // 0.01f //0.005f

        VERIFY(fsimilar(1.f, DET(blend), det_tolerance));
        VERIFY(fsimilar(1.f, DET(cd.state.blend_to.get()), det_tolerance));

        Fmatrix fm = Fmatrix().mul_43(cd.state.blend_to.get(), blend);
        if (ik_collide_blend)
            m_foot.GetFootStepMatrix(cd.state.goal, fm, collide_data, true, true);
        else
            cd.state.goal.set(fm, cd.state.blend_to.collide_state());
        VERIFY(fsimilar(DET(cd.state.goal.get()), 1.f, det_tolerance));
    }
    else
    {
        if (!ik_collide_blend) // cd.state.foot_step ||
        {
            Fmatrix blend = cd.state.blend_to.get();
            ik_goal_matrix m;
            cd.state.blending = !clamp_change(
                blend, sv_state.goal(m).get(), cd.l, cd.a, linear_tolerance, angualar_tolerance); // 0.01f //0.005f
            cd.state.goal.set(blend, cd.state.blend_to.collide_state());
        }
        else
        {
            ik_goal_matrix m;
            cd.state.blending = !blend_collide(cd.state.goal, cd, sv_state.goal(m), cd.state.blend_to);
        }
    }
    if (!cd.state.blending)
        reset_blend_speed(cd);
}
static const s32 unstuck_time_delta_min = 500;
static const s32 unstuck_time_delta_max = 1200;
IC void new_foot_matrix(const ik_goal_matrix& m, SCalculateData& cd)
{
    cd.state.collide_pos = m;
    cd.state.unstuck_time = Device.dwTimeGlobal + Random.randI(unstuck_time_delta_min, unstuck_time_delta_max);
    reset_blend_speed(cd);
}
static const float unstuck_tolerance_linear = 0.3f;
static const float unstuck_tolerance_angular = M_PI / 4.f;

void CIKLimb::SetNewStepGoal(const SIKCollideData& cld, SCalculateData& cd)
{
    if (!state_valide(sv_state))
    {
        ik_goal_matrix foot;
        m_foot.GetFootStepMatrix(foot, cd /* gl_goal*/, cld, false, true);
        new_foot_matrix(foot, cd);
        return;
    }
    if (!sv_state.foot_step() || !anim_state.glue())
    {
        ik_goal_matrix foot;
        m_foot.GetFootStepMatrix(foot, cd /*cl*/, cld, false, true); // find where we can place the foot
        new_foot_matrix(foot, cd);
        // reset_blend_speed( cd );
        // cd.state.blending = true; ?
    }

    //	if( sv_state.ref_bone( ) != ref_bone( ) )
    //	{
    // m_foot.GetFootStepMatrix( cd.state.collide_pos, cd /*cl*/, cld, false ); // find where we can place the foot

    // cd.state.blending = true;
    // reset_blend_speed( cd );
    //	}

    if (anim_state.auto_unstuck())
    {
        cd.state.idle = true;
        ik_goal_matrix tmp;
        m_foot.GetFootStepMatrix(tmp, cd /*cl*/, cld, false, true);
        const ik_goal_matrix foot = tmp;
        const Fmatrix cl_pos = cd.state.collide_pos.get();
        //||
        if (!clamp_change(Fmatrix().set(cl_pos), foot.get(), unstuck_tolerance_linear, unstuck_tolerance_angular,
                unstuck_tolerance_linear, unstuck_tolerance_angular) ||
            (Device.dwTimeGlobal > cd.state.unstuck_time &&
                !clamp_change(Fmatrix().set(cl_pos), foot.get(), 10.f * EPS_L, EPS_L, 10.f * EPS_L, EPS_L)))
        {
            new_foot_matrix(foot, cd);
            // cd.state.collide_pos = foot;//.set( foot, ik_goal_matrix::cl_aligned );
            // reset_blend_speed( cd );
            cd.state.blending = true;
        }
    }

    blend_speed_accel(cd);

    if (cd.state.blending)
        cd.state.blend_to = cd.state.collide_pos;
    else
        cd.state.goal = cd.state.collide_pos;
}

#ifdef DEBUG
void CIKLimb::DBGDrawSetNewGoal(SCalculateData& cd, const SIKCollideData& cld)
{
    if (ph_dbg_draw_mask.test(phDbgDrawIKGoal))
    {
        if (cd.state.foot_step && state_valide(sv_state))
        {
            DBG_DrawMatrix(cd.state.collide_pos.get(), 1.0f, 100);
            DBG_DrawPoint(cd.state.collide_pos.get().c, 0.1, color_xrgb(0, 255, 255));
        }

        if (ph_dbg_draw_mask1.test(phDbgDrawIKBlending) && cd.state.blending)
        {
            if (cd.state.foot_step != sv_state.foot_step())
                cd.state.count = 50;
            int c = 55 + 200 / 50 * cd.state.count;
            if (cd.state.count > 0)
                DBG_OpenCashedDraw();
            Fvector l_toe;
            m_foot.ToePosition(l_toe);
            Fvector a0;
            ik_goal_matrix m;
            sv_state.goal(m).get().transform_tiny(a0, l_toe);
            Fvector a1;
            sv_state.goal(m).get().transform_tiny(a1, l_toe);
            DBG_DrawLine(a0, a1, color_xrgb(c, 0, c));
            Fvector a2;
            cd.state.goal.get().transform_tiny(a2, l_toe);
            DBG_DrawLine(a1, a2, color_xrgb(0, c, 0));
            /*
                        Fvector a3; sv_state_DBR.goal.transform_tiny( a3, l_toe );
                        DBG_DrawLine( a3, a0, color_xrgb( c, c , 0 ) );
                        if( Fvector( ).sub( a3, a0 ).magnitude() > 0.1f )
                                DBG_DrawLine( a3, a0, color_xrgb( c, 0 , 0 ) );
            */
            if (cd.state.count > -1)
            {
                DBG_ClosedCashedDraw(3000);
                --cd.state.count;
            }
        }
    }
    //	sv_state.get_state( sv_state_DBR );
}
#endif

void CIKLimb::ToeTimeDiff(Fvector& v, const SCalculateData& cd) const
{
    if (null_frame())
    {
        v.set(0, 0, 0);
        return;
    }
    VERIFY(Device.fTimeDelta > 0.f);
    VERIFY(state_valide(sv_state));
    Fvector p0, p1, l_toe;
    m_foot.ToePosition(l_toe);
    Fmatrix mtr;
    sv_state.anim_pos(mtr).transform_tiny(p0, l_toe);
    cd.state.anim_pos.transform_tiny(p1, l_toe);
    Fvector dir;
    dir.sub(p1, p0);
    dir.mul(dir, 1.f / Device.fTimeDelta);
    v = dir;
}

void CIKLimb::ToeTimeDiffPredict(Fvector& v) const { v.set(0, -1, 0); }
static const float pick_dir_mix_in_factor = 0.01f;
static const float pick_dir_mix_in_doun_factor = 0.01f;
void pick_dir_update(Fvector& v, const Fvector& previous_dir, const Fvector& new_dir)
{
    Fvector dir = new_dir;
    dir.mul(pick_dir_mix_in_factor);

    if (dir.y > 0)
        dir.y = -dir.y;

    dir.add(previous_dir);

    dir.add(Fvector().set(0, -0.05f, 0));

    float m = dir.magnitude();

    if (m < EPS)
        v.set(previous_dir);
    else
        v.set(dir.mul(1.f / m));

    VERIFY(_valid(v));
}

IC void CIKLimb::GetPickDir(Fvector& v, SCalculateData& cd) const
{
    v.set(0, -1, 0);
    /*
        if( !state_valide( sv_state ) )
        {
            cd.state.pick = v;
            VERIFY( _valid( v ) );
    #ifdef	DEBUG
            if( ph_dbg_draw_mask.test( phDbgIK ) )
                Msg( "prev state not valide" );
    #endif
            return;
        }
    //
        Fvector dir;
        ToeTimeDiff( dir, cd );

        Fvector lpick; sv_state.pick( lpick );

        pick_dir_update( v, lpick, dir );

        cd.state.pick =v;

    #ifdef DEBUG
        if( ph_dbg_draw_mask.test( phDbgDrawIKGoal )  )
        {
            Fvector p ; m_foot.ToePosition( p );
            cd.state.anim_pos.transform_tiny( p );
            DBG_DrawLine( p, Fvector().add( p, Fvector( ).mul( cd.state.pick, 1 ) ), color_xrgb( 255, 0, 255 ) );
        }
    #endif
    */
}

void CIKLimb::AnimGoal(Fmatrix& gl) { Kinematics()->Bone_GetAnimPos(gl, m_bones[m_foot.ref_bone()], 1 << 0, false); }
void CIKLimb::SetAnimGoal(SCalculateData& cd)
{
    AnimGoal(cd.state.anim_pos);
    cd.state.goal.set(Fmatrix().mul_43(*cd.m_obj, cd.state.anim_pos), ik_goal_matrix::cl_undefined);
    cd.state.anim_pos = cd.state.goal.get();
    cd.state.blend_to = cd.state.goal;
    cd.apply = true;
}

void CIKLimb::Update(CGameObject* O, const CBlend* b, const extrapolation::points& object_pose_extrapolation)
{
    if (!m_collide || !sv_state.valide())
    {
        collide_data.collided = false;
        return;
    }
    anim_state.update(KinematicsAnimated(), b, get_id());

    Fmatrix anim_foot;
    AnimGoal(anim_foot);

    // Fmatrix current_foot;
    // current_foot = m_foot.Kinematics( )->LL_GetTransform( m_bones[m_foot.ref_bone()] );

    // ik_goal_matrix m;
    // Fmatrix foot = Fmatrix().mul_43( Fmatrix().invert( O->XFORM() ) ,sv_state.goal( m ).get() );
    // m_foot.ref_bone_to_foot( foot )

    m_foot.Collide(collide_data, collider, anim_foot, O->XFORM(), O, anim_state.step());

    step_predict(O, b, state_predict, object_pose_extrapolation);
}

float CIKLimb::ObjShiftDown(float current_shift, const SCalculateData& cd) const
{
    Fvector hip;
    cd.m_obj->transform_tiny(hip, Kinematics()->LL_GetTransform(m_bones[0]).c);
    hip.y -= current_shift;
    Fmatrix m;
    Fvector g;
    g.sub(m_foot.ref_bone_to_foot(m, cd.state.goal.get()).c, hip);
    float l = m_limb.Length();
    return -g.y - _sqrt(l * l - g.x * g.x - g.z * g.z);
}

float CIKLimb::get_time_to_step_begin(const CBlend& B) const
{
    float time = phInfinity;
    if (anim_state.time_step_begin(KinematicsAnimated(), B, m_id, time))
        return time;
    else
        return phInfinity;
}

struct ssaved_callback : private Noncopyable
{
    ssaved_callback(CBoneInstance& bi)
        : _bi(bi), callback(bi.callback()), callback_param(bi.callback_param()),
          callback_overwrite(bi.callback_overwrite()), callback_type(bi.callback_type())
    {
    }
    void restore() { _bi.set_callback(callback_type, callback, callback_param, callback_overwrite); }
    const BoneCallback callback;
    void* callback_param;
    const BOOL callback_overwrite;
    const u32 callback_type;
    CBoneInstance& _bi;
};
static void get_matrix(CBoneInstance* P)
{
    VERIFY(_valid(P->mTransform));
    *((Fmatrix*)P->callback_param()) = P->mTransform;
}
u16 CIKLimb::foot_matrix_predict(Fmatrix& foot, Fmatrix& toe, float time, IKinematicsAnimated* K) const
{
    // CBlend *control = 0;
    u32 blends_count = K->LL_PartBlendsCount(0);
    buffer_vector<CBlend> saved_blends(_alloca(blends_count * sizeof(CBlend)), blends_count);

    for (u32 i = 0; i < blends_count; ++i)
    {
        CBlend B = *K->LL_PartBlend(0, i);
        saved_blends.push_back(B);
    }

    for (u32 i = 0; i < blends_count; ++i)
    {
        CBlend& B = *K->LL_PartBlend(0, i);
        if (B.update(time, 0))
            B.blendAmount = 0;
    }

    CBoneInstance& bi2 = Kinematics()->LL_GetBoneInstance(m_bones[2]);
    CBoneInstance& bi3 = Kinematics()->LL_GetBoneInstance(m_bones[3]);
    ssaved_callback cb2(bi2);
    ssaved_callback cb3(bi3);
    Fmatrix m_b2, m_b3;
    bi2.set_callback(bctCustom, get_matrix, &m_b2, FALSE);
    bi3.set_callback(bctCustom, get_matrix, &m_b3, FALSE);

    Kinematics()->Bone_GetAnimPos(foot, m_bones[3], u8(-1), false);
    u16 ref_b = m_foot.get_ref_bone(m_b2, m_b3);
    foot = m_b2;
    toe = m_b3;

    cb2.restore();
    cb3.restore();

    for (u32 i = 0; i < blends_count; ++i)
        *K->LL_PartBlend(0, i) = saved_blends[i];

    return ref_b;
}
void CIKLimb::step_predict(CGameObject* O, const CBlend* b, ik_limb_state_predict& state,
    const extrapolation::points& object_pose_extrapolation) // const
{
    if (!b)
        return;
    state.time_to_footstep = get_time_to_step_begin(*b);
    if (state.time_to_footstep == phInfinity)
        return;
    float footstep_time = Device.fTimeGlobal + state.time_to_footstep;

    Fmatrix footstep_object;
    object_pose_extrapolation.extrapolate(footstep_object, footstep_time);

    Fmatrix foot, toe;
    u16 ref_b = foot_matrix_predict(foot, toe, state.time_to_footstep, KinematicsAnimated());
    u16 ref_b_save = m_foot.ref_bone();
    m_foot.set_ref_bone(ref_b);
    SIKCollideData cld;
    Fmatrix m_ref_b;
    if (ref_b == 2)
        m_ref_b = foot;
    else
    {
        VERIFY(ref_b == 3);
        m_ref_b = toe;
    }
    m_foot.Collide(cld, state.collider, m_ref_b, footstep_object, O, true);
    const Fmatrix gl_goal = Fmatrix().mul_43(footstep_object, m_ref_b);

    ik_goal_matrix gl_cl_goal;
    m_foot.GetFootStepMatrix(gl_cl_goal, gl_goal, cld, false, true);

// m_foot.ref_bone_to_foot
#ifdef DEBUG
    if (ph_dbg_draw_mask1.test(phDbgDrawIKPredict))
    {
        DBG_DrawPoint(gl_cl_goal.get().c, 0.03f, color_xrgb(255, 0, 0));
        DBG_DrawPoint(gl_goal.c, 0.03f, color_xrgb(0, 0, 255));
        DBG_DrawLine(gl_cl_goal.get().c, gl_goal.c, color_xrgb(0, 255, 0));
        DBG_DrawMatrix(gl_cl_goal.get(), 0.3f);
        DBG_DrawMatrix(gl_goal, 0.5f, 155);

        DBG_DrawMatrix(O->XFORM(), 1.f);
        DBG_DrawMatrix(footstep_object, 1.f);
    }
#endif
    state.footstep_shift = gl_cl_goal.get().c.y - gl_goal.c.y;

    m_foot.set_ref_bone(ref_b_save);
}

Fmatrix& CIKLimb::GetHipInvert(Fmatrix& ihip, const SCalculateData& cd)
{
    Fmatrix H;
    CBoneData& bd = Kinematics()->LL_GetData(m_bones[0]);
    H.set(bd.bind_transform);
    H.mulA_43(Kinematics()->LL_GetTransform(bd.GetParentID()));
    H.c.set(Kinematics()->LL_GetTransform(m_bones[0]).c);
    ihip.invert(H);
    return ihip;
}

Matrix& CIKLimb::Goal(Matrix& gl, const Fmatrix& xm, const SCalculateData& cd)
{
#ifdef DEBUG
    const Fmatrix& obj = *cd.m_obj;
    if (ph_dbg_draw_mask.test(phDbgDrawIKGoal))
    {
        Fmatrix DBGG;
        DBGG.mul_43(obj, xm);
        DBG_DrawMatrix(DBGG, 0.2f);
        if (cd.do_collide)
        {
            ik_goal_matrix m;
            DBG_DrawLine(sv_state.goal(m).get().c, DBGG.c, color_xrgb(255, 0, 255));

            DBG_DrawPoint(sv_state.goal(m).get().c, 0.05, color_xrgb(255, 255, 255));
            DBG_DrawPoint(DBGG.c, 0.04, color_xrgb(0, 255, 0));
            Fvector ch;
            ch.sub(DBGG.c, sv_state.goal(m).get().c);
            if (ch.magnitude() > 0.5f)
            {
                DBG_DrawMatrix(sv_state.goal(m).get(), 3.5f);
            }
        }
        Fmatrix DBH;
        GetHipInvert(DBH, cd);
        DBH.invert();
        DBGG.mul_43(obj, DBH);
        DBG_DrawMatrix(DBGG, 0.2f);
    }
#endif

    Fmatrix H;
    GetHipInvert(H, cd);
    Fmatrix G;
    G.mul_43(H, xm);
    XM2IM(G, gl);
    return gl;
}

void CIKLimb::CalculateBones(SCalculateData& cd)
{
    VERIFY(cd.m_angles);
    IKinematics* K = cd.m_limb->Kinematics();
    ssaved_callback sv0(K->LL_GetBoneInstance(m_bones[0]));
    ssaved_callback sv1(K->LL_GetBoneInstance(m_bones[1]));
    ssaved_callback sv2(K->LL_GetBoneInstance(m_bones[2]));

    K->LL_GetBoneInstance(m_bones[0]).set_callback(bctCustom, BonesCallback0, &cd, TRUE);
    K->LL_GetBoneInstance(m_bones[1]).set_callback(bctCustom, BonesCallback1, &cd, TRUE);
    K->LL_GetBoneInstance(m_bones[2]).set_callback(bctCustom, BonesCallback2, &cd, TRUE);

    CBoneData& BD = K->LL_GetData(m_bones[0]);
    K->Bone_Calculate(&BD, &K->LL_GetTransform(BD.GetParentID()));

    sv0.restore();
    sv1.restore();
    sv2.restore();
}

void DBG_DrawRotationLimitsY(const Fmatrix& start, float ang, float l, float h)
{
#ifdef DEBUG
    DBG_DrawRotationY(start, ang - EPS, ang + EPS, 0.15f, color_xrgb(0, 255, 0), false, 1);
    DBG_DrawRotationY(start, l, h, 0.15f, color_argb(50, 0, 250, 0), true);
#endif // DEBUG
}

void DBG_DrawRotationLimitsZ(const Fmatrix& start, float ang, float l, float h)
{
#ifdef DEBUG
    DBG_DrawRotationZ(start, ang - EPS, ang + EPS, 0.15f, color_xrgb(0, 0, 255), false, 1);
    DBG_DrawRotationZ(start, l, h, 0.15f, color_argb(50, 0, 0, 250), true);
#endif // DEBUG
}

void DBG_DrawRotationLimitsX(const Fmatrix& start, float ang, float l, float h)
{
#ifdef DEBUG
    DBG_DrawRotationX(start, ang + EPS, ang - EPS, 0.15f, color_xrgb(255, 0, 0), false, 1);
    DBG_DrawRotationX(start, l, h, 0.15f, color_argb(50, 255, 0, 0), true);
#endif // DEBUG
}

void DBG_DrawRotation3(const Fmatrix& start, const float angs[7], const AngleInt limits[7], u16 y, u16 z, u16 x)
{
    Fmatrix DBGG = start;
    DBG_DrawRotationLimitsY(DBGG, -angs[y], -limits[y].Low(), -limits[y].High());
    DBGG.mulB_43(Fmatrix().rotateY(-angs[y]));
    DBG_DrawRotationLimitsZ(DBGG, -angs[z], -limits[z].Low(), -limits[z].High());
    DBGG.mulB_43(Fmatrix().rotateZ(-angs[z]));
    DBG_DrawRotationLimitsX(DBGG, -angs[x], -limits[x].Low(), -limits[x].High());
}

IC void ang_evaluate(Fmatrix& M, const float ang[3])
{
    VERIFY(_valid(ang[0]));
    VERIFY(_valid(ang[1]));
    VERIFY(_valid(ang[2]));
    Fmatrix ry;
    ry.rotateY(-ang[0]);
    Fmatrix rz;
    rz.rotateZ(-ang[1]);
    Fmatrix rx;
    rx.rotateX(-ang[2]);
    M.mul_43(Fmatrix().mul_43(ry, rz), rx);
    VERIFY(_valid(M));
}

IC void CIKLimb::get_start(Fmatrix& start, SCalculateData& D, u16 bone)
{
    CIKLimb& L = *D.m_limb;
    CBoneData& BD = L.Kinematics()->LL_GetData(L.m_bones[bone]);
    start.mul_43(L.Kinematics()->LL_GetTransform(BD.GetParentID()), BD.bind_transform);
}

void CIKLimb::BonesCallback0(CBoneInstance* B)
{
    SCalculateData* D = (SCalculateData*)B->callback_param();
    VERIFY(D);
    float const* x = D->m_angles;
    Fmatrix bm;
    ang_evaluate(bm, x);
    Fmatrix start;
    get_start(start, *D, 0);
    B->mTransform.mul_43(start, bm);

#ifdef DEBUG
    CIKLimb& L = *D->m_limb;
    if (ph_dbg_draw_mask1.test(phDbgDrawIKLimits))
        DBG_DrawRotation3(Fmatrix().mul_43(*D->m_obj, start), x, L.m_limb.jt_limits, 0, 1, 2);
    if (ph_dbg_draw_mask.test(phDbgDrawIKGoal))
    {
        DBG_DrawMatrix(Fmatrix().mul_43(*D->m_obj, start), 1.f);
        DBG_DrawMatrix(Fmatrix().mul_43(*D->m_obj, Fmatrix().mul_43(start, bm)), 0.75f);
    }
#endif
    VERIFY2(_valid(B->mTransform), "CIKLimb::BonesCallback0");
}
void CIKLimb::BonesCallback1(CBoneInstance* B)
{
    SCalculateData* D = (SCalculateData*)B->callback_param();

    float const* x = D->m_angles;
    Fmatrix bm;
    bm.rotateY(x[3]);

    Fmatrix start;
    get_start(start, *D, 1);
    B->mTransform.mul_43(start, bm);
    VERIFY2(_valid(B->mTransform), "CIKLimb::BonesCallback1");
}
void CIKLimb::BonesCallback2(CBoneInstance* B)
{
    SCalculateData* D = (SCalculateData*)B->callback_param();

    float const* x = D->m_angles;
    Fmatrix bm;
    ang_evaluate(bm, x + 4);

    Fmatrix start;
    get_start(start, *D, 2);

    VERIFY2(_valid(bm), "CIKLimb::BonesCallback2");
    VERIFY2(_valid(start), "CIKLimb::BonesCallback2");

    B->mTransform.mul_43(start, bm);

#ifdef DEBUG
    CIKLimb& L = *D->m_limb;
    if (ph_dbg_draw_mask1.test(phDbgDrawIKLimits))
    {
        DBG_DrawRotation3(Fmatrix().mul_43(*D->m_obj, start), x, L.m_limb.jt_limits, 4, 5, 6);
    }
    if (ph_dbg_draw_mask.test(phDbgDrawIKGoal))
    {
        DBG_DrawMatrix(Fmatrix().mul_43(*D->m_obj, Fmatrix().mul_43(start, bm)), 0.3f);
        DBG_DrawMatrix(Fmatrix().mul_43(*D->m_obj, start), 0.3f);
    }
#endif
    VERIFY2(_valid(B->mTransform), "CIKLimb::BonesCallback2");
}
