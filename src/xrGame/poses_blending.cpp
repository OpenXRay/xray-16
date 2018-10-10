#include "StdAfx.h"
#include "poses_blending.h"
#include "xrCore/_quaternion.h"

poses_interpolation::poses_interpolation(const Fmatrix& m0, const Fmatrix& m1)
    : p0(m0.c), p1(m1.c), q0(Fquaternion().set(m0)), q1(Fquaternion().set(m1))
{
}

void poses_interpolation::pose(Fmatrix& p, float factor) const
{
    p.rotation(Fquaternion().slerp(q0, q1, factor));
    p.c.lerp(p0, p1, factor);
}

poses_blending::poses_blending(const Fmatrix& m0, const Fmatrix& m1, float target_time_)
    : interpolation(m0, m1), target_time(target_time_)
{
}

void poses_blending::pose(Fmatrix& p, float time) const
{
    VERIFY(target_time > EPS_S);
    VERIFY(time >= 0.f);
    VERIFY(time < target_time);

    interpolation.pose(p, time / target_time);
}
