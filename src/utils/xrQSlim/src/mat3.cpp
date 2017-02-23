/************************************************************************

  3x3 Matrix class

  $Id: mat3.cxx,v 1.3 2001/11/19 16:08:30 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "mat3.h"

Mat3 Mat3::I() { return Mat3(Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1)); }
Mat3& Mat3::diag(double d)
{
    *this = 0.0;
    row[0][0] = row[1][1] = row[2][2] = d;
    return *this;
}

Mat3 diag(const Vec3& v) { return Mat3(Vec3(v[0], 0, 0), Vec3(0, v[1], 0), Vec3(0, 0, v[2])); }
Mat3 Mat3::outer_product(const Vec3& v)
{
    Mat3 A;
    double x = v[0], y = v[1], z = v[2];

    A(0, 0) = x * x;
    A(0, 1) = x * y;
    A(0, 2) = x * z;
    A(1, 0) = A(0, 1);
    A(1, 1) = y * y;
    A(1, 2) = y * z;
    A(2, 0) = A(0, 2);
    A(2, 1) = A(1, 2);
    A(2, 2) = z * z;

    return A;
}

Mat3 Mat3::outer_product(const Vec3& u, const Vec3& v)
{
    Mat3 A;

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            A(i, j) = u[i] * v[j];

    return A;
}

Mat3 operator*(const Mat3& n, const Mat3& m)
{
    Mat3 A;

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            A(i, j) = n[i] * m.col(j);

    return A;
}

Mat3 adjoint(const Mat3& m) { return Mat3(m[1] ^ m[2], m[2] ^ m[0], m[0] ^ m[1]); }
double invert(Mat3& inv, const Mat3& m)
{
    Mat3 A = adjoint(m);
    double d = A[0] * m[0];

    if (d == 0.0)
        return 0.0;

    inv = transpose(A) / d;
    return d;
}
