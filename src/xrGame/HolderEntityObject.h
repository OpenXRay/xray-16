#pragma once

#include "holder_custom.h"
#include "PhysicsShellHolder.h"

class CCameraBase;

class CHolderEntityObject final : public CPhysicsShellHolder, public CHolderCustom
{
    typedef CPhysicsShellHolder inheritedPH;
    typedef CHolderCustom inheritedHolder;

private:
    CCameraBase* camera;

    static void BoneCallbackX(CBoneInstance* B);
    static void BoneCallbackY(CBoneInstance* B);
    void SetBoneCallbacks();
    void ResetBoneCallbacks();

public:
    //casts
    CHolderCustom* cast_holder_custom() override { return this; }

public:
    //general
    CHolderEntityObject();
    ~CHolderEntityObject() override;

    void Load(pcstr section) override;

    bool net_Spawn(CSE_Abstract* DC) override;
    void net_Destroy() override;
    void net_Export(NET_Packet& P) override; // export to server
    void net_Import(NET_Packet& P) override; // import from server

    void UpdateCL() override;

    void Hit(SHit* pHDS) override;

private:
    //shooting
    Fvector3 m_camera_position{};
    Fvector3 m_exit_position{};
    Fvector3 m_camera_angle{};

    Fvector2 m_lim_x_rot{}, m_lim_y_rot{}; //in bone space
    bool m_bAllowWeapon;

public:
    //HolderCustom
    bool Use(const Fvector& pos, const Fvector& dir, const Fvector& foot_pos) override { return !Owner(); };
    void OnMouseMove(int x, int y) override;
    void OnKeyboardPress(int dik) override;
    void OnKeyboardRelease(int dik) override;
    void OnKeyboardHold(int dik) override;
    void OnControllerPress(int cmd, float x, float y) override;
    void OnControllerRelease(int cmd, float x, float y) override;
    void OnControllerHold(int cmd, float x, float y) override;
    void OnControllerAttitudeChange(Fvector change) override;
    CInventory* GetInventory() override { return nullptr; };
    void cam_Update(float dt, float fov = 90.0f) override;

    void renderable_Render(u32 context_id, IRenderable* root) override;

    void attach_actor_script(bool bForce = false);
    void detach_actor_script(bool bForce = false);

    bool attach_Actor(CGameObject* actor) override;
    void detach_Actor() override;
    bool allowWeapon() const override { return m_bAllowWeapon; };
    bool HUDView() const override { return true; };
    Fvector ExitPosition() override { return m_exit_position; };

    CCameraBase* Camera() override { return camera; };

    void Action(u16 id, u32 flags) override;
    void SetParam(int id, Fvector2 val) override;
};
