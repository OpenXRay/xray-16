#include "StdAfx.h"
#pragma hdrstop

#include "CameraFirstEye.h"
#include "xr_level_controller.h"
#include "xrEngine/xr_object.h"
#include "xrCore/_quaternion.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCameraFirstEye::CCameraFirstEye(IGameObject* p, u32 flags) : CCameraBase(p, flags), lookat_active(false) {}
CCameraFirstEye::~CCameraFirstEye() {}
void CCameraFirstEye::Load(LPCSTR section)
{
    inherited::Load(section);
    style = csFirstEye;
}

void CCameraFirstEye::UpdateLookat()
{
    if (!lookat_active)
        return;

    Fvector _dest_dir;
    _dest_dir.sub(lookat_point, vPosition);

    Fmatrix _m;
    _m.identity();
    _m.k.normalize_safe(_dest_dir);
    Fvector::generate_orthonormal_basis(_m.k, _m.j, _m.i);

    Fvector xyz;
    _m.getXYZi(xyz);

    if (fsimilar(yaw, xyz.y, EPS) && fsimilar(pitch, xyz.x, EPS))
        lookat_active = false;

    yaw = angle_inertion_var(yaw, xyz.y, 1.0f, PI, PI, Device.fTimeDelta);

    pitch = angle_inertion_var(pitch, xyz.x, 1.0f, PI, PI, Device.fTimeDelta);
}

void CCameraFirstEye::Update(Fvector& point, Fvector& noise_dangle)
{
    vPosition.set(point);

    UpdateLookat();

    Fmatrix mR, R;
    Fmatrix rX, rY, rZ;
    rX.rotateX(noise_dangle.x);
    rY.rotateY(-noise_dangle.y);
    rZ.rotateZ(noise_dangle.z);
    R.mul_43(rY, rX);
    R.mulB_43(rZ);

    mR.identity();
    Fquaternion Q;
    Q.rotationYawPitchRoll(roll, yaw, pitch);
    mR.rotation(Q);
    mR.transpose();
    mR.mulB_43(R);

    vDirection.set(mR.k);
    vNormal.set(mR.j);

    if (m_Flags.is(flRelativeLink))
    {
        parent->XFORM().transform_dir(vDirection);
        parent->XFORM().transform_dir(vNormal);
    }
}

void CCameraFirstEye::Move(int cmd, float val, float factor)
{
    if (bClampPitch)
    {
        while (pitch < lim_pitch[0])
            pitch += PI_MUL_2;
        while (pitch > lim_pitch[1])
            pitch -= PI_MUL_2;
    };
    switch (cmd)
    {
    case kDOWN: pitch -= val ? val : (rot_speed.y * Device.fTimeDelta / factor); break;
    case kUP: pitch += val ? val : (rot_speed.y * Device.fTimeDelta / factor); break;
    case kLEFT: yaw -= val ? val : (rot_speed.x * Device.fTimeDelta / factor); break;
    case kRIGHT: yaw += val ? val : (rot_speed.x * Device.fTimeDelta / factor); break;
    }
    if (bClampYaw)
        clamp(yaw, lim_yaw[0], lim_yaw[1]);
    if (bClampPitch)
        clamp(pitch, lim_pitch[0], lim_pitch[1]);
}

void CCameraFirstEye::OnActivate(CCameraBase* old_cam)
{
    if (old_cam && (m_Flags.is(flRelativeLink) == old_cam->m_Flags.is(flRelativeLink)))
        yaw = (old_cam)->yaw;
}
