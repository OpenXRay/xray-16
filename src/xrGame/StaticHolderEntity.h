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
    u16 m_rotate_x_bone, m_rotate_y_bone, m_camera_bone;
    float m_tgt_x_rot, m_tgt_y_rot, m_cur_x_rot, m_cur_y_rot, m_bind_x_rot, m_bind_y_rot;
    Fvector m_bind_x, m_bind_y;

    Fmatrix m_i_bind_x_xform, m_i_bind_y_xform, m_fire_bone_xform;
    Fvector2 m_lim_x_rot, m_lim_y_rot; //in bone space
    bool m_bAllowWeapon;
protected:
    void SetDesiredDir(float h, float p);
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

    bool attach_Actor(CGameObject* actor) override;
    void detach_Actor() override;
    bool allowWeapon() const override { return m_bAllowWeapon; };
    bool HUDView() const override { return true; };
    Fvector ExitPosition() override { return Fvector().set(0.0f, 0.0f, 0.0f); };

    CCameraBase* Camera() override { return camera; };

    void Action(u16 id, u32 flags) override;
    void SetParam(int id, Fvector2 val) override;
};
