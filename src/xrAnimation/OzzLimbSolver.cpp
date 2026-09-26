#include "stdafx.h"
#include "OzzLimbSolver.h"
#include "OzzMath.h"

#include "xrCore/Profiler/Profiler.h"
#include <ozz/animation/runtime/ik_two_bone_job.h>
#include <ozz/base/maths/math_constant.h>
#include <ozz/base/maths/simd_quaternion.h>
#include <algorithm>
#include <cmath>

namespace XRay::Animation
{
OzzLimbSolver::OzzLimbSolver()
{
    m_middleBind = Fidentity;
    m_endBind = Fidentity;
}

pcstr OzzLimbSolver::FailureName(Failure failure)
{
    switch (failure)
    {
    case Failure::None: return "none";
    case Failure::UninitializedBind: return "uninitialized_bind";
    case Failure::MiddleBindTransform: return "middle_bind_transform";
    case Failure::EndBindTransform: return "end_bind_transform";
    case Failure::BindLength: return "bind_length";
    case Failure::BindHinge: return "bind_hinge";
    case Failure::StartTransform: return "start_transform";
    case Failure::GoalTransform: return "goal_transform";
    case Failure::KneePosition: return "knee_position";
    case Failure::TargetDistance: return "target_distance";
    case Failure::TargetTooClose: return "target_too_close";
    case Failure::HingeReach: return "hinge_reach";
    case Failure::PoleDirection: return "pole_direction";
    case Failure::Job: return "job";
    case Failure::EndTransform: return "end_transform";
    case Failure::EndPosition: return "endpoint_error";
    case Failure::EndInverse: return "end_inverse";
    case Failure::StartRotation: return "start_rotation";
    case Failure::MiddleRotation: return "middle_rotation";
    case Failure::EndRotation: return "end_rotation";
    default: return "unknown";
    }
}

bool OzzLimbSolver::IsRigidTransform(const Fmatrix& matrix, Result& result, Failure failure)
{
    if (!_valid(matrix))
    {
        result = {failure, "non_finite", 1.f, 0.f};
        return false;
    }

    const float affineError = std::max({_abs(matrix._14), _abs(matrix._24), _abs(matrix._34),
        _abs(matrix._44 - 1.f)});
    if (affineError > EPS)
    {
        result = {failure, "affine_error", affineError, EPS};
        return false;
    }

    using namespace ozz::math;
    const Float4x4 transform = ImportOzzMatrix(matrix);
    if (!AreAllTrue3(IsNormalizedEst(transform)))
    {
        const float error = std::max({_abs(matrix.i.square_magnitude() - 1.f),
            _abs(matrix.j.square_magnitude() - 1.f), _abs(matrix.k.square_magnitude() - 1.f)});
        result = {failure, "axis_length_squared_error", error, kNormalizationToleranceEstSq};
        return false;
    }
    if (!AreAllTrue1(IsOrthogonal(transform)))
    {
        const SimdFloat4 normal = NormalizeSafe3(Cross3(transform.cols[0], transform.cols[1]),
            simd_float4::zero());
        const SimdFloat4 axis = NormalizeSafe3(transform.cols[2], simd_float4::zero());
        const float error = _abs(GetX(Dot3(normal, axis)) - 1.f);
        result = {failure, "orthogonality_error", error, kNormalizationToleranceSq};
        return false;
    }
    return true;
}

bool OzzLimbSolver::ProjectPole(Fvector& pole, const Fvector& direction)
{
    pole.mad(direction, -pole.dotproduct(direction));
    const float lengthSquared = pole.square_magnitude();
    if (!_valid(lengthSquared) || lengthSquared <= EPS * EPS)
    {
        return false;
    }

    pole.mul(1.f / _sqrt(lengthSquared));
    return true;
}

bool OzzLimbSolver::Initialize(const Fmatrix& middleBind, const Fmatrix& endBind)
{
    m_bindResult = {};
    if (!IsRigidTransform(middleBind, m_bindResult, Failure::MiddleBindTransform) ||
        !IsRigidTransform(endBind, m_bindResult, Failure::EndBindTransform))
    {
        return false;
    }

    m_upperLength = middleBind.c.magnitude();
    m_lowerLength = endBind.c.magnitude();
    if (m_upperLength <= EPS || m_lowerLength <= EPS)
    {
        m_bindResult = {Failure::BindLength, "min_bone_length",
            std::min(m_upperLength, m_lowerLength), EPS};
        return false;
    }

    Fvector upper;
    upper.set(middleBind.i.dotproduct(middleBind.c), middleBind.j.dotproduct(middleBind.c),
        middleBind.k.dotproduct(middleBind.c));
    const float cosine = upper.x * endBind.c.x + upper.z * endBind.c.z;
    const float sine = upper.z * endBind.c.x - upper.x * endBind.c.z;
    m_bendPhase = std::atan2(sine, cosine);
    m_bendScale = 2.f * _sqrt(cosine * cosine + sine * sine);
    m_bendBias = m_upperLength * m_upperLength + m_lowerLength * m_lowerLength +
        2.f * upper.y * endBind.c.y;
    if (!_valid(m_bendScale) || m_bendScale <= EPS * EPS)
    {
        m_bindResult = {Failure::BindHinge, "hinge_scale", m_bendScale, EPS * EPS};
        return false;
    }
    if (!_valid(m_bendBias))
    {
        m_bindResult = {Failure::BindHinge, "non_finite_hinge_bias", 1.f, 0.f};
        return false;
    }

    m_middleBind = middleBind;
    m_endBind = endBind;
    return true;
}

OzzLimbSolver::Result OzzLimbSolver::Solve(const Fmatrix& start, const Fmatrix& goal, const Fvector& knee,
    Fmatrix (&rotations)[3]) const
{
    ZoneScopedN("Animation::OzzIK");

    if (m_bindResult.failure != Failure::None)
    {
        return m_bindResult;
    }
    Result result;
    if (!IsRigidTransform(start, result, Failure::StartTransform) ||
        !IsRigidTransform(goal, result, Failure::GoalTransform))
    {
        return result;
    }
    if (!_valid(knee))
    {
        return {Failure::KneePosition, "non_finite", 1.f, 0.f};
    }

    Fvector direction;
    direction.sub(goal.c, start.c);
    const float distanceSquared = direction.square_magnitude();
    const float minimumDistance = _abs(m_upperLength - m_lowerLength);
    if (!_valid(distanceSquared) || distanceSquared <= EPS * EPS)
    {
        return {Failure::TargetDistance, "target_distance_squared", distanceSquared, EPS * EPS};
    }
    if (distanceSquared < minimumDistance * minimumDistance)
    {
        return {Failure::TargetTooClose, "target_distance_squared", distanceSquared,
            minimumDistance * minimumDistance};
    }

    const float distance = _sqrt(distanceSquared);
    direction.mul(1.f / distance);
    const float maximumDistance = (m_upperLength + m_lowerLength) * 0.9999f;
    const float targetDistance = std::min(distance, maximumDistance);
    const float bendCosine = (targetDistance * targetDistance - m_bendBias) / m_bendScale;
    if (!_valid(bendCosine) || bendCosine < -1.f - EPS || bendCosine > 1.f + EPS)
    {
        return {Failure::HingeReach, "abs_bend_cosine", _abs(bendCosine), 1.f + EPS};
    }
    const float bendAngle = std::acos(std::clamp(bendCosine, -1.f, 1.f));
    Fvector target;
    target.mad(start.c, direction, targetDistance);

    Fvector pole;
    pole.sub(knee, start.c);
    if (!ProjectPole(pole, direction))
    {
        pole = start.i;
        if (!ProjectPole(pole, direction))
        {
            pole = start.k;
            if (!ProjectPole(pole, direction))
            {
                pole = start.j;
                if (!ProjectPole(pole, direction))
                {
                    return {Failure::PoleDirection};
                }
            }
        }
    }

    using namespace ozz::math;
    const Float4x4 startModel = ImportOzzMatrix(start);
    const Float4x4 middleLocal = ImportOzzMatrix(m_middleBind);
    const Float4x4 endLocal = ImportOzzMatrix(m_endBind);
    const Float4x4 middleModel = startModel * middleLocal;
    const Float4x4 endModel = middleModel * endLocal;

    SimdQuaternion startCorrection;
    SimdQuaternion middleCorrection;
    ozz::animation::IKTwoBoneJob job;
    job.start_joint = &startModel;
    job.mid_joint = &middleModel;
    job.end_joint = &endModel;
    job.target = simd_float4::Load3PtrU(&target.x);
    job.pole_vector = simd_float4::Load3PtrU(&pole.x);
    job.mid_axis = simd_float4::Load(0.f, m_bendPhase + bendAngle < 0.f ? 1.f : -1.f, 0.f, 0.f);
    job.weight = 1.f;
    job.soften = 1.f;
    job.start_joint_correction = &startCorrection;
    job.mid_joint_correction = &middleCorrection;
    if (!job.Run())
    {
        return {Failure::Job};
    }

    const float startRotationLengthSquared = GetX(Length4Sqr(startCorrection.xyzw));
    if (!_valid(startRotationLengthSquared) || startRotationLengthSquared <= EPS * EPS)
    {
        return {Failure::StartRotation, "quaternion_length_squared", startRotationLengthSquared, EPS * EPS};
    }
    const float middleRotationLengthSquared = GetX(Length4Sqr(middleCorrection.xyzw));
    if (!_valid(middleRotationLengthSquared) || middleRotationLengthSquared <= EPS * EPS)
    {
        return {Failure::MiddleRotation, "quaternion_length_squared", middleRotationLengthSquared, EPS * EPS};
    }
    const Float4x4 startRotation = Float4x4::FromQuaternion(Normalize(startCorrection).xyzw);
    const Float4x4 middleRotation = Float4x4::FromQuaternion(Normalize(middleCorrection).xyzw);
    const Float4x4 solvedEnd = startModel * startRotation * middleLocal * middleRotation * endLocal;
    Fmatrix endTransform;
    ExportOzzMatrix(solvedEnd, endTransform);
    const float tolerance = std::max(EPS_L, (m_upperLength + m_lowerLength) * EPS_L);
    if (!IsRigidTransform(endTransform, result, Failure::EndTransform))
    {
        return result;
    }
    const float endpointErrorSquared = endTransform.c.distance_to_sqr(target);
    if (endpointErrorSquared > tolerance * tolerance)
    {
        return {Failure::EndPosition, "endpoint_error_m", _sqrt(endpointErrorSquared), tolerance};
    }

    SimdInt4 invertible;
    const Float4x4 inverseEnd = Invert(solvedEnd, &invertible);
    if (!AreAllTrue1(invertible))
    {
        return {Failure::EndInverse};
    }

    Float4x4 desiredEnd = ImportOzzMatrix(goal);
    desiredEnd.cols[3] = solvedEnd.cols[3];
    Float4x4 endRotation = inverseEnd * desiredEnd;
    endRotation.cols[3] = simd_float4::w_axis();
    ExportOzzMatrix(startRotation, rotations[0]);
    ExportOzzMatrix(middleRotation, rotations[1]);
    ExportOzzMatrix(endRotation, rotations[2]);
    if (!IsRigidTransform(rotations[0], result, Failure::StartRotation) ||
        !IsRigidTransform(rotations[1], result, Failure::MiddleRotation) ||
        !IsRigidTransform(rotations[2], result, Failure::EndRotation))
    {
        return result;
    }
    return {};
}
}
