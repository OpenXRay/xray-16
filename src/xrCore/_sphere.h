#pragma once

#include "_vector3d.h"

struct Fsphere
{
    Fvector3 P;
    float R;

public:
    void set(const Fvector3& _P, float _R)
    {
        P.set(_P);
        R = _R;
    }

    void set(const Fsphere& S)
    {
        P.set(S.P);
        R = S.R;
    }

    void identity()
    {
        P.set(0, 0, 0);
        R = 1;
    }

    enum ERP_Result : u32
    {
        rpNone = 0,
        rpOriginInside = 1,
        rpOriginOutside = 2,
    };

    // Ray-sphere intersection
    ICF ERP_Result intersect(const Fvector3& S, const Fvector3& D, float range, int& quantity, float afT[2]) const
    {
        // set up quadratic Q(t) = a*t^2 + 2*b*t + c
        Fvector3 kDiff;
        kDiff.sub(S, P);
        float fA = range * range;
        const float fB = kDiff.dotproduct(D) * range;
        const float fC = kDiff.square_magnitude() - R * R;
        ERP_Result result = rpNone;

        const float fDiscr = fB * fB - fA * fC;
        if (fDiscr < 0.0f)
        {
            quantity = 0;
        }
        else if (fDiscr > 0.0f)
        {
            const float fRoot = _sqrt(fDiscr);
            const float fInvA = (1.0f) / fA;
            afT[0] = range * (-fB - fRoot) * fInvA;
            afT[1] = range * (-fB + fRoot) * fInvA;
            if (afT[0] >= 0.0f)
            {
                quantity = 2;
                result = rpOriginOutside;
            }
            else if (afT[1] >= 0.0f)
            {
                quantity = 1;
                afT[0] = afT[1];
                result = rpOriginInside;
            }
            else
                quantity = 0;
        }
        else
        {
            afT[0] = range * (-fB / fA);
            if (afT[0] >= 0.0f)
            {
                quantity = 1;
                result = rpOriginOutside;
            }
            else
                quantity = 0;
        }
        return result;
    }
    /*
     int quantity;
     float afT[2];
     Fsphere::ERP_Result result = sS.intersect(ray.pos,ray.fwd_dir,range,quantity,afT);

     if (Fsphere::rpOriginInside || ((result==Fsphere::rpOriginOutside)&&(afT[0]<range))){
     if (b_nearest) {
     switch(result){
     case Fsphere::rpOriginInside: range = afT[0]<range?afT[0]:range; break;
     case Fsphere::rpOriginOutside: range = afT[0]; break;
     }
     range2 =range*range;
     }
     */
    ICF ERP_Result intersect_full(const Fvector3& start, const Fvector3& dir, float& dist) const
    {
        int quantity;
        float afT[2];
        typename Fsphere::ERP_Result result = intersect(start, dir, dist, quantity, afT);

        if ((result == Fsphere::rpOriginInside) || ((result == Fsphere::rpOriginOutside) && (afT[0] < dist)))
        {
            switch (result)
            {
            case Fsphere::rpOriginInside: dist = afT[0] < dist ? afT[0] : dist; break;
            case Fsphere::rpOriginOutside: dist = afT[0]; break;
            }
        }
        return result;
    }

    ICF ERP_Result intersect(const Fvector3& start, const Fvector3& dir, float& dist) const
    {
        int quantity;
        float afT[2];
        ERP_Result result = intersect(start, dir, dist, quantity, afT);
        if (rpNone != result)
        {
            VERIFY(quantity > 0);
            if (afT[0] < dist)
            {
                dist = afT[0];
                return result;
            }
        }
        return rpNone;
    }

    ERP_Result intersect2(const Fvector3& S, const Fvector3& D, float& range) const
    {
        Fvector3 Q;
        Q.sub(P, S);

        float R2 = R * R;
        float c2 = Q.square_magnitude();
        float v = Q.dotproduct(D);
        float d = R2 - (c2 - v * v);

        if (d > 0.f)
        {
            float _range = v - _sqrt(d);
            if (_range < range)
            {
                range = _range;
                return (c2 < R2) ? rpOriginInside : rpOriginOutside;
            }
        }
        return rpNone;
    }

    ICF BOOL intersect(const Fvector3& S, const Fvector3& D) const
    {
        Fvector3 Q;
        Q.sub(P, S);

        float c = Q.magnitude();
        float v = Q.dotproduct(D);
        float d = R * R - (c * c - v * v);
        return (d > 0);
    }

    ICF BOOL intersect(const Fsphere& S) const
    {
        float SumR = R + S.R;
        return P.distance_to_sqr(S.P) < SumR * SumR;
    }

    BOOL contains(const Fvector3& PT) const { return P.distance_to_sqr(PT) <= (R * R + EPS_S); }

    // returns true if this wholly contains the argument sphere
    BOOL contains(const Fsphere& S) const
    {
        // can't contain a sphere that's bigger than me !
        const float RDiff = R - S.R;
        if (RDiff < 0)
            return false;

        return (P.distance_to_sqr(S.P) <= RDiff * RDiff);
    }

    // return's volume of sphere
    float volume() const { return (PI_MUL_4 / 3.f) * (R * R * R); }
};

inline bool _valid(const Fsphere& s)
{
    return _valid(s.P) && _valid(s.R);
}

void XRCORE_API Fsphere_compute(Fsphere& dest, const Fvector* verts, int count);
