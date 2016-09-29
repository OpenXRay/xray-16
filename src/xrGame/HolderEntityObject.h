#pragma once

#include "holder_custom.h"
#include "physicsshellholder.h"

class CCameraBase;
class CHolderEntityObject : public CPhysicsShellHolder,
                            public CHolderCustom
{
    using inheritedPH = CPhysicsShellHolder;
    using inheritedHolder = CHolderCustom;

    CCameraBase* camera;

    static void BoneCallbackX(CBoneInstance* B);
    static void BoneCallbackY(CBoneInstance* B);
    void SetBoneCallbacks();
    void ResetBoneCallbacks();

    //casts
public:
    CHolderCustom* cast_holder_custom() override { return this; }

    //general
    CHolderEntityObject();
    virtual ~CHolderEntityObject();

    void Load(LPCSTR section) override;

    BOOL net_Spawn(CSE_Abstract* DC) override;
    void net_Destroy() override;
    void net_Export(NET_Packet& P) override; // export to server
    void net_Import(NET_Packet& P) override; // import from server

    void UpdateCL() override;

    void Hit(SHit* pHDS) override;

    //shooting
private:
    Fvector3 m_camera_position;
    Fvector3 m_exit_position;
    Fvector3 m_camera_angle;

    Fvector2 m_lim_x_rot, m_lim_y_rot; //in bone space
    bool m_bAllowWeapon;
protected:
    virtual bool IsHudModeNow() { return false; };

    //HolderCustom
public:
    bool Use(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos) override { return !Owner(); };
    void OnMouseMove(int x, int y) override;
    void OnKeyboardPress(int dik) override;
    void OnKeyboardRelease(int dik) override;
    void OnKeyboardHold(int dik) override;
    CInventory* GetInventory() override { return nullptr; };
    void cam_Update(float dt, float fov = 90.0f) override;

    void renderable_Render() override;

    virtual void attach_actor_script(bool bForce = false);
    virtual void detach_actor_script(bool bForce = false);

    bool attach_Actor(CGameObject* actor) override;
    void detach_Actor() override;
    bool allowWeapon() const override { return m_bAllowWeapon; };
    bool HUDView() const override { return true; };
    Fvector ExitPosition() override { return m_exit_position; };

    CCameraBase* Camera() override { return camera; };

    void Action(u16 id, u32 flags) override;
    void SetParam(int id, Fvector2 val) override;
};
