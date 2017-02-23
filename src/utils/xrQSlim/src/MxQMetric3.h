#ifndef MXQMETRIC3_INCLUDED // -*- C++ -*-
#define MXQMETRIC3_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  3D Quadric Error Metric

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxQMetric3.h,v 1.14 1999/12/15 18:07:45 garland Exp $

 ************************************************************************/

#include "MxMat3.h"
#include "MxMat4.h"

class MxQuadric3
{
private:
    double a2, ab, ac, ad;
    double b2, bc, bd;
    double c2, cd;
    double d2;

    double r;

    void init(double a, double b, double c, double d, double area);
    void init(const Mat4& Q, double area);

public:
    MxQuadric3() { clear(); }
    MxQuadric3(double a, double b, double c, double d, double area = 1.0) { init(a, b, c, d, area); }
    MxQuadric3(const float* n, double d, double area = 1.0) { init(n[0], n[1], n[2], d, area); }
    MxQuadric3(const double* n, double d, double area = 1.0) { init(n[0], n[1], n[2], d, area); }
    MxQuadric3(const MxQuadric3& Q) { *this = Q; }
    Mat3 tensor() const;
    Vec3 vector() const { return Vec3(ad, bd, cd); }
    double offset() const { return d2; }
    double area() const { return r; }
    Mat4 homogeneous() const;

    void set_coefficients(const double*);
    void set_area(double a) { r = a; }
    void point_constraint(const float*);

    void clear(double val = 0.0) { a2 = ab = ac = ad = b2 = bc = bd = c2 = cd = d2 = r = val; }
    MxQuadric3& operator=(const MxQuadric3& Q);
    MxQuadric3& operator+=(const MxQuadric3& Q);
    MxQuadric3& operator-=(const MxQuadric3& Q);
    MxQuadric3& operator*=(double s);
    MxQuadric3& transform(const Mat4& P);

    double evaluate(double x, double y, double z) const;
    double evaluate(const double* v) const { return evaluate(v[0], v[1], v[2]); }
    double evaluate(const float* v) const { return evaluate(v[0], v[1], v[2]); }
    double operator()(double x, double y, double z) const { return evaluate(x, y, z); }
    double operator()(const double* v) const { return evaluate(v[0], v[1], v[2]); }
    double operator()(const float* v) const { return evaluate(v[0], v[1], v[2]); }
    bool optimize(Vec3& v) const;
    bool optimize(float* x, float* y, float* z) const;

    bool optimize(Vec3& v, const Vec3& v1, const Vec3& v2) const;
    bool optimize(Vec3& v, const Vec3& v1, const Vec3& v2, const Vec3& v3) const;

    /*
        ostream& write(ostream& out)
        {
            return out << a2 << " " << ab << " " << ac << " " << ad << " "
                   << b2 << " " << bc << " " << bd << " " << c2 << " "
                   << cd << " " << d2 << " " << r;
        }

        ostream& write_full(ostream& out)
        {
            return out << a2 << " " << ab << " " << ac << " " << ad << " "
                   << ab << " " << b2 << " " << bc << " " << bd << " "
                   << ac << " " << bc << " " << c2 << " " << cd << " "
                   << ad << " " << bd << " " << cd << " " << d2;
        }


        istream& read(istream& in)
        {
            return in >> a2 >> ab >> ac >> ad >> b2
                  >> bc >> bd >> c2 >> cd >> d2 >> r;
        }


        istream& read_full(istream& in)
        {
            return in >> a2 >> ab >> ac >> ad
                  >> ab >> b2 >> bc >> bd
                  >> ac >> bc >> c2 >> cd
                  >> ad >> bd >> cd >> d2;
        }
    */
};

////////////////////////////////////////////////////////////////////////
//
// Quadric visualization routines
//

#define MX_RED_ELLIPSOIDS 0x1
#define MX_GREEN_ELLIPSOIDS 0x2
#define MX_CHARCOAL_ELLIPSOIDS 0x3

extern void mx_quadric_shading(int c = MX_GREEN_ELLIPSOIDS, bool twosided = true);
extern void mx_draw_quadric(const MxQuadric3& Q, double r, const float* v = NULL);
extern void mx_draw_osculant(float k1, float k2, float extent = 1.0);

// MXQMETRIC3_INCLUDED
#endif
