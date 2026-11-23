/////////////////////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "PHCharacter.h"
#include "Physics.h"
#include "ExtendedGeom.h"
#include "PHCapture.h"

#include "Include/xrRender/Kinematics.h"

#include "IPhysicsShellHolder.h"
#include "xrCore/Animation/Bone.hpp"
#include "xrEngine/device.h"
#include "xrPhysicsCore/IPhysicsCore.h" // Добавлен инклуд нашего ядра

extern class CPHWorld* ph_world;
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
bool can_capture(CPHCharacter* a_character, IPhysicsShellHolder* a_taget_object)
{
    if (!a_taget_object || !a_taget_object->ObjectPPhysicsShell() ||
        !a_taget_object->ObjectPPhysicsShell()->isActive() || a_taget_object->IsInventoryItem() || !a_character ||
        !a_character->b_exist || !a_character->PhysicsRefObject() ||
        !a_character->PhysicsRefObject()->ObjectKinematics())
        return false;

    IKinematics* p_kinematics = a_character->PhysicsRefObject()->ObjectKinematics();
    VERIFY(p_kinematics);
    CInifile* ini = p_kinematics->LL_UserData();
    if (!ini || !ini->section_exist("capture"))
        return false;
    return true;
}

bool can_capture(CPHCharacter* a_character, IPhysicsShellHolder* a_taget_object, u16 a_taget_element)
{
    if (!can_capture(a_character, a_taget_object) || a_taget_element == BI_NONE ||
        !a_taget_object->ObjectKinematics())
        return false;

    IKinematics* K = a_taget_object->ObjectKinematics();

    if (!K || !K->LL_GetBoneInstance(a_taget_element).callback_param())
        return false;
    return true;
}

static CBoneInstance* get_capture_bone(CPHCharacter* a_character)
{
    VERIFY(a_character);
    VERIFY(a_character->PhysicsRefObject());
    IKinematics* p_kinematics = a_character->PhysicsRefObject()->ObjectKinematics();
    VERIFY(p_kinematics);
    CInifile* ini = p_kinematics->LL_UserData();
    VERIFY(ini);
    VERIFY(ini->section_exist("capture"));
    u16 capture_bone_id = p_kinematics->LL_BoneID(ini->r_string("capture", "bone"));

    R_ASSERT2(capture_bone_id != BI_NONE, "wrong capture bone");

    return &p_kinematics->LL_GetBoneInstance(capture_bone_id);
}

CPHCapture::CPHCapture(
    CPHCharacter* a_character, IPhysicsShellHolder* a_taget_object, NearestToPointCallback* cb /*=0*/)
    : m_joint(INVALID_JOINT_HANDLE), m_ajoint(INVALID_JOINT_HANDLE), m_char_handle(INVALID_CHARACTER_VIRTUAL_HANDLE),
      m_taget_object(a_taget_object), m_character(a_character),
      b_disabled(false), b_character_feedback(false), e_state(cstFree),
      ///////////////////////////////////////////////////////////////
      m_taget_element(0),
      m_capture_pos(Fvector().set(0, 0, 0)), m_back_force(0), m_pull_force(0), m_capture_force(0),
      m_capture_distance(0), m_capture_time(0), m_time_start(0), m_capture_bone(0),
      b_collide(false)
{
    if (!can_capture(a_character, a_taget_object))
        return;

    VERIFY(m_taget_object);
    VERIFY(m_taget_object->ObjectPPhysicsShell());
    VERIFY(a_character);
    m_capture_bone = get_capture_bone(a_character);
    VERIFY(m_capture_bone);
    m_taget_element = m_taget_object->ObjectPPhysicsShell()->NearestToPoint(m_capture_bone->mTransform.c, cb);
    if (!m_taget_element)
        return;
    Init();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
CPHCapture::CPHCapture(CPHCharacter* a_character, IPhysicsShellHolder* a_taget_object, u16 a_taget_element)
    : m_joint(INVALID_JOINT_HANDLE), m_ajoint(INVALID_JOINT_HANDLE), m_char_handle(INVALID_CHARACTER_VIRTUAL_HANDLE),
      b_disabled(false), b_character_feedback(false),
      m_taget_object(a_taget_object), m_character(a_character), e_state(cstFree),
      ///////////////////////////////////////////////////////////////
      m_taget_element(0),
      m_capture_pos(Fvector().set(0, 0, 0)), m_back_force(0), m_pull_force(0), m_capture_force(0),
      m_capture_distance(0), m_capture_time(0), m_time_start(0), m_capture_bone(0),
      b_collide(false)

{
    if (!can_capture(a_character, a_taget_object, a_taget_element))
        return;
    VERIFY(m_taget_object);
    VERIFY(a_character);
    m_capture_bone = get_capture_bone(a_character);

    IKinematics* K = m_taget_object->ObjectKinematics();
    VERIFY(K);

    CBoneInstance& tag_bone = K->LL_GetBoneInstance(a_taget_element);
    VERIFY(tag_bone.callback_param());

    m_taget_element = (CPhysicsElement*)tag_bone.callback_param();
    VERIFY(m_taget_element);

    Init();
}

void CPHCapture::Init()
{
    VERIFY(m_taget_element);
    VERIFY(m_character);
    VERIFY(m_character->PhysicsRefObject());
    VERIFY(m_character->PhysicsRefObject()->ObjectKinematics());
    VERIFY(m_capture_bone);

    IKinematics* p_kinematics = m_character->PhysicsRefObject()->ObjectKinematics();
    VERIFY(p_kinematics);
    CInifile* ini = p_kinematics->LL_UserData();
    VERIFY(ini);

    Fvector dir;
    Fvector capture_bone_position;
    capture_bone_position.set(m_capture_bone->mTransform.c);
    b_character_feedback = true;
    (m_character->PhysicsRefObject())->ObjectXFORM().transform_tiny(capture_bone_position);

    m_taget_element->GetGlobalPositionDynamic(&dir);
    dir.sub(capture_bone_position, dir);

    m_pull_distance = ini->r_float("capture", "pull_distance");
    if (dir.magnitude() > m_pull_distance)
        return;

    float pool_force_factor = 4.f;
    m_capture_distance = ini->r_float("capture", "distance");
    m_capture_force = ini->r_float("capture", "capture_force");
    m_capture_time = ini->r_u32("capture", "time_limit") * 1000;
    m_time_start = Device.dwTimeGlobal;
    float max_pull_force = ini->r_float("capture", "pull_force");
    m_pull_force = pool_force_factor * ph_world->Gravity() * m_taget_element->PhysicsShell()->getMass();
    if (m_pull_force > max_pull_force)
        m_pull_force = max_pull_force;
    float pulling_vel_scale = ini->r_float("capture", "velocity_scale");

    m_taget_element->set_DynamicLimits(default_l_limit * pulling_vel_scale, default_w_limit * pulling_vel_scale);
    m_character->SetObjectContactCallback(object_contactCallbackFun);

    // Ручная инициализация острова m_island больше не требуется

    IPhysicsShellHolder* A = (m_character->PhysicsRefObject());
    if (A->IsActor())
    {
        A->HideAllWeapons(true);
    }
    CPHUpdateObject::Activate();
    e_state = cstPulling;
}

void CPHCapture::Release()
{
    if (e_state == cstReleased || e_state == cstFree)
        return;

    // Уничтожаем суставы и тела через новое ядро (ODE удалено)
    if (m_joint != INVALID_JOINT_HANDLE)
    {
        GetPhysicsCore()->DestroyJoint(m_joint);
        m_joint = INVALID_JOINT_HANDLE;
    }

    if (m_ajoint != INVALID_JOINT_HANDLE)
    {
        GetPhysicsCore()->DestroyJoint(m_ajoint);
        m_ajoint = INVALID_JOINT_HANDLE;
    }

    if (m_char_handle != INVALID_CHARACTER_VIRTUAL_HANDLE)
    {
        GetPhysicsCore()->DestroyBody(m_char_handle);
        m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;
    }

    if (e_state == cstPulling && m_taget_element && !m_taget_object->ObjectGetDestroy() &&
        m_taget_object->ObjectPPhysicsShell() && m_taget_object->ObjectPPhysicsShell()->isActive())
    {
        m_taget_element->set_DynamicLimits();
    }

    b_collide = true;
    IPhysicsShellHolder* A = (m_character->PhysicsRefObject());
    if (A)
    {
        A->HideAllWeapons(false);
    }

    e_state = cstReleased;
}
#include "PHElement.h"
void CPHCapture::Deactivate()
{
    Release();
    if (m_taget_element)
    {
        VERIFY(dynamic_cast<CPHElement*>(m_taget_element));
        m_taget_element->Enable();
    }
    if (m_character)
        m_character->SetObjectContactCallback(0);

    CPHUpdateObject::Deactivate();
    e_state = cstFree;
    m_character = NULL;
    m_taget_object = NULL;
    m_taget_element = NULL;
}
