#pragma once

#include "_vector3d.h"

struct XRCORE_API Fcylinder
{
    Fvector3 m_center;
	Fvector3 m_direction;
    float m_height;
    float m_radius;

    auto& invalidate()
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

    int intersect(const Fvector3& start, const Fvector3& dir, float afT[2], ecode code[2]) const;

    enum ERP_Result : u32
    {
        rpNone = 0,
        rpOriginInside = 1,
        rpOriginOutside = 2,
    };

    ERP_Result intersect(const Fvector3& start, const Fvector3& dir, float& dist) const;
};

inline bool _valid(const Fcylinder& c)
{
    return _valid(c.m_center) && _valid(c.m_direction) && _valid(c.m_height) && _valid(c.m_radius);
}
