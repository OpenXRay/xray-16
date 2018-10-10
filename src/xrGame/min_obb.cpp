#include "StdAfx.h"
#include "magic_box3.h"
#include "magic_minimize_nd.h"

class PointArray
{
public:
    PointArray(int iQuantity, const Fvector* akPoint) : m_akPoint(akPoint) { m_iQuantity = iQuantity; }
    int m_iQuantity;
    const Fvector* m_akPoint;
};

static void FromAxisAngle(Fmatrix& self, const Fvector& rkAxis, float fRadians)
{
    float fCos = _cos(fRadians);
    float fSin = _sin(fRadians);
    float fOneMinusCos = 1.0f - fCos;
    float fX2 = rkAxis.x * rkAxis.x;
    float fY2 = rkAxis.y * rkAxis.y;
    float fZ2 = rkAxis.z * rkAxis.z;
    float fXYM = rkAxis.x * rkAxis.y * fOneMinusCos;
    float fXZM = rkAxis.x * rkAxis.z * fOneMinusCos;
    float fYZM = rkAxis.y * rkAxis.z * fOneMinusCos;
    float fXSin = rkAxis.x * fSin;
    float fYSin = rkAxis.y * fSin;
    float fZSin = rkAxis.z * fSin;

    self.identity();
    self._11 = fX2 * fOneMinusCos + fCos;
    self._12 = fXYM - fZSin;
    self._13 = fXZM + fYSin;
    self._21 = fXYM + fZSin;
    self._22 = fY2 * fOneMinusCos + fCos;
    self._23 = fYZM - fXSin;
    self._31 = fXZM - fYSin;
    self._32 = fYZM + fXSin;
    self._33 = fZ2 * fOneMinusCos + fCos;
}

static Fvector GetColumn(Fmatrix& self, const u32& index)
{
    switch (index)
    {
    case 0: return (Fvector().set(self._11, self._21, self._31));
    case 1: return (Fvector().set(self._12, self._22, self._32));
    case 2: return (Fvector().set(self._13, self._23, self._33));
    default: NODEFAULT;
    }
#ifdef DEBUG
    return (Fvector().set(flt_max, flt_max, flt_max));
#endif // DEBUG
}

//----------------------------------------------------------------------------
static float Volume(const float* afAngle, void* pvUserData)
{
    int iQuantity = ((PointArray*)pvUserData)->m_iQuantity;
    const Fvector* akPoint = ((PointArray*)pvUserData)->m_akPoint;

    float fCos0 = _cos(afAngle[0]);
    float fSin0 = _sin(afAngle[0]);
    float fCos1 = _cos(afAngle[1]);
    float fSin1 = _sin(afAngle[1]);
    Fvector kAxis = Fvector().set(fCos0 * fSin1, fSin0 * fSin1, fCos1);
    Fmatrix kRot;
    FromAxisAngle(kRot, kAxis, afAngle[2]);

    Fvector kMin;
    kRot.transform_tiny(kMin, akPoint[0]);
    Fvector kMax = kMin;
    for (int i = 1; i < iQuantity; i++)
    {
        Fvector kTest;
        kRot.transform_tiny(kTest, akPoint[i]);

        if (kTest.x < kMin.x)
            kMin.x = kTest.x;
        else if (kTest.x > kMax.x)
            kMax.x = kTest.x;

        if (kTest.y < kMin.y)
            kMin.y = kTest.y;
        else if (kTest.y > kMax.y)
            kMax.y = kTest.y;

        if (kTest.z < kMin.z)
            kMin.z = kTest.z;
        else if (kTest.z > kMax.z)
            kMax.z = kTest.z;
    }

    float fVolume = (kMax.x - kMin.x) * (kMax.y - kMin.y) * (kMax.z - kMin.z);
    return fVolume;
}
//----------------------------------------------------------------------------
static void MinimalBoxForAngles(int iQuantity, const Fvector* akPoint, float afAngle[3], MagicBox3& rkBox)
{
    float fCos0 = _cos(afAngle[0]);
    float fSin0 = _sin(afAngle[0]);
    float fCos1 = _cos(afAngle[1]);
    float fSin1 = _sin(afAngle[1]);
    Fvector kAxis = Fvector().set(fCos0 * fSin1, fSin0 * fSin1, fCos1);
    Fmatrix kRot;
    FromAxisAngle(kRot, kAxis, afAngle[2]);

    Fvector kMin;
    kRot.transform_tiny(kMin, akPoint[0]);
    Fvector kMax = kMin;
    for (int i = 1; i < iQuantity; i++)
    {
        Fvector kTest;
        kRot.transform_tiny(kTest, akPoint[i]);

        if (kTest.x < kMin.x)
            kMin.x = kTest.x;
        else if (kTest.x > kMax.x)
            kMax.x = kTest.x;

        if (kTest.y < kMin.y)
            kMin.y = kTest.y;
        else if (kTest.y > kMax.y)
            kMax.y = kTest.y;

        if (kTest.z < kMin.z)
            kMin.z = kTest.z;
        else if (kTest.z > kMax.z)
            kMax.z = kTest.z;
    }

    Fvector kMid = Fvector().add(kMax, kMin).mul(0.5f);
    Fvector kRng = Fvector().sub(kMax, kMin).mul(0.5f);

    kRot.transform_tiny(rkBox.Center(), kMid);
    rkBox.Axis(0) = GetColumn(kRot, 0);
    rkBox.Axis(1) = GetColumn(kRot, 1);
    rkBox.Axis(2) = GetColumn(kRot, 2);
    rkBox.Extent(0) = kRng.x;
    rkBox.Extent(1) = kRng.y;
    rkBox.Extent(2) = kRng.z;
}
//----------------------------------------------------------------------------
MagicBox3 MagicMinBox(int iQuantity, const Fvector* akPoint)
{
    int iMaxLevel = 8;
    int iMaxBracket = 8;
    int iMaxIterations = 32;
    PointArray kPA(iQuantity, akPoint);
    MinimizeND<3> kMinimizer(Volume, iMaxLevel, iMaxBracket, iMaxIterations, &kPA);

    float afA0[3] = {0.0f, 0.0f, 0.0f};

    float afA1[3] = {PI, PI_DIV_2, PI};

    // compute some samples to narrow down the search region
    float fMinVolume = flt_max;
    float afAngle[3], afAInitial[3];
    const int iMax = 3;
    for (int i0 = 0; i0 <= iMax; i0++)
    {
        afAngle[0] = afA0[0] + i0 * (afA1[0] - afA0[0]) / iMax;
        for (int i1 = 0; i1 <= iMax; i1++)
        {
            afAngle[1] = afA0[1] + i1 * (afA1[1] - afA0[1]) / iMax;
            for (int i2 = 0; i2 <= iMax; i2++)
            {
                afAngle[2] = afA0[2] + i2 * (afA1[2] - afA0[2]) / iMax;
                float fVolume = Volume(afAngle, &kPA);
                if (fVolume < fMinVolume)
                {
                    fMinVolume = fVolume;
                    afAInitial[0] = afAngle[0];
                    afAInitial[1] = afAngle[1];
                    afAInitial[2] = afAngle[2];
                }
            }
        }
    }

    float afAMin[3], fVMin;
    kMinimizer.GetMinimum(afA0, afA1, afAInitial, afAMin, fVMin);

    MagicBox3 kBox;
    MinimalBoxForAngles(iQuantity, akPoint, afAMin, kBox);
    return kBox;
}
//----------------------------------------------------------------------------
