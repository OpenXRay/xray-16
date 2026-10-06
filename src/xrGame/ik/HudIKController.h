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
        Bone
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
    void EvaluateArm(u16 arm, bool enabled, Pending& pending);
    bool Calibrate(u16 arm, pcstr& status);
    bool ArmsOverlap(u16 a, u16 b) const;
    bool IsAncestorOrSelf(u16 bone, u16 ancestor) const;
    void ApplyArm(u16 arm, const Pending& pending);
    bool IsPoseFresh() const;

    static xr_vector<CallbackLink*> s_links;

    IKinematics* m_skeleton = nullptr;
    CallbackLink* m_link = nullptr;
    ArmSettings m_settings[2];
    ArmState m_state[2];
    Calibration m_calibration[2];
    xr_vector<Fmatrix> m_pose;
    bool m_poseValid = false;
    u32 m_poseFrame = 0;
    bool m_snapshotRequested = false;
    bool m_applying = false;
};
