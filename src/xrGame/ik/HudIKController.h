#pragma once

#include "Include/xrRender/Kinematics.h"
#include "xrAnimation/OzzLimbSolver.h"
#include "xrCore/Animation/Bone.hpp"

class CHudIKController
{
public:
    enum class TargetSpace : u32
    {
        Animated,
        Model,
        Bone,
        Gun
    };

    class ArmSettings
    {
    public:
        bool enabled = false;
        u16 bones[3] = {BI_NONE, BI_NONE, BI_NONE};
        TargetSpace space = TargetSpace::Animated;
        u16 targetBone = BI_NONE;
        Fvector position{};
        Fvector rotation{};
        Fvector elbowOffset{};
        float weight = 1.f;
    };

    class ArmState
    {
    public:
        bool valid = false;
        bool solved = false;
        pcstr status = "inactive";
        Fmatrix animated[3]{};
        Fmatrix resolved[3]{};
        Fmatrix target{};
        Fvector elbow{};
        float error = 0.f;
        u32 frame = 0;
    };

    class GunSettings
    {
    public:
        bool enabled = false;
        u16 bone = BI_NONE;
        TargetSpace space = TargetSpace::Animated;
        Fvector position{};
        Fvector rotation{};
    };

    class GunState
    {
    public:
        bool valid = false;
        bool applied = false;
        pcstr status = "inactive";
        Fmatrix animated{};
        Fmatrix target{};
        Fmatrix resolved{};
        Fmatrix delta{};
        u32 frame = 0;
    };

    CHudIKController();
    ~CHudIKController();
    CHudIKController(const CHudIKController&) = delete;
    CHudIKController& operator=(const CHudIKController&) = delete;

    void Bind(IKinematics* skeleton);
    void Unbind();
    IKinematics* Skeleton() const;

    const ArmSettings& GetArm(u16 arm) const;
    void SetArm(u16 arm, const ArmSettings& settings);
    const ArmState& GetState(u16 arm) const;
    void ResetArm(u16 arm);
    void ResetAll();
    bool CaptureTarget(u16 arm, TargetSpace space, u16 targetBone = BI_NONE);

    const GunSettings& GetGun() const;
    void SetGun(const GunSettings& settings);
    const GunState& GetGunState() const;
    pcstr GetGunCaptureStatus() const;
    bool SupportsGunLead() const;
    bool HasGunBoneHint() const;
    void SetGunLeadCapable(bool capable);
    u16 SuggestGunBone() const;
    void SetGunBones(u16 fireBone, u16 lightBone);
    bool HasExternalGun() const;
    void SetExternalGun(u16 anchorBone, const Fmatrix& attachOffset);
    void ClearExternalGun();
    bool GetExternalGunTransform(Fmatrix& pose) const;
    bool CaptureTwoHand();
    void ReleaseTwoHand();
    bool CaptureGunTarget(TargetSpace space);
    void TransformGunDirection(Fvector& direction) const;
    void RequestSnapshot();

private:
    class CallbackLink
    {
    public:
        CHudIKController* controller = nullptr;
        IKinematics* skeleton = nullptr;
        UpdateCallback previous = nullptr;
        void* previousParam = nullptr;
    };

    class Calibration
    {
    public:
        bool valid = false;
        u16 bones[3] = {BI_NONE, BI_NONE, BI_NONE};
        Fmatrix bind[3]{};
        Fmatrix hingeFrame{};
        Fmatrix hingeInverse{};
        XRay::Animation::OzzLimbSolver solver;
        pcstr status = "inactive";
    };

    class Pending
    {
    public:
        bool evaluated = false;
        bool apply = false;
        Fmatrix resolved[3]{};
    };

    class GunPlan
    {
    public:
        bool valid = false;
        bool active = false;
        bool external = false;
        u16 bone = BI_NONE;
        Fmatrix raw{};
        Fmatrix target{};
        Fmatrix delta{};
    };

    class SavedCallback
    {
    public:
        BoneCallback callback = nullptr;
        void* param = nullptr;
        BOOL overwrite = FALSE;
        u32 type = 0;
    };

    class BoneOverride
    {
    public:
        Fmatrix transform{};
    };

    static void FinalCallback(IKinematics* skeleton);
    static void OverrideCallback(CBoneInstance* bone);
    static CallbackLink* FindLink(IKinematics* skeleton);

    void EnsureCallback();
    void OnCalculated();
    bool CapturePose();
    void ApplyDefaults(u16 arm);
    void ClearState(u16 arm, pcstr status);
    void EvaluateArm(u16 arm, bool enabled, const GunPlan& gun, Pending& pending);
    void EvaluateGun(bool enabled, bool referenced, GunPlan& plan);
    bool BuildGunPlan(u16 bone, bool active, GunPlan& plan, pcstr& status) const;
    bool ComputeExternalRaw(Fmatrix& raw, pcstr& status) const;
    void ResetExternal();
    bool ValidateGun(u16 bone, pcstr& status) const;
    u16 ResolveGunBone() const;
    bool IsArmBone(u16 bone) const;
    void ClearGunState(pcstr status);
    bool Calibrate(u16 arm, pcstr& status);
    bool ArmsOverlap(u16 a, u16 b) const;
    bool IsAncestorOrSelf(u16 bone, u16 ancestor) const;
    void ApplyArm(u16 arm, const Pending& pending);
    void ApplyGun(const GunPlan& plan);
    bool IsPoseFresh() const;

    static xr_vector<CallbackLink*> s_links;

    IKinematics* m_skeleton = nullptr;
    CallbackLink* m_link = nullptr;
    ArmSettings m_settings[2];
    ArmState m_state[2];
    GunSettings m_gun;
    GunState m_gunState;
    pcstr m_gunCaptureStatus = "inactive";
    u16 m_fireBone = BI_NONE;
    u16 m_lightBone = BI_NONE;
    bool m_gunLeadCapable = false;
    bool m_externalGun = false;
    u16 m_externalAnchor = BI_NONE;
    Fmatrix m_externalOffset{};
    bool m_externalPublished = false;
    Fmatrix m_externalPose{};
    Calibration m_calibration[2];
    xr_vector<Fmatrix> m_pose;
    bool m_poseValid = false;
    u32 m_poseFrame = 0;
    bool m_snapshotRequested = false;
    bool m_applying = false;
};
