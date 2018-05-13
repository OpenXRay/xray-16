/************************************************************************

  Jacobi iteration on 3x3 matrices.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  NOTE: This code was adapted from VTK source code (vtkMath.cxx) which
        seems to have been adapted directly from Numerical Recipes in C.
    See Hoppe's Recon software for an alternative adaptation of the
    Numerical Recipes code.

  $Id: MxMat3-jacobi.cxx,v 1.11 2000/12/14 17:48:24 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxMat3.h"
#include "xrCommon/math_funcs_inline.h"

#define ROT(a, i, j, k, l)\
    g = a[i][j];\
    h = a[k][l];\
    a[i][j] = g - s * (h + g * tau);\
    a[k][l] = h + s * (g - h * tau);

#define MAX_ROTATIONS 60

// Description:
// Jacobi iteration for the solution of eigenvectors/eigenvalues of a 3x3
// real symmetric matrix. Square 3x3 matrix a; output eigenvalues in w;
// and output eigenvectors in v. Resulting eigenvalues/vectors are sorted
// in decreasing order; eigenvectors are normalized.
//
static bool internal_jacobi(double a[3][3], double w[3], double v[3][3])
{
    int i, j, k, iq, ip;
    double tresh, theta, tau, t, sm, s, h, g, c;
    double b[3], z[3], tmp;

    // initialize
    for (ip = 0; ip < 3; ip++)
    {
        for (iq = 0; iq < 3; iq++)
            v[ip][iq] = 0.0;
        v[ip][ip] = 1.0;
    }
    for (ip = 0; ip < 3; ip++)
    {
        b[ip] = w[ip] = a[ip][ip];
        z[ip] = 0.0;
    }

    // begin rotation sequence
    for (i = 0; i < MAX_ROTATIONS; i++)
    {
        sm = 0.0;
        for (ip = 0; ip < 2; ip++)
        {
            for (iq = ip + 1; iq < 3; iq++)
                sm += _abs(a[ip][iq]);
        }
        if (sm == 0.0)
            break;

        if (i < 4)
            tresh = 0.2 * sm / (9);
        else
            tresh = 0.0;

        for (ip = 0; ip < 2; ip++)
        {
            for (iq = ip + 1; iq < 3; iq++)
            {
                g = 100.0 * _abs(a[ip][iq]);
                if (i > 4 && (_abs(w[ip]) + g) == _abs(w[ip]) && (_abs(w[iq]) + g) == _abs(w[iq]))
                {
                    a[ip][iq] = 0.0;
                }
                else if (_abs(a[ip][iq]) > tresh)
                {
                    h = w[iq] - w[ip];
                    if ((_abs(h) + g) == _abs(h))
                        t = (a[ip][iq]) / h;
                    else
                    {
                        theta = 0.5 * h / (a[ip][iq]);
                        t = 1.0 / (_abs(theta) + _sqrt(1.0 + theta * theta));
                        if (theta < 0.0)
                            t = -t;
                    }
                    c = 1.0 / _sqrt(1 + t * t);
                    s = t * c;
                    tau = s / (1.0 + c);
                    h = t * a[ip][iq];
                    z[ip] -= h;
                    z[iq] += h;
                    w[ip] -= h;
                    w[iq] += h;
                    a[ip][iq] = 0.0;
                    for (j = 0; j < ip - 1; j++)
                    {
                        ROT(a, j, ip, j, iq)
                    }
                    for (j = ip + 1; j < iq - 1; j++)
                    {
                        ROT(a, ip, j, j, iq)
                    }
                    for (j = iq + 1; j < 3; j++)
                    {
                        ROT(a, ip, j, iq, j)
                    }
                    for (j = 0; j < 3; j++)
                    {
                        ROT(v, j, ip, j, iq)
                    }
                }
            }
        }

        for (ip = 0; ip < 3; ip++)
        {
            b[ip] += z[ip];
            w[ip] = b[ip];
            z[ip] = 0.0;
        }
    }

    if (i >= MAX_ROTATIONS)
    {
        FATAL("Error computing eigenvalues.");
        return false;
    }

    // sort eigenfunctions
    for (j = 0; j < 3; j++)
    {
        k = j;
        tmp = w[k];
        for (i = j; i < 3; i++)
        {
            if (w[i] >= tmp)
            {
                k = i;
                tmp = w[k];
            }
        }
        if (k != j)
        {
            w[k] = w[j];
            w[j] = tmp;
            for (i = 0; i < 3; i++)
            {
                tmp = v[i][j];
                v[i][j] = v[i][k];
                v[i][k] = tmp;
            }
        }
    }
    // insure eigenvector consistency (i.e., Jacobi can compute vectors that
    // are negative of one another (.707,.707,0) and (-.707,-.707,0). This can
    // reek havoc in hyperstreamline/other stuff. We will select the most
    // positive eigenvector.
    int numPos;
    for (j = 0; j < 3; j++)
    {
        for (numPos = 0, i = 0; i < 3; i++)
            if (v[i][j] >= 0.0)
                numPos++;
        if (numPos < 2)
            for (i = 0; i < 3; i++)
                v[i][j] *= -1.0;
    }

    return true;
}

#undef ROT
#undef MAX_ROTATIONS

bool jacobi(const Mat3& m, Vec3& eig_vals, Vec3 eig_vecs[3])
{
    double a[3][3], w[3], v[3][3];
    int i, j;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            a[i][j] = m(i, j);

    bool result = internal_jacobi(a, w, v);
    if (result)
    {
        for (i = 0; i < 3; i++)
            eig_vals[i] = w[i];

        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++)
                eig_vecs[i][j] = v[j][i];
    }

    return result;
}

bool jacobi(const Mat3& m, double* eig_vals, double* eig_vecs)
{
    double a[3][3], v[3][3];
    int i, j;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            a[i][j] = m(i, j);

    bool result = internal_jacobi(a, eig_vals, v);
    if (result)
    {
        int index = 0;
        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++)
                // eig_vecs[i][j] = v[j][i];
                eig_vecs[index++] = v[j][i];
    }

    return result;
}
