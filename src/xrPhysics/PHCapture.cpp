/////////////////////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"

#include "PHCapture.h"
#include "PHCharacter.h"
#include "Physics.h"
#include "ExtendedGeom.h"

#include "Include/xrRender/Kinematics.h"
#include "IPhysicsShellHolder.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrEngine/device.h"
#include "PHElement.h"

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

IPHCapture* phcapture_create(CPHCharacter* ch, IPhysicsShellHolder* object, NearestToPointCallback* cb /*=0*/)
{
    VERIFY(ch);
    return xr_new<CPHCapture>(ch, object, cb);
}

IPHCapture* phcapture_create(CPHCharacter* ch, IPhysicsShellHolder* object, u16 element)
{
    VERIFY(ch);
    return xr_new<CPHCapture>(ch, object, element);
}

void phcapture_destroy(IPHCapture*& c)
{
    CPHCapture* capture = smart_cast<CPHCapture*>(c);
    xr_delete(capture);
    c = 0;
}

void CPHCapture::CreateBody()
{
    auto* core = GetPhysicsCore();
    m_char_handle = core->CreateBox(Fvector().set(.01f, .01f, .01f), Fvector().set(0, 0, 0), 1);
    core->SetBodyMotionType(m_char_handle, 1);
    core->SetBodyObjectLayer(m_char_handle, 4);
}

CPHCapture::~CPHCapture()
{
    Deactivate();
}

bool CPHCapture::Invalid()
{
    return !m_taget_object->ObjectPPhysicsShell() || !m_taget_object->ObjectPPhysicsShell()->isActive() ||
        !m_character->b_exist;
}

void CPHCapture::PhDataUpdate(float step)
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

void CPHCapture::PhTune(float step)
{
    if (e_state == cstFree)
        return;

    VERIFY(m_character && m_character->b_exist);
    VERIFY(m_taget_object);
    VERIFY(m_taget_object->ObjectPPhysicsShell());
    VERIFY(m_taget_object->ObjectPPhysicsShell()->isFullActive());
    VERIFY(m_taget_element);
    VERIFY(m_taget_element->isFullActive());

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
    case cstPulling: break;
    case cstCaptured:
    {
        if (b_disabled)
        {
            if (m_taget_element->get_body() != INVALID_CHARACTER_VIRTUAL_HANDLE)
                GetPhysicsCore()->DeactivateBody(m_taget_element->get_body());
        }
    }
    break;
    case cstReleased: break;
    default: NODEFAULT;
    }
}

void CPHCapture::PullingUpdate()
{
    if (!m_taget_element->isActive() || Device.dwTimeGlobal - m_time_start > m_capture_time)
    {
        Release();
        return;
    }

    Fvector dir;
    Fvector capture_bone_position;

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

    if (dist > EPS)
        dir.mul(1.f / dist);

    if (dist < m_capture_distance)
    {
        m_back_force = 0.f;

        CreateBody();

        CPHElement* e = static_cast<CPHElement*>(m_taget_element);
        CharacterVirtualHandle target_body = e->get_body();

        if (target_body == INVALID_CHARACTER_VIRTUAL_HANDLE)
        {
            Release();
            return;
        }

        auto* core = GetPhysicsCore();
        core->SetBodyPosition(m_char_handle, capture_bone_position);
        const Fvector freeLow = Fvector().set(-PI, -PI, -PI);
        const Fvector freeHigh = Fvector().set(PI, PI, PI);
        m_joint = core->CreateJoint(3, m_char_handle, target_body, capture_bone_position,
            Fvector().set(1, 0, 0), Fvector().set(0, 1, 0), Fvector().set(0, 0, 1), freeLow, freeHigh);
        R_ASSERT(m_joint != INVALID_JOINT_HANDLE);
        core->SetJointFeedback(m_joint, &m_joint_feedback);
        for (int axis = 0; axis < 3; ++axis)
            core->SetJointMotor(m_joint, axis, m_capture_force * .2f, 0);
        m_ajoint = INVALID_JOINT_HANDLE;

        m_taget_element->set_LinearVel(Fvector().set(0, 0, 0));
        m_taget_element->set_AngularVel(Fvector().set(0, 0, 0));
        m_taget_element->set_DynamicLimits();

        e_state = cstCaptured;
        return;
    }

    m_taget_element->applyForce(dir, m_pull_force);
}

void CPHCapture::CapturedUpdate()
{
    if (m_character->CPHObject::is_active())
    {
        m_taget_element->Enable();
    }

    if (!m_taget_element->isActive() || m_joint_feedback.f2.square_magnitude() > m_capture_force * m_capture_force)
    {
        Release();
        return;
    }

    const float reaction = m_joint_feedback.f1.magnitude();
    if (b_character_feedback && reaction > m_capture_force / 2.2f) {
        const float scale = m_capture_force / (15.f * reaction);
        m_character->ApplyForce(m_joint_feedback.f1.x * scale,
            m_joint_feedback.f1.y * scale, m_joint_feedback.f1.z * scale);
    }
    Fvector capture_bone_position;
    capture_bone_position.set(m_capture_bone->mTransform.c);
    m_character->PhysicsRefObject()->ObjectXFORM().transform_tiny(capture_bone_position);

    GetPhysicsCore()->SetBodyPosition(m_char_handle, capture_bone_position);
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
    b_collide = true;
}

void CPHCapture::object_contactCallbackFun(
    bool& do_colide, bool bo1,
    CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom,
    const Fvector& contact_normal, const Fvector& contact_pos,
    SGameMtl* material_1, SGameMtl* material_2)
{
    if (!my_geom || !oposite_geom)
        return;

    IPhysicsShellHolder* capturer1 = my_geom->ph_ref_object;
    IPhysicsShellHolder* capturer2 = oposite_geom->ph_ref_object;

    if (capturer1)
    {
        IPHCapture* icapture = capturer1->PHCapture();
        CPHCapture* capture = static_cast<CPHCapture*>(icapture);
        if (capture && capture->m_taget_element)
        {
            if (capture->m_taget_element->PhysicsRefObject() == capturer2)
            {
                do_colide = false;
                capture->m_taget_element->Enable();
                if (capture->e_state == CPHCapture::cstReleased)
                    capture->ReleaseInCallBack();
            }
        }
    }

    if (capturer2)
    {
        CPHCapture* capture = static_cast<CPHCapture*>(capturer2->PHCapture());
        if (capture && capture->m_taget_element)
        {
            if (capture->m_taget_element->PhysicsRefObject() == capturer1)
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
