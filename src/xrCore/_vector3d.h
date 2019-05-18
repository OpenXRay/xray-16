#pragma once
#ifndef __V3D__
#define __V3D__
#include <algorithm>
#include <cmath>
#include "xrCommon/inlining_macros.h"
#include "_types.h"
#include "_random.h"
#include "math_constants.h"

template <class T>
struct _vector3
{
    using TYPE = T;
    using Self = _vector3<T>;
    using SelfRef = Self&;
    using SelfCRef = const Self&;

    T x, y, z;

    // access operators
    ICF T& operator[](int i) { return *((T*)this + i); }
    ICF T& operator[](int i) const { return *((T*)this + i); }

    ICF SelfRef set(T _x, T _y, T _z) noexcept
    {
        x = _x;
        y = _y;
        z = _z;
        return *this;
    }

    ICF SelfRef set(const _vector3<float>& v) noexcept
    {
        x = T(v.x);
        y = T(v.y);
        z = T(v.z);
        return *this;
    }

    ICF SelfRef set(const _vector3<double>& v) noexcept
    {
        x = T(v.x);
        y = T(v.y);
        z = T(v.z);
        return *this;
    }

    ICF SelfRef set(float* p) noexcept
    {
        x = p[0];
        y = p[1];
        z = p[2];
        return *this;
    }

    ICF SelfRef set(double* p) noexcept
    {
        x = p[0];
        y = p[1];
        z = p[2];
        return *this;
    }

    // XXX: The vast majority of these basic math operations can be expressed as non-class functions.
    ICF SelfRef add(const Self& v) noexcept
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    ICF SelfRef add(T s) noexcept
    {
        x += s;
        y += s;
        z += s;
        return *this;
    }

    ICF SelfRef add(const Self& a, const Self& v) noexcept
    {
        x = a.x + v.x;
        y = a.y + v.y;
        z = a.z + v.z;
        return *this;
    }

    ICF SelfRef add(const Self& a, T s) noexcept
    {
        x = a.x + s;
        y = a.y + s;
        z = a.z + s;
        return *this;
    }

    ICF SelfRef sub(const Self& v) noexcept
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    ICF SelfRef sub(T s) noexcept
    {
        x -= s;
        y -= s;
        z -= s;
        return *this;
    }

    ICF SelfRef sub(const Self& a, const Self& v) noexcept
    {
        x = a.x - v.x;
        y = a.y - v.y;
        z = a.z - v.z;
        return *this;
    }

    ICF SelfRef sub(const Self& a, T s) noexcept
    {
        x = a.x - s;
        y = a.y - s;
        z = a.z - s;
        return *this;
    }

    ICF SelfRef mul(const Self& v) noexcept
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }

    ICF SelfRef mul(T s) noexcept
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    ICF SelfRef mul(const Self& a, const Self& v) noexcept
    {
        x = a.x * v.x;
        y = a.y * v.y;
        z = a.z * v.z;
        return *this;
    }

    ICF SelfRef mul(const Self& a, T s) noexcept
    {
        x = a.x * s;
        y = a.y * s;
        z = a.z * s;
        return *this;
    }

    ICF SelfRef div(const Self& v) noexcept
    {
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }

    ICF SelfRef div(T s) noexcept
    {
        x /= s;
        y /= s;
        z /= s;
        return *this;
    }

    ICF SelfRef div(const Self& a, const Self& v) noexcept
    {
        x = a.x / v.x;
        y = a.y / v.y;
        z = a.z / v.z;
        return *this;
    }

    ICF SelfRef div(const Self& a, T s) noexcept
    {
        x = a.x / s;
        y = a.y / s;
        z = a.z / s;
        return *this;
    }

    SelfRef invert()
    {
        x = -x;
        y = -y;
        z = -z;
        return *this;
    }

    SelfRef invert(const Self& a)
    {
        x = -a.x;
        y = -a.y;
        z = -a.z;
        return *this;
    }

    SelfRef min(const Self& v1, const Self& v2)
    {
        x = std::min(v1.x, v2.x);
        y = std::min(v1.y, v2.y);
        z = std::min(v1.z, v2.z);
        return *this;
    }

    SelfRef min(const Self& v)
    {
        x = std::min(x, v.x);
        y = std::min(y, v.y);
        z = std::min(z, v.z);
        return *this;
    }

    SelfRef max(const Self& v1, const Self& v2)
    {
        x = std::max(v1.x, v2.x);
        y = std::max(v1.y, v2.y);
        z = std::max(v1.z, v2.z);
        return *this;
    }

    SelfRef max(const Self& v)
    {
        x = std::max(x, v.x);
        y = std::max(y, v.y);
        z = std::max(z, v.z);
        return *this;
    }

    SelfRef abs(const Self& v)
    {
        x = _abs(v.x);
        y = _abs(v.y);
        z = _abs(v.z);
        return *this;
    }

    ICF bool similar(const Self& v, T E = EPS_L) const
    {
        return _abs(x - v.x) < E && _abs(y - v.y) < E && _abs(z - v.z) < E;
    };

    SelfRef set_length(T l);

    // Align vector3 by axis (!y)
    SelfRef align();


    // Squeeze
    SelfRef squeeze(T Epsilon);


    // Clamp vector3
    SelfRef clamp(const Self& min, const Self& max);

    SelfRef clamp(const Self& _v);

    // Interpolate vectors (inertion)
    SelfRef inertion(const Self& p, T v);
    SelfRef average(const Self& p);
    SelfRef average(const Self& p1, const Self& p2);
    SelfRef lerp(const Self& p1, const Self& p2, T t);

    // Direct vector3 from point P by dir D with length M
    SelfRef mad(const Self& d, T m);
    SelfRef mad(const Self& p, const Self& d, T m);
    SelfRef mad(const Self& d, const Self& s);
    SelfRef mad(const Self& p, const Self& d, const Self& s);

    // SQ magnitude
    T square_magnitude() const;

    // magnitude
    T magnitude() const;
    // Normalize
    T normalize_magn();

    SelfRef normalize();

    // Safe-Normalize
    SelfRef normalize_safe();

    // Normalize
    SelfRef normalize(const Self& v);

    // Safe-Normalize
    SelfRef normalize_safe(const Self& v);

    SelfRef random_dir(CRandom& R = ::Random);
    SelfRef random_dir(const Self& ConeAxis, float ConeAngle, CRandom& R = ::Random);

    SelfRef random_point(const Self& BoxSize, CRandom& R = ::Random);
    SelfRef random_point(T r, CRandom& R = ::Random);

    // DotProduct
    ICF T dotproduct(const Self& v) const // v1*v2
    {
        return x * v.x + y * v.y + z * v.z;
    }

    // CrossProduct
    SelfRef crossproduct(const Self& v1, const Self& v2); // (v1,v2) -> this

    // Distance calculation
    T distance_to_xz(const Self& v) const;
    T distance_to_xz_sqr(const Self& v) const;
    // Distance calculation
    T distance_to_sqr(const Self& v) const;

    // Distance calculation
    T distance_to(const Self& v) const;
    // Barycentric coords
    SelfRef from_bary(const Self& V1, const Self& V2, const Self& V3, T u, T v, T w);
    SelfRef from_bary(const Self& V1, const Self& V2, const Self& V3, const Self& B);

    SelfRef from_bary4(const Self& V1, const Self& V2, const Self& V3, const Self& V4, T u, T v, T w, T t);

    SelfRef mknormal_non_normalized(const Self& p0, const Self& p1, const Self& p2);
    SelfRef mknormal(const Self& p0, const Self& p1, const Self& p2);

    SelfRef setHP(T h, T p);
    void getHP(T& h, T& p) const;
    float getH() const;
    float getP() const;

    SelfRef reflect(const Self& dir, const Self& norm);
    SelfRef slide(const Self& dir, const Self& norm);

    static void generate_orthonormal_basis(const _vector3<T>& dir, _vector3<T>& up, _vector3<T>& right);
    static void generate_orthonormal_basis_normalized(_vector3<T>& dir, _vector3<T>& up, _vector3<T>& right);
};

using Fvector = _vector3<float>;
using Fvector3 = _vector3<float>;

using Dvector = _vector3<double>;
using Dvector3 = _vector3<double>;

using Ivector = _vector3<s32>;
using Ivector3 = _vector3<s32>;

template <class T>
bool _valid(const _vector3<T>& v)
{
    return _valid((T)v.x) && _valid((T)v.y) && _valid((T)v.z);
}

//////////////////////////////////////////////////////////////////////////
#pragma warning(push)
#pragma warning(disable : 4244)
double rsqrt(double v);
bool exact_normalize(float* a);
bool exact_normalize(Fvector3& a);
#pragma warning(pop)

#endif
