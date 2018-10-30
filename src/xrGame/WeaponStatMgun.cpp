#include "StdAfx.h"
#include "WeaponStatMgun.h"
#include "Include/xrRender/Kinematics.h"
#include "xrPhysics/PhysicsShell.h"
#include "WeaponAmmo.h"
#include "Common/object_broker.h"
#include "ai_sounds.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "CameraFirstEye.h"
#include "xr_level_controller.h"
#include "game_object_space.h"
#include "Level.h"

void CWeaponStatMgun::BoneCallbackX(CBoneInstance* B)
{
    CWeaponStatMgun* P = static_cast<CWeaponStatMgun*>(B->callback_param());
    Fmatrix rX;
    rX.rotateX(P->m_cur_x_rot);
    B->mTransform.mulB_43(rX);
}

void CWeaponStatMgun::BoneCallbackY(CBoneInstance* B)
{
    CWeaponStatMgun* P = static_cast<CWeaponStatMgun*>(B->callback_param());
    Fmatrix rY;
    rY.rotateY(P->m_cur_y_rot);
    B->mTransform.mulB_43(rY);
}

CWeaponStatMgun::CWeaponStatMgun()
{
    m_Ammo = new CCartridge();
    camera = new CCameraFirstEye(
        this, CCameraBase::flRelativeLink | CCameraBase::flPositionRigid | CCameraBase::flDirectionRigid);
    camera->Load("mounted_weapon_cam");
}

CWeaponStatMgun::~CWeaponStatMgun()
{
    delete_data(m_Ammo);
    xr_delete(camera);
}

void CWeaponStatMgun::SetBoneCallbacks()
{
    m_pPhysicsShell->EnabledCallbacks(FALSE);

    CBoneInstance& biX = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(m_rotate_x_bone);
    biX.set_callback(bctCustom, BoneCallbackX, this);
    CBoneInstance& biY = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(m_rotate_y_bone);
    biY.set_callback(bctCustom, BoneCallbackY, this);
}

void CWeaponStatMgun::ResetBoneCallbacks()
{
    CBoneInstance& biX = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(m_rotate_x_bone);
    biX.reset_callback();
    CBoneInstance& biY = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(m_rotate_y_bone);
    biY.reset_callback();

    m_pPhysicsShell->EnabledCallbacks(TRUE);
}

void CWeaponStatMgun::Load(LPCSTR section)
{
    inheritedPH::Load(section);
    inheritedShooting::Load(section);

    m_sounds.LoadSound(section, "snd_shoot", "sndShot", false, SOUND_TYPE_WEAPON_SHOOTING);
    m_Ammo->Load(pSettings->r_string(section, "ammo_class"), 0);
    camMaxAngle = pSettings->r_float(section, "cam_max_angle");
    camMaxAngle = _abs(deg2rad(camMaxAngle));
    camRelaxSpeed = pSettings->r_float(section, "cam_relax_speed");
    camRelaxSpeed = _abs(deg2rad(camRelaxSpeed));

    VERIFY(!fis_zero(camMaxAngle));
    VERIFY(!fis_zero(camRelaxSpeed));
}

BOOL CWeaponStatMgun::net_Spawn(CSE_Abstract* DC)
{
    if (!inheritedPH::net_Spawn(DC))
        return FALSE;

    IKinematics* K = smart_cast<IKinematics*>(Visual());
    CInifile* pUserData = K->LL_UserData();

    R_ASSERT2(pUserData, "Empty WeaponStatMgun user data!");

    m_rotate_x_bone = K->LL_BoneID(pUserData->r_string("mounted_weapon_definition", "rotate_x_bone"));
    m_rotate_y_bone = K->LL_BoneID(pUserData->r_string("mounted_weapon_definition", "rotate_y_bone"));
    m_fire_bone = K->LL_BoneID(pUserData->r_string("mounted_weapon_definition", "fire_bone"));
    m_camera_bone = K->LL_BoneID(pUserData->r_string("mounted_weapon_definition", "camera_bone"));

    U16Vec fixed_bones;
    fixed_bones.push_back(K->LL_GetBoneRoot());
    PPhysicsShell() = P_build_Shell(this, false, fixed_bones);

    CBoneData& bdX = K->LL_GetData(m_rotate_x_bone);
    VERIFY(bdX.IK_data.type == jtJoint);
    m_lim_x_rot.set(bdX.IK_data.limits[0].limit.x, bdX.IK_data.limits[0].limit.y);
    CBoneData& bdY = K->LL_GetData(m_rotate_y_bone);
    VERIFY(bdY.IK_data.type == jtJoint);
    m_lim_y_rot.set(bdY.IK_data.limits[1].limit.x, bdY.IK_data.limits[1].limit.y);

    xr_vector<Fmatrix> matrices;
    K->LL_GetBindTransform(matrices);
    m_i_bind_x_xform.invert(matrices[m_rotate_x_bone]);
    m_i_bind_y_xform.invert(matrices[m_rotate_y_bone]);
    m_bind_x_rot = matrices[m_rotate_x_bone].k.getP();
    m_bind_y_rot = matrices[m_rotate_y_bone].k.getH();
    m_bind_x.set(matrices[m_rotate_x_bone].c);
    m_bind_y.set(matrices[m_rotate_y_bone].c);

    m_cur_x_rot = m_bind_x_rot;
    m_cur_y_rot = m_bind_y_rot;
    m_destEnemyDir.setHP(m_bind_y_rot, m_bind_x_rot);
    XFORM().transform_dir(m_destEnemyDir);

    inheritedShooting::Light_Create();

    processing_activate();
    setVisible(TRUE);
    setEnabled(TRUE);
    return TRUE;
}

void CWeaponStatMgun::net_Destroy()
{
    inheritedPH::net_Destroy();
    processing_deactivate();
}

void CWeaponStatMgun::net_Export(NET_Packet& P) // export to server
{
    inheritedPH::net_Export(P);
    P.w_u8(IsWorking() ? 1 : 0);
    save_data(m_destEnemyDir, P);
}

void CWeaponStatMgun::net_Import(NET_Packet& P) // import from server
{
    inheritedPH::net_Import(P);
    u8 state = P.r_u8();
    load_data(m_destEnemyDir, P);

    if (TRUE == IsWorking() && !state)
        FireEnd();
    if (FALSE == IsWorking() && state)
        FireStart();
}

void CWeaponStatMgun::UpdateCL()
{
    inheritedPH::UpdateCL();
    UpdateBarrelDir();
    UpdateFire();

    if (OwnerActor() && OwnerActor()->IsMyCamera())
    {
        cam_Update(Device.fTimeDelta, g_fov);
        OwnerActor()->Cameras().UpdateFromCamera(Camera());
        OwnerActor()->Cameras().ApplyDevice();
    }
}

// void CWeaponStatMgun::Hit(	float P, Fvector &dir,	IGameObject* who,
//							s16 element,Fvector p_in_object_space,
//							float impulse, ALife::EHitType hit_type)
void CWeaponStatMgun::Hit(SHit* pHDS)
{
    if (NULL == Owner())
        //		inheritedPH::Hit(P,dir,who,element,p_in_object_space,impulse,hit_type);
        inheritedPH::Hit(pHDS);
}

void CWeaponStatMgun::UpdateBarrelDir()
{
    IKinematics* K = smart_cast<IKinematics*>(Visual());
    m_fire_bone_xform = K->LL_GetTransform(m_fire_bone);

    m_fire_bone_xform.mulA_43(XFORM());
    m_fire_pos.set(0, 0, 0);
    m_fire_bone_xform.transform_tiny(m_fire_pos);
    m_fire_dir.set(0, 0, 1);
    m_fire_bone_xform.transform_dir(m_fire_dir);

    m_allow_fire = true;
    Fmatrix XFi;
    XFi.invert(XFORM());
    Fvector dep;
    XFi.transform_dir(dep, m_destEnemyDir);
    { // x angle
        m_i_bind_x_xform.transform_dir(dep);
        dep.normalize();
        m_tgt_x_rot = angle_normalize_signed(m_bind_x_rot - dep.getP());
        float sv_x = m_tgt_x_rot;

        clamp(m_tgt_x_rot, -m_lim_x_rot.y, -m_lim_x_rot.x);
        if (!fsimilar(sv_x, m_tgt_x_rot, EPS_L))
            m_allow_fire = FALSE;
    }
    { // y angle
        m_i_bind_y_xform.transform_dir(dep);
        dep.normalize();
        m_tgt_y_rot = angle_normalize_signed(m_bind_y_rot - dep.getH());
        float sv_y = m_tgt_y_rot;
        clamp(m_tgt_y_rot, -m_lim_y_rot.y, -m_lim_y_rot.x);
        if (!fsimilar(sv_y, m_tgt_y_rot, EPS_L))
            m_allow_fire = FALSE;
    }

    m_cur_x_rot = angle_inertion_var(m_cur_x_rot, m_tgt_x_rot, 0.5f, 3.5f, PI_DIV_6, Device.fTimeDelta);
    m_cur_y_rot = angle_inertion_var(m_cur_y_rot, m_tgt_y_rot, 0.5f, 3.5f, PI_DIV_6, Device.fTimeDelta);
}

void CWeaponStatMgun::cam_Update(float dt, float fov)
{
    Fvector P, Da;
    Da.set(0, 0, 0);

    IKinematics* K = smart_cast<IKinematics*>(Visual());
    K->CalculateBones_Invalidate();
    K->CalculateBones(TRUE);
    const Fmatrix& C = K->LL_GetTransform(m_camera_bone);
    XFORM().transform_tiny(P, C.c);

    Fvector d = C.k;
    XFORM().transform_dir(d);
    Fvector2 des_cam_dir;

    d.getHP(des_cam_dir.x, des_cam_dir.y);
    des_cam_dir.mul(-1.0f);

    Camera()->yaw = angle_inertion_var(Camera()->yaw, des_cam_dir.x, 0.5f, 7.5f, PI_DIV_6, Device.fTimeDelta);
    Camera()->pitch = angle_inertion_var(Camera()->pitch, des_cam_dir.y, 0.5f, 7.5f, PI_DIV_6, Device.fTimeDelta);

    if (OwnerActor())
    {
        // rotate head
        OwnerActor()->Orientation().yaw = -Camera()->yaw;
        OwnerActor()->Orientation().pitch = -Camera()->pitch;
    }

    Camera()->Update(P, Da);
    Level().Cameras().UpdateFromCamera(Camera());
}

void CWeaponStatMgun::renderable_Render()
{
    inheritedPH::renderable_Render();

    RenderLight();
}

void CWeaponStatMgun::SetDesiredDir(float h, float p) { m_destEnemyDir.setHP(h, p); }
void CWeaponStatMgun::Action(u16 id, u32 flags)
{
    inheritedHolder::Action(id, flags);
    switch (id)
    {
    case kWPN_FIRE:
    {
        if (flags == CMD_START)
            FireStart();
        else
            FireEnd();
    }
    break;
    }
}

void CWeaponStatMgun::SetParam(int id, Fvector2 val)
{
    inheritedHolder::SetParam(id, val);
    switch (id)
    {
    case DESIRED_DIR: SetDesiredDir(val.x, val.y); break;
    }
}

bool CWeaponStatMgun::attach_Actor(CGameObject* actor)
{
    inheritedHolder::attach_Actor(actor);
    SetBoneCallbacks();
    FireEnd();
    return true;
}

void CWeaponStatMgun::detach_Actor()
{
    inheritedHolder::detach_Actor();
    ResetBoneCallbacks();
    FireEnd();
}
