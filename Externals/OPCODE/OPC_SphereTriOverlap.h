
// Original code by David Eberly in Magic.
BOOL SphereCollider::SphereTriOverlap(const Point& vert0, const Point& vert1, const Point& vert2)
{
    // Stats
    mNbVolumePrimTests++;

    Point TriEdge0 = vert1 - vert0;
    Point TriEdge1 = vert2 - vert0;

    Point kDiff = vert0 - mCenter;
    float fA00 = TriEdge0.SquareMagnitude();
    float fA01 = TriEdge0 | TriEdge1;
    float fA11 = TriEdge1.SquareMagnitude();
    float fB0 = kDiff | TriEdge0;
    float fB1 = kDiff | TriEdge1;
    float fC = kDiff.SquareMagnitude();
    float fDet = _abs(fA00 * fA11 - fA01 * fA01);
    float u = fA01 * fB1 - fA11 * fB0;
    float v = fA01 * fB0 - fA00 * fB1;
    float SqrDist;

    if (u + v <= fDet)
    {
        if (u < 0.0f)
        {
            if (v < 0.0f) // region 4
            {
                if (fB0 < 0.0f)
                {
                    v = 0.0f;
                    if (-fB0 >= fA00)
                    {
                        u = 1.0f;
                        SqrDist = fA00 + 2.0f * fB0 + fC;
                    }
                    else
                    {
                        u = -fB0 / fA00;
                        SqrDist = fB0 * u + fC;
                    }
                }
                else
                {
                    u = 0.0f;
                    if (fB1 >= 0.0f)
                    {
                        v = 0.0f;
                        SqrDist = fC;
                    }
                    else if (-fB1 >= fA11)
                    {
                        v = 1.0f;
                        SqrDist = fA11 + 2.0f * fB1 + fC;
                    }
                    else
                    {
                        v = -fB1 / fA11;
                        SqrDist = fB1 * v + fC;
                    }
                }
            }
            else // region 3
            {
                u = 0.0f;
                if (fB1 >= 0.0f)
                {
                    v = 0.0f;
                    SqrDist = fC;
                }
                else if (-fB1 >= fA11)
                {
                    v = 1.0f;
                    SqrDist = fA11 + 2.0f * fB1 + fC;
                }
                else
                {
                    v = -fB1 / fA11;
                    SqrDist = fB1 * v + fC;
                }
            }
        }
        else if (v < 0.0f) // region 5
        {
            v = 0.0f;
            if (fB0 >= 0.0f)
            {
                u = 0.0f;
                SqrDist = fC;
            }
            else if (-fB0 >= fA00)
            {
                u = 1.0f;
                SqrDist = fA00 + 2.0f * fB0 + fC;
            }
            else
            {
                u = -fB0 / fA00;
                SqrDist = fB0 * u + fC;
            }
        }
        else // region 0
        {
            // minimum at interior point
            if (fDet == 0.0f)
            {
                u = 0.0f;
                v = 0.0f;
                SqrDist = flt_max;
            }
            else
            {
                float fInvDet = 1.0f / fDet;
                u *= fInvDet;
                v *= fInvDet;
                SqrDist = u * (fA00 * u + fA01 * v + 2.0f * fB0) + v * (fA01 * u + fA11 * v + 2.0f * fB1) + fC;
            }
        }
    }
    else
    {
        float fTmp0, fTmp1, fNumer, fDenom;

        if (u < 0.0f) // region 2
        {
            fTmp0 = fA01 + fB0;
            fTmp1 = fA11 + fB1;
            if (fTmp1 > fTmp0)
            {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00 - 2.0f * fA01 + fA11;
                if (fNumer >= fDenom)
                {
                    u = 1.0f;
                    v = 0.0f;
                    SqrDist = fA00 + 2.0f * fB0 + fC;
                }
                else
                {
                    u = fNumer / fDenom;
                    v = 1.0f - u;
                    SqrDist = u * (fA00 * u + fA01 * v + 2.0f * fB0) + v * (fA01 * u + fA11 * v + 2.0f * fB1) + fC;
                }
            }
            else
            {
                u = 0.0f;
                if (fTmp1 <= 0.0f)
                {
                    v = 1.0f;
                    SqrDist = fA11 + 2.0f * fB1 + fC;
                }
                else if (fB1 >= 0.0f)
                {
                    v = 0.0f;
                    SqrDist = fC;
                }
                else
                {
                    v = -fB1 / fA11;
                    SqrDist = fB1 * v + fC;
                }
            }
        }
        else if (v < 0.0f) // region 6
        {
            fTmp0 = fA01 + fB1;
            fTmp1 = fA00 + fB0;
            if (fTmp1 > fTmp0)
            {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00 - 2.0f * fA01 + fA11;
                if (fNumer >= fDenom)
                {
                    v = 1.0f;
                    u = 0.0f;
                    SqrDist = fA11 + 2.0f * fB1 + fC;
                }
                else
                {
                    v = fNumer / fDenom;
                    u = 1.0f - v;
                    SqrDist = u * (fA00 * u + fA01 * v + 2.0f * fB0) + v * (fA01 * u + fA11 * v + 2.0f * fB1) + fC;
                }
            }
            else
            {
                v = 0.0f;
                if (fTmp1 <= 0.0f)
                {
                    u = 1.0f;
                    SqrDist = fA00 + 2.0f * fB0 + fC;
                }
                else if (fB0 >= 0.0f)
                {
                    u = 0.0f;
                    SqrDist = fC;
                }
                else
                {
                    u = -fB0 / fA00;
                    SqrDist = fB0 * u + fC;
                }
            }
        }
        else // region 1
        {
            fNumer = fA11 + fB1 - fA01 - fB0;
            if (fNumer <= 0.0f)
            {
                u = 0.0f;
                v = 1.0f;
                SqrDist = fA11 + 2.0f * fB1 + fC;
            }
            else
            {
                fDenom = fA00 - 2.0f * fA01 + fA11;
                if (fNumer >= fDenom)
                {
                    u = 1.0f;
                    v = 0.0f;
                    SqrDist = fA00 + 2.0f * fB0 + fC;
                }
                else
                {
                    u = fNumer / fDenom;
                    v = 1.0f - u;
                    SqrDist = u * (fA00 * u + fA01 * v + 2.0f * fB0) + v * (fA01 * u + fA11 * v + 2.0f * fB1) + fC;
                }
            }
        }
    }

    return _abs(SqrDist) < mRadius2;
}
