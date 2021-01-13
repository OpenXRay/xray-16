/************************************************************************

  Jacobi iteration on 4x4 matrices.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  NOTE: This code was adapted from VTK source code (vtkMath.cxx) which
        seems to have been adapted directly from Numerical Recipes in C.
    See Hoppe's Recon software for an alternative adaptation of the
    Numerical Recipes code.

  $Id: MxMat4-jacobi.cxx,v 1.6 1999/01/22 17:24:47 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxMat4.h"
#include "xrCore/_std_extensions.h"

#define ROT(a, i, j, k, l)\
    g = a[i][j];\
    h = a[k][l];\
    a[i][j] = g - s * (h + g * tau);\
    a[k][l] = h + s * (g - h * tau);

#define MAX_ROTATIONS 60

static bool internal_jacobi4(double a[4][4], double w[4], double v[4][4])
{
    int i, j, k, iq, ip;
    double tresh, theta, tau, t, sm, s, h, g, c;
    double b[4], z[4], tmp;

    // initialize
    for (ip = 0; ip < 4; ip++)
    {
        for (iq = 0; iq < 4; iq++)
            v[ip][iq] = 0.0;
        v[ip][ip] = 1.0;
    }
    for (ip = 0; ip < 4; ip++)
    {
        b[ip] = w[ip] = a[ip][ip];
        z[ip] = 0.0;
    }

    // begin rotation sequence
    for (i = 0; i < MAX_ROTATIONS; i++)
    {
        sm = 0.0;
        for (ip = 0; ip < 3; ip++)
        {
            for (iq = ip + 1; iq < 4; iq++)
                sm += _abs(a[ip][iq]);
        }
        if (sm == 0.0)
            break;

        if (i < 4)
            tresh = 0.2 * sm / (16);
        else
            tresh = 0.0;

        for (ip = 0; ip < 3; ip++)
        {
            for (iq = ip + 1; iq < 4; iq++)
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
                    for (j = iq + 1; j < 4; j++)
                    {
                        ROT(a, ip, j, iq, j)
                    }
                    for (j = 0; j < 4; j++)
                    {
                        ROT(v, j, ip, j, iq)
                    }
                }
            }
        }

        for (ip = 0; ip < 4; ip++)
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
    for (j = 0; j < 4; j++)
    {
        k = j;
        tmp = w[k];
        for (i = j; i < 4; i++)
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
            for (i = 0; i < 4; i++)
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
    for (j = 0; j < 4; j++)
    {
        for (numPos = 0, i = 0; i < 4; i++)
            if (v[i][j] >= 0.0)
                numPos++;
        if (numPos < 3)
            for (i = 0; i < 4; i++)
                v[i][j] *= -1.0;
    }

    return true;
}

#undef ROT
#undef MAX_ROTATIONS

bool jacobi(const Mat4& m, Vec4& eig_vals, Vec4 eig_vecs[4])
{
    double a[4][4], w[4], v[4][4];
    int i, j;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            a[i][j] = m(i, j);

    bool result = internal_jacobi4(a, w, v);
    if (result)
    {
        for (i = 0; i < 4; i++)
            eig_vals[i] = w[i];

        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                eig_vecs[i][j] = v[j][i];
    }

    return result;
}
