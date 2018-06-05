/////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "PHCapture.h"
#include "PHCharacter.h"
#include "Physics.h"
#include "ExtendedGeom.h"

#include "Include/xrRender/Kinematics.h"
#include "IPhysicsShellHolder.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrEngine/device.h"
#include "MathUtilsOde.h"
#include "PHElement.h"

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
IPHCapture* phcapture_create(CPHCharacter* ch, IPhysicsShellHolder* object, NearestToPointCallback* cb /*=0*/)
{
    // m_capture=new CPHCapture(m_character,
    //							 object,
    //							 cb
    //							 );
    VERIFY(ch);
    // VERIFY( object );
    // VERIFY( cb );
    return new CPHCapture(ch, object, cb);
}
IPHCapture* phcapture_create(CPHCharacter* ch, IPhysicsShellHolder* object, u16 element)
{
    VERIFY(ch);
    return new CPHCapture(ch, object, element);
}
void phcapture_destroy(IPHCapture*& c)
{
    CPHCapture* capture = smart_cast<CPHCapture*>(c);
    xr_delete(capture);
    c = 0;
}

void CPHCapture::CreateBody()
{
    m_body = dBodyCreate(0);
    m_island.AddBody(m_body);
    dMass m;
    dMassSetSphere(&m, 1.f, 1000000.f);
    dMassAdjust(&m, 100000.f);
    dBodySetMass(m_body, &m);
    dBodySetGravityMode(m_body, 0);
}

CPHCapture::~CPHCapture() { Deactivate(); }
bool CPHCapture::Invalid()
{
    return !m_taget_object->ObjectPPhysicsShell() || !m_taget_object->ObjectPPhysicsShell()->isActive() ||
        !m_character->b_exist;
}

void CPHCapture::PhDataUpdate(dReal /**step/**/)
{
    switch (e_state)
    {
    case cstFree: break;
    case cstPulling: PullingUpdate(); break;
    case cstCaptured: CapturedUpdate(); break;
    case cstReleased: ReleasedUpdate(); break;
    default: NODEFAULT;
    }
}

void CPHCapture::PhTune(dReal /**step/**/)
{
    if (e_state == cstFree)
        return;

    // if(!m_taget_object->PPhysicsShell())	{
    //	b_failed=true;
    //	return;			//. hack
    //}
    VERIFY(m_character && m_character->b_exist);
    VERIFY(m_taget_object);
    VERIFY(m_taget_object->ObjectPPhysicsShell());
    VERIFY(m_taget_object->ObjectPPhysicsShell()->isFullActive());
    VERIFY(m_taget_element);
    VERIFY(m_taget_element->isFullActive());
    VERIFY(m_island.DActiveIsland() == &m_island);

    bool act_capturer = m_character->CPHObject::is_active();
    bool act_taget = m_taget_object->ObjectPPhysicsShell()->isEnabled();

    b_disabled = !act_capturer && !act_taget;
    if (act_capturer)
    {
        m_taget_element->Enable();
    }
    if (act_taget)
    {
        m_character->Enable();
    }
    switch (e_state)
    {
    case cstPulling:; break;
    case cstCaptured:
    {
        if (b_disabled)
            dBodyDisable(m_body);
        else
        {
            m_character->Island().Merge(&m_island);
            m_taget_element->PhysicsShell()->PIsland()->Merge(&m_island);
            VERIFY(!m_island.IsActive());
        }
    }
    break;
    case cstReleased:; break;
    default: NODEFAULT;
    }
}

void CPHCapture::PullingUpdate()
{
    if (!m_taget_element->isActive() || inl_ph_world().Device().dwTimeGlobal - m_time_start > m_capture_time)
    {
        Release();
        return;
    }

    Fvector dir;
    Fvector capture_bone_position;
    // IGameObject* object=smart_cast<IGameObject*>(m_character->PhysicsRefObject());
    capture_bone_position.set(m_capture_bone->mTransform.c);
    m_character->PhysicsRefObject()->ObjectXFORM().transform_tiny(capture_bone_position);
    m_taget_element->GetGlobalPositionDynamic(&dir);
    dir.sub(capture_bone_position, dir);
    float dist = dir.magnitude();
    if (dist > m_pull_distance)
    {
        Release();
        return;
    }
    dir.mul(1.f / dist);
    if (dist < m_capture_distance)
    {
        m_back_force = 0.f;

        m_joint = dJointCreateBall(0, 0);
        m_island.AddJoint(m_joint);
        m_ajoint = dJointCreateAMotor(0, 0);
        m_island.AddJoint(m_ajoint);
        dJointSetAMotorMode(m_ajoint, dAMotorEuler);
        dJointSetAMotorNumAxes(m_ajoint, 3);

        CreateBody();
        dBodySetPosition(m_body, capture_bone_position.x, capture_bone_position.y, capture_bone_position.z);
        VERIFY(smart_cast<CPHElement*>(m_taget_element));
        CPHElement* e = static_cast<CPHElement*>(m_taget_element);
        dJointAttach(m_joint, m_body, e->get_body());
        dJointAttach(m_ajoint, m_body, e->get_body());
        dJointSetFeedback(m_joint, &m_joint_feedback);
        dJointSetFeedback(m_ajoint, &m_joint_feedback);
        dJointSetBallAnchor(m_joint, capture_bone_position.x, capture_bone_position.y, capture_bone_position.z);

        dJointSetAMotorAxis(m_ajoint, 0, 1, dir.x, dir.y, dir.z);

        if (dir.x > EPS)
        {
            if (dir.y > EPS)
            {
                float mag = dir.x * dir.x + dir.y * dir.y;
                dJointSetAMotorAxis(m_ajoint, 2, 2, -dir.y / mag, dir.x / mag, 0.f);
            }
            else if (dir.z > EPS)
            {
                float mag = dir.x * dir.x + dir.z * dir.z;
                dJointSetAMotorAxis(m_ajoint, 2, 2, -dir.z / mag, 0.f, dir.x / mag);
            }
            else
            {
                dJointSetAMotorAxis(m_ajoint, 2, 2, 1.f, 0.f, 0.f);
            }
        }
        else
        {
            if (dir.y > EPS)
            {
                if (dir.z > EPS)
                {
                    float mag = dir.y * dir.y + dir.z * dir.z;
                    dJointSetAMotorAxis(m_ajoint, 2, 2, 0.f, -dir.z / mag, dir.y / mag);
                }
                else
                {
                    dJointSetAMotorAxis(m_ajoint, 2, 2, 0.f, 1.f, 0.f);
                }
            }
            else
            {
                dJointSetAMotorAxis(m_ajoint, 2, 2, 0.f, 0.f, 1.f);
            }
        }
        // float hi=-M_PI/2.f,lo=-hi;
        // dJointSetAMotorParam(m_ajoint,dParamLoStop ,lo);
        // dJointSetAMotorParam(m_ajoint,dParamHiStop ,hi);
        // dJointSetAMotorParam(m_ajoint,dParamLoStop2 ,lo);
        // dJointSetAMotorParam(m_ajoint,dParamHiStop2 ,hi);
        // dJointSetAMotorParam(m_ajoint,dParamLoStop3 ,lo);
        // dJointSetAMotorParam(m_ajoint,dParamHiStop3 ,hi);

        dJointSetAMotorParam(m_ajoint, dParamFMax, m_capture_force * 0.2f);
        dJointSetAMotorParam(m_ajoint, dParamVel, 0.f);

        dJointSetAMotorParam(m_ajoint, dParamFMax2, m_capture_force * 0.2f);
        dJointSetAMotorParam(m_ajoint, dParamVel2, 0.f);

        dJointSetAMotorParam(m_ajoint, dParamFMax3, m_capture_force * 0.2f);
        dJointSetAMotorParam(m_ajoint, dParamVel3, 0.f);

        ///////////////////////////////////
        float sf = 0.1f, df = 10.f;

        float erp = ERP(world_spring * sf, world_damping * df);
        float cfm = CFM(world_spring * sf, world_damping * df);
        dJointSetAMotorParam(m_ajoint, dParamStopERP, erp);
        dJointSetAMotorParam(m_ajoint, dParamStopCFM, cfm);

        dJointSetAMotorParam(m_ajoint, dParamStopERP2, erp);
        dJointSetAMotorParam(m_ajoint, dParamStopCFM2, cfm);

        dJointSetAMotorParam(m_ajoint, dParamStopERP3, erp);
        dJointSetAMotorParam(m_ajoint, dParamStopCFM3, cfm);
        /////////////////////////////////////////////////////////////////////
        /// dJointSetAMotorParam(m_joint1,dParamFudgeFactor ,0.1f);
        // dJointSetAMotorParam(m_joint1,dParamFudgeFactor2 ,0.1f);
        // dJointSetAMotorParam(m_joint1,dParamFudgeFactor3 ,0.1f);
        /////////////////////////////////////////////////////////////////////////////
        sf = 0.1f, df = 10.f;
        erp = ERP(world_spring * sf, world_damping * df);
        cfm = CFM(world_spring * sf, world_damping * df);
        dJointSetAMotorParam(m_ajoint, dParamCFM, cfm);
        dJointSetAMotorParam(m_ajoint, dParamCFM2, cfm);
        dJointSetAMotorParam(m_ajoint, dParamCFM3, cfm);

        ///////////////////////////

        // dJointSetAMotorParam(m_ajoint,dParamLoStop ,0.f);
        // dJointSetAMotorParam(m_ajoint,dParamHiStop ,0.f);
        m_taget_element->set_LinearVel(Fvector().set(0, 0, 0));
        m_taget_element->set_AngularVel(Fvector().set(0, 0, 0));

        m_taget_element->set_DynamicLimits();
        // m_taget_object->PPhysicsShell()->set_JointResistance()
        e_state = cstCaptured;
        return;
    }
    m_taget_element->applyForce(dir, m_pull_force);
}

void CPHCapture::CapturedUpdate()
{
    m_island.Unmerge();
    if (m_character->CPHObject::is_active())
    {
        m_taget_element->Enable();
    }

    if (!m_taget_element->isActive() ||
        dDOT(m_joint_feedback.f2, m_joint_feedback.f2) > m_capture_force * m_capture_force)
    {
        Release();
        return;
    }

    float mag = dSqrt(dDOT(m_joint_feedback.f1, m_joint_feedback.f1));

    // m_back_force=m_back_force*0.999f+ ((mag<m_capture_force/5.f) ? mag : (m_capture_force/5.f))*0.001f;
    //
    if (b_character_feedback && mag > m_capture_force / 2.2f)
    {
        float f = mag / (m_capture_force / 15.f);
        m_character->ApplyForce(m_joint_feedback.f1[0] / f, m_joint_feedback.f1[1] / f, m_joint_feedback.f1[2] / f);
    }

    Fvector capture_bone_position;
    // IGameObject* object=smart_cast<IGameObject*>(m_character->PhysicsRefObject());
    capture_bone_position.set(m_capture_bone->mTransform.c);
    m_character->PhysicsRefObject()->ObjectXFORM().transform_tiny(capture_bone_position);
    dBodySetPosition(m_body, capture_bone_position.x, capture_bone_position.y, capture_bone_position.z);
}

void CPHCapture::ReleasedUpdate()
{
    if (b_disabled)
        return;
    if (!b_collide)
    {
        e_state = cstFree;
        m_taget_element->Enable();
    }
    b_collide = false;
}

void CPHCapture::ReleaseInCallBack()
{
    //	if(!b_failed) return;
    b_collide = true;
}

void CPHCapture::object_contactCallbackFun(
    bool& do_colide, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/)
{
    dxGeomUserData* l_pUD1 = NULL;
    dxGeomUserData* l_pUD2 = NULL;
    l_pUD1 = retrieveGeomUserData(c.geom.g1);
    l_pUD2 = retrieveGeomUserData(c.geom.g2);

    if (!l_pUD1)
        return;
    if (!l_pUD2)
        return;

    // CEntityAlive* capturer=smart_cast<CEntityAlive*>(l_pUD1->ph_ref_object);
    IPhysicsShellHolder* capturer = (l_pUD1->ph_ref_object);
    if (capturer)
    {
        IPHCapture* icapture = capturer->PHCapture();
        CPHCapture* capture = static_cast<CPHCapture*>(icapture);
        if (capture)
        {
            if (capture->m_taget_element->PhysicsRefObject() == l_pUD2->ph_ref_object)
            {
                do_colide = false;
                capture->m_taget_element->Enable();
                if (capture->e_state == CPHCapture::cstReleased)
                    capture->ReleaseInCallBack();
            }
        }
    }

    capturer = l_pUD2->ph_ref_object;
    if (capturer)
    {
        CPHCapture* capture = static_cast<CPHCapture*>(capturer->PHCapture());
        if (capture)
        {
            if (capture->m_taget_element->PhysicsRefObject() == l_pUD1->ph_ref_object)
            {
                do_colide = false;
                capture->m_taget_element->Enable();
                if (capture->e_state == CPHCapture::cstReleased)
                    capture->ReleaseInCallBack();
            }
        }
    }
}
void CPHCapture::RemoveConnection(IPhysicsShellHolder* O)
{
    if (m_taget_object == O)
    {
        Deactivate();
    }
}

void CPHCapture::NetRelcase(CPhysicsShell* s)
{
    VERIFY(s);
    VERIFY(s->get_ElementByStoreOrder(0));
    VERIFY(s->get_ElementByStoreOrder(0)->PhysicsRefObject());
    RemoveConnection(s->get_ElementByStoreOrder(0)->PhysicsRefObject());
}
