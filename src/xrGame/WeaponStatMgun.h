#pragma once

#include "holder_custom.h"
#include "ShootingObject.h"
#include "PhysicsShellHolder.h"
#include "HudSound.h"
class CCartridge;
class CCameraBase;

#define DESIRED_DIR 1

class CWeaponStatMgun : public CPhysicsShellHolder, public CHolderCustom, public CShootingObject
{
private:
    typedef CPhysicsShellHolder inheritedPH;
    typedef CHolderCustom inheritedHolder;
    typedef CShootingObject inheritedShooting;

private:
    CCameraBase* camera;
    //
    static void BoneCallbackX(CBoneInstance* B);
    static void BoneCallbackY(CBoneInstance* B);
    void SetBoneCallbacks();
    void ResetBoneCallbacks();

    HUD_SOUND_COLLECTION m_sounds;

    // casts
public:
    virtual CHolderCustom* cast_holder_custom() { return this; }
    // general
public:
    CWeaponStatMgun();
    virtual ~CWeaponStatMgun();

    virtual void Load(LPCSTR section);

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Export(NET_Packet& P); // export to server
    virtual void net_Import(NET_Packet& P); // import from server

    virtual void UpdateCL();

    virtual void Hit(SHit* pHDS);

    // shooting
private:
    u16 m_rotate_x_bone, m_rotate_y_bone, m_fire_bone, m_camera_bone;
    float m_tgt_x_rot, m_tgt_y_rot, m_cur_x_rot, m_cur_y_rot, m_bind_x_rot, m_bind_y_rot;
    Fvector m_bind_x, m_bind_y;
    Fvector m_fire_dir, m_fire_pos;

    Fmatrix m_i_bind_x_xform, m_i_bind_y_xform, m_fire_bone_xform;
    Fvector2 m_lim_x_rot, m_lim_y_rot; // in bone space
    CCartridge* m_Ammo;
    float m_barrel_speed;
    Fvector2 m_dAngle;
    Fvector m_destEnemyDir;
    bool m_allow_fire;
    float camRelaxSpeed;
    float camMaxAngle;

protected:
    void UpdateBarrelDir();
    virtual const Fvector& get_CurrentFirePoint();
    virtual const Fmatrix& get_ParticlesXFORM();

    virtual void FireStart();
    virtual void FireEnd();
    virtual void UpdateFire();
    virtual void OnShot();
    void AddShotEffector();
    void RemoveShotEffector();
    void SetDesiredDir(float h, float p);
    virtual bool IsHudModeNow() { return false; };
    // HolderCustom
public:
    virtual bool Use(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos) { return !Owner(); };
    virtual void OnMouseMove(int x, int y);
    virtual void OnKeyboardPress(int dik);
    virtual void OnKeyboardRelease(int dik);
    virtual void OnKeyboardHold(int dik);
    virtual CInventory* GetInventory() { return NULL; };
    virtual void cam_Update(float dt, float fov = 90.0f);

    virtual void renderable_Render();

    virtual bool attach_Actor(CGameObject* actor);
    virtual void detach_Actor();
    virtual bool allowWeapon() const { return false; };
    virtual bool HUDView() const { return true; };
    virtual Fvector ExitPosition() { return Fvector().set(0.0f, 0.0f, 0.0f); };
    virtual CCameraBase* Camera() { return camera; };
    virtual void Action(u16 id, u32 flags);
    virtual void SetParam(int id, Fvector2 val);
};
