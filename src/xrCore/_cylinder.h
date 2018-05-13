#pragma once
#include "_vector3d.h"

template <class T>
class _cylinder
{
public:
    using TYPE = T;
    using Self = _cylinder<T>;
    using VEC_TYPE = _vector3<T>;
    using SelfRef = Self&;
    using SelfCRef = const Self&;

public:
	VEC_TYPE m_center;
	VEC_TYPE m_direction;
    T m_height;
    T m_radius;

public:
    IC SelfRef invalidate()
    {
        m_center.set(0, 0, 0);
        m_direction.set(0, 0, 0);
        m_height = 0;
        m_radius = 0;
        return *this;
    }

    enum ecode
    {
        cyl_cap,
        cyl_wall,
        cyl_none
    };

    int intersect(const VEC_TYPE& start, const VEC_TYPE& dir, T afT[2], ecode code[2]) const;

    enum ERP_Result
    {
        rpNone = 0,
        rpOriginInside = 1,
        rpOriginOutside = 2,
        fcv_forcedword = u32(-1)
    };

    ERP_Result intersect(const VEC_TYPE& start, const VEC_TYPE& dir, T& dist) const;
};

using Fcylinder = _cylinder<float>;
using Dcylinder = _cylinder<double>;

template <class T>
bool _valid(const _cylinder<T>& c)
{
    return _valid(c.m_center) && _valid(c.m_direction) && _valid(c.m_height) && _valid(c.m_radius);
}
