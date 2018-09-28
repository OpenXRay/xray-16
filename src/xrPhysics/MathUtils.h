#pragma once
#ifndef MATH_UTILS_H
#define MATH_UTILS_H
#include "xrCore/_fbox.h"
#include "xrCore/_obb.h"
#include "xrPhysics.h"
#include "xrCore/_std_extensions.h"
#ifdef DEBUG
#include "xrCore/dump_string.h"
#endif
constexpr float phInfinity = std::numeric_limits<float>::infinity();

template <class T> struct _quaternion;
typedef _quaternion<float> Fquaternion;

IC float* cast_fp(Fvector& fv) { return (float*)(&fv); }
IC const float* cast_fp(const Fvector& fv) { return (const float*)(&fv); }
IC Fvector& cast_fv(float* fp) { return *((Fvector*)fp); }
IC const Fvector& cast_fv(const float* fp) { return *((const Fvector*)fp); }
IC float dXZMag(const float* v) { return _sqrt(v[0] * v[0] + v[2] * v[2]); }
IC float dXZMag(const Fvector& v) { return dXZMag(cast_fp(v)); }
IC float dXZDot(const float* v0, const float* v1) { return v0[0] * v1[0] + v0[2] * v1[2]; }
IC float dXZDotNormalized(const Fvector& v0, const Fvector& v1)
{
    return (v0.x * v1.x + v0.z * v1.z) / _sqrt((v0.x * v0.x + v0.z * v0.z) * (v1.x * v1.x + v1.z * v1.z));
}
IC float dXZDotNormalized(const float* v0, const float* v1) { return dXZDotNormalized(cast_fv(v0), cast_fv(v1)); }
IC float dXZDot(const Fvector& v0, const Fvector& v1) { return v0.x * v1.x + v0.z * v1.z; }
IC void dVectorSet(float* vd, const float* vs)
{
    vd[0] = vs[0];
    vd[1] = vs[1];
    vd[2] = vs[2];
}

IC void dVectorSetInvert(float* vd, const float* vs)
{
    vd[0] = -vs[0];
    vd[1] = -vs[1];
    vd[2] = -vs[2];
}

IC void dVectorSetZero(float* vd)
{
    vd[0] = 0.f;
    vd[1] = 0.f;
    vd[2] = 0.f;
}

IC void dVector4Set(float* vd, const float* vs)
{
    vd[0] = vs[0];
    vd[1] = vs[1];
    vd[2] = vs[2];
    vd[3] = vs[3];
}

IC void dVector4SetZero(float* vd)
{
    vd[0] = 0.f;
    vd[1] = 0.f;
    vd[2] = 0.f;
    vd[3] = 0.f;
}

IC void dQuaternionSet(float* vd, const float* vs) { dVector4Set(vd, vs); }
IC void dVectorAdd(float* v, const float* av)
{
    v[0] += av[0];
    v[1] += av[1];
    v[2] += av[2];
}
IC void dVectorAddMul(float* v, const float* ad, float mul)
{
    v[0] += ad[0] * mul;
    v[1] += ad[1] * mul;
    v[2] += ad[2] * mul;
}
IC void dVectorAdd(float* v, const float* av0, const float* av1)
{
    v[0] = av0[0] + av1[0];
    v[1] = av0[1] + av1[1];
    v[2] = av0[2] + av1[2];
}

IC void dVectorSub(float* v, const float* av)
{
    v[0] -= av[0];
    v[1] -= av[1];
    v[2] -= av[2];
}

IC void dVectorSub(float* v, const float* av1, const float* av0)
{
    v[0] = av1[0] - av0[0];
    v[1] = av1[1] - av0[1];
    v[2] = av1[2] - av0[2];
}
IC void dVectorInvert(float* v)
{
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
}
IC void dVectorMul(float* v, float mt)
{
    v[0] *= mt;
    v[1] *= mt;
    v[2] *= mt;
}
IC void dVectorMul(float* res, const float* av, float mt)
{
    res[0] = av[0] * mt;
    res[1] = av[1] * mt;
    res[2] = av[2] * mt;
}
IC void dVectorInterpolate(float* v, float* to, float k) // changes to
{
    dVectorMul(v, 1.f - k);
    dVectorMul(to, k);
    dVectorAdd(v, to);
}

IC void dVectorDeviation(const float* vector3_from, const float* vector3_to, float* vector_dev)
{
    dVectorSub(vector_dev, vector3_to, vector3_from);
}

IC void dVectorDeviationAdd(const float* vector3_from, const float* vector3_to, float* vector_dev)
{
    vector_dev[0] += vector3_from[0] - vector3_to[0];
    vector_dev[1] += vector3_from[1] - vector3_to[1];
    vector_dev[2] += vector3_from[2] - vector3_to[2];
}

IC void dMatrixSmallDeviation(const float* matrix33_from, const float* matrix33_to, float* vector_dev)
{
    vector_dev[0] = matrix33_from[10] - matrix33_to[10];
    vector_dev[1] = matrix33_from[2] - matrix33_to[2];
    vector_dev[2] = matrix33_from[4] - matrix33_to[4];
}

IC void dMatrixSmallDeviationAdd(const float* matrix33_from, const float* matrix33_to, float* vector_dev)
{
    vector_dev[0] += matrix33_from[10] - matrix33_to[10];
    vector_dev[1] += matrix33_from[2] - matrix33_to[2];
    vector_dev[2] += matrix33_from[4] - matrix33_to[4];
}

// XXX: Not used, but move to xrMiscMath
// void twoq_2w(const Fquaternion& q1, const Fquaternion& q2, float dt, Fvector& w) noexcept;

IC float to_mag_and_dir(const Fvector& in_v, Fvector& out_v)
{
    float mag = in_v.magnitude();
    if (!fis_zero(mag))
        out_v.mul(in_v, 1.f / mag);
    else
        out_v.set(0.f, 0.f, 0.f);
    return mag;
}

IC float to_mag_and_dir(Fvector& in_out_v) { return to_mag_and_dir(in_out_v, in_out_v); }
IC void to_vector(Fvector& v, float mag, const Fvector dir) { v.mul(dir, mag); }
IC void prg_pos_on_axis(const Fvector& in_ax_p, const Fvector& in_ax_d, Fvector& in_out_pos)
{
    in_out_pos.sub(in_ax_p);
    float ax_mag = in_ax_d.magnitude();
    float prg = in_out_pos.dotproduct(in_ax_d) / ax_mag;
    in_out_pos.set(in_ax_d);
    in_out_pos.mul(prg / ax_mag);
    in_out_pos.add(in_ax_p);
}
IC float prg_pos_on_plane(const Fvector& in_norm, float d, const Fvector& in_pos, Fvector& out_pos)
{
    float prg = d - in_pos.dotproduct(in_norm);
    Fvector diff;
    diff.set(in_norm);
    diff.mul(prg);
    out_pos.add(in_pos, diff);
    return prg;
}

IC void prg_on_normal(const Fvector& in_norm, const Fvector& in_dir, Fvector& out_dir)
{
    float prg = -in_dir.dotproduct(in_norm);
    Fvector diff;
    diff.set(in_norm);
    diff.mul(prg);
    out_dir.add(in_dir, diff);
    return;
}

IC void restrict_vector_in_dir(Fvector& V, const Fvector& dir)
{
    Fvector sub;
    sub.set(dir);
    float dotpr = dir.dotproduct(V);
    if (dotpr > 0.f)
    {
        sub.mul(dotpr);
        V.sub(sub);
    }
}
IC bool check_obb_sise(const Fobb& obb)
{
    return (
        !fis_zero(obb.m_halfsize.x, EPS_L) || !fis_zero(obb.m_halfsize.y, EPS_L) || !fis_zero(obb.m_halfsize.z, EPS_L));
}

IC float fsignum(float val) { return val < 0.f ? -1.f : 1.f; }
IC void save_max(float& max, float val)
{
    if (val > max)
        max = val;
}
IC void save_min(float& min, float val)
{
    if (val < min)
        min = val;
}

IC void limit_above(float& val, float limit)
{
    if (val > limit)
        val = limit;
}

IC void limit_below(float& val, float limit)
{
    if (val < limit)
        val = limit;
}

IC void TransferenceToThrowVel(Fvector& in_transference_out_vel, float time, float gravity_accel)
{
    in_transference_out_vel.mul(1.f / time);
    in_transference_out_vel.y += time * gravity_accel / 2.f;
}
IC float ThrowMinVelTime(const Fvector& transference, float gravity_accel)
{
    return _sqrt(2.f * transference.magnitude() / gravity_accel);
}
// returns num result, tgA result tangents of throw angle
IC u8 TransferenceAndThrowVelToTgA(
    const Fvector& transference, float throw_vel, float gravity_accel, Fvector2& tgA, float& s)
{
    float sqx = transference.x * transference.x + transference.z * transference.z;
    float sqv = throw_vel * throw_vel;
    float sqD4 = 1.f - gravity_accel / (sqv * sqv) * (2.f * transference.y * sqv + gravity_accel * sqx);
    if (sqD4 < 0.f)
        return 0;
    s = _sqrt(sqx);
    float mlt = sqv / (gravity_accel * s);
    if (sqD4 == 0.f)
    {
        tgA.x = tgA.y = mlt;
        return 1;
    }
    float D4 = _sqrt(sqD4);
    tgA.x = mlt * (1.f - D4);
    tgA.y = mlt * (1.f + D4);
    return 2;
}
IC u8 TransferenceAndThrowVelToTgA(const Fvector& transference, float throw_vel, float gravity_accel, Fvector2& tgA)
{
    float s;
    return TransferenceAndThrowVelToTgA(transference, throw_vel, gravity_accel, tgA, s);
}
IC u8 TransferenceAndThrowVelToThrowDir(
    const Fvector& transference, float throw_vel, float gravity_accel, Fvector throw_dir[2])
{
    throw_dir[0] = throw_dir[1] = transference;
    Fvector2 tgA;
    float s;
    u8 ret = TransferenceAndThrowVelToTgA(transference, throw_vel, gravity_accel, tgA, s);
    switch (ret)
    {
    case 0: return 0; break;
    case 2: throw_dir[1].y = tgA.y * s; throw_dir[1].normalize();
    case 1:
        throw_dir[0].y = tgA.x * s;
        throw_dir[0].normalize();
        break;
    default: NODEFAULT;
    }
    return ret;
}
#define MAX_OF(x, on_x, y, on_y, z, on_z) \
    if (x > y)                            \
    {                                     \
        if (x > z)                        \
        {                                 \
            on_x;                         \
        }                                 \
        else                              \
        {                                 \
            on_z;                         \
        }                                 \
    }                                     \
    else                                  \
    \
{                                  \
        if (y > z)                        \
        {                                 \
            on_y;                         \
        }                                 \
        else                              \
        {                                 \
            on_z;                         \
        }                                 \
    \
}

#define MIN_OF(x, on_x, y, on_y, z, on_z) \
    if (x < y)                            \
    {                                     \
        if (x < z)                        \
        {                                 \
            on_x;                         \
        }                                 \
        else                              \
        {                                 \
            on_z;                         \
        }                                 \
    }                                     \
    else                                  \
    \
{                                  \
        if (y < z)                        \
        {                                 \
            on_y;                         \
        }                                 \
        else                              \
        {                                 \
            on_z;                         \
        }                                 \
    \
}

#define NON_MIN_OF(x, on_x1, on_x2, y, on_y1, on_y2, z, on_z1, on_z2) \
    if (x < y)                                                        \
    {                                                                 \
        if (x < z)                                                    \
        {                                                             \
            if (y < z)                                                \
            {                                                         \
                on_z1;                                                \
                on_y2;                                                \
            }                                                         \
            else                                                      \
            {                                                         \
                on_z2;                                                \
                on_y1;                                                \
            }                                                         \
        }                                                             \
        else                                                          \
        {                                                             \
            on_x2;                                                    \
            on_y1;                                                    \
        }                                                             \
    }                                                                 \
    else                                                              \
    \
{                                                              \
        if (y < z)                                                    \
        {                                                             \
            if (x > z)                                                \
            {                                                         \
                on_x1;                                                \
                on_z2;                                                \
            }                                                         \
            else                                                      \
            {                                                         \
                on_z1;                                                \
                on_x2;                                                \
            }                                                         \
        }                                                             \
        else                                                          \
        {                                                             \
            on_x1;                                                    \
            on_y2;                                                    \
        }                                                             \
    \
}

#define SORT(x, on_x1, on_x2, on_x3, y, on_y1, on_y2, on_y3, z, on_z1, on_z2, on_z3) \
    if (x < y)                                                                       \
    {                                                                                \
        if (x < z)                                                                   \
        {                                                                            \
            if (y < z)                                                               \
            {                                                                        \
                on_z1;                                                               \
                on_y2;                                                               \
                on_x3;                                                               \
            }                                                                        \
            else                                                                     \
            {                                                                        \
                on_z2;                                                               \
                on_y1;                                                               \
                on_x3;                                                               \
            }                                                                        \
        }                                                                            \
        else                                                                         \
        {                                                                            \
            on_x2;                                                                   \
            on_y1;                                                                   \
            on_z3;                                                                   \
        }                                                                            \
    }                                                                                \
    else                                                                             \
    \
{                                                                             \
        if (y < z)                                                                   \
        {                                                                            \
            if (x > z)                                                               \
            {                                                                        \
                on_x1;                                                               \
                on_z2;                                                               \
                on_y3;                                                               \
            }                                                                        \
            else                                                                     \
            {                                                                        \
                on_z1;                                                               \
                on_x2;                                                               \
                on_y3;                                                               \
            }                                                                        \
        }                                                                            \
        else                                                                         \
        {                                                                            \
            on_x1;                                                                   \
            on_y2;                                                                   \
            on_z3;                                                                   \
        }                                                                            \
    \
}
//////////////////////////////////////////////////////////////////////////////////////

struct SInertVal
{
    float val;
    const float inertion;
    SInertVal(float inert) : inertion(inert) { R_ASSERT(inert > 0.f && inert < 1.f); }
    IC void new_val(float new_val) { val = inertion * val + (1 - inertion) * new_val; }
private:
    SInertVal& operator=(SInertVal& v) { R_ASSERT(false); }
};

IC float DET(const Fmatrix& a)
{
    return ((a._11 * (a._22 * a._33 - a._23 * a._32) - a._12 * (a._21 * a._33 - a._23 * a._31) +
        a._13 * (a._21 * a._32 - a._22 * a._31)));
}

IC bool valid_pos(const Fvector& P, const Fbox& B)
{
    Fbox BB = B;
    BB.grow(100000);
    return !!BB.contains(P);
}

#ifdef DEBUG
const float DET_CHECK_EPS = 0.15f; // scale -35%  !? ;)
const float DET_CHECK_FATAL_EPS = 0.8f; // scale -35%  !? ;)
#define VERIFY_RMATRIX(M)                                  \
    {                                                      \
        float d = DET(M);                                  \
        if (!fsimilar(d, 1.f, DET_CHECK_EPS))              \
        {                                                  \
            Msg("! matrix: %s ", get_string(M).c_str());   \
            Msg("! determinant: %f ", d);                  \
            Msg("! Is not valid rotational matrix");       \
            VERIFY(fsimilar(d, 1.f, DET_CHECK_FATAL_EPS)); \
        }                                                  \
    };
#else
#define VERIFY_RMATRIX(M)
#endif

#endif // include guard
