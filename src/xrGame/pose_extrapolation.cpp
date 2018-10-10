#include "StdAfx.h"

#include "pose_extrapolation.h"

using namespace extrapolation;

static u32 extrapolate_pose_update_mseconds = 50;
void points::update(const Fmatrix& m)
{
    if (Device.dwTimeGlobal - last_update < extrapolate_pose_update_mseconds)
        return;
    m_points.push_back(point().set(pose().set(m), Device.fTimeGlobal));
    last_update = Device.dwTimeGlobal;
}

void points::init(const Fmatrix& m)
{
    m_points.fill_in(point().set(pose().set(m), Device.fTimeGlobal));
    last_update = Device.dwTimeGlobal;
}

static void spline_coefs_linar(const point& p0, const point& p1, pose& a0, pose& a1)
{
    a1 = p0.pose();
    if (p1.time() - p0.time() > EPS_S)
        a1.invert().add(p1.pose()).mul(1.f / (p1.time() - p0.time())); // ( y0^(-1) * y1 ) * ( t1 - t0 )
    else
        a1.identity();

    a0 = a1;
    a0.mul(p1.time()).invert().add(p0.pose()); // (a1*t1)^1 * y0
}

static void spline_linear(const point& p0, const point& p1, float t, pose& p)
{
    pose a0, a1;
    spline_coefs_linar(p0, p1, a0, a1);

    p = a0;
    p.add(a1.mul(t));
}

void points::extrapolate(Fmatrix& m, float time) const
{
    pose p;
    spline_linear(m_points[0], m_points[1], time, p);
    p.get(m);
}

pose::pose() : p(Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX)), r(Fquaternion().set(0, 0, 0, 0)) {}
pose& pose::set(const Fmatrix& m)
{
    p.set(m.c);
    r.set(m);
    return *this;
}
Fmatrix& pose::get(Fmatrix& m) const
{
    m.rotation(r);
    m.translate_over(p);
    return m;
}
pose& pose::mul(float v)
{
    Fvector axis;
    float angle;
    r.get_axis_angle(axis, angle);
    r.rotation(axis, angle * v);

    p.mul(v);
    return *this;
}

pose& pose::add(const pose& pose_)
{
    // Fmatrix m0,m1;
    // set( Fmatrix().mul_43( get( m0 ), p.get( m1 ) ) );
    p.add(pose_.p);
    r = Fquaternion().mul(r, pose_.r);
    return *this;
}

pose& pose::invert()
{
    r.inverse();
    p.invert();
    return *this;
}
pose& pose::identity()
{
    r.identity();
    p.set(0, 0, 0);
    return *this;
}
