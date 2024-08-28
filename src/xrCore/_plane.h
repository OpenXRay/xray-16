#pragma once

#include "_vector3d.h"
#include "_matrix.h"

class Fplane
{
public:
    using TYPE     = float;
    using Self     = Fplane;
    using SelfRef  = Self&;
    using SelfCRef = const Self&;

public:
    Fvector3 n;
    float d;

public:
    IC SelfRef set(Self& P)
    {
        n.set(P.n);
        d = P.d;
        return *this;
    }
    IC BOOL similar(Self& P, float eps_n = EPS, float eps_d = EPS)
    {
        return (n.similar(P.n, eps_n) && (_abs(d - P.d) < eps_d));
    }
    ICF SelfRef build(const Fvector3& v1, const Fvector3& v2, const Fvector3& v3)
    {
        Fvector3 t1, t2;
        n.crossproduct(t1.sub(v1, v2), t2.sub(v1, v3)).normalize();
        d = -n.dotproduct(v1);
        return *this;
    }
    ICF SelfRef build_precise(const Fvector3& v1, const Fvector3& v2, const Fvector3& v3)
    {
        Fvector3 t1, t2;
        n.crossproduct(t1.sub(v1, v2), t2.sub(v1, v3));
        exact_normalize(n);
        d = -n.dotproduct(v1);
        return *this;
    }
    ICF SelfRef build(const Fvector3& _p, const Fvector3& _n)
    {
        d = -n.normalize(_n).dotproduct(_p);
        return *this;
    }
    ICF SelfRef build_unit_normal(const Fvector3& _p, const Fvector3& _n)
    {
        VERIFY(fsimilar(_n.magnitude(), 1, EPS));
        d = -n.set(_n).dotproduct(_p);
        return *this;
    }
    IC SelfCRef project(Fvector3& pdest, Fvector3 const& psrc) const
    {
        pdest.mad(psrc, n, -classify(psrc));
        return *this;
    }
    IC SelfRef project(Fvector3& pdest, Fvector3 const& psrc)
    {
        pdest.mad(psrc, n, -classify(psrc));
        return *this;
    }
    ICF float classify(const Fvector3& v) const { return n.dotproduct(v) + d; }
    IC SelfRef normalize()
    {
        float denom = 1.f / n.magnitude();
        n.mul(denom);
        d *= denom;
        return *this;
    }
    IC float distance(const Fvector3& v) { return _abs(classify(v)); }
    IC BOOL intersectRayDist(const Fvector3& P, const Fvector3& D, float& dist)
    {
        float numer = classify(P);
        float denom = n.dotproduct(D);

        if (_abs(denom) < EPS_S) // normal is orthogonal to vector3, cant intersect
            return FALSE;

        dist = -(numer / denom);
        return ((dist > 0.f) || fis_zero(dist));
    }
    ICF BOOL intersectRayPoint(const Fvector3& P, const Fvector3& D, Fvector3& dest)
    {
        float numer = classify(P);
        float denom = n.dotproduct(D);

        if (_abs(denom) < EPS_S)
            return FALSE; // normal is orthogonal to vector3, cant intersect
        else
        {
            float dist = -(numer / denom);
            dest.mad(P, D, dist);
            return ((dist > 0.f) || fis_zero(dist));
        }
    }
    IC BOOL intersect(const Fvector3& u, const Fvector3& v, // segment
        Fvector3& isect) // intersection point
    {
        float denom, dist;
        Fvector3 t;

        t.sub(v, u);
        denom = n.dotproduct(t);
        if (_abs(denom) < EPS)
            return false; // they are parallel

        dist = -(n.dotproduct(u) + d) / denom;
        if (dist < -EPS || dist > 1 + EPS)
            return false;
        isect.mad(u, t, dist);
        return true;
    }

    IC BOOL intersect_2(const Fvector3& u, const Fvector3& v, // segment
        Fvector3& isect) // intersection point
    {
        float dist1, dist2;
        Fvector3 t;

        dist1 = n.dotproduct(u) + d;
        dist2 = n.dotproduct(v) + d;

        if (dist1 * dist2 < 0.0f)
            return false;

        t.sub(v, u);
        isect.mad(u, t, dist1 / _abs(dist1 - dist2));

        return true;
    }
    IC SelfRef transform(Fmatrix& M)
    {
        // rotate the normal
        M.transform_dir(n);
        // slide the offset
        d -= M.c.dotproduct(n);
        return *this;
    }
};

inline bool _valid(const Fplane& s)
{
    return _valid(s.n) && _valid(s.d);
}
