#ifndef GFXVEC4_INCLUDED // -*- C++ -*-
#define GFXVEC4_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  4D Vector class

  $Id: vec4.h,v 1.19 2002/09/20 16:02:25 garland Exp $

 ************************************************************************/

#include "vec3.h"

template <class T>
class TVec4
{
private:
    T elt[4];

public:
    // Standard constructors
    //
    TVec4(T s = 0) { *this = s; }
    TVec4(T x, T y, T z, T w)
    {
        elt[0] = x;
        elt[1] = y;
        elt[2] = z;
        elt[3] = w;
    }

    // Copy constructors & assignment operators
    template <class U>
    TVec4(const TVec4<U>& v)
    {
        *this = v;
    }
    template <class U>
    TVec4(const TVec3<U>& v, T w)
    {
        elt[0] = v[0];
        elt[1] = v[1];
        elt[2] = v[2];
        elt[3] = w;
    }
    template <class U>
    TVec4(const U v[4])
    {
        elt[0] = v[0];
        elt[1] = v[1];
        elt[2] = v[2];
        elt[3] = v[3];
    }
    template <class U>
    TVec4& operator=(const TVec4<U>& v)
    {
        elt[0] = v[0];
        elt[1] = v[1];
        elt[2] = v[2];
        elt[3] = v[3];
        return *this;
    }
    TVec4& operator=(T s)
    {
        elt[0] = elt[1] = elt[2] = elt[3] = s;
        return *this;
    }

    // Descriptive interface
    //
    typedef T value_type;
    static int dim() { return 4; }
    // Access methods
    //
    operator T*() { return elt; }
    operator const T*() const { return elt; }
    // Assignment and in-place arithmetic methods
    //
    inline TVec4& operator+=(const TVec4& v);
    inline TVec4& operator-=(const TVec4& v);
    inline TVec4& operator*=(T s);
    inline TVec4& operator/=(T s);
};

////////////////////////////////////////////////////////////////////////
//
// Method definitions
//

template <class T>
inline TVec4<T>& TVec4<T>::operator+=(const TVec4<T>& v)
{
    elt[0] += v[0];
    elt[1] += v[1];
    elt[2] += v[2];
    elt[3] += v[3];
    return *this;
}

template <class T>
inline TVec4<T>& TVec4<T>::operator-=(const TVec4<T>& v)
{
    elt[0] -= v[0];
    elt[1] -= v[1];
    elt[2] -= v[2];
    elt[3] -= v[3];
    return *this;
}

template <class T>
inline TVec4<T>& TVec4<T>::operator*=(T s)
{
    elt[0] *= s;
    elt[1] *= s;
    elt[2] *= s;
    elt[3] *= s;
    return *this;
}

template <class T>
inline TVec4<T>& TVec4<T>::operator/=(T s)
{
    elt[0] /= s;
    elt[1] /= s;
    elt[2] /= s;
    elt[3] /= s;
    return *this;
}

////////////////////////////////////////////////////////////////////////
//
// Operator definitions
//

template <class T>
inline TVec4<T> operator+(const TVec4<T>& u, const TVec4<T>& v)
{
    return TVec4<T>(u[0] + v[0], u[1] + v[1], u[2] + v[2], u[3] + v[3]);
}

template <class T>
inline TVec4<T> operator-(const TVec4<T>& u, const TVec4<T>& v)
{
    return TVec4<T>(u[0] - v[0], u[1] - v[1], u[2] - v[2], u[3] - v[3]);
}

template <class T>
inline TVec4<T> operator-(const TVec4<T>& u)
{
    return TVec4<T>(-u[0], -u[1], -u[2], -u[3]);
}

#if _MSC_VER >= 1200
// Normally, we use the <class T, class N> construct below to allow the scalar
// argument to be different than the template type.  This, for example, allows
// the user to write things like v/2.  Unfortunately, Microsoft VC6.0 (aka
// v1200) gets confused by this.  We used to include explicit versions for the
// case of int's, but this was causing silent (and incorrect) coercion of
// floats to ints.
//
template <class T>
inline TVec4<T> operator*(T s, const TVec4<T>& v)
{
    return TVec4<T>(v[0] * s, v[1] * s, v[2] * s, v[3] * s);
}
template <class T>
inline TVec4<T> operator*(const TVec4<T>& v, T s)
{
    return s * v;
}

template <class T>
inline TVec4<T> operator/(const TVec4<T>& v, T s)
{
    return TVec4<T>(v[0] / s, v[1] / s, v[2] / s, v[3] / s);
}
#else
template <class T, class N>
inline TVec4<T> operator*(N s, const TVec4<T>& v)
{
    return TVec4<T>(v[0] * s, v[1] * s, v[2] * s, v[3] * s);
}
template <class T, class N>
inline TVec4<T> operator*(const TVec4<T>& v, N s)
{
    return s * v;
}

template <class T, class N>
inline TVec4<T> operator/(const TVec4<T>& v, N s)
{
    return TVec4<T>(v[0] / s, v[1] / s, v[2] / s, v[3] / s);
}
#endif

template <class T>
inline T operator*(const TVec4<T>& u, const TVec4<T>& v)
{
    return u[0] * v[0] + u[1] * v[1] + u[2] * v[2] + u[3] * v[3];
}
/*
template<class T>
inline std::ostream &operator<<(std::ostream &out, const TVec4<T>& v)
    { return out <<v[0] <<" " <<v[1] <<" " <<v[2] <<" " <<v[3]; }

template<class T>
inline std::istream &operator>>(std::istream &in, TVec4<T>& v)
    { return in >> v[0] >> v[1] >> v[2] >> v[3]; }
*/
////////////////////////////////////////////////////////////////////////
//
// Misc. function definitions
//

template <class T>
inline TVec4<T> cross(const TVec4<T>& a, const TVec4<T>& b, const TVec4<T>& c)
{
    // Code adapted from VecLib4d.c in Graphics Gems V

    T d1 = (b[2] * c[3]) - (b[3] * c[2]);
    T d2 = (b[1] * c[3]) - (b[3] * c[1]);
    T d3 = (b[1] * c[2]) - (b[2] * c[1]);
    T d4 = (b[0] * c[3]) - (b[3] * c[0]);
    T d5 = (b[0] * c[2]) - (b[2] * c[0]);
    T d6 = (b[0] * c[1]) - (b[1] * c[0]);

    return TVec4<T>(-a[1] * d1 + a[2] * d2 - a[3] * d3, a[0] * d1 - a[2] * d4 + a[3] * d5,
        -a[0] * d2 + a[1] * d4 - a[3] * d6, a[0] * d3 - a[1] * d5 + a[2] * d6);
}

template <class T>
inline T norm2(const TVec4<T>& v)
{
    return v * v;
}
template <class T>
inline T norm(const TVec4<T>& v)
{
    return _sqrt(norm2(v));
}

template <class T>
inline void unitize(TVec4<T>& v)
{
    T l = norm2(v);
    if (l != 1.0 && l != 0.0)
        v /= _sqrt(l);
}

template <class T>
inline TVec3<T> proj(const TVec4<T>& v)
{
    TVec3<T> u(v[0], v[1], v[2]);
    if (v[3] != 1.0 && v[3] != 0.0)
        u /= v[3];
    return u;
}

typedef TVec4<double> Vec4;
typedef TVec4<float> Vec4f;

// GFXVEC4_INCLUDED
#endif
