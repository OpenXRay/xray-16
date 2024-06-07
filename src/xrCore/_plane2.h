#pragma once

class Fplane2
{
public:
    using TYPE = float;
    using Self = Fplane2;
    using SelfRef = Self&;
    using SelfCRef = const Self&;

public:
    Fvector2 n;
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
    IC SelfRef build(const Fvector2& _p, const Fvector2& _n)
    {
        d = -n.normalize(_n).dotproduct(_p);
        return *this;
    }
    IC SelfRef project(Fvector2& pdest, Fvector2& psrc)
    {
        pdest.mad(psrc, n, -classify(psrc));
        return *this;
    }
    IC float classify(const Fvector2& v) const { return n.dotproduct(v) + d; }
    IC SelfRef normalize()
    {
        float denom = 1.f / n.magnitude();
        n.mul(denom);
        d *= denom;
        return *this;
    }
    IC float distance(const Fvector2& v) { return _abs(classify(v)); }
    IC BOOL intersectRayDist(const Fvector2& P, const Fvector2& D, float& dist)
    {
        float numer = classify(P);
        float denom = n.dotproduct(D);

        if (_abs(denom) < EPS_S) // normal is orthogonal to vector3, cant intersect
            return FALSE;

        dist = -(numer / denom);
        return ((dist > 0.f) || fis_zero(dist));
    }
    IC BOOL intersectRayPoint(const Fvector2& P, const Fvector2& D, Fvector2& dest)
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
    IC BOOL intersect(const Fvector2& u, const Fvector2& v, // segment
        Fvector2& isect) // intersection point
    {
        float denom, dist;
        Fvector2 t;

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

    IC BOOL intersect_2(const Fvector2& u, const Fvector2& v, // segment
        Fvector2& isect) // intersection point
    {
        float dist1, dist2;
        Fvector2 t;

        dist1 = n.dotproduct(u) + d;
        dist2 = n.dotproduct(v) + d;

        if (dist1 * dist2 < 0.0f)
            return false;

        t.sub(v, u);
        isect.mad(u, t, dist1 / _abs(dist1 - dist2));

        return true;
    }
};

inline bool _valid(const Fplane2& s)
{
    return _valid(s.n) && _valid(s.d);
}
