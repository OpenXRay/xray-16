#pragma once

#include "xrAnimation.h"
#include "xrCore/_matrix.h"

namespace XRay::Animation
{
class XRANIMATION_API OzzLimbSolver
{
public:
    enum class Failure : u32
    {
        None,
        UninitializedBind,
        MiddleBindTransform,
        EndBindTransform,
        BindLength,
        BindHinge,
        StartTransform,
        GoalTransform,
        KneePosition,
        TargetDistance,
        TargetTooClose,
        HingeReach,
        PoleDirection,
        Job,
        EndTransform,
        EndPosition,
        EndInverse,
        StartRotation,
        MiddleRotation,
        EndRotation,
        Count
    };

    class Result
    {
    public:
        Failure failure = Failure::None;
        pcstr metric = "none";
        float value = 0.f;
        float limit = 0.f;
    };

    OzzLimbSolver();

    bool Initialize(const Fmatrix& middleBind, const Fmatrix& endBind);
    Result Solve(const Fmatrix& start, const Fmatrix& goal, const Fvector& knee,
        Fmatrix (&rotations)[3]) const;
    static pcstr FailureName(Failure failure);

private:
    static bool IsRigidTransform(const Fmatrix& matrix, Result& result, Failure failure);
    static bool ProjectPole(Fvector& pole, const Fvector& direction);

    Fmatrix m_middleBind;
    Fmatrix m_endBind;
    float m_upperLength = 0.f;
    float m_lowerLength = 0.f;
    float m_bendPhase = 0.f;
    float m_bendScale = 0.f;
    float m_bendBias = 0.f;
    Result m_bindResult = {Failure::UninitializedBind};
};
}
