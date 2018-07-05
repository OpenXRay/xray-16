#include "stdafx.h"

#include "ikfoot.h"

#include "ik_collide_data.h"
#include "GameObject.h"

#include "xrPhysics/MathUtils.h"
#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrEngine/ennumerateVertices.h"
#include "Common/Noncopyable.hpp"

#ifdef DEBUG
#include "PHDebug.h"
#endif

CIKFoot::CIKFoot()
    : m_K(nullptr), m_bind_b2_to_b3(Fidentity), m_foot_width(0),
      m_ref_bone(u16(-1)), m_foot_bone_id(BI_NONE), m_toe_bone_id(BI_NONE) {}

void CIKFoot::Create(IKinematics* K, LPCSTR section, u16 bones[4])
{
    VERIFY(K);
    m_K = K;

    /// defaults
    m_ref_bone = 2;
    if (m_ref_bone == 2)
    {
        m_foot_normal.v.set(1, 0, 0); // 2
        m_foot_normal.bone = 2;
        m_foot_direction.v.set(0, 0, 1); // 2
        m_foot_direction.bone = 2;
    }
    else
    {
        m_foot_normal.v.set(0, 0, -1); // 3
        m_foot_normal.bone = 3;
        m_foot_direction.v.set(1, 0, 0); // 3
        m_foot_direction.bone = 3;
    }

    //	m_foot_normal.v			.set( 1, 0, 0 );//2
    //	m_foot_normal.bone		= 2;

    // load settings
    if (section)
    {
        if (!!K->LL_UserData()->r_bool(section, "align_toe"))
            m_ref_bone = 3;
        m_foot_normal.bone = m_ref_bone;
        m_foot_direction.bone = m_ref_bone;

        m_foot_normal.v = Kinematics()->LL_UserData()->r_fvector3(section, "foot_normal");
        m_foot_direction.v = Kinematics()->LL_UserData()->r_fvector3(section, "foot_direction");
    }
    set_toe(bones);
}

struct envc : private Noncopyable, public SEnumVerticesCallback
{
    Fvector& pos;
    Fvector start_pos;
    const Fmatrix& i_bind_transform;
    const Fvector& ax;
    envc(const Fmatrix& _i_bind_transform, const Fvector& _ax, Fvector& _pos)
        : SEnumVerticesCallback(), i_bind_transform(_i_bind_transform), ax(_ax), pos(_pos)
    {
        start_pos.set(0, 0, 0);
    }
    void operator()(const Fvector& p)
    {
        Fvector lpos;
        i_bind_transform.transform_tiny(lpos, p);
        // Fvector diff;diff.sub( lpos, pos );
        if (Fvector().sub(lpos, start_pos).dotproduct(ax) > Fvector().sub(pos, start_pos).dotproduct(ax))
            pos.set(lpos);
    }
};
void CIKFoot::set_toe(u16 bones[4])
{
    VERIFY(Kinematics());

    m_foot_bone_id = bones[2];
    m_toe_bone_id = bones[3];

    xr_vector<Fmatrix> binds;
    Kinematics()->LL_GetBindTransform(binds);

    const Fmatrix bind_ref = binds[bones[m_ref_bone]];
    const Fmatrix ibind_ref = Fmatrix().invert(bind_ref);

    const Fmatrix bind2 = binds[bones[2]];
    const Fmatrix ibind2 = Fmatrix().invert(bind2);

    // const Fmatrix ref_to_b2	= Fmatrix().mul_43( ibind2, bind_ref );
    const Fmatrix b2to_ref = Fmatrix().mul_43(ibind_ref, bind2);

    const Fmatrix bind3 = binds[bones[3]];
    const Fmatrix ibind3 = Fmatrix().invert(bind3);

    m_bind_b2_to_b3.mul_43(ibind2, bind3);
    ///////////////////////////////////////////////////////
    Fvector ax, foot_normal, foot_dir;
    get_local_vector(2, foot_normal, m_foot_normal);
    get_local_vector(2, foot_dir, m_foot_direction);

    // ref_to_b2.transform_tiny( foot_normal, m_foot_normal.v );
    // ref_to_b2.transform_tiny( foot_dir, m_foot_direction.v );

    ax.add(foot_normal, foot_dir);
    ax.normalize();
    ///////////////////////////////////////////////////////
    Fvector pos;
    pos.set(0, 0, 0);
    Fmatrix ibind = ibind3;
    envc pred(ibind, ax, pos);
    /////////////////////////////////////////////////////////
    Kinematics()->EnumBoneVertices(pred, bones[3]);
    bind3.transform_tiny(pos);
    ibind2.transform_tiny(pos);
    m_toe_position.v.set(pos);
    /////////////////////////////////////////////////////////
    ibind.set(ibind2);
    ax.set(foot_normal);
    Kinematics()->EnumBoneVertices(pred, bones[2]);
    m_toe_position.v.x = _max(pos.x, m_toe_position.v.x);
    /////////////////////////////////////////////////////////
    ax.sub(foot_normal, foot_dir);
    ax.normalize();
    pred.start_pos.set(0, 0, 0);
    pos.set(0, 0, 0);
    Kinematics()->EnumBoneVertices(pred, bones[2]);
    m_heel_position.v = pred.pos;
    m_heel_position.v.add(Fvector().mul(foot_dir, Fvector().sub(m_toe_position.v, pos).dotproduct(foot_dir) * 0.2f));
    m_heel_position.bone = 2;
    ///////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////
    bind2.transform_tiny(m_toe_position.v);
    ibind_ref.transform_tiny(m_toe_position.v);
    m_toe_position.bone = ref_bone();
    /////////////////////////////////////////////////////////

    get_local_vector(foot_normal, m_foot_normal);
    m_foot_width = (Fvector().sub(m_toe_position.v, b2to_ref.c)).dotproduct(foot_normal);
}

Fmatrix& CIKFoot::foot_to_ref_bone_transform(Fmatrix& m) const
{
    if (m_ref_bone == 2)
    {
        m.set(Fidentity);
        return m;
    }
    m.mul_43(
        Fmatrix().invert(Kinematics()->LL_GetTransform(m_foot_bone_id)), Kinematics()->LL_GetTransform(m_toe_bone_id));
    return m;
}

Fmatrix& CIKFoot::ref_bone_to_foot_transform(Fmatrix& m) const
{
    return m.invert(Fmatrix().set(foot_to_ref_bone_transform(m)));
}

Fmatrix& CIKFoot::foot_to_ref_bone(Fmatrix& ref_bone, const Fmatrix& foot) const
{
    if (m_ref_bone == 2)
    {
        ref_bone = foot;
        return ref_bone;
    }
    Fmatrix m;
    foot_to_ref_bone_transform(m);
    ref_bone.mul_43(foot, m);
    return ref_bone;
}

Fmatrix& CIKFoot::foot_to_ref_bone(Fmatrix& m) const { return foot_to_ref_bone(m, Fmatrix().set(m)); }
Fmatrix& CIKFoot::ref_bone_to_foot(Fmatrix& foot, const Fmatrix& ref_bone) const
{
    if (m_ref_bone == 2)
    {
        foot = ref_bone;
        return foot;
    }
    Fmatrix m;
    ref_bone_to_foot_transform(m);
    /*
        Fmatrix b2 = Kinematics()->LL_GetTransform( m_bones[2] );
        Fmatrix b3 = Kinematics()->LL_GetTransform( m_bones[3] );
        //m.mul_43( Fmatrix().invert( Kinematics()->LL_GetTransform(m_bones[2] ) ),Kinematics()->LL_GetTransform(
       m_bones[3] ) );

        Fmatrix ib3; ib3.invert( b3 );
        Fmatrix ib2; ib2.invert( b2 );
        m.mul_43( ib3, b2  );
        m.mul_43( ib2, b3  );
        m.invert();
    */
    foot.mul_43(ref_bone, m);
    return foot;
}

Fmatrix& CIKFoot::ref_bone_to_foot(Fmatrix& m) const { return ref_bone_to_foot(m, Fmatrix().set(m)); }
int ik_allign_free_foot = 0;
ik_goal_matrix::e_collide_state CIKFoot::CollideFoot(float angle, float& out_angle, const Fvector& global_toe,
    const Fvector& foot_normal, const Fvector& global_bone_pos, const Fplane& p, const Fvector& ax) const
{
    float dfoot_tri = -p.d - p.n.dotproduct(global_bone_pos); // dist from foot bone pos to tri plain
    Fvector axp;
    axp.sub(global_toe, global_bone_pos);
    float dfoot_toe = p.n.dotproduct(axp);
    out_angle = angle;
    if (dfoot_tri < m_foot_width * _abs(foot_normal.dotproduct(p.n)))
        return ik_goal_matrix::cl_aligned;
    axp.sub(Fvector().mul(ax, axp.dotproduct(ax))); // vector from nc_toe to ax
    float dtoe_ax = axp.magnitude();

    out_angle = 0.f;

    if (dtoe_ax < EPS_S)
        return ik_goal_matrix::cl_free;
    if (dfoot_toe > dtoe_ax - EPS_S)
        return ik_goal_matrix::cl_free;
    if (dfoot_toe < dfoot_tri)
        return ik_goal_matrix::cl_free;

    float ang_nc = acosf(dfoot_toe / dtoe_ax);
    float ang_c = acosf(dfoot_tri / dtoe_ax);
    out_angle = -(ang_c - ang_nc);
    return ik_goal_matrix::cl_rotational;
}

static const float min_dot = 0.9f; // M_SQRT1_2;//M_SQRT1_2;

bool CIKFoot::make_shift(
    Fmatrix& xm, const Fvector& cl_point, bool collide, const Fplane& p, const Fvector& pick_dir) const
{
    Fvector shift = pick_dir;

    // Fvector toe; ToePosition( toe ); xm.transform_tiny( toe );
    Fvector point;
    xm.transform_tiny(point, cl_point);
    float dot = p.n.dotproduct(shift);
    if (_abs(dot) < min_dot)
    {
        shift.add(Fvector().mul(p.n, min_dot - _abs(dot)));
        dot = p.n.dotproduct(shift);
    }
    VERIFY(!fis_zero(dot));
    float shift_m = (-p.d - p.n.dotproduct(point)) / dot;
    if (collide && shift_m > 0.f)
        return false;
    clamp(shift_m, -collide_dist, collide_dist);
    shift.mul(shift_m);
    xm.c.add(shift);
#if 0
	if(shift_m > 0.f)
	{
		DBG_OpenCashedDraw();
		DBG_DrawLine( toe, Fvector().add( toe, shift ), color_xrgb( 255, 255, 255 )  );
		DBG_ClosedCashedDraw( 1000 );
	}
#endif
    return true;
}

bool CIKFoot::GetFootStepMatrix(
    ik_goal_matrix& m, const SCalculateData& cd, const SIKCollideData& cld, bool collide, bool rotate) const
{
    return GetFootStepMatrix(m, cd.state.anim_pos, cld, collide, rotate);
}

ik_goal_matrix::e_collide_state CIKFoot::rotate(
    Fmatrix& xm, const Fplane& p, const Fvector& foot_normal, const Fvector& global_point, bool collide) const
{
    Fvector ax;
    ax.crossproduct(p.n, foot_normal);
    float s = ax.magnitude();
    clamp(s, 0.f, 1.f);
    float angle = asinf(-s);
    VERIFY(_valid(angle));
    clamp<float>(angle, -M_PI / 6, M_PI / 6);
    ik_goal_matrix::e_collide_state cl_state = ik_goal_matrix::cl_undefined;
    if (!fis_zero(s))
    {
        cl_state = ik_goal_matrix::cl_aligned;
        ax.mul(1.f / s);
        ref_bone_to_foot(xm);
        if (collide)
            cl_state = CollideFoot(angle, angle, global_point, foot_normal, xm.c, p, ax);
        // if( cld.m_pick_dir )
        Fvector c = xm.c;
        xm.mulA_43(Fmatrix().rotation(ax, angle));
        xm.c = c;
        foot_to_ref_bone(xm);
    }
    return cl_state;
}

bool CIKFoot::GetFootStepMatrix(ik_goal_matrix& m, const Fmatrix& g_anim, const SIKCollideData& cld, bool collide,
    bool rotation, bool b_make_shift /*=true*/) const
{
    const Fmatrix global_anim = g_anim;
    Fvector local_point;
    ToePosition(local_point); // toe position in bone[2] space
    Fvector global_point;
    global_anim.transform_tiny(global_point, local_point); // non collided toe in global space
    Fvector foot_normal;
    FootNormal(foot_normal);
    global_anim.transform_dir(foot_normal);
#ifdef DEBUG
// if( ph_dbg_draw_mask.test( phDbgDrawIKGoal ) )
//{
//	DBG_DrawLine( global_point, Fvector().add( global_point, foot_normal ), color_xrgb( 0, 255, 255) );
//}
#endif
    if (cld.m_collide_point == ik_foot_geom::heel || cld.m_collide_point == ik_foot_geom::side)
    {
        Fmatrix foot;
        ref_bone_to_foot(foot, g_anim);
        Fvector heel;
        HeelPosition(heel);
        foot.transform_tiny(global_point, heel);
#ifdef DEBUG
        if (ph_dbg_draw_mask.test(phDbgDrawIKGoal))
            DBG_DrawPoint(global_point, 0.01, color_xrgb(0, 255, 255));
#endif
        Fmatrix foot_to_ref;
        ref_bone_to_foot_transform(foot_to_ref).transform_tiny(local_point, heel);
    }

    float dtoe_tri = -cld.m_plane.d - cld.m_plane.n.dotproduct(global_point);
    if (!cld.collided || _abs(dtoe_tri) > collide_dist)
    {
        m.set(global_anim, ik_goal_matrix::cl_free);
        return false;
    }

    Fplane p = cld.m_plane;
    Fmatrix xm;
    xm.set(global_anim);
    ik_goal_matrix::e_collide_state cl_state = ik_goal_matrix::cl_undefined;
    if (rotation) //! collide || ik_allign_free_foot
        cl_state = rotate(xm, p, foot_normal, global_point, collide);

    if (b_make_shift && make_shift(xm, local_point, collide, p, cld.m_pick_dir))
        switch (cl_state)
        {
        case ik_goal_matrix::cl_aligned: break;
        case ik_goal_matrix::cl_undefined:
        case ik_goal_matrix::cl_free: cl_state = ik_goal_matrix::cl_translational; break;
        case ik_goal_matrix::cl_rotational: cl_state = ik_goal_matrix::cl_mixed; break;
        default: NODEFAULT;
        }
    else if (cl_state == ik_goal_matrix::cl_undefined)
        cl_state = ik_goal_matrix::cl_free;

    VERIFY(_valid(xm));
    m.set(xm, cl_state);
#ifdef DEBUG
    if (ph_dbg_draw_mask.test(phDbgDrawIKGoal))
    {
        DBG_DrawPoint(global_point, 0.03f, color_rgba(255, 0, 0, 255));
    }
    if (!fsimilar(_abs(DET(g_anim) - 1.f), _abs(DET(m.get()) - 1.f), 0.001f))
        Msg("scale g_anim: %f scale m: %f ", DET(g_anim), DET(m.get()));
#endif

    return true;
}

u16 CIKFoot::get_ref_bone(const Fmatrix& foot_transform, const Fmatrix& toe_transform) const
{
    Fvector n0, n1;
    get_local_vector(2, n0, m_foot_normal);
    get_local_vector(3, n1, m_foot_normal);

    foot_transform.transform_tiny(n0);
    toe_transform.transform_tiny(n1);

    n0.normalize();
    n1.normalize();
    if (n0.dotproduct(n1) < 0.99f)
        return 3;
    else
        return 2;
}
void CIKFoot::set_ref_bone()
{
    set_ref_bone(
        get_ref_bone(Kinematics()->LL_GetTransform(m_foot_bone_id), Kinematics()->LL_GetTransform(m_toe_bone_id)));
}
void CIKFoot::SetFootGeom(ik_foot_geom& fg, const Fmatrix& ref_bone, const Fmatrix& object_matrix) const
{
    Fmatrix gl_bone;
    gl_bone.mul_43(object_matrix, ref_bone);

    Fvector pos_toe;
    ToePosition(pos_toe);
    gl_bone.transform_tiny(pos_toe);

    Fvector heel;
    Fvector pos_hill;
    Fmatrix foot = (Fmatrix().mul_43(object_matrix, ref_bone_to_foot(foot, ref_bone)));
    foot.transform_tiny(pos_hill, HeelPosition(heel));
    const Fvector v_m = Fvector().add(pos_toe, pos_hill).mul(0.5f);

    Fvector normal, direction;
    get_local_vector(normal, m_foot_normal);
    get_local_vector(direction, m_foot_direction);

    Fvector v_side = Fvector().crossproduct(normal, direction);
    gl_bone.transform_dir(v_side);
    float vsm = v_side.magnitude();
    VERIFY(vsm > EPS_L);
    v_side.mul(Fvector().sub(pos_toe, pos_hill).magnitude() / vsm);

    fg.set(pos_toe, pos_hill, Fvector().add(v_m, v_side));
}
void CIKFoot::Collide(SIKCollideData& cld, ik_foot_collider& collider, const Fmatrix& ref_bone,
    const Fmatrix& object_matrix, CGameObject* O, bool foot_step) const
{
    VERIFY(O->Visual()->dcast_PKinematics() == Kinematics());

    ik_foot_geom fg;
    SetFootGeom(fg, ref_bone, object_matrix);
    collider.collide(cld, fg, O, foot_step);
}
