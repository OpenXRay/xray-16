#pragma once
#ifndef __V2D__
#define __V2D__
#include "xrCommon/inlining_macros.h"
#include "xrCore/math_constants.h"


#ifdef min
# undef min
#endif
#ifdef max
# undef max
#endif


template <class T>
struct _vector2
{
public:
    typedef T TYPE;
    typedef _vector2<T> Self;
    typedef Self& SelfRef;
    typedef const Self& SelfCRef;

public:
    T x, y;

    IC SelfRef set(float _u, float _v)
    {
        x = T(_u);
        y = T(_v);
        return *this;
    }
    IC SelfRef set(double _u, double _v)
    {
        x = T(_u);
        y = T(_v);
        return *this;
    }
    IC SelfRef set(int _u, int _v)
    {
        x = T(_u);
        y = T(_v);
        return *this;
    }
    IC SelfRef set(const Self& p)
    {
        x = p.x;
        y = p.y;
        return *this;
    }
    IC SelfRef abs(const Self& p)
    {
        x = _abs(p.x);
        y = _abs(p.y);
        return *this;
    }
    IC SelfRef min(const Self& p)
    {
        x = std::min(x, p.x);
        y = std::min(y, p.y);
        return *this;
    }
    IC SelfRef min(T _x, T _y)
    {
        x = std::min(x, _x);
        y = std::min(y, _y);
        return *this;
    }
    IC SelfRef max(const Self& p)
    {
        x = std::max(x, p.x);
        y = std::max(y, p.y);
        return *this;
    }
    IC SelfRef max(T _x, T _y)
    {
        x = std::max(x, _x);
        y = std::max(y, _y);
        return *this;
    }
    IC SelfRef sub(const T p)
    {
        x -= p;
        y -= p;
        return *this;
    }
    IC SelfRef sub(const Self& p)
    {
        x -= p.x;
        y -= p.y;
        return *this;
    }
    IC SelfRef sub(const Self& p1, const Self& p2)
    {
        x = p1.x - p2.x;
        y = p1.y - p2.y;
        return *this;
    }
    IC SelfRef sub(const Self& p, float d)
    {
        x = p.x - d;
        y = p.y - d;
        return *this;
    }
    IC SelfRef add(const T p)
    {
        x += p;
        y += p;
        return *this;
    }
    IC SelfRef add(const Self& p)
    {
        x += p.x;
        y += p.y;
        return *this;
    }
    IC SelfRef add(const Self& p1, const Self& p2)
    {
        x = p1.x + p2.x;
        y = p1.y + p2.y;
        return *this;
    }
    IC SelfRef add(const Self& p, float d)
    {
        x = p.x + d;
        y = p.y + d;
        return *this;
    }
    IC SelfRef mul(const T s)
    {
        x *= s;
        y *= s;
        return *this;
    }
    IC SelfRef mul(const Self& p)
    {
        x *= p.x;
        y *= p.y;
        return *this;
    }
    IC SelfRef div(const T s)
    {
        x /= s;
        y /= s;
        return *this;
    }
    IC SelfRef div(const Self& p)
    {
        x /= p.x;
        y /= p.y;
        return *this;
    }
    IC SelfRef rot90(void)
    {
        float t = -x;
        x = y;
        y = t;
        return *this;
    }
    IC SelfRef cross(const Self& D)
    {
        x = D.y;
        y = -D.x;
        return *this;
    }
    IC T dot(Self& p) { return x * p.x + y * p.y; }
    IC T dot(const Self& p) const { return x * p.x + y * p.y; }
    IC SelfRef norm(void)
    {
        float m = _sqrt(x * x + y * y);
        x /= m;
        y /= m;
        return *this;
    }
    IC SelfRef norm_safe(void)
    {
        float m = _sqrt(x * x + y * y);
        if (m)
        {
            x /= m;
            y /= m;
        }
        return *this;
    }
    IC T distance_to(const Self& p) const { return _sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y)); }
    IC T square_magnitude(void) const { return x * x + y * y; }
    IC T magnitude(void) const { return _sqrt(square_magnitude()); }
    IC SelfRef mad(const Self& p, const Self& d, T r)
    {
        x = p.x + d.x * r;
        y = p.y + d.y * r;
        return *this;
    }
    IC Self Cross()
    {
        // vector3 orthogonal to (x,y) is (y,-x)
        Self kCross;
        kCross.x = y;
        kCross.y = -x;
        return kCross;
    }

    IC bool similar(Self& p, T eu, T ev) const { return _abs(x - p.x) < eu && _abs(y - p.y) < ev; }
    IC bool similar(const Self& p, float E = EPS_L) const { return _abs(x - p.x) < E && _abs(y - p.y) < E; };
    // average arithmetic
    IC SelfRef averageA(Self& p1, Self& p2)
    {
        x = (p1.x + p2.x) * .5f;
        y = (p1.y + p2.y) * .5f;
        return *this;
    }
    // average geometric
    IC SelfRef averageG(Self& p1, Self& p2)
    {
        x = _sqrt(p1.x * p2.x);
        y = _sqrt(p1.y * p2.y);
        return *this;
    }

    T& operator[](int i) const
    {
        // assert: 0 <= i < 2; x and y are packed into 2*sizeof(float) bytes
        return (T&)*(&x + i);
    }

    IC SelfRef normalize(void) { return norm(); }
    IC SelfRef normalize_safe(void) { return norm_safe(); }
    IC SelfRef normalize(const Self& v)
    {
        float m = _sqrt(v.x * v.x + v.y * v.y);
        x = v.x / m;
        y = v.y / m;
        return *this;
    }
    IC SelfRef normalize_safe(const Self& v)
    {
        float m = _sqrt(v.x * v.x + v.y * v.y);
        if (m)
        {
            x = v.x / m;
            y = v.y / m;
        }
        return *this;
    }
    IC float dotproduct(const Self& p) const { return dot(p); }
    IC float crossproduct(const Self& p) const { return y * p.x - x * p.y; }
    IC float getH(void) const
    {
        if (fis_zero(y))
            if (fis_zero(x))
                return (0.f);
            else
                return ((x > 0.0f) ? -PI_DIV_2 : PI_DIV_2);
        else if (y < 0.f)
            return (-(atanf(x / y) - PI));
        else
            return (-atanf(x / y));
    }
};

typedef _vector2<float> Fvector2;
typedef _vector2<double> Dvector2;
typedef _vector2<int> Ivector2;

template <class T>
bool _valid(const _vector2<T>& v)
{ return _valid((T)v.x) && _valid((T)v.y); }

#endif
