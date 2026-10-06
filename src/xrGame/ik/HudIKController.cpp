#include "StdAfx.h"
#include "HudIKController.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
using Solver = XRay::Animation::OzzLimbSolver;

constexpr float RigidTolerance = 1e-3f;
constexpr float HingeTolerance = 1e-3f;
constexpr u32 FreshFrames = 3;
constexpr float IdentityTolerance = 1e-5f;
constexpr float ZeroPositionTolerance = 1e-6f;
constexpr float ZeroRotationTolerance = 1e-4f;
constexpr u32 AutoRetryFrames = 10;
constexpr float BendReferenceFraction = 0.02f;
constexpr float BendAxisTolerance = 1e-6f;
constexpr float BendOppositeTolerance = 1e-3f;
constexpr float HingeSideTolerance = 1e-2f;
constexpr pcstr DefaultBoneNames[2][2][3] = {
    {
        {"l_upperarm", "l_forearm", "l_hand"},
        {"r_upperarm", "r_forearm", "r_hand"},
    },
    {
        {"bip01_l_upperarm", "bip01_l_forearm", "bip01_l_hand"},
        {"bip01_r_upperarm", "bip01_r_forearm", "bip01_r_hand"},
    },
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

bool SameVector(const Fvector& a, const Fvector& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool SameGun(const CHudIKController::GunSettings& a, const CHudIKController::GunSettings& b)
{
    return a.enabled == b.enabled && a.bone == b.bone && a.space == b.space && SameVector(a.position, b.position) &&
        SameVector(a.rotation, b.rotation);
}

bool SameMatrix(const Fmatrix& a, const Fmatrix& b)
{
    for (u32 row = 0; row < 4; ++row)
    {
        for (u32 column = 0; column < 4; ++column)
        {
            if (a.m[row][column] != b.m[row][column])
            {
                return false;
            }
        }
    }
    return true;
}

bool IsIdentityDelta(const Fmatrix& matrix)
{
    if (!_valid(matrix))
    {
        return false;
    }
    for (u32 row = 0; row < 4; ++row)
    {
        for (u32 column = 0; column < 4; ++column)
        {
            if (_abs(matrix.m[row][column] - Fidentity.m[row][column]) > IdentityTolerance)
            {
                return false;
            }
        }
    }
    return true;
}

bool IsNearZero(const Fvector& vector, float tolerance)
{
    return _abs(vector.x) <= tolerance && _abs(vector.y) <= tolerance && _abs(vector.z) <= tolerance;
}

bool ToDegrees(const Fmatrix& matrix, Fvector& position, Fvector& rotation)
{
    Fvector hpb;
    matrix.getHPB(hpb);
    hpb.mul(180.f / PI);
    if (!_valid(matrix.c) || !_valid(hpb))
    {
        return false;
    }
    position = matrix.c;
    rotation = hpb;
    return true;
}

bool BoneIsAncestorOrSelf(IKinematics* skeleton, u16 bone, u16 ancestor)
{
    const u16 count = skeleton->LL_BoneCount();
    for (u16 steps = 0; bone != BI_NONE && bone < count && steps <= count; ++steps)
    {
        if (bone == ancestor)
        {
            return true;
        }
        bone = skeleton->GetBoneData(bone).GetParentID();
    }
    return false;
}

bool IsDirectChain(IKinematics* skeleton, const u16 (&bones)[3])
{
    const u16 count = skeleton->LL_BoneCount();
    for (u16 j = 0; j < 3; ++j)
    {
        if (bones[j] >= count)
        {
            return false;
        }
    }
    if (bones[0] == bones[1] || bones[1] == bones[2] || bones[0] == bones[2])
    {
        return false;
    }
    return skeleton->GetBoneData(bones[1]).GetParentID() == bones[0] &&
        skeleton->GetBoneData(bones[2]).GetParentID() == bones[1];
}

bool ChainsOverlap(IKinematics* skeleton, const u16 (&a)[3], const u16 (&b)[3])
{
    for (u16 j = 0; j < 3; ++j)
    {
        if (BoneIsAncestorOrSelf(skeleton, b[j], a[0]) || BoneIsAncestorOrSelf(skeleton, a[j], b[0]))
        {
            return true;
        }
    }
    return false;
}

void ResolveDefaultBones(IKinematics* skeleton, u16 (&result)[2][3])
{
    const u16 count = skeleton->LL_BoneCount();
    u16 resolved[2][2][3];
    u32 found[2] = {0, 0};
    for (u32 layout = 0; layout < 2; ++layout)
    {
        for (u32 arm = 0; arm < 2; ++arm)
        {
            for (u32 j = 0; j < 3; ++j)
            {
                const u16 bone = skeleton->LL_BoneID(DefaultBoneNames[layout][arm][j]);
                resolved[layout][arm][j] = bone < count ? bone : u16(BI_NONE);
                if (resolved[layout][arm][j] != BI_NONE)
                {
                    ++found[layout];
                }
            }
        }
    }
    u32 choice = found[1] > found[0] ? 1 : 0;
    for (u32 layout = 0; layout < 2; ++layout)
    {
        if (found[layout] == 6 && IsDirectChain(skeleton, resolved[layout][0]) &&
            IsDirectChain(skeleton, resolved[layout][1]) &&
            !ChainsOverlap(skeleton, resolved[layout][0], resolved[layout][1]))
        {
            choice = layout;
            break;
        }
    }
    for (u32 arm = 0; arm < 2; ++arm)
    {
        for (u32 j = 0; j < 3; ++j)
        {
            result[arm][j] = resolved[choice][arm][j];
        }
    }
}

Fvector TransportBendHint(const Fvector& shoulder, const Fvector& animatedElbow, const Fvector& animatedWrist,
    const Fvector& target, const Fvector& hingeAxis, float limbLength, const Fvector& offset)
{
    Fvector fallback;
    fallback.add(animatedElbow, offset);

    Fvector from;
    Fvector to;
    from.sub(animatedWrist, shoulder);
    to.sub(target, shoulder);
    const float fromLength = from.magnitude();
    const float toLength = to.magnitude();
    if (!_valid(fromLength) || !_valid(toLength) || fromLength <= EPS || toLength <= EPS)
    {
        return fallback;
    }
    from.mul(1.f / fromLength);
    to.mul(1.f / toLength);

    Fvector bend;
    bend.sub(animatedElbow, shoulder);
    const float along = bend.dotproduct(from);
    bend.mad(from, -along);
    float bendLength = bend.magnitude();
    if (!_valid(bendLength))
    {
        return fallback;
    }
    const float referenceLength = BendReferenceFraction * limbLength;
    Fvector side;
    float effectiveLength = bendLength;
    if (bendLength < referenceLength)
    {
        Fvector hingeSide;
        hingeSide.crossproduct(from, hingeAxis);
        const float hingeLength = hingeSide.magnitude();
        if (_valid(hingeLength) && hingeLength > HingeSideTolerance)
        {
            hingeSide.mul(1.f / hingeLength);
            float weight = bendLength / referenceLength;
            weight = weight * weight * (3.f - 2.f * weight);
            side = hingeSide;
            if (bendLength > EPS)
            {
                Fvector chordSide = bend;
                chordSide.mul(1.f / bendLength);
                Fvector turn;
                turn.crossproduct(hingeSide, chordSide);
                const float turnCosine = hingeSide.dotproduct(chordSide);
                const float turnSine = turn.dotproduct(from);
                float angle = PI;
                if (turnCosine >= 0.f || _abs(turnSine) > BendOppositeTolerance)
                {
                    angle = std::atan2(turnSine, turnCosine);
                }
                angle *= weight;
                Fvector swing;
                swing.crossproduct(from, hingeSide);
                side.mul(std::cos(angle));
                side.mad(swing, std::sin(angle));
            }
            effectiveLength = referenceLength;
        }
    }
    if (effectiveLength == bendLength)
    {
        if (bendLength <= EPS)
        {
            return fallback;
        }
        side = bend;
        side.mul(1.f / bendLength);
    }
    bendLength = effectiveLength;

    const float cosine = std::clamp(from.dotproduct(to), -1.f, 1.f);
    Fvector axis;
    axis.crossproduct(from, to);
    const float sine = axis.magnitude();
    const float axisTolerance = cosine < 0.f ? BendOppositeTolerance : BendAxisTolerance;
    Fvector rotated = side;
    if (_valid(sine) && sine > axisTolerance)
    {
        axis.mul(1.f / sine);
        Fvector swing;
        swing.crossproduct(axis, side);
        rotated.mul(cosine);
        rotated.mad(swing, sine);
        rotated.mad(axis, axis.dotproduct(side) * (1.f - cosine));
    }

    Fvector hint;
    hint.mad(shoulder, to, along);
    hint.mad(rotated, bendLength);
    hint.add(offset);
    return _valid(hint) ? hint : fallback;
}

bool IsTransientFailure(pcstr status)
{
    const pcstr transient[] = {"hidden_bone", "bone_callback_conflict", "non_rigid_bone", "singular_transform",
        "stale_pose"};
    for (pcstr entry : transient)
    {
        if (std::strcmp(status, entry) == 0)
        {
            return true;
        }
    }
    return false;
}
}

xr_vector<CHudIKController::CallbackLink*> CHudIKController::s_links;

CHudIKController::CHudIKController()
{
    for (u16 arm = 0; arm < 2; ++arm)
    {
        m_state[arm].status = "inactive";
    }
    ResetExternal();
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
    m_gun = GunSettings();
    m_gunCaptureStatus = "inactive";
    ClearGunState("disabled");
    ResetExternal();
    ClearCollisionOffset();
    m_pose.clear();
    m_poseValid = false;
    m_snapshotRequested = false;
    ArmAuto();
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
    m_gun = GunSettings();
    m_gunCaptureStatus = "inactive";
    m_gunLeadCapable = false;
    m_fireBone = BI_NONE;
    m_lightBone = BI_NONE;
    ClearGunState("inactive");
    ResetExternal();
    ClearCollisionOffset();
    m_pose.clear();
    m_poseValid = false;
    m_snapshotRequested = false;
    m_autoState = AutoState::Idle;
    m_autoReady = false;
    m_autoRetryFrame = 0;
    m_autoStatus = "idle";
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
    const bool bonesChanged = current.bones[0] != settings.bones[0] || current.bones[1] != settings.bones[1] ||
        current.bones[2] != settings.bones[2];
    if (bonesChanged)
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
    if (wasEnabled != current.enabled || (current.enabled && m_autoState == AutoState::Pending))
    {
        CancelAuto();
    }
    else if (bonesChanged && m_autoState == AutoState::Pending)
    {
        RefreshAuto();
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
    const bool wasEnabled = m_settings[arm].enabled;
    ApplyDefaults(arm);
    m_calibration[arm].valid = false;
    ClearState(arm, "disabled");
    if (wasEnabled)
    {
        CancelAuto();
    }
    else if (m_autoState == AutoState::Pending)
    {
        RefreshAuto();
    }
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
    m_gun = GunSettings();
    m_gunCaptureStatus = "inactive";
    ClearGunState("disabled");
    ResetExternal();
    ClearCollisionOffset();
    ArmAuto();
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
    case TargetSpace::Gun:
    {
        GunPlan plan;
        pcstr gunStatus = "inactive";
        const u16 gunBone = m_externalGun ? u16(BI_NONE) : (m_gun.bone != BI_NONE ? m_gun.bone : ResolveGunBone());
        if (!BuildGunPlan(gunBone, m_gun.enabled, plan, gunStatus))
        {
            return false;
        }
        Fmatrix gunInverse;
        if (!gunInverse.invert_b(m_gun.enabled ? plan.target : plan.raw))
        {
            return false;
        }
        offset.mul_43(gunInverse, wrist);
        break;
    }
    case TargetSpace::GunAnimated:
        offset.identity();
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
    u16 bones[2][3];
    ResolveDefaultBones(m_skeleton, bones);
    for (u16 j = 0; j < 3; ++j)
    {
        m_settings[arm].bones[j] = bones[arm][j];
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
    bool enabled[2] = {m_settings[0].enabled, m_settings[1].enabled};
    bool gunEnabled = m_gun.enabled;
    if (!gunEnabled)
    {
        ClearCollisionOffset();
    }
    const bool autoDue = IsAutoDue();
    if (!enabled[0] && !enabled[1] && !gunEnabled && !m_snapshotRequested && !autoDue)
    {
        m_externalPublished = false;
        return;
    }
    const bool preview = m_snapshotRequested;
    m_snapshotRequested = false;

    if (!CapturePose())
    {
        m_externalPublished = false;
        for (u16 arm = 0; arm < 2; ++arm)
        {
            if (enabled[arm] || preview)
            {
                ClearState(arm, "no_skeleton");
            }
        }
        if (gunEnabled || preview)
        {
            ClearGunState("no_skeleton");
        }
        if (autoDue)
        {
            RecordAutoFailure("no_skeleton");
        }
        return;
    }

    if (autoDue)
    {
        AttemptAuto();
        enabled[0] = m_settings[0].enabled;
        enabled[1] = m_settings[1].enabled;
        gunEnabled = m_gun.enabled;
    }
    const bool gunReferenced = (enabled[0] && (m_settings[0].space == TargetSpace::Gun ||
                                                  m_settings[0].space == TargetSpace::GunAnimated)) ||
        (enabled[1] && (m_settings[1].space == TargetSpace::Gun || m_settings[1].space == TargetSpace::GunAnimated));

    GunPlan gun;
    if (gunEnabled || gunReferenced || preview)
    {
        EvaluateGun(gunEnabled, gunReferenced, gun);
    }
    else
    {
        m_externalPublished = false;
    }

    Pending pending[2];
    for (u16 arm = 0; arm < 2; ++arm)
    {
        if (enabled[arm] || preview)
        {
            EvaluateArm(arm, enabled[arm], gun, pending[arm]);
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

    if (gun.active)
    {
        if (!gun.external && IsIdentityDelta(gun.delta) && !pending[0].apply && !pending[1].apply)
        {
            m_gunState.status = "animation_passthrough";
        }
        else
        {
            ApplyGun(gun);
        }
    }
    if (gun.valid && gun.external && !gun.active)
    {
        m_gunState.resolved.mul_43(m_skeleton->LL_GetTransform(m_externalAnchor), m_externalOffset);
    }
    if (gun.valid && !gun.external && gun.bone < m_skeleton->LL_BoneCount())
    {
        m_gunState.resolved = m_skeleton->LL_GetTransform(gun.bone);
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
    return BoneIsAncestorOrSelf(m_skeleton, bone, ancestor);
}

bool CHudIKController::ArmsOverlap(u16 a, u16 b) const
{
    return ChainsOverlap(m_skeleton, m_settings[a].bones, m_settings[b].bones);
}

void CHudIKController::EvaluateArm(u16 arm, bool enabled, const GunPlan& gun, Pending& pending)
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

    Fmatrix followDelta;
    followDelta.identity();
    Fmatrix reference;
    switch (settings.space)
    {
    case TargetSpace::Animated:
        reference.set(m_pose[b2]);
        break;
    case TargetSpace::Model:
        reference.identity();
        break;
    case TargetSpace::Gun:
        if (!gun.valid)
        {
            state.status = "gun_unavailable";
            return;
        }
        reference.set(gun.active ? gun.target : gun.raw);
        break;
    case TargetSpace::GunAnimated:
        if (gun.valid && gun.active)
        {
            followDelta = gun.delta;
        }
        reference.mul_43(followDelta, m_pose[b2]);
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
        if (gun.active && !gun.external && IsAncestorOrSelf(settings.targetBone, gun.bone))
        {
            if (settings.targetBone == gun.bone)
            {
                reference.set(gun.target);
            }
            else
            {
                reference.mul_43(gun.delta, m_pose[settings.targetBone]);
            }
        }
        else
        {
            reference.set(m_pose[settings.targetBone]);
        }
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

    if (settings.space == TargetSpace::GunAnimated &&
        (!gun.valid || !gun.active ||
            (IsIdentityDelta(followDelta) && IsNearZero(settings.position, ZeroPositionTolerance) &&
                IsNearZero(settings.rotation, ZeroRotationTolerance) &&
                IsNearZero(settings.elbowOffset, ZeroPositionTolerance))))
    {
        state.target = m_pose[b2];
        state.elbow = m_pose[b1].c;
        state.error = 0.f;
        state.status = m_gun.enabled && !gun.valid ? "gun_unavailable" : "animation_passthrough";
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
    if (settings.space == TargetSpace::GunAnimated)
    {
        Fvector hingeAxis;
        Fmatrix hingeBase;
        hingeBase.mul_43(m_pose[b0], calibration.bind[1]);
        hingeBase.transform_dir(hingeAxis, calibration.hingeFrame.j);
        state.elbow = TransportBendHint(start.c, m_pose[b1].c, m_pose[b2].c, state.target.c, hingeAxis,
            calibration.solver.Length(), settings.elbowOffset);
    }
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

void CHudIKController::ApplyGun(const GunPlan& plan)
{
    if (plan.external)
    {
        m_externalPose = plan.target;
        m_externalPublished = true;
        m_gunState.applied = true;
        return;
    }

    BoneOverride data;
    data.transform = plan.target;
    SavedCallback saved;
    CBoneInstance& instance = m_skeleton->LL_GetBoneInstance(plan.bone);
    saved.callback = instance.callback();
    saved.param = instance.callback_param();
    saved.overwrite = instance.callback_overwrite();
    saved.type = instance.callback_type();
    instance.set_callback(bctCustom, &CHudIKController::OverrideCallback, &data, TRUE);

    Fmatrix identity;
    identity.identity();
    const u16 parentId = m_skeleton->GetBoneData(plan.bone).GetParentID();
    Fmatrix* parent = parentId == BI_NONE ? &identity : &m_skeleton->LL_GetTransform(parentId);

    m_applying = true;
    m_skeleton->Bone_Calculate(&m_skeleton->LL_GetData(plan.bone), parent);
    m_applying = false;

    CBoneInstance& restored = m_skeleton->LL_GetBoneInstance(plan.bone);
    restored.set_callback(saved.type, saved.callback, saved.param, saved.overwrite);
    m_gunState.applied = true;
}

void CHudIKController::ClearGunState(pcstr status)
{
    m_gunState = GunState();
    m_gunState.status = status;
}

bool CHudIKController::IsArmBone(u16 bone) const
{
    const u16 count = m_skeleton->LL_BoneCount();
    for (u16 arm = 0; arm < 2; ++arm)
    {
        for (u16 j = 0; j < 3; ++j)
        {
            const u16 armBone = m_settings[arm].bones[j];
            if (armBone < count && IsAncestorOrSelf(armBone, bone))
            {
                return true;
            }
        }
    }
    return false;
}

u16 CHudIKController::SuggestGunBone() const
{
    if (!m_skeleton || m_externalGun)
    {
        return BI_NONE;
    }
    const u16 count = m_skeleton->LL_BoneCount();
    if (m_fireBone >= count || IsArmBone(m_fireBone))
    {
        return BI_NONE;
    }
    u16 candidate = m_fireBone;
    for (u16 steps = 0; steps < count; ++steps)
    {
        const u16 parent = m_skeleton->GetBoneData(candidate).GetParentID();
        if (parent == BI_NONE || parent >= count || IsArmBone(parent))
        {
            break;
        }
        candidate = parent;
    }
    return candidate;
}

bool CHudIKController::SupportsGunLead() const
{
    return m_skeleton && (m_gunLeadCapable || m_externalGun);
}

bool CHudIKController::HasGunBoneHint() const
{
    return m_skeleton && m_fireBone < m_skeleton->LL_BoneCount();
}

void CHudIKController::SetGunLeadCapable(bool capable)
{
    if (m_gunLeadCapable == capable)
    {
        return;
    }
    m_gunLeadCapable = capable;
    if (!m_skeleton)
    {
        return;
    }
    if (capable)
    {
        if (m_autoState == AutoState::Pending)
        {
            RefreshAuto();
        }
        return;
    }
    const bool hadGun = m_gun.enabled || m_gun.bone != BI_NONE;
    m_gun = GunSettings();
    m_externalPublished = false;
    m_gunCaptureStatus = "inactive";
    ClearGunState("disabled");
    RearmAuto();
    if (hadGun && !m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

u16 CHudIKController::ResolveGunBone() const
{
    if (m_externalGun)
    {
        return BI_NONE;
    }
    return m_gun.bone != BI_NONE ? m_gun.bone : SuggestGunBone();
}

bool CHudIKController::ValidateGun(u16 bone, pcstr& status) const
{
    if (!m_skeleton)
    {
        status = "no_skeleton";
        return false;
    }
    const u16 count = u16(m_pose.size());
    if (!SupportsGunLead())
    {
        status = "unsupported";
        return false;
    }
    if (bone >= count || bone >= m_skeleton->LL_BoneCount())
    {
        status = "missing_bone";
        return false;
    }
    const bool hasFire = m_fireBone != BI_NONE;
    const bool hasLight = hasFire && m_lightBone != BI_NONE;
    if ((hasFire && m_fireBone >= count) || (hasLight && m_lightBone >= count))
    {
        status = "missing_bone";
        return false;
    }
    if (IsArmBone(bone))
    {
        status = "gun_contains_arm";
        return false;
    }
    if (hasFire && !IsAncestorOrSelf(m_fireBone, bone))
    {
        status = "fire_bone_outside";
        return false;
    }
    if (hasLight && !IsAncestorOrSelf(m_lightBone, bone))
    {
        status = "light_bone_outside";
        return false;
    }
    if (!m_skeleton->LL_GetBoneVisible(bone) || (hasFire && !m_skeleton->LL_GetBoneVisible(m_fireBone)) ||
        (hasLight && !m_skeleton->LL_GetBoneVisible(m_lightBone)))
    {
        status = "hidden_bone";
        return false;
    }
    CBoneInstance& instance = m_skeleton->LL_GetBoneInstance(bone);
    if (instance.callback() || instance.callback_overwrite())
    {
        status = "bone_callback_conflict";
        return false;
    }
    if (!IsRigid(m_pose[bone]))
    {
        status = "non_rigid_bone";
        return false;
    }
    return true;
}

bool CHudIKController::ComputeExternalRaw(Fmatrix& raw, pcstr& status) const
{
    if (!m_skeleton)
    {
        status = "no_skeleton";
        return false;
    }
    if (!m_externalGun)
    {
        status = "unsupported";
        return false;
    }
    const u16 count = u16(m_pose.size());
    if (m_externalAnchor >= count || m_externalAnchor >= m_skeleton->LL_BoneCount())
    {
        status = "missing_bone";
        return false;
    }
    if (!m_skeleton->LL_GetBoneVisible(m_externalAnchor))
    {
        status = "hidden_bone";
        return false;
    }
    if (!IsRigid(m_pose[m_externalAnchor]))
    {
        status = "non_rigid_bone";
        return false;
    }
    if (!IsRigid(m_externalOffset))
    {
        status = "non_rigid_offset";
        return false;
    }
    Fmatrix result;
    result.mul_43(m_pose[m_externalAnchor], m_externalOffset);
    if (!IsRigid(result))
    {
        status = "non_rigid_bone";
        return false;
    }
    raw = result;
    return true;
}

bool CHudIKController::ComputeGunRaw(u16 bone, Fmatrix& raw, pcstr& status) const
{
    if (m_externalGun)
    {
        return ComputeExternalRaw(raw, status);
    }
    if (!ValidateGun(bone, status))
    {
        return false;
    }
    raw.set(m_pose[bone]);
    return true;
}

bool CHudIKController::BuildGunPlan(u16 bone, bool active, GunPlan& plan, pcstr& status, bool collision) const
{
    plan = GunPlan();
    status = "inactive";
    if (!_valid(m_gun.position) || !_valid(m_gun.rotation))
    {
        status = "invalid_settings";
        return false;
    }
    if (m_gun.space != TargetSpace::Animated && m_gun.space != TargetSpace::Model)
    {
        status = "invalid_space";
        return false;
    }

    Fmatrix raw;
    if (!ComputeGunRaw(bone, raw, status))
    {
        return false;
    }

    Fmatrix inverse;
    if (!inverse.invert_b(raw))
    {
        status = "singular_transform";
        return false;
    }

    Fvector radians = m_gun.rotation;
    radians.mul(PI / 180.f);
    Fmatrix offset;
    offset.setHPB(radians.x, radians.y, radians.z);
    offset.translate_over(m_gun.position);
    Fmatrix target;
    if (m_gun.space == TargetSpace::Animated)
    {
        target.mul_43(raw, offset);
    }
    else
    {
        target.set(offset);
    }
    if (collision && active)
    {
        target.c.add(m_collisionOffset);
    }
    if (!_valid(target) || !IsRigid(target))
    {
        status = "invalid_settings";
        return false;
    }

    Fmatrix delta;
    delta.mul_43(target, inverse);
    if (!_valid(delta))
    {
        status = "invalid_settings";
        return false;
    }

    plan.valid = true;
    plan.active = active;
    plan.external = m_externalGun;
    plan.bone = m_externalGun ? u16(BI_NONE) : bone;
    plan.raw = raw;
    plan.target = target;
    plan.delta = delta;
    return true;
}

void CHudIKController::EvaluateGun(bool enabled, bool referenced, GunPlan& plan)
{
    plan = GunPlan();
    m_externalPublished = false;
    u16 bone = BI_NONE;
    if (!m_externalGun)
    {
        bone = m_gun.bone;
        if (!enabled && bone == BI_NONE)
        {
            if (!referenced)
            {
                ClearGunState("disabled");
                return;
            }
            bone = ResolveGunBone();
        }
    }
    ClearGunState("inactive");
    m_gunState.frame = Device.dwFrame;

    pcstr status = "inactive";
    GunPlan built;
    if (!BuildGunPlan(bone, enabled, built, status, true))
    {
        m_gunState.status = status;
        return;
    }

    m_gunState.valid = true;
    m_gunState.animated = built.raw;
    m_gunState.target = built.target;
    m_gunState.resolved = built.target;
    m_gunState.delta = built.delta;
    if (enabled)
    {
        m_gunState.collision = m_collisionOffset;
    }
    m_gunState.status = enabled ? "ok" : "preview";
    plan = built;
}

const CHudIKController::GunSettings& CHudIKController::GetGun() const
{
    return m_gun;
}

const CHudIKController::GunState& CHudIKController::GetGunState() const
{
    return m_gunState;
}

void CHudIKController::SetGun(const GunSettings& settings)
{
    if (!m_skeleton || !SupportsGunLead())
    {
        return;
    }
    GunSettings next = settings;
    if (m_externalGun)
    {
        next.bone = BI_NONE;
    }
    if (SameGun(m_gun, next))
    {
        return;
    }
    const bool wasGunEnabled = m_gun.enabled;
    m_gun = next;
    if (!m_gun.enabled)
    {
        m_externalPublished = false;
        ClearCollisionOffset();
    }
    ClearGunState(m_gun.enabled ? "pending" : "disabled");
    if (m_gun.enabled)
    {
        EnsureCallback();
    }
    if ((m_autoState == AutoState::Pending && m_gun.enabled) || wasGunEnabled != m_gun.enabled)
    {
        CancelAuto();
    }
    else if (m_autoState == AutoState::Pending)
    {
        RefreshAuto();
    }
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

void CHudIKController::SetGunBones(u16 fireBone, u16 lightBone)
{
    if (!m_skeleton)
    {
        return;
    }
    const u16 count = m_skeleton->LL_BoneCount();
    const u16 fire = fireBone < count ? fireBone : BI_NONE;
    const u16 light = fire != BI_NONE && lightBone < count ? lightBone : BI_NONE;
    if (fire == m_fireBone && light == m_lightBone)
    {
        return;
    }
    m_fireBone = fire;
    m_lightBone = light;
    m_gun = GunSettings();
    ClearCollisionOffset();
    m_externalPublished = false;
    m_gunCaptureStatus = "inactive";
    ClearGunState("disabled");
    RearmAuto();
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

pcstr CHudIKController::GetGunCaptureStatus() const
{
    return m_gunCaptureStatus;
}

bool CHudIKController::CaptureGunTarget(TargetSpace space)
{
    if (!m_skeleton || (space != TargetSpace::Animated && space != TargetSpace::Model))
    {
        m_gunCaptureStatus = m_skeleton ? "invalid_space" : "no_skeleton";
        return false;
    }
    if (!IsPoseFresh())
    {
        RequestSnapshot();
        m_gunCaptureStatus = "stale_pose";
        return false;
    }
    const u16 bone = ResolveGunBone();
    pcstr status = "inactive";
    Fmatrix raw;
    if (!ComputeGunRaw(bone, raw, status))
    {
        m_gunCaptureStatus = status;
        return false;
    }

    Fvector position{};
    Fvector rotation{};
    if (space == TargetSpace::Model)
    {
        if (!ToDegrees(raw, position, rotation))
        {
            m_gunCaptureStatus = "invalid_settings";
            return false;
        }
    }

    GunSettings updated = m_gun;
    updated.bone = bone;
    updated.space = space;
    updated.position = position;
    updated.rotation = rotation;
    SetGun(updated);
    m_gunCaptureStatus = "ok";
    return true;
}

bool CHudIKController::ValidateArmChains(pcstr& status) const
{
    const u16 count = u16(m_pose.size());
    for (u16 arm = 0; arm < 2; ++arm)
    {
        const ArmSettings& settings = m_settings[arm];
        for (u16 j = 0; j < 3; ++j)
        {
            if (settings.bones[j] >= count || settings.bones[j] >= m_skeleton->LL_BoneCount())
            {
                status = "missing_bones";
                return false;
            }
            if (!m_skeleton->LL_GetBoneVisible(settings.bones[j]))
            {
                status = "hidden_bone";
                return false;
            }
            if (!IsRigid(m_pose[settings.bones[j]]))
            {
                status = "non_rigid_bone";
                return false;
            }
            CBoneInstance& instance = m_skeleton->LL_GetBoneInstance(settings.bones[j]);
            if (instance.callback() || instance.callback_overwrite())
            {
                status = "bone_callback_conflict";
                return false;
            }
        }
        const u16 b0 = settings.bones[0];
        const u16 b1 = settings.bones[1];
        const u16 wristId = settings.bones[2];
        if (b0 == b1 || b1 == wristId || b0 == wristId ||
            m_skeleton->GetBoneData(b1).GetParentID() != b0 ||
            m_skeleton->GetBoneData(wristId).GetParentID() != b1)
        {
            status = "not_direct_chain";
            return false;
        }
    }
    if (ArmsOverlap(0, 1))
    {
        status = "arm_overlap";
        return false;
    }
    return true;
}

void CHudIKController::CommitTwoHand(const ArmSettings (&arms)[2], const GunSettings& gun)
{
    for (u16 arm = 0; arm < 2; ++arm)
    {
        if (!m_settings[arm].enabled)
        {
            m_state[arm].solved = false;
            m_state[arm].status = "pending";
        }
        m_settings[arm] = arms[arm];
    }
    SetGun(gun);
    m_state[0].solved = false;
    m_state[1].solved = false;
    EnsureCallback();
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

bool CHudIKController::CaptureTwoHand()
{
    if (!m_skeleton)
    {
        m_gunCaptureStatus = "no_skeleton";
        return false;
    }
    if (!IsPoseFresh())
    {
        RequestSnapshot();
        m_gunCaptureStatus = "stale_pose";
        return false;
    }
    const bool external = m_externalGun;
    const u16 bone = external ? u16(BI_NONE) : ResolveGunBone();
    pcstr status = "inactive";
    Fmatrix reference;
    if (!ComputeGunRaw(bone, reference, status))
    {
        m_gunCaptureStatus = status;
        return false;
    }
    Fmatrix inverse;
    if (!inverse.invert_b(reference))
    {
        m_gunCaptureStatus = "singular_transform";
        return false;
    }
    if (!ValidateArmChains(status))
    {
        m_gunCaptureStatus = status;
        return false;
    }

    ArmSettings updated[2];
    for (u16 arm = 0; arm < 2; ++arm)
    {
        const ArmSettings& settings = m_settings[arm];
        Fmatrix offset;
        offset.mul_43(inverse, m_pose[settings.bones[2]]);
        Fvector position;
        Fvector rotation;
        if (!ToDegrees(offset, position, rotation))
        {
            m_gunCaptureStatus = "invalid_settings";
            return false;
        }
        updated[arm] = settings;
        updated[arm].enabled = true;
        updated[arm].space = external ? TargetSpace::Gun : TargetSpace::Bone;
        updated[arm].targetBone = external ? u16(BI_NONE) : bone;
        updated[arm].position = position;
        updated[arm].rotation = rotation;
        updated[arm].weight = 1.f;
    }

    GunSettings gun;
    gun.enabled = true;
    gun.bone = bone;
    gun.space = TargetSpace::Animated;
    CommitTwoHand(updated, gun);
    CancelAuto();
    m_gunCaptureStatus = "ok";
    return true;
}

bool CHudIKController::ActivateFromPose(pcstr& status)
{
    const u16 bone = m_externalGun ? u16(BI_NONE) : ResolveGunBone();
    GunPlan plan;
    if (!BuildGunPlan(bone, true, plan, status) || !ValidateArmChains(status))
    {
        return false;
    }

    ArmSettings updated[2];
    for (u16 arm = 0; arm < 2; ++arm)
    {
        updated[arm] = m_settings[arm];
        updated[arm].enabled = true;
        updated[arm].space = TargetSpace::GunAnimated;
        updated[arm].targetBone = BI_NONE;
        updated[arm].position.set(0.f, 0.f, 0.f);
        updated[arm].rotation.set(0.f, 0.f, 0.f);
        updated[arm].elbowOffset.set(0.f, 0.f, 0.f);
        updated[arm].weight = 1.f;
    }

    GunSettings gun = m_gun;
    gun.enabled = true;
    gun.bone = m_externalGun ? u16(BI_NONE) : bone;
    CommitTwoHand(updated, gun);
    return true;
}

bool CHudIKController::ActivateTwoHand()
{
    if (!m_skeleton)
    {
        m_gunCaptureStatus = "no_skeleton";
        return false;
    }
    if (!IsPoseFresh())
    {
        RequestSnapshot();
        m_gunCaptureStatus = "stale_pose";
        return false;
    }
    pcstr status = "inactive";
    if (!ActivateFromPose(status))
    {
        m_gunCaptureStatus = status;
        return false;
    }
    m_autoState = AutoState::Active;
    m_autoReady = false;
    m_autoStatus = "active";
    m_gunCaptureStatus = "ok";
    return true;
}

void CHudIKController::ReleaseTwoHand()
{
    if (!m_skeleton)
    {
        return;
    }
    CancelAuto();
    ClearCollisionOffset();
    for (u16 arm = 0; arm < 2; ++arm)
    {
        if (m_settings[arm].enabled)
        {
            m_settings[arm].enabled = false;
            ClearState(arm, "disabled");
        }
    }
    GunSettings gun = m_gun;
    gun.enabled = false;
    SetGun(gun);
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

void CHudIKController::TransformGunDirection(Fvector& direction) const
{
    if (m_externalGun || !m_gunState.valid || !m_gunState.applied || m_gunState.frame != Device.dwFrame)
    {
        return;
    }
    Fvector rotated;
    m_gunState.delta.transform_dir(rotated, direction);
    if (_valid(rotated) && rotated.square_magnitude() > EPS * EPS)
    {
        rotated.normalize();
        direction = rotated;
    }
}

void CHudIKController::SetCollisionOffset(const Fvector& offset)
{
    if (!m_skeleton || !_valid(offset))
    {
        m_collisionOffset = Fvector();
        return;
    }
    m_collisionOffset = offset;
}

void CHudIKController::ClearCollisionOffset()
{
    m_collisionOffset = Fvector();
}

const Fvector& CHudIKController::GetCollisionOffset() const
{
    return m_collisionOffset;
}

bool CHudIKController::GetRawBoneTransform(u16 bone, Fmatrix& transform) const
{
    if (!m_skeleton || !m_poseValid || m_poseFrame != Device.dwFrame || bone >= m_pose.size())
    {
        return false;
    }
    if (!_valid(m_pose[bone]))
    {
        return false;
    }
    transform = m_pose[bone];
    return true;
}

void CHudIKController::ResetExternal()
{
    m_externalGun = false;
    m_externalAnchor = BI_NONE;
    m_externalOffset.identity();
    m_externalPublished = false;
    m_externalPose.identity();
}

bool CHudIKController::HasExternalGun() const
{
    return m_skeleton && m_externalGun;
}

void CHudIKController::SetExternalGun(u16 anchorBone, const Fmatrix& attachOffset)
{
    if (!m_skeleton)
    {
        return;
    }
    if (anchorBone >= m_skeleton->LL_BoneCount() || !_valid(attachOffset))
    {
        ClearExternalGun();
        return;
    }
    if (m_externalGun && m_externalAnchor == anchorBone && SameMatrix(m_externalOffset, attachOffset))
    {
        return;
    }
    const bool wasExternal = m_externalGun;
    m_externalGun = true;
    m_externalAnchor = anchorBone;
    m_externalOffset = attachOffset;
    m_externalPublished = false;
    m_poseValid = false;
    ClearCollisionOffset();
    if (wasExternal)
    {
        ClearGunState(m_gun.enabled ? "pending" : "disabled");
        if (m_autoState == AutoState::Pending)
        {
            RefreshAuto();
        }
        if (!m_applying)
        {
            m_skeleton->CalculateBones_Invalidate();
        }
        return;
    }
    m_gun = GunSettings();
    m_gunCaptureStatus = "inactive";
    ClearGunState("disabled");
    RearmAuto();
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

void CHudIKController::ClearExternalGun()
{
    if (!m_externalGun)
    {
        return;
    }
    ResetExternal();
    m_poseValid = false;
    ClearCollisionOffset();
    if (!m_skeleton)
    {
        return;
    }
    m_gun = GunSettings();
    m_gunCaptureStatus = "inactive";
    ClearGunState("disabled");
    for (u16 arm = 0; arm < 2; ++arm)
    {
        if (m_settings[arm].space == TargetSpace::Gun || m_settings[arm].space == TargetSpace::GunAnimated)
        {
            ApplyDefaults(arm);
            m_calibration[arm].valid = false;
            ClearState(arm, "disabled");
        }
    }
    RearmAuto();
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

bool CHudIKController::GetExternalGunTransform(Fmatrix& pose) const
{
    if (!m_skeleton || !m_externalGun || !m_externalPublished || !_valid(m_externalPose))
    {
        return false;
    }
    pose = m_externalPose;
    return true;
}

pcstr CHudIKController::GetAutoStatus() const
{
    return m_autoStatus;
}

void CHudIKController::ArmAuto()
{
    if (!m_skeleton)
    {
        return;
    }
    m_autoState = AutoState::Pending;
    RefreshAuto();
}

void CHudIKController::RearmAuto()
{
    if (m_autoState == AutoState::Active)
    {
        m_autoState = AutoState::Pending;
    }
    if (m_autoState == AutoState::Pending)
    {
        RefreshAuto();
    }
}

void CHudIKController::CancelAuto()
{
    if (!m_skeleton)
    {
        return;
    }
    m_autoState = AutoState::Cancelled;
    m_autoReady = false;
    m_autoStatus = "cancelled";
}

void CHudIKController::RefreshAuto()
{
    m_autoReady = false;
    m_autoRetryFrame = 0;
    if (!m_skeleton || m_autoState != AutoState::Pending)
    {
        return;
    }
    pcstr status = "inactive";
    if (!AutoPrerequisites(status))
    {
        m_autoStatus = status;
        return;
    }
    m_autoReady = true;
    m_autoStatus = "pending";
    EnsureCallback();
    if (!m_applying)
    {
        m_skeleton->CalculateBones_Invalidate();
    }
}

bool CHudIKController::AutoPrerequisites(pcstr& status) const
{
    if (!SupportsGunLead())
    {
        status = "unsupported";
        return false;
    }
    const u16 count = m_skeleton->LL_BoneCount();
    const u16 bone = m_externalGun ? u16(BI_NONE) : ResolveGunBone();
    if (!m_externalGun && bone >= count)
    {
        status = "missing_bone";
        return false;
    }
    for (u16 arm = 0; arm < 2; ++arm)
    {
        for (u16 j = 0; j < 3; ++j)
        {
            if (m_settings[arm].bones[j] >= count)
            {
                status = "missing_bones";
                return false;
            }
        }
        if (!IsDirectChain(m_skeleton, m_settings[arm].bones))
        {
            status = "not_direct_chain";
            return false;
        }
    }
    if (ArmsOverlap(0, 1))
    {
        status = "arm_overlap";
        return false;
    }
    if (!m_externalGun && IsArmBone(bone))
    {
        status = "gun_contains_arm";
        return false;
    }
    return true;
}

bool CHudIKController::IsAutoDue() const
{
    return m_autoReady && m_autoState == AutoState::Pending && Device.dwFrame >= m_autoRetryFrame;
}

void CHudIKController::RecordAutoFailure(pcstr status)
{
    m_autoStatus = status;
    if (!IsTransientFailure(status))
    {
        m_autoReady = false;
        return;
    }
    m_autoRetryFrame = Device.dwFrame + AutoRetryFrames;
}

void CHudIKController::AttemptAuto()
{
    pcstr status = "inactive";
    if (!ActivateFromPose(status))
    {
        RecordAutoFailure(status);
        return;
    }
    m_autoState = AutoState::Active;
    m_autoReady = false;
    m_autoStatus = "active";
}
