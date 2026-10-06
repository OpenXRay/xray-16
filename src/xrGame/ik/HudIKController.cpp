#include "StdAfx.h"
#include "HudIKController.h"

#include <algorithm>
#include <cmath>

namespace
{
using Solver = XRay::Animation::OzzLimbSolver;

constexpr float RigidTolerance = 1e-3f;
constexpr float HingeTolerance = 1e-3f;
constexpr u32 FreshFrames = 3;
constexpr pcstr DefaultBoneNames[2][3] = {
    {"bip01_l_upperarm", "bip01_l_forearm", "bip01_l_hand"},
    {"bip01_r_upperarm", "bip01_r_forearm", "bip01_r_hand"},
};

bool IsRigid(const Fmatrix& matrix)
{
    if (!_valid(matrix))
    {
        return false;
    }
    const float affineError = std::max({_abs(matrix._14), _abs(matrix._24), _abs(matrix._34),
        _abs(matrix._44 - 1.f)});
    if (affineError > RigidTolerance)
    {
        return false;
    }
    if (_abs(matrix.i.square_magnitude() - 1.f) > RigidTolerance ||
        _abs(matrix.j.square_magnitude() - 1.f) > RigidTolerance ||
        _abs(matrix.k.square_magnitude() - 1.f) > RigidTolerance)
    {
        return false;
    }
    if (_abs(matrix.i.dotproduct(matrix.j)) > RigidTolerance ||
        _abs(matrix.i.dotproduct(matrix.k)) > RigidTolerance ||
        _abs(matrix.j.dotproduct(matrix.k)) > RigidTolerance)
    {
        return false;
    }
    Fvector cross;
    cross.crossproduct(matrix.j, matrix.k);
    return matrix.i.dotproduct(cross) > 0.f;
}

bool BuildHingeFrame(const Fmatrix& middleBind, const Fmatrix& endBind, Fmatrix& frame, Fmatrix& inverse)
{
    Fvector upper;
    upper.set(middleBind.i.dotproduct(middleBind.c), middleBind.j.dotproduct(middleBind.c),
        middleBind.k.dotproduct(middleBind.c));
    const Fvector lower = endBind.c;
    const float upperLength = upper.magnitude();
    const float lowerLength = lower.magnitude();
    if (!_valid(upperLength) || !_valid(lowerLength) || upperLength <= EPS || lowerLength <= EPS)
    {
        return false;
    }

    Fvector forward = upper;
    forward.mul(1.f / upperLength);

    Fvector normal;
    normal.crossproduct(upper, lower);
    const float normalLength = normal.magnitude();
    if (_valid(normalLength) && normalLength > HingeTolerance * upperLength * lowerLength)
    {
        normal.mul(1.f / normalLength);
    }
    else
    {
        Fvector axes[3];
        axes[0].set(0.f, 1.f, 0.f);
        axes[1].set(1.f, 0.f, 0.f);
        axes[2].set(0.f, 0.f, 1.f);
        u32 least = 0;
        for (u32 axis = 1; axis < 3; ++axis)
        {
            if (_abs(forward.dotproduct(axes[axis])) < _abs(forward.dotproduct(axes[least])))
            {
                least = axis;
            }
        }
        normal = axes[least];
    }

    normal.mad(forward, -normal.dotproduct(forward));
    const float projectedLength = normal.magnitude();
    if (!_valid(projectedLength) || projectedLength <= EPS)
    {
        return false;
    }
    normal.mul(1.f / projectedLength);

    Fvector side;
    side.crossproduct(forward, normal);
    frame.identity();
    frame.i.set(forward);
    frame.j.set(normal);
    frame.k.set(side);
    if (!IsRigid(frame))
    {
        return false;
    }

    inverse.identity();
    inverse.i.set(frame.i.x, frame.j.x, frame.k.x);
    inverse.j.set(frame.i.y, frame.j.y, frame.k.y);
    inverse.k.set(frame.i.z, frame.j.z, frame.k.z);
    return IsRigid(inverse);
}

bool BlendLocal(const Fmatrix& animated, const Fmatrix& solved, float weight, Fmatrix& result)
{
    Fquaternion animatedRotation;
    Fquaternion solvedRotation;
    Fquaternion blended;
    animatedRotation.set(animated);
    animatedRotation.normalize();
    solvedRotation.set(solved);
    solvedRotation.normalize();
    if (!_valid(animatedRotation) || !_valid(solvedRotation))
    {
        return false;
    }
    blended.slerp(animatedRotation, solvedRotation, weight);
    blended.normalize();
    if (!_valid(blended))
    {
        return false;
    }
    result.identity();
    result.rotation(blended);
    result.c.lerp(animated.c, solved.c, weight);
    return _valid(result);
}

float Distance(const Fvector& a, const Fvector& b)
{
    return a.distance_to(b);
}
}

xr_vector<CHudIKController::CallbackLink*> CHudIKController::s_links;

CHudIKController::CHudIKController()
{
    for (u16 arm = 0; arm < 2; ++arm)
    {
        m_state[arm].status = "inactive";
    }
}

CHudIKController::~CHudIKController()
{
    Unbind();
}

void CHudIKController::Bind(IKinematics* skeleton)
{
    Unbind();
    if (!skeleton)
    {
        return;
    }
    m_skeleton = skeleton;
    for (u16 arm = 0; arm < 2; ++arm)
    {
        ApplyDefaults(arm);
        m_calibration[arm].valid = false;
        ClearState(arm, "disabled");
    }
    m_pose.clear();
    m_poseValid = false;
    m_snapshotRequested = false;
}

void CHudIKController::Unbind()
{
    if (m_link)
    {
        CallbackLink* link = m_link;
        m_link = nullptr;
        if (m_skeleton && m_skeleton->GetUpdateCallback() == &CHudIKController::FinalCallback &&
            m_skeleton->GetUpdateCallbackParam() == link)
        {
            m_skeleton->Callback(link->previous, link->previousParam);
        }
        const auto it = std::find(s_links.begin(), s_links.end(), link);
        if (it != s_links.end())
        {
            s_links.erase(it);
        }
        xr_delete(link);
    }
    m_skeleton = nullptr;
    for (u16 arm = 0; arm < 2; ++arm)
    {
        m_settings[arm] = ArmSettings();
        m_calibration[arm].valid = false;
        ClearState(arm, "inactive");
    }
    m_pose.clear();
    m_poseValid = false;
    m_snapshotRequested = false;
    m_applying = false;
}

IKinematics* CHudIKController::Skeleton() const
{
    return m_skeleton;
}

const CHudIKController::ArmSettings& CHudIKController::GetArm(u16 arm) const
{
    static const ArmSettings fallback;
    return arm < 2 ? m_settings[arm] : fallback;
}

void CHudIKController::SetArm(u16 arm, const ArmSettings& settings)
{
    if (arm > 1 || !m_skeleton)
    {
        return;
    }
    ArmSettings& current = m_settings[arm];
    const bool wasEnabled = current.enabled;
    if (current.bones[0] != settings.bones[0] || current.bones[1] != settings.bones[1] ||
        current.bones[2] != settings.bones[2])
    {
        m_calibration[arm].valid = false;
    }
    current = settings;
    if (!current.enabled)
    {
        if (wasEnabled)
        {
            ClearState(arm, "disabled");
        }
    }
    else
    {
        if (!wasEnabled)
        {
            m_state[arm].solved = false;
            m_state[arm].status = "pending";
        }
        EnsureCallback();
    }
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

const CHudIKController::ArmState& CHudIKController::GetState(u16 arm) const
{
    static const ArmState fallback;
    return arm < 2 ? m_state[arm] : fallback;
}

void CHudIKController::ResetArm(u16 arm)
{
    if (arm > 1 || !m_skeleton)
    {
        return;
    }
    ApplyDefaults(arm);
    m_calibration[arm].valid = false;
    ClearState(arm, "disabled");
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

void CHudIKController::ResetAll()
{
    if (!m_skeleton)
    {
        return;
    }
    m_snapshotRequested = false;
    m_poseValid = false;
    m_pose.clear();
    for (u16 arm = 0; arm < 2; ++arm)
    {
        ApplyDefaults(arm);
        m_calibration[arm].valid = false;
        ClearState(arm, "disabled");
    }
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

bool CHudIKController::CaptureTarget(u16 arm, TargetSpace space, u16 targetBone)
{
    if (arm > 1 || !m_skeleton)
    {
        return false;
    }
    if (!IsPoseFresh())
    {
        RequestSnapshot();
        return false;
    }
    const ArmSettings& settings = m_settings[arm];
    const u16 count = u16(m_pose.size());
    const u16 wristId = settings.bones[2];
    if (wristId >= count || !m_skeleton->LL_GetBoneVisible(wristId))
    {
        return false;
    }
    const Fmatrix& wrist = m_pose[wristId];
    if (!IsRigid(wrist))
    {
        return false;
    }

    Fmatrix offset;
    u16 storedBone = BI_NONE;
    switch (space)
    {
    case TargetSpace::Animated:
        offset.identity();
        break;
    case TargetSpace::Model:
        offset.set(wrist);
        break;
    case TargetSpace::Bone:
    {
        if (targetBone >= count || !m_skeleton->LL_GetBoneVisible(targetBone))
        {
            return false;
        }
        const Fmatrix& reference = m_pose[targetBone];
        Fmatrix inverse;
        if (!IsRigid(reference) || !inverse.invert_b(reference))
        {
            return false;
        }
        offset.mul_43(inverse, wrist);
        storedBone = targetBone;
        break;
    }
    default:
        return false;
    }

    Fvector hpb;
    offset.getHPB(hpb);
    hpb.mul(180.f / PI);
    if (!_valid(offset.c) || !_valid(hpb))
    {
        return false;
    }

    ArmSettings& target = m_settings[arm];
    target.space = space;
    target.targetBone = storedBone;
    target.position = offset.c;
    target.rotation = hpb;
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
    return true;
}

void CHudIKController::RequestSnapshot()
{
    if (!m_skeleton)
    {
        return;
    }
    m_snapshotRequested = true;
    EnsureCallback();
}

CHudIKController::CallbackLink* CHudIKController::FindLink(IKinematics* skeleton)
{
    void* param = skeleton->GetUpdateCallbackParam();
    for (CallbackLink* link : s_links)
    {
        if (link == param && link->skeleton == skeleton)
        {
            return link;
        }
    }
    return nullptr;
}

void CHudIKController::FinalCallback(IKinematics* skeleton)
{
    CallbackLink* link = FindLink(skeleton);
    if (!link)
    {
        return;
    }
    const UpdateCallback entryCallback = skeleton->GetUpdateCallback();
    void* entryParam = skeleton->GetUpdateCallbackParam();
    const UpdateCallback previous = link->previous;
    void* previousParam = link->previousParam;
    if (previous)
    {
        skeleton->Callback(previous, previousParam);
        previous(skeleton);
        if (std::find(s_links.begin(), s_links.end(), link) == s_links.end())
        {
            return;
        }
        if (skeleton->GetUpdateCallback() != previous ||
            skeleton->GetUpdateCallbackParam() != previousParam)
        {
            return;
        }
        skeleton->Callback(entryCallback, entryParam);
    }
    if (link->controller && link->controller->m_skeleton == skeleton)
    {
        link->controller->OnCalculated();
    }
}

void CHudIKController::OverrideCallback(CBoneInstance* bone)
{
    const auto* data = static_cast<const BoneOverride*>(bone->callback_param());
    bone->mTransform = data->transform;
}

void CHudIKController::EnsureCallback()
{
    if (!m_skeleton || m_link)
    {
        return;
    }
    CallbackLink* link = xr_new<CallbackLink>();
    link->controller = this;
    link->skeleton = m_skeleton;
    link->previous = m_skeleton->GetUpdateCallback();
    link->previousParam = m_skeleton->GetUpdateCallbackParam();
    s_links.push_back(link);
    m_link = link;
    m_skeleton->Callback(&CHudIKController::FinalCallback, link);
}

void CHudIKController::ApplyDefaults(u16 arm)
{
    m_settings[arm] = ArmSettings();
    if (!m_skeleton)
    {
        return;
    }
    const u16 count = m_skeleton->LL_BoneCount();
    for (u16 j = 0; j < 3; ++j)
    {
        const u16 bone = m_skeleton->LL_BoneID(DefaultBoneNames[arm][j]);
        m_settings[arm].bones[j] = bone < count ? bone : BI_NONE;
    }
}

void CHudIKController::ClearState(u16 arm, pcstr status)
{
    m_state[arm] = ArmState();
    m_state[arm].status = status;
}

bool CHudIKController::IsPoseFresh() const
{
    return m_poseValid && Device.dwFrame >= m_poseFrame && Device.dwFrame - m_poseFrame <= FreshFrames;
}

bool CHudIKController::CapturePose()
{
    const u16 count = m_skeleton->LL_BoneCount();
    if (!count)
    {
        m_poseValid = false;
        return false;
    }
    m_pose.resize(count);
    for (u16 bone = 0; bone < count; ++bone)
    {
        m_pose[bone] = m_skeleton->LL_GetTransform(bone);
    }
    m_poseValid = true;
    m_poseFrame = Device.dwFrame;
    return true;
}

void CHudIKController::OnCalculated()
{
    if (m_applying || !m_skeleton)
    {
        return;
    }
    const bool enabled[2] = {m_settings[0].enabled, m_settings[1].enabled};
    if (!enabled[0] && !enabled[1] && !m_snapshotRequested)
    {
        return;
    }
    const bool preview = m_snapshotRequested;
    m_snapshotRequested = false;

    if (!CapturePose())
    {
        for (u16 arm = 0; arm < 2; ++arm)
        {
            if (enabled[arm] || preview)
            {
                ClearState(arm, "no_skeleton");
            }
        }
        return;
    }

    Pending pending[2];
    for (u16 arm = 0; arm < 2; ++arm)
    {
        if (enabled[arm] || preview)
        {
            EvaluateArm(arm, enabled[arm], pending[arm]);
        }
    }

    if (enabled[0] && enabled[1] && ArmsOverlap(0, 1))
    {
        for (u16 arm = 0; arm < 2; ++arm)
        {
            ArmState& state = m_state[arm];
            pending[arm].apply = false;
            state.solved = false;
            state.status = "arm_overlap";
            for (u16 j = 0; j < 3; ++j)
            {
                state.resolved[j] = state.animated[j];
            }
            state.error = Distance(state.animated[2].c, state.target.c);
        }
    }

    for (u16 arm = 0; arm < 2; ++arm)
    {
        if (pending[arm].apply)
        {
            ApplyArm(arm, pending[arm]);
        }
    }
}

bool CHudIKController::Calibrate(u16 arm, pcstr& status)
{
    Calibration& calibration = m_calibration[arm];
    const ArmSettings& settings = m_settings[arm];
    if (calibration.valid && calibration.bones[0] == settings.bones[0] &&
        calibration.bones[1] == settings.bones[1] && calibration.bones[2] == settings.bones[2])
    {
        return true;
    }
    calibration.valid = false;

    Fmatrix bind[3];
    for (u16 j = 0; j < 3; ++j)
    {
        bind[j] = m_skeleton->GetBoneData(settings.bones[j]).get_bind_transform();
        if (!IsRigid(bind[j]))
        {
            status = "non_rigid_bone";
            return false;
        }
    }
    Fmatrix hingeFrame;
    Fmatrix hingeInverse;
    const bool framed = BuildHingeFrame(bind[1], bind[2], hingeFrame, hingeInverse);
    if (!framed)
    {
        hingeFrame.identity();
        hingeInverse.identity();
    }
    Fmatrix virtualMiddle;
    Fmatrix virtualEnd;
    virtualMiddle.mul_43(bind[1], hingeFrame);
    virtualEnd.mul_43(hingeInverse, bind[2]);
    if (!calibration.solver.Initialize(virtualMiddle, virtualEnd))
    {
        Fmatrix rotations[3];
        Fvector knee;
        knee.set(0.f, 0.f, 0.f);
        const Solver::Result result = calibration.solver.Solve(Fidentity, Fidentity, knee, rotations);
        status = result.failure == Solver::Failure::None ? "calibration_failed"
                                                         : Solver::FailureName(result.failure);
        return false;
    }
    if (!framed)
    {
        status = "calibration_failed";
        return false;
    }
    calibration.hingeFrame = hingeFrame;
    calibration.hingeInverse = hingeInverse;
    for (u16 j = 0; j < 3; ++j)
    {
        calibration.bind[j] = bind[j];
        calibration.bones[j] = settings.bones[j];
    }
    calibration.valid = true;
    return true;
}

bool CHudIKController::IsAncestorOrSelf(u16 bone, u16 ancestor) const
{
    const u16 count = m_skeleton->LL_BoneCount();
    for (u16 steps = 0; bone != BI_NONE && bone < count && steps <= count; ++steps)
    {
        if (bone == ancestor)
        {
            return true;
        }
        bone = m_skeleton->GetBoneData(bone).GetParentID();
    }
    return false;
}

bool CHudIKController::ArmsOverlap(u16 a, u16 b) const
{
    for (u16 j = 0; j < 3; ++j)
    {
        if (IsAncestorOrSelf(m_settings[b].bones[j], m_settings[a].bones[0]) ||
            IsAncestorOrSelf(m_settings[a].bones[j], m_settings[b].bones[0]))
        {
            return true;
        }
    }
    return false;
}

void CHudIKController::EvaluateArm(u16 arm, bool enabled, Pending& pending)
{
    pending = Pending();
    pending.evaluated = true;

    ArmState& state = m_state[arm];
    state = ArmState();
    state.frame = Device.dwFrame;
    state.status = "inactive";

    const ArmSettings& settings = m_settings[arm];
    const u16 count = u16(m_pose.size());

    if (!_valid(settings.position) || !_valid(settings.rotation) || !_valid(settings.elbowOffset) ||
        !_valid(settings.weight))
    {
        state.status = "invalid_settings";
        return;
    }

    const u16 b0 = settings.bones[0];
    const u16 b1 = settings.bones[1];
    const u16 b2 = settings.bones[2];
    if (b0 >= count || b1 >= count || b2 >= count)
    {
        state.status = "missing_bones";
        return;
    }
    const u16 parentId = m_skeleton->GetBoneData(b0).GetParentID();
    if (b0 == b1 || b1 == b2 || b0 == b2 || m_skeleton->GetBoneData(b1).GetParentID() != b0 ||
        m_skeleton->GetBoneData(b2).GetParentID() != b1)
    {
        state.status = "not_direct_chain";
        return;
    }
    if (parentId != BI_NONE && parentId >= count)
    {
        state.status = "not_direct_chain";
        return;
    }
    if (!m_skeleton->LL_GetBoneVisible(b0) || !m_skeleton->LL_GetBoneVisible(b1) ||
        !m_skeleton->LL_GetBoneVisible(b2) || (parentId != BI_NONE && !m_skeleton->LL_GetBoneVisible(parentId)))
    {
        state.status = "hidden_bone";
        return;
    }
    const u16 ids[3] = {b0, b1, b2};
    for (u16 j = 0; j < 3; ++j)
    {
        CBoneInstance& instance = m_skeleton->LL_GetBoneInstance(ids[j]);
        if (instance.callback() || instance.callback_overwrite())
        {
            state.status = "bone_callback_conflict";
            return;
        }
    }

    const Fmatrix& parentPose = parentId == BI_NONE ? Fidentity : m_pose[parentId];
    if (!IsRigid(parentPose) || !IsRigid(m_pose[b0]) || !IsRigid(m_pose[b1]) || !IsRigid(m_pose[b2]))
    {
        state.status = "non_rigid_bone";
        return;
    }

    Fmatrix reference;
    switch (settings.space)
    {
    case TargetSpace::Animated:
        reference.set(m_pose[b2]);
        break;
    case TargetSpace::Model:
        reference.identity();
        break;
    case TargetSpace::Bone:
        if (settings.targetBone >= count)
        {
            state.status = "target_bone_missing";
            return;
        }
        if (!m_skeleton->LL_GetBoneVisible(settings.targetBone))
        {
            state.status = "target_bone_hidden";
            return;
        }
        if (!IsRigid(m_pose[settings.targetBone]))
        {
            state.status = "non_rigid_target";
            return;
        }
        reference.set(m_pose[settings.targetBone]);
        break;
    default:
        state.status = "invalid_settings";
        return;
    }

    Fvector radians = settings.rotation;
    radians.mul(PI / 180.f);
    Fmatrix offset;
    offset.setHPB(radians.x, radians.y, radians.z);
    offset.translate_over(settings.position);
    state.target.mul_43(reference, offset);
    if (!_valid(state.target))
    {
        state.status = "invalid_settings";
        return;
    }
    state.elbow.add(m_pose[b1].c, settings.elbowOffset);
    for (u16 j = 0; j < 3; ++j)
    {
        state.animated[j] = m_pose[ids[j]];
        state.resolved[j] = m_pose[ids[j]];
    }
    state.error = Distance(state.animated[2].c, state.target.c);
    state.valid = _valid(state.elbow) && _valid(state.error);
    if (!state.valid)
    {
        state.status = "invalid_settings";
        return;
    }

    const float weight = std::clamp(settings.weight, 0.f, 1.f);
    if (weight <= 0.f)
    {
        state.status = "weight_zero";
        return;
    }

    pcstr status = "inactive";
    if (!Calibrate(arm, status))
    {
        state.status = status;
        return;
    }
    const Calibration& calibration = m_calibration[arm];

    Fmatrix start;
    start.mul_43(parentPose, calibration.bind[0]);
    start.c = m_pose[b0].c;

    Fmatrix rotations[3];
    const Solver::Result result = calibration.solver.Solve(start, state.target, state.elbow, rotations);
    if (result.failure != Solver::Failure::None)
    {
        state.status = Solver::FailureName(result.failure);
        return;
    }

    Fmatrix solved[3];
    Fmatrix chain;
    Fmatrix hingeLocal;
    Fmatrix middleRotation;
    hingeLocal.mul_43(calibration.hingeFrame, rotations[1]);
    middleRotation.mul_43(hingeLocal, calibration.hingeInverse);
    solved[0].mul_43(start, rotations[0]);
    chain.mul_43(solved[0], calibration.bind[1]);
    solved[1].mul_43(chain, middleRotation);
    chain.mul_43(solved[1], calibration.bind[2]);
    solved[2].mul_43(chain, rotations[2]);
    if (!IsRigid(solved[0]) || !IsRigid(solved[1]) || !IsRigid(solved[2]))
    {
        state.status = "solution_non_rigid";
        return;
    }

    Fmatrix resolved[3];
    if (weight >= 1.f)
    {
        for (u16 j = 0; j < 3; ++j)
        {
            resolved[j] = solved[j];
        }
    }
    else
    {
        const Fmatrix* animatedParents[3] = {&parentPose, &m_pose[b0], &m_pose[b1]};
        const Fmatrix* solvedParents[3] = {&parentPose, &solved[0], &solved[1]};
        Fmatrix blended[3];
        for (u16 j = 0; j < 3; ++j)
        {
            Fmatrix animatedInverse;
            Fmatrix solvedInverse;
            if (!animatedInverse.invert_b(*animatedParents[j]) || !solvedInverse.invert_b(*solvedParents[j]))
            {
                state.status = "singular_transform";
                return;
            }
            Fmatrix animatedLocal;
            Fmatrix solvedLocal;
            animatedLocal.mul_43(animatedInverse, m_pose[ids[j]]);
            solvedLocal.mul_43(solvedInverse, solved[j]);
            if (!BlendLocal(animatedLocal, solvedLocal, weight, blended[j]))
            {
                state.status = "blend_failed";
                return;
            }
        }
        resolved[0].mul_43(parentPose, blended[0]);
        resolved[1].mul_43(resolved[0], blended[1]);
        resolved[2].mul_43(resolved[1], blended[2]);
        if (!IsRigid(resolved[0]) || !IsRigid(resolved[1]) || !IsRigid(resolved[2]))
        {
            state.status = "solution_non_rigid";
            return;
        }
    }

    for (u16 j = 0; j < 3; ++j)
    {
        state.resolved[j] = resolved[j];
        pending.resolved[j] = resolved[j];
    }
    state.error = Distance(resolved[2].c, state.target.c);
    state.solved = enabled;
    state.status = enabled ? "ok" : "preview";
    pending.apply = enabled;
}

void CHudIKController::ApplyArm(u16 arm, const Pending& pending)
{
    const Calibration& calibration = m_calibration[arm];
    BoneOverride overrides[3];
    SavedCallback saved[3];
    for (u16 j = 0; j < 3; ++j)
    {
        CBoneInstance& instance = m_skeleton->LL_GetBoneInstance(calibration.bones[j]);
        saved[j].callback = instance.callback();
        saved[j].param = instance.callback_param();
        saved[j].overwrite = instance.callback_overwrite();
        saved[j].type = instance.callback_type();
        overrides[j].transform = pending.resolved[j];
        instance.set_callback(bctCustom, &CHudIKController::OverrideCallback, &overrides[j], TRUE);
    }

    Fmatrix identity;
    identity.identity();
    const u16 parentId = m_skeleton->GetBoneData(calibration.bones[0]).GetParentID();
    Fmatrix* parent = parentId == BI_NONE ? &identity : &m_skeleton->LL_GetTransform(parentId);

    m_applying = true;
    m_skeleton->Bone_Calculate(&m_skeleton->LL_GetData(calibration.bones[0]), parent);
    m_applying = false;

    for (u16 j = 0; j < 3; ++j)
    {
        CBoneInstance& instance = m_skeleton->LL_GetBoneInstance(calibration.bones[j]);
        instance.set_callback(saved[j].type, saved[j].callback, saved[j].param, saved[j].overwrite);
    }
}
