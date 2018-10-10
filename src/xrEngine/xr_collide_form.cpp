#include "stdafx.h"
#include "IGame_Level.h"
#include "xr_collide_form.h"
#include "xr_object.h"
#include "xrCDB/xr_area.h"
#include "x_ray.h"
#include "Common/LevelStructure.hpp"
#include "xrCore/FMesh.hpp"
#include "xrCDB/Frustum.h"

#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"
#ifdef DEBUG
IC float DET(const Fmatrix& a)
{
    return ((a._11 * (a._22 * a._33 - a._23 * a._32) - a._12 * (a._21 * a._33 - a._23 * a._31) +
        a._13 * (a._21 * a._32 - a._22 * a._31)));
}
#include "ObjectDump.h"
#endif
using namespace collide;
//----------------------------------------------------------------------
// Class : CXR_CFObject
// Purpose : stores collision form
//----------------------------------------------------------------------
ICollisionForm::ICollisionForm(IGameObject* _owner, ECollisionFormType tp)
{
    owner = _owner;
    m_type = tp;
    bv_sphere.identity();
}

ICollisionForm::~ICollisionForm() {}
//----------------------------------------------------------------------------------
void CCF_Skeleton::SElement::center(Fvector& center) const
{
    switch (type)
    {
    case SBoneShape::stBox:
        center.set(-b_IM.c.dotproduct(b_IM.i), -b_IM.c.dotproduct(b_IM.j), -b_IM.c.dotproduct(b_IM.k));
        break;
    case SBoneShape::stSphere: center.set(s_sphere.P); break;
    case SBoneShape::stCylinder: center.set(c_cylinder.m_center); break;
    default: NODEFAULT;
    }
}
bool pred_find_elem(const CCF_Skeleton::SElement& E, u16 elem) { return E.elem_id < elem; }
bool CCF_Skeleton::_ElementCenter(u16 elem_id, Fvector& e_center)
{
    auto it = std::lower_bound(elements.begin(), elements.end(), elem_id, pred_find_elem);
    if (it->elem_id == elem_id)
    {
        it->center(e_center);
        return true;
    }
    return false;
}

IC bool RAYvsOBB(const Fmatrix& IM, const Fvector& b_hsize, const Fvector& S, const Fvector& D, float& R, BOOL bCull)
{
    Fbox E = {-b_hsize.x, -b_hsize.y, -b_hsize.z, b_hsize.x, b_hsize.y, b_hsize.z};
    // XForm world-2-local
    Fvector SL, DL, PL;
    IM.transform_tiny(SL, S);
    IM.transform_dir(DL, D);

    // Actual test
    Fbox::ERP_Result rp_res = E.Pick2(SL, DL, PL);
    if ((rp_res == Fbox::rpOriginOutside) || (!bCull && (rp_res == Fbox::rpOriginInside)))
    {
        float d = PL.distance_to_sqr(SL);
        if (d < R * R)
        {
            R = _sqrt(d);
            VERIFY(R >= 0.f);
            return true;
        }
    }
    return false;
}
IC bool RAYvsSPHERE(const Fsphere& s_sphere, const Fvector& S, const Fvector& D, float& R, BOOL bCull)
{
    Fsphere::ERP_Result rp_res = s_sphere.intersect(S, D, R);
    VERIFY(R >= 0.f);
    return ((rp_res == Fsphere::rpOriginOutside) || (!bCull && (rp_res == Fsphere::rpOriginInside)));
}
IC bool RAYvsCYLINDER(const Fcylinder& c_cylinder, const Fvector& S, const Fvector& D, float& R, BOOL bCull)
{
    // Actual test
    Fcylinder::ERP_Result rp_res = c_cylinder.intersect(S, D, R);
    VERIFY(R >= 0.f);
    return ((rp_res == Fcylinder::rpOriginOutside) || (!bCull && (rp_res == Fcylinder::rpOriginInside)));
}

CCF_Skeleton::CCF_Skeleton(IGameObject* O) : ICollisionForm(O, cftObject)
{
    // getVisData
    IRenderVisual* pVisual = O->Visual();
    // IKinematics* K = PKinematics(pVisual); VERIFY3(K,"Can't create skeleton without Kinematics.",*O->cNameVisual());
    IKinematics* K = PKinematics(pVisual);
    VERIFY3(K, "Can't create skeleton without Kinematics.", *O->cNameVisual());
    // bv_box.set (K->vis.box);
    bv_box.set(pVisual->getVisData().box);
    bv_box.getsphere(bv_sphere.P, bv_sphere.R);
    vis_mask = 0;
}

void CCF_Skeleton::BuildState()
{
    dwFrame = Device.dwFrame;
    IRenderVisual* pVisual = owner->Visual();
    IKinematics* K = PKinematics(pVisual);
    K->CalculateBones();
    const Fmatrix& L2W = owner->XFORM();

    if (vis_mask != K->LL_GetBonesVisible())
    {
        vis_mask = K->LL_GetBonesVisible();
        elements.clear();
        bv_box.set(pVisual->getVisData().box);
        bv_box.getsphere(bv_sphere.P, bv_sphere.R);
        for (u16 i = 0; i < K->LL_BoneCount(); i++)
        {
            if (!K->LL_GetBoneVisible(i))
                continue;
            SBoneShape& shape = K->LL_GetData(i).shape;
            if (SBoneShape::stNone == shape.type)
                continue;
            if (shape.flags.is(SBoneShape::sfNoPickable))
                continue;
            elements.push_back(SElement(i, shape.type));
        }
    }

    for (auto I = elements.begin(); I != elements.end(); I++)
    {
        if (!I->valid())
            continue;
        SBoneShape& shape = K->LL_GetData(I->elem_id).shape;
        Fmatrix ME, T, TW;
        const Fmatrix& Mbone = K->LL_GetTransform(I->elem_id);

        VERIFY2(DET(Mbone) > EPS,
            (make_string("0 scale bone matrix, %d \n", I->elem_id) + dbg_object_full_dump_string(owner)).c_str());

        switch (I->type)
        {
        case SBoneShape::stBox:
        {
            const Fobb& B = shape.box;
            B.xform_get(ME);

            // VERIFY2( DET(ME)>EPS, ( make_string("0 scale bone matrix, %d \n", I->elem_id ) +
            // dbg_object_full_dump_string( owner ) ).c_str() );

            I->b_hsize.set(B.m_halfsize);
            // prepare matrix World to Element
            T.mul_43(Mbone, ME); // model space
            TW.mul_43(L2W, T); // world space
            bool b = I->b_IM.invert_b(TW);
            // check matrix validity
            if (!b)
            {
                Msg("! ERROR: invalid bone xform . Bone disabled.");
                Msg("! ERROR: bone_id=[%d], world_pos[%f,%f,%f]", I->elem_id, VPUSH(TW.c));
                Msg("visual name %s", owner->cNameVisual().c_str());
                Msg("object name %s", owner->cName().c_str());
#ifdef DEBUG
                Msg(dbg_object_full_dump_string(owner).c_str());
#endif //#ifdef DEBUG
                I->elem_id = u16(-1); //. hack - disable invalid bone
            }
        }
        break;
        case SBoneShape::stSphere:
        {
            const Fsphere& S = shape.sphere;
            Mbone.transform_tiny(I->s_sphere.P, S.P);
            L2W.transform_tiny(I->s_sphere.P);
            I->s_sphere.R = S.R;
        }
        break;
        case SBoneShape::stCylinder:
        {
            const Fcylinder& C = shape.cylinder;
            Mbone.transform_tiny(I->c_cylinder.m_center, C.m_center);
            L2W.transform_tiny(I->c_cylinder.m_center);
            Mbone.transform_dir(I->c_cylinder.m_direction, C.m_direction);
            L2W.transform_dir(I->c_cylinder.m_direction);
            I->c_cylinder.m_height = C.m_height;
            I->c_cylinder.m_radius = C.m_radius;
        }
        break;
        }
    }
}

void CCF_Skeleton::BuildTopLevel()
{
    dwFrameTL = Device.dwFrame;
    IRenderVisual* K = owner->Visual();
    vis_data& vis = K->getVisData();
    Fbox& B = vis.box;
    bv_box.vMin.average(B.vMin);
    bv_box.vMax.average(B.vMax);
    bv_box.grow(0.05f);
    bv_sphere.P.average(vis.sphere.P);
    bv_sphere.R += vis.sphere.R;
    bv_sphere.R *= 0.5f;
    VERIFY(_valid(bv_sphere));
}

BOOL CCF_Skeleton::_RayQuery(const collide::ray_defs& Q, collide::rq_results& R)
{
    if (dwFrameTL != Device.dwFrame)
        BuildTopLevel();

    Fsphere w_bv_sphere;
    owner->XFORM().transform_tiny(w_bv_sphere.P, bv_sphere.P);
    w_bv_sphere.R = bv_sphere.R;

    //
    float tgt_dist = Q.range;
    float aft[2];
    int quant;
    Fsphere::ERP_Result res1 = w_bv_sphere.intersect(Q.start, Q.dir, tgt_dist, quant, aft);
    if ((Fsphere::rpNone == res1) || ((Fsphere::rpOriginOutside == res1) && (aft[0] > tgt_dist)))
        return FALSE;

    if (dwFrame != Device.dwFrame)
        BuildState();
    else
    {
        IKinematics* K = PKinematics(owner->Visual());
        if (K->LL_GetBonesVisible() != vis_mask)
        {
            // Model changed between ray-picks
            dwFrame = Device.dwFrame - 1;
            BuildState();
        }
    }

    BOOL bHIT = FALSE;
    for (auto I = elements.begin(); I != elements.end(); I++)
    {
        if (!I->valid())
            continue;
        bool res = false;
        float range = Q.range;
        switch (I->type)
        {
        case SBoneShape::stBox:
            res = RAYvsOBB(I->b_IM, I->b_hsize, Q.start, Q.dir, range, Q.flags & CDB::OPT_CULL);
            break;
        case SBoneShape::stSphere:
            res = RAYvsSPHERE(I->s_sphere, Q.start, Q.dir, range, Q.flags & CDB::OPT_CULL);

            break;
        case SBoneShape::stCylinder:
            res = RAYvsCYLINDER(I->c_cylinder, Q.start, Q.dir, range, Q.flags & CDB::OPT_CULL);
            break;
        }
        if (res)
        {
            bHIT = TRUE;
            R.append_result(owner, range, I->elem_id, Q.flags & CDB::OPT_ONLYNEAREST);
            if (Q.flags & CDB::OPT_ONLYFIRST)
                break;
        }
    }
    return bHIT;
}

//----------------------------------------------------------------------------------
CCF_EventBox::CCF_EventBox(IGameObject* O) : ICollisionForm(O, cftShape)
{
    Fvector A[8], B[8];
    A[0].set(-1, -1, -1);
    A[1].set(-1, -1, +1);
    A[2].set(-1, +1, +1);
    A[3].set(-1, +1, -1);
    A[4].set(+1, +1, +1);
    A[5].set(+1, +1, -1);
    A[6].set(+1, -1, +1);
    A[7].set(+1, -1, -1);

    const Fmatrix& T = O->XFORM();
    for (int i = 0; i < 8; i++)
    {
        A[i].mul(.5f);
        T.transform_tiny(B[i], A[i]);
    }
    bv_box.set(-.5f, -.5f, -.5f, +.5f, +.5f, +.5f);
    Fvector R;
    R.set(bv_box.vMin);
    T.transform_dir(R);
    bv_sphere.R = R.magnitude();

    Planes[0].build(B[0], B[3], B[5]);
    Planes[1].build(B[1], B[2], B[3]);
    Planes[2].build(B[6], B[5], B[4]);
    Planes[3].build(B[4], B[2], B[1]);
    Planes[4].build(B[3], B[2], B[4]);
    Planes[5].build(B[1], B[0], B[6]);
}

BOOL CCF_EventBox::Contact(IGameObject* O)
{
    IRenderVisual* V = O->Visual();
    vis_data& vis = V->getVisData();
    Fvector& P = vis.sphere.P;
    float R = vis.sphere.R;

    Fvector PT;
    O->XFORM().transform_tiny(PT, P);

    for (int i = 0; i < 6; i++)
    {
        if (Planes[i].classify(PT) > R)
            return FALSE;
    }
    return TRUE;
}
BOOL CCF_EventBox::_RayQuery(const collide::ray_defs& Q, collide::rq_results& R) { return FALSE; }
/*
void CCF_EventBox::_BoxQuery(const Fbox& B, const Fmatrix& M, u32 flags)
{ return; }
*/

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
CCF_Shape::CCF_Shape(IGameObject* _owner) : ICollisionForm(_owner, cftShape) {}
BOOL CCF_Shape::_RayQuery(const collide::ray_defs& Q, collide::rq_results& R)
{
    // Convert ray into local model space
    Fvector dS, dD;
    Fmatrix temp;
    temp.invert(owner->XFORM());
    temp.transform_tiny(dS, Q.start);
    temp.transform_dir(dD, Q.dir);

    //
    if (!bv_sphere.intersect(dS, dD))
        return FALSE;

    BOOL bHIT = FALSE;
    for (u32 el = 0; el < shapes.size(); el++)
    {
        shape_def& shape = shapes[el];
        float range = Q.range;
        switch (shape.type)
        {
        case 0:
        {
            // sphere
            Fsphere::ERP_Result rp_res = shape.data.sphere.intersect(dS, dD, range);
            if ((rp_res == Fsphere::rpOriginOutside) ||
                (!(Q.flags & CDB::OPT_CULL) && (rp_res == Fsphere::rpOriginInside)))
            {
                bHIT = TRUE;
                R.append_result(owner, range, el, Q.flags & CDB::OPT_ONLYNEAREST);
                if (Q.flags & CDB::OPT_ONLYFIRST)
                    return TRUE;
            }
        }
        break;
        case 1: // box
        {
            Fbox box;
            box.identity();
            Fmatrix& B = shape.data.ibox;
            Fvector S1, D1, P;
            B.transform_tiny(S1, dS);
            B.transform_dir(D1, dD);
            Fbox::ERP_Result rp_res = box.Pick2(S1, D1, P);
            if ((rp_res == Fbox::rpOriginOutside) || (!(Q.flags & CDB::OPT_CULL) && (rp_res == Fbox::rpOriginInside)))
            {
                float d = P.distance_to_sqr(dS);
                if (d < range * range)
                {
                    range = _sqrt(d);
                    bHIT = TRUE;
                    R.append_result(owner, range, el, Q.flags & CDB::OPT_ONLYNEAREST);
                    if (Q.flags & CDB::OPT_ONLYFIRST)
                        return TRUE;
                }
            }
        }
        break;
        }
    }
    return bHIT;
}
/*
void CCF_Shape::_BoxQuery(const Fbox& B, const Fmatrix& M, u32 flags)
{ return; }
*/
void CCF_Shape::add_sphere(Fsphere& S)
{
    shapes.push_back(shape_def());
    shapes.back().type = 0;
    shapes.back().data.sphere.set(S);
}

void CCF_Shape::add_box(Fmatrix& B)
{
    shapes.push_back(shape_def());
    shapes.back().type = 1;
    shapes.back().data.box.set(B);
    shapes.back().data.ibox.invert(B);
}

void CCF_Shape::ComputeBounds()
{
    bv_box.invalidate();

    BOOL bCalcSphere = (shapes.size() > 1);
    for (u32 el = 0; el < shapes.size(); el++)
    {
        switch (shapes[el].type)
        {
        case 0: // sphere
        {
            Fsphere T = shapes[el].data.sphere;
            Fvector P;
            P.set(T.P);
            P.sub(T.R);
            bv_box.modify(P);
            P.set(T.P);
            P.add(T.R);
            bv_box.modify(P);
            bv_sphere = T;
        }
        break;
        case 1: // box
        {
            Fvector A, B;
            Fmatrix& T = shapes[el].data.box;

            // Build points
            A.set(-.5f, -.5f, -.5f);
            T.transform_tiny(B, A);
            bv_box.modify(B);
            A.set(-.5f, -.5f, +.5f);
            T.transform_tiny(B, A);
            bv_box.modify(B);
            A.set(-.5f, +.5f, +.5f);
            T.transform_tiny(B, A);
            bv_box.modify(B);
            A.set(-.5f, +.5f, -.5f);
            T.transform_tiny(B, A);
            bv_box.modify(B);
            A.set(+.5f, +.5f, +.5f);
            T.transform_tiny(B, A);
            bv_box.modify(B);
            A.set(+.5f, +.5f, -.5f);
            T.transform_tiny(B, A);
            bv_box.modify(B);
            A.set(+.5f, -.5f, +.5f);
            T.transform_tiny(B, A);
            bv_box.modify(B);
            A.set(+.5f, -.5f, -.5f);
            T.transform_tiny(B, A);
            bv_box.modify(B);

            bCalcSphere = TRUE;
        }
        break;
        }
    }
    if (bCalcSphere)
        bv_box.getsphere(bv_sphere.P, bv_sphere.R);
}

BOOL CCF_Shape::Contact(IGameObject* O)
{
    // Build object-sphere in World-Space
    Fsphere S;
    if (O->Visual())
    {
        O->Center(S.P);
        S.R = O->Radius();
    }
    else if (O->GetCForm())
    {
        S = O->GetCForm()->getSphere();
        O->XFORM().transform_tiny(S.P);
    }
    else
        return FALSE;

    // Get our matrix
    const Fmatrix& XF = Owner()->XFORM();

    // Iterate
    for (u32 el = 0; el < shapes.size(); el++)
    {
        switch (shapes[el].type)
        {
        case 0: // sphere
        {
            Fsphere Q;
            Fsphere& T = shapes[el].data.sphere;
            XF.transform_tiny(Q.P, T.P);
            Q.R = T.R;
            if (S.intersect(Q))
                return TRUE;
        }
        break;
        case 1: // box
        {
            Fmatrix Q;
            Fmatrix& T = shapes[el].data.box;
            Q.mul_43(XF, T);

            // Build points
            Fvector A, B[8];
            Fplane P;
            A.set(-.5f, -.5f, -.5f);
            Q.transform_tiny(B[0], A);
            A.set(-.5f, -.5f, +.5f);
            Q.transform_tiny(B[1], A);
            A.set(-.5f, +.5f, +.5f);
            Q.transform_tiny(B[2], A);
            A.set(-.5f, +.5f, -.5f);
            Q.transform_tiny(B[3], A);
            A.set(+.5f, +.5f, +.5f);
            Q.transform_tiny(B[4], A);
            A.set(+.5f, +.5f, -.5f);
            Q.transform_tiny(B[5], A);
            A.set(+.5f, -.5f, +.5f);
            Q.transform_tiny(B[6], A);
            A.set(+.5f, -.5f, -.5f);
            Q.transform_tiny(B[7], A);

            P.build(B[0], B[3], B[5]);
            if (P.classify(S.P) > S.R)
                break;
            P.build(B[1], B[2], B[3]);
            if (P.classify(S.P) > S.R)
                break;
            P.build(B[6], B[5], B[4]);
            if (P.classify(S.P) > S.R)
                break;
            P.build(B[4], B[2], B[1]);
            if (P.classify(S.P) > S.R)
                break;
            P.build(B[3], B[2], B[4]);
            if (P.classify(S.P) > S.R)
                break;
            P.build(B[1], B[0], B[6]);
            if (P.classify(S.P) > S.R)
                break;
            return TRUE;
        }
        break;
        }
    }
    return FALSE;
}
