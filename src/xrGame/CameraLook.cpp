#include "stdafx.h"
#pragma hdrstop

#include "CameraLook.h"
#include "xrEngine/Cameramanager.h"
#include "xr_level_controller.h"
#include "actor.h"

CCameraLook::CCameraLook(IGameObject* p, u32 flags) : CCameraBase(p, flags) {}
void CCameraLook::Load(LPCSTR section)
{
    inherited::Load(section);
    style = csLookAt;
    lim_zoom = pSettings->r_fvector2(section, "lim_zoom");
    dist = (lim_zoom[0] + lim_zoom[1]) * 0.5f;
    prev_d = 0;
}

CCameraLook::~CCameraLook() {}
void CCameraLook::Update(Fvector& point, Fvector& /**noise_dangle/**/)
{
    vPosition.set(point);
    Fmatrix mR;
    mR.setHPB(-yaw, -pitch, -roll);

    vDirection.set(mR.k);
    vNormal.set(mR.j);

    if (m_Flags.is(flRelativeLink))
    {
        parent->XFORM().transform_dir(vDirection);
        parent->XFORM().transform_dir(vNormal);
    }
    UpdateDistance(point);
}

void CCameraLook::UpdateDistance(Fvector& point)
{
    Fvector vDir;
    collide::rq_result R;

    float covariance = VIEWPORT_NEAR * 6.f;
    vDir.invert(vDirection);
    g_pGameLevel->ObjectSpace.RayPick(point, vDir, dist + covariance, collide::rqtBoth, R, parent);

    float d = psCamSlideInert * prev_d + (1.f - psCamSlideInert) * (R.range - covariance);
    prev_d = d;

    vPosition.mul(vDirection, -d - VIEWPORT_NEAR);
    vPosition.add(point);
}

void CCameraLook::Move(int cmd, float val, float factor)
{
    switch (cmd)
    {
    case kCAM_ZOOM_IN: dist -= val ? val : (rot_speed.z * Device.fTimeDelta); break;
    case kCAM_ZOOM_OUT: dist += val ? val : (rot_speed.z * Device.fTimeDelta); break;
    case kDOWN: pitch -= val ? val : (rot_speed.x * Device.fTimeDelta / factor); break;
    case kUP: pitch += val ? val : (rot_speed.x * Device.fTimeDelta / factor); break;
    case kLEFT: yaw -= val ? val : (rot_speed.y * Device.fTimeDelta / factor); break;
    case kRIGHT: yaw += val ? val : (rot_speed.y * Device.fTimeDelta / factor); break;
    }
    if (bClampYaw)
        clamp(yaw, lim_yaw[0], lim_yaw[1]);
    if (bClampPitch)
        clamp(pitch, lim_pitch[0], lim_pitch[1]);
    clamp(dist, lim_zoom[0], lim_zoom[1]);
}

void CCameraLook::OnActivate(CCameraBase* old_cam)
{
    if (old_cam && (m_Flags.is(flRelativeLink) == old_cam->m_Flags.is(flRelativeLink)))
    {
        yaw = old_cam->yaw;
        vPosition.set(old_cam->vPosition);
    }
    if (yaw > PI_MUL_2)
        yaw -= PI_MUL_2;
    if (yaw < -PI_MUL_2)
        yaw += PI_MUL_2;
}

#include "xrEngine/xr_input.h"
#include "visual_memory_manager.h"
#include "actor_memory.h"

int cam_dik = SDL_SCANCODE_LSHIFT;

Fvector CCameraLook2::m_cam_offset;
void CCameraLook2::OnActivate(CCameraBase* old_cam) { CCameraLook::OnActivate(old_cam); }
void CCameraLook2::Update(Fvector& point, Fvector&)
{
    if (!m_locked_enemy)
    { // autoaim
        if (pInput->iGetAsyncKeyState(cam_dik))
        {
            const CVisualMemoryManager::VISIBLES& vVisibles = Actor()->memory().visual().objects();
            CVisualMemoryManager::VISIBLES::const_iterator v_it = vVisibles.begin();
            float _nearest_dst = flt_max;

            for (; v_it != vVisibles.end(); ++v_it)
            {
                const IGameObject* _object_ = (*v_it).m_object;
                if (!Actor()->memory().visual().visible_now(smart_cast<const CGameObject*>(_object_)))
                    continue;

                IGameObject* object_ = const_cast<IGameObject*>(_object_);

                CEntityAlive* EA = smart_cast<CEntityAlive*>(object_);
                if (!EA || !EA->g_Alive())
                    continue;

                float d = object_->Position().distance_to_xz(Actor()->Position());
                if (!m_locked_enemy || d < _nearest_dst)
                {
                    m_locked_enemy = object_;
                    _nearest_dst = d;
                }
            }
            //.			if(m_locked_enemy) Msg("enemy is %s", *m_locked_enemy->cNameSect() );
        }
    }
    else
    {
        if (!pInput->iGetAsyncKeyState(cam_dik))
        {
            m_locked_enemy = NULL;
            //.			Msg				("enemy is NILL");
        }
    }

    if (m_locked_enemy)
        UpdateAutoAim();

    Fmatrix mR;
    mR.setHPB(-yaw, -pitch, -roll);

    vDirection.set(mR.k);
    vNormal.set(mR.j);

    Fmatrix a_xform;
    a_xform.setXYZ(0, -yaw, 0);
    a_xform.translate_over(point);
    Fvector _off = m_cam_offset;
    a_xform.transform_tiny(_off);
    vPosition.set(_off);
}

void CCameraLook2::UpdateAutoAim()
{
    Fvector _dest_point;
    m_locked_enemy->Center(_dest_point);
    _dest_point.y += 0.2f;

    Fvector _dest_dir;
    _dest_dir.sub(_dest_point, vPosition);

    Fmatrix _m;
    _m.identity();
    _m.k.normalize_safe(_dest_dir);
    Fvector::generate_orthonormal_basis(_m.k, _m.j, _m.i);

    Fvector xyz;
    _m.getXYZi(xyz);

    yaw = angle_inertion_var(yaw, xyz.y, m_autoaim_inertion_yaw.x, m_autoaim_inertion_yaw.y, PI, Device.fTimeDelta);

    pitch =
        angle_inertion_var(pitch, xyz.x, m_autoaim_inertion_pitch.x, m_autoaim_inertion_pitch.y, PI, Device.fTimeDelta);
}

void CCameraLook2::Load(LPCSTR section)
{
    CCameraLook::Load(section);
    m_cam_offset = pSettings->r_fvector3(section, "offset");
    m_autoaim_inertion_yaw = pSettings->r_fvector2(section, "autoaim_speed_y");
    m_autoaim_inertion_pitch = pSettings->r_fvector2(section, "autoaim_speed_x");
}

void CCameraFixedLook::Load(LPCSTR section)
{
    CCameraLook::Load(section);
    style = csFixed;
}

void CCameraFixedLook::OnActivate(CCameraBase* old_cam)
{
    inherited::OnActivate(old_cam);
    m_current_dir.rotationYawPitchRoll(-pitch, -yaw, -roll);

    Fmatrix rm;
    Fmatrix trm;
    Fmatrix brm;
    brm.setXYZ(-pitch, -yaw, -roll);
    trm.rotateX(PI_DIV_2);
    rm.mul(brm, trm);
    rm.getXYZ(pitch, yaw, roll);
    m_final_dir.set(rm);
    pitch = -pitch;
    yaw = -yaw;
    roll = -roll;
}

void CCameraFixedLook::Move(int cmd, float val, float factor) {}
void CCameraFixedLook::Update(Fvector& point, Fvector& noise_dangle)
{
    Fquaternion new_dir;
    new_dir.slerp(m_current_dir, m_final_dir, Device.fTimeDelta); // 1 sec
    m_current_dir.set(new_dir);

    Fmatrix rm;
    rm.rotation(m_current_dir);
    vPosition.set(point);
    vDirection.set(rm.k);
    vNormal.set(rm.j);

    UpdateDistance(point);
}

void CCameraFixedLook::Set(float Y, float P, float R)
{
    inherited::Set(Y, P, R);
    Fmatrix rm;
    rm.setXYZ(-P, -Y, -R);
    m_current_dir.set(rm);
    m_final_dir.set(m_current_dir);
}
